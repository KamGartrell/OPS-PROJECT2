#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

// Define the license, author, description, and version of the kernel module
MODULE_LICENSE("GPL");
MODULE_AUTHOR("GROUP 3");
MODULE_DESCRIPTION("COP4610 Project 2 Part 2: Timer Kernel Module");
MODULE_VERSION("1.0");

// Define constants for the proc file name, buffer length, and nanoseconds per second
#define PROC_NAME "timer"
#define BUFFER_LEN 256
#define NANOSEC_PER_SEC_VAL 1000000000L

// Declare static variables for the proc entry, last time, and prior call flag
static struct proc_dir_entry *timerProcEntry;
static struct timespec64 lastTime;
static bool hasPriorCall = false;

// Function to read the current time and elapsed time from the proc file
static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
	// Declare local variables for the current time, buffer, and length
    struct timespec64 currentTime;
	char buf[BUFFER_LEN];
	int len = 0;

	// If the user already read the buffer on this cat call, return 0 (EOF)
	if (*ppos > 0)
		return 0;

    // Get the current time using ktime_get_real_ts64
	ktime_get_real_ts64(&currentTime);

    // If this is the first call, just print the current time OR calculate and print the elapsed time
	if (!hasPriorCall) {
        // Format the current time into the buffer
		len = snprintf(buf, sizeof(buf), "Current time: %lld.%09lld\n", (long long)currentTime.tv_sec, currentTime.tv_nsec);
		hasPriorCall = true;
	} else {
		long long diffSec = currentTime.tv_sec - lastTime.tv_sec;
		long long diffNSec = currentTime.tv_nsec - lastTime.tv_nsec;

		// Borrow 1 second if nanoseconds wrapped around
		if (diffNSec < 0) {
			diffSec -= 1;
			diffNSec += NANOSEC_PER_SEC_VAL;
		}

		len = snprintf(buf, sizeof(buf), "Current time: %lld.%09lld\n" "Elapsed time: %lld.%09lld\n", (long long)currentTime.tv_sec, currentTime.tv_nsec, diffSec, diffNSec);
	}

	// Save current time for the next read
	lastTime = currentTime;

    // Copy the formatted buffer to user space and return the number of bytes read
	return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

// Define the proc_ops structure for the timer proc entry
static const struct proc_ops timer_proc_ops = {
	.proc_read = timer_read,
};

// Module initialization function to create the proc entry
static int __init timer_init(void) {
    // Create the proc entry with read permissions and associate it with the timer_proc_ops
	timerProcEntry = proc_create(PROC_NAME, 0444, NULL, &timer_proc_ops);
	if (!timerProcEntry)
		return -ENOMEM; // Return error if proc entry creation fails

	hasPriorCall = false;
	return 0;
}

// Module cleanup function to remove the proc entry
static void __exit timer_exit(void) {
	if (timerProcEntry)
		proc_remove(timerProcEntry);
}

// Register the module initialization and cleanup functions
module_init(timer_init);
module_exit(timer_exit);