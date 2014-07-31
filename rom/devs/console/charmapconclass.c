/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code for CONU_CHARMAP console units.
    Lang: english
*/

#include <string.h>

#include <exec/ports.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/alib.h>
#include <intuition/intuition.h>

#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/sghooks.h>
#include <libraries/gadtools.h>

#include <graphics/rastport.h>
#include <aros/asmcall.h>

#include <stdlib.h>

#define SDEBUG 0
//#define DEBUG 1
#define DEBUG 0
#include <aros/debug.h>

#include "console_gcc.h"
#include "consoleif.h"

#include "charmap.h"

#define	PROP_FLAGS \
	AUTOKNOB | FREEVERT | PROPNEWLOOK | PROPBORDERLESS

#define CODE_COPY	'C'
#define CODE_PASTE	'V'

struct MyEditHookMsg
{
    struct Message msg;
    struct SGWork *sgw;
    WORD code;
};

static const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";
struct Scroll
{
    struct Gadget scroller;     /* proportionnal gadget */
    struct Gadget down;         /* down gadget */
    struct Gadget up;           /* up gadget */
    struct PropInfo pinfo;      /* PropInfo for scroller */
    struct Image simage;        /* image for scroller */
    struct Image *upimage;      /* Boopsi image for up arrow */
    struct Image *downimage;    /* ditto for down arrow */
};

// FIXME: Abstract out the non-GUI aspects
struct charmapcondata
{
    /* Start of the scrollback buffer */
    struct charmap_line *top_of_scrollback;
    /* The line currently displayed at the top of the screen */
    struct charmap_line *top_of_window;
    /* Saved position for the top of the screen at the end of 
       the buffer; where the buffer is reset to if there is 
       output while scrolling */
    struct charmap_line *saved_top_of_window;
    ULONG saved_scrollback_pos;

    ULONG scrollback_size;      /* Total size of the scrollback buffer */
    ULONG scrollback_pos;       /* Position of the top of the window */

    ULONG scrollback_max;       /* Maximum number of lines in scrollback
                                   buffer on top of CHAR_YMAX(o) */

    BOOL unrendered;            /* Unrendered cursor while scrolled back? */

    /* FIXME: Belongs in snipmap class */
    /* Current selection */
    LONG select_x_min;
    LONG select_y_min;
    struct charmap_line *select_line_min;
    LONG select_x_max;
    LONG select_y_max;
    struct charmap_line *select_line_max;
    BOOL active_selection;      /* If true, mouse move will affect the selection */
    BOOL ignore_drag;


    UBYTE boopsigad;            /* Type of right prop gadget of window */
    struct Scroll *prop;

    /* Active gadget */
    APTR activeGad;

    /* Libraries */
    struct Library *ccd_GfxBase;
};


/* Template used to quickly fill constant fields */
CONST struct Scroll ScrollBar = {
    {
        /* PropGadget */
        NULL, 0, 0, 0, 0,
        GFLG_RELRIGHT | GFLG_RELHEIGHT,
        GACT_RIGHTBORDER | GACT_FOLLOWMOUSE | GACT_IMMEDIATE |
        GACT_RELVERIFY,
        GTYP_PROPGADGET,
        NULL, NULL, NULL, 0, NULL,
        0, NULL
    },
    {
        /* Up-gadget */
        NULL, 0, 0, 0, 0,
        GFLG_RELRIGHT | GFLG_RELBOTTOM | GFLG_GADGHIMAGE |
        GFLG_GADGIMAGE,
        GACT_RIGHTBORDER | GACT_RELVERIFY | GACT_IMMEDIATE,
        GTYP_BOOLGADGET,
        NULL, NULL, NULL, 0, NULL,
        1, NULL
    },
    {
        /* Down-Gadget */
        NULL, 0, 0, 0, 0,
        GFLG_RELRIGHT | GFLG_RELBOTTOM | GFLG_GADGHIMAGE |
        GFLG_GADGIMAGE,
        GACT_RIGHTBORDER | GACT_RELVERIFY | GACT_IMMEDIATE,
        GTYP_BOOLGADGET,
        NULL, NULL, NULL, 0, NULL,
        2, NULL
    },
    {
        /* PropInfo */
        PROP_FLAGS,
        MAXPOT, MAXPOT,
        MAXBODY, MAXBODY
    }
    /* Other values may be NULL */
};


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)cl->cl_UserData)

#undef GfxBase
#define GfxBase (((struct charmapcondata *)INST_DATA(cl, o))->ccd_GfxBase)

static VOID charmapcon_refresh(Class *cl, Object *o, LONG off);

/*** Allocate and attach a prop gadget to the window ***/
static VOID charmapcon_add_prop(Class *cl, Object *o)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct Scroll *pg;
    struct Image *dummy;
    struct Window *win = CU(o)->cu_Window;

    UWORD height, size_width, size_height;

    /* If the window is a backdrop'ed one, use a simplified BOOPSI propgadget
     * because the next propgadget aspect depends on window activated state */
    if (win->Flags & WFLG_BACKDROP)
    {
        /* Yes this is actually a (struct Gadget *)... */
        if ((data->prop = pg =
                (struct Scroll *)NewObject(NULL, "propgclass", GA_Top, 0,
                    GA_Left, win->Width - 10, GA_Width, 10, GA_Height,
                    win->Height, GA_RelVerify, TRUE, GA_FollowMouse, TRUE,
                    GA_Immediate, TRUE, PGA_VertPot, MAXPOT, PGA_VertBody,
                    MAXBODY, PGA_Freedom, FREEVERT, PGA_NewLook, TRUE,
                    TAG_END)))
        {
            /* And finally, add it to the window */
            AddGList(win, (struct Gadget *)pg, 0, 1, NULL);
            RefreshGList((struct Gadget *)pg, win, NULL, 1);
        }

        data->boopsigad = TRUE;
        return;
    }
    data->boopsigad = FALSE;

    /* Get memory */
    if ((data->prop = pg = (void *)AllocMem(sizeof(*pg), MEMF_PUBLIC)))
    {
        /* Copy default flags/modes/etc. */
        CopyMem(&ScrollBar, pg, sizeof(*pg));
        pg->pinfo.Flags = PROP_FLAGS;
        struct DrawInfo *di;

        di = (void *)GetScreenDrawInfo(win->WScreen);

        /* We need to get size-gadget height, to adjust properly arrows */
        if ((dummy = (struct Image *)NewObject(NULL, "sysiclass",
                    SYSIA_Which, SIZEIMAGE,
                    SYSIA_DrawInfo, (IPTR) di, TAG_END)))
        {
            size_width = dummy->Width;  /* width of up/down-gadgets */
            size_height = dummy->Height;        /* bottom offset */

            /* We don't need the image anymore */
            DisposeObject(dummy);

            /* Get the boopsi image of the up and down arrow */
            if ((pg->upimage = (struct Image *)NewObject(NULL, "sysiclass",
                        SYSIA_Which, UPIMAGE,
                        SYSIA_DrawInfo, (IPTR) di, TAG_END)))
            {
                pg->up.GadgetRender = pg->up.SelectRender =
                    (APTR) pg->upimage;
                height = pg->upimage->Height;

                if ((pg->downimage =
                        (struct Image *)NewObject(NULL, "sysiclass",
                            SYSIA_Which, DOWNIMAGE, SYSIA_DrawInfo,
                            (IPTR) di, TAG_END)))
                {
                    struct Gadget *G = (void *)pg;
                    WORD hoffset = size_width / 4;

                    pg->down.GadgetRender = pg->down.SelectRender =
                        (APTR) pg->downimage;

                    /* Release drawinfo */
                    FreeScreenDrawInfo(win->WScreen, di);

                    /* Now init all sizes/positions relative to window's
                     * borders */
                    G->Height =
                        -(win->BorderTop + size_height + 2 * height + 2);
                    G->TopEdge = win->BorderTop + 1;
                    G->Width = size_width - hoffset * 2 + 2;
                    G->LeftEdge = -(size_width - hoffset);
                    G++;
                    pg->up.LeftEdge = G->LeftEdge = -(size_width - 1);
                    G->Width = pg->up.Width = size_width;
                    G->Height = pg->up.Height = height;
                    G->TopEdge = -(size_height + height - 1);
                    pg->up.TopEdge = G->TopEdge - height;

                    /* Other fields */
                    pg->scroller.GadgetRender = (APTR) &pg->simage;
                    pg->scroller.SpecialInfo = (APTR) &pg->pinfo;

                    /* Link gadgets */
                    pg->scroller.NextGadget = &pg->up;
                    pg->up.NextGadget = &pg->down;

                    /* And finally, add them to the window */
                    AddGList(win, &pg->scroller, 0, 3, NULL);
                    RefreshGList(&pg->scroller, win, NULL, 3);

                    return;
                }
                DisposeObject(pg->upimage);
            }
        }
        FreeMem(pg, sizeof(*pg));
        FreeScreenDrawInfo(win->WScreen, di);
    }
    return;
}

static VOID charmapcon_adj_prop(Class *cl, Object *o)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct Window *w = CU(o)->cu_Window;
    ULONG VertBody, VertPot;

    ULONG hidden =
        data->scrollback_size >
        CHAR_YMAX(o) ? data->scrollback_size - CHAR_YMAX(o) - 1 : 0;

    if (hidden > 0)
    {
        VertPot = (data->scrollback_pos) * MAXPOT / hidden;
        VertBody = CHAR_YMAX(o) * MAXBODY / data->scrollback_size;
    }
    else
    {
        VertPot = 0;
        VertBody = MAXBODY;
    }

    if (VertPot > MAXPOT)
    {
        VertPot = MAXPOT;
        D(bug("VERTPOT SET TOO HIGH. Adjusted\n"));
    }

    NewModifyProp((struct Gadget *)&(data->prop->scroller), w, NULL,
        ((struct PropInfo *)data->prop->scroller.SpecialInfo)->Flags,
        MAXPOT, VertPot, MAXBODY, VertBody, 1);
}

/*** Free resources allocated for scroller ***/
void charmapcon_free_prop(Class *cl, Object *o)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct Window *win = CU(o)->cu_Window;

    if (data->prop)
    {
        if (win)
        {
            if (data->boopsigad)
                RemoveGadget(win, (struct Gadget *)data->prop);
            else
                RemoveGList(win, &data->prop->scroller, 3);
        }
        if (data->boopsigad)
            DisposeObject(data->prop);
        else
        {
            /* Free elements */
            DisposeObject(data->prop->upimage);
            DisposeObject(data->prop->downimage);

            /* Free struct */
            FreeMem(data->prop, sizeof(*data->prop));
        }
    }
}



/***********  CharMapCon::New()  **********************/

static Object *charmapcon_new(Class *cl, Object *o, struct opSet *msg)
{
    EnterFunc(bug("CharMapCon::New()\n"));
    APTR newGfxBase = TaggedOpenLibrary(TAGGEDOPEN_GRAPHICS);
    if (newGfxBase == NULL)
        return NULL;

    o = (Object *) DoSuperMethodA(cl, o, (Msg) msg);
    if (o)
    {
        struct charmapcondata *data = INST_DATA(cl, o);

        /* Clear for checking inside dispose() whether stuff was allocated.
           Basically this is bug-prevention.
         */
        memset(data, 0, sizeof(struct charmapcondata));

        data->scrollback_max = 500;     /* FIXME: Don't hardcode it */
        data->ccd_GfxBase = newGfxBase;
        charmapcon_add_prop(cl, o);

        ReturnPtr("CharMapCon::New", Object *, o);
    }

    CloseLibrary(newGfxBase);
    ReturnPtr("CharMapCon::New", Object *, NULL);

}

/***********  CharMapCon::Dispose()  **************************/

static VOID charmapcon_dispose(Class *cl, Object *o, Msg msg)
{
    struct charmapcondata *data = INST_DATA(cl, o);

    charmap_dispose_lines(data->top_of_scrollback);
    charmapcon_free_prop(cl, o);

    CloseLibrary(data->ccd_GfxBase);

    DoSuperMethodA(cl, o, msg);
}

/*********  CharMapCon::DoCommand()  ****************************/

static struct charmap_line *charmapcon_find_line(Class *cl, Object *o,
    ULONG ycp)
{
    struct charmapcondata *data = INST_DATA(cl, o);

    // Find the line. This is inefficient but the number of lines on screen
    // should never be very high.
    // FIXME: Optimizing the case of appending to the end (e.g. know what line
    // is on the last line of the buffer).

    struct charmap_line *line = data->top_of_window;
    if (!line)
    {
        D(bug("Initializing charmap\n"));
        data->top_of_window = data->top_of_scrollback = line =
            charmap_newline(0, 0);
        data->scrollback_size = 1;
    }

    D(bug("Finding line %ld\n", ycp));
    while (ycp > 0)
    {
        if (!line->next)
        {
            charmap_newline(0, line);
            data->scrollback_size += 1;
        }
        line = line->next;
        ycp -= 1;
    }

    while (data->scrollback_size > data->scrollback_max + CHAR_YMAX(o) &&
        data->top_of_window != data->top_of_scrollback)
    {
        data->scrollback_size -= 1;
        data->scrollback_pos -= 1;

        /* FIXME: Needs testing... */
        if (data->select_line_max == data->select_line_min &&
            data->select_line_min == data->top_of_scrollback)
        {
            /* The entire selection has scrolled out, but we keep it in case
               it is still active
             */
            data->select_line_min = data->select_line_min->next;
            data->select_line_max = data->select_line_min;
            data->select_x_min = 0;
            data->select_x_max = 0;
            data->select_y_min += 1;
            data->select_y_max += 1;
        }
        else if (data->top_of_scrollback == data->select_line_min)
        {
            data->select_line_min = data->select_line_min->next;
            data->select_y_min += 1;
            data->select_x_min = 0;
        }
        else if (data->top_of_scrollback == data->select_line_max)
        {
            /* "Reversed" selection */
            data->select_line_max = data->select_line_max->next;
            data->select_y_max += 1;
            data->select_x_max = 0;
        }

        data->top_of_scrollback =
            charmap_dispose_line(data->top_of_scrollback);
    }

    return line;
}

static VOID charmap_ascii(Class *cl, Object *o, ULONG xcp, ULONG ycp,
    char *str, ULONG len)
{
    struct charmap_line *line = charmapcon_find_line(cl, o, ycp);
    ULONG oldsize = line->size;

    // Ensure the line has sufficient capacity.
    if (line->size < xcp + len)
        charmap_resize(line, xcp + len);

    // .. copy the required data
    memset(line->fgpen + xcp, CU(o)->cu_FgPen, len);
    memset(line->bgpen + xcp, CU(o)->cu_BgPen, len);
    memset(line->flags + xcp, CU(o)->cu_TxFlags, len);
    memcpy(line->text + xcp, str, len);

    // If cursor output is moved further right on the screen than
    // the last output, we need to fill the line
    if (oldsize < xcp)
    {
        memset(line->fgpen + oldsize, CU(o)->cu_FgPen, xcp - oldsize);
        memset(line->bgpen + oldsize, CU(o)->cu_BgPen, xcp - oldsize);
        memset(line->flags + oldsize, CU(o)->cu_TxFlags, xcp - oldsize);
        memset(line->text + oldsize, ' ', xcp - oldsize);
    }
}

static VOID charmap_scroll_up(Class *cl, Object *o, ULONG y)
{
    struct charmapcondata *data = INST_DATA(cl, o);

    if (!data->top_of_window)
        return;

    while (y--)
    {
        if (!data->top_of_window->next)
        {
            charmap_newline(0, data->top_of_window);
            data->scrollback_size += 1;
        }
        data->top_of_window = data->top_of_window->next;
        data->scrollback_pos += 1;
        data->select_y_max -= 1;
        data->select_y_min -= 1;
    }

    if (data->scrollback_size - CHAR_YMAX(o) - 1 <= data->scrollback_pos &&
        data->unrendered)
    {
        Console_RenderCursor(o);
        data->unrendered = 0;
    }

    while (data->scrollback_size > data->scrollback_max + CHAR_YMAX(o) &&
        data->top_of_window != data->top_of_scrollback)
    {
        data->scrollback_size -= 1;
        data->scrollback_pos -= 1;
        data->top_of_scrollback =
            charmap_dispose_line(data->top_of_scrollback);
    }
}

static VOID charmap_scroll_down(Class *cl, Object *o, ULONG y)
{
    // FIXME: Need to adjust cursor position or reset to bottom when editing.
    struct charmapcondata *data = INST_DATA(cl, o);
    if (!data->unrendered)
    {
        Console_UnRenderCursor(o);
        data->unrendered = 1;
        data->saved_scrollback_pos = data->scrollback_pos;
        data->saved_top_of_window = data->top_of_window;

        // FIXME: Select position.
    }
    if (data->top_of_window)
    {
        while (y-- && data->top_of_window->prev)
        {
            data->top_of_window = data->top_of_window->prev;
            data->scrollback_pos -= 1;
            data->select_y_max += 1;
            data->select_y_min += 1;
        }
    }
}

static VOID charmapcon_scroll_to(Class *cl, Object *o, ULONG y)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct Window *w = CU(o)->cu_Window;
    struct RastPort *rp = w->RPort;
    LONG off = data->scrollback_pos - y;
    LONG old_pos = data->scrollback_pos;

    if (off == 0)
        return;

    Console_UnRenderCursor(o);

    if (off > 0)
        charmap_scroll_down(cl, o, off);
    else
        charmap_scroll_up(cl, o, -off);

    /* Correct offset to account for the fact we might reach the
     * top or bottom of the buffer:
     */
    off = old_pos - data->scrollback_pos;

    /* A whole screenful? If so we have no choice but a full refresh
     * (though we could double buffer... Not sure that's worth the 
     * memory cost)
     */
    if (abs(off) > CHAR_YMAX(o))
    {
        charmapcon_refresh(cl, o, 0);
        Console_RenderCursor(o);
        return;
    }

    /* Avoid a full refresh by scrolling the rastport.
     * Use "standard" background to reduce flicker
     */
    SetBPen(rp, CU(o)->cu_BgPen);
    if (off > 0)
    {
        ScrollRaster(rp,
            0,
            -YRSIZE * off,
            GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));
    }
    else
    {
        ScrollRaster(rp,
            0,
            -YRSIZE * off,
            GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o));

    }

    /* Partial refresh */
    charmapcon_refresh(cl, o, off);

    Console_RenderCursor(o);
}


static VOID charmap_delete_char(Class *cl, Object *o, ULONG x, ULONG y)
{
    struct charmap_line *line = charmapcon_find_line(cl, o, y);

    if (!line || x >= line->size)
        return;

    // FIXME: Shrink the buffer, or keep track of capacity separately.
    if (x + 1 >= line->size)
    {
        line->text[x] = 0;
        return;
    }

    memmove(line->fgpen + x, line->fgpen + x + 1, 1);
    memmove(line->bgpen + x, line->bgpen + x + 1, 1);
    memmove(line->flags + x, line->flags + x + 1, 1);
    memmove(line->text + x, line->text + x + 1, 1);
}

static VOID charmap_insert_char(Class *cl, Object *o, ULONG x, ULONG y)
{
    struct charmap_line *line = charmapcon_find_line(cl, o, y);

    if (x >= line->size)
        return;

    /* FIXME: This is wasteful, since it copies the buffers straight over,
     * so we have to do memmove's further down. */
    charmap_resize(line, line->size + 1);

    memmove(line->fgpen + x + 1, line->fgpen + x, line->size - x - 1);
    memmove(line->bgpen + x + 1, line->bgpen + x, line->size - x - 1);
    memmove(line->flags + x + 1, line->flags + x, line->size - x - 1);
    memmove(line->text + x + 1, line->text + x, line->size - x - 1);

    line->fgpen[x] = CU(o)->cu_FgPen;
    line->bgpen[x] = CU(o)->cu_BgPen;
    line->flags[x] = CU(o)->cu_TxFlags;
    line->text[x] = ' ';
}

static VOID charmap_formfeed(Class *cl, Object *o)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct charmap_line *line = data->top_of_window;

    while (line)
    {
        charmap_resize(line, 0);
        line = line->next;
    }
}

static VOID charmapcon_docommand(Class *cl, Object *o,
    struct P_Console_DoCommand *msg)
{
    IPTR *params = msg->Params;
    struct charmapcondata *data = INST_DATA(cl, o);

    EnterFunc(bug
        ("CharMapCon::DoCommand(o=%p, cmd=%d, params=%p) x=%d, y=%d, ymax=%d\n",
            o, msg->Command, params, XCP, YCP, CHAR_YMAX(o)));

    // This is a bit of a hack: Set position to bottom in order to prevent
    // output while scrolled.

    ULONG old_scrollback_size = data->scrollback_size;
    ULONG old_scrollback_pos = data->scrollback_pos;

    if (data->unrendered)
    {
        data->unrendered = 0;
        data->scrollback_pos = data->saved_scrollback_pos;
        data->select_y_min += old_scrollback_pos - data->scrollback_pos;
        data->select_y_max += old_scrollback_pos - data->scrollback_pos;
        data->top_of_window = data->saved_top_of_window;
        charmapcon_refresh(cl, o, 0);
        Console_RenderCursor(o);
    }

    switch (msg->Command)
    {
    case C_ASCII:
        charmap_ascii(cl, o, XCP, YCP, (char *)&params[0], 1);
        DoSuperMethodA(cl, o, (Msg) msg);
        break;

    case C_ASCII_STRING:
        charmap_ascii(cl, o, XCP, YCP, (char *)params[0], (int)params[1]);
        DoSuperMethodA(cl, o, (Msg) msg);
        break;

    case C_FORMFEED:
        charmap_formfeed(cl, o);
        DoSuperMethodA(cl, o, (Msg) msg);
        break;

    case C_DELETE_CHAR:        /* FIXME: can it have params!? */
        charmap_delete_char(cl, o, XCP, YCP);
        DoSuperMethodA(cl, o, (Msg) msg);
        break;

    case C_INSERT_CHAR:
        charmap_insert_char(cl, o, XCP, YCP);
        DoSuperMethodA(cl, o, (Msg) msg);
        break;

    case C_SCROLL_UP:
        {
            D(bug("C_SCROLL_UP area (%d, %d) to (%d, %d), %d\n",
                    GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o),
                    YRSIZE * params[0]));
            charmap_scroll_up(cl, o, params[0]);
            DoSuperMethodA(cl, o, (Msg) msg);
            break;
        }

    case C_SCROLL_DOWN:
        {
            D(bug("C_SCROLL_DOWN area (%d, %d) to (%d, %d), %d\n",
                    GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o), GFX_YMAX(o),
                    YRSIZE * params[0]));
            charmap_scroll_down(cl, o, params[0]);
            DoSuperMethodA(cl, o, (Msg) msg);
            break;
        }

    default:
        DoSuperMethodA(cl, o, (Msg) msg);
        break;
    }

    if (old_scrollback_size != data->scrollback_size ||
        old_scrollback_pos != data->scrollback_pos)
        charmapcon_adj_prop(cl, o);

    ReturnVoid("CharMapCon::DoCommand");
}

/**************************
**  CharMapCon::ClearCell()  **
**************************/
static VOID charmapcon_clearcell(Class *cl, Object *o,
    struct P_Console_ClearCell *msg)
{
    // FIXME, insert space.
    DoSuperMethodA(cl, o, (Msg) msg);
}

static VOID charmapcon_refresh_lines(Class *cl, Object *o, LONG fromLine,
    LONG toLine)
{
    struct Window *w = CU(o)->cu_Window;
    struct RastPort *rp = w->RPort;
    struct charmapcondata *data = INST_DATA(cl, o);

    if (fromLine < CHAR_YMIN(o))
        fromLine = CHAR_YMIN(o);
    if (toLine > CHAR_YMAX(o))
        toLine = CHAR_YMAX(o);

    D(bug("fromLine: %ld, toLine: %ld, char_ymax: %ld\n", fromLine, toLine,
            CHAR_YMAX(o)));

    if (toLine < fromLine)
        return;

    Console_UnRenderCursor(o);

    D(bug("Rendering charmap\n"));

    struct charmap_line *line = charmapcon_find_line(cl, o, fromLine);
    ULONG y = GFX_YMIN(o) + fromLine * YRSIZE + rp->Font->tf_Baseline;
    ULONG yc = fromLine;
    UBYTE flags = 255;

    LONG selectstart_x, selectstart_y, selectend_x, selectend_y;
    if (data->select_y_min == data->select_y_max)
    {
        selectstart_y = selectend_y = data->select_y_min;
        selectstart_x = MIN(data->select_x_min, data->select_x_max);
        selectend_x = MAX(data->select_x_min, data->select_x_max);
    }
    else if (data->select_y_min < data->select_y_max)
    {
        selectstart_y = data->select_y_min;
        selectstart_x = data->select_x_min;
        selectend_y = data->select_y_max;
        selectend_x = data->select_x_max;
    }
    else
    {
        selectstart_y = data->select_y_max;
        selectstart_x = data->select_x_max;
        selectend_y = data->select_y_min;
        selectend_x = data->select_x_min;
    }

    while (line && yc <= toLine)
    {
        const char *str = line->text;
        ULONG start = 0;
        ULONG remaining_space = CHAR_XMAX(o) + 1;
        Move(rp, GFX_XMIN(o), y);
        while (line->size > start && remaining_space > 0 && str[start])
        {
            /* Identify a batch of characters with the same fgpen/bgpen
               to avoid having to move/set pens and do Text() on single
               characters */

            UBYTE fgpen = line->fgpen[start];
            UBYTE bgpen = line->bgpen[start];

            /* Is any part of this line part of a selection? 
             * If so, we bake in a state transition on "stop".
             * This code is messy - there must be a nicer way.
             */
            ULONG stop = 9999999;
            BOOL in_selection = 0;

            if (yc > selectstart_y && yc < selectend_y)
            {
                in_selection = 1;
            }
            else if (yc == selectstart_y)
            {
                if (yc == selectend_y)
                {
                    if (start >= selectstart_x && start < selectend_x)
                    {
                        in_selection = 1;
                        stop = selectend_x;
                    }
                    else if (start < selectstart_x)
                    {
                        stop = selectstart_x;
                    }
                }
                else
                {
                    if (start >= selectstart_x)
                        in_selection = 1;
                    else
                        stop = selectstart_x;
                }
            }
            else if (yc == selectend_y)
            {
                /* In this case, the selection *ends* on selectend_x */
                if (start < selectend_x)
                {
                    in_selection = 1;
                    stop = selectend_x;
                }
            }

            if (stop == start)
                stop += 1;

            if (in_selection)
            {
                fgpen = line->bgpen[start];
                bgpen = line->fgpen[start];
            }

            ULONG len = 0;
            while (line->size > start + len && str[start + len] &&
                len < remaining_space &&
                line->fgpen[start] == line->fgpen[start + len] &&
                line->bgpen[start] == line->bgpen[start + len] &&
                line->flags[start] == line->flags[start + len] &&
                start + len < stop)
                len += 1;

            setabpen(GfxBase, rp, line->flags[start], fgpen, bgpen);
            if ((line->flags[start] & CON_TXTFLAGS_MASK) !=
                (flags & CON_TXTFLAGS_MASK))
            {
                SetSoftStyle(rp, line->flags[start], CON_TXTFLAGS_MASK);
            }
            flags = line->flags[start];

            Text(rp, &str[start], len);

            start += len;
            remaining_space -= len;
        }

        /* Clear to EOL, without overwriting scroll bar (ClearEOL does) */
        SetAPen(rp, CU(o)->cu_BgPen);
        RectFill(rp,
            GFX_X(o, start), GFX_Y(o, yc),
            GFX_XMAX(o), GFX_Y(o, yc + 1) - 1);
        y += YRSIZE;
        yc++;

        /* We want to make sure we have lines covering the window once
           there's something to scroll back, as that simplies resize handling
           etc. 
         */
        if (!line->next && yc <= toLine)
        {
            line->next = charmap_newline(0, line);
            data->scrollback_size += 1;
        }
        line = line->next;
    }

    if (yc < toLine)
    {
        SetAPen(rp, CU(o)->cu_BgPen);
        RectFill(rp,
            GFX_XMIN(o), GFX_Y(o, yc), GFX_XMAX(o), GFX_Y(o, toLine));
    }

    Console_RenderCursor(o);
}


/*
 * Refresh the full console unless "off" is provided.
 * If off is set to a positive value, it indicates the
 * number of rows from the top we start rendering.
 * If off is set to a negative value, it indicates the
 * number of rows from the top we stop rendering.
 * This is used for partial refreshes when the screen is
 * scrolled up/down.
 */
static VOID charmapcon_refresh(Class *cl, Object *o, LONG off)
{
    LONG fromLine = 0;
    LONG toLine = CHAR_YMAX(o) - CHAR_YMIN(o);

    if (off > 0)
        toLine = off - 1;
    if (off < 0)
        fromLine = CHAR_YMAX(o) + off + 1;

    charmapcon_refresh_lines(cl, o, fromLine, toLine);
}


static ULONG charmapcon_calc_selection_size(struct charmap_line *first,
    struct charmap_line *last, ULONG minx, ULONG maxx)
{
    struct charmap_line *cur = first;
    ULONG size = 0;
    while (cur)
    {
        if (cur->text)
        {
            if (cur == first)
            {
                if (cur == last)
                    size += abs(maxx - minx);
                else
                    size += cur->size - minx;
            }
            else if (cur == last)
            {
                size += maxx;
            }
            else
            {
                size += cur->size;
            }
        }
        if (cur != last || (cur == last && maxx == cur->size))
            size += 1;          /* line feed */

        if (cur == last)
            break;
        cur = cur->next;
    }
    return size;
}

char *charmapcon_get_selection(ULONG size,
    struct charmap_line *first,
    struct charmap_line *last, ULONG minx, ULONG maxx)
{
    char *buf = AllocMem(size, MEMF_ANY);
    char *bufpos = buf;
    struct charmap_line *cur = first;

    if (!bufpos)
        return 0;

    while (cur)
    {
        if (cur->text)
        {
            /* empty lines may not have text ptrs */
            if (cur == first)
            {
                if (cur == last)
                {
                    CopyMem(cur->text + MIN(minx, maxx), bufpos,
                        abs(maxx - minx));
                    bufpos += abs(maxx - minx);
                }
                else
                {
                    CopyMem(cur->text + minx, bufpos, cur->size - minx);
                    bufpos += cur->size - minx;
                }
            }
            else if (cur == last)
            {
                CopyMem(cur->text, bufpos, maxx);
                bufpos += maxx;
            }
            else
            {
                CopyMem(cur->text, bufpos, cur->size);
                bufpos += cur->size;
            }
        }
        if (cur != last || (cur == last && maxx == cur->size))  /* line feed */
        {
            *bufpos = '\r';
            bufpos += 1;
        }
        if (cur == last)
            break;
        cur = cur->next;
    }
    return buf;
}

struct ConClipData
{
    ULONG flags;                /* always zero? */
    ULONG size;                 /* does not include NUL termination */
    APTR buffer;                /* NUL-terminated string! */
};

/* FIXME: Belongs in snipmapcon - here temporarily until refactored out
   selection code */
static VOID charmapcon_copy(Class *cl, Object *o, Msg copymsg)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct MsgPort replyport, *port;
    struct SGWork sgw;
    struct MyEditHookMsg msg;
    char *buf;

    struct charmap_line *first, *last;
    ULONG minx, maxx, size;

    /* Create a string from the contents of the scrollback buffer */
    if (data->select_y_min < data->select_y_max)
    {
        first = data->select_line_min;
        last = data->select_line_max;
        minx = data->select_x_min;
        maxx = data->select_x_max;
    }
    else
    {
        first = data->select_line_max;
        last = data->select_line_min;
        minx = data->select_x_max;
        maxx = data->select_x_min;
    }

    size = charmapcon_calc_selection_size(first, last, minx, maxx);
    buf = charmapcon_get_selection(size, first, last, minx, maxx);
    D(bug("%d bytes copied\n"));

    /* If Conclip is running, we prefer using that */
    if (IsListEmpty(&ConsoleDevice->sniphooks)
        && (port = FindPort(CONCLIP_PORTNAME)))
    {
        /* AROS conclip format */
        D(bug("AROS conclip\n"));
        replyport.mp_Node.ln_Type = NT_MSGPORT;
        replyport.mp_Node.ln_Name = NULL;
        replyport.mp_Node.ln_Pri = 0;
        replyport.mp_Flags = PA_SIGNAL;
        replyport.mp_SigBit = SIGB_SINGLE;
        replyport.mp_SigTask = FindTask(NULL);
        NewList(&replyport.mp_MsgList);

        msg.msg.mn_Node.ln_Type = NT_MESSAGE;
        msg.msg.mn_ReplyPort = &replyport;
        msg.msg.mn_Length = sizeof(msg);

        msg.code = CODE_COPY;
        msg.sgw = &sgw;

        sgw.Gadget = 0;
        sgw.WorkBuffer = buf;
        sgw.PrevBuffer = 0;
        sgw.IEvent = 0;
        sgw.Code = CODE_COPY;
        sgw.Actions = 0;
        sgw.LongInt = 0;
        sgw.GadgetInfo = 0;
        sgw.EditOp = EO_BIGCHANGE;
        sgw.BufferPos = 0;
        sgw.NumChars = size;

        SetSignal(0, SIGF_SINGLE);
        PutMsg(port, &msg.msg);
        WaitPort(&replyport);

        FreeMem(buf, size);
        return;
    }

    ObtainSemaphore(&ConsoleDevice->copyBufferLock);
    FreeMem((APTR) ConsoleDevice->copyBuffer,
        ConsoleDevice->copyBufferSize);

    ConsoleDevice->copyBuffer = buf;
    if (ConsoleDevice->copyBuffer)
        ConsoleDevice->copyBufferSize = size;
    else
        ConsoleDevice->copyBufferSize = 0;

    if (!IsListEmpty(&ConsoleDevice->sniphooks))
    {
        /* OS2-3.x compatible conclip format */
        struct Hook *conhook;
        struct ConClipData ccd;

        D(bug("AOS conclip\n"));
        if (ConsoleDevice->copyBufferSize)
        {
            ccd.flags = 0;
            ccd.size = ConsoleDevice->copyBufferSize;
            /* must be NUL-terminated */
            ccd.buffer = AllocVec(ccd.size + 1, MEMF_CLEAR);
            if (ccd.buffer)
            {
                CopyMem(ConsoleDevice->copyBuffer, ccd.buffer, ccd.size);
                ForeachNode(&ConsoleDevice->sniphooks, conhook)
                {
                    D(bug("Calling AOS conclip hook %p\n", conhook));
                    CALLHOOKPKT(conhook, NULL, &ccd);
                }
                FreeVec(ccd.buffer);
            }
        }
    }
    ReleaseSemaphore(&ConsoleDevice->copyBufferLock);
}


/**********************************
**  CharMapCon::NewWindowSize()  **
**********************************/
static VOID charmapcon_newwindowsize(Class *cl, Object *o,
    struct P_Console_NewWindowSize *msg)
{
    struct charmapcondata *data = INST_DATA(cl, o);

    WORD old_ycp = YCP;

    DoSuperMethodA(cl, o, (Msg) msg);
    D(bug("CharMapCon::NewWindowSize(o=%p) x=%d, y=%d, ymax=%d\n",
            o, XCP, YCP, CHAR_YMAX(o)));

    // Is console empty? Unlikely, but anyway.
    if (!data->top_of_window)
        return;

    // Scroll up if new window size has forced the cursor up
    if (old_ycp > CHAR_YMAX(o))
    {
        charmap_scroll_up(cl, o, old_ycp - CHAR_YMAX(o));
    }

    charmapcon_refresh(cl, o, 0);
}

static VOID charmapcon_handlemouse(Class *cl, Object *o,
    struct P_Console_HandleGadgets *msg)
{
    struct charmapcondata *data = INST_DATA(cl, o);
    struct Window *w = CU(o)->cu_Window;
    struct InputEvent *e = msg->Event;

    /* We have the following states:
     * - No active selection and no mouse button => ignore
     * - No active selection and left mouse button => Start selection
     * - Active selection and no mouse button => End selection
     * - Active selection and left mouse button => Update selection
     */

    LONG x, y;

    /*
     * Take window-relative mouse position.
     * The original code here failed when the screen was moved away from (0, 0)
     *  position. x and y were unaware of shifted display (ie_X and ie_Y are
     * raw physical coordinates, and w->LeftEdge and w->TopEdge are screen-
     * relative), and this caused lockups here.
     * TODO: Verify this intuition's behavior with AmigaOS3.1 using screentest
     * program. (sonic)
     *
     x = e->ie_X - w->LeftEdge;
     y = e->ie_Y - w->TopEdge;
     */
    x = w->MouseX;
    y = w->MouseY;

    /* Active selection */

    if (!(e->ie_Qualifier & IEQUALIFIER_LEFTBUTTON))
    {
        /* End selection */
        data->active_selection = 0;
        data->ignore_drag = 0;
        return;
    }

    if (!data->active_selection)
    {
        if (e->ie_Qualifier & IEQUALIFIER_LEFTBUTTON && !data->ignore_drag)
        {
            /* Inside the console area? */
            if (x >= GFX_XMIN(o) && x <= GFX_XMAX(o) &&
                y >= GFX_YMIN(o) && y <= GFX_YMAX(o))
            {
                D(bug("activated selection with x: %ld, y: %ld,"
                    " xmin: %ld, ymin: %Ld, xmax: %ld, ymax: %ld\n",
                    x, y, GFX_XMIN(o), GFX_YMIN(o), GFX_XMAX(o),
                    GFX_YMAX(o)));

                /* We need to clear these lines 
                 */
                LONG old_min_y =
                    MIN(data->select_y_min, data->select_y_max);
                LONG old_max_y =
                    MAX(data->select_y_min, data->select_y_max);

                /* Yes, so start selection */
                data->active_selection = 1;
                data->ignore_drag = 0;
                data->select_x_min = (x - GFX_XMIN(o)) / XRSIZE;
                data->select_y_min = (y - GFX_YMIN(o)) / YRSIZE;
                data->select_line_min =
                    charmapcon_find_line(cl, o, data->select_y_min);

                data->select_x_max = data->select_x_min;
                data->select_y_max = data->select_y_min;

                /* Clear */

                /* FIXME: Determine exactly which lines are affected, as
                   follows:

                   - If max_y has increased, render from old_max_y to new
                   - If min_y has decreased, refresh from new min y to old
                   - If both have changed, refresh in two batches.
                   - If there's been a reversal of the relative positions of
                   y_min/y_max, refresh the entire range.
                 */
                charmapcon_refresh_lines(cl, o, old_min_y, old_max_y);

            }
            else
                data->ignore_drag = 1;
        }
        return;
    }

    /* Update selection. */
    if (x >= GFX_XMIN(o) && x <= GFX_XMAX(o) &&
        y >= GFX_YMIN(o) && y <= GFX_YMAX(o))
    {

        LONG xmax, ymax;
        xmax = data->select_x_max;
        ymax = data->select_y_max;
        data->select_x_max = (x - GFX_XMIN(o)) / XRSIZE;
        data->select_y_max = (y - GFX_YMIN(o)) / YRSIZE;
        data->select_line_max =
            charmapcon_find_line(cl, o, data->select_y_max);
        /* FIXME: More intelligent refresh */
        if (xmax != data->select_x_max || ymax != data->select_y_max)
        {
            charmapcon_refresh_lines(cl, o, MIN(ymax,
                    MIN(data->select_y_min, data->select_y_max)), MAX(ymax,
                    MAX(data->select_y_min, data->select_y_max)));
        }
    }
    else
    {
        /* FIXME: Outside the console area, we need to scroll the window
           and just update the selection based on that */
    }

}

static VOID charmapcon_handlegadgets(Class *cl, Object *o,
    struct P_Console_HandleGadgets *msg)
{
    struct InputEvent *e = msg->Event;

    if (e->ie_Class == IECLASS_RAWMOUSE)
    {
        charmapcon_handlemouse(cl, o, msg);
        return;
    }

    struct charmapcondata *data = INST_DATA(cl, o);

    if (e->ie_Class == IECLASS_GADGETUP)
    {
        data->activeGad = 0;
        return;
    }

    if (e->ie_Class == IECLASS_GADGETDOWN)
    {
        /* We pass 0 from consoletask if the mouse wheel is being used */
        if ((IPTR) e->ie_EventAddress == 1)
            data->activeGad = (APTR) &(data->prop->up);
        else if ((IPTR) e->ie_EventAddress == 2)
            data->activeGad = (APTR) &(data->prop->down);
        else
            data->activeGad = e->ie_EventAddress;
    }

    if (data->activeGad == (APTR) &(data->prop->scroller))
    {
        ULONG hidden =
            data->scrollback_size >
            CHAR_YMAX(o) ? data->scrollback_size - CHAR_YMAX(o) - 1 : 0;
        ULONG pos =
            (((struct PropInfo *)((struct Gadget *)&(data->prop->
                        scroller))->SpecialInfo)->VertPot * hidden +
            (MAXPOT / 2)) / MAXPOT;

        if (pos != data->scrollback_pos)
            charmapcon_scroll_to(cl, o, pos);
    }
    else if (data->activeGad == (APTR) &(data->prop->down))
    {
        if (data->scrollback_pos + CHAR_YMAX(o) < data->scrollback_size - 1)
        {
            charmap_scroll_up(cl, o, 1);
            charmapcon_refresh(cl, o, 0);
            charmapcon_adj_prop(cl, o);
        }
    }
    else if (data->activeGad == (APTR) &(data->prop->up))
    {
        if (data->top_of_window != data->top_of_scrollback)
        {
            charmap_scroll_down(cl, o, 1);
            charmapcon_refresh(cl, o, 0);
            charmapcon_adj_prop(cl, o);
        }
    }
}



AROS_UFH3S(IPTR, dispatch_charmapconclass,
    AROS_UFHA(Class *, cl, A0),
    AROS_UFHA(Object *, o, A2), AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR retval = 0UL;

    switch (msg->MethodID)
    {
    case OM_NEW:
        retval = (IPTR) charmapcon_new(cl, o, (struct opSet *)msg);
        break;

    case OM_DISPOSE:
        charmapcon_dispose(cl, o, msg);
        break;

    case M_Console_DoCommand:
        charmapcon_docommand(cl, o, (struct P_Console_DoCommand *)msg);
        break;

    case M_Console_ClearCell:
        // FIXME: scroll down to end here if it's not there already.
        charmapcon_clearcell(cl, o, (struct P_Console_ClearCell *)msg);
        break;

    case M_Console_NewWindowSize:
        charmapcon_newwindowsize(cl, o,
            (struct P_Console_NewWindowSize *)msg);
        break;

    case M_Console_HandleGadgets:
        //D(bug("CharMapCon::HandleGadgets\n"));
        charmapcon_handlegadgets(cl, o,
            (struct P_Console_HandleGadgets *)msg);
        break;

        /* FIXME: Belongs in snimapcon - here temporarily until refactored out
           selection code */
    case M_Console_Copy:
        charmapcon_copy(cl, o, msg);
        break;

    default:
        retval = DoSuperMethodA(cl, o, msg);
        break;
    }

    return retval;

    AROS_USERFUNC_EXIT
}

#undef ConsoleDevice

Class *makeCharMapConClass(struct ConsoleBase *ConsoleDevice)
{

    Class *cl;

    cl = MakeClass(NULL, NULL, STDCONCLASSPTR,
        sizeof(struct charmapcondata), 0UL);
    if (cl)
    {
        cl->cl_Dispatcher.h_Entry = (APTR) dispatch_charmapconclass;
        cl->cl_Dispatcher.h_SubEntry = NULL;

        cl->cl_UserData = (IPTR) ConsoleDevice;

        return (cl);
    }
    return (NULL);
}
