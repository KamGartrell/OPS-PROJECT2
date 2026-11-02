// SPDX-License-Identifier: GPL-2.0
/*
 * syscalls.c - elevator system calls (548, 549, 550)
 *
 * Lives in <kernel>/syscalls/ and is built into the kernel image.
 *
 * The real elevator logic lives in the loadable module elevator.ko, so each
 * system call dispatches through an exported function pointer ("STUB").
 * The module sets the pointers in its init function and resets them to NULL
 * in its exit function.  While no module is loaded the calls return -ENOSYS.
 *
 * Syscall numbers (arch/x86/entry/syscalls/syscall_64.tbl):
 *   548  start_elevator
 *   549  issue_request
 *   550  stop_elevator
 */
#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/syscalls.h>

/* Hooks filled in by the elevator (or syscheck) module. */
int (*STUB_start_elevator)(void) = NULL;
int (*STUB_issue_request)(int, int, int) = NULL;
int (*STUB_stop_elevator)(void) = NULL;
EXPORT_SYMBOL(STUB_start_elevator);
EXPORT_SYMBOL(STUB_issue_request);
EXPORT_SYMBOL(STUB_stop_elevator);

/* int start_elevator(void) */
SYSCALL_DEFINE0(start_elevator)
{
	if (STUB_start_elevator != NULL)
		return STUB_start_elevator();
	return -ENOSYS;
}

/* int issue_request(int start_floor, int destination_floor, int type) */
SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor,
		int, type)
{
	if (STUB_issue_request != NULL)
		return STUB_issue_request(start_floor, destination_floor, type);
	return -ENOSYS;
}

/* int stop_elevator(void) */
SYSCALL_DEFINE0(stop_elevator)
{
	if (STUB_stop_elevator != NULL)
		return STUB_stop_elevator();
	return -ENOSYS;
}
