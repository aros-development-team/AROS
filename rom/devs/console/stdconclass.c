/*
    (C) 1995-97 AROS - The Amiga Research OS
    $Id$

    Desc: Code for CONU_STANDARD console units.
    Lang: english
*/
#include <proto/graphics.h>
#include <proto/boopsi.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <aros/asmcall.h>
#include <string.h>

#include "console_gcc.h"
#include "consoleif.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

struct stdcondata
{
    struct DrawInfo *dri;
};


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)cl->cl_UserData)



/***********  StdCon::New()  **********************/

static Object *stdcon_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("StdCon::New()\n"));
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct stdcondata *data = INST_DATA(cl, o);
	ULONG dispmid = OM_DISPOSE;
	/* Clear for checking inside dispose() whether stuff was allocated.
	   Basically this is bug-prevention.
	*/
	memset(data, 0, sizeof (struct stdcondata));
	
	
	data->dri = GetScreenDrawInfo(CU(o)->cu_Window->WScreen);
	if (data->dri)
	{
	    CU(o)->cu_BgPen = data->dri->dri_Pens[BACKGROUNDPEN];
	    CU(o)->cu_FgPen = data->dri->dri_Pens[TEXTPEN];
	    
	    ReturnPtr("StdCon::New", Object *, o);
	}
    	CoerceMethodA(cl, o, (Msg)&dispmid);
    }
    ReturnPtr("StdCon::New", Object *, NULL);
    
}

/***********  StdCon::Dispose()  **************************/

static VOID stdcon_dispose(Class *cl, Object *o, Msg msg)
{
    struct stdcondata *data= INST_DATA(cl, o);
    if (data->dri)
    	FreeScreenDrawInfo(CU(o)->cu_Window->WScreen, data->dri);
	
    /* Let superclass free its allocations */
    DoSuperMethodA(cl, o, msg);
    
    return;
}



/*********  StdCon::DoCommand()  ****************************/

static VOID stdcon_docommand(Class *cl, Object *o, struct P_Console_DoCommand *msg)

{

    struct Window   *w  = CU(o)->cu_Window;
    struct RastPort *rp = w->RPort;
    UBYTE *params = msg->Params;
    struct stdcondata *data = INST_DATA(cl, o);
    
    EnterFunc(bug("StdCon::DoCommand(o=%p, cmd=%d, params=%p)\n",
    	o, msg->Command, params));
    switch (msg->Command)
    {
    case C_ASCII:
    	
    	D(bug("Writing char %c at (%d, %d)\n",
    		params[0], CP_X(o), CP_Y(o) + rp->Font->tf_Baseline));
    		
	Console_RenderCursor(o);

    	SetAPen(rp, CU(o)->cu_FgPen);
    	SetDrMd(rp, JAM2);
    	Move(rp, CP_X(o), CP_Y(o) + rp->Font->tf_Baseline);
    	Text(rp, &params[0], 1);
    	
    	Console_Right(o, 1);
	
	/* Rerender the cursor */
	Console_RenderCursor(o);

    	break;

    case C_FORMFEED: {
    	/* Clear the console */

        UBYTE oldpen = rp->FgPen;
        
    	SetAPen( rp, CU(o)->cu_BgPen );
    	RectFill(rp 
    		,CU(o)->cu_XROrigin
    		,CU(o)->cu_YROrigin
    		,CU(o)->cu_XRExtant
    		,CU(o)->cu_YRExtant);
    		
    	SetAPen(rp, oldpen);
    	
    } break;

    case C_BELL:
        /* !!! maybe trouble with LockLayers() here !!! */
//    	DisplayBeep(CU(o)->cu_Window->WScreen);
    	break;

    case C_BACKSPACE:
        Console_RenderCursor(o);
    	Console_Left(o, 1);
	Console_RenderCursor(o);
    	break;

    case C_CURSOR_BACKWARD:
        Console_RenderCursor(o);
        Console_Left(o, params[0]);
	Console_RenderCursor(o);
	break;

    case C_CURSOR_FORWARD:
        Console_RenderCursor(o);
        Console_Right(o, params[0]);
	Console_RenderCursor(o);
	break;
	
    case C_DELETE_CHAR: /* FIXME: can it have params!? */
        Console_RenderCursor(o);
	Console_ClearCell(o, XCCP, YCCP);
	Console_RenderCursor(o);
	break;
	
    case C_HTAB:
    	break;
	
    case C_LINEFEED:
    	D(bug("Got linefeed command\n"));
	Console_ClearCell(o, XCCP, YCCP);
    	Console_Down(o, 1);
	
	/* Check for linefeed mode (LF or LF+CR) */
	
	D(bug("conflags: %d\n", ICU(o)->conFlags));
	if (ICU(o)->conFlags & CF_LF_MODE_ON)
	{
	    UBYTE dummy;
	    /* Do carriage return */
	    Console_DoCommand(o, C_CARRIAGE_RETURN, &dummy);
	}
	Console_RenderCursor(o);
    	break;
	

/*    case C_VTAB:
    	break;
*/
    case C_CARRIAGE_RETURN:
    	
    	/* Goto start of line */
    	CU(o)->cu_XCP = CHAR_XMIN(o);
    	CU(o)->cu_XCCP = CHAR_XMIN(o);
/*     	Console_Down(o, 1); */
    	break;


    case C_INDEX:
    	Console_Down(o, 1);
    	break;

    case C_NEXT_LINE:
    	D(bug("Got NEXT LINE cmd\n"));
    	CU(o)->cu_XCP = CHAR_XMIN(o);
    	Console_Down(o, 1);
    	break;

    case C_REVERSE_IDX:
    	Console_Up(o, 1);
    	break;

    case C_CURSOR_POS:
    	break;

    case C_CURSOR_HTAB:
    	break;

    case C_ERASE_IN_LINE: {
    	UBYTE param = 1;
        UBYTE oldpen = rp->FgPen;
        
	Console_RenderCursor(o);

    	/* Clear till EOL */
        
    	SetAPen( rp, CU(o)->cu_BgPen );
	SetDrMd( rp, JAM2);
	
    	RectFill(rp 
    		,CU(o)->cu_XROrigin + CU(o)->cu_XCP * CU(o)->cu_XRSize
    		,CU(o)->cu_YROrigin + CU(o)->cu_YCP * CU(o)->cu_YRSize
    		,CU(o)->cu_XRExtant
    		,CU(o)->cu_YROrigin + (CU(o)->cu_YCP + 1) * CU(o)->cu_YRSize - 1);

    		
    	SetAPen(rp, oldpen);
    	
	Console_RenderCursor(o);
    	
    } break;
	
    case C_ERASE_IN_DISPLAY: {
    	UBYTE param = 1;
        UBYTE oldpen = rp->FgPen;
        
    	/* Clear till EOL */
    	Console_DoCommand(o, C_ERASE_IN_LINE, &param);
    	
    	/* Clear rest of area */

	Console_RenderCursor(o);
        
    	SetAPen( rp, CU(o)->cu_BgPen );
	SetDrMd( rp, JAM2);
	
    	RectFill(rp 
    		,CU(o)->cu_XROrigin
    		,CU(o)->cu_YROrigin + (CU(o)->cu_YCP + 1) * CU(o)->cu_YRSize
    		,CU(o)->cu_XRExtant
    		,CU(o)->cu_YRExtant);
    		
    	SetAPen(rp, oldpen);
    	
	Console_RenderCursor(o);
    	
    } break;

    case C_INSERT_LINE:
    	/* Scroll lines up */
//    	Scroll(unit, unit->cu_YCP, unit->cu_YMax, 1, ConsoleDevice);
    	break;
	
    case C_SCROLL_UP: {
        UBYTE oldpen = rp->FgPen;
	
    	D(bug("C_SCROLL_UP area (%d, %d) to (%d, %d), %d\n",
		GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o), - rp->Font->tf_YSize));
		
	SetAPen( rp, CU(o)->cu_BgPen );
#warning LockLayers problem here ?    
    	ScrollRaster(rp
		, 0
		, rp->Font->tf_YSize
		, GFX_XMIN(o)
		, GFX_YMIN(o)
		, GFX_XMAX(o)
		, GFX_YMAX(o) );
	SetAPen(rp, oldpen);
	
	
	break; }

/*    case C_:
    	break;

    case C_:
    	break;

    case C_:
    	break;

    case C_:
    	break;
*/
    default:
    	DoSuperMethodA(cl, o, (Msg)msg);
	break;
    }

    ReturnVoid("StdCon::DoCommand");
}

/*********  StdCon::RenderCursor()  ****************************/
static VOID stdcon_rendercursor(Class *cl, Object *o, struct P_Console_RenderCursor *msg)
{
     struct RastPort *rp = RASTPORT(o);
     struct stdcondata *data = INST_DATA(cl, o);
     
     /* SetAPen(rp, data->dri->dri_Pens[FILLPEN]); */
     SetDrMd(rp, COMPLEMENT);
     RectFill(rp
     	, CP_X(o)
	, CP_Y(o)
	, CP_X(o) + rp->Font->tf_XSize - 1
	, CP_Y(o) + rp->Font->tf_YSize - 1
     );
     SetDrMd(rp, JAM2);
}

/**************************
**  StdCon::ClearCell()  **
**************************/
static VOID stdcon_clearcell(Class *cl, Object *o, struct P_Console_ClearCell *msg)
{
     struct RastPort *rp = RASTPORT(o);
     struct stdcondata *data = INST_DATA(cl, o);
     
     SetAPen(rp, data->dri->dri_Pens[BACKGROUNDPEN]);
     SetDrMd(rp, JAM1);
     RectFill(rp
     	, GFX_X(o, msg->X)
	, GFX_Y(o, msg->Y)
	, GFX_X(o, msg->X) + rp->Font->tf_XSize - 1
	, GFX_Y(o, msg->Y) + rp->Font->tf_YSize - 1
     );
}

/*******************************
**  StdCon::NewWindowSize()  **
*******************************/
static VOID stdcon_newwindowsize(Class *cl, Object *o, struct P_Console_NewWindowSize *msg)
{
    struct RastPort *rp = RASTPORT(o);
    struct stdcondata *data = INST_DATA(cl, o);
    WORD old_xmax = CHAR_XMAX(o);
    WORD old_ymax = CHAR_YMAX(o);
    WORD old_xcp = XCP;
    WORD old_ycp = YCP;
    
    WORD x1, y1, x2, y2;
    
    DoSuperMethodA(cl, o, (Msg)msg);
    
    if (CHAR_XMAX(o) < old_xmax)
    {
        x1 = GFX_XMAX(o) + 1;
	y1 = GFX_YMIN(o);
	x2 = WINDOW(o)->Width - WINDOW(o)->BorderRight - 1;
	y2 = WINDOW(o)->Height - WINDOW(o)->BorderBottom - 1;
	
	if ((x2 >= x1) && (y2 >= y1))
	{
	    SetAPen(rp, 0);
	    SetDrMd(rp, JAM2),
	    RectFill(rp, x1, y1, x2, y2);
	}
    }

    if (CHAR_YMAX(o) < old_ymax)
    {
        x1 = GFX_XMIN(o);
	y1 = GFX_YMAX(o) + 1;
	x2 = WINDOW(o)->Width - WINDOW(o)->BorderRight - 1;
	y2 = WINDOW(o)->Height - WINDOW(o)->BorderBottom - 1;
	
	if ((x2 >= x1) && (y2 >= y1))
	{
	    SetAPen(rp, 0);
	    SetDrMd(rp, JAM2),
	    RectFill(rp, x1, y1, x2, y2);
	}
    }

    if ((old_xcp != XCP) || (old_ycp != YCP)) 
    {
        SetAPen(rp, 0);
	SetDrMd(rp, JAM2);
	RectFill(rp, GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));
	
    	Console_RenderCursor(o);
    }
    return;
}



AROS_UFH3S(IPTR, dispatch_stdconclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    
    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = (IPTR)stdcon_new(cl, o, (struct opSet *)msg);
	break;
	
    case OM_DISPOSE:
    	stdcon_dispose(cl, o, msg);
	break;
	
    case M_Console_DoCommand:
    	stdcon_docommand(cl, o, (struct P_Console_DoCommand *)msg);
	break;
	
    case M_Console_RenderCursor:
    	stdcon_rendercursor(cl, o, (struct P_Console_RenderCursor *)msg);
	break;
	
    case M_Console_ClearCell:
    	stdcon_clearcell(cl, o, (struct P_Console_ClearCell *)msg);
	break;
	
    case M_Console_NewWindowSize:
        stdcon_newwindowsize(cl, o, (struct P_Console_NewWindowSize *)msg);
	break;
	
    default:
    	retval = DoSuperMethodA(cl, o, msg);
	break;
    }
    
    return retval;
    
}

#undef ConsoleDevice

Class *makeStdConClass(struct ConsoleBase *ConsoleDevice)
{
    
   Class *cl;
   	
   cl = MakeClass(NULL, NULL ,CONSOLECLASSPTR , sizeof(struct stdcondata), 0UL);
   if (cl)
   {
    	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_stdconclass;
    	cl->cl_Dispatcher.h_SubEntry = NULL;

    	cl->cl_UserData = (IPTR)ConsoleDevice;

   	return (cl);
    }
    return (NULL);
}

