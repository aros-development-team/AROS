/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Base class for console units
    Lang: english
*/

#include <proto/boopsi.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
#include <devices/conunit.h>
#include <intuition/classes.h>
#include <intuition/intuition.h>
#include "consoleif.h"
#include "console_gcc.h"

#define DEBUG 1
#include <aros/debug.h>
/*
   Base class for the classes handling standard, charmap and snipmap
   consoles. This is a whitebox base class (like GADGETCLASS), which means subclasses
   can have direct access to its instance data.
*/

struct consoledata
{
    struct ConUnit unit;
};

static VOID getcoordptrs(Object *con, WORD **x_ptr, WORD **y_ptr, UWORD cp_type);


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
    	struct ConUnit *unit;
	struct consoledata *data;

    	struct RastPort *rp = win->RPort;
    	    	
	data = INST_DATA(cl, o);
	
	unit = (struct ConUnit *)data;
    	/* Initialize the unit fields */
    	unit->cu_Window = win;
    	    	

	/* For now one should use only proportional fonts */
    	unit->cu_XRSize	= rp->Font->tf_XSize;
    	unit->cu_YRSize	= rp->Font->tf_YSize;
    	    	
    	D(bug("cu_XRSize: %d, cu_YRSize: %d\n",
    	    	unit->cu_XRSize, unit->cu_YRSize));

     	/* Use whole window for console */
    	unit->cu_XMax 	  = (win->Width  - (win->BorderLeft + win->BorderRight )) / unit->cu_XRSize;
    	unit->cu_YMax 	  = (win->Height - (win->BorderTop  + win->BorderBottom)) / unit->cu_YRSize;
    	    	
    	unit->cu_XROrigin = win->BorderLeft;
    	unit->cu_YROrigin = win->BorderTop;

    	D(bug("cu_XROrigin: %d, cu_YROrigin: %d\n",
    	unit->cu_XROrigin, unit->cu_YROrigin));
    	    	
    	unit->cu_XRExtant = win->BorderLeft + (unit->cu_XRSize * unit->cu_XMax);
    	unit->cu_YRExtant = win->BorderTop  + (unit->cu_YRSize * unit->cu_YMax);
	
	unit->cu_XCP = MIN_XCP;
	unit->cu_YCP = MIN_YCP;
	    
	
    }
    ReturnPtr("Console::New", Object *, o);
    
}
 



/**********************
**  Console::Left()  **
**********************/
static VOID console_left(Class *cl, Object *o, struct P_Console_Left *msg)
{
    WORD *x_ptr = NULL, *y_ptr = NULL; /* Keep compiler happy */
    
    getcoordptrs(o, &x_ptr, &y_ptr, msg->Type);
    *x_ptr -= msg->Num;
    if (*x_ptr < 0)
    {
    	*x_ptr = CU(o)->cu_XMax;
	
	Console_Up(o, msg->Type, 1); 
    }
    
    return;
}

/***********************
**  Console::Right()  **
***********************/
static VOID console_right(Class *cl, Object *o, struct P_Console_Right *msg)
{
    WORD *x_ptr = NULL, *y_ptr = NULL; /* Keep compiler happy */
    
    getcoordptrs(o, &x_ptr, &y_ptr, msg->Type);

    *x_ptr += msg->Num;
    if (*x_ptr > CU(o)->cu_XMax)
    {
    	*x_ptr = 0;
    	
	Console_Down(o, msg->Type, 1);
    }
    
    return;
}

/********************
**  Console::Up()  **
********************/
static VOID console_up(Class *cl, Object *o, struct P_Console_Up *msg)
{
    WORD *x_ptr = NULL, *y_ptr = NULL; /* Keep compiler happy */
    
    getcoordptrs(o, &x_ptr, &y_ptr, msg->Type);
    *y_ptr -= msg->Num;
    if (*y_ptr < 0)
    {
    	UBYTE scroll_param = msg->Num;
	
    	Console_DoCommand(o, C_SCROLL_DOWN, &scroll_param);

    	*y_ptr = 0;
    }
    return;
}

/**********************
**  Console::Down()  **
**********************/
static VOID console_down(Class *cl, Object *o, struct P_Console_Down *msg)
{
    WORD *x_ptr = NULL, *y_ptr = NULL; /* Keep compiler happy */
    
    getcoordptrs(o, &x_ptr, &y_ptr, msg->Type);
    
    *y_ptr += msg->Num;
    
    if (*y_ptr > CU(o)->cu_YMax)
    {
    	UBYTE scroll_param = msg->Num;
    	Console_DoCommand(o, C_SCROLL_UP, &scroll_param);     
	
    	*y_ptr = CU(o)->cu_YMax;
    }
    return;
}

AROS_UFH3(static IPTR, dispatch_consoleclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
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
    default:
    	retval = DoSuperMethodA(cl, o, msg);
	break;
	
    }
    return (retval);
	
}

/************************
**  Support functions  **
************************/

/*********************
**  getcoordptrs()  **
*********************/

static VOID getcoordptrs(Object *con, WORD **x_ptr, WORD **y_ptr, UWORD cp)
{
    switch (cp)
    {
        case COORD_WRITEPOS:
	    *x_ptr = &(CU(con)->cu_XCP);
	    *y_ptr = &(CU(con)->cu_YCP);
	    break;
	
	case COORD_CURSOR:
	    *x_ptr = &(CU(con)->cu_XCP);
	    *y_ptr = &(CU(con)->cu_YCP);
	    break;
	    
	default:
//	    kprintf("Invalid value of coordpairtype in getcoordptrs: %d\n", cp);
	    break;
    }
    return;
    
}


#undef ConsoleDevice

Class *makeconsoleclass(struct ConsoleBase *ConsoleDevice)
{
    
   Class *cl;
   	
   cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct consoledata), 0UL);
   if (cl)
   {
    	cl->cl_Dispatcher.h_Entry = (APTR) AROS_ASMSYMNAME(dispatch_consoleclass);
    	cl->cl_Dispatcher.h_SubEntry = NULL;

    	cl->cl_UserData = (IPTR)ConsoleDevice;

   	return (cl);
    }
    return (NULL);
}

