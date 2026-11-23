/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Desc:
*/

/*
 * List of all possible syscalls. They are private and there's no need
 * to implement all of them.
 */

#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#define SC_CAUSE        0x000
#define SC_DISPATCH     0x001
#define SC_SWITCH       0x002
#define SC_SCHEDULE     0x003
#define SC_CLI          0x004
#define SC_STI          0x005
#define SC_SUPERSTATE   0x006
#define SC_ISSUPERSTATE 0x007
#define SC_INVALIDATED  0x008
#define SC_RTAS		    0x009
#define SC_SUPERVISOR	0x00A
#define SC_CACHECLEARE  0x00B
#define SC_GETCPUNUMBER 0x00C
#define SC_USERSTATE    0x00D
/*
 * SMP exec asks the kernel for the run list moves it may not make itself:
 * SC_RESCHEDTASK moves a task (argument 1) to the list for a state
 * (argument 2); SC_REMSWITCH switches the calling task away for RemTask(),
 * without dispatching another one.
 */
#define SC_RESCHEDTASK  0x00E
#define SC_REMSWITCH    0x00F
#define SC_REBOOT	    0x100
#define SC_POWEROFF	    0x101

#endif
