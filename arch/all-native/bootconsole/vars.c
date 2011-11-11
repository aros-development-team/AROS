/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common screen console variables.
*/

#include <bootconsole.h>

#include "console.h"

/*
 * Variables in this file are interntionally placed into .data section
 * because they need to survive warm reboots.
 */

/* Display buffer parameters. */
__attribute__((section(".data"))) void         *scr_FrameBuffer  = VGA_TEXT_ADDR;	/* VRAM address			*/
__attribute__((section(".data"))) unsigned int  scr_Width	 = VGA_TEXT_WIDTH;	/* Display width in characters	*/
__attribute__((section(".data"))) unsigned int  scr_Height       = VGA_TEXT_HEIGHT;	/* Display height in characters	*/

/* Current output position (in characters) */
__attribute__((section(".data"))) unsigned int scr_XPos = 0;
__attribute__((section(".data"))) unsigned int scr_YPos = 0;
