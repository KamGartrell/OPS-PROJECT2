#include <unistd.h> // Include the unistd.h header for system calls

int main(void) {

	getpid(); // Call to get the process ID of the calling process
	getppid(); // Call to get the parent process ID of the calling process
	getuid(); // Call to get the real user ID of the calling process
	geteuid(); // Call to get the effective user ID of the calling process
	getgid(); // Call to get the real group ID of the calling process

	return 0;
}