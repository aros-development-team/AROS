/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Base class for console units
    Lang: english
*/

#include <proto/intuition.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
#include <devices/conunit.h>
#include <intuition/classes.h>
#include <intuition/intuition.h>
#include "consoleif.h"
#include "console_gcc.h"

#include <string.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

VOID normalizecoords(Object *o, WORD *x_ptr, WORD *y_ptr);

/*
   Base class for the classes handling standard, charmap and snipmap
   consoles. This is a whitebox base class (like GADGETCLASS), which means subclasses
   can have direct access to its instance data.
*/

struct consoledata
{
    struct intConUnit intunit;
};


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)cl->cl_UserData)


/*********************
**  Console::New()  **
*********************/
static Object *console_new(Class *cl, Object *o, struct opSet *msg)
{
    struct Window *win;
    EnterFunc(bug("Console::New()\n"));
    
    /* Get console window */
    win = (struct Window *)GetTagData(A_Console_Window, NULL, msg->ops_AttrList);
    if (!win)
    	ReturnPtr ("Console::New", Object *, NULL);
	
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct ConUnit 		*unit;
	struct consoledata 	*data;
    	struct RastPort 	*rp = win->RPort;
	WORD			i;
    	    	
	data = INST_DATA(cl, o);
	
	unit = (struct ConUnit *)data;
	
	memset(data, 0, sizeof(struct consoledata));
	
    	/* Initialize the unit fields */
    	unit->cu_Window = win;
    	    	

	/* For now one should use only non-proportional fonts */
    	unit->cu_XRSize	= rp->Font->tf_XSize;
    	unit->cu_YRSize	= rp->Font->tf_YSize;
    	    	
    	D(bug("cu_XRSize: %d, cu_YRSize: %d\n",
    	    	unit->cu_XRSize, unit->cu_YRSize));

     	/* Use whole window for console. */
    	unit->cu_XMax 	  = (win->Width  - (win->BorderLeft + win->BorderRight )) / unit->cu_XRSize - 1;
    	unit->cu_YMax 	  = (win->Height - (win->BorderTop  + win->BorderBottom)) / unit->cu_YRSize - 1;
    	    	
    	unit->cu_XROrigin = win->BorderLeft;
    	unit->cu_YROrigin = win->BorderTop;

    	D(bug("cu_XROrigin: %d, cu_YROrigin: %d\n",
    	unit->cu_XROrigin, unit->cu_YROrigin));
    	    	
    	unit->cu_XRExtant = win->BorderLeft + (unit->cu_XRSize * (unit->cu_XMax + 1) - 1);
    	unit->cu_YRExtant = win->BorderTop  + (unit->cu_YRSize * (unit->cu_YMax + 1) - 1);
	
	unit->cu_XCP = DEF_CHAR_XMIN;
	unit->cu_YCP = DEF_CHAR_YMIN;

	unit->cu_XCCP = DEF_CHAR_XMIN;
	unit->cu_YCCP = DEF_CHAR_YMIN;
	
	for(i = 0; i < MAXTABS - 1; i++)
	{
	    unit->cu_TabStops[i] = i * 8;
	}
	unit->cu_TabStops[i] = (UWORD)-1;
	
	ICU(o)->conFlags = 0UL;
	ICU(o)->numStoredChars = 0;

    	SET_MODE(o, PMB_ASM); /* auto-scroll-mode ON */
	SET_MODE(o, PMB_AWM); /* auto-wrap-mode ON */ 
	CLEAR_MODE(o, M_LNM); /* linefeed mode = linefeed only */
    }
    ReturnPtr("Console::New", Object *, o);
    
}
 


/**********************
**  Console::Left()  **
**********************/
static VOID console_left(Class *cl, Object *o, struct P_Console_Left *msg)
{
    WORD newx;
    
    EnterFunc(bug("Console::Left()\n"));

    newx = XCCP - msg->Num;
    
    if (CHECK_MODE(o, PMB_AWM))
    {
    	WORD scrollcount = 0;

	while(newx < CHAR_XMIN(o))
	{
            newx += (CHAR_XMAX(o) + 1);
	    scrollcount++;
	}
	
	XCP = XCCP = newx;
	
        Console_Up(o, scrollcount);

    }
    else
    {
    	if (newx < CHAR_XMIN(o)) newx = CHAR_XMIN(o);
    }
    
    XCP = XCCP = newx; /* XCP always same as XCCP?? */

    D(bug("XCP=%d, XCCP=%d\n", XCP, XCCP));
       
    ReturnVoid("Console::Left");
}

/***********************
**  Console::Right()  **
***********************/
static VOID console_right(Class *cl, Object *o, struct P_Console_Right *msg)
{
    WORD newx;
    
    EnterFunc(bug("Console::Right()\n"));

    newx = XCCP + msg->Num;
    
    if (CHECK_MODE(o, PMB_AWM))
    {
    	WORD scrollcount = 0;
	
    	while(newx > CHAR_XMAX(o))
    	{
            newx -= (CHAR_XMAX(o) + 1);
	    scrollcount++;
    	}
	
	XCP = XCCP = newx;
	
	Console_Down(o, scrollcount);

    }
    else
    {
    	if (newx > CHAR_XMAX(o)) newx = CHAR_XMAX(o);
    }

    XCP = XCCP = newx; /* XCP always same as XCCP?? */

    D(bug("XCP=%d, XCCP=%d\n", XCP, XCCP));
       
    ReturnVoid("Console::Right");
}

/********************
**  Console::Up()  **
********************/
static VOID console_up(Class *cl, Object *o, struct P_Console_Up *msg)
{
    EnterFunc(bug("Console::Up(num=%d)\n", msg->Num));

    YCCP -= msg->Num;
    
    if (YCCP < 0)
    {
    	if (CHECK_MODE(o, PMB_ASM))
	{
    	    IPTR scroll_param = -YCCP;

    	    YCCP = YCP = 0;
	    Console_DoCommand(o, C_SCROLL_DOWN, 1, &scroll_param);
	}
	else
	{
	    YCCP = 0;
	}
    }
    YCP = YCCP; /* YCP always same as YCCP ?? */
    
    D(bug("New coords: char (%d, %d), gfx (%d, %d)\n",
    	XCCP, YCCP, CP_X(o), CP_Y(o) ));
    ReturnVoid("Console::Up");
}



/**********************
**  Console::Down()  **
**********************/
static VOID console_down(Class *cl, Object *o, struct P_Console_Down *msg)
{
    EnterFunc(bug("Console::Down(num=%d)\n", msg->Num));

    YCCP += msg->Num;

    if (YCCP > CHAR_YMAX(o))
    {
    	if (CHECK_MODE(o, PMB_ASM))
	{
    	    IPTR scroll_param = YCCP - CHAR_YMAX(o);

    	    YCCP = YCP = CHAR_YMAX(o);
	    Console_DoCommand(o, C_SCROLL_UP, 1, &scroll_param);
	}
	else
	{
	    YCCP = CHAR_YMAX(o);
	}
    }
    YCP = YCCP; /* YCP always same as YCCP ?? */

    D(bug("New coords: char (%d, %d), gfx (%d, %d)\n",
    	XCCP, YCCP, CP_X(o), CP_Y(o) ));
    ReturnVoid("Console::Down");
}

/***************************
**  Console::DoCommand()  **
***************************/
static VOID console_docommand(Class *cl, Object *o, struct P_Console_DoCommand *msg)
{
    EnterFunc(bug("Console::DoCommand(cmd=%d)\n", msg->Command));

    switch (msg->Command)
    {
    	case C_SET_LF_MODE:
	    D(bug("Set LF mode ON\n"));
	    /* LF==LF+CR */
	    /* ICU(o)->conFlags |= CF_LF_MODE_ON ; */
	    SET_MODE(o, M_LNM);
	    break;

    	case C_RESET_LF_MODE:
	    /* LF==LF */
	    D(bug("Set LF mode OFF\n"));
	    /* ICU(o)->conFlags &= ~CF_LF_MODE_ON; */
	    CLEAR_MODE(o, M_LNM);
	    break;
	
	case C_SET_AUTOSCROLL_MODE:
	    SET_MODE(o, PMB_ASM);
	    break;
	    
	case C_RESET_AUTOSCROLL_MODE:
	    CLEAR_MODE(o, PMB_ASM);
	    break;
	    
	case C_SET_AUTOWRAP_MODE:
	    SET_MODE(o, PMB_AWM);
	    break;
	 
	case C_RESET_AUTOWRAP_MODE:
	    CLEAR_MODE(o, PMB_AWM);
	    break;
	    
	case C_SELECT_GRAPHIC_RENDITION:
	    D(bug("Select graphic Rendition\n"));
	    {
	        UBYTE i, param;
		
		for(i = 0; i < msg->NumParams; i++)
		{
		    param = msg->Params[i];

		    switch(param)
		    {
		        case 30:
			case 31:
			case 32:
			case 33:
			case 34:
			case 35:
			case 36:
			case 37:
			    CU(o)->cu_FgPen = param - 30;
			    break;
			    
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			    CU(o)->cu_BgPen = param - 40;
			    break;
			    
		    } /* switch(param) */
		    
		} /* for(i = 0; i < msg->NumParams; i++) */
	    }
	    break;


	case C_SET_RAWEVENTS:
	    D(bug("Set Raw Events\n"));
	    {
	        UBYTE i, param;
		
		for(i = 0; i < msg->NumParams; i++)
		{
		    param = msg->Params[i];

		    if (param <= IECLASS_MAX)
		    {
		    	SET_RAWEVENT(o, param);
		    }
		    
		} /* for(i = 0; i < msg->NumParams; i++) */
	    }
	    break;	  

	case C_RESET_RAWEVENTS:
	    D(bug("Set Raw Events\n"));
	    {
	        UBYTE i, param;
		
		for(i = 0; i < msg->NumParams; i++)
		{
		    param = msg->Params[i];

		    if (param <= IECLASS_MAX)
		    {
		    	RESET_RAWEVENT(o, param);
		    }
		    
		} /* for(i = 0; i < msg->NumParams; i++) */
	    }
	    break;	  
	     
    } /* switch (msg->Command) */
    
    ReturnVoid("Console::DoCommand");
}

/**********************************
**  Console::GetDefaultParams()  **
**********************************/
static VOID console_getdefaultparams(Class *cl, Object *o, struct P_Console_GetDefaultParams *msg)
{
    switch (msg->Command)
    {
	case C_INSERT_CHAR:
	case C_CURSOR_UP:
	case C_CURSOR_DOWN:
	case C_CURSOR_FORWARD:
	case C_CURSOR_BACKWARD:
	case C_CURSOR_NEXT_LINE:
	case C_CURSOR_PREV_LINE:
	    msg->Params[0] = 1;
	    break;
	case C_CURSOR_POS:
	    msg->Params[0] = YCCP + 1;
	    msg->Params[1] = XCCP + 1;
	    break;

#warning Autodocs state commands in between here, has params RKRM: Devs saye the do not
	case C_CURSOR_HTAB:
	case C_DELETE_CHAR:
	case C_SCROLL_UP:
	case C_SCROLL_DOWN:
	    msg->Params[0] = 1;
	    break;

	case C_CURSOR_TAB_CTRL:
	    msg->Params[0] = 0; /* set tab */
	    break;


	case C_CURSOR_BACKTAB:
	    msg->Params[0] = 1;
	    break;

	case C_SELECT_GRAPHIC_RENDITION:
	    /* don't do anything, as params may be in any order */
	    break;

	case C_SET_RAWEVENTS:
	    /* don't do anything, as params may be in any order */
	    break;

	case C_RESET_RAWEVENTS:
	    /* don't do anything, as params may be in any order */
	    break;

    } /* switch (msg->Command) */

    return;
}

/*******************************
**  Console::NewWindowSize()  **
*******************************/
static VOID console_newwindowsize(Class *cl, Object *o, struct P_Console_NewWindowSize *msg)
{
    struct ConUnit *unit;
    struct consoledata *data;
    struct Window *win;

    data = INST_DATA(cl, o);
    unit = (struct ConUnit *)data;

    win = unit->cu_Window;
    
    unit->cu_XMax     = (win->Width  - (win->BorderLeft + win->BorderRight )) / unit->cu_XRSize - 1;
    unit->cu_YMax     = (win->Height - (win->BorderTop  + win->BorderBottom)) / unit->cu_YRSize - 1;

    unit->cu_XRExtant = win->BorderLeft + (unit->cu_XRSize * (unit->cu_XMax + 1) - 1);
    unit->cu_YRExtant = win->BorderTop  + (unit->cu_YRSize * (unit->cu_YMax + 1) - 1);

    if (unit->cu_XCCP > unit->cu_XMax) unit->cu_XCCP = unit->cu_XMax;
    if (unit->cu_YCCP > unit->cu_YMax) unit->cu_YCCP = unit->cu_YMax;
    
    unit->cu_XCP = unit->cu_XCCP;
    unit->cu_YCP = unit->cu_YCCP;
    
    return;
}

/********* Console class dispatcher **********************************/
AROS_UFH3S(IPTR, dispatch_consoleclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;
    
    switch (msg->MethodID)
    {
    case OM_NEW:
    	retval = (IPTR)console_new(cl, o, (struct opSet *)msg);
	break;
	
    case M_Console_Left:
    	console_left(cl, o, (struct P_Console_Left *)msg);
	break;

    case M_Console_Right:
    	console_right(cl, o, (struct P_Console_Right *)msg);
	break;

    case M_Console_Up:
    	console_up(cl, o, (struct P_Console_Up *)msg);
	break;

    case M_Console_Down:
    	console_down(cl, o, (struct P_Console_Down *)msg);
	break;

    case M_Console_DoCommand:
    	console_docommand(cl, o, (struct P_Console_DoCommand *)msg);
	break;

    case M_Console_GetDefaultParams:
    	console_getdefaultparams(cl, o, (struct P_Console_GetDefaultParams *)msg);
	break;

    case M_Console_NewWindowSize:
    	console_newwindowsize(cl, o, (struct P_Console_NewWindowSize *)msg);
	break;
	
    default:
    	retval = DoSuperMethodA(cl, o, msg);
	break;
	
    }
    return (retval);

    AROS_USERFUNC_EXIT	
}


/************************
**  normalizecoords()  **
************************/

/* Normalizes "out of window" coords, so that they
   fit into the window
*/

#define ABS(a) (((a) < 0) ? -(a) : (a))


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)OCLASS(o)->cl_UserData)

#warning Currently dead code
VOID normalizecoords(Object *o, WORD *x_ptr, WORD *y_ptr)
{
    EnterFunc(bug("normalizecoords(o=%p, x=%d, y=%d)\n",
    	o, *x_ptr, *y_ptr));

    if (*x_ptr > CU(o)->cu_XMax) /* charpos too far to the right */
    {
	D(bug("Pos right of window\n"));
    	/* Increase y */
	Console_Down(o, *x_ptr / CHAR_XMAX(o) );
	
	/* Normalize x */
    	*x_ptr = *x_ptr % (CHAR_XMAX(o) - CHAR_XMIN(o));
    }
    else if (*x_ptr < CHAR_XMIN(o))
    {
	D(bug("Pos left of window\n"));
	    	
	/* Decrease y */
	Console_Up(o, ABS(*x_ptr) / CHAR_XMAX(o) - CHAR_XMIN(o) );
	
	/* Normalize Z */
	*x_ptr = *x_ptr % (CHAR_XMAX(o) - CHAR_XMIN(o));
    
    }
    
    if (*y_ptr > CHAR_YMAX(o))	/* pos below window bounds */
    {
	
    	*y_ptr = CHAR_YMAX(o);
    }
    else if (*y_ptr < CHAR_YMIN(o))
    {
    	UBYTE scroll_param = CHAR_YMIN(o) - *y_ptr;	/* pos above window bounds */
	
	D(bug("Pos above window\n"));
	
    	Console_DoCommand(o, C_SCROLL_DOWN, 1, &scroll_param);

    	*y_ptr = CHAR_YMIN(o);
    
    }

    ReturnVoid("normalizecoords");

}


#undef ConsoleDevice

Class *makeConsoleClass(struct ConsoleBase *ConsoleDevice)
{
    
   Class *cl;
   	
   cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct consoledata), 0UL);
   if (cl)
   {
    	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_consoleclass;
    	cl->cl_Dispatcher.h_SubEntry = NULL;

    	cl->cl_UserData = (IPTR)ConsoleDevice;

   	return (cl);
    }
    return (NULL);
}

