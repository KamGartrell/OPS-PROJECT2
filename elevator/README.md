# Elevator

A Linux kernel modification and module project that implements system call tracing, a kernel
timer module using /proc/timer, and a multi-threaded pet elevator scheduling kernel module
controlled via custom system calls and /proc/elevator.

## Group Members
- **Kameryn Gartrell**: ksg22b@fsu.edu
- **Matthew Chen**: mc23c@fsu.edu
- **Tyler Dussault**: [email]@fsu.edu

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**: Added five system calls to a C program and verified the trace count
  using strace
- **Assigned to**: Kameryn Gartrell

### Part 2: Timer Kernel Module
- **Responsibilities**: Created the my_timer kernel module to track current and elapsed time in
  /proc/timer
- **Assigned to**: Kameryn Gartrell

### Part 3a: Adding System Calls
- **Responsibilities**: Added the `start_elevator` (548), `issue_request` (549) and
  `stop_elevator` (550) system calls to the kernel (`syscalls.c`, `syscall_64.tbl`, `syscalls.h`,
  top-level `Makefile`). Each call dispatches through an exported `STUB_*` function pointer that
  `elevator.ko` fills in on load, and returns `-ENOSYS` when no module is loaded.
- **Assigned to**: Matthew Chen

### Part 3b: Kernel Compilation
- **Responsibilities**: Wrote scripts to patch, configure (certificate options disabled), build
  and install linux-6.16.x with the new system calls, and verify the result with `uname -r` and
  `part3/tests/system-calls-test`.
- **Assigned to**: Matthew Chen

### Part 3c: Threads
- **Responsibilities**: The kthread that runs the elevator: movement between floors, the 2.0 s
  travel and 1.0 s load/unload delays, and the state transitions (OFFLINE, IDLE, LOADING, UP,
  DOWN).
- **Assigned to**: Tyler Dussault

### Part 3d: Linked List
- **Responsibilities**: The linked lists (`linux/list.h`) of pets waiting on each floor and pets
  riding the elevator, with pets allocated dynamically using `kmalloc`.
- **Assigned to**: Kameryn Gartrell, Tyler Dussault

### Part 3e: Mutexes
- **Responsibilities**: Mutex locking around the shared floor and elevator data, used by the
  system calls, the kthread and the /proc/elevator reader.
- **Assigned to**: Kameryn Gartrell, Matthew Chen

### Part 3f: Scheduling Algorithm
- **Responsibilities**: Deciding which direction to travel and which pets to load (FIFO
  boarding, 5 pet / 50 lb limit) and when to go idle.
- **Assigned to**: Tyler Dussault

## File Listing
```
elevator/
├── Makefile
├── README.md
├── part1/
│   ├── Makefile
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   └── part1.trace
├── part2/
│   ├── Makefile
│   └── src/
│       └── timer.c
└── part3/
    ├── Makefile
    ├── syscalls.c                  (kernel source, copied into <kernel>/syscalls/)
    ├── src/                        (elevator module sources, in progress)
    ├── kernel/
    │   ├── apply_syscalls.sh       (patches a kernel tree with the syscalls)
    │   ├── build_kernel.sh         (configures, builds and installs the kernel)
    │   └── syscalls.Makefile       (installed as <kernel>/syscalls/Makefile)
    └── tests/
        ├── elevator-test/          (producer.c, consumer.c, wrappers.h, run_tests.sh, Makefile)
        └── system-calls-test/      (syscheck.c, test-syscalls.c/.h, Makefile)
```

# How to Compile & Execute

### Requirements
- **Compiler**: gcc
- **Tools**: make, strace
- **Kernel headers**: `linux-headers` for the running kernel (Parts 2 and 3 build against
  `/lib/modules/$(uname -r)/build`)
- **Part 3 kernel**: linux-6.16.x source and the build dependencies installed by
  `part3/kernel/build_kernel.sh`

## Part 1

### Compilation
```bash
cd part1
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
cd part2
make
```
This will build the kernel module part2/my_timer.ko.
### Execution
```bash
sudo insmod my_timer.ko
cat /proc/timer
sudo rmmod my_timer
```
This will load the module, read the current and elapsed time from /proc/timer, and unload the
module.

## Part 3

### Step 1: Kernel with system calls (do this first; the build takes a long time)
Download the linux-6.16.x tarball from kernel.org, extract it to `/usr/src` and link it into
your home directory so it is easy to edit:
```bash
sudo tar -xvf linux-6.16.x.tar.xz -C /usr/src
sudo chown -R $USER /usr/src/linux-6.16.x
ln -s /usr/src/linux-6.16.x ~/linux-6.16.x
```
Then add the system calls, build and install the kernel:
```bash
cd part3/kernel
./apply_syscalls.sh ~/linux-6.16.x
./build_kernel.sh ~/linux-6.16.x      # make menuconfig opens: just save and exit
sudo reboot
uname -r                              # should be 6.16.x (without -generic)
```
`apply_syscalls.sh` copies `part3/syscalls.c` and `syscalls.Makefile` into `<kernel>/syscalls/`,
adds entries 548-550 to `arch/x86/entry/syscalls/syscall_64.tbl`, adds the prototypes to
`include/linux/syscalls.h`, and adds `syscalls/` to `core-y` in the top-level `Makefile`.
It can be re-run safely.

`build_kernel.sh` installs the build dependencies, disables `SYSTEM_TRUSTED_KEYS` and
`SYSTEM_REVOCATION_KEYS`, runs `make menuconfig`, `make -j$(nproc)`, `sudo make modules_install`,
`sudo make install` and `sudo update-grub`. It does not reboot. Pass `--skip-menuconfig` for an
unattended build. The kernel only needs to be built once; after that the system call logic is
compiled as the `elevator` module.

### Step 2: Verify the system calls
```bash
cd part3/tests/system-calls-test
make
sudo insmod syscheck.ko
./test-syscalls
sudo rmmod syscheck
```

### Step 3: Compile and run the elevator
```bash
cd part3
make
sudo insmod elevator.ko
cd tests/elevator-test
make
./consumer --start
./producer 10
watch -n 1 cat /proc/elevator
./consumer --stop
sudo rmmod elevator
```
This builds part3/elevator.ko, loads it, starts the elevator, issues 10 random pet requests,
shows the elevator state, then stops the elevator and unloads the module. `run_tests.sh` in the
same directory runs a longer scripted start/stop/producer sequence.

## Development Log
Each member records their contributions here.

### Kameryn Gartrell

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-21 | Completed Part 1 and 2 |
| YYYY-MM-DD | [Description of task]  |

### Matthew Chen

| Date       | Work Completed / Notes |
|------------|------------------------|
| 2026-10-24 | Wrote `syscalls.c` (548/549/550 with exported `STUB_*` hooks) and `apply_syscalls.sh` |
| 2026-10-24 | Wrote `build_kernel.sh` (config, build, install) and documented the kernel build in the README |
| YYYY-MM-DD | [Build and boot the kernel, run `test-syscalls`] |

### Tyler Dussault

| Date       | Work Completed / Notes |
|------------|------------------------|
| YYYY-MM-DD | [Description of task]  |
| YYYY-MM-DD | [Description of task]  |

## Meetings
Document in-person meetings, their purpose, and what was discussed.

| Date       | Attendees            | Topics Discussed | Outcomes / Decisions |
|------------|----------------------|------------------|-----------------------|
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |
| YYYY-MM-DD | [Names]              | [Agenda items]   | [Actions/Next steps]  |

## Bugs
- **Elevator module not finished**: `part3/src/` and `part3/Makefile` are not written yet, so
  `make` in `part3/` does not produce `elevator.ko` and the top-level `make` fails until they are.
- **Module unload race**: the syscalls call through the `STUB_*` pointers without any lock, so
  unloading `elevator.ko` while a syscall is running could crash the kernel. Not an issue for the
  normal start, run, stop, rmmod sequence.
- **Kernel build not yet verified**: the patch script was tested on a mock kernel tree only; the
  real 6.16.x build is still to be confirmed (see development log).

## Considerations
- Github Copilot helped with syntax formatting and documentation
- Google Gemini helped with documentation and help with architecture/concepts of project
- Claude (Anthropic) helped write the Part 3a/3b system call source, kernel patch script, kernel
  build script and the related README sections
