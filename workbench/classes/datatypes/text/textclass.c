/**********************************************************************
 text.datatype - (c) 2000 by Sebastian Bauer

 This text.datatype engine
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

#include <libraries/reqtools.h>
#include <proto/reqtools.h>

#ifdef COMPILE_DATATYPE
#include <proto/datatypes.h>
#endif

#include "compilerspecific.h"
#include "support.h"
#include "textclass.h"

/* Define the following to enable the debug version */
/* #define MYDEBUG */
#include "debug.h"

#ifdef _AROS
#define NO_PRINTER 1
#else
#define NO_PRINTER 0
#endif

/* Some prototypes */
static void CopyText(struct Text_Data *td);
static void ClearSelected(struct Text_Data *td, struct RastPort *rp);

/**************************************************************************
 Returns the xoffset of the given line node (relative to the previous one)
**************************************************************************/
static int GetRelativeOffset(struct Line *line, LONG fontx)
{
    struct Line *prev_line = (struct Line *) Node_Prev(line);

    if (prev_line)
    {
	if (!(prev_line->ln_Flags & LNF_LF))
	{
	    return (int) (line->ln_XOffset - prev_line->ln_XOffset - fontx * prev_line->ln_TextLen);
	}
    }

    return line->ln_XOffset;
}

/**************************************************************************
 Fill in sorted marking information
**************************************************************************/
static void PrepareMark(struct Text_Data *td, LONG * mark_x1, LONG * mark_y1, LONG * mark_x2, LONG * mark_y2, struct Line **mark_line1, struct Line **mark_line2)
{
    /* Fill in the mark stuff in the correct order */

    if (td->mark_y1 != -1)
    {
	if (td->mark_y1 < td->mark_y2)
	{
	    *mark_x1 = td->mark_x1;
	    *mark_y1 = td->mark_y1;
	    *mark_line1 = td->mark_line1;
	    *mark_x2 = td->mark_x2;
	    *mark_y2 = td->mark_y2;
	    *mark_line2 = td->mark_line2;
	}
	else
	{
	    if (td->mark_y1 > td->mark_y2)
	    {
		*mark_x1 = td->mark_x2;
		*mark_y1 = td->mark_y2;
		*mark_line1 = td->mark_line2;
		*mark_x2 = td->mark_x1;
		*mark_y2 = td->mark_y1;
		*mark_line2 = td->mark_line1;
	    }
	    else
	    {
		*mark_y1 = *mark_y2 = td->mark_y1;
		if (td->mark_x1 < td->mark_x2)
		{
		    *mark_x1 = td->mark_x1;
		    *mark_x2 = td->mark_x2;
		    *mark_line1 = td->mark_line1;
		    *mark_line2 = td->mark_line2;
		}
		else
		{
		    *mark_x1 = td->mark_x2;
		    *mark_x2 = td->mark_x1;
		    *mark_line1 = td->mark_line2;
		    *mark_line2 = td->mark_line1;
		}
	    }
	}
    }
    else
    {
	*mark_line1 = *mark_line2 = NULL;
	*mark_y1 = *mark_y2 = *mark_x1 = *mark_x2 = -1;
    }

}

/**************************************************************************
 Initialize the text object. E.g. open the font defined td->attr
**************************************************************************/
static int InitText(struct Text_Data *td)
{
    struct TextFont *font = GfxBase->DefaultFont;

    NewList(&td->line_list);

    if (!td->attr.ta_Name)
    {
	if ((td->attr.ta_Name = StrCopy(font->tf_Message.mn_Node.ln_Name)))
	{
	    td->attr.ta_YSize = font->tf_YSize;
	    td->attr.ta_Style = td->attr.ta_Flags = 0;
	}
    }

    if (td->attr.ta_Name)
    {
	td->mark_x1 = -1;
	td->mark_x2 = -1;
	td->mark_y1 = -1;
	td->mark_y2 = -1;

	if ((td->font = OpenFont(&td->attr)))
	    return 1;

	FreeVec(td->attr.ta_Name);

	td->attr.ta_Name = StrCopy("topaz.font");
	td->attr.ta_YSize = 8;
	td->attr.ta_Style = td->attr.ta_Flags = 0;

	if ((td->font = OpenFont(&td->attr)))
	    return 1;

	FreeVec(td->attr.ta_Name);
    }

    return 0;
}

/**************************************************************************
 ...
**************************************************************************/
static int LoadText(struct Text_Data *td, STRPTR filename, BPTR file)
{
    if ((td->title = StrCopy(filename)))
    {
	D(bug("text.datatype/LoadText: Get the file size\n"));
	if ((td->buffer_allocated_len = GetFileSize(file)) >= 0)
	{
	    D(bug("text.datatype/LoadText: Allocated\n"));
	    if ((td->buffer_allocated = AllocVec(td->buffer_allocated_len + 1, MEMF_PUBLIC)))
	    {
		D(bug("text.datatype/LoadText: Read\n"));
		if ((Read(file, td->buffer_allocated, td->buffer_allocated_len) == td->buffer_allocated_len))
		{
		    td->buffer_allocated[td->buffer_allocated_len] = 10;
		    return 1;
		}
		FreeVec(td->buffer_allocated);
		td->buffer_allocated = NULL;
	    }
	}
	FreeVec(td->title);
	td->title = NULL;
    }

    return NULL;
}

/**************************************************************************
 ...
**************************************************************************/
static int LoadTextAsIFF(struct Text_Data *td, STRPTR filename, struct IFFHandle *iff)
{
    StopChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('C','H','R','S'));

    td->buffer_allocated_len = 0;
    td->buffer_allocated = 0;

    while (1)
    {
	struct ContextNode *cn;
	LONG error = ParseIFF(iff, IFFPARSE_SCAN);

	if (!(!error || error == IFFERR_EOC))
	    break;

	cn = CurrentChunk(iff);
	if (!cn)
	    continue;

	if (cn->cn_ID == MAKE_ID('C','H','R','S'))
	{
	    LONG pre_size = td->buffer_allocated_len;
	    LONG add_size = cn->cn_Size;

	    if (td->buffer_allocated)
	    {
		STRPTR newbuf = (STRPTR) AllocVec(pre_size + add_size, 0);
		if (newbuf)
		{
		    CopyMem(td->buffer_allocated, newbuf, pre_size);
		    FreeVec(td->buffer_allocated);
		    td->buffer_allocated = newbuf;
		}
		else
		{
		    FreeVec(td->buffer_allocated);
		    td->buffer_allocated = NULL;
		}
	    }
	    else
		td->buffer_allocated = (STRPTR) AllocVec(add_size, 0);

	    if (td->buffer_allocated)
	    {
		td->buffer_allocated_len = pre_size + add_size;
		ReadChunkBytes(iff, &td->buffer_allocated[pre_size], add_size);
	    }
	}
    }

    if (!td->buffer_allocated)
	return 0;

    td->title = StrCopy(filename);

    return 1;
}

/**************************************************************************
 ...
**************************************************************************/
static void DisposeText(struct Text_Data *td)
{
#ifndef COMPILE_DATATYPE
    if (td->line_pool)
	DeletePool(td->line_pool);
#endif
    if (td->buffer_allocated)
	FreeVec(td->buffer_allocated);
    if (td->font)
	CloseFont(td->font);
    if (td->attr.ta_Name)
	FreeVec(td->attr.ta_Name);
    if (td->title)
	FreeVec(td->title);
}


/**************************************************************************
 ...
**************************************************************************/
static void DrawText(struct Text_Data *td, struct RastPort *rp)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    LONG fontx = td->font->tf_XSize;
    LONG fonty = td->font->tf_YSize;
    LONG baseline = td->font->tf_Baseline;

    LONG linenum = 0;
    LONG minlinenum = td->vert_top;
    LONG maxlinenum = td->vert_top + td->vert_visible;
    LONG mincolnum = td->horiz_top;
    LONG collen = td->horiz_visible;

    LONG y = 0;
    ULONG apen, bpen, mode;
    struct TextFont *oldfont;
    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;

    LONG newcollen = td->width / td->font->tf_XSize;

    if (newcollen < collen)
	collen = newcollen;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

    D(bug("text.datatype/DrawText: DrawText()\n"));

    GetRPAttrs(rp,
	       RPTAG_APen, &apen,
	       RPTAG_BPen, &bpen,
	       RPTAG_DrMd, &mode,
	       RPTAG_Font, &oldfont,
	       TAG_DONE);

    SetFont(rp, td->font);

    if (td->use_vert_diff)
    {
	if (td->vert_diff > 0)
	{
	    minlinenum = maxlinenum - td->vert_diff;
	}
	else
	    maxlinenum = minlinenum - td->vert_diff;

	td->use_vert_diff = FALSE;
    }

    if (td->use_horiz_diff)
    {
	if (td->horiz_diff > 0)
	{
	    mincolnum = mincolnum + collen - td->horiz_diff;
	    collen = td->horiz_diff;
	}
	else
	    collen = -td->horiz_diff;
	td->use_horiz_diff = FALSE;
    }

    while (line)
    {
	if (mark_line1 == line)
	{
	    mark = TRUE;
	}

	if (linenum >= td->vert_top)
	{
	    if (linenum >= minlinenum)
	    {
		struct TextExtent te;
		STRPTR text = line->ln_Text;
		LONG save_len = line->ln_TextLen;
		LONG len = save_len;
		LONG xoff = line->ln_XOffset;
		BOOL norectfill;

		/* if end of page - no longer draw */
		if (linenum + 1 > maxlinenum)
		    break;

		/* subtract the mincolumn and skip chars which should not displayed */
		xoff -= mincolnum * fontx;
		if (xoff < 0)
		{
		    len += xoff / fontx;
		    xoff = 0;
		    norectfill = TRUE;
		}
		else
		    norectfill = FALSE;

		if (len > 0)
		{
		    LONG width = collen * fontx - xoff;

		    text += save_len - len;

		    if (width > 0)
		    {
			len = TextFit(rp, text, len, &te, NULL, 1, width, fonty);
			if (len)
			{
			    LONG xpix = td->left + xoff + (mincolnum - td->horiz_top) * fontx;
			    LONG y1 = y + td->top;
			    LONG y2 = y + fonty + td->top - 1;

			    Move(rp, xpix, y1 + baseline);
			    SetSoftStyle(rp, line->ln_Style, AskSoftStyle(rp));

			    if (!mark)
			    {
				if (!norectfill)
				{
				    LONG x1 = xpix - GetRelativeOffset(line, fontx);
				    LONG x2 = xpix - 1;

				    if (x1 < td->left)
					x1 = td->left;
				    if (x1 <= x2)
				    {
					SetAPen(rp, line->ln_BgPen);
					RectFill(rp, x1, y1, x2, y2);
				    }
				}

				SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
				Text(rp, text, len);

			    }
			    else
			    {
				BOOL mark_start;

				if (mark_line1 == line)
				{
				    LONG newlen = mark_x1 - line->ln_XOffset / fontx;
				    LONG val = line->ln_XOffset / fontx - mincolnum;
				    LONG pen_no;

				    if (val < 0)
					newlen += val;

				    if (newlen > len)
					newlen = len;
				    if (newlen < 0)
				    {
					pen_no = 3;
					newlen = 0;
				    }
				    else
				    {
					pen_no = line->ln_BgPen;
				    }

				    if (!norectfill)
				    {
					LONG x1 = xpix - GetRelativeOffset(line, fontx);
					LONG x2 = xpix - 1;

					if (x1 < td->left)
					    x1 = td->left;
					if (x1 <= x2)
					{
					    SetAPen(rp, pen_no);
					    RectFill(rp, x1, y1, x2, y2);
					}
				    }

				    if (newlen)
				    {
					SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
					Text(rp, text, newlen);
					text += newlen;
					len -= newlen;
				    }
				    mark_start = TRUE;

				}       /* if(mark_line1 == line) */
				else
				    mark_start = FALSE;

				if (len > 0)
				{
				    if (mark_line2 == line)
				    {
					LONG newlen;

					if (mark_start)
					{
					    LONG val = mark_x1 - line->ln_XOffset / fontx;      //- mincolnum;

					    newlen = mark_x2 - mark_x1;
					    if (val < 0)
						newlen += val;
					    if (mark_x2 < mincolnum)
						newlen = 0;
					}
					else
					{
					    LONG val = line->ln_XOffset / fontx - mincolnum;

					    newlen = mark_x2 - line->ln_XOffset / fontx;        // - td->horiz_top;

					    if (val < 0)
						newlen += val;
					}

					if (!mark_start && !norectfill)
					{
					    LONG x1 = xpix - GetRelativeOffset(line, fontx);
					    LONG x2 = xpix - 1;

					    if (x1 < td->left)
						x1 = td->left;
					    if (x1 <= x2)
					    {
						SetAPen(rp, 3);
						RectFill(rp, x1, y1, x2, y2);
					    }
					}

					if (newlen < 0)
					    newlen = 0;
					if (newlen > len)
					    newlen = len;

					SetABPenDrMd(rp, 1, 3, JAM2);
					Text(rp, text, newlen);
					text += newlen;
					len -= newlen;

					if (len > 0)
					{
					    SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
					    Text(rp, text, len);
					}
				    }   /* if(mark_line2 == line) */
				    else
				    {
					if (mark_line1 != line && !norectfill)
					{
					    LONG x1 = xpix - GetRelativeOffset(line, fontx);
					    LONG x2 = xpix - 1;

					    if (x1 < td->left)
						x1 = td->left;
					    if (x1 <= x2)
					    {
						SetAPen(rp, 3);
						RectFill(rp, x1, y1, x2, y2);
					    }
					}

					SetABPenDrMd(rp, 1, 3, JAM2);
					Text(rp, text, len);

				    } /* if(mark_line2 == line) else ... */

				} /* if(len>0) */

			    } /* if(!mark) else ... */

			} /* if(len) */

		    } /* if(width > 0) */

		} /* if(len > 0) */

	    } /* if(linenum >= minlinenum) */

	    if (line->ln_Flags & LNF_LF)
	    {
		y += fonty;
	    }

	} /* if(linenum >= td->vert_top) */


	if (mark_line2 == line)
	    mark = FALSE;

	if (line->ln_Flags & LNF_LF)
	{
	    linenum++;
	}
	line = (struct Line *) Node_Next(line);

    } /* while(line) */

    SetABPenDrMd(rp, apen, bpen, mode);
    SetFont(rp, oldfont);
}

/**************************************************************************
 
**************************************************************************/
static void DrawMarkedText(struct Text_Data *td, struct RastPort *rp, LONG marked)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    LONG fontx = td->font->tf_XSize;
    LONG fonty = td->font->tf_YSize;
    LONG baseline = td->font->tf_Baseline;

    LONG linenum = 0;
    LONG minlinenum = td->vert_top;
    LONG maxlinenum = td->vert_top + td->vert_visible;
    LONG mincolnum = td->horiz_top;
    LONG collen = td->horiz_visible;

    LONG y = 0;
    ULONG apen, bpen, mode;
    struct TextFont *oldfont;
    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;

    LONG newcollen = td->width / td->font->tf_XSize;

    if (newcollen < collen)
	collen = newcollen;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

    GetRPAttrs(rp,
	       RPTAG_APen, &apen,
	       RPTAG_BPen, &bpen,
	       RPTAG_DrMd, &mode,
	       RPTAG_Font, &oldfont,
	       TAG_DONE);

    SetFont(rp, td->font);

    if (td->use_vert_diff)
    {
	if (td->vert_diff > 0)
	{
	    minlinenum = maxlinenum - td->vert_diff;
	}
	else
	{
	    maxlinenum = minlinenum - td->vert_diff;
	}
	td->use_vert_diff = FALSE;
    }

    if (td->use_horiz_diff)
    {
	if (td->horiz_diff > 0)
	{
	    mincolnum = mincolnum + collen - td->horiz_diff;
	    collen = td->horiz_diff;
	}
	else
	{
	    collen = -td->horiz_diff;
	}
	td->use_horiz_diff = FALSE;
    }

    while (line)
    {
	if (mark_line1 == line)
	{
	    mark = TRUE;
	}

	if (linenum >= td->vert_top)
	{
	    if (linenum >= minlinenum && mark)
	    {
		struct TextExtent te;
		STRPTR text = line->ln_Text;
		LONG save_len = line->ln_TextLen;
		LONG len = save_len;
		LONG xoff = line->ln_XOffset;

		if (linenum + 1 > maxlinenum)
		    break;

		xoff -= mincolnum * fontx;
		if (xoff < 0)
		{
		    len += xoff / fontx;
		    xoff = 0;
		}

		if (len > 0)
		{
		    LONG width = collen * fontx - xoff;

		    text += save_len - len;

		    if (width > 0)
		    {
			len = TextFit(rp, text, len, &te, NULL, 1, width, fonty);

			if (len)
			{
			    LONG xpix = td->left + xoff + (mincolnum - td->horiz_top) * fontx;
			    LONG y1 = y + td->top;
			    LONG y2 = y + fonty + td->top - 1;
			    BOOL mark_start;

			    Move(rp, xpix, y1 + baseline);
			    SetSoftStyle(rp, line->ln_Style, AskSoftStyle(rp));


			    if (mark_line1 == line)
			    {
				LONG newlen = mark_x1 - line->ln_XOffset / fontx;
				LONG val = line->ln_XOffset / fontx - mincolnum;
				LONG pen_no;

				if (val < 0)
				    newlen += val;

				if (newlen > len)
				    newlen = len;
				if (newlen < 0)
				{
				    LONG x1 = xpix - GetRelativeOffset(line, fontx);
				    LONG x2 = xpix - 1;

				    pen_no = marked ? 3 : line->ln_BgPen;
				    newlen = 0;

				    if (x1 < td->left)
					x1 = td->left;
				    if (x1 <= x2)
				    {
					SetAPen(rp, pen_no);
					RectFill(rp, x1, y1, x2, y2);
				    }
				}

				if (newlen)
				{
				    Move(rp, xpix + newlen * fontx, y1 + baseline);

				    text += newlen;
				    len -= newlen;
				}
				mark_start = TRUE;

			    }   /* if(mark_line1 == line) */
			    else
				mark_start = FALSE;

			    if (len > 0)
			    {
				if (mark_line2 == line)
				{
				    LONG newlen;

				    if (mark_start)
				    {
					LONG val = mark_x1 - line->ln_XOffset / fontx;

					newlen = mark_x2 - mark_x1;
					if (val < 0)
					    newlen += val;
				    }
				    else
				    {
					LONG val = line->ln_XOffset / fontx - mincolnum;

					newlen = mark_x2 - line->ln_XOffset / fontx;    // - td->horiz_top;

					if (val < 0)
					    newlen += val;
				    }

				    if (!mark_start)
				    {
					LONG x1 = xpix - GetRelativeOffset(line, fontx);
					LONG x2 = xpix - 1;

					if (x1 < td->left)
					    x1 = td->left;
					if (x1 <= x2)
					{
					    SetAPen(rp, marked ? 3 : line->ln_BgPen);
					    RectFill(rp, x1, y1, x2, y2);
					}
				    }

				    if (newlen < 0)
					newlen = 0;
				    if (newlen > len)
					newlen = len;

				    if (marked)
					SetABPenDrMd(rp, 1, 3, JAM2);
				    else
					SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);

				    Text(rp, text, newlen);
				    text += newlen;
				    len -= newlen;
				}       /* if(mark_line2 == line) */
				else
				{
				    if (mark_line1 != line)
				    {
					LONG x1 = xpix - GetRelativeOffset(line, fontx);
					LONG x2 = xpix - 1;

					if (x1 < td->left)
					    x1 = td->left;
					if (x1 <= x2)
					{
					    SetAPen(rp, marked ? 3 : line->ln_BgPen);
					    RectFill(rp, x1, y1, x2, y2);
					}
				    }

				    if (marked)
					SetABPenDrMd(rp, 1, 3, JAM2);
				    else
					SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);

				    Text(rp, text, len);

				}       /* if(mark_line2 == line) else ... */

			    }   /* if(len>0) */

			}       /* if(len) */

		    }   /* if(width > 0) */

		}       /* if(len > 0) */

	    }   /* if(linenum >= minlinenum && mark) */

	    if (line->ln_Flags & LNF_LF)
		y += fonty;

	}       /* if(linenum >= td->vert_top) */


	if (mark_line2 == line)
	    mark = FALSE;

	if (line->ln_Flags & LNF_LF)
	{
	    linenum++;
	}
	line = (struct Line *) line->ln_Link.mln_Succ;
    }   /* while(line) */

    SetABPenDrMd(rp, apen, bpen, mode);
    SetFont(rp, oldfont);

}

/**************************************************************************
 Scroll the area vertical (depending on td->vert_diff)
**************************************************************************/
static void ScrollYText(struct Text_Data *td, struct RastPort *rp)
{
    LONG addx = td->left;
    LONG addy = td->top;
    LONG maxx = td->left + td->width - 1;
    LONG maxy = td->top + td->height - 1;

    ScrollRasterBF(rp, 0, td->vert_diff * td->font->tf_YSize, addx, addy, maxx, maxy);
}

/**************************************************************************
 Scrool the area horizontal (depending on td->horiz_diff)
**************************************************************************/
static void ScrollXText(struct Text_Data *td, struct RastPort *rp)
{
    LONG addx = td->left;
    LONG addy = td->top;
    LONG maxx = td->left + td->width - 1;
    LONG maxy = td->top + td->height - 1;

    ScrollRasterBF(rp, td->horiz_diff * td->font->tf_XSize, 0, addx, addy, maxx, maxy);
}

/**************************************************************************************************/

static void SetTopText(struct Text_Data *td, struct RastPort *rp, LONG newy)
{
    if (newy < 0)
	newy = 0;

    if (rp)
    {
	if ((td->vert_diff = newy - td->vert_top))
	{
	    if (abs(td->vert_diff) < td->vert_visible)
	    {
		ScrollYText(td, rp);
		td->vert_top = newy;
		if ((rp->Layer->Flags & LAYERREFRESH))
		{
		    BeginUpdate(rp->Layer);
		    EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		    DrawText(td, rp);
		    EndUpdate(rp->Layer, TRUE);
		}

		td->use_vert_diff = TRUE;
	    }
	    else
	    {
		EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		td->vert_top = newy;
	    }
	    DrawText(td, rp);
	}
    }
    else
	td->vert_top = newy;
}

/**************************************************************************
 ...
**************************************************************************/
static void SetTopHorizText(struct Text_Data *td, struct RastPort *rp, LONG newx)
{
    if (rp)
    {
	if ((td->horiz_diff = newx - td->horiz_top))
	{
	    if (abs(td->horiz_diff) < td->horiz_visible)
	    {
		ScrollXText(td, rp);
		td->horiz_top = newx;
		if ((rp->Layer->Flags & LAYERREFRESH))
		{
		    BeginUpdate(rp->Layer);
		    EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		    DrawText(td, rp);
		    EndUpdate(rp->Layer, TRUE);
		}
		td->use_horiz_diff = TRUE;
	    }
	    else
	    {
		EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		td->horiz_top = newx;
	    }

	    DrawText(td, rp);
	}
    }
    else
	td->horiz_top = newx;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted.
**************************************************************************/
static struct Line *FindLine(struct Text_Data *td, LONG * px, LONG y)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    LONG linenum = 0;
    LONG x = *px;

    while (line)
    {
	if (y == linenum)
	{
	    LONG xcur = line->ln_XOffset / td->font->tf_XSize;
	    struct Line *prev_line;

	    if (x >= xcur && x <= xcur + line->ln_TextLen)
		break;

	    if ((prev_line = (struct Line *) Node_Prev(line)))
	    {
		if (!(prev_line->ln_Flags & LNF_LF))
		{
		    LONG line_xstart = prev_line->ln_XOffset / td->font->tf_XSize + prev_line->ln_TextLen;
		    if (x >= line_xstart && x < xcur)
		    {
			break;
		    }
		}
		else
		{
		    if (x < xcur)
		    {
			*px = 0;
			break;
		    }
		}
	    }

	    if (line->ln_Flags & LNF_LF)
	    {
		*px = xcur + line->ln_TextLen;
		break;
	    }

	}       /* if(y == linenum) */

	if (line->ln_Flags & LNF_LF)
	{
	    linenum++;
	}
	line = (struct Line *) Node_Next(line);

    }   /* while(line) */

    return line;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted to the beginning of a word.
**************************************************************************/
static struct Line *FindWordBegin(struct Text_Data *td, LONG * px, LONG y)
{
    struct Line *line = FindLine(td, px, y);

    if (line)
    {
	LONG wordx = *px - line->ln_XOffset / td->font->tf_XSize;
	if (wordx < 0)
	    wordx = 0;

	while (wordx > 0)
	{
	    if (line->ln_Text[wordx] == ' ')
	    {
		wordx++;
		break;
	    }
	    wordx--;
	}

	if (wordx >= line->ln_TextLen)
	    wordx = line->ln_TextLen - 1;
	if (wordx < 0)
	    wordx = 0;
	*px = wordx + line->ln_XOffset / td->font->tf_XSize;
    }

    return line;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted to the end of a word.
**************************************************************************/
static struct Line *FindWordEnd(struct Text_Data *td, LONG * px, LONG y)
{
    struct Line *line = FindLine(td, px, y);

    if (line)
    {
	LONG wordx = *px - line->ln_XOffset / td->font->tf_XSize;

	if (wordx >= line->ln_TextLen)
	    wordx = line->ln_TextLen - 1;

	while (wordx < line->ln_TextLen)
	{
	    if (line->ln_Text[wordx] == ' ')
		break;
	    wordx++;
	}
	if (wordx < 0)
	    wordx = 0;
	*px = wordx + line->ln_XOffset / td->font->tf_XSize;
    }

    return line;
}

/**************************************************************************
 Handle Mouse Inputs especially the marking of the text
**************************************************************************/
static int HandleMouse(struct Text_Data *td, struct RastPort *rp, LONG x, LONG y, LONG code, ULONG secs, ULONG mics)
{
    x = (x / td->font->tf_XSize) + td->horiz_top;
    y = (y / td->font->tf_YSize) + td->vert_top;

    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;

    if (code == SELECTDOWN)
    {
	LONG old_dclick = td->doubleclick;

	ClearSelected(td, rp);
	td->mark_line1 = td->mark_line2 = FindLine(td, &x, y);
	td->mark_x1 = x;
	td->mark_x2 = x;
	td->mark_y1 = y;
	td->mark_y2 = y;
	td->pressed = TRUE;
	td->doubleclick = 0;

	if (DoubleClick(td->lastsecs, td->lastmics, secs, mics))
	{
	    if (old_dclick == 0)
	    {
		/* x wird an Grenzen angepaßt */
		LONG xstart = x;
		LONG xend = x;

		struct Line *newline = FindWordBegin(td, &xstart, y);

		FindWordEnd(td, &xend, y);

		td->mark_x1 = xstart;
		td->mark_y1 = y;
		td->mark_line1 = newline;
		td->mark_x2 = xend;
		td->mark_y2 = y;
		td->mark_line2 = newline;

		DrawMarkedText(td, rp, TRUE);

		td->doubleclick = 1;
	    }
	    else
	    {
		/* Tripple click */
		LONG x = 0;

		td->doubleclick = 2;
		secs = mics = 0;

		td->mark_line1 = FindLine(td, &x, y);
		td->mark_x1 = x;
		td->mark_y1 = y;

		x = 0x7fffffff;
		td->mark_line2 = FindLine(td, &x, y);
		td->mark_x2 = x;
		td->mark_y2 = y;

		DrawMarkedText(td, rp, TRUE);
	    }

	}       /* if (DoubleClick(td->lastsecs, td->lastmics, secs, mics)) */

	td->lastsecs = secs;
	td->lastmics = mics;

    }   /* if (code == SELECTDOWN) */
    else
    {
	if (td->pressed)
	{
	    struct Line *newline = FindLine(td, &x, y);

	    struct Line *old_line = td->mark_line1;
	    LONG old_x = td->mark_x1;
	    LONG old_y = td->mark_y1;

	    BOOL mouse_up;
	    BOOL erase;
	    BOOL twotimes;

	    if (y < td->mark_y2 || (y == td->mark_y2 && x < td->mark_x2))
		mouse_up = TRUE;
	    else
		mouse_up = FALSE;

	    if (td->doubleclick == 1)
	    {
		if ((td->mark_y1 == td->mark_y2 && td->mark_x1 > td->mark_x2) ||
		    (td->mark_y1 > td->mark_y2))
		    FindWordBegin(td, &x, y);
		else
		    FindWordEnd(td, &x, y);
	    }
	    else if (td->doubleclick == 2)
	    {
		if (td->mark_y1 > y)
		{
		    x = 0;
		    newline = FindLine(td, &x, y);
		}
		else
		{
		    x = 0x7fffffff;
		    newline = FindLine(td, &x, y);
		}
	    }

	    if (td->mark_y1 < td->mark_y2 || (td->mark_y1 == td->mark_y2 && td->mark_x1 < td->mark_x2))
	    {
		if (!mouse_up)
		    erase = TRUE;
		else
		    erase = FALSE;

		if (td->mark_y1 < y || (td->mark_y1 == y && td->mark_x1 < x))
		    twotimes = FALSE;
		else
		    twotimes = TRUE;
	    }
	    else
	    {
		if (mouse_up)
		    erase = TRUE;
		else
		    erase = FALSE;

		if (td->mark_y1 < y || (td->mark_y1 == y && td->mark_x1 < x))
		    twotimes = TRUE;
		else
		    twotimes = FALSE;
	    }

	    if (twotimes)
	    {
		DrawMarkedText(td, rp, FALSE);

		td->mark_x2 = x;
		td->mark_y2 = y;
		td->mark_line2 = newline;

		DrawMarkedText(td, rp, TRUE);
	    }
	    else
	    {
		td->mark_x1 = td->mark_x2;
		td->mark_y1 = td->mark_y2;
		td->mark_line1 = td->mark_line2;
		td->mark_x2 = x;
		td->mark_y2 = y;
		td->mark_line2 = newline;

		DrawMarkedText(td, rp, erase);

		td->mark_x1 = old_x;
		td->mark_y1 = old_y;
		td->mark_line1 = old_line;
	    }

	    td->mark_x2 = x;
	    td->mark_y2 = y;
	    td->mark_line2 = newline;

	    if (code == SELECTUP)
		td->pressed = FALSE;

#ifndef COMPILE_DATATYPE
	    if (code == SELECTUP)
	    {
		CopyText(td);
	    }
#endif
	}       /* if(td->pressed) */

    }   /* if (code == SELECTDOWN) else ... */

    return 0;
}

/**************************************************************************
 Clear the selected area
**************************************************************************/
static void ClearSelected(struct Text_Data *td, struct RastPort *rp)
{
    BOOL refresh;
    if (td->mark_x1 != -1 || td->mark_x2 != -1)
	refresh = TRUE;
    else
	refresh = FALSE;

    D(bug("text.datatype/ClearSelected: ClearSelected rp=0x%lx  %ld\n", rp, refresh));

    td->mark_x1 = -1;
    td->mark_x2 = -1;
    td->mark_y1 = -1;
    td->mark_y2 = -1;
    td->mark_line1 = td->mark_line2 = NULL;
    td->pressed = FALSE;

    if (rp && refresh)
	DrawText(td, rp);
}

/**************************************************************************
 Copy the selected text passage
**************************************************************************/
static void CopyText(struct Text_Data *td)
{
    struct IFFHandle *iff = PrepareClipboard();

    if (iff)
    {
	struct Line *line = (struct Line *) List_First(&td->line_list);

	UWORD fontx = td->font->tf_XSize;
	BOOL mark = FALSE;
	LONG mark_y1, mark_x1;
	LONG mark_y2, mark_x2;
	struct Line *mark_line1, *mark_line2;

	PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

	while (line->ln_Link.mln_Succ)
	{
	    LONG len;
	    STRPTR text = line->ln_Text;

	    if (mark_line1 == line)
	    {
		LONG txt_add;

		if (mark_line2 == line)
		    len = mark_x2 - mark_x1;
		else
		{
		    len = line->ln_TextLen - (mark_x1 - line->ln_XOffset / fontx);
		    mark = TRUE;
		}

		txt_add = mark_x1 - line->ln_XOffset / fontx;

		if (txt_add > 0)
		    text += txt_add;
		else
		{
		    LONG space = GetRelativeOffset(line, fontx) / fontx;

		    if (space > 0)
			len -= space;
		    while (space > 0)
		    {
			WriteChunkBytes(iff, "\t", 1);
			space -= 8;
		    }
		}

		if (len > 0)
		{
		    WriteChunkBytes(iff, text, len);
		    if (mark)
			if (line->ln_Flags & LNF_LF)
			    WriteChunkBytes(iff, "\n", 1);
		}
	    } /* if(mark_line1 == line) */
	    else
	    {
		if (mark)
		{
		    if (mark_line2 == line)
		    {
			len = mark_x2 - line->ln_XOffset / fontx;
			if (len > 0)
			{
			    LONG space = GetRelativeOffset(line, fontx) / fontx;
			    while (space > 0)
			    {
				WriteChunkBytes(iff, "\t", 1);
				space -= 8;
			    }

			    WriteChunkBytes(iff, text, len);
			}
			mark = FALSE;
		    }
		    else
		    {
			LONG space = GetRelativeOffset(line, fontx) / fontx;
			while (space > 0)
			{
			    WriteChunkBytes(iff, "\t", 1);
			    space -= 8;
			}

			WriteChunkBytes(iff, text, line->ln_TextLen);

			if (line->ln_Flags & LNF_LF)
			    WriteChunkBytes(iff, "\n", 1);
		    }

		} /* if(mark) */

	    } /* if(mark_line1 == line) else ... */

	    line = (struct Line *) line->ln_Link.mln_Succ;

	} /* while(line->ln_Link.mln_Succ) */
	FreeClipboard(iff);

    } /* if(iff) */
}

/**************************************************************************
 Print the text (whole text only)
**************************************************************************/
static void PrintText(struct Text_Data *td, union printerIO *printer_io)
{
    D(bug("text.datatype/PrintText: Printing Text...\n"));

    if (printer_io)
    {
	struct Line *line = (struct Line *) List_First(&td->line_list);
	LONG sig_mask = 1L << printer_io->ios.io_Message.mn_ReplyPort->mp_SigBit;
	LONG sigs;

	while (line)
	{
	    LONG fontx = td->font->tf_XSize;
	    LONG space_len = GetRelativeOffset(line, fontx) / fontx;

	    while (space_len)
	    {
		printer_io->ios.io_Length = 1;
		printer_io->ios.io_Data = (APTR) " ";
		printer_io->ios.io_Command = CMD_WRITE;
		DoIO((struct IORequest *) printer_io);
		space_len--;
	    }

	    if (line->ln_TextLen)
	    {
		printer_io->ios.io_Length = line->ln_TextLen;
		printer_io->ios.io_Data = (APTR) line->ln_Text;
		printer_io->ios.io_Command = CMD_WRITE;
		DoIO((struct IORequest *) printer_io);
	    }

	    if (line->ln_Flags & LNF_LF)
	    {
		printer_io->ios.io_Length = 1;
		printer_io->ios.io_Data = (APTR) "\n";
		printer_io->ios.io_Command = CMD_WRITE;
		SendIO((struct IORequest *) printer_io);

		sigs = Wait(SIGBREAKF_CTRL_C | sig_mask);
		if (sigs & SIGBREAKF_CTRL_C)
		{
		    if (!(sigs & sig_mask))
		    {
			AbortIO((struct IORequest *) printer_io);
			WaitIO((struct IORequest *) printer_io);
		    }
		    break;
		}
	    }
	    line = (struct Line *) line->ln_Link.mln_Succ;

	}       /* while(line) */

    }   /* if (printer_io) */

    D(bug("text.datatype/PrintText: Printing Text finished...\n"));
}

#ifndef COMPILE_DATATYPE

/**************************************************************************
 Load an AscII Text in a ascii.datatype compatible manner.
 Taken from the Datatypes Tutorial of the Amiga Developer KIT.
**************************************************************************/
static int LoadAsAscII(struct Text_Data *td)
{
    if ((td->line_pool = CreatePool(MEMF_CLEAR, 16384, 16384)))
    {
	ULONG visible = 0, total = 0;
	struct RastPort trp;
	ULONG hunit = 1;
	ULONG bsig = 0;

	/* Switches */
	BOOL linefeed = FALSE;
	BOOL newseg = FALSE;
	BOOL abort = FALSE;

	/* Attributes obtained from super-class */
	struct TextFont *font = td->font;
	struct List *linelist = &td->line_list;
	ULONG wrap = FALSE;
	ULONG bufferlen = td->buffer_allocated_len;
	STRPTR buffer = td->buffer_allocated;

	/* Line information */
	ULONG num, offset, swidth;
	ULONG anchor = 0, newanchor;
	ULONG style = FS_NORMAL;
	struct Line *line;
	ULONG yoffset = 0;
	UBYTE fgpen = 1;
	UBYTE bgpen = 0;
	ULONG tabspace;
	ULONG numtabs;
	ULONG i, j;

	ULONG nomwidth, nomheight;

	/* Initialize the temporary RastPort */
	InitRastPort(&trp);
	SetFont(&trp, font);

	/* Calculate the nominal size */
	nomheight = (ULONG) (24 * font->tf_YSize);
	nomwidth = (ULONG) (80 * font->tf_XSize);

	/* Calculate the tab space */
	tabspace = font->tf_XSize * 8;

	/* We only need to perform layout if we are doing word wrap, or this
	   is the initial layout call */

	/* Delete the old line list */
	while ((line = (struct Line *) RemHead(linelist)))
	    FreePooled(td->line_pool, line, sizeof(struct Line));

	/* Step through the text buffer */
	for (i = offset = num = numtabs = 0;
	     (i <= bufferlen) && (bsig == 0) && !abort;
	     i++)
	{
	    /* Check for end of line */
	    if (buffer[i] == 10)        // && buffer[i+1]==10)

	    {
		newseg = linefeed = TRUE;
		newanchor = i + 1;
	    }
	    /* Check for end of page */
	    else if (buffer[i] == 12)
	    {
		newseg = linefeed = TRUE;
		newanchor = i + 1;
	    }
	    /* Check for tab */
	    else if (buffer[i] == 9)
	    {
		/* See if we need to terminate a line segment */
		if ((numtabs == 0) && num)
		    newseg = TRUE;
		numtabs++;
	    }
	    else
	    {
		/* See if we have any TABs that we need to finish out */
		if (numtabs)
		{
		    offset += (((offset / tabspace) + 1) * tabspace) - offset;
		    num = numtabs = 0;
		    anchor = i;
		}

		/* Compute the width of the line. */
		swidth = TextLength(&trp, &buffer[anchor], num + 1);
		num++;
	    }

	    /* Time for a new text segment yet? */
	    if (newseg)
	    {
		/* Allocate a new line segment from our memory pool */
		if ((line = AllocPooled(td->line_pool, sizeof(struct Line))))
		{
		    swidth = TextLength(&trp, &buffer[anchor], num);
		    line->ln_Text = &buffer[anchor];
		    line->ln_TextLen = num;
		    line->ln_XOffset = offset;
		    line->ln_YOffset = yoffset + font->tf_Baseline;
		    line->ln_Width = swidth;
		    line->ln_Height = font->tf_YSize;
		    line->ln_Flags = (linefeed) ? LNF_LF : NULL;
		    line->ln_FgPen = fgpen;
		    line->ln_BgPen = bgpen;
		    line->ln_Style = style;
		    line->ln_Data = NULL;

		    /* Add the line to the list */
		    AddTail(linelist, (struct Node *) line);

		    /* Increment the line count */
		    if (linefeed)
		    {
			yoffset += font->tf_YSize;
			offset = 0;
			total++;
		    }
		    else
		    {
			/* Increment the offset */
			offset += swidth;
		    }
		}
		else
		{
		    abort = TRUE;
		}

		/* Clear the variables */
		newseg = linefeed = FALSE;
		anchor = newanchor;
		num = 0;

		/* Check to see if layout has been aborted */
		bsig = CheckSignal(SIGBREAKF_CTRL_C);

	    } /* if (newseg) */

	} /* step through the text buffer */

    } /* if((td->line_pool = CreatePool(MEMF_CLEAR, 16384, 16384))) */

    return 1;
}


#endif

/**************************************************************************************************/

#ifdef COMPILE_DATATYPE

const static ULONG supported_methods[] =
{
    DTM_COPY,
    DTM_SELECT,
    DTM_CLEARSELECTED,
    DTM_WRITE,
    DTM_PRINT,
    ~0,
};

STATIC ULONG notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
    return DoMethod(o, OM_NOTIFY, &tag1, ginfo, flags);
}

struct AsyncMethodMsg
{
    struct Message amm_ExecMessage;
    Object *amm_Object;
    Msg amm_Msg;
};

/* SAVEDS or simliar not needed */
ULONG asyncmethodfunc(void)
{
#ifndef _AROS
    struct Library *SysBase = *((struct Library **) 4L);
#endif
    struct Process *proc = (struct Process *) FindTask(NULL);
    Object *obj;
    Msg msg;
    struct AsyncMethodMsg *amsg;

    WaitPort(&proc->pr_MsgPort);
    amsg = (struct AsyncMethodMsg *) GetMsg(&proc->pr_MsgPort);
    obj = amsg->amm_Object;
    msg = amsg->amm_Msg;
    ReplyMsg(&amsg->amm_ExecMessage);

    D(bug("text.datatype/asyncmethodfunc: Recieved object : %lx, msg : %lx\n",obj,msg));

    return DoMethodA(obj,msg);
}

STATIC struct Process *DoAsyncMethodA(Class *cl,Object *obj,Msg msg,struct TagItem *tagList)
{
    struct MsgPort *mport;
    struct Process *proc = NULL;

    if ((mport = CreateMsgPort()))
    {
	struct AsyncMethodMsg *amsg = (struct AsyncMethodMsg*)AllocVec(sizeof(struct AsyncMethodMsg),MEMF_PUBLIC);
	if (amsg)
	{
	    amsg->amm_Object = obj;
	    amsg->amm_Msg    = msg;

	    amsg->amm_ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
	    amsg->amm_ExecMessage.mn_ReplyPort    = mport;

	    if ((proc = CreateNewProcTags(NP_Entry,asyncmethodfunc,
					  (tagList) ? TAG_MORE : TAG_IGNORE,tagList,
					  TAG_DONE)))
	    {
		PutMsg(&proc->pr_MsgPort,&amsg->amm_ExecMessage);
		WaitPort(mport);
	    }
	    FreeVec(amsg);
	}
	DeleteMsgPort(mport);
    }
    return proc;
}

STATIC struct Process *DoAsyncMethod(Class *cl,Object *obj,Msg msg,ULONG tag1,...)
{
    return DoAsyncMethodA(cl,obj,msg,(struct TagItem *) &tag1);
}

STATIC struct Gadget *DT_NewMethod(struct IClass *cl, Object * o, struct opSet *msg)
{
    struct Gadget *g;
    struct TagItem *attrs = msg->ops_AttrList;
    ULONG st;

    st = GetTagData(DTA_SourceType, DTST_FILE, attrs);

    if (st != DTST_CLIPBOARD && st != DTST_FILE)
    {
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    if ((g = (struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg)))
    {
	struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
	struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;
	struct TagItem *ti;

	memset(td, 0, sizeof(struct Text_Data));
	if ((ti = FindTagItem(DTA_TextAttr, msg->ops_AttrList)))
	{
	    struct TextAttr *ta = (struct TextAttr *) (struct TextAttr *) ti->ti_Data;

	    td->attr.ta_Name = StrCopy(ta->ta_Name);
	    td->attr.ta_YSize = ta->ta_YSize;
	}
	InitText(td);

	si->si_HorizUnit = 1;
	si->si_VertUnit = td->font->tf_YSize;

	if (st == DTST_CLIPBOARD || st == DTST_FILE)
	{
	    APTR handle;
	    if (GetDTAttrs((Object *) g, DTA_Handle, &handle, TAG_DONE) != 1)
		handle = NULL;
	    if (handle)
	    {
		if (st == DTST_CLIPBOARD)
		{
		    if (LoadTextAsIFF(td, "Clipboard", (struct IFFHandle *) handle))
			return g;
		}
		else
		{
		    char buf[16];
		    LONG type;
		    STRPTR name;
		    struct DataType *dt;

		    if (GetDTAttrs((Object *) g, DTA_Name, &name, DTA_DataType, &dt, TAG_DONE) != 2)
		    {
			name = NULL;
			dt = NULL;
		    }

		    if (!name)
			name = "Unnamed";
		    if (dt)
		    {
			type = dt->dtn_Header->dth_Flags & DTF_TYPE_MASK;
		    }
		    else
			type = DTF_MISC;

		    D(bug("text.datatype/DT_NewMethod: Handle: 0x%lx    type: %lx\n", handle, dt->dtn_Header->dth_Flags));

		    td->oldmarkactivation = 0;
		    if (GetVar("TEXTDT_MENUMARKING", buf, sizeof(buf), LV_VAR) > 0)
		    {
			if (*buf == '1')
			    td->oldmarkactivation = 1;
		    }

		    switch (type)
		    {
		    case DTF_IFF:
			{
			    if (LoadTextAsIFF(td, name, (struct IFFHandle *) handle))
				return g;
			}
			break;

		    case DTF_BINARY:
		    case DTF_ASCII:
			{
			    D(bug("text.datatype/DT_NewMethod: Loading the text %s\n", name));

			    if (LoadText(td, name, (BPTR) handle))
			    {
				D(bug("text.datatype/DT_NewMethod: Text loaded\n"));
				return g;
			    }
			}
			break;

		    default:
			td->title = StrCopy(name);
			return g;

		    } /* switch (type) */

		} /* if (st == DTST_CLIPBOARD) else ... */

	    } /* if (handle) */
	    else
		SetIoErr(ERROR_OBJECT_NOT_FOUND);

	} /* if(st == DTST_CLIPBOARD || st == DTST_FILE) */
	CoerceMethod(cl, (Object *) g, OM_DISPOSE);

    } /* if((g = (struct Gadget*)DoSuperMethodA(cl, o, (Msg)msg))) */
    return NULL;

}

STATIC VOID DT_DisposeMethod(struct IClass * cl, Object * o, Msg msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

    DisposeText(td);
    DoSuperMethodA(cl, o, msg);
}

STATIC ULONG DT_GetMethod(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    switch (msg->opg_AttrID)
    {
    case TDTA_Buffer:
	D(bug("text.datatype/DT_GetMethod: Tag ID: TDTA_Buffer 0x%lx\n", td->buffer_allocated));
	*msg->opg_Storage = (ULONG) td->buffer_allocated;

	break;

    case TDTA_BufferLen:
	D(bug("text.datatype/DT_GetMethod: Tag ID: TDTA_BufferLen  %ld\n", td->buffer_allocated_len));
	*msg->opg_Storage = (ULONG) td->buffer_allocated_len;

	break;

    case TDTA_LineList:
	D(bug("text.datatype/DT_GetMethod: Tag ID: TDTA_LineList\n"));
	*msg->opg_Storage = (ULONG) & td->line_list;
	break;

    case TDTA_WordWrap:
	D(bug("text.datatype/DT_GetMethod: Tag ID: TDTA_WordWrap\n"));
	*msg->opg_Storage = FALSE;
	break;

    case DTA_TextAttr:
	D(bug("text.datatype/DT_GetMethod: Tag ID: DTA_TextAttr (%s)\n", td->attr.ta_Name));
	*msg->opg_Storage = (ULONG) & td->attr;
	break;

    case DTA_TextFont:
	D(bug("text.datatype/DT_GetMethod: Tag ID: DTA_TextFont\n"));
	*msg->opg_Storage = (ULONG) td->font;
	break;

    case DTA_ObjName:
	D(bug("text.datatype/DT_GetMethod: Tag ID: DTA_ObjName\n"));
	*msg->opg_Storage = (ULONG) td->title;
	break;

    case DTA_Methods:
	D(bug("text.datatype/DT_GetMethod: Tag ID: DTA_Methods\n"));
	*msg->opg_Storage = (ULONG) supported_methods;
	break;

    default:
	D(bug("text.datatype/DT_GetMethod: Tag ID: 0x%lx\n", msg->opg_AttrID));
	return DoSuperMethodA(cl, (Object *) g, (Msg) msg);
    }
    return 1;
}

STATIC ULONG DT_SetMethod(struct IClass * cl, struct Gadget * g, struct opSet * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;

    LONG top_vert = 0;
    LONG top_horiz = 0;
    BOOL new_top_vert = FALSE;
    BOOL new_top_horiz = FALSE;
    BOOL redraw_all = FALSE;
    BOOL layout = FALSE;

    ULONG retval = 0;

    D(bug("text.datatype/DT_SetMethod: GInfo: 0x%lx\n", msg->ops_GInfo));

    while ((ti = NextTagItem(&tl)))
    {
	switch (ti->ti_Tag)
	{
	case GA_Width:
	    D(bug("text.datatype/DT_SetMethod: GA_Width  %ld\n", ti->ti_Data));
	    break;

	case GA_Height:
	    D(bug("text.datatype/DT_SetMethod: GA_Height  %ld\n", ti->ti_Data));
	    break;

	case GA_Left:
	    D(bug("text.datatype/DT_SetMethod: GA_Left  %ld\n", ti->ti_Data));
	    break;

	case GA_Top:
	    D(bug("text.datatype/DT_SetMethod: GA_Top  %ld\n", ti->ti_Data));
	    break;

	case GA_RelWidth:
	    D(bug("text.datatype/DT_SetMethod: GA_RelWidth  %ld\n", ti->ti_Data));
	    break;

	case GA_RelHeight:
	    D(bug("text.datatype/DT_SetMethod: GA_RelHeight  %ld\n", ti->ti_Data));
	    break;

	case GA_ID:
	    D(bug("text.datatype/DT_SetMethod: GA_ID  %ld\n", ti->ti_Data));
	    break;

	case DTA_TextAttr:
	    D(bug("text.datatype/DT_SetMethod: DTA_TextAttr  0x%lx  %s\n", ti->ti_Data, ((struct TextAttr *) ti->ti_Data)->ta_Name));
	    break;

	case DTA_TopVert:
	    {
		D(bug("text.datatype/DT_SetMethod: DTA_TopVert  %ld\n", ti->ti_Data));
		top_vert = ti->ti_Data;
		if (top_vert < 0)
		    top_vert = 0;

		if (top_vert != td->vert_top)
		    new_top_vert = TRUE;
	    }
	    break;

	case DTA_TopHoriz:
	    {
		D(bug("text.datatype/DT_SetMethod: DTA_TopHoriz  %ld\n", ti->ti_Data));
		top_horiz = ti->ti_Data;
		if (top_horiz < 0)
		    top_horiz = 0;

		if (top_horiz != td->horiz_top)
		    new_top_horiz = TRUE;
	    }
	    break;

	case DTA_VisibleVert:
	    D(bug("text.datatype/DT_SetMethod: DTA_VisibleVert  %ld\n", ti->ti_Data));
	    td->vert_visible = ti->ti_Data;
	    redraw_all = TRUE;
	    break;

	case DTA_TotalVert:
	    D(bug("text.datatype/DT_SetMethod: DTA_TotalVert  %ld\n", ti->ti_Data));
	    break;

	case DTA_NominalVert:
	    D(bug("text.datatype/DT_SetMethod: DTA_NominalVert  %ld\n", ti->ti_Data));
	    break;

	case DTA_VertUnit:
	    D(bug("text.datatype/DT_SetMethod: DTA_VertUnit  %ld\n", ti->ti_Data));
	    break;

	case DTA_VisibleHoriz:
	    D(bug("text.datatype/DT_SetMethod: DTA_VisibleHoriz  %ld\n", ti->ti_Data));
	    td->horiz_visible = ti->ti_Data;
	    redraw_all = TRUE;
	    break;

	case DTA_TotalHoriz:
	    D(bug("text.datatype/DT_SetMethod: DTA_TotalHoriz  %ld\n", ti->ti_Data));
	    break;

	case DTA_NominalHoriz:
	    D(bug("text.datatype/DT_SetMethod: DTA_NominalHoriz  %ld\n", ti->ti_Data));
	    break;

	case DTA_HorizUnit:
	    D(bug("text.datatype/DT_SetMethod: DTA_HorizUnit  %ld\n", ti->ti_Data));
	    break;

	case DTA_Title:
	    D(bug("text.datatype/DT_SetMethod: DTA_Title  %ld\n", ti->ti_Data));
	    break;

	case DTA_Busy:
	    D(bug("text.datatype/DT_SetMethod: DTA_Busy  %ld\n", ti->ti_Data));
	    break;

	case DTA_Sync:
	    D(bug("text.datatype/DT_SetMethod: DTA_Sync  %ld\n", ti->ti_Data));
	    break;

	case DTA_LayoutProc:
	    D(bug("text.datatype/DT_SetMethod: DTA_LayoutProc  0x%lx\n", ti->ti_Data));
	    break;

	case TDTA_Buffer:
	    D(bug("text.datatype/DT_SetMethod: TDTA_Buffer  0x%lx\n", ti->ti_Data));
	    td->buffer_allocated = (STRPTR) ti->ti_Data;
	    break;

	case TDTA_BufferLen:
	    D(bug("text.datatype/DT_SetMethod: TDTA_BufferLen  0x%ld\n", ti->ti_Data));
	    td->buffer_allocated_len = (ULONG) ti->ti_Data;
	    break;

	default:
	    D(bug("text.datatype/DT_SetMethod: 0x%lx  %ld\n", ti->ti_Tag, ti->ti_Data));
	    break;

	} /* switch (ti->ti_Tag) */

    } /* while ((ti = NextTagItem(&tl))) */

    if (layout && msg->ops_GInfo)
    {
	DoMethod((Object *) g, GM_LAYOUT, msg->ops_GInfo, TRUE);
    }

    if (redraw_all || (new_top_horiz && new_top_vert))
    {
	td->update_type = 1;
	retval = 1;
    }
    else
    {
	if (new_top_horiz || new_top_vert)
	{
	    if (new_top_horiz)
	    {
		retval = 1;
		td->update_type = 2;
		td->update_arg = top_horiz;
	    }

	    if (new_top_vert)
	    {
		retval = 1;
		td->update_type = 3;
		td->update_arg = top_vert;
	    }
	}
    }

    retval += DoSuperMethodA(cl, (Object *) g, (Msg) msg);
    D(bug("text.datatype/DT_SetMethod: retval: %ld\n", retval));
    return retval;
}

STATIC ULONG DT_Render(struct IClass * cl, struct Gadget * g, struct gpRender * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;

    D(bug("text.datatype/DT_Render: gpr_Redraw: %ld  si->si_Flags: %lx\n", msg->gpr_Redraw, si->si_Flags));

    if (!(si->si_Flags & DTSIF_LAYOUT))
    {
	struct IBox *domain;
	LONG vh, vv;

	if (GetDTAttrs((Object *) g,
		       DTA_Domain, &domain,
		       DTA_VisibleHoriz, &vh,
		       DTA_VisibleVert, &vv,
		       TAG_DONE) == 3)
	{
	    ObtainSemaphore(&(si->si_Lock));

	    td->left = domain->Left;
	    td->top = domain->Top;
	    td->width = domain->Width;
	    td->height = domain->Height;

	    /* Needed for AmigaGuide */
	    td->horiz_visible = vh;
	    td->vert_visible = vv;

	    D(bug("text.datatype/DT_Render: in Render: %ld  %ld\n", vh, vv));

	    if (msg->gpr_Redraw == GREDRAW_REDRAW)
	    {
		DrawText(td, msg->gpr_RPort);
	    }
	    else
	    {
		if (msg->gpr_Redraw == GREDRAW_UPDATE)
		{
		    D(bug("text.datatype/DT_Render: Update: %ld\n", td->update_type));
		    switch (td->update_type)
		    {
		    case 1:
			DrawText(td, msg->gpr_RPort);
			break;

		    case 2:
			SetTopHorizText(td, msg->gpr_RPort, td->update_arg);
			break;

		    case 3:
			SetTopText(td, msg->gpr_RPort, td->update_arg);
			break;
		    }

		    td->update_type = 0;
		}
	    }
	    ReleaseSemaphore(&(si->si_Lock));

	} /* if (GetDtAttrs(... */

    } /* if(!(si->si_Flags & DTSIF_LAYOUT)) */

    return 0;
}

STATIC LONG DT_HandleInputMethod(struct IClass * cl, struct Gadget * g, struct gpInput * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct InputEvent *ievent = msg->gpi_IEvent;
    struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;
    LONG retval = GMR_MEACTIVE;

    if (!AttemptSemaphore(&(si->si_Lock)))
	return GMR_NOREUSE;

    if (ievent->ie_Class == IECLASS_RAWMOUSE || ievent->ie_Class == IECLASS_TIMER)
    {
	if (ievent->ie_Class == IECLASS_RAWMOUSE)
	{
	    if (ievent->ie_Code == SELECTUP)
	    {
		td->mouse_pressed = FALSE;
		retval = GMR_NOREUSE;
	    }
	    else
	    {
		if (ievent->ie_Code == SELECTDOWN)
		{
		    td->mouse_pressed = TRUE;
		}
	    }
	}

	if (td->mouse_pressed)
	{
	    LONG diff_x, diff_y;
	    LONG x = msg->gpi_Mouse.X, y = msg->gpi_Mouse.Y;
	    LONG newx, newy;

	    if (x<0)
		diff_x = x / td->font->tf_XSize;
	    else
	    {
		if (x > td->width)
		    diff_x = (x - td->width + td->font->tf_XSize) / td->font->tf_XSize;
		else
		    diff_x = 0;
	    }
 

	    if (y < 0)
		diff_y = y / td->font->tf_YSize;
	    else
	    {
		if (y > td->height)
		    diff_y = (y - td->height + td->font->tf_YSize) / td->font->tf_YSize;
		else
		    diff_y = 0;
	    }

	    if (diff_x)
	    {
#ifdef _AROS
		IPTR val;
		LONG top, total, visible;
		
		GetDTAttrs((Object *)g, DTA_TopHoriz, &val, TAG_DONE); top = (LONG)val;
		GetDTAttrs((Object *)g, DTA_TotalHoriz, &val, TAG_DONE); total = (LONG)val;
		GetDTAttrs((Object *)g, DTA_VisibleHoriz, &val, TAG_DONE); visible = (LONG)val;

		newx = td->horiz_top + ((diff_x < 0) ? -1 : 1);

		if (newx + visible > total) newx = total - visible;
		if (newx < 0) newx = 0;
		
		if (newx != top)
		    notifyAttrChanges((Object *) g, msg->gpi_GInfo, NULL,
				      GA_ID, g->GadgetID,
				      DTA_TopHoriz, newx,
				      TAG_DONE);
#else
		newx = td->horiz_top + ((diff_x < 0) ? -1 : 1);
		notifyAttrChanges((Object *) g, ((struct gpLayout *) msg)->gpl_GInfo, NULL,
				  GA_ID, g->GadgetID,
				  DTA_TopHoriz, newx,
				  TAG_DONE);
#endif
	    }

	    if (diff_y)
	    {
#ifdef _AROS
		IPTR val;
		LONG top, total, visible;
		
		GetDTAttrs((Object *)g, DTA_TopVert, &val, TAG_DONE); top = (LONG)val;
		GetDTAttrs((Object *)g, DTA_TotalVert, &val, TAG_DONE); total = (LONG)val;
		GetDTAttrs((Object *)g, DTA_VisibleVert, &val, TAG_DONE); visible = (LONG)val;

		newy = td->vert_top + ((diff_y < 0) ? -1 : 1);

		if (newy + visible > total) newy = total - visible;
		if (newy < 0) newy = 0;
		
		if (newy != top)
		    notifyAttrChanges((Object *) g, msg->gpi_GInfo, NULL,
				      GA_ID, g->GadgetID,
				      DTA_TopVert, newy,
				      TAG_DONE);
		
#else
		newy = td->vert_top + ((diff_y < 0) ? -1 : 1);
		notifyAttrChanges((Object *) g, msg->gpi_GInfo, NULL,
				  GA_ID, g->GadgetID,
				  DTA_TopVert, newy,
				  TAG_DONE);
#endif
	    }
	}
    }

    if (!td->oldmarkactivation || si->si_Flags & DTSIF_DRAGSELECT)
    {
	if (ievent->ie_Class == IECLASS_RAWMOUSE)
	{
	    struct RastPort *rp;
	    rp = ObtainGIRPort(msg->gpi_GInfo);

	    if (ievent->ie_Code == SELECTUP)
	    {
		HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, SELECTUP, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
		retval = GMR_NOREUSE;
	    }
	    else
	    {
		if (ievent->ie_Code == SELECTDOWN)
		{
		    HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, SELECTDOWN, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
		}
		else
		    HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, NULL, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
	    }

	    if (rp)
		ReleaseGIRPort(rp);
	}
	else
	{
	    if (ievent->ie_Class == IECLASS_TIMER)
	    {
		struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);

		HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, NULL, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
		if (rp)
		    ReleaseGIRPort(rp);
	    }
	}
    }

    ReleaseSemaphore(&(si->si_Lock));

    return retval;
}

STATIC VOID DT_Write(struct IClass * cl, struct Gadget * g, struct dtWrite * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    D(bug("text.datatype/DT_Write: %ld\n", msg->dtw_Mode));

    if (msg->dtw_Mode == DTWM_RAW && msg->dtw_FileHandle)
    {
	Write(msg->dtw_FileHandle, td->buffer_allocated, td->buffer_allocated_len);
    }
}

STATIC VOID DT_Print(struct IClass *cl, struct Gadget *g, struct dtPrint *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    PrintText(td, msg->dtp_PIO);
}

STATIC LONG strseg(struct Line *line, STRPTR str, LONG slen)
{
    STRPTR text = line->ln_Text;
    STRPTR ptr;
    LONG llen = line->ln_TextLen;
    LONG len;

    while (llen >= slen)
    {
	ptr = str;
	len = slen;

	while (ToLower(*text) == ToLower(*ptr))
	{
	    text++;
	    ptr++;
	    llen--;
	    len--;
	}

	if (len==0)
	    return text - line->ln_Text - slen;
	else
	{
	    text++;
	    llen--;
	}
    }
    return -1;
}

STATIC VOID DT_SearchString(Class *cl,Object *obj,LONG direction,struct dttSearchText *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, obj);
    struct List *list;

    if (GetAttr(TDTA_LineList,obj,(ULONG *) &list))
    {
	struct Line *line;
	LONG len = msg->dttst_TextLen;
	LONG found = td->search_line;
	LONG found_x = 0;
	LONG y = 0;

	line = (struct Line *)list->lh_Head;

	if (found == -1 && direction == -1)
	{
	  GetAttr(DTA_TotalVert,obj,(ULONG*)&found);
	}

	if (direction == -1)
	    found--;

	/* Find the correct line node */
	while (y <= found && line->ln_Link.mln_Succ)
	{
	    if (line->ln_Flags & LNF_LF)
		y++;
	    line = (struct Line *) line->ln_Link.mln_Succ;
	}

	found = -1;

	if (direction == -1)
	{
	    if (line->ln_Link.mln_Pred)
		line = (struct Line *) line->ln_Link.mln_Pred;

	    while (line->ln_Link.mln_Pred && found == -1)
	    {
		if (line->ln_Flags & LNF_LF)
		    y--;

		if (line->ln_TextLen >= len)
		{
		    found_x = strseg(line,msg->dttst_Text,msg->dttst_TextLen);
		    if (found_x >= 0)
		    {
			found = y;
			break;
		    }
		}
		line = (struct Line *) line->ln_Link.mln_Pred;
	     }
	} else
	{
	    while (line->ln_Link.mln_Succ && found == -1)
	    {
		if (line->ln_TextLen >= len)
		{
		    found_x = strseg(line,msg->dttst_Text,msg->dttst_TextLen);
		    if (found_x >= 0)
		    {
			found = y;
			break;
		    }
		}
		if (line->ln_Flags & LNF_LF)
		    y++;
		line = (struct Line *) line->ln_Link.mln_Succ;
	    }
	}

	td->search_line = found;

	if (found < 0)
	{
	    DisplayBeep(msg->dttst_GInfo->gi_Screen);
/*          found = 0;*/
	} else
	{
	    struct RastPort *rp = ObtainGIRPort(msg->dttst_GInfo);
	    if (td->mark_line1 && rp)
	    {
		DrawMarkedText(td,rp,FALSE);
		td->mark_line1 = td->mark_line2 = NULL;
	    }

	    notifyAttrChanges(obj,msg->dttst_GInfo,0,DTA_TopVert,found,TAG_DONE);

	    td->mark_line1 = td->mark_line2 = line;
	    td->mark_x1 = found_x + line->ln_XOffset / td->font->tf_XSize;
	    td->mark_x2 = td->mark_x1 + msg->dttst_TextLen;
	    td->mark_y1 = td->mark_y2 = found;

	    if (rp)
	    {
		DrawMarkedText(td,rp,TRUE);
		ReleaseGIRPort(rp);
	    }
	}
    }
}

STATIC VOID DT_GetString(Class *cl,Object *g,struct dttGetString *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct Library *ReqToolsBase;

    if ((ReqToolsBase = OpenLibrary("reqtools.library",38)))
    {
	struct GadgetInfo *ginfo = &msg->dttgs_GInfo;

	/*
	 *  SAS/C doesn't like non-constant data in initialisation.
	 *  So we have to add ginfo->gi_Window later.
	 */
	struct TagItem tags[] =
	{
	    {RT_Window          , 0                                     },
	    {RT_ReqPos          , REQPOS_CENTERWIN                      },
	    {RT_LockWindow      , TRUE                                  },
	    {RT_Underscore      , '_'                                   },
	    {RTGS_GadFmt        , (ULONG) "_Ok|_From Top|_From Bottom|_Cancel"  },
	    {TAG_DONE                                                   }
	};        
	
	ULONG retval = rtGetStringA(td->search_buffer,sizeof(td->search_buffer),
				   "Text Datatype Search String", NULL, tags);

	tags[0].ti_Data=(IPTR) ginfo->gi_Window;
				 
	CloseLibrary(ReqToolsBase);

	if (retval)
	{
	    ULONG search_method;
	    Forbid();

	    if (retval == 1 || retval == 2)
	    {
		search_method = DTTM_SEARCH_NEXT;
	    }   else /* 3 */
	    {
		search_method = DTTM_SEARCH_PREV;
	    }

	    if (retval == 2 || retval == 3)
	    {
		td->search_line = -1;
	    }

	    DoMethod(g,search_method,ginfo,td->search_buffer,strlen(td->search_buffer));
	    Permit();
	}
    }

    if (FindTask(NULL) == (struct Task*)td->search_proc)
	td->search_proc = NULL;
}

#ifdef _AROS
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(STACKULONG *, msg, A1))
{
    AROS_USERFUNC_INIT
#else
ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 LONG * msg)
{
#endif
    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);        /* Small Data */

    switch (*msg)
    {
    case OM_NEW:
	D(bug("text.datatype/DT_Dispatcher: Method OM_NEW\n"));
	return (ULONG) DT_NewMethod(cl, o, (struct opSet *) msg);

    case OM_DISPOSE:
	D(bug("text.datatype/DT_Dispatcher: Method OM_DISPOSE\n"));
	DT_DisposeMethod(cl, o, (Msg) msg);
	break;

    case OM_UPDATE:
	D(bug("text.datatype/DT_Dispatcher: Method OM_UPDATE\n"));

    case OM_SET:
	if (*msg == OM_SET)
	    D(bug("text.datatype/DT_Dispatcher: Method OM_SET\n"));
	return DT_SetMethod(cl, (struct Gadget *) o, (struct opSet *) msg);

    case OM_GET:
	D(bug("text.datatype/DT_Dispatcher: Method OM_GET\n"));
	return DT_GetMethod(cl, (struct Gadget *) o, (struct opGet *) msg);

    case GM_RENDER:
	D(bug("text.datatype/DT_Dispatcher: Method GM_RENDER\n"));
	return DT_Render(cl, (struct Gadget *) o, (struct gpRender *) msg);

    case GM_LAYOUT:
	{
	    struct Gadget *g = (struct Gadget *) o;
	    ULONG retval = 0;

	    D(bug("text.datatype/DT_Dispatcher: Method GM_LAYOUT\n"));

	    notifyAttrChanges(o, ((struct gpLayout *) msg)->gpl_GInfo, NULL,
			      GA_ID, g->GadgetID,
			      DTA_Busy, TRUE,
			      TAG_DONE);

	    retval += DoSuperMethodA(cl, o, (Msg) msg);
	    return retval;
	}
	break;

    case GM_GOACTIVE:
	D(bug("text.datatype/DT_Dispatcher: Method GM_GOACTIVE\n"));
	return (ULONG) DT_HandleInputMethod(cl, (struct Gadget *) o, (struct gpInput *) msg);

    case GM_HANDLEINPUT:
	D(bug("text.datatype/DT_Dispatcher: Method GM_HANLDEINPUT\n"));
	return (ULONG) DT_HandleInputMethod(cl, (struct Gadget *) o, (struct gpInput *) msg);

    case DTM_CLEARSELECTED:
	{
	    struct RastPort *rp;
	    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

	    D(bug("text.datatype/DT_Dispatcher: Method DTM_CLEARSELECTED\n"));

	    rp = ObtainGIRPort(((struct dtGeneral *) msg)->dtg_GInfo);
	    ClearSelected(td, rp);
	    if (rp)
		ReleaseGIRPort(rp);

	    return 1;
	}

    case DTM_COPY:
	{
	    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

	    D(bug("text.datatype/DT_Dispatcher: Method DTM_COPY\n"));

	    CopyText(td);
	    return 1;
	}

    case DTM_SELECT:
	{
	    D(bug("text.datatype/DT_Dispatcher: Method DTM_SELECT\n"));
	}
	break;

    case DTM_WRITE:
	{
	    D(bug("text.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
	    DT_Write(cl, (struct Gadget *) o, (struct dtWrite *) msg);
	}
	break;

    case DTM_PRINT:
	{
	    D(bug("text.datatype/DT_Dispatcher: Method DTM_PRINT\n"));
	    DT_Print(cl, (struct Gadget *) o, (struct dtPrint *) msg);
	}
	break;

    case DTM_TRIGGER:
	{
	    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);
	    ULONG function = ((struct dtTrigger*)msg)->dtt_Function;

	    D(bug("text.datatype/DT_Dispatcher: Method DTM_TRIGGER\n"));

	    if (function == STM_ACTIVATE_FIELD)
	    {
		if (!td->search_proc)
		{
		    td->msg.MethodID           = DTTM_GET_STRING;
		    /* copy the full GadgetInfo, because this request is asyncron and datatypes
		     * creates this structure on the stack (Oh No) !
		     */
		    td->msg.dttgs_GInfo        = *((struct dtTrigger*)msg)->dtt_GInfo;
/*                  td->msg.dttgs_SearchMethod = DTTM_SEARCH_NEXT;*/

		    td->search_proc = DoAsyncMethod(cl,o,(Msg) &td->msg,NP_Name,"text.datatype getstring process",TAG_DONE);
		}
	    } else
	    {
		ULONG search_method;

		if (function == STM_PREV_FIELD || function == STM_BROWSE_PREV)
		    search_method = DTTM_SEARCH_PREV;
		else search_method = DTTM_SEARCH_NEXT;

		if (td->search_buffer[0])
		{
		    DoMethod(o,search_method, ((struct dtTrigger*)msg)->dtt_GInfo,
			     td->search_buffer,strlen(td->search_buffer));
		}
	    }
	}
	break;

	case DTTM_GET_STRING:
		D(bug("text.datatype/DT_Dispatcher: Method DTTM_GET_STRING\n"));
		DT_GetString(cl,o,(struct dttGetString *) msg);
		break;

	case DTTM_SEARCH_NEXT:
		D(bug("text.datatype/DT_Dispatcher: Method DTTM_SEARCH_NEXT\n"));
		DT_SearchString(cl,o,1,(struct dttSearchText *) msg);
		break;

	case DTTM_SEARCH_PREV:
		D(bug("text.datatype/DT_Dispatcher: Method DTTM_SEARCH_PREV\n"));
		DT_SearchString(cl,o,-1,(struct dttSearchText *) msg);
		break;

    default:
	D(bug("text.datatype/DT_Dispatcher: Method %ld=0x%lx\n", *msg, *msg));
	return DoSuperMethodA(cl, o, (Msg) msg);
    }

    return 0;
#ifdef _AROS
    AROS_USERFUNC_EXIT
#endif
}

struct IClass *DT_MakeClass(struct Library *textbase)
{
    struct IClass *cl = MakeClass("text.datatype", DATATYPESCLASS, NULL, sizeof(struct Text_Data), NULL);

    if (cl)
    {
#ifdef _AROS
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
	cl->cl_UserData = (IPTR) textbase;     /* Required by datatypes */
    }

    return cl;

}

#else

/**************************************************************************
 The following functions are for the non datatype interface
 (used only for debugging)
**************************************************************************/

APTR Text_Create(void)
{
    struct Text_Data *hd = (struct Text_Data *) AllocVec(sizeof(*hd), 0x10000);

    if (hd)
    {
	return hd;
    }

    return NULL;
}

VOID Text_SetFrameBox(APTR mem, struct Screen * scr, struct RastPort * rp, LONG left, LONG top, LONG width, LONG height)
{
    struct Text_Data *hd = (struct Text_Data *) mem;

    if (hd)
    {
	hd->left = left;
	hd->top = top;
	hd->width = width;
	hd->height = height;
	hd->screen = scr;
	hd->rp = rp;
    }
}

VOID Text_Load(APTR mem, STRPTR filename)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	BPTR file = Open(filename, MODE_OLDFILE);

	if (file)
	{
	    InitText(td);
	    if (LoadText(td, filename, file))
	    {
		LoadAsAscII(td);
		td->vert_visible = td->height / td->font->tf_YSize;
		td->horiz_visible = td->width / td->font->tf_XSize;
	    }
	    Close(file);
	}
    }
}

VOID Text_Redraw(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	DrawText(td, td->rp);
    }
}

VOID Text_Free(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	DisposeText(td);
	FreeVec(td);
    }
}

ULONG Text_PageHeight(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;
    return 0;
}

ULONG Text_PageWidth(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;
    return 0;
}

ULONG Text_VisibleHeight(APTR mem)
{
    struct Text_Data *ht = (struct Text_Data *) mem;
    return 0;
}

ULONG Text_VisibleTop(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
	return (ULONG) td->vert_top;

    return 0;
}

ULONG Text_VisibleHoriz(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
	return (ULONG) td->horiz_top;

    return 0;
}

VOID Text_SetVisibleTop(APTR mem, ULONG newy)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	SetTopText(td, td->rp, newy);
    }
}

VOID Text_SetVisibleLeft(APTR mem, ULONG newx)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	SetTopHorizText(td, td->rp, newx);
    }
}

VOID Text_HandleMouse(APTR mem, LONG x, LONG y, LONG code, ULONG secs, ULONG mics)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	HandleMouse(td, td->rp, x - td->left, y - td->top, code, secs, mics);
    }
}

VOID Text_Print(APTR mem)
{
    struct Text_Data *td = (struct Text_Data *) mem;

    if (td)
    {
	struct MsgPort *printer_port;
	union printerIO *printer_io;

	if ((printer_port = CreateMsgPort()))
	{
	    if ((printer_io = (union printerIO *) CreateIORequest(printer_port, sizeof(union printerIO))))
	    {
		if (!(OpenDevice("printer.device", 0, (struct IORequest *) printer_io, 0)))
		{
		    PrintText(td, printer_io);
		    CloseDevice((struct IORequest *) printer_io);
		}
		DeleteIORequest((struct IORequest *) printer_io);
	    }
	    DeleteMsgPort(printer_port);
	}
    }
}

#endif

/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
/**************************************************************************************************/
