/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common screen console variables.
*/

#include "bootconsole.h"

/*
 * Variables in this file are interntionally placed into .data section
 * because they need to survive warm reboots.
 */

/* Display buffer parameters. */
__attribute__((section(".data"))) unsigned char scr_Type         = SCR_UNKNOWN;
__attribute__((section(".data"))) void         *scr_FrameBuffer  = 0;	/* VRAM address			*/
__attribute__((section(".data"))) unsigned int  scr_Width	 = 0;	/* Display width in characters	*/
__attribute__((section(".data"))) unsigned int  scr_Height       = 0;	/* Display height in characters	*/

/* Current output position (in characters) */
__attribute__((section(".data"))) unsigned int scr_XPos = 0;
__attribute__((section(".data"))) unsigned int scr_YPos = 0;
