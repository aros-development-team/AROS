/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <string.h>
#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <gadgets/aroslist.h>

#include "aroslistview_intern.h"

//#define TURN_OFF_DEBUG

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

STATIC VOID HandleSpecialMinWidth( struct ColumnAttrs           *colattrs,
				   struct LVData		*data
);

#define ForeachViewedCol(iterator, numcols) \
	for (iterator = 0; iterator < numcols; iterator ++)


/**************************
**  ParseFormatString()  **
**************************/
BOOL ParseFormatString( STRPTR                  formatstr,
			struct LVData		*data
)
{
    #undef NUMPARAMS
    #define NUMPARAMS 5
    UBYTE templ[] = "COL=C/N,BAR=B/S,PREPARSE=P/K,DELTA=D/N,MINWIDTH=MIW/N";

    struct RDArgs *rdarg;
    BOOL success = FALSE;
    struct ColumnAttrs *colattrs;

    /* Give ReadArgs() som stack space to chew on so that time is not
     * wasted on memory allocations
     */
    #undef WORKBUFSIZE
    #define WORKBUFSIZE 500
    UBYTE workbuf[WORKBUFSIZE];

    colattrs = data->lvd_ColAttrs;

    rdarg = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL);
    if (rdarg)
    {
	UWORD curcol = 0;
	BOOL morecolumns = TRUE;


	/* Reset the as-big-as-biggest-text property */
	data->lvd_Flags &= ~LVFLG_SPECIALCOLWIDTH;

	do
	{
	    UWORD len  = 0;

	    STRPTR str = formatstr;
	    IPTR argsarray[NUMPARAMS];
	    memset(argsarray, 0, UB(&argsarray[NUMPARAMS]) - UB(&argsarray[0]));

	    /* An example of format string is ",PREPARSE=c9, ," */
	    while (*str && *str != ',')
	    {
		len ++;
		str ++;
	    }
	    if (!*str)
		morecolumns = FALSE;

	    /* Skip comma */
	    str ++;

	    rdarg->RDA_Source.CS_Buffer = formatstr;
	    rdarg->RDA_Source.CS_Length = len;
	    rdarg->RDA_Source.CS_CurChr = 0;
	    rdarg->RDA_Buffer = workbuf;
	    rdarg->RDA_BufSiz = WORKBUFSIZE;

	    if (!ReadArgs(templ, argsarray, rdarg))
	    {
		FreeArgs(rdarg);
		success = FALSE;
		break;
	    }
	    else
	    {

		/* Insert parsed info into colattr array */
		/* A COL argument specified ? */
		colattrs[curcol].ca_DHIndex = (UBYTE)((argsarray[0]) ? *((ULONG *)argsarray[0]) : curcol);
		if (argsarray[1]) /* BAR */
		    colattrs[curcol].ca_Flags |= CAFLG_BAR;
		if (argsarray[2]) /* PREPARSE */
		{
		    /* Parse the preparse string */
		    str = (STRPTR)argsarray[2];

		    while (*str)
		    {
			if (*str == 'c') /* Centre align text */
			{
			    colattrs[curcol].ca_Flags &= ~CA_ALIGN_MASK;
			    colattrs[curcol].ca_Flags |= CA_ALIGN_CENTRE;
			}
			else if (*str == 'l') /* Left align text */
			{
			    colattrs[curcol].ca_Flags &= ~CA_ALIGN_MASK;
			    colattrs[curcol].ca_Flags |= CA_ALIGN_LEFT;
			}
			else if (*str == 'r') /* Right align text */
			{
			    colattrs[curcol].ca_Flags &= ~CA_ALIGN_MASK;
			    colattrs[curcol].ca_Flags |= CA_ALIGN_RIGHT;
			}
			else if (*str >= '0' && *str <= 9) /* Text background pen */
			{
			    colattrs[curcol].ca_Pen = *str - '0';;
			}
			str ++;

		    }
		}

		if (argsarray[3]) /* DELTA */
		    colattrs[curcol].ca_Delta = (UWORD)((ULONG *)argsarray)[3];

		if (argsarray[4]) /* MINWIDTH */
		{
		    colattrs[curcol].ca_MinWidth = (UWORD)((ULONG *)argsarray)[4];
		    if (colattrs[curcol].ca_MinWidth == -1)
		    {
			data->lvd_Flags |= LVFLG_SPECIALCOLWIDTH;
			colattrs[curcol].ca_Flags |= CAFLG_SPECIALCOLWIDTH;
		    }
		}

		curcol ++;
		formatstr += len + 1;
	    }
	}
	while (morecolumns);

	data->lvd_ViewedColumns = curcol;

	FreeDosObject(DOS_RDARGS, rdarg);
    }

    return (success);
}

/********************
**  GetGadgetIBox  **
********************/

/* Figure out the size of the gadget rectangle, taking relative
 * positioning into account.
 */
VOID GetGadgetIBox(Object *o, struct GadgetInfo *gi, struct IBox *ibox)
{
    ibox->Left	 = EG(o)->LeftEdge;
    ibox->Top	 = EG(o)->TopEdge;
    ibox->Width  = EG(o)->Width;
    ibox->Height = EG(o)->Height;

    if (gi)
    {
	if (EG(o)->Flags & GFLG_RELRIGHT)
	    ibox->Left	 += gi->gi_Domain.Width - 1;

	if (EG(o)->Flags & GFLG_RELBOTTOM)
	    ibox->Top	 += gi->gi_Domain.Height - 1;

	if (EG(o)->Flags & GFLG_RELWIDTH)
	    ibox->Width  += gi->gi_Domain.Width;

	if (EG(o)->Flags & GFLG_RELHEIGHT)
	    ibox->Height += gi->gi_Domain.Height;
    }
}

/**********************
**  RenderEntries()  **
**********************/
VOID RenderEntries( Class                   *cl,
		    Object		    *o,
		    struct gpRender	    *msg,
		    LONG		    startpos,
		    UWORD		    num,
		    BOOL		    erase
)
{
    struct AROSP_List_GetEntry getentry_msg;
    APTR item;
    struct IBox container;
    struct TextFont *oldfont;
    WORD top;
    LONG activepos;
    LONG pos;

    struct LVData *data = INST_DATA(cl, o);
    UWORD *pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;

    GetGadgetIBox(o, msg->gpr_GInfo, &container);

    top = container.Top + LV_BORDERWIDTH_Y
		+ (startpos - data->lvd_First) * data->lvd_EntryHeight;

    SetAPen(msg->gpr_RPort, pens[data->lvd_FrontPen]);
    SetDrMd(msg->gpr_RPort, JAM1);

    oldfont = msg->gpr_RPort->Font;
    SetFont(msg->gpr_RPort, data->lvd_Font);

    if (!(data->lvd_Flags & (LVFLG_READONLY|LVFLG_MULTISELECT)))
	GetAttr(AROSA_List_Active, data->lvd_List, &activepos);

    getentry_msg.MethodID  = AROSM_List_GetEntry;
    getentry_msg.ItemPtr   = &item;

     /* Start rendering the listview entries */
    for (pos = startpos; num --; pos ++)
    {

	register UWORD col;
	struct TextExtent te;
	struct ColumnAttrs *colattrs = data->lvd_ColAttrs;

	BOOL erase_this_entry = erase;
	UWORD erasepen = data->lvd_BackPen;

	if (!(data->lvd_Flags & LVFLG_READONLY))
	{
	    if (data->lvd_Flags & LVFLG_MULTISELECT)
	    {
		LONG state;
		DoMethod(data->lvd_List,
			AROSM_List_Select,
			pos,
			AROSV_List_Select_Ask,
			&state);
		if (state)
		{
		    erase_this_entry = TRUE;
		    erasepen = FILLPEN;
		}
	    }
	    else
	    {
		if (pos == activepos)
		{
		    erase_this_entry = TRUE;
		    erasepen = FILLPEN;
		}
	    }
	}

	if (erase_this_entry)
	{
	     /* Erase the old text line */
	    SetAPen(msg->gpr_RPort, pens[erasepen]);

	    RectFill(msg->gpr_RPort,
		container.Left + LV_BORDERWIDTH_X,
		top,
		container.Left + container.Width - LV_BORDERWIDTH_X - 1,
		top + data->lvd_EntryHeight - 1);

	    SetAPen(msg->gpr_RPort, pens[data->lvd_FrontPen]);

	}


	getentry_msg.Position = pos;
	DoMethodA(data->lvd_List, (Msg)&getentry_msg);
	if (!item)
	    break;

	CallHookPkt( data->lvd_DisplayHook,
		     data->lvd_DHArray,
		     item);

	ForeachViewedCol(col, data->lvd_ViewedColumns)
	{
	    UWORD idx, len;
	    WORD left;

	    /* Get the index into DisplayHookArray for getting text for this column */
	    idx = colattrs[col].ca_DHIndex;
D(bug("Render: idx=%d, col=%d\n", idx, col));
	    /* How many characters of the string
	     * returned by DispHook are we able to view ?
	     */
	    len = TextFit(msg->gpr_RPort,
			data->lvd_DHArray[idx],
			strlen(data->lvd_DHArray[idx]),
			&te,
			NULL,
			1,
			colattrs[col].ca_Width,
			10000); /* We allready know that the height fit */
D(bug("Textfit len: %d\n", len));
	    /* Where do we place the len characters ? */
	    switch (colattrs[col].ca_Flags & CA_ALIGN_MASK)
	    {
	    case CA_ALIGN_LEFT:
		left = colattrs[col].ca_Left;
		break;

	    case CA_ALIGN_RIGHT:
		left = colattrs[col].ca_Right - te.te_Width;
		break;

	    default:
		left = colattrs[col].ca_Left + ((colattrs[col].ca_Width - te.te_Width) >> 1);
		break;
	    }

D(bug("Render: left=%d,idx=%d,text=%s\n", left, idx, data->lvd_DHArray[idx]));

	    Move(msg->gpr_RPort, left, top + data->lvd_Font->tf_Baseline);
	    Text(msg->gpr_RPort, data->lvd_DHArray[idx], len);

	} /* ForeachViewedCol */

	top += data->lvd_EntryHeight;

    } /* for (entries to view) */
    SetFont(msg->gpr_RPort, oldfont);

    return;
}


/******************************
**  HandleSpecialMinWidth()  **
******************************/

STATIC VOID HandleSpecialMinWidth( struct ColumnAttrs           *colattrs,
				   struct LVData		*data
)
{
    register UWORD i;
    register LONG pos;
    APTR item;
    struct RastPort rp;

    InitRastPort(&rp);

    SetFont(&rp, data->lvd_Font);


    /* Initialize the minwidths to 0 */
    ForeachViewedCol(i, data->lvd_ViewedColumns)
	if (colattrs[i].ca_Flags & CAFLG_SPECIALCOLWIDTH)
	    colattrs[i].ca_MinWidth = 0;

    for (pos = 0; ; pos ++)
    {
	DoMethod(data->lvd_List, AROSM_List_GetEntry, pos, &item);
	if (!item)
	    break;

	/* Get the texts for a row in the list */
	CallHookPkt(data->lvd_DisplayHook,
		    data->lvd_DHArray,
		    item);

	ForeachViewedCol(i, data->lvd_ViewedColumns)
	{
	    UWORD idx = colattrs[i].ca_DHIndex;

	    if (colattrs[i].ca_Flags & CAFLG_SPECIALCOLWIDTH)
	    {
		UWORD length;

		length = TextLength(&rp,
				    data->lvd_DHArray[idx],
				    strlen(data->lvd_DHArray[idx]));

		if (length > colattrs[i].ca_MinWidth)
		     colattrs[i].ca_MinWidth = length;

	     } /* if (column should be viewed and width is as long as biggest textlength) */

	} /* ForeachViewedCol() */

    } /* for (iterate through list of items) */

    return;
}

/************************
** ComputeColumnWidts  **
************************/

VOID ComputeColumnWidths(UWORD                      listwidth,
			struct LVData		    *data
)
{

    /* First handle columns that want their minwidth to be the
     * size of the largest entry in that column
     */

    struct ColumnAttrs *colattrs = data->lvd_ColAttrs;
    UWORD sum_minwidths = 0;
    UWORD remainder;
    WORD on_each_col, pixels2divide;
    register UWORD i;

    /* Find number of pixels to divide between columns */
    listwidth -=  LV_BORDERWIDTH_X * 2
		+ data->lvd_HorSpacing * (data->lvd_ViewedColumns + 1);

    if (data->lvd_Flags & LVFLG_SPECIALCOLWIDTH)
	HandleSpecialMinWidth(colattrs, data);

    /* Compute the sum of the minwidths and the number of columns to be  */
    ForeachViewedCol(i, data->lvd_ViewedColumns)
    {
	sum_minwidths += colattrs[i].ca_MinWidth;
    }


    /* Now we have three cases, 1) sum_minwidths > listwidth or
     * 2) sum_minwidths < listwidth or 3) sum_minwidths == listwidth.
     *
     * 1): Divide listwidth - sum_minwidths pixels among the columns
     *
     * 2): Take sum_minwidths - listwidth pixels away from the columns
     *
     * 3): Just use each column's minwidth as width for the column
     */

    pixels2divide = listwidth - sum_minwidths;

    /* Divide pixels equally among the columns */

    on_each_col = pixels2divide / data->lvd_ViewedColumns;
    remainder = pixels2divide % data->lvd_ViewedColumns;

    if (pixels2divide > 0)
    {
	ForeachViewedCol(i, data->lvd_ViewedColumns)
	{
	    colattrs[i].ca_Width = colattrs[i].ca_MinWidth;
	    colattrs[i].ca_Width += on_each_col;

	    if (remainder)
	    {
		colattrs[i].ca_Width ++;
		remainder --;
	    }
	}
    }
    else if (pixels2divide < 0)
    {
	ForeachViewedCol(i, data->lvd_ViewedColumns)
	{
	    colattrs[i].ca_Width = colattrs[i].ca_MinWidth;
	    colattrs[i].ca_Width += on_each_col;

	    if (remainder)
	    {
		colattrs[i].ca_Width --;
		remainder --;
	    }
	}
    }
    else /* pixels2divide == 0 */
    {
	ForeachViewedCol(i, data->lvd_ViewedColumns)
	{
	    colattrs[i].ca_Width = colattrs[i].ca_MinWidth;
	}
    }

    return;
}

/**************************
**  ComputeColLeftRight  **
**************************/

VOID ComputeColLeftRight(UWORD gadleft, struct LVData *data)
{
    struct ColumnAttrs *colattrs;
    register UWORD i;
    register UWORD left;

    left = gadleft + LV_BORDERWIDTH_X;
    colattrs = data->lvd_ColAttrs;

    ForeachViewedCol(i, data->lvd_ViewedColumns)
    {
	left += data->lvd_HorSpacing;

	colattrs[i].ca_Left  = left;
	colattrs[i].ca_Right = (left += colattrs[i].ca_Width);
    }
    return;
}


/*********************
**  DrawListBorder  **
*********************/

VOID DrawListBorder( struct RastPort                *rp,
			    UWORD		    *pens,
			    struct IBox 	    *bbox,
			    BOOL		    recessed
)
{
    SetAPen (rp, pens[(recessed) ? SHINEPEN : SHADOWPEN]);

    /* right */
    RectFill (rp
	    , bbox->Left + bbox->Width	- LV_BORDERWIDTH_X
	    , bbox->Top
	    , bbox->Left + bbox->Width	- 1
	    , bbox->Top  + bbox->Height - 1
	);

		/* bottom */
    RectFill (rp
	    , bbox->Left
	    , bbox->Top  + bbox->Height - LV_BORDERWIDTH_Y
	    , bbox->Left + bbox->Width	- LV_BORDERWIDTH_X
	    , bbox->Top  + bbox->Height - 1
	);

    SetAPen (rp, pens[(recessed) ? SHADOWPEN : SHINEPEN]);

    /* top */
    RectFill (rp
	    , bbox->Left
	    , bbox->Top
	    , bbox->Left + bbox->Width - LV_BORDERWIDTH_X
	    , bbox->Top + LV_BORDERWIDTH_Y - 1
	);

    /* left */
    RectFill (rp
	    , bbox->Left
	    , bbox->Top
	    , bbox->Left + LV_BORDERWIDTH_X - 1
	    , bbox->Top + bbox->Height - LV_BORDERWIDTH_Y
	);

    WritePixel (rp, bbox->Left + bbox->Width - 1, bbox->Top);
    WritePixel (rp, bbox->Left, bbox->Top + bbox->Height - 1);

    return;
}


/*******************
**  ShownEntries  **
*******************/
UWORD ShownEntries(struct LVData        *data,
		   struct IBox		*container
)

{
    ULONG numentries;
    UWORD shown;

    GetAttr(AROSA_List_Entries, data->lvd_List, &numentries);

    /* This formula has a little "bug": The height of the rendered texts
    ** are ibox.Height - height of 2 borders  - 1 horizontal spacing line, but
    ** since hor sp. always is < entryheight, the formula provides the right result
    */

    shown = (container->Height - LV_BORDERWIDTH_Y * 2) / data->lvd_EntryHeight;

    shown = MIN(shown, numentries - data->lvd_First);

    return (shown);
}

/******************
**  NotifyAttrs  **
******************/

VOID NotifyAttrs(Class *cl, Object *o, struct opSet *msg, struct TagItem *tags)
{
    struct TagItem idtags[] =
    {
	{GA_ID, 		(IPTR)EG(o)->GadgetID},
	{TAG_MORE,		(IPTR)tags}
    };

    struct opUpdate nmsg =  {OM_NOTIFY, idtags, msg->ops_GInfo, 0};
    struct LVData *data = INST_DATA(cl, o);

    data->lvd_NotifyCount ++;
    DoSuperMethodA(cl, o,  (Msg)&nmsg);
    data->lvd_NotifyCount --;
    return;
}

