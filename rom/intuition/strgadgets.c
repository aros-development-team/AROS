/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/keymap.h>
#include <intuition/screens.h>
#include <intuition/cghooks.h>
#include <graphics/gfxmacros.h>
#include <devices/inputevent.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* atol() */
#include <stdio.h>  /* snprintf() */
#include <string.h>

#include "intuition_intern.h"
#include "strgadgets.h"

#ifdef __MORPHOS__
size_t  stccpy(char *s1, const char *s2, size_t n);
#endif

#undef DEBUG
#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************/

#define CURSORPEN   0
#define STRBACKPEN  1
#define STRTEXTPEN  2

#define NUMPENS 3
#define STRALIGNMASK    (GACT_STRINGLEFT|GACT_STRINGCENTER|GACT_STRINGRIGHT)

#define KEYBUFSIZE  10
#define SHIFT       (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)

#define HELPRAW     95  /* Raw     */
#define CTRL_A      1
#define CTRL_B      2
#define CTRL_C      3
#define CTRL_D      4
#define CTRL_E      5
#define CTRL_F      6
#define CTRL_G      7
#define CTRL_H      8
#define CTRL_I      9
#define CTRL_J      10
#define CTRL_K      11
#define CTRL_L      12
#define CTRL_M      13
#define CTRL_N      14
#define CTRL_O      15
#define CTRL_P      16
#define CTRL_Q      17
#define CTRL_R      18
#define CTRL_S      19
#define CTRL_T      20
#define CTRL_U      21
#define CTRL_V      22
#define CTRL_W      23
#define CTRL_X      24
#define CTRL_Y      25
#define CTRL_Z      26

#define BACKSPACE   8   /* Vanilla */
#define TAB     9   /* Vanilla */
#define TABRAW      66  /* Raw     */
#define RETURN      13  /* Vanilla */
#define DELETE      127 /* Vanilla */
#define HOMERAW     112 /* Raw     */
#define ENDRAW      113 /* Raw     */

/*****************************************************************************************/

VOID UpdateStringInfo(struct Gadget *);

/*****************************************************************************************/

#define CharXSize(char, rp) ((rp->Font->tf_Flags & FPF_PROPORTIONAL) ?    \
    	    	    	    rp->Font->tf_XSize : TextLength(rp, &(char), 1))


#undef MIN
#define MIN(a, b) ((a < b) ? a : b)

/*****************************************************************************************/

char    cursorkey[] = "_";
#define CURSORKEYLEN    1

/*****************************************************************************************/

STATIC WORD MaxDispPos(struct StringInfo *strinfo, struct BBox *bbox,
                       struct RastPort *rp, struct IntuitionBase *IntuitionBase)
{

    WORD            	numfit, max_disppos, numchars;
    struct TextExtent   te;
    BOOL            	cursor_at_end;

    cursor_at_end = (strinfo->BufferPos == strinfo->NumChars);

    EnterFunc(bug("MaxDispPos(current length: %d, bufferpos=%d)\n", strinfo->NumChars, strinfo->BufferPos));

    D(bug("cursor_at_end: %d\n", cursor_at_end));

    if (cursor_at_end) /* Cursor at end of string ? */
    {
        D(bug("Making cursor last char\n"));
        numchars = strinfo->NumChars + 1; /* Take cursor into account */

        /*  This has allready been done by UpdateDisp() which called us
            strinfo->Buffer[strinfo->NumChars] = 0x20;
         
        */
    }
    else
    {

        numchars = strinfo->NumChars;
    }


    /* Find the amount of characters that fit into the bbox, counting
    ** from the last character in the buffer and forward,
    */
    numfit = TextFit(rp,
                     &(strinfo->Buffer[numchars - 1]),
                     numchars, &te, NULL,
                     -1, bbox->Width, rp->Font->tf_YSize);


    max_disppos = numchars - numfit;

    /*    if ((max_disppos > 0) && (!cursor_at_end))
        max_disppos --;
      */

    D(bug("Numchars w/cursor: %d, Numfit: %d, maxdisppos=%d  bbox->Width = %d  te->te_Width = %d\n",
          numchars, numfit, max_disppos, bbox->Width, te.te_Width));

    ReturnInt("MaxDispPos", WORD, max_disppos);
}

/*****************************************************************************************/

void UpdateDisp(struct Gadget       *gad,
                struct BBox     *bbox,
                struct RastPort     *rp,
                struct IntuitionBase    *IntuitionBase)
{


    struct TextExtent    te;
    struct StringInfo   *strinfo = (struct StringInfo *)gad->SpecialInfo;
    STRPTR          	 dispstr;

    EnterFunc(bug("UpdateDisp(gad=%p, bbox=%p, rp=%p)\n",
                  gad, bbox, rp));

    /* If the cursor is at the trailing \0, insert a SPACE instead */
    if (strinfo->BufferPos == strinfo->NumChars)
        strinfo->Buffer[strinfo->NumChars] = 0x20;

    /* In this function we check if the cursor has gone outside
    ** of the visible area (because of application setting
    ** strinfo->BufferPos or strinfo->DispPos to a different value, or
    ** because of user input).
    ** This is made a bit difficult by the rule (R), that there
    ** should NOT be available space on the right, and characters
    ** scrolled out at the left, at the same time.
    ** We have 3 possible scenarios:
    ** 1) Cursor to the left of DispPos:
    **    Set DispPos to the lowest of BufferPos and the
    **    maximum allowed disppos (according to (R) ).
    ** 2) Cursor to the right of visible area:
    **    Set dispose sou that the cursor is the last visible character.
    **    This afheres to (R).
    ** 3) Cursor inside visible area. Do a check on rule (R),
    **    and if DispPos > max allowed, then adjust it down,
    **    so that the last character in the buffer becomes last character
    **    displayed. (The cursor will still be visible after adjustion)
    */


    /* 1) Cursor to the left of visible area */
    if (strinfo->BufferPos < strinfo->DispPos)
    {
        WORD max_disppos;

        max_disppos = MaxDispPos(strinfo, bbox, rp, IntuitionBase);
        strinfo->DispPos = MIN(strinfo->BufferPos, max_disppos);
    }
    else /* Cursor equal to the right of disppos [ 2) or 3) ] */
    {
        UWORD strsize;

        /* How many pixels are there from current 1st displayed to the cursor ? */
        strsize = TextLength(rp,
                             strinfo->Buffer + strinfo->DispPos,
                             strinfo->BufferPos - strinfo->DispPos + 1);

        /* 2) More than fits into the gadget ? */
        if (strsize > bbox->Width)
        {
            /* Compute new DispPos such that the cursor is at the right */
            strinfo->DispPos = strinfo->BufferPos
                               - TextFit(rp,
                                         &(strinfo->Buffer[strinfo->BufferPos]),
                                         strinfo->NumChars, &te, NULL, -1,
                                         bbox->Width, rp->Font->tf_YSize)
                               + 1;

            D(bug("cursor right of visible area, new disppos: %d\n", strinfo->DispPos));
        }
        else /* 3). Cursor inside gadget */
        {
            WORD max_disppos;

            max_disppos = MaxDispPos(strinfo, bbox, rp, IntuitionBase);
            if (strinfo->DispPos > max_disppos)
                strinfo->DispPos = max_disppos;

        } /* if (cursor inside or to the right of visible area )*/

    }

    /* Update the DispCount */
    /* It might be necessary with special handling for centre aligned gads */
    dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    strinfo->DispCount = TextFit(rp, dispstr,
                                 strinfo->NumChars - strinfo->DispPos,
                                 &te, NULL, 1,
                                 bbox->Width,
                                 rp->Font->tf_YSize);

    /* 0-terminate string */
    strinfo->Buffer[strinfo->NumChars] = 0x00;
    ReturnVoid("UpdateDisp");
}

/*****************************************************************************************/

STATIC UWORD GetTextLeft(struct Gadget      *gad,
                         struct BBox        *bbox,
                         struct RastPort    *rp,
                         struct IntuitionBase   *IntuitionBase)
{
    /* Gets left position of text in the string gadget */

    struct StringInfo   *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD           	 text_left = 0;
    STRPTR          	 dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    UWORD           	 dispstrlen;
    BOOL            	 cursor_at_end;

    cursor_at_end = (strinfo->BufferPos == strinfo->NumChars);

    dispstrlen = strinfo->NumChars - strinfo->DispPos;

    /* Calcluate start offset of gadget text */
    switch (gad->Activation & STRALIGNMASK)
    {
	case GACT_STRINGLEFT:
    	#warning FIXME: is this default: correct?
	default:
            text_left = bbox->Left;
            break;

	case GACT_STRINGCENTER:
            {
        	WORD textwidth = TextLength(rp, dispstr, dispstrlen);

        	if (cursor_at_end) textwidth += TextLength(rp, cursorkey, CURSORKEYLEN);
        	text_left = bbox->Left + ((bbox->Width - textwidth) / 2);
            }
            break;

	case GACT_STRINGRIGHT:
            {
        	WORD textwidth = TextLength(rp, dispstr, dispstrlen);

        	if (cursor_at_end) textwidth += TextLength(rp, cursorkey, CURSORKEYLEN);
        	text_left =  bbox->Left + (bbox->Width - 1 - textwidth);
            }
            break;
    }
    return (text_left);
}

/*****************************************************************************************/

STATIC UWORD GetTextRight(struct Gadget     *gad,
                          struct BBox       *bbox,
                          struct RastPort   *rp,
                          struct IntuitionBase  *IntuitionBase)
{
    /* Gets right offset of text in the string gadget */

    struct StringInfo   *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD           	 text_right = 0;
    STRPTR          	 dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    UWORD           	 dispstrlen;
    BOOL            	 cursor_at_end;

    cursor_at_end = (strinfo->BufferPos == strinfo->NumChars);

    dispstrlen = strinfo->NumChars - strinfo->DispPos;

    /* Calcluate start offset of gadget text */
    switch (gad->Activation & STRALIGNMASK)
    {
    case GACT_STRINGLEFT:
    	#warning FIXME: is this default: correct?
	default:
            text_right =  bbox->Left + TextLength(rp, dispstr, dispstrlen);
            break;

	case GACT_STRINGCENTER:
            {
        	WORD textwidth = TextLength(rp, dispstr, dispstrlen);

        	if (cursor_at_end) textwidth += TextLength(rp, cursorkey, CURSORKEYLEN);
        	text_right = bbox->Left + bbox->Width - 1 - ((bbox->Width - textwidth) / 2);
            }
            break;

	case GACT_STRINGRIGHT:
            text_right = bbox->Left + bbox->Width  - 1;
            break;
    }
    return (text_right);
}

/*****************************************************************************************/

STATIC VOID GetPensAndFont(struct Gadget *gad,
                           UWORD     *pens,
                           struct Window *win,
                           struct RastPort *rp,
                           struct IntuitionBase *IntuitionBase)
{

    struct DrawInfo *dri = GetScreenDrawInfo(win->WScreen);
    BOOL    	     docursor = FALSE;

    SetFont(rp, dri->dri_Font);

    if (gad->Flags & GFLG_STRINGEXTEND)
    {
        struct StringExtend *strext;

        strext = ((struct StringInfo *)gad->SpecialInfo)->Extension;

        if (strext->Font)
        {
            SetFont(rp, strext->Font);
        }

        if ((gad->Flags & GFLG_SELECTED) == GFLG_SELECTED)
        {
            pens[STRTEXTPEN]    = strext->ActivePens[0];
            pens[STRBACKPEN]    = strext->ActivePens[1];
        }
        else
        {
            pens[STRTEXTPEN]    = strext->Pens[0];
            pens[STRBACKPEN]    = strext->Pens[1];
        }

    }
    else
    {
        /* We don't care to lock the screen because the window we're
        ** drawn on does this. If the window is moved to another
        ** public screen we will be called again to rerender (and get new
        ** valid pens). If the window is closed, and the screen is closed
        ** we won't be here anymore.
        */

        /* jDc: set the font to Topaz,8! */
        if (GetPrivIBase(IntuitionBase)->TopazFont)
        {
            SetFont(rp,GetPrivIBase(IntuitionBase)->TopazFont);
        } else {
            SetFont(rp,GfxBase->DefaultFont);
        }

#if 0
        if ((gad->Flags & GFLG_SELECTED) == GFLG_SELECTED)
        {
            pens[STRTEXTPEN]    = dri->dri_Pens[TEXTPEN];
            pens[STRBACKPEN]    = dri->dri_Pens[BACKGROUNDPEN];

        }
        else
        {
#endif
            pens[STRTEXTPEN]    = dri->dri_Pens[TEXTPEN];
            pens[STRBACKPEN]    = dri->dri_Pens[BACKGROUNDPEN];
#if 0
        }
#endif

    }
    pens[CURSORPEN] = dri->dri_Pens[FILLPEN];

    /* do some protection against stupid apps so we don't get into white text on white background pb */
    if (pens[STRTEXTPEN] == pens[STRBACKPEN])
    {
        if (pens[STRTEXTPEN] != 0)
        {
            pens[STRBACKPEN] = 0;
            if (pens[STRTEXTPEN] != 3) docursor = TRUE;
        }
	else
	{
            pens[STRTEXTPEN] = 1;
            pens[STRBACKPEN] = 0;
            docursor = TRUE;
        }
    }

    if ((pens[CURSORPEN] == pens[STRTEXTPEN]) || (pens[CURSORPEN] == pens[STRBACKPEN]))
    {
        pens[CURSORPEN] = (docursor ? 3 : ((pens[STRTEXTPEN] == 2) ? 2 : 1));
    }

    FreeScreenDrawInfo(win->WScreen, dri);

    return;
}

/*****************************************************************************************/

ULONG HandleStrInput(   struct Gadget       *gad,
                      struct GadgetInfo *ginfo,
                      struct InputEvent *ievent,
                      UWORD         *imsgcode,
                      struct IntuitionBase  *IntuitionBase)
{
    struct SGWork   	 sgw;
    struct StringInfo   *strinfo = (struct StringInfo *)gad->SpecialInfo;
    struct StringExtend *strext = NULL;
    ULONG           	 command = 0;

    EnterFunc(bug("HandleStrInput(gad=%p, ginfo=%p, ievent=%p)\n",
                  gad, ginfo, ievent));

    if ((ievent->ie_Class == IECLASS_TIMER)) return 0;

    D(bug("Gadget text: %s\n", strinfo->Buffer));

    if (!ginfo)
        ReturnInt("HandleStrInput", ULONG, 0UL);

    UpdateStringInfo(gad);

    /* Initialize SGWork */
    sgw.Gadget  = gad;
    sgw.StringInfo  = strinfo;
    sgw.WorkBuffer  = strinfo->Buffer; /* default */
    sgw.PrevBuffer  = strinfo->Buffer;
    sgw.Modes   = 0;
    sgw.IEvent  = ievent;
    sgw.Code    = 0;
    sgw.BufferPos   = strinfo->BufferPos;
    sgw.NumChars    = strinfo->NumChars;
    sgw.Actions = 0;
    sgw.LongInt = strinfo->LongInt;
    sgw.GadgetInfo  = ginfo;
    sgw.EditOp  = EO_NOOP;

    if (gad->Flags & GFLG_STRINGEXTEND)
    {
        D(bug("HandleStrInput: Extended gadget\n"));
        strext = strinfo->Extension;
        if (strext->WorkBuffer)
        {
            sgw.WorkBuffer = strext->WorkBuffer;
            /* The edit hook gets *copy* of the current buffer contents */
            strcpy(sgw.WorkBuffer, strinfo->Buffer);
        }
        sgw.Modes = strext->InitialModes;
    }

    switch (ievent->ie_Class)
    {
	case IECLASS_RAWMOUSE:
            if (ievent->ie_Code == SELECTDOWN)
            {
        	command = SGH_CLICK;
        	sgw.Actions = SGA_USE | SGA_REDISPLAY;
        	D(bug("HandleStrInput: RAWMOUSE event\n"));
            }
            break;

	case IECLASS_RAWKEY:
        {
            UBYTE buf;

            D(bug("HandleStrInput: RAWKEY event\n"));
            if (ievent->ie_Code & IECODE_UP_PREFIX)
            {
                D(bug("HandleStrInput: filter UP event\n"));
            }
            else
            {
                command = SGH_KEY;
                sgw.Actions = SGA_USE;
                if (1 == MapRawKey(sgw.IEvent, &buf, 1, strinfo->AltKeyMap))
                {
                    D(bug("HandleStrInput: sgw.Code 0x%lx\n",buf));
                    sgw.Code = (UWORD)buf;
                }
            }
	    break;
        }

    }

    if (!command)
        ReturnInt("HandleStrInput", ULONG , 0UL);

    /* Call the global editing hook */

    D(bug("calling global hook, Buffer=%s, WorkBuffer=%s\n",
          strinfo->Buffer, sgw.WorkBuffer));
    CallHookPkt(GetPrivIBase(IntuitionBase)->GlobalEditHook, &sgw, &command);

    /* If there is a local edit hook, run it */
    if (strext)
    {
        if (strext->EditHook)
        {
            D(bug("calling local edit hook\n"));
            CallHookPkt(strext->EditHook, &sgw, &command);
        }
    }

    /* Copy possibly changed stuff into stringgad */
    if (sgw.Actions & SGA_USE)
    {
        if (strext)
        {
            if (strext->WorkBuffer)
                strcpy(strinfo->Buffer, strext->WorkBuffer);
        }

        strinfo->BufferPos = sgw.BufferPos;
        strinfo->NumChars  = sgw.NumChars;
        strinfo->LongInt   = sgw.LongInt;

#if 0
        if (gad->Activation & GACT_LONGINT)
        {
            kprintf("strinfo->LongInt = %d\n",strinfo->LongInt);
            */
        }
        else
        {
            kprintf("strinfo->Buffer = \"%s\"\n",strinfo->Buffer);
            */
        }
#endif
    }

    if (sgw.Actions & SGA_BEEP)
    {
        D(bug("SGA_BEEP not yet implemented. (lack of DisplayBeep())\n"));
    }

    if (sgw.Actions & (SGA_END | SGA_NEXTACTIVE | SGA_PREVACTIVE))
    {
        gad->Flags &= ~GFLG_SELECTED;
        *imsgcode = sgw.Code;
        D(bug("HandleStrInput: SGA_END\n"));
    }

    if (sgw.Actions & SGA_REDISPLAY)
    {
        D(bug("HandleStrInput: SGA_REDISPLAY\n"));
        /* Hack for making strgclass work */
        if ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
        {
            struct RastPort *rp;
            D(bug("HandleStrInput: Rerendering boopsi gadget\n"));

            if ((rp = ObtainGIRPort(ginfo)) != NULL)
            {
                struct gpRender method;

                method.MethodID   = GM_RENDER;
                method.gpr_GInfo  = ginfo;
                method.gpr_RPort  = rp;
                method.gpr_Redraw = GREDRAW_UPDATE;
		
                Custom_DoMethodA(gad, (Msg)&method);

                ReleaseGIRPort(rp);
            }
        } /* if gadget is a strgclass object */
        else
        {
            D(bug("HandleStrInput: Rerendering intuition gadget\n"));
            RefreshStrGadget(gad, ginfo->gi_Window, ginfo->gi_Requester, IntuitionBase);
        }
    }

    ReturnInt("HandleStrInput", ULONG, sgw.Actions);
}

/*****************************************************************************************/

STATIC ULONG DoSGHClick(struct SGWork *sgw, struct IntuitionBase *IntuitionBase)
{

    struct Gadget   	*gad;
    struct StringInfo   *strinfo;
    struct BBox     	 bbox;

    struct TextFont 	*oldfont;
    struct RastPort 	*rp;
    struct Window   	*window;
    struct Requester    *req;

    UWORD           	 text_left, text_right;
    WORD            	 mousex;

    window = sgw->GadgetInfo->gi_Window;
    req = sgw->GadgetInfo->gi_Requester;

    GetGadgetDomain(sgw->Gadget, window->WScreen, window, NULL, (struct IBox *)&bbox);
    mousex = window->MouseX - bbox.Left;

    EnterFunc(bug("DoSGHClick(sgw=%p)\n", sgw));

    D(bug("Gadget text: %s\n", sgw->WorkBuffer));

    rp      = sgw->GadgetInfo->gi_RastPort;
    oldfont = rp->Font;
    gad     = sgw->Gadget;
    strinfo = (struct StringInfo *)gad->SpecialInfo;

    CalcBBox(window, req, gad, &bbox);

    {
        struct DrawInfo *dri;

        dri = GetScreenDrawInfo(window->WScreen);
        SetFont(rp, dri->dri_Font);
        FreeScreenDrawInfo(window->WScreen, dri);
    }

    if (gad->Flags & GFLG_STRINGEXTEND)
    {
        struct StringExtend *strext = strinfo->Extension;
	
        if (strext->Font)
        {
            SetFont(rp, strext->Font);
        }
    }

    /* If we are made active, save contents to undobuffer (if any exists) */
    if (!(gad->Flags & GFLG_SELECTED))
    {

        if (strinfo->UndoBuffer)
        {
            D(bug("sghclick: saving into undo buffer\n"));
            strcpy(strinfo->UndoBuffer, strinfo->Buffer);
        }
        gad->Flags |= GFLG_SELECTED;
    }

    /* Get left & righ offsets of strgad text */
    text_left  = GetTextLeft (gad, &bbox, rp, IntuitionBase);
    text_right = GetTextRight(gad, &bbox, rp, IntuitionBase);

    D(bug("sghclick: text_left=%d, text_right=%d\n", text_left, text_right));
    D(bug("sghclick: disppos=%d, dispcount=%d, cursor=%d\n",
          strinfo->DispPos, strinfo->DispCount, sgw->BufferPos));
    D(bug("Gadget text: %s\n", sgw->WorkBuffer));

    /* Check if mouseclick is inside displayed text */
    if ((mousex >= text_left) && (mousex <= text_right))
    {
        /* Find new cursor pos. Uses TextFit() to handle proportional fonts. */

        struct TextExtent te;
        STRPTR      	  dispstr = strinfo->Buffer + strinfo->DispPos;

        sgw->BufferPos =   strinfo->DispPos
                           + TextFit(rp, dispstr, sgw->NumChars - strinfo->DispPos,
                                     &te, NULL, 1,
                                     mousex - text_left, rp->Font->tf_YSize);
        D(bug("sghclick: click inside text.\n"));
    }
    else /* Click not inside text */
    {
        if (mousex < text_left)
        {
            /* Click on empty space at left. Set cursor to first visible */
            sgw->BufferPos = strinfo->DispPos;
            D(bug("sghclick: click left of text.\n"));
        }
        else
        {
            /* Click on empty space at right. Set cursor to last visible */
            sgw->BufferPos = strinfo->DispPos + strinfo->DispCount;
            D(bug("sghclick: click right of text.\n"));
        }

    } /* if (click is on text or not) */

    D(bug("sghclick: new cursor position: %d\n", sgw->BufferPos));

    sgw->Actions = (SGA_USE|SGA_REDISPLAY);
    sgw->EditOp = EO_MOVECURSOR;

    SetFont(rp, oldfont);

    D(bug("Gadget text: %s\n", sgw->WorkBuffer));

    ReturnInt ("DoSGHClick", ULONG, 1);
}

/*****************************************************************************************/

VOID MoveCharsLeft(STRPTR str, UWORD first, UWORD last, UWORD steps)
{
    register UWORD i;

    for (i = first; i <= last; i ++)
    {
        str[i - steps] = str[i];
    }
    str[last] = 0;
}

/*****************************************************************************************/

STATIC ULONG DoSGHKey(struct SGWork *sgw, struct IntuitionBase *IntuitionBase)
{

    struct Gadget   	*gad;
    struct StringInfo   *strinfo;
    UBYTE           	 letter;
    ULONG           	 qual;

    EnterFunc(bug("DoSGHKey(sgw=%p)\n", sgw));

    gad = sgw->Gadget;
    strinfo = sgw->StringInfo;

    qual = sgw->IEvent->ie_Qualifier;

    D(bug("sghkey: RawKey 0x%lx, sgw_Code 0x%lx\n",sgw->IEvent->ie_Code,sgw->Code));

    sgw->EditOp = EO_NOOP;

    D(bug("sghkey: qual 0x%lx ic_Flags 0x%lx Modes 0x%lx\n",
          qual,
          GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags,
          sgw->Modes));

    if (sgw->Code == 0)
    {
        /* RAW Keys */

        letter = sgw->IEvent->ie_Code;

        if ((letter == CURSORLEFT) && (!(qual & SHIFT)))
        {
            D(bug("sghkey: CURSOR_LEFT\n"));
            if (sgw->BufferPos > 0)
            {
                sgw->EditOp = EO_MOVECURSOR;
                sgw->BufferPos --;
            }
        }
        else if ((letter == CURSORRIGHT) && (!(qual & SHIFT)))
        {

            D(bug("sghkey: CURSOR_RIGHT\n"));
            if (sgw->BufferPos < sgw->NumChars)
            {
                sgw->EditOp = EO_MOVECURSOR;
                sgw->BufferPos ++;

            }
        }
        else if ( ((letter == CURSORLEFT) && (qual & SHIFT)) ||
                  (letter == HOMERAW) )
        {
            if (sgw->BufferPos > 0)
            {
                sgw->BufferPos = 0;
                sgw->EditOp = EO_MOVECURSOR;
            }
        }
        else if ( ((letter == CURSORRIGHT) && (qual & SHIFT)) ||
                  (letter == ENDRAW) )
        {
            if (sgw->BufferPos < sgw->NumChars)
            {
                sgw->BufferPos = sgw->NumChars;
                sgw->EditOp = EO_MOVECURSOR;
            }
        }
        else if ((letter == TABRAW) && (qual & SHIFT))
        {
            D(bug("sghkey: SHIFT TAB\n"));
            sgw->EditOp = EO_SPECIAL; /* FIXME: ??? is this correct ??? */
            sgw->Code = 9;
            sgw->Actions = (SGA_USE|SGA_PREVACTIVE);
        }
        else if (letter == HELPRAW)
        {}

    }
    else if (qual & IEQUALIFIER_RCOMMAND)
    {
        /* ANSI key but pressed together with right Amiga key */
    #if 0
        sgw->EditOp  = EO_SPECIAL; /* FIXME: ??? is this correct ??? */
        sgw->Actions = (SGA_USE|SGA_REUSE|SGA_END);
    #endif
        letter = ToUpper(sgw->Code);

    /* stegerg: URGENTCHECMKE. Why disabled for AROS? */
    #ifndef __MORPHOS__
        #warning we need to handle RAMIGA-Q,RAMIGA-X here
    #else
        if(letter == 'Q')
        {
            sgw->EditOp = EO_RESET;

            if (strinfo->UndoBuffer)
            {
                D(bug("sghclick: saving into undo buffer\n"));
                stccpy(strinfo->UndoBuffer, sgw->WorkBuffer,strinfo->MaxChars);
                sgw->BufferPos = strinfo->UndoPos;
                sgw->NumChars = strlen(sgw->WorkBuffer);
            }
        }
        else
        if(letter == 'X')
        {   
            /* RCmd-X  clears the input buffer. In fixed field mode
             * jump cursor to the start of the buffer
             */
            D(bug("sghkey: CTRL-X\n"));

            if (sgw->Modes & SGM_FIXEDFIELD)
            {
                D(bug("sghkey: SGM_FIXEDFIELD\n"));
                sgw->BufferPos = 0;
                sgw->EditOp = EO_MOVECURSOR;
            }
            else
            {
                sgw->BufferPos = 0;
                sgw->WorkBuffer[sgw->BufferPos] = '\0';
                sgw->NumChars = sgw->BufferPos;
                sgw->EditOp = EO_CLEAR;
            }                                       
        }
    #endif
    }
    else
    {
        /* ANSI key */

        letter = sgw->Code;

        if ((qual & IEQUALIFIER_CONTROL) &&
            (GetPrivIBase(IntuitionBase)->IControlPrefs.ic_Flags & ICF_STRGAD_FILTER) &&
            !(sgw->Modes & SGM_NOFILTER))
        {
            /* ANSI key but pressed together with the control key */

            D(bug("sghkey: CTRL pressed..filtering on\n"));

            if (letter == CTRL_A)
            {
                /* CTRL-A  jump cursor to start of buffer */
                D(bug("sghkey: CTRL-A\n"));
                sgw->BufferPos = 0;
                sgw->EditOp = EO_MOVECURSOR;
            }
            else
                if (letter == CTRL_B)
                {
                    /* CTRL-B  move the cursor back one char */
                    D(bug("sghkey: CTRL-B\n"));
                    if (sgw->BufferPos > 0)
                    {
                        sgw->BufferPos--;
                        sgw->EditOp = EO_MOVECURSOR;
                    }
                }
                else
                    if (letter == CTRL_D)
                    {
                        /* CTRL-D  delete current char */
                        D(bug("sghkey: CTRL-D\n"));
                        if (sgw->BufferPos != sgw->NumChars)
                        {
                            MoveCharsLeft(sgw->WorkBuffer, sgw->BufferPos + 1, sgw->NumChars - 1, 1);
                            sgw->NumChars --;
                            sgw->EditOp = EO_DELFORWARD;
                        }
                    }
                    else
                        if (letter == CTRL_E)
                        {
                            /* CTRL-E  move the cursor to the end of the line */
                            D(bug("sghkey: CTRL-E\n"));
                            if (sgw->BufferPos != sgw->NumChars)
                            {
                                sgw->BufferPos = sgw->NumChars;
                                sgw->EditOp = EO_MOVECURSOR;
                            }
                        }
                        else
                            if (letter == CTRL_F)
                            {
                                /* CTRL-F  move the cursor forward one char */
                                D(bug("sghkey: CTRL-F\n"));
                                if (sgw->BufferPos != sgw->NumChars)
                                {
                                    sgw->BufferPos++;
                                    sgw->EditOp = EO_MOVECURSOR;
                                }
                            }
                            else
                                if (letter == CTRL_H)
                                {
                                    /* CTRL-H  Delete the character to the left of the cursor
                                     * In fixed field mode, move cursor to previous character
                                     */
                                    D(bug("sghkey: CTRL-A\n"));
                                    if (sgw->Modes & SGM_FIXEDFIELD)
                                    {
                                        D(bug("sghkey: SGM_FIXEDFIELD\n"));
                                        if (sgw->BufferPos > 0)
                                        {
                                            sgw->BufferPos--;
                                            sgw->EditOp = EO_MOVECURSOR;
                                        }
                                    }
                                    else
                                    {
                                        if (sgw->BufferPos > 0)
                                        {
                                            int len;
                                            len = sgw->NumChars - sgw->BufferPos;
                                            memcpy(&sgw->WorkBuffer[sgw->BufferPos-1],&sgw->WorkBuffer[sgw->BufferPos],len);

                                            sgw->BufferPos--;
                                            sgw->WorkBuffer[sgw->BufferPos + len] = 0;
                                            sgw->NumChars--;
                                            sgw->EditOp = EO_DELBACKWARD;
                                        }
                                    }
                                }
                                else
                                    if (letter == CTRL_K)
                                    {
                                        /* CTRL-K Delete from the character under the cursor to the end
                                         * of the string
                                         */
                                        D(bug("sghkey: CTRL-K\n"));
                                        sgw->WorkBuffer[sgw->BufferPos] = 0;
                                        sgw->NumChars = sgw->BufferPos;
                                        sgw->EditOp = EO_DELFORWARD;
                                    }
                                    else
                                        if (letter == CTRL_M)
                                        {
                                            /* CTRL-M Equivalent to Return or Enter
                                             */
                                            D(bug("sghkey: CTRL-M\n"));
                                            sgw->EditOp  = EO_ENTER;
                                            sgw->Code    = 0;
                                            sgw->Actions = (SGA_USE|SGA_END);
                                        }
                                        else
                                            if (letter == CTRL_W)
                                            {
                                                /* CTRL-W  Delete the previous word. In fixed field mode, jump
                                                 * cursor to the start of the previous word
                                                 */
                                                D(bug("sghkey: CTRL-W\n"));
                                                if (sgw->Modes & SGM_FIXEDFIELD)
                                                {
                                                    D(bug("sghkey: SGM_FIXEDFIELD\n"));
                                                }
                                                else
                                                {
                                                    if (sgw->BufferPos > 0)
                                                    {
                                                        int len;
                                                        int pos;
                                                        /*
                                                         * search space
                                                         */
                                                        for (pos=sgw->BufferPos-1;pos >= 0;pos--)
                                                        {
                                                            if (sgw->WorkBuffer[pos] == ' ')
                                                            {
                                                                break;
                                                            }
                                                        }
                                                        len = sgw->NumChars - sgw->BufferPos;
                                                        memcpy(&sgw->WorkBuffer[pos],&sgw->WorkBuffer[sgw->BufferPos],len);

                                                        sgw->NumChars -= sgw->BufferPos - pos;
                                                        sgw->BufferPos = pos;
                                                        sgw->WorkBuffer[sgw->BufferPos + len] = 0;
                                                        sgw->EditOp = EO_DELBACKWARD;
                                                    }
                                                }
                                            }
                                            else
                                                if (letter == CTRL_U)
                                                {
                                                    /* CTRL-U  Delete from the character to the left of the cursor
                                                     * to the start of the buffer. In fixed field mode, jump cursor to the start
                                                     * of the buffer
                                                     */
                                                    D(bug("sghkey: CTRL-U\n"));
                                                    if (sgw->Modes & SGM_FIXEDFIELD)
                                                    {
                                                        D(bug("sghkey: SGM_FIXEDFIELD\n"));
                                                        sgw->BufferPos = 0;
                                                        sgw->EditOp = EO_MOVECURSOR;
                                                    }
                                                    else
                                                    {
                                                        if (sgw->BufferPos > 0)
                                                        {
                                                            int len;
                                                            len = sgw->NumChars - sgw->BufferPos;
                                                            memcpy(&sgw->WorkBuffer[0],&sgw->WorkBuffer[sgw->BufferPos],len);

                                                            sgw->BufferPos = 0;
                                                            sgw->WorkBuffer[len] = 0;
                                                            sgw->NumChars = len;
                                                            sgw->EditOp = EO_DELBACKWARD;
                                                        }
                                                    }
                                                }
                                                else
                                                    if (letter == CTRL_X)
                                                    {
                                                        /* CTRL-X  clears the input buffer (like right amiga-x). In fixed field mode
                                                         * jump cursor to the start of the buffer
                                                         */
                                                        D(bug("sghkey: CTRL-X\n"));
                                                        if (sgw->Modes & SGM_FIXEDFIELD)
                                                        {
                                                            D(bug("sghkey: SGM_FIXEDFIELD\n"));
                                                            sgw->BufferPos = 0;
                                                            sgw->EditOp = EO_MOVECURSOR;
                                                        }
                                                        else
                                                        {
                                                            sgw->BufferPos = 0;
                                                            sgw->WorkBuffer[sgw->BufferPos] = 0;
                                                            sgw->NumChars = sgw->BufferPos;
                                                            sgw->EditOp = EO_CLEAR;
                                                        }
                                                    }
                                                    else
                                                        if (letter == CTRL_Z)
                                                        {
                                                            /* CTRL-Z  Jump cursor to the end of the buffer
                                                             */
                                                            D(bug("sghkey: CTRL-Z\n"));
                                                            sgw->BufferPos = sgw->NumChars;
                                                            sgw->EditOp = EO_MOVECURSOR;
                                                        }
        }
        else
            if (letter == BACKSPACE)
            {
                if (sgw->BufferPos != 0)
                {
                    UWORD first = sgw->BufferPos;
                    UWORD last  = sgw->NumChars - 1;
                    UWORD steps;
                    if (qual &  SHIFT)
                    {
                        steps = sgw->BufferPos;

                        sgw->BufferPos = 0;
                        sgw->NumChars -= steps;
                    }
                    else
                    {
                        sgw->NumChars --;
                        sgw->BufferPos --;
                        steps = 1;
                    }
                    MoveCharsLeft(sgw->WorkBuffer, first, last, steps);
                    sgw->EditOp = EO_DELBACKWARD;
                }
            }
            else if (letter == DELETE)
            {
                /* Check whether cursor is at the trailing 0 */
                if (sgw->BufferPos != sgw->NumChars)
                {
                    if (qual & SHIFT)
                    {
                        sgw->WorkBuffer[sgw->BufferPos] = 0;
                        sgw->NumChars = sgw->BufferPos;
                    }
                    else
                    {
                        MoveCharsLeft(sgw->WorkBuffer, sgw->BufferPos + 1, sgw->NumChars - 1, 1);
                        sgw->NumChars --;
                    }
                    sgw->EditOp = EO_DELFORWARD;
                }
            }
            else if (letter == RETURN)
            {
                D(bug("sghkey: ENTER\n"));
                sgw->EditOp  = EO_ENTER;
                sgw->Code    = 0;
                sgw->Actions = (SGA_USE|SGA_END);
            }
            else if (letter == TAB)
            {
                D(bug("sghkey: TAB\n"));
                sgw->EditOp  = EO_SPECIAL; /* FIXME: ??? is this correct ??? */
                sgw->Actions = (SGA_USE|SGA_NEXTACTIVE);
            }
            else
            {

                /* Validity check of letter */
                if (gad->Activation & GACT_LONGINT)
                {
                    if (letter == '-')
                    {
                        if ((sgw->BufferPos != 0) || ((sgw->NumChars > 0) && (sgw->WorkBuffer[0] == '-')))
                            sgw->EditOp = EO_BADFORMAT;

                    }
                    else if ((letter < 0x30) ||  (letter > 0x39))
                    {
                        sgw->EditOp = EO_BADFORMAT;
                    }
                    else if (sgw->WorkBuffer[sgw->BufferPos] == '-')
                    {
                        sgw->EditOp = EO_BADFORMAT;
                    }
                }
                else /* Integer gadget ? */
                {
                    /* Is key a printable character ? */
#warning Locale should be used here...
                    if ((letter & 0x60) == 0)
                    {
                        sgw->EditOp = EO_BADFORMAT;
                    }
                } /* if (integer or string gadget) */

                if (sgw->EditOp != EO_BADFORMAT)
                {

                    if ((sgw->Modes & SGM_REPLACE) && (sgw->WorkBuffer[sgw->BufferPos]))
                    {
                        D(bug("sghkey: replacing char at pos %d\n", sgw->BufferPos));

                        sgw->WorkBuffer[sgw->BufferPos] = letter;
                        sgw->EditOp = EO_REPLACECHAR;

                        if (sgw->BufferPos < strinfo->MaxChars - 1)
                            sgw->BufferPos ++;

                    }
                    else
                    {
                        /* Insert mode. Check if there is space for one more character
                        ** NOTE: MaxChars inludes traing \0, so therefore the '- 1'
                        */
                        if (sgw->NumChars < (strinfo->MaxChars - 1))
                        {
                            register UWORD i;

                            D(bug("sghkey: inserting char at pos %d\n", sgw->BufferPos));
                            /* Move characters to the right of insertion point one step to the right */
                            for (i = sgw->NumChars; i > sgw->BufferPos; i --)
                            {
                                sgw->WorkBuffer[i] = sgw->WorkBuffer[i - 1];
                            }

                            /* Insert letter  */
                            sgw->WorkBuffer[i] = letter;
                            sgw->EditOp = EO_INSERTCHAR;
                            sgw->NumChars ++;
                            sgw->BufferPos ++;

                        }
                        else
                        {
                            sgw->EditOp = EO_NOOP;
                        } /* if (enough space for ione mor letter) */
                    } /* if (Replace or Insert mode) */
                } /* If (user pressed valid letter) */
            } /* Vanilla key but not backspace, delete, ... */
    } /* if (key or scancode) */

    /* Null-terminate the new string */
    sgw->WorkBuffer[sgw->NumChars] = 0;

    if (sgw->EditOp != EO_NOOP)
        sgw->Actions |= (SGA_USE|SGA_REDISPLAY);

    /* Integer gadget ? */
    if ((sgw->Actions & SGA_USE) && (gad->Activation & GACT_LONGINT))
    {
        sgw->LongInt = atol(sgw->WorkBuffer);
        D(bug("Updated string number to %d\n", sgw->LongInt));
    }

    ReturnInt ("DoSGHKey", ULONG, 1);
}

/*****************************************************************************************/

AROS_UFH3(ULONG, GlobalEditFunc,
          AROS_UFHA(struct Hook *,      hook,       A0),
          AROS_UFHA(struct SGWork *,        sgw,        A2),
          AROS_UFHA(ULONG *,            command,    A1)
         )
{
    AROS_USERFUNC_INIT

    ULONG retcode = 0;

    switch (*command)
    {
	case SGH_CLICK:
            retcode = DoSGHClick(sgw, (struct IntuitionBase *)hook->h_Data);
            break;

	case SGH_KEY:
            retcode = DoSGHKey  (sgw, (struct IntuitionBase *)hook->h_Data);
            break;
    }

    return retcode;

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

VOID RefreshStrGadget(struct Gadget *gad,
                      struct Window     *win,
                      struct Requester  *req,
                      struct IntuitionBase  *IntuitionBase)
{
    struct GadgetInfo    gi;
    struct RastPort 	*rp;

    EnterFunc(bug("RefreshStrGadget(gad=%p, win=%s)\n", gad, win->Title));

    SetupGInfo(&gi, win, req, gad, IntuitionBase);

    if ((rp = ObtainGIRPort(&gi)))
    {
        if (gad->GadgetRender)
        {
            if (gad->Flags & GFLG_GADGIMAGE)
            {
                DrawImage(rp,
                          (struct Image *)gad->GadgetRender,
                          gad->LeftEdge,
                          gad->TopEdge);
            }
            else
            {
                DrawBorder(rp,
                           (struct Border *)gad->GadgetRender,
                           gad->LeftEdge,
                           gad->TopEdge);
            }
        }
        ReleaseGIRPort(rp);
    }
    UpdateStrGadget(gad, win, req, IntuitionBase);

    ReturnVoid("RefreshStrGadget");
}

/*****************************************************************************************/

VOID UpdateStringInfo(struct Gadget *gad)
{
    /* Updates the stringinfo in case user has set some fields */

    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;

    EnterFunc(bug("UpdateStringInfo(gad=%p)\n", gad));

#if 0
    if (gad->Activation & GACT_LONGINT)
    {
        if ((strinfo->NumChars != 0) || (strinfo->LongInt != 0))
        {
            /* NOTE: The max number of chars written INCLUDES trailing \0 */
            snprintf(strinfo->Buffer, strinfo->MaxChars, "%d", strinfo->LongInt);
        }
    }
#endif

    strinfo->NumChars = strlen(strinfo->Buffer);

    if (strinfo->BufferPos > strinfo->NumChars)
    {
        strinfo->BufferPos = strinfo->NumChars;
    }

    D(bug("%s gadget contains buffer %s of length %d\n",
          (gad->Activation & GACT_LONGINT) ? "Integer" : "String",
          strinfo->Buffer, strinfo->NumChars));

    ReturnVoid("UpdateStringInfo");
}

/*****************************************************************************************/

VOID UpdateStrGadget(struct Gadget  *gad,
                     struct Window      *win,
                     struct Requester   *req,
                     struct IntuitionBase   *IntuitionBase)
{
    struct GadgetInfo    gi;
    struct BBox     	 bbox;
    struct StringInfo   *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD           	 text_left;
    UWORD           	 text_top;
    struct RastPort 	*rp;
    STRPTR          	 dispstr;
    UWORD           	 dispstrlen;
    UWORD           	 pens[NUMPENS];

    EnterFunc(bug("UpdateStrGadget(current text=%s)\n", strinfo->Buffer));

    SetupGInfo(&gi, win, req, gad, IntuitionBase);

    rp = ObtainGIRPort(&gi);
    if (!rp) return;

    GetPensAndFont(gad, pens, win, rp, IntuitionBase);

    CalcBBox(win, req, gad, &bbox);

    /* Update the stringinfo struct in case of user change */
    UpdateStringInfo(gad);

    /* Update the DispPos and DispCount fields so that the gadget renders properly */
    UpdateDisp(gad, &bbox, rp, IntuitionBase);


    dispstr = strinfo->Buffer + strinfo->DispPos;
    dispstrlen = MIN(strinfo->DispCount, strinfo->NumChars - strinfo->DispPos);

    /* Clear the background */
    SetAPen(rp, pens[STRBACKPEN]);
    SetDrMd(rp, JAM1);

    RectFill(rp,
             bbox.Left,
             bbox.Top,
             bbox.Left + bbox.Width  - 1,
             bbox.Top  + bbox.Height - 1);

    text_left = GetTextLeft(gad, &bbox, rp, IntuitionBase);


    /* Write the text into the gadget */
    SetABPenDrMd(rp, pens[STRTEXTPEN], pens[STRBACKPEN], JAM2);

    text_top =   bbox.Top
                 + ((bbox.Height - rp->Font->tf_YSize) >> 1)
                 + rp->Font->tf_Baseline;

    Move(rp, text_left, text_top);

    D(bug("usg: Writing text %s of length %d at (%d, %d)\n",
          dispstr, strinfo->DispCount, text_left, text_top));

    Text(rp, dispstr, strinfo->DispCount);

    if (gad->Flags & GFLG_SELECTED)
    {
        UWORD cursoroffset = strinfo->BufferPos - strinfo->DispPos;
        /* Render cursor */
//      SetAfPt(rp,NULL,0);
        D(bug("usg: Number of characters: %d\n", strinfo->NumChars));


        text_left += TextLength(rp, dispstr, cursoroffset);

    #if 1
        if (strinfo->BufferPos < strinfo->NumChars)
        {
            SetABPenDrMd(rp, pens[STRTEXTPEN], pens[CURSORPEN], JAM2);
            Move(rp, text_left, text_top);
            Text(rp,
                 dispstr + cursoroffset,
                 1 );
        }
        else
        {
            struct TextExtent   te;
	    
            SetABPenDrMd(rp, pens[STRTEXTPEN], pens[CURSORPEN], JAM2|INVERSVID);
            TextExtent(rp, cursorkey, CURSORKEYLEN,&te);
	    
            RectFill(rp,
                text_left+te.te_Extent.MinX,
                text_top+te.te_Extent.MinY,
                text_left+te.te_Extent.MaxX,
                text_top+te.te_Extent.MaxY);
        }
    #else
        Move(rp, text_left, text_top);
        Text(rp,
             ((strinfo->BufferPos < strinfo->NumChars)
              ? dispstr + cursoroffset
              : cursorkey),
             1 );
    #endif
    
    } /* if (gadget selected => render cursor) */

    if (gad->Flags & GFLG_DISABLED )
    {
        struct DrawInfo *dri = GetScreenDrawInfo(win->WScreen);

        RenderDisabledPattern(rp, dri, bbox.Left,
                              bbox.Top,
                              bbox.Left + bbox.Width - 1,
                              bbox.Top + bbox.Height - 1,
                              IntuitionBase );

        if (dri) FreeScreenDrawInfo(win->WScreen, dri);
    }

    ReleaseGIRPort(rp);

    ReturnVoid("UpdateStrGadget");
}
