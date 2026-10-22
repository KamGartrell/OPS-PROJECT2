# Elevator

A Linux kernel modification and module project that implements system call tracing, a kernel timer module using /proc/timer, and a multi-threaded pet elevator scheduling kernel module controlled via custom system calls and /proc/elevator.

## Group Members
- **Kameryn Gartrell**: ksg22b@fsu.edu
- **Jane Smith**: js19@fsu.edu
- **Alex Brown**: ab19@fsu.edu
## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Added five system calls to a C program and verifying the trace count using strace
- **Assigned to**: Kameryn Gartrell

### Part 2: Timer Kernel Module
- **Responsibilities**: Created the my_timer kernel module to track current and elapsed time in /proc/timer
- **Assigned to**: Kameryn Gartrell

### Part 3a: Adding System Calls
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown

### Part 3b: Kernel Compilation
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, Jane Smith

### Part 3c: Threads
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, Jane Smith

### Part 3d: Linked List
- **Responsibilities**: [Description]
- **Assigned to**: Jane Smith

### Part 3e: Mutexes
- **Responsibilities**: [Description]
- **Assigned to**: John Doe

### Part 3f: Scheduling Algorithm
- **Responsibilities**: [Description]
- **Assigned to**: Alex Brown, John Doe

## File Listing
```
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── Makefile
│   └── src/
│       └── timer.c
├── part3/
│   ├── src/
│   ├── tests/
│   ├── Makefile
│   └── sys_call.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: gcc for C/C++
- **Dependencies**: unistd.h, linux/init.h, linux/module.h, linux/proc_fs.h, linux/timekeeping.h

## Part 1

### Compilation
```bash
make
```
This will build the executables part1/empty and part1/part1.
### Execution
```bash
make trace
```
This will run strace on both executables and output empty.trace and part1.trace.

## Part 2

### Compilation
```bash
make
```
This will build the kernel module in part2/my_timer.ko.
### Execution
```bash
sudo insmod my_timer.ko
cat /proc/timer
sudo rmmod my_timer
```
This will load the module, read the current and elapsed time from /proc/timer, and unload the module.

## Part 3

### Compilation
```bash
make
```
This will build the kernel module in part3/elevator.ko.
### Execution
```bash
sudo insmod elevator.ko
```
This will load the elevator kernel module.

## Development Log
Each member records their contributions here.

### Kameryn Gartrell

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-21 | Completed Part 1 and 2 |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

### [Member 2]

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


### [Member 3]

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |


## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |



## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Considerations
- Github Copilot helped with syntax formatting and documentation
- Google Gemnii helped with documentation and help with archiecture/concepts of project