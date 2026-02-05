[![Open in Codespaces](https://classroom.github.com/assets/launch-codespace-2972f46106e565e64193e422d61a12cf1da4916b45550586e14ef0a7c637dd04.svg)](https://classroom.github.com/open-in-codespaces?assignment_repo_id=22541301)
Introduction
============
HAWX is a system based on a fairly simple microkernel. At the core of
this microkernel is a system of circular buffers called ports. The
system has a statically defined number of these ports, each of which
contains a 1024 byte buffer. These ports are used for two primary
purposes:

  1. Communication with Devices
  2. Communication Between Processes

To understand the first case, we first need to take a look at the
machine which HAWX is designed to operate. The machine consists of
the computer itself (CPU and RAM), one hard disk, and a serial
terminal. The basic setup is shown in the diagram below:

    +------------+                +-----------+
    |  Computer  |     UART       | SERIAL    |
    |    CPU     | <------------> |  TERMINAL |
    |    RAM     |                |           |
    +------------+                +-----------+
          ^                      / .:::::::. /
          | VIRTIO               ------------
          |
        __V___ 
       | HARD |
       | DISK | 
        ------

Essentially, the kernel controls the machine's RAM, communicates with
the hard disk via the VIRTIO block device interface, and it
communicates with the user via the UART device which is attached to
a serial terminal. Of course, most of these operations are privileged,
and so the kernel must provide an interface to allow user processes to use 
these devices.

There are three ports which are associated with the serial terminal
and the hard disk. These are defined by the constants in
`kernel/port.h`:

    // Predefined ports
    #define PORT_CONSOLEIN  0 // Serial input
    #define PORT_CONSOLEOUT 1 // Serial output
    #define PORT_DISKCMD    2 // Disk command port

The first port will be populated with the input from the serial
terminal's keyboard as it comes in, and the second port will send
anything written to its buffer to the terminal. The third port will be
used to send commands to the disk.

The user process interface is provided via the port system calls:

    int port_acquire(int port);
    void port_close(int port);
    int port_write(int port, char *buf, int n);
    int port_read(int port, char *buf, int n);

We will see these system calls in a future assignment. For now, we are
going to focus on the task of creating the data structures and
management system for the ports themselves. In this assignment, you
will fill in the code that implements the kernel side of the
conversation with ports.

Getting Started
===============
This assignment's main function will start the kernel space and
then run four unit tests on the HAWX port system. For each test,
the system will print either "FAILED" or "PASSED". So intuitively,
this is what a total failure of the system would look like:

    HAWX kernel is booting

    port init test...FAILED
    port write test...FAILED
    port read test...FAILED
    port acquire/close test...FAILED
    panic: All done! (for now...)

Let's see what actually happens when we try to run with just the
starter code. Run the following command:

    make qemu

As it is now, when you run HAWX, you will see the following output:

    panic: All done! (for now...)

This doesn't bode well! The system doesn't even work well enough to
tell us that it failed all of its tests! What gives?! Well, we are
writing an operating system, and in OS development, output is a
privilege, not a right! Recall from the introduction that console
output is buffered through the port system. the `PORT_CONSOLEOUT` port
is where `printf` sends all of its output because `printf` is an
inherently buffered output mechanism. Since we haven't implemented
the ports yet, `printf` output will not be seen. We do, however,
get to see the output of the `panic` function.

`panic` puts the kernel into a panicked state. This is the equivalent
of Windows' famed "blue screen of death." Once you call the panic
function, the following things happen:

1. The UART buffered output is forcibly flushed to the screen.
2. The `panic` function uses low-level driver calls to print the panic
   message.
3. The kernel "spinlocks", entering an infinite loop which does nothing.

This means that all `panic` needs to function is a working UART (serial)
driver. This allows us the ability to always force output and then
halt the kernel. (Unless, of course, something goes wrong with the
serial driver!)

So now that we know that our kernel is hopelessly deadlocked, we need
to shut the machine down. To do this, press Ctrl-a followed by x to exit
qemu.

Exploring the Source Code
=========================
HAWX is derived from xv6. Compared to xv6, it is a simplified kernel
which runs on a single CPU. For now, there is no user environment, and
all of the code is contained in the kernel directory. The files in
this directory are:

* `console.h` - Header file containing low-level text i/o interface.
* `disk.h` - Header file for the disk driver.
* `elf.h` - Header file for the Executable and Linkable Format (ELF).
* `entry.S` - The entry point of the kernel.
* `kernel.ld` - The kernel's linker script.
* `kernelvec.S` - The kernel's trap vector.
* `libprecompiled.a` - Precompiled files from other assignments.
* `main.c` - The main function of the kernel.
* `mem.h` - Header file for the virtual memory subystem.
* `memlayout.h` - Layout of the machine's physical memory.
* `plic.c` - The platform-level interrupt controller (PLIC).
* `port.c` - The implementation of the port system.
* `port.h` - Header file of the port system.
* `printf.c` - A few functions for formatted output.
* `proc.h` - Header file for process-related types and functions.
* `riscv.h` - Wrappers for riscv assembly instructions.
* `scheduler.h` - Header file for the process scheduler.
* `start.c` - The part of the kernel which calls main.
* `string.c` - String and memory manipulation functions.
* `string.h` - Header file for string and memory functions.
* `swtch.S` - Context switch code.
* `syscall.h` - Header file for system calls.
* `tests.c` - Unit tests for various kernel subsystems.
* `tests.h` - Header file for kernel tests.
* `trampoline.S` - Trampoline code for entering/exiting user space.
* `trap.h` - Header file for trap handling.
* `types.h` - Header file for basic types.
* `virtio.h` - Header file for the VIRTIO block device.
* `types.h` - Defined types for the kernel.

Take a moment and look through each of these files. Understanding how
the kernel as a whole works will help you succeed in your kernel
assignments. One thing to especially take note of is the presence of
`printf.c` and `string.c`. These files contain functions that are
usually part of the C standard library. However, when the kernel is
running there is nothing present other than the code the kernel brought
with it. That means, we have to bring in any code that we need! These
files contain simplified versions of these functions. They will mostly
work as expected, though functions like `printf` only partially support
the full features of their standard counterparts. They will work well
enough for our purposes though.

The key parts to explore for this assignment are `main.c`, `port.c`,
and `port.h`. In `main.c`, we have the kernel's main function:

    // start() jumps here in supervisor mode
    void
    main()
    {
      // initialize ports
      port_init();
  
      // initialize uart
      uartinit();
      printf("\n");
      printf("HAWX kernel is booting\n");
      printf("\n");
      uartflush();
  
      // initialize traps
      trapinit();
  
      //initialize virtual memory
      vm_init();
  
      //initialize the device interrupts
      plicinit();
  
      port_test();
      panic("All done! (for now...)");
    }

As you can see, the kernel is very incomplete at this stage. It simply
displays a start message, initializes a few subsystems, and then runs
the unit test for the port system. After it does all this, it panics
with the message that it is all done. That is because we do not have
a user space, system calls, or anything else in this version of the
kernel. The entire purpose of the kernel at this stage is to test its
core port system.

In the `port.h` file, you will see the following struct:

    // Define the Port struct with buffer, head, tail, etc.
    struct port {
      int free;                   // Is port free?
      int owner;                  // ID of the process owning the port
      int type;                   // Type of the port (free or kernel)
      int head, tail;             // Indexes for the circular buffer
      int count;                  // Number of items in buffer
      char buffer[PORT_BUF_SIZE]; // Data buffer
    };

This struct defines a port and the `port.c` file contains a global
variable which has an array of `NPORT` numbers of ports. A port is
essentially a circular buffer. Each port supports concurrent producers
and consumers by tracking head, tail, and count indices.  For more 
information about circular buffers, read the comments at the top of 
the `port.c` file.

Debugging the Chicken and the Egg
=================================
Alright, so we've identified a major "chicken and egg" problem here.
In order to get normal output, the ports system has to be at least
partially functional. We CAN get output all the time at the expense
of halting the system. Is there a middle ground, you may ask? The
answer, is definitely yes! Since we are running our kernel in a
simulated environment, we can actually use the gdb debugger to
halt the kernel and inspect its memory. The kernel is, after all,
just a C program. If you are not familiar with using the gdb debugger,
it would be in your best interest to read through at least part of
the [gdb tutorial](https://www.gdbtutorial.com/).

Running gdb against a qemu target like this is just a tad more
involved than using it on a standard program. We must do this in
two parts. First, we build and run the system using this command:

    make qemu-gdb

This will build everything and then start the qemu system in debug
mode. Now, we need to open a second terminal window and run the
following command from within the `hawx-ports` directory:

    gdb-multiarch

Note that this is a special version of gdb which supports multiple
CPU architectures. `-multiarch` is not a command line argument, but
rather part of the program's name; so don't put a space in there.

From here, you can issue gdb commands in the gdb window and see the
output of HAWX in the qemu window. If you are using Visual Studio Code
or GitHub Codespaces, I recommend that use the split tab on the terminal
to keep the two windows side by side.

One thing to try is to `list port_test` and set a breakpoint inside
that function. This function is in the file `tests.c` and contains
the unit tests you need to pass. From here, you can continue in
gdb and step through the function to see where it fails. All of the
tests will fail, of course, but this will allow you to see which
checks are failing in particular. Pay close attention to the order in
which the tests fail -- earlier failures often cascade into later
ones.

When you are done, type `quit` in the gdb window to kill the debugger.
This will cause the qemu instance to resume execution without 
interruption.  Shutdown the qemu machine in the usual way.

Implementing the Port System
============================
Your assignment is to complete the port system. After studying the
various source files, you will be ready to begin. In the file `port.c`,
there are several functions which have a comment which reads:

    // YOUR CODE HERE

These are the sections which you must fill in to complete the
assignment. These are found in the following functions:

* `port_init`
* `port_close`
* `port_acquire`
* `port_write`
* `port_read`

The comments in and around these functions will tell you how these
are meant to perform, and contain several useful hints for how to
implement these functions. If you like, you are welcome to add static
helper functions to the port.c file, but please do not edit any other
source files in this assignment. Also, you should not modify the
`struct port` definition or any of the constants in `port.h`

Finally, in the `tests.c` file, you will see the `port_test` function.
This is the set of unit tests that checks that your functions work.  
You are not allowed to alter this function, but feel free to read it 
as it may give you clues to the expected behavior of the port system.

Once you have successfully completed the assignment, when you run
HAWX you will see the following output:

    HAWX kernel is booting

    port init test...PASSED
    port write test...PASSED
    port read test...PASSED
    port acquire/close test...PASSED
    panic: All done! (for now...)

Once you get that set of four passes, pat yourself on the back. You've
taken the first steps on the path to becoming an operating systems
developer! May God have mercy on your soul.
