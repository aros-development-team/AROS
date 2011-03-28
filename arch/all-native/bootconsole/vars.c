/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common screen console variables.
*/

#include <bootconsole.h>

#include "console.h"

/* Display buffer parameters */
void         *scr_FrameBuffer  = 0;	/* VRAM address			*/
unsigned int  scr_Width	       = 0;	/* Display width in characters	*/
unsigned int  scr_Height       = 0;	/* Display height in characters	*/

/* Current output position (in characters) */
unsigned int scr_XPos = 0;
unsigned int scr_YPos = 0;
