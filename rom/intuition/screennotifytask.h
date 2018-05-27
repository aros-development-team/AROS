#ifndef SCREENNOTIFYTASK_H
#define SCREENNOTIFYTASK_H

/*
    Copyright  1995-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define SCREENNOTIFYTASK_NAME 	    	"« Screennotify Handler »"
#define SCREENNOTIFYTASK_STACKSIZE  	AROS_STACKSIZE
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
