/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Code for AROS GTYP_STRGADGET.
    Lang: english
*/


#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <intuition/screens.h>
#include <intuition/sghooks.h>
#include <intuition/cghooks.h>
#include <devices/inputevent.h>
#include <aros/asmcall.h>
#include <stdlib.h> /* atol() */
#include "intuition_intern.h"
#include "strgadgets.h"

#undef DEBUG
#define DEBUG 0
#	include <aros/debug.h>


#define CURSORPEN	0
#define STRBACKPEN	1
#define STRTEXTPEN	4

#define NUMPENS 3
#define STRALIGNMASK (GACT_STRINGLEFT|GACT_STRINGCENTER|GACT_STRINGRIGHT)

AROS_UFP3(STATIC ULONG, GlobalEditFunc,
    AROS_UFPA(struct Hook *,		hook,		A0),
    AROS_UFPA(struct SGWork *,		sgw,		A2),
    AROS_UFPA(ULONG *, 			command,	A1)
);

struct RPPres
{
    UBYTE OldAPen;
    UBYTE OldBPen;
    UBYTE OldDrMd;
    struct TextFont *OldFont;
};

#define PreserveRP(rp, pres)	\
    pres.OldAPen = GetAPen(rp);	\
    pres.OldBPen = GetBPen(rp);	\
    pres.OldDrMd = GetDrMd(rp);	\
    pres.OldFont = rp->Font;
    
#define ResetRP(rp, pres)	\
    SetAPen(rp, pres.OldAPen);	\
    SetBPen(rp, pres.OldBPen);	\
    SetDrMd(rp, pres.OldDrMd);	\
    SetFont(rp, pres.OldFont);
    

#define CharXSize(char, rp) 			\
 ((rp->Font->tf_Flags & FPF_PROPORTIONAL) 	\
 	? rp->Font->tf_XSize 			\
 	: TextLength(rp, &(char), 1))


#undef MIN
#define MIN(a, b) ((a < b) ? a : b)

/*****************
**  UpdateDisp  **
*****************/
void UpdateDisp(struct Gadget 		*gad,
		struct BBox		*bbox,
		struct RastPort		*rp,
		struct IntuitionBase	*IntuitionBase)
{
   
/*    WORD biggest_valid_disppos; */
    /* dispcount of the charcter at end of a string */
/*    UWORD end_dispcount; */
    struct TextExtent te;
    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;
    STRPTR dispstr;

    /* If the cursor is at the trailing \0, insert a SPACE instead */
    if (strinfo->BufferPos == strinfo->NumChars)
    	strinfo->Buffer[strinfo->NumChars] = 0x20;

    if (strinfo->BufferPos < strinfo->DispPos)
    	strinfo->DispPos = strinfo->BufferPos;
    else if (strinfo->BufferPos > strinfo->DispPos)
    {
    	UWORD strsize = TextLength(rp,
    				strinfo->Buffer + strinfo->DispPos,
    				strinfo->BufferPos - strinfo->DispPos + 1);
    	    
	if (strsize > bbox->Width)
	{
    	    strinfo->DispPos = strinfo->BufferPos
    	    		- TextFit(rp, 
    				&(strinfo->Buffer[strinfo->BufferPos]),
    			   	strinfo->NumChars, &te, NULL, -1,
    			   	bbox->Width, rp->Font->tf_YSize)
    			+ 1;
	    D(bug("ed: disppos to %d, because cursor was out of visible area\n",
		strinfo->DispPos));
	}
    }   
    
    /* Check if there is there is free space in the gad but chars 
    ** scrolled out.
    */
/*    end_dispcount = TextFit(rp,
    	&(strinfo->Buffer[strinfo->NumChars - 1]),
    	strinfo->NumChars, &te, NULL,
    	-1, bbox->Width, rp->Font->tf_YSize);

    if (strinfo->BufferPos == strinfo->NumChars)
    	end_dispcount --;
    			
    D(bug("ud: Space available in strgad: %d\n", end_dispcount));
    			
    biggest_valid_disppos = strinfo->NumChars - end_dispcount;
    
    D(bug("ud: Biggest valid disppos: %d\n", biggest_valid_disppos));
    if (strinfo->DispPos > 0)
    {
    	if (strinfo->DispPos > biggest_valid_disppos)
    	{
    	    strinfo->DispPos = biggest_valid_disppos;
    	}
    }
*/    
    /* Update the DispCount */
    /* It might be necessary with special handling for centre aligned gads */
    dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    strinfo->DispCount = TextFit(rp,
    			dispstr,
    			strinfo->NumChars - strinfo->DispPos,
    			&te,
    			NULL,
    			1,
    			gad->Width,
    			rp->Font->tf_YSize);

    /* 0-terminate string */			
    strinfo->Buffer[strinfo->NumChars] = 0x00;
    return;
}

/******************
**  GetTextLeft  **
******************/
STATIC UWORD GetTextLeft(struct Gadget		*gad,
			struct BBox		*bbox,
			struct RastPort		*rp,
			struct IntuitionBase	*IntuitionBase)
{
    /* Gets left position of text in the string gadget */
    
    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD  text_left;
    STRPTR dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    UWORD dispstrlen;
    
    dispstrlen = strinfo->NumChars - strinfo->DispPos;
    
    /* Calcluate start offset of gadget text */
    switch (gad->Activation & STRALIGNMASK)
    {
    case GACT_STRINGLEFT:
        text_left = bbox->Left;
        break;

    case GACT_STRINGCENTER:
       	D(bug("GACT_STRINGCENTER not implemented yet\n"));
       	break;

    case GACT_STRINGRIGHT:
    	text_left =  bbox->Left 
    		   + (  bbox->Width  - 1
    		   	- TextLength(rp, dispstr, dispstrlen)
    		     );
    	break;
    }
    return (text_left);
}

/*******************
**  GetTextRight  **
*******************/
STATIC UWORD GetTextRight(struct Gadget		*gad,
			struct BBox		*bbox,
			struct RastPort		*rp,
			struct IntuitionBase	*IntuitionBase)
{
    /* Gets right offset of text in the string gadget */
    
    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD  text_right;
    STRPTR dispstr = &(strinfo->Buffer[strinfo->DispPos]);
    UWORD dispstrlen;
    
    dispstrlen = strinfo->NumChars - strinfo->DispPos;
    
    /* Calcluate start offset of gadget text */
    switch (gad->Activation & STRALIGNMASK)
    {
    case GACT_STRINGLEFT:
    	text_right =  bbox->Left + TextLength(rp, dispstr, dispstrlen);
        break;

    case GACT_STRINGCENTER:
       	D(bug("GACT_STRINGCENTER not implemented yet\n"));
       	break;

    case GACT_STRINGRIGHT:
        text_right = bbox->Left + bbox->Width  - 1;
    	break;
    }
    return (text_right);    
}


/*********************
**  GetPensAndFont  **
*********************/
STATIC VOID GetPensAndFont(struct Gadget *gad,
			UWORD 		 *pens,
			struct Window	*win)
{   

    struct DrawInfo *dri = GetScreenDrawInfo(win->WScreen);

    D(bug("GetPensAndFont(gad=%p, pens=%p, win=%s, screen=%s)\n",
    	gad, pens, win->Title, win->WScreen->Title));

    
    if (gad->Flags & GFLG_STRINGEXTEND)
    {
    	struct StringExtend *strext;

   	 
    	strext = ((struct StringInfo *)gad->SpecialInfo)->Extension;
    	
    	if (strext->Font)
    	    SetFont(win->RPort, strext->Font);

    	if ((gad->Flags & GFLG_SELECTED) == GFLG_SELECTED)
    	{
    	    pens[STRTEXTPEN]	= strext->ActivePens[0];
    	    pens[STRBACKPEN]	= strext->ActivePens[1];

	}
	else
	{
    	    pens[STRTEXTPEN]	= strext->Pens[0];
    	    pens[STRBACKPEN]	= strext->Pens[1];
	}
	
	D(bug("gpaf: strext exists\n"));
    }
    else
    {
    	/* We don't care to lock the screen because the window we're
    	** drawn on does this. If the window is moved to another
    	** public screen we will be called again to rerender (and get new
    	** valid pens). If the window is closed, and the screen is closed
    	** we won't be here anymore.
    	*/

    	if ((gad->Flags & GFLG_SELECTED) == GFLG_SELECTED)
    	{
    	    pens[STRTEXTPEN]	= dri->dri_Pens[HIGHLIGHTTEXTPEN];
    	    pens[STRBACKPEN]	= dri->dri_Pens[SHINEPEN];

	}
	else
	{
    	    pens[STRTEXTPEN]	= dri->dri_Pens[TEXTPEN];
    	    pens[STRBACKPEN]	= dri->dri_Pens[BACKGROUNDPEN];
	}
    	

    }
    pens[CURSORPEN] = dri->dri_Pens[FILLPEN];
    
    D(bug("gpaf: c=%d, t=%d, b=%d\n",
    	pens[CURSORPEN], pens[STRTEXTPEN], pens[STRBACKPEN]));
    ReturnVoid("GetPensAndFont");
}

/*********************
**  HandleStrInput  **
*********************/

ULONG HandleStrInput(	struct Gadget 		*gad,
			struct GadgetInfo	*ginfo,
		  	struct InputEvent	*ievent,
		  	UWORD			*imsgcode,
		      	struct IntuitionBase	*IntuitionBase)
{
    struct SGWork sgw;
    struct Hook globhook;
    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;
    struct StringExtend *strext = NULL;
    ULONG command = 0;

    D(bug("HandleStrInput(gad=%p, ginfo=%p, ievent=%p)\n",
    	gad, ginfo, ievent));
    if (!ginfo)
    	ReturnInt("HandleStrInput", ULONG, 0UL);
    	
    /* Initialize SGWork */
    sgw.Gadget		= gad;
    sgw.StringInfo	= strinfo;
    sgw.WorkBuffer	= strinfo->Buffer; /* default */
    sgw.PrevBuffer	= strinfo->Buffer;
    sgw.Modes		= 0;
    sgw.IEvent		= ievent;
    sgw.Code		= 0;
    sgw.BufferPos	= strinfo->BufferPos;
    sgw.NumChars	= strinfo->NumChars;
    sgw.Actions		= 0;
    sgw.LongInt		= strinfo->LongInt;
    sgw.GadgetInfo	= ginfo;
    sgw.EditOp		= EO_NOOP;

    if (gad->Flags & GFLG_STRINGEXTEND)
    {
    	D(bug("hsi: Extended gadget\n"));
    	strext = strinfo->Extension;
    	if (strext->WorkBuffer)
    	    sgw.WorkBuffer = strext->WorkBuffer;
    	    
    	sgw.Modes = strext->InitialModes;
    }

    switch (ievent->ie_Class)
    {
    case IECLASS_RAWMOUSE:
    	if (ievent->ie_Code == SELECTDOWN)
    	    command = SGH_CLICK;
    	D(bug("hsi: RAWMOUSE event\n"));    	    
    	break;
    	    
    case IECLASS_RAWKEY:
    	command = SGH_KEY;
    	D(bug("hsi: RAWKEY event\n"));    	    
    	break;
    }
    
    if (!command)
    	ReturnInt("HandleStrInput", ULONG , 0UL);
    	
    /* Call the global edititng hook */
    globhook.h_Entry	= (APTR)AROS_ASMSYMNAME(GlobalEditFunc);
    globhook.h_SubEntry = NULL;
    globhook.h_Data	= IntuitionBase;
    
    D(bug("hsi: calling global hook\n"));
    CallHookPkt(&globhook, &sgw, &command);
    
    /* If there is a local edit hook, run it */
    if (strext)
    {
    	if (strext->EditHook)
    	{
    	    D(bug("hsi: calling local hook\n"));    	
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
    }

    if (sgw.Actions & SGA_BEEP)
    {
    	D(bug("SGA_BEEP not yet implemented. (lack of DisplayBeep())\n"));
    }
    if (sgw.Actions & SGA_END)
    {
    	gad->Flags &= ~GFLG_SELECTED;
    	*imsgcode = sgw.Code;
    	D(bug("hsi: SGA_END\n"));
    }
    
    if (sgw.Actions & SGA_REDISPLAY)
    {
    	D(bug("hsi: SGA_REDISPLAY\n"));
    	/* Hack for making strgclass work */
    	if ((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_CUSTOMGADGET)
    	{
    	    struct RastPort *rp;
    	    D(bug("hsi: Rerendering boopsi gadget\n"));
    	        	    
    	    if ((rp = ObtainGIRPort(ginfo)) != NULL)
    	    {
    	    	DoMethod((Object *)gad, GM_RENDER, ginfo, rp, GREDRAW_UPDATE); 
    	    	
    	    	ReleaseGIRPort(rp);
    	    }
    	} /* if gadget is a strgclass object */
    	else
    	{
    	    D(bug("hsi: Rerendering intuition gadget\n"));    	
    	    RefreshStrGadget(gad, ginfo->gi_Window, IntuitionBase);
    	}
    }

    ReturnInt("HandleStrInput", ULONG, sgw.Actions);
}

 
/*****************
**  DoSGHClick  **
*****************/

STATIC ULONG DoSGHClick(struct SGWork *sgw, struct IntuitionBase *IntuitionBase)
{

    struct Gadget *gad;
    struct StringInfo *strinfo;
    struct BBox bbox;
    
    struct TextFont *oldfont;
    struct RastPort *rp;

    UWORD text_left, text_right;
    
    UWORD mousex = sgw->IEvent->ie_position.ie_xy.ie_x;
    
    D(bug("DoSGHClick(sgw=%p)\n", sgw));
    
    rp	    =  sgw->GadgetInfo->gi_RastPort;
    oldfont = rp->Font;    
    gad     = sgw->Gadget;
    strinfo = (struct StringInfo *)gad->SpecialInfo;
    
    CalcBBox(sgw->GadgetInfo->gi_Window, gad, &bbox);

    if (gad->Flags & GFLG_STRINGEXTEND)
    {
    	struct StringExtend *strext = strinfo->Extension;
    	if (strext->Font)
    	     SetFont(rp, strext->Font);
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
    /* Check if mouseclick is inside displayed text */
    if ((mousex >= text_left) && (mousex <= text_right))
    {
    	/* Find new cursor pos. Uses TextFit() to handle proportional fonts. */

    	struct TextExtent te;
    	STRPTR dispstr = strinfo->Buffer + strinfo->DispPos;
    	
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

    ReturnInt ("DoSGHClick", ULONG, 1);
}

/***************
**  DoSGHKey  **
***************/
#define CURSOR_RIGHT	'C'
#define CURSOR_LEFT	'D'
#define SHIFT_CLEFT	" @"
#define SHIFT_CRIGHT	" A"
#define HELP		"?~"
#define BACKSPACE	0x08
#define TAB		0x09
#define ENTER		0x0A
#define RETURN		0x0D
#define DELETE		0x7F

#define CSI		155

VOID MoveCharsLeft(STRPTR str, UWORD first, UWORD last, UWORD steps)
{
    register UWORD i;
    
    for (i = first; i <= last; i ++)
    {				
	str[i - steps] = str[i];
    }
    str[last] = 0;
}
    
/* !!!! Temporary kludge until there is a keymap.library !!!! */
extern LONG intui_RawKeyConvert(struct InputEvent *, STRPTR,
					LONG, struct KeyMap *);

STATIC ULONG DoSGHKey(struct SGWork *sgw, struct IntuitionBase *IntuitionBase)
{

    struct Gadget *gad;
    struct StringInfo *strinfo;
    
    UBYTE letter;
    #define KEYBUFSIZE 10
    UBYTE keybuf[KEYBUFSIZE];
    LONG numchars;
    ULONG qual;
    
    gad = sgw->Gadget;
    strinfo = sgw->StringInfo;
    
    numchars = intui_RawKeyConvert(sgw->IEvent, keybuf, KEYBUFSIZE, NULL);
    if (numchars == -1)
    	return (FALSE);
    	
    D(bug("sghkey: RawKeyConvert successfull\n"));
    	
    letter = keybuf[0];
    qual = sgw->IEvent->ie_Qualifier;
    
    #define SHIFT (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)
    D(bug("sghkey: converted to letter %d\n", letter));
    if (letter == CSI)
    {
    	D(bug("sghkey: <CSI> found\n"));
    	letter = keybuf[1];
   	D(bug("sghkey: Keybuf=%s\n", keybuf + 1));
    	if (letter == CURSOR_LEFT)
    	{
    	    D(bug("sghkey: CURSOR_LEFT\n"));
    	    if (sgw->BufferPos > 0)
    	    {
    	    	sgw->EditOp = EO_MOVECURSOR;
    	    	sgw->BufferPos --;
    	    }
    	}
    	else if (letter == CURSOR_RIGHT)
    	{
    	    
    	    D(bug("sghkey: CURSOR_RIGHT\n"));
    	    if (sgw->BufferPos < sgw->NumChars)
    	    {
    	    	sgw->EditOp = EO_MOVECURSOR;
    	    	sgw->BufferPos ++;
    	    	     
    	    }
    	}
    	else if (strcmp(keybuf + 1, SHIFT_CLEFT) == 0)
    	{
    	    if (sgw->BufferPos > 0)
    	    {
    	    	sgw->BufferPos = 0;
    	    	sgw->EditOp = EO_MOVECURSOR;
    	    }
    	}
    	else if (strcmp(keybuf + 1, SHIFT_CRIGHT) == 0)
    	{
    	    if (sgw->BufferPos < sgw->NumChars)
    	    {
    	    	sgw->BufferPos = sgw->NumChars;
    	    	sgw->EditOp = EO_MOVECURSOR;
    	    }
    	}
    	else if (strcmp(keybuf + 1, HELP) == 0)
    	{
    	}
    	
    }
    else if (letter == BACKSPACE)
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
    else if (letter == ENTER || letter == RETURN)
    {
    	D(bug("sghkey: ENTER\n"));
    	sgw->EditOp  = EO_ENTER;
    	sgw->Actions = (SGA_USE|SGA_END);
    }
    else
    {
    
    	/* Validity check of letter */
    	if (gad->Activation & GACT_LONGINT)
    	{
    	    if (letter == '-')
    	    {
    	    	if (sgw->BufferPos != 0)
    	    	    sgw->EditOp = EO_BADFORMAT;
    	
    	    }
    	    else if (letter < 0x30 && letter > 0x39)
    	    {
    	     	sgw->EditOp = EO_BADFORMAT;
    	    }
    	}
    	else /* Integer gadget ? */
    	{
    	    /* Is key a standard ASCII letter number or '-' ? */
    	    if (    (letter < 32) 
    	         || (letter > 128)
    	        )
    	    {
    	    	sgw->EditOp = EO_BADFORMAT;
    	    }
        } /* if (integer or string gadget) */
    
    	if (sgw->EditOp != EO_BADFORMAT)
    	{
    	
    	    if (sgw->Modes & SGM_REPLACE)
    	    {
	    	D(bug("sghkey: replacing char at pos %d\n", sgw->BufferPos));
	    
    	    	sgw->WorkBuffer[sgw->BufferPos] = letter;
    	    	sgw->EditOp = EO_REPLACECHAR;
    	    
    	    	if (sgw->BufferPos < strinfo->MaxChars - 1)
    	    	    sgw->BufferPos ++;
    	    }
    	    else 
    	    {
    	        /* Insert mode. Check if there is space for one more character */
    	        if (sgw->NumChars < strinfo->MaxChars)
    	    	{
		    register UWORD i;
		
		    D(bug("sghkey: inserting char at pos %d\n", sgw->BufferPos));
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
    } /* if (key or scancode) */
    
    /* Integer value changed ? */
    if (    (gad->Activation & GACT_LONGINT)
    	 && ((sgw->EditOp == EO_INSERTCHAR) || (sgw->EditOp == EO_REPLACECHAR)) )
    {
    	sgw->LongInt = atol(sgw->WorkBuffer);
    }
    
    if (sgw->EditOp != EO_NOOP)	
    	sgw->Actions |= (SGA_USE|SGA_REDISPLAY);	     
    return (1);   
}


/*********************
**  GlobalEditFunc  **
*********************/
AROS_UFH3(STATIC ULONG, GlobalEditFunc,
    AROS_UFHA(struct Hook *,		hook,		A0),
    AROS_UFHA(struct SGWork *,		sgw,		A2),
    AROS_UFHA(ULONG *, 			command,	A1)
)
{
    ULONG retcode;
        
    switch (*command)
    {
    case SGH_CLICK:
    	DoSGHClick(sgw, (struct IntuitionBase *)hook->h_Data);
    	break;
    
    case SGH_KEY:
    	DoSGHKey  (sgw, (struct IntuitionBase *)hook->h_Data);    
    	break;
    
    }
    return (retcode);
}

/***********************
**  RefreshStrGadget  **
***********************/

VOID RefreshStrGadget(struct Gadget	*gad,
		struct Window		*win,
		struct IntuitionBase	*IntuitionBase)
{

    D(bug("RefreshStrGadget(gad=%p, win=%s)\n", gad, win->Title));
	
    if (gad->GadgetRender)
    {
  	DrawBorder(win->RPort,
    		(struct Border *)gad->GadgetRender,
    		gad->LeftEdge,
    		gad->TopEdge);
    }
    
    UpdateStrGadget(gad, win, IntuitionBase);
    
    ReturnVoid("RefreshStrGadget");
}		


/***********************
**  UpdateStrGadget  **
***********************/

VOID UpdateStrGadget(struct Gadget	*gad,
		struct Window		*win,
		struct IntuitionBase	*IntuitionBase)
{
    struct BBox bbox;

    struct StringInfo *strinfo = (struct StringInfo *)gad->SpecialInfo;
    UWORD text_left;
    
    UWORD text_top;
    
    struct RPPres rppres;
    struct RastPort *rp = win->RPort;
    
    STRPTR dispstr;
    UWORD dispstrlen;
    
    UWORD pens[NUMPENS];
    
    D(bug("UpdateStrGadget(gad=%p, win=%s)\n", gad, win->Title));

    CalcBBox(win, gad, &bbox);
    /* Update the strinfo->Disp* fields in case application has set
    ** strinfo->BufferPos;
    ** Assures (BufferPos - DispPos < DispCount)
    */

    D(bug("Calling UpdateDisp: disppos=%d, dispcount=%d, cursor=%d\n",
    	strinfo->DispPos, strinfo->DispCount, strinfo->BufferPos));
    UpdateDisp(gad, &bbox, rp, IntuitionBase);

    D(bug("usg: DispPos update: dispos=%d, dispcount=%d, cursor=%d\n",
    	strinfo->DispPos, strinfo->DispCount, strinfo->BufferPos)); 
    	
    dispstr = strinfo->Buffer + strinfo->DispPos;
    dispstrlen = MIN(strinfo->DispCount, strinfo->NumChars - strinfo->DispPos);
            


    /* Preserve rastport stuff */
    PreserveRP(rp, rppres);

    GetPensAndFont(gad, pens, win);
    
    /* Clear the background */
    SetAPen(rp, pens[STRBACKPEN]);
    SetDrMd(rp, JAM1);

    D(bug("usg: Filling background with pen %d\n", pens[STRBACKPEN]));
    RectFill(rp,
    	bbox.Left,
    	bbox.Top,
    	bbox.Left + bbox.Width  - 1,
    	bbox.Top  + bbox.Height - 1);

    text_left = GetTextLeft(gad, &bbox, rp, IntuitionBase);
    
    		
    
    
    /* Write the text into the gadget */
    SetABPenDrMd(rp, pens[STRTEXTPEN], pens[STRBACKPEN], JAM1);

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
        SetABPenDrMd(rp, pens[STRTEXTPEN], pens[CURSORPEN], JAM2);
	D(bug("usg: Number of characters: %d\n", strinfo->NumChars));
	

    	text_left += TextLength(rp, dispstr, cursoroffset);

    	Move(rp, text_left, text_top);
    	Text(rp, 
    	    ((strinfo->BufferPos < strinfo->NumChars)
    	    	? dispstr + cursoroffset
    	    	: (STRPTR)" "),
	    1 );
    } /* if (gadget selected => render cursor) */
    
    /* Reinsert old values */
    ResetRP(rp, rppres);

    ReturnVoid("UpdateStrGadget");
}
