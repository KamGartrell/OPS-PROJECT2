// SPDX-License-Identifier: GPL-2.0
/*
 * elevator.c - pet elevator kernel module (COP4610 Project 2, Part 3)
 *
 * - The three system calls (548-550) live in the kernel (syscalls.c) and
 *   dispatch through the STUB_* function pointers that this module fills in
 *   on load and clears on unload.
 * - One kthread, created on insmod, runs the elevator. While the elevator is
 *   OFFLINE it just sleeps and polls; start_elevator() wakes it up by
 *   changing the state.
 * - Every piece of shared data (floor lists, rider list, state, counters) is
 *   protected by elev_lock. The lock is never held while sleeping.
 * - Scheduling: SCAN. Sweep up to floor 5, reverse, sweep down to floor 1,
 *   and repeat, stopping at every floor where a rider gets off or a pet
 *   going the same way can get on. The elevator only sits IDLE when there
 *   is nothing to carry and nothing waiting. Boarding is FIFO per floor.
 * - /proc/elevator shows the state of the building.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("GROUP 6");
MODULE_DESCRIPTION("COP4610 Project 2 Part 3: Pet Elevator Kernel Module");
MODULE_VERSION("1.0");

#define PROC_NAME	"elevator"
#define NUM_FLOORS	5
#define MAX_PETS	5
#define MAX_WEIGHT	50
#define NUM_TYPES	4
#define MOVE_MS		2000	/* time to travel between two floors */
#define LOAD_MS		1000	/* time to load/unload at a floor */
#define POLL_MS		100	/* how often an idle/offline elevator re-checks */

#define DIR_UP		1
#define DIR_DOWN	(-1)

/* Hooks defined in the kernel's syscalls/syscalls.c */
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

enum elevator_state { OFFLINE, IDLE, LOADING, UP, DOWN };

static const char * const state_names[] = {
	"OFFLINE", "IDLE", "LOADING", "UP", "DOWN"
};

/* Indexed by pet type: 0 = Chihuahua, 1 = Pug, 2 = Pughuahua, 3 = Dachshund */
static const int pet_weight[NUM_TYPES] = { 3, 14, 10, 16 };
static const char pet_letter[NUM_TYPES] = { 'C', 'P', 'H', 'D' };

struct pet {
	struct list_head list;
	int type;
	int start_floor;
	int dest_floor;
};

/* ---- shared data, all protected by elev_lock ---- */
static DEFINE_MUTEX(elev_lock);
static struct list_head floors[NUM_FLOORS + 1];	/* waiting pets, [1..5] */
static LIST_HEAD(riders);			/* pets inside the elevator */
static enum elevator_state elev_state = OFFLINE;
static int cur_floor = 1;
static int elev_dir = DIR_UP;
static int load_weight;
static int num_riders;
static int num_waiting;
static int num_serviced;
static bool stopping;				/* stop requested, finishing up */
/* ------------------------------------------------- */

static struct task_struct *elev_thread;
static struct proc_dir_entry *elev_proc;

static int pet_direction(const struct pet *p)
{
	return p->dest_floor > p->start_floor ? DIR_UP : DIR_DOWN;
}

/* Can this waiting pet (head of the current floor's line) board right now? */
static bool can_board(const struct pet *p)
{
	if (stopping)
		return false;
	if (num_riders >= MAX_PETS)
		return false;
	if (load_weight + pet_weight[p->type] > MAX_WEIGHT)
		return false;
	/* SCAN: only take pets that travel in the current sweep direction */
	if (pet_direction(p) != elev_dir)
		return false;
	return true;
}

/* Does the elevator need to stop on this floor to unload or load? */
static bool should_stop_here(void)
{
	struct pet *p;

	list_for_each_entry(p, &riders, list) {
		if (p->dest_floor == cur_floor)
			return true;
	}
	if (!list_empty(&floors[cur_floor])) {
		p = list_first_entry(&floors[cur_floor], struct pet, list);
		return can_board(p);
	}
	return false;
}

static void unload_pets(void)
{
	struct pet *p, *tmp;

	list_for_each_entry_safe(p, tmp, &riders, list) {
		if (p->dest_floor != cur_floor)
			continue;
		list_del(&p->list);
		load_weight -= pet_weight[p->type];
		num_riders--;
		num_serviced++;
		kfree(p);
	}
}

static void load_pets(void)
{
	struct pet *p;

	while (!list_empty(&floors[cur_floor])) {
		p = list_first_entry(&floors[cur_floor], struct pet, list);
		if (!can_board(p))
			break;
		list_move_tail(&p->list, &riders);
		num_riders++;
		load_weight += pet_weight[p->type];
		num_waiting--;
	}
}

/* At the top or bottom floor the sweep turns around. */
static void turn_at_end_of_building(void)
{
	if (elev_dir == DIR_UP && cur_floor == NUM_FLOORS)
		elev_dir = DIR_DOWN;
	else if (elev_dir == DIR_DOWN && cur_floor == 1)
		elev_dir = DIR_UP;
}

/*
 * SCAN: keep sweeping in the current direction. Returns the direction to
 * move, or 0 when there is nothing to carry and nothing to pick up.
 */
static int choose_direction(void)
{
	if (num_riders == 0 && (stopping || num_waiting == 0))
		return 0;
	return elev_dir;
}

static int elevator_thread_fn(void *data)
{
	int dir;

	while (!kthread_should_stop()) {
		mutex_lock(&elev_lock);

		if (elev_state == OFFLINE) {
			mutex_unlock(&elev_lock);
			msleep(POLL_MS);
			continue;
		}

		/* 1. Unload and load at this floor (1 second) */
		turn_at_end_of_building();
		if (should_stop_here()) {
			elev_state = LOADING;
			unload_pets();
			load_pets();
			mutex_unlock(&elev_lock);
			msleep(LOAD_MS);
			continue;
		}

		/* 2. Move one floor (2 seconds) */
		dir = choose_direction();
		if (dir != 0) {
			elev_state = (dir == DIR_UP) ? UP : DOWN;
			mutex_unlock(&elev_lock);
			msleep(MOVE_MS);
			mutex_lock(&elev_lock);
			cur_floor += dir;
			mutex_unlock(&elev_lock);
			continue;
		}

		/* 3. Nothing to do */
		if (stopping && elev_state == IDLE) {
			elev_state = OFFLINE;	/* finished stopping */
			stopping = false;
			mutex_unlock(&elev_lock);
			continue;
		}
		if (stopping) {
			elev_state = IDLE;	/* show IDLE briefly, then OFFLINE */
			mutex_unlock(&elev_lock);
			msleep(LOAD_MS);
			continue;
		}
		elev_state = IDLE;
		mutex_unlock(&elev_lock);
		msleep(POLL_MS);
	}
	return 0;
}

/* ---------------- system call implementations ---------------- */

static int elevator_start(void)
{
	int ret = 0;

	if (mutex_lock_interruptible(&elev_lock))
		return -ERESTARTSYS;
	if (elev_state != OFFLINE) {
		ret = 1;		/* already active (or still stopping) */
	} else {
		elev_state = IDLE;
		cur_floor = 1;
		elev_dir = DIR_UP;
		stopping = false;
	}
	mutex_unlock(&elev_lock);
	return ret;
}

static int elevator_issue_request(int start_floor, int destination_floor,
				  int type)
{
	struct pet *p;

	if (start_floor < 1 || start_floor > NUM_FLOORS ||
	    destination_floor < 1 || destination_floor > NUM_FLOORS ||
	    start_floor == destination_floor ||
	    type < 0 || type >= NUM_TYPES)
		return 1;

	p = kmalloc(sizeof(*p), GFP_KERNEL);
	if (!p)
		return -ENOMEM;
	p->type = type;
	p->start_floor = start_floor;
	p->dest_floor = destination_floor;

	if (mutex_lock_interruptible(&elev_lock)) {
		kfree(p);
		return -ERESTARTSYS;
	}
	list_add_tail(&p->list, &floors[start_floor]);
	num_waiting++;
	mutex_unlock(&elev_lock);
	return 0;
}

static int elevator_stop(void)
{
	int ret = 0;

	if (mutex_lock_interruptible(&elev_lock))
		return -ERESTARTSYS;
	if (stopping)
		ret = 1;		/* already deactivating */
	else if (elev_state != OFFLINE)
		stopping = true;	/* thread drains the riders, then goes OFFLINE */
	mutex_unlock(&elev_lock);
	return ret;
}

/* ---------------------- /proc/elevator ---------------------- */

static int elevator_show(struct seq_file *m, void *v)
{
	struct pet *p;
	int f, count;

	if (mutex_lock_interruptible(&elev_lock))
		return -ERESTARTSYS;

	seq_printf(m, "Elevator state: %s\n", state_names[elev_state]);
	seq_printf(m, "Current floor: %d\n", cur_floor);
	seq_printf(m, "Current load: %d lbs\n", load_weight);
	seq_puts(m, "Elevator status:");
	list_for_each_entry(p, &riders, list)
		seq_printf(m, " %c%d", pet_letter[p->type], p->dest_floor);
	seq_putc(m, '\n');

	for (f = NUM_FLOORS; f >= 1; f--) {
		count = 0;
		list_for_each_entry(p, &floors[f], list)
			count++;
		seq_printf(m, "[%c] Floor %d: %d", f == cur_floor ? '*' : ' ',
			   f, count);
		list_for_each_entry(p, &floors[f], list)
			seq_printf(m, " %c%d", pet_letter[p->type], p->dest_floor);
		seq_putc(m, '\n');
	}

	seq_printf(m, "Number of pets: %d\n", num_riders);
	seq_printf(m, "Number of pets waiting: %d\n", num_waiting);
	seq_printf(m, "Number of pets serviced: %d\n", num_serviced);

	mutex_unlock(&elev_lock);
	return 0;
}

static int elevator_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, elevator_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
	.proc_open	= elevator_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

/* ------------------------ module setup ------------------------ */

static void free_all_pets(void)
{
	struct pet *p, *tmp;
	int f;

	mutex_lock(&elev_lock);
	list_for_each_entry_safe(p, tmp, &riders, list) {
		list_del(&p->list);
		kfree(p);
	}
	for (f = 1; f <= NUM_FLOORS; f++) {
		list_for_each_entry_safe(p, tmp, &floors[f], list) {
			list_del(&p->list);
			kfree(p);
		}
	}
	num_riders = 0;
	num_waiting = 0;
	load_weight = 0;
	mutex_unlock(&elev_lock);
}

static int __init elevator_init(void)
{
	int f, err;

	for (f = 0; f <= NUM_FLOORS; f++)
		INIT_LIST_HEAD(&floors[f]);

	elev_proc = proc_create(PROC_NAME, 0444, NULL, &elevator_proc_ops);
	if (!elev_proc)
		return -ENOMEM;

	elev_thread = kthread_run(elevator_thread_fn, NULL, "elevator_thread");
	if (IS_ERR(elev_thread)) {
		err = PTR_ERR(elev_thread);
		proc_remove(elev_proc);
		return err;
	}

	STUB_start_elevator = elevator_start;
	STUB_issue_request = elevator_issue_request;
	STUB_stop_elevator = elevator_stop;
	return 0;
}

static void __exit elevator_exit(void)
{
	/* Stop new system calls from reaching the module, then stop the thread */
	STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

	kthread_stop(elev_thread);
	proc_remove(elev_proc);
	free_all_pets();
	mutex_destroy(&elev_lock);
}

module_init(elevator_init);
module_exit(elevator_exit);