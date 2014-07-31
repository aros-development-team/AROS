/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code for CONU_STANDARD console units.
    Lang: english
*/
#include <string.h>

#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <exec/rawfmt.h>
#include <aros/asmcall.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include "console_gcc.h"
#include "consoleif.h"


struct stdcondata
{
    struct DrawInfo *dri;
    WORD rendercursorcount;
    BOOL cursorvisible;

    /* Libraries */
    struct Library *scd_GfxBase;
};


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)cl->cl_UserData)

/***********  StdCon::New()  **********************/

static Object *stdcon_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("StdCon::New()\n"));
    o = (Object *) DoSuperMethodA(cl, o, (Msg) msg);
    if (o)
    {
        struct stdcondata *data = INST_DATA(cl, o);
        ULONG dispmid = OM_DISPOSE;
        /* Clear for checking inside dispose() whether stuff was allocated.
           Basically this is bug-prevention.
         */
        memset(data, 0, sizeof(struct stdcondata));

        data->scd_GfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
        if (data->scd_GfxBase)
        {
            data->dri = GetScreenDrawInfo(CU(o)->cu_Window->WScreen);
            if (data->dri)
            {
                CU(o)->cu_BgPen = data->dri->dri_Pens[BACKGROUNDPEN];
                CU(o)->cu_FgPen = data->dri->dri_Pens[TEXTPEN];

                data->cursorvisible = TRUE;
                Console_RenderCursor(o);

                ReturnPtr("StdCon::New", Object *, o);
            }
        }
        CoerceMethodA(cl, o, (Msg) &dispmid);
    }
    ReturnPtr("StdCon::New", Object *, NULL);

}

/***********  StdCon::Dispose()  **************************/

static VOID stdcon_dispose(Class *cl, Object *o, Msg msg)
{
    struct stdcondata *data = INST_DATA(cl, o);

    if (data->scd_GfxBase)
        CloseLibrary(data->scd_GfxBase);

    if (data->dri)
        FreeScreenDrawInfo(CU(o)->cu_Window->WScreen, data->dri);

    /* Let superclass free its allocations */
    DoSuperMethodA(cl, o, msg);

    return;
}

VOID setabpen(struct Library *GfxBase, struct RastPort *rp, UBYTE tflags,
    UBYTE FgPen, UBYTE BgPen)
{
    UBYTE fg = FgPen, bg = BgPen;
    UBYTE style = JAM2;

    if (tflags & CON_TXTFLAGS_CONCEALED)
        fg = bg;
    else if (tflags & CON_TXTFLAGS_REVERSED)
        style |= INVERSVID;
    SetABPenDrMd(rp, fg, bg, style);
}

static void setstyle(struct Library *GfxBase, struct RastPort *rp,
    Object *o)
{
    UBYTE tflags = CU(o)->cu_TxFlags;
    setabpen(GfxBase, rp, tflags, CU(o)->cu_FgPen, CU(o)->cu_BgPen);
    SetSoftStyle(rp, tflags, CON_TXTFLAGS_MASK);
}

/*********  StdCon::DoCommand()  ****************************/

static VOID stdcon_docommand(Class *cl, Object *o,
    struct P_Console_DoCommand *msg)
{
    struct Window *w = CU(o)->cu_Window;
    struct RastPort *rp = w->RPort;
    IPTR *params = msg->Params;
    struct stdcondata *data = INST_DATA(cl, o);
    struct Library *GfxBase = data->scd_GfxBase;

    EnterFunc(bug("StdCon::DoCommand(o=%p, cmd=%d, params=%p)\n",
            o, msg->Command, params));

    switch (msg->Command)
    {
    case C_NIL:
        /* do nothing */
        break;

    case C_ASCII:

        D(bug("Writing char %c at (%d, %d)\n",
                params[0], CP_X(o), CP_Y(o) + rp->Font->tf_Baseline));

        Console_UnRenderCursor(o);

        setstyle(GfxBase, rp, o);
        Move(rp, CP_X(o), CP_Y(o) + rp->Font->tf_Baseline);
        {
            UBYTE c = params[0];
            Text(rp, &c, 1);
        }

        Console_Right(o, 1);

        /* Rerender the cursor */
        Console_RenderCursor(o);

        break;

    case C_ASCII_STRING:
        D(bug("Writing string %.*s at (%d, %d)\n",
                (int)params[1], (char *)params[0], CP_X(o),
                CP_Y(o) + rp->Font->tf_Baseline));

        Console_UnRenderCursor(o);

        setstyle(GfxBase, rp, o);

        {
            ULONG len = params[1];
            STRPTR str = (STRPTR) params[0];

            while (len)
            {
                ULONG remaining_space = CHAR_XMAX(o) + 1 - XCCP;
                ULONG line_len =
                    len < remaining_space ? len : remaining_space;

                Move(rp, CP_X(o), CP_Y(o) + rp->Font->tf_Baseline);
                Text(rp, str, line_len);

                Console_Right(o, line_len);

                len -= line_len;
                str += line_len;
            }
        }

        /* Rerender the cursor */
        Console_RenderCursor(o);

        break;

    case C_FORMFEED:
        {
            /* Clear the console */

            UBYTE oldpen = rp->FgPen;
            IPTR newcurpos[2] = { 0, 0 };

            Console_UnRenderCursor(o);

            SetAPen(rp, CU(o)->cu_BgPen);
            RectFill(rp, CU(o)->cu_XROrigin, CU(o)->cu_YROrigin,
                CU(o)->cu_XRExtant, CU(o)->cu_YRExtant);

            SetAPen(rp, oldpen);

            Console_DoCommand(o, C_CURSOR_POS, 2, newcurpos);

            Console_RenderCursor(o);

            break;
        }

    case C_BELL:
        /* !!! maybe trouble with LockLayers() here !!! */
//      DisplayBeep(CU(o)->cu_Window->WScreen);
        break;

    case C_BACKSPACE:
        Console_UnRenderCursor(o);
        Console_Left(o, 1);
        Console_RenderCursor(o);
        break;

    case C_CURSOR_BACKWARD:
        Console_UnRenderCursor(o);
        Console_Left(o, params[0]);
        Console_RenderCursor(o);
        break;

    case C_CURSOR_FORWARD:
        Console_UnRenderCursor(o);
        Console_Right(o, params[0]);
        Console_RenderCursor(o);
        break;

    case C_DELETE_CHAR:        /* FIXME: can it have params!? */
        {
            UBYTE oldpen = rp->FgPen;
            Console_UnRenderCursor(o);
            SetAPen(rp, CU(o)->cu_BgPen);
            ScrollRaster(rp,
                XRSIZE,
                0,
                GFX_X(o, XCP),
                GFX_Y(o, YCP), GFX_XMAX(o), GFX_Y(o, YCP + 1));
            SetAPen(rp, oldpen);
            Console_RenderCursor(o);
        }
        break;
        Console_RenderCursor(o);
        break;

    case C_HTAB:
        {
            WORD x = XCCP, i = 0;

            while ((CU(o)->cu_TabStops[i] != (UWORD) - 1) &&
                (CU(o)->cu_TabStops[i] <= x))
            {
                i++;
            }
            if (CU(o)->cu_TabStops[i] != (UWORD) - 1)
            {
                Console_UnRenderCursor(o);
                Console_Right(o, CU(o)->cu_TabStops[i] - x);
                Console_RenderCursor(o);
            }
            break;
        }

    case C_CURSOR_HTAB:
        {
            WORD i = params[0];

            do
            {
                IPTR dummy;

                Console_DoCommand(o, C_HTAB, 0, &dummy);

            }
            while (--i > 0);
            break;
        }

    case C_CURSOR_BACKTAB:
        {
            WORD count = params[0];

            Console_UnRenderCursor(o);

            do
            {
                WORD x = XCCP, i = 0;

                while ((CU(o)->cu_TabStops[i] != (UWORD) - 1) &&
                    (CU(o)->cu_TabStops[i] < x))
                {
                    i++;
                }

                i--;

                if (i >= 0)
                    if (CU(o)->cu_TabStops[i] != (UWORD) - 1)
                    {
                        Console_Left(o, x - CU(o)->cu_TabStops[i]);
                    }

            }
            while (--count > 0);

            Console_RenderCursor(o);

            break;
        }

    case C_LINEFEED:
        D(bug("Got linefeed command\n"));
        /*Console_ClearCell(o, XCCP, YCCP); */
        Console_UnRenderCursor(o);

        Console_Down(o, 1);

        /* Check for linefeed mode (LF or LF+CR) */

        D(bug("conflags: %d\n", ICU(o)->conFlags));

        /* if (ICU(o)->conFlags & CF_LF_MODE_ON) */
        if (CHECK_MODE(o, M_LNM))
        {
            CU(o)->cu_XCP = CHAR_XMIN(o);
            CU(o)->cu_XCCP = CHAR_XMIN(o);
        }
        Console_RenderCursor(o);
        break;

    case C_CURSOR_PREV_LINE:
        Console_UnRenderCursor(o);
        CU(o)->cu_XCP = CHAR_XMIN(o);
        CU(o)->cu_XCCP = CHAR_XMIN(o);
        Console_Up(o, 1);
        Console_RenderCursor(o);
        break;

    case C_CURSOR_UP:
    case C_VTAB:
        Console_UnRenderCursor(o);
        Console_Up(o, 1);
        Console_RenderCursor(o);
        break;

    case C_CURSOR_NEXT_LINE:
        Console_UnRenderCursor(o);
        CU(o)->cu_XCP = CHAR_XMIN(o);
        CU(o)->cu_XCCP = CHAR_XMIN(o);
        Console_Down(o, 1);
        Console_RenderCursor(o);
        break;

    case C_CURSOR_DOWN:
        Console_UnRenderCursor(o);
        Console_Down(o, 1);
        Console_RenderCursor(o);
        break;

    case C_CARRIAGE_RETURN:
        /* Goto start of line */

        Console_UnRenderCursor(o);
        CU(o)->cu_XCP = CHAR_XMIN(o);
        CU(o)->cu_XCCP = CHAR_XMIN(o);
        Console_RenderCursor(o);
        break;

    case C_INDEX:
        Console_Down(o, 1);
        break;

    case C_NEXT_LINE:
        D(bug("Got NEXT LINE cmd\n"));
        Console_Down(o, 1);
        Console_Left(o, XCP);
        break;

    case C_REVERSE_IDX:
        Console_Up(o, 1);
        break;

    case C_CURSOR_POS:
        {
            WORD y = ((WORD) params[0]) - 1;
            WORD x = ((WORD) params[1]) - 1;

            if (x < CHAR_XMIN(o))
            {
                x = CHAR_XMIN(o);
            }
            else if (x > CHAR_XMAX(o))
            {
                x = CHAR_XMAX(o);
            }

            if (y < CHAR_YMIN(o))
            {
                y = CHAR_YMIN(o);
            }
            else if (y > CHAR_YMAX(o))
            {
                y = CHAR_YMAX(o);
            }

            Console_UnRenderCursor(o);

            XCCP = XCP = x;
            YCCP = YCP = y;

            Console_RenderCursor(o);
            break;
        }

    case C_ERASE_IN_LINE:
        {
            /*UBYTE param = 1; */
            UBYTE oldpen = rp->FgPen;

            Console_UnRenderCursor(o);

            /* Clear till EOL */

            SetAPen(rp, CU(o)->cu_BgPen);
            SetDrMd(rp, JAM2);

            RectFill(rp, CU(o)->cu_XROrigin + XCP * XRSIZE,
                CU(o)->cu_YROrigin + YCP * YRSIZE, CU(o)->cu_XRExtant,
                CU(o)->cu_YROrigin + (YCP + 1) * YRSIZE - 1);


            SetAPen(rp, oldpen);

            Console_RenderCursor(o);

        }
        break;

    case C_ERASE_IN_DISPLAY:
        {
            IPTR param = 1;
            UBYTE oldpen = rp->FgPen;

            /* Clear till EOL */
            Console_DoCommand(o, C_ERASE_IN_LINE, 1, &param);

            /* Clear rest of area */

            Console_UnRenderCursor(o);

            SetAPen(rp, CU(o)->cu_BgPen);
            SetDrMd(rp, JAM2);

            RectFill(rp, CU(o)->cu_XROrigin,
                CU(o)->cu_YROrigin + (YCP + 1) * YRSIZE, CU(o)->cu_XRExtant,
                CU(o)->cu_YRExtant);

            SetAPen(rp, oldpen);

            Console_RenderCursor(o);

            break;
        }

    case C_INSERT_CHAR:
        {
            UBYTE oldpen = rp->FgPen;
            Console_UnRenderCursor(o);
            SetAPen(rp, CU(o)->cu_BgPen);
            ScrollRaster(rp,
                -XRSIZE,
                0,
                GFX_X(o, XCP),
                GFX_Y(o, YCP), GFX_XMAX(o), GFX_Y(o, YCP + 1));
            SetAPen(rp, oldpen);
            Console_RenderCursor(o);
        }
        break;

    case C_INSERT_LINE:
        {
            UBYTE oldpen = rp->FgPen;

            Console_UnRenderCursor(o);
            SetAPen(rp, CU(o)->cu_BgPen);

            ScrollRaster(rp,
                0,
                -YRSIZE,
                GFX_XMIN(o), GFX_Y(o, YCP), GFX_XMAX(o), GFX_YMAX(o));

            SetAPen(rp, oldpen);

            Console_RenderCursor(o);
            break;
        }

    case C_DELETE_LINE:
        {
            UBYTE oldpen = rp->FgPen;

            Console_UnRenderCursor(o);
            SetAPen(rp, CU(o)->cu_BgPen);

            ScrollRaster(rp,
                0,
                YRSIZE,
                GFX_XMIN(o), GFX_Y(o, YCP), GFX_XMAX(o), GFX_YMAX(o));

            SetAPen(rp, oldpen);

            Console_RenderCursor(o);
            break;
        }

    case C_SCROLL_UP:
        {
            UBYTE oldpen = rp->FgPen;

            D(bug("C_SCROLL_UP area (%d, %d) to (%d, %d), %d\n",
                    GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o),
                    YRSIZE * params[0]));

            Console_UnRenderCursor(o);

            SetAPen(rp, CU(o)->cu_BgPen);
/* FIXME: LockLayers problem here ? */
            ScrollRaster(rp, 0, YRSIZE * params[0], GFX_XMIN(o),
                GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));
            SetAPen(rp, oldpen);

            Console_RenderCursor(o);

            break;
        }

    case C_SCROLL_DOWN:
        {
            UBYTE oldpen = rp->FgPen;

            D(bug("C_SCROLL_DOWN area (%d, %d) to (%d, %d), %d\n",
                    GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o),
                    YRSIZE * params[0]));

            Console_UnRenderCursor(o);

            SetAPen(rp, CU(o)->cu_BgPen);
/* FIXME: LockLayers problem here?     */
            ScrollRaster(rp, 0, -YRSIZE * params[0], GFX_XMIN(o),
                GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));
            SetAPen(rp, oldpen);

            Console_RenderCursor(o);

            break;
        }

    case C_CURSOR_VISIBLE:
        if (!data->cursorvisible)
        {
            data->cursorvisible = TRUE;
            data->rendercursorcount--;
            Console_RenderCursor(o);
        }
        break;

    case C_CURSOR_INVISIBLE:
        if (data->cursorvisible)
        {
            Console_UnRenderCursor(o);
            data->cursorvisible = FALSE;
            data->rendercursorcount++;
        }
        break;

    case C_SET_TOP_OFFSET:
        Console_UnRenderCursor(o);
        CU(o)->cu_YROrigin = params[0];
        CU(o)->cu_YMax =
            (w->Height - (CU(o)->cu_YROrigin +
                w->BorderBottom)) / CU(o)->cu_YRSize - 1;
        Console_RenderCursor(o);
        Console_NewWindowSize(o);
        break;

    case C_SET_PAGE_LENGTH:
        Console_UnRenderCursor(o);
        CU(o)->cu_YMax = params[0];
        // FIXME: Need to set something that prevents NewWindowSize to
        // change YMax
        Console_RenderCursor(o);
        Console_NewWindowSize(o);
        break;

    case C_WINDOW_STATUS_REQUEST:
        {
            UBYTE reply[32];
            NewRawDoFmt("\x9b" "1;1;%d;%d r", RAWFMTFUNC_STRING, reply,
                CU(o)->cu_YMax, CU(o)->cu_XMax);
            con_inject((struct ConsoleBase *)cl->cl_UserData, CU(o), reply,
                -1);
            break;
        }

    case C_DEVICE_STATUS_REPORT:
        {
            UBYTE reply[32];
            NewRawDoFmt("\x9b" "%d;%dR", RAWFMTFUNC_STRING, reply,
                CU(o)->cu_YCP, CU(o)->cu_XCP);
            con_inject((struct ConsoleBase *)cl->cl_UserData, CU(o), reply,
                -1);
            break;
        }

    default:
        DoSuperMethodA(cl, o, (Msg) msg);
        break;
    }

    ReturnVoid("StdCon::DoCommand");
}

/*********  StdCon::RenderCursor()  ****************************/
static VOID stdcon_rendercursor(Class *cl, Object *o,
    struct P_Console_RenderCursor *msg)
{
    struct RastPort *rp = RASTPORT(o);
    struct stdcondata *data = INST_DATA(cl, o);
    struct Library *GfxBase = data->scd_GfxBase;

    /* SetAPen(rp, data->dri->dri_Pens[FILLPEN]); */

    data->rendercursorcount++;

    if (data->cursorvisible && (data->rendercursorcount == 1))
    {
        SetDrMd(rp, COMPLEMENT);
        RectFill(rp, CP_X(o), CP_Y(o), CP_X(o) + XRSIZE - 1,
            CP_Y(o) + YRSIZE - 1);
        SetDrMd(rp, JAM2);
    }
}

/*********  StdCon::UnRenderCursor()  ****************************/
static VOID stdcon_unrendercursor(Class *cl, Object *o,
    struct P_Console_UnRenderCursor *msg)
{
    struct RastPort *rp = RASTPORT(o);
    struct stdcondata *data = INST_DATA(cl, o);
    struct Library *GfxBase = data->scd_GfxBase;

    data->rendercursorcount--;

    /* SetAPen(rp, data->dri->dri_Pens[FILLPEN]); */

    if (data->cursorvisible && (data->rendercursorcount == 0))
    {
        SetDrMd(rp, COMPLEMENT);
        RectFill(rp, CP_X(o), CP_Y(o), CP_X(o) + XRSIZE - 1,
            CP_Y(o) + YRSIZE - 1);
        SetDrMd(rp, JAM2);
    }
}

/**************************
**  StdCon::ClearCell()  **
**************************/
static VOID stdcon_clearcell(Class *cl, Object *o,
    struct P_Console_ClearCell *msg)
{
    struct RastPort *rp = RASTPORT(o);
    struct stdcondata *data = INST_DATA(cl, o);
    struct Library *GfxBase = data->scd_GfxBase;

    SetAPen(rp, data->dri->dri_Pens[BACKGROUNDPEN]);
    SetDrMd(rp, JAM1);
    RectFill(rp, GFX_X(o, msg->X), GFX_Y(o, msg->Y), GFX_X(o,
            msg->X) + XRSIZE - 1, GFX_Y(o, msg->Y) + YRSIZE - 1);
}

/*******************************
**  StdCon::NewWindowSize()  **
*******************************/
static VOID stdcon_newwindowsize(Class *cl, Object *o,
    struct P_Console_NewWindowSize *msg)
{
    struct RastPort *rp = RASTPORT(o);
    struct stdcondata *data = INST_DATA(cl, o);
    struct Library *GfxBase = data->scd_GfxBase;
    WORD old_xmax = CHAR_XMAX(o);
    WORD old_ymax = CHAR_YMAX(o);
    WORD old_xcp = XCP;
    WORD old_ycp = YCP;

    WORD x1, y1, x2, y2;

    DoSuperMethodA(cl, o, (Msg) msg);

    if (CHAR_XMAX(o) < old_xmax)
    {
        x1 = GFX_XMAX(o) + 1;
        y1 = GFX_YMIN(o);
        x2 = GFX_XMAX(o);
        y2 = GFX_YMAX(o) - 1;

        if ((x2 >= x1) && (y2 >= y1))
        {
            SetAPen(rp, 0);
            SetDrMd(rp, JAM2), RectFill(rp, x1, y1, x2, y2);
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
            SetDrMd(rp, JAM2), RectFill(rp, x1, y1, x2, y2);
        }
    }

    if ((old_xcp != XCP) || (old_ycp != YCP))
    {
        SetAPen(rp, 0);
        SetDrMd(rp, JAM2);
        RectFill(rp, GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));
        data->rendercursorcount--;
        Console_RenderCursor(o);
    }
    return;
}



AROS_UFH3S(IPTR, dispatch_stdconclass,
    AROS_UFHA(Class *, cl, A0),
    AROS_UFHA(Object *, o, A2), AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = (IPTR) stdcon_new(cl, o, (struct opSet *)msg);
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

    case M_Console_UnRenderCursor:
        stdcon_unrendercursor(cl, o,
            (struct P_Console_UnRenderCursor *)msg);
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

    AROS_USERFUNC_EXIT
}

#undef ConsoleDevice

Class *makeStdConClass(struct ConsoleBase *ConsoleDevice)
{
    Class *cl;

    cl = MakeClass(NULL, NULL, CONSOLECLASSPTR, sizeof(struct stdcondata),
        0UL);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry = (APTR) dispatch_stdconclass;
        cl->cl_Dispatcher.h_SubEntry = NULL;

        cl->cl_UserData = (IPTR) ConsoleDevice;

        return (cl);
    }
    return NULL;
}
