#ifndef KERNEL_IPI_H_
#define KERNEL_IPI_H_
/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * List of all possible (private) kernal IPI message types.
 */

#define IPI_NOP                 0x000
#define IPI_CAUSE               0x001
#define IPI_DISPATCH            0x002
#define IPI_SWITCH              0x003
#define IPI_SCHEDULE            0x004
#define IPI_CLI                 0x005
#define IPI_STI                 0x006
#define IPI_ADDTASK	        0x101
#define IPI_REMTASK	        0x102
#define IPI_REBOOT	        0x10F
#endif
