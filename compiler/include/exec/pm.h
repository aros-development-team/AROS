#ifndef EXEC_PM_H
#define EXEC_PM_H

/*
    Copyright © 2023, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Power management structures and constants
    Lang: english
*/

/* Actions for ShutdownA() */

#define SD_ACTION_POWEROFF              0
#define SD_ACTION_COLDREBOOT            1
#define SD_ACTION_WARMREBOOT            2

#define SD_ACTION_MASK                  0x00000007
#define SD_FLAG_EMERGENCY               0x00000008      /* Only the most basic/essential code should be executed    */

/* Shutdown Priority levels */
#define SD_PRI_OUTPUT				    0               /* displays/audio devices should stop now                   */
#define SD_PRI_DOS				        -40             /* filesystems and io should stop                           */
#define SD_PRI_REBOOT					-64             /* reboot occurs at this level                              */
#define SD_PRI_OFF						-128            /* complete power off occurs at this level                  */

/* Technically these are kernel definitions... */
#define PM_STATE_OFF					0

#define PM_STATE_1                      0x10 /* Max Performance level */
#define PM_STATE_2                      0x20
#define PM_STATE_3                      0x30
#define PM_STATE_4                      0x40
#define PM_STATE_5                      0x50 /* Min Performance level */

#define PM_STATE_IDLE                   0x90
#define PM_STATE_SLEEP                  0xF0
#define PM_STATE_REBOOT					0xFF

#endif	/* EXEC_PM_H */
