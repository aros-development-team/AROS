/*
    (C) 1995-97 AROS - The Amiga Replacement OS
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

#ifndef TURN_OFF_DEBUG
#define DEBUG 1
#endif

#include <aros/debug.h>

STATIC VOID ComputeColLeftRight(UWORD gadleft, struct LVData *data);
STATIC VOID HandleSpecialMinWidth( struct ColumnAttrs		*colattrs,
				   struct RastPort		*rp,
				   struct LVData		*data,
				   struct LVBase_intern		*AROSListviewBase);
STATIC VOID ComputeColumnWidths(UWORD 			    listwidth,
				struct RastPort 	    *rp,
				struct LVData 		    *data,
				struct LVBase_intern  *AROSListviewBase);



#define ForeachViewedCol(iterator, numcols) \
	for (iterator = 0; iterator < numcols; iterator ++)


/**************************
**  ParseFormatString()  **
**************************/
BOOL ParseFormatString(	STRPTR			formatstr,
			struct LVData		*data,
			struct LVBase_intern	*AROSListviewBase)
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
            
            /* An example of format string is ",PREPARSE=c9", ," */
            while (*str && *str != ',')
            {
            	len ++;
            	str ++;
            }
            
            if (!*str)
            	morecolumns = FALSE;
            
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
    	    	    	    colattrs[curcol].ca_Pen = *str - '0';
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
		formatstr += len;
	    }
    	}
    	while (morecolumns);
    	
    	data->lvd_ViewedColumns = curcol;
    	
    	FreeDosObject(DOS_RDARGS, rdarg);
    }
    
    return (success);
}

/********************
**  GetGdagetIBox  **
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

/*****************
**  RenderLine  **
*****************/

struct Pos
{
    WORD Left;
    WORD Top;
};

VOID RenderLine(struct Pos 		    *pos,
		struct AROSP_List_GetEntry  *getentry_msg,
		struct RastPort 	    *rp,
		struct LVData 		    *data,
		struct LVBase_intern 	    *AROSListviewBase)
{
    register UWORD col;
    struct TextExtent te;
    struct ColumnAttrs *colattrs = data->lvd_ColAttrs;

    DoMethodA(data->lvd_List, (Msg)getentry_msg);
    if (!*(getentry_msg->ItemPtr))
       return;;
     	
    CallHookPkt( data->lvd_DisplayHook,
	         data->lvd_DHArray,
		 *(getentry_msg->ItemPtr));
    	    	
    ForeachViewedCol(col, data->lvd_ViewedColumns)
    {
        UWORD idx, len;
	        	    
        /* Get the index into DisplayHookArray for getting text for this column */
        idx = colattrs[col].ca_DHIndex;
/*      D(bug("idx=%d, col = %d\n", idx, col));
 */   	    
        /* How many characters of the string 
	 * returned by DispHook are we able to view ?
	 */

    	len = TextFit(	rp,
    		       	data->lvd_DHArray[idx],
	    	 	strlen(data->lvd_DHArray[idx]),
    	    		&te,
    	    		NULL,
    	    		1,
    	    		colattrs[col].ca_Width,
    	    		10000); /* We allready know that the height fit */
    	    		   
    	/* Where do we place the len characters ? */
    	switch (colattrs[col].ca_Flags & CA_ALIGN_MASK)
    	{
    	    case CA_ALIGN_LEFT:
    	        pos->Left = colattrs[col].ca_Left;
    	        break;
    	    	    
    	    case CA_ALIGN_RIGHT:
    	        pos->Left = colattrs[col].ca_Right - te.te_Width;
    	        break;
    	    	    
    	    case CA_ALIGN_CENTRE:
    	        pos->Left = colattrs[col].ca_Left + ((colattrs[col].ca_Width - te.te_Width) >> 1);
    	        break;
    	}
    	    
/*    	D(bug("Rendering entry at (%d,%d)\n", left, top));
*/	Move(rp, pos->Left, pos->Top);
    	Text(rp, data->lvd_DHArray[idx], len);
    }
    
    return;
}

/**********************
**  RenderEntries()  **
**********************/
VOID RenderEntries( Object 		    *o,
		    struct gpRender	    *msg,
		    UWORD		    entryheight,
		    UWORD		    numvisible,
		    struct IBox		    *container,
		    struct LVBase_intern    *AROSListviewBase)
{
    UWORD width;
    struct AROSP_List_GetEntry getentry_msg;
    APTR item;
    struct LVData *data;
    
    struct Pos pos;

    UWORD *pens;
        
    data = INST_DATA(OCLASS(o), o);

    pens = msg->gpr_GInfo->gi_DrInfo->dri_Pens;
    
    width  = container->Width - 2 * LV_BORDERWIDTH_X;
    pos.Top	   = container->Top   + LV_BORDERWIDTH_Y 
    			     - ( msg->gpr_RPort->Font->tf_YSize 
    			       - msg->gpr_RPort->Font->tf_Baseline);
    pos.Left   = container->Left  + LV_BORDERWIDTH_X;

    /* Compute widths of each column */
    ComputeColumnWidths(width - (data->lvd_ViewedColumns + 1) * data->lvd_HorSpacing,
    			msg->gpr_RPort, 
    			data,
    			LVB(AROSListviewBase));
    
    /* Compute left and right offsets for each column */
    ComputeColLeftRight(pos.Left, data);

        
    getentry_msg.MethodID  = AROSM_List_GetEntry;
    getentry_msg.ItemPtr   = &item;
         	    
     /* Start rendering the listview entries */
    SetAPen(msg->gpr_RPort, pens[TEXTPEN]);
    for (getentry_msg.Position = data->lvd_First; numvisible --; getentry_msg.Position ++)
    {
	pos.Top += entryheight;

    	RenderLine( &pos,
    		    &getentry_msg,
    		    msg->gpr_RPort,
    		    data,
    		    LVB(AROSListviewBase));

    }	    

    return;
}

/******************************
**  HandleSpecialMinWidth()  **
******************************/

STATIC VOID HandleSpecialMinWidth( struct ColumnAttrs		*colattrs,
				   struct RastPort		*rp,
				   struct LVData		*data,
				   struct LVBase_intern	*AROSListviewBase)
{
    register UWORD i;
    register LONG pos;
    APTR item;
  
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
    	    if (colattrs[i].ca_Flags & CAFLG_SPECIALCOLWIDTH)
    	    {
    		UWORD length;

    		length = TextLength(rp,
    				    data->lvd_DHArray[i],
    		    		    strlen(data->lvd_DHArray[i]));
    		    
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

STATIC VOID ComputeColumnWidths(UWORD 			    listwidth,
				struct RastPort 	    *rp,
				struct LVData 		    *data,
				struct LVBase_intern	    *AROSListviewBase)
{

    /* First handle columns that want their minwidth to be the 
     * size of the largest entry in that column
     */   
     
    struct ColumnAttrs *colattrs = data->lvd_ColAttrs;
    UWORD sum_minwidths = 0; 
    UWORD remainder;
    WORD on_each_col, pixels2divide;
    register UWORD i;
     
    if (data->lvd_Flags & LVFLG_SPECIALCOLWIDTH)
	HandleSpecialMinWidth(colattrs, rp, data, LVB(AROSListviewBase));
    
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

STATIC VOID ComputeColLeftRight(UWORD gadleft, struct LVData *data)
{
    struct ColumnAttrs *colattrs;
    register UWORD i;
    register UWORD left;
    
    left = gadleft;
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

VOID DrawListBorder( struct RastPort	    	    *rp,
			    UWORD		    *pens,
			    struct IBox		    *bbox,
			    BOOL		    recessed,
			    struct LVBase_intern    *AROSListviewBase)
{
    SetAPen (rp, pens[(recessed) ? SHINEPEN : SHADOWPEN]);
    
    /* right */
    RectFill (rp
	    , bbox->Left + bbox->Width	- LV_BORDERWIDTH_X - 1
	    , bbox->Top
	    , bbox->Left + bbox->Width	- 1
	    , bbox->Top	 + bbox->Height - 1
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
	    , bbox->Top	+ LV_BORDERWIDTH_Y - 1
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

/********************
**  DoResizeStuff  **
********************/

VOID DoResizeStuff( Object 		    *o,
		    struct gpRender	    *msg,
		    UWORD 		    *entryheightptr,
		    UWORD 		    *numvisibleptr,
		    struct IBox		    *container)
{
    UWORD height;
    UWORD vertspacing = LVD(INST_DATA(OCLASS(o), o))->lvd_VertSpacing;
    
    /* We have to set our sizes according to the font */
    *entryheightptr = msg->gpr_RPort->Font->tf_YSize + vertspacing;
    	    
    /* Find the height we have to divide among the entrys */
    height =   container->Height - vertspacing - (LV_BORDERWIDTH_Y * 2);
    	    
    /* We should only have wholly visible entries */
    *numvisibleptr = height / *entryheightptr;
    EG(o)->Height -= height % *entryheightptr; /* Remove superfluous height */
    
    /* Recalulate container width */
    container->Height = EG(o)->Height + 
    			((EG(o)->Flags & GFLG_RELHEIGHT) ? 
    				msg->gpr_GInfo->gi_Domain.Height : 0);
    
    return;

}

/*********************
**  UpdatePGATotal  **
*********************/

VOID UpdatePGATotal(struct LVData	    *data,
		    struct GadgetInfo	    *ginfo,
		    struct LVBase_intern    *AROSListviewBase)
{
    /* Updates the propgadget's PGA_Total to the current
     * numbers of entries in the list. 
     * (Can't use notification for this since List is subclass
     * of rootclass.
     */
     
    struct TagItem tags[2];
   
    tags[0].ti_Tag = PGA_Total;
    GetAttr(AROSA_List_Entries, data->lvd_List, &(tags[0].ti_Data));
    tags[1].ti_Tag  = TAG_END;
    
    DoMethod(data->lvd_Prop, OM_UPDATE, tags, ginfo, 0);
    
    return;
}
		    
