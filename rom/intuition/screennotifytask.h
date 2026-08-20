#ifndef SCREENNOTIFYTASK_H
#define SCREENNOTIFYTASK_H

/*
    Copyright  1995-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#define SCREENNOTIFYTASK_NAME 	    	"\xAB Screennotify Handler \xBB"
/*
 * The handler only replies notification messages; measured peak stack
 * use on m68k is under 400 bytes, and 16 KB of chip RAM is a real cost
 * on small Amigas.
 */
#ifdef __mc68000
#define SCREENNOTIFYTASK_STACKSIZE  	4096
#else
#define SCREENNOTIFYTASK_STACKSIZE  	AROS_STACKSIZE
#endif
#define SCREENNOTIFYTASK_PRIORITY   	0

/* Structure passed to the DefaultMenuHandler task when it's initialized */

BOOL InitDefaultScreennotifyHandler(struct IntuitionBase *IntuitionBase);

struct ScreennotifyTaskParams
{
    struct IntuitionBase    *intuitionBase;
    struct Task             *Caller;
    struct MsgPort          *ScreennotifyHandlerPort; /* filled in by ScreennotifyHandler task */
    BOOL                     success;
};

#endif /* SCREENNOTIFYTASK_H */
