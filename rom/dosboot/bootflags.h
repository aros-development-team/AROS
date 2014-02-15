#ifndef DOSBOOT_BOOTFLAGS_H
#define DOSBOOT_BOOTFLAGS_H

/*
   Copyright © 1995-2014, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Boot flag definitions
   Lang: english
*/

/* Boot flags. PRIVATE AND AROS SPECIFIC! Subject to change! */
#define BF_NO_STARTUP_SEQUENCE 0x0001
#define BF_NO_DISPLAY_DRIVERS  0x0002
#define BF_NO_COMPOSITION      0x0004
#define BF_EMERGENCY_CONSOLE   0x0008    /* Use emergency console */

#endif /* DOSBOOT_BOOTFLAGS_H */
