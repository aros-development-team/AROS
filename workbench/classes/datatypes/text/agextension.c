/**********************************************************************
 text.datatype - amigaguide extensions for MorphOS

 by Stefan Ruppert
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/textclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/layers.h>
#include "text_intern.h"

#include "libdefs.h"

#ifndef __AROS__
#ifndef __MORPHOS__
#include <libraries/reqtools.h>
#include <proto/reqtools.h>
#endif
#endif

#include <aros/libcall.h>

#ifdef COMPILE_DATATYPE
#include <proto/datatypes.h>
#endif

#include "compilerspecific.h"
#include "getsearchstring.h"
#include "support.h"
#include "textclass.h"

/* Define the following to enable the debug version */
#undef MYDEBUG
#include "debug.h"



#ifdef MORPHOS_AG_EXTENSION

/* need this function from textclass.c module. */
struct Line *NewFindLine(struct Text_Data *td, LONG x, LONG y, LONG *xpos, LONG *line_xpos);
__varargs68k IPTR notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ...);


void PrepareAGExtension(struct Text_Data *td)
{
    /* calculate number of links in text document and mark first link
       which can be activated using STM_ACTIVATE_FIELD trigger method.
    */
    if (td->links == -1)
    {
	struct Line *line = (struct Line *) List_First(&td->line_list);
	LONG y = 0;
	while(line)
	{
	    if (line->ln_Flags & LNF_LINK)
	    {
		if(td->links == -1)
		{
		    td->links = 0;
		    td->marked_line = line;
		    line->ln_Flags |= LNF_MARKED;
		}
		++td->links;
            }
	    /* setup correct yoffset */
	    line->ln_YOffset = y;
	    if (line->ln_Flags & LNF_LF)
		y += td->vert_unit;
	    line = (struct Line *) Node_Next(line);
	}
    }
}

void DrawLineBevel(struct Text_Data *td, struct RastPort *rp, 
                   struct Line *line, BOOL text)
{
    LONG x1 = td->left + line->ln_XOffset - td->horiz_top * td->horiz_unit;
    LONG y1 = td->top + line->ln_YOffset - td->vert_top * td->vert_unit;
    LONG x2;
    LONG y2;
    UBYTE above,below;

    ULONG apen, bpen, mode;
    struct TextFont *oldfont;
    struct Region *new_region = NULL;
    struct Region *old_region = NULL;
    struct Rectangle rect;

    if (text)
    {

	if (!(new_region = NewRegion()))
	    return;

	rect.MinX = td->left;
	rect.MinY = td->top;
	rect.MaxX = td->left + td->width - 1;
	rect.MaxY = td->top + td->vert_visible * td->vert_unit - 1;


	OrRectRegion(new_region,&rect);
	old_region = installclipregion(rp->Layer,new_region);
    }

    x2 = x1 + line->ln_Width;
    y2 = y1 + line->ln_Height - 1;

    if(line->ln_Flags & LNF_SELECTED)
    {
	below = td->shinepen;
	above = td->shadowpen;
    } else
    {
	above = td->shinepen;
	below = td->shadowpen;
    }

    if (text)
    {
	UBYTE bgpen = line->ln_BgPen;
	if (line->ln_Flags & LNF_MARKED)
	    bgpen = td->fillpen;

	GetRPAttrs(rp,
		   RPTAG_APen, (ULONG) &apen,
		   RPTAG_BPen, (ULONG) &bpen,
		   RPTAG_DrMd, (ULONG) &mode,
		   RPTAG_Font, (ULONG) &oldfont,
		   TAG_DONE);

	SetFont(rp, td->font);
	SetABPenDrMd(rp, line->ln_FgPen, bgpen, JAM2);
	Move(rp, x1+2, y1 + td->font->tf_Baseline);
	Text(rp, line->ln_Text, line->ln_TextLen);
    } else
    {
	GetRPAttrs(rp,
		   RPTAG_APen, (ULONG) &apen,
		   TAG_DONE);
    }

    SetAPen(rp,above);
    Move(rp,x1    ,y1);
    Draw(rp,x1    ,y2);
    Move(rp,x1 + 1,y2);
    Draw(rp,x1 + 1,y1);
    Draw(rp,x2 - 1,y1);
    SetAPen(rp,below);
    Draw(rp,x2 - 1,y2);
    Move(rp,x2 - 2,y1 + 1);
    Draw(rp,x2 - 2,y2);
    Draw(rp,x1 + 1,y2);

    if (text)
    {
	installclipregion(rp->Layer,old_region);
	DisposeRegion(new_region);
	SetABPenDrMd(rp, apen, bpen, mode);
	SetFont(rp, oldfont);
    } else
    {
	SetAPen(rp, apen);
    }
}

void TriggerWord(struct Text_Data *td, UBYTE *word)
{
    if (*word != '\0')
    {
	struct dtTrigger trigger;

	trigger.MethodID = DTM_TRIGGER;
	trigger.dtt_GInfo = td->ginfo;
	trigger.dtt_Function = STM_COMMAND | STMD_STRPTR;
	trigger.dtt_Data = word;

	DoMethodA(td->obj, (Msg) &trigger);
    }
}

static void TriggerLink(struct Text_Data *td, struct Line *line)
{
    if (line->ln_Flags & LNF_LINK)
    {
	struct dtTrigger trigger;

	trigger.MethodID = DTM_TRIGGER;
	trigger.dtt_GInfo = td->ginfo;
	trigger.dtt_Function = STM_COMMAND | STMD_STRPTR;
	trigger.dtt_Data = line->ln_Data;

	DoMethodA(td->obj, (Msg) &trigger);
    }
}

BOOL CheckMarkedLink(struct Text_Data *td, LONG y)
{
    BOOL rc = FALSE;
    if(td->marked_line != NULL)
    {
	struct Line *marked = td->marked_line;
	struct Line *line;
	LONG my = marked->ln_YOffset / td->vert_unit;
	if(my < y)
	{
	    line = marked;
	    while(line)
	    {
		if ((line->ln_Flags & LNF_LINK) && (line->ln_YOffset / td->vert_unit) >= y)
		{
		    marked->ln_Flags &= ~LNF_MARKED;
                    td->last_marked_line = marked;
		    line->ln_Flags |= LNF_MARKED;
		    td->marked_line = line;
		    rc = TRUE;
		    break;
		}
		line = (struct Line *) Node_Next(line);
	    }
	} else if(my >= y + td->vert_visible - 1)
	{
	    line = marked;
	    while(line)
	    {
		if ((line->ln_Flags & LNF_LINK) && (line->ln_YOffset / td->vert_unit) <= y + td->vert_visible - 1)
		{
		    marked->ln_Flags &= ~LNF_MARKED;
                    td->last_marked_line = marked;
		    line->ln_Flags |= LNF_MARKED;
		    td->marked_line = line;
		    rc = TRUE;
		    break;
		}
		line = (struct Line *) Node_Prev(line);
	    }
	}
    }

    if (rc && td->ginfo != NULL)
    {
	struct RastPort *rp;

	if ((rp = ObtainGIRPort(td->ginfo)) != NULL)
	{
	    DrawLineBevel(td, rp, td->last_marked_line, TRUE);
	    DrawLineBevel(td, rp, td->marked_line, TRUE);
	    td->last_marked_line = NULL;
	    ReleaseGIRPort(rp);
	}
    }
    return rc;
}

int HandleMouseLink(struct Text_Data *td, struct RastPort *rp, LONG x, LONG y, LONG code)
{
    LONG xcur;

    if (code == SELECTDOWN)
    {
	struct Line *line = NewFindLine(td, x, y, &xcur, NULL);
	
	/* GEORGFIX: line can be NULL so added "line" to checks */
	if (line && line->ln_Flags & LNF_LINK)
	{
	    if (td->marked_line != NULL)
	    {
		td->marked_line->ln_Flags &= ~LNF_MARKED;
		DrawLineBevel(td, rp, td->marked_line, TRUE);
	    }
	    line->ln_Flags |= LNF_SELECTED | LNF_MARKED;
	    td->selected_line = line;
	    DrawLineBevel(td, rp, line, TRUE);
	    td->marked_line = line;
	    td->link_pressed = TRUE;
            return 1;
	}
    } else if(td->link_pressed)
    {
	if (code == SELECTUP)
	{
	    if (td->selected_line != NULL)
	    {
		td->selected_line->ln_Flags &= ~LNF_SELECTED;
		DrawLineBevel(td, rp, td->selected_line, TRUE);

		if(NewFindLine(td, x, y, &xcur, NULL) == td->selected_line)
		{
		    /* user released mouse button over a link.
                       trigger the link destination. */
		    TriggerLink(td, td->marked_line);
		}
		td->selected_line = NULL;
	    }
	    td->link_pressed = FALSE;
	} else if(td->selected_line != NULL)
	{
	    if(NewFindLine(td, x, y, &xcur, NULL) != td->selected_line)
	    {
		td->selected_line->ln_Flags &= ~LNF_SELECTED;
		DrawLineBevel(td, rp, td->selected_line, TRUE);
		td->selected_line = NULL;
	    }
	} else
	{
	    struct Line *line = NewFindLine(td, x, y, &xcur, NULL);
	    if (line->ln_Flags & LNF_LINK)
	    {
		if (td->marked_line != NULL && line != td->marked_line)
		{
		    td->marked_line->ln_Flags &= ~LNF_MARKED;
		    DrawLineBevel(td, rp, td->marked_line, TRUE);		
		}
		line->ln_Flags |= LNF_SELECTED | LNF_MARKED;
		td->selected_line = line;
		DrawLineBevel(td, rp, line, TRUE);
		td->marked_line = line;
	    }
	}
	return 1;
    }
    return 0;
}

void DT_AGTrigger(struct IClass *cl, Object *o, struct dtTrigger *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);
    ULONG function = ((struct dtTrigger*)msg)->dtt_Function;
    struct Line *select = td->marked_line;
    struct Line *line;
    LONG newtopy;
    LONG topy;
    LONG objtop = td->vert_top;

    if (select == NULL)
	return;

    GetDTAttrs(o, DTA_TopVert, (ULONG) &objtop, TAG_DONE);
    newtopy = objtop;

    D(bug("%ld\n",function));

    switch(function & STMF_METHOD_MASK)
    {
    case STM_PREV_FIELD:
	line = (struct Line *) Node_Prev(select);
	while(line)
	{
	    if (line->ln_Flags & LNF_LINK)
	    {
		select->ln_Flags &= ~LNF_MARKED;
		line->ln_Flags |= LNF_MARKED;
		select = line;
		break;
	    }
	    line = (struct Line *) Node_Prev(line);
	}
	if (line == NULL)
	{
	    line = (struct Line *) List_Last(&td->line_list);
	    while(line)
	    {
		if (line->ln_Flags & LNF_LINK)
		{
		    select->ln_Flags &= ~LNF_MARKED;
		    line->ln_Flags |= LNF_MARKED;
		    select = line;
		    break;
		}
		line = (struct Line *) Node_Prev(line);
	    }
	}
	topy = select->ln_YOffset / td->vert_unit;
	if (topy < objtop)
	{
	    newtopy = topy;
	} else if (topy >= objtop + td->vert_visible)
	{
	    newtopy = topy - td->vert_visible + 1;
	}
	break;
    case STM_NEXT_FIELD:
	line = (struct Line *) Node_Next(select);
	while(line)
	{
	    if (line->ln_Flags & LNF_LINK)
	    {
		select->ln_Flags &= ~LNF_MARKED;
		line->ln_Flags |= LNF_MARKED;
		select = line;
		break;
	    }
	    line = (struct Line *) Node_Next(line);
	}
	if (line == NULL)
	{
	    line = (struct Line *) List_First(&td->line_list);
	    while(line)
	    {
		if (line->ln_Flags & LNF_LINK)
		{
		    select->ln_Flags &= ~LNF_MARKED;
		    line->ln_Flags |= LNF_MARKED;
		    select = line;
		    break;
		}
		line = (struct Line *) Node_Next(line);
	    }
	}
	topy = select->ln_YOffset / td->vert_unit;
	if (topy >= objtop + td->vert_visible)
	{
	    newtopy = topy - td->vert_visible + 1;
	} else if(topy < objtop)
        {
	    newtopy = topy;
	}
	break;
    case STM_ACTIVATE_FIELD:
	if (td->marked_line != NULL)
	{
	    TriggerLink(td, td->marked_line);
	}
	break;
    }

    if (td->marked_line != select)
    {
	struct RastPort *rp = NULL;
	td->last_marked_line = td->marked_line;
	td->marked_line = select;

	D(bug("newtop %ld, objtop %ld\n", newtopy, objtop));
	if (newtopy == objtop)
	{
	   if ((rp = ObtainGIRPort(td->ginfo)) != NULL)
	   {
		DrawLineBevel(td, rp, td->last_marked_line, TRUE);
		DrawLineBevel(td, rp, td->marked_line, TRUE);
		td->last_marked_line = NULL;
		ReleaseGIRPort(rp);
	   }
	}

	/* if new selected line couldn't be draw update whole object. */	
	if (rp == NULL || newtopy != objtop)
	{
	    notifyAttrChanges(o, msg->dtt_GInfo, 0,
				GA_ID, ((struct Gadget *) o)->GadgetID,
				DTA_TopVert, newtopy,
				DTA_Sync, TRUE,
				TAG_DONE);
	}
    }
}

#endif
