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

#if !defined(__AROS__) || defined(__MORPHOS__)
#include "libdefs.h"
#endif

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

#ifdef MORPHOS_AG_EXTENSION
#include "agextension.h"
#endif

/* Define the following to enable the debug version */
//#define MYDEBUG
#include "debug.h"

#ifndef __varargs68k
#define __varargs68k
#endif

#ifdef __AROS__
#define NO_PRINTER 1
#define NO_GMORE_SCROLLRASTER 1
#else
#define NO_PRINTER 0
#define NO_GMORE_SCROLLRASTER 1
#endif

/* 17 is reserved for help */
#define STM_SEARCH 18
#define STM_SEARCH_NEXT 19
#define STM_SEARCH_PREV 20

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
	    return (int) (line->ln_XOffset - prev_line->ln_XOffset - prev_line->ln_Width/*fontx * prev_line->ln_TextLen*/);
	}
    }

    return line->ln_XOffset;
}

/**************************************************************************
 ...
**************************************************************************/
static int GetLineStartX(struct Line *line)
{
    struct Line *prev_line = (struct Line *) Node_Prev(line);
    if (prev_line)
    {
	if (!(prev_line->ln_Flags & LNF_LF))
	{
	    return (int)(prev_line->ln_XOffset + prev_line->ln_Width);
	}
    }
    return 0;
}

/**************************************************************************
 Returns the x pos of the line start
**************************************************************************/
static int GetLineCharX(struct Line *line)
{
  int x = 0;
  if (!line) return 0;
  while ((line = (struct Line*)Node_Prev(&line->ln_Link)))
  {
    if (line->ln_Flags & LNF_LF) break;
    x += line->ln_TextLen;
  }
  return x;
}

/**************************************************************************

**************************************************************************/
struct Line *FindLineChar(struct Line *line, int tofind)
{
  int x = 0;
  while (line)
  {
    if (x + line->ln_TextLen >= tofind) return line;
    x += line->ln_TextLen;
    if (line->ln_Flags & LNF_LF) return (struct Line*)Node_Next(&line->ln_Link);
    line = (struct Line*)Node_Next(&line->ln_Link);
  }
  return NULL;
}

/**************************************************************************
 
**************************************************************************/
struct Line *GetNextLine(struct Line *line)
{
  while (line)
  {
    int lf = (line->ln_Flags & LNF_LF)?1:0;
    line = (struct Line*)Node_Next(&line->ln_Link);
    if (lf) return line;
  }
  return NULL;
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
		    if (td->mark_x1 > td->mark_x2)
		    {
			*mark_x1 = td->mark_x2;
			*mark_x2 = td->mark_x1;
			*mark_line1 = td->mark_line2;
			*mark_line2 = td->mark_line1;
		     }
		     else
		     {
			*mark_x1 = *mark_x2 = td->mark_x1;
			*mark_line1 = td->mark_line1;
			*mark_line2 = td->mark_line2;

			if (td->mark_line1 != td->mark_line2)
			{
			    if (Node_Prev(td->mark_line1) == (struct MinNode*)td->mark_line2)
			    {
				*mark_line1 = td->mark_line2;
				*mark_line2 = td->mark_line1;
			    }
			}
		     }
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
    /* initialize the line list */
    NewList(&td->line_list);

    /* the delimers */
    td->word_delim = "\t *-,()<>[];\"";

#ifdef MORPHOS_AG_EXTENSION
       td->links = -1;
       td->shinepen  = 2;
       td->shadowpen = 1;
       td->marked_line = NULL;
#endif

    /* If no font is set use the default (fixed) font */
    if (!td->attr.ta_Name)
    {
	struct TextFont *font = GfxBase->DefaultFont;

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

	InitRastPort(&td->font_rp);

	if ((td->font = OpenFont(&td->attr)))
	    return 1;

	FreeVec(td->attr.ta_Name);

	/* The opening has failed, so try to open the topaz font */
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
 Loads the whole file in the buffer
**************************************************************************/
static int LoadText(struct Text_Data *td, STRPTR filename, BPTR file)
{
    if ((td->title = StrCopy(filename)))
    {
	D(bug("text.datatype/LoadText: Get the file size\n"));
	if ((td->buffer_allocated_len = GetFileSize(file)) >= 0)
	{
	    if ((td->buffer_allocated = AllocVec(td->buffer_allocated_len + 1, MEMF_PUBLIC)))
	    {
		D(bug("text.datatype/LoadText: Buffer allocated at 0x%lx now reading in the file\n",td->buffer_allocated));
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

    return 0;
}

/**************************************************************************
 Loads the text as an IFF FTXT text file
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
 Free all needed resources
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
 Draw the text
**************************************************************************/
static void DrawText(struct Text_Data *td, struct RastPort *rp)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
#ifdef MORPHOS_AG_EXTENSION
       LONG fonty = td->vert_unit;
       LONG abs_y_line = 0;
#else
    LONG fonty = td->font->tf_YSize;
#endif
    LONG baseline = td->font->tf_Baseline;

    LONG linenum = 0;
    LONG minlinenum = td->vert_top;
    LONG maxlinenum = td->vert_top + td->vert_visible;
    LONG y_line = 0;

    ULONG apen, bpen, mode;
    ULONG fillpen, filltextpen;
    struct TextFont *oldfont;

    struct Region *new_region;
    struct Region *old_region;
    struct Rectangle rect;

    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;
    LONG curlinelen = 0;

    fillpen = td->fillpen;
    filltextpen = td->filltextpen;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

    D(bug("text.datatype/DrawText\n"));

    if (!(new_region = NewRegion()))
	return;

#ifdef MORPHOS_AG_EXTENSION
	PrepareAGExtension(td);
	/* if sub-class setup wrong vert unit adjust here at least to font height */
	if (fonty < td->font->tf_YSize)
            fonty = td->font->tf_YSize;
#endif

    if (td->use_vert_diff)
    {
	if (td->vert_diff > 0) minlinenum = maxlinenum - td->vert_diff;
	else maxlinenum = minlinenum - td->vert_diff;
	td->use_vert_diff = FALSE;
    }

    rect.MinX = td->left;
    rect.MinY = td->top;
    rect.MaxX = td->left + td->width - 1;
    rect.MaxY = td->top + td->height - 1;

    if (td->use_horiz_diff)
    {
	if (td->horiz_diff > 0)
	{
	    rect.MinX = rect.MaxX - td->horiz_diff * td->horiz_unit;
	} else
	{
	    rect.MaxX = rect.MinX - td->horiz_diff * td->horiz_unit;
	}
	if (rect.MinX < td->left) rect.MinX = td->left;
	if (rect.MaxX > td->left + td->width - 1) rect.MaxX = td->left + td->width - 1;
	td->use_horiz_diff = FALSE;
    }

    OrRectRegion(new_region,&rect);
    old_region = installclipregion(rp->Layer,new_region);

    GetRPAttrs(rp,
	       RPTAG_APen, (IPTR) &apen,
	       RPTAG_BPen, (IPTR) &bpen,
	       RPTAG_DrMd, (IPTR) &mode,
	       RPTAG_Font, (IPTR) &oldfont,
	       TAG_DONE);

    SetFont(rp, td->font);

#ifdef MORPHOS_AG_EXTENSION
	/* clear last marked line (render in non marked state) */
	if (td->last_marked_line != NULL)
	{
            DrawLineBevel(td, rp, td->last_marked_line, TRUE);
            td->last_marked_line = NULL;
	}
#endif
    D(bug("text.datatype/DrawText: Start Loop\n"));

    while (line)
    {
	if (mark_line1 == line)
	    mark = TRUE;

	if (linenum >= td->vert_top)
	{
	    /* if end of page - no longer draw */
	    if (linenum >= maxlinenum)
		break;

	    if (linenum >= minlinenum)
	    {
		STRPTR text = line->ln_Text;
		LONG len = line->ln_TextLen;

		if (len > 0)
		{
		    LONG x_text = line->ln_XOffset - td->horiz_top * td->horiz_unit;
/*		    LONG width = td->width - x_text;*/

#ifdef MORPHOS_AG_EXTENSION
                    /* remember yoffset for rendering link boxes correctly */
                    line->ln_YOffset = abs_y_line;
                    /* clear possible link box rests */
                    if (fonty > td->font->tf_YSize)
                	EraseRect(rp, td->left + x_text, td->top + y_line + fonty-1,
                                      td->left + td->width - 1, td->top + y_line + fonty-1);
                    /* add some space to the link box */
                    if (line->ln_Flags & LNF_LINK)
                	x_text += AG_LINK_ADDWIDTH;
#endif
		    Move(rp, td->left + x_text, td->top + y_line + baseline);
		    SetSoftStyle(rp, line->ln_Style, AskSoftStyle(rp));

		    if (mark)
		    {
			BOOL mark_start = FALSE, mark_end = FALSE;
			BOOL draw_rect = FALSE;

			if (mark_line1 == line) mark_start = TRUE;
			if (mark_line2 == line) mark_end = TRUE;

			if (mark_start)
			{
			    LONG start_len = mark_x1 - curlinelen - 1;

			    if (start_len > 0)
			    {
			        SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
			        Text(rp, text, start_len);
			        text += start_len;
			    } else
			    {
			    	if (start_len < 0) draw_rect = TRUE;
			    }

			    D(bug("start_len: %ld\n",start_len));

			    if (mark_end)
			    {
			    	len = mark_x2 - mark_x1;
			    	if (start_len < 0) len--;
			    }
			    else
			    {
			    	len -= mark_x1 - curlinelen - 1;
			    	if (start_len < 0) len--;
			    }
			}   else
			{
			    draw_rect = TRUE;
			    if (mark_end)
			    {
				len = mark_x2 - curlinelen - 1;
			    }
			}

			if (len > 0)
			{
			    SetABPenDrMd(rp, filltextpen, fillpen, JAM2);
			    Text(rp, text, len);
			}

			{
			    LONG x1 = GetLineStartX(line) + td->left - td->horiz_top * td->horiz_unit;
			    LONG x2 = line->ln_XOffset - 1 + td->left - td->horiz_top * td->horiz_unit;

			    if (x1 <= x2)
			    {
				SetAPen(rp,draw_rect?fillpen:line->ln_BgPen);
				RectFill(rp,x1,td->top + y_line,x2,td->top + y_line + fonty - 1);
			    }
			}

			if (mark_end)
			{
			    LONG len_sub = mark_x2 - curlinelen - 1;
//			    if (mark_start && draw_rect) len_sub++; /* if marking starts on a tab */

			    if (len_sub < 0) len_sub = 0;

			    text = line->ln_Text + len_sub;
			    len = line->ln_TextLen - len_sub;

			    if (len > 0)
			    {
				SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
				Text(rp, text, len);
			    }
			}
		    } else
		    {
//			if (width > 0)
			{
			    {
				LONG x1 = GetLineStartX(line) + td->left - td->horiz_top * td->horiz_unit;
				LONG x2 = line->ln_XOffset - 1 + td->left - td->horiz_top * td->horiz_unit;

				if (x1 <= x2)
				{
				    SetAPen(rp,line->ln_BgPen);
				    RectFill(rp,x1,td->top + y_line,x2,td->top + y_line + fonty - 1);
				}
			    }

#ifdef MORPHOS_AG_EXTENSION
                            /* change the background color if the line is marked */
                            {
                                UBYTE bpen = line->ln_BgPen;
                                if (line->ln_Flags & LNF_MARKED)
                                    bpen = td->fillpen;
                                SetABPenDrMd(rp, line->ln_FgPen, bpen, JAM2);
                            }
#else

			    SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
#endif
			    Text(rp, text, len);
			}
		    } /* if(width > 0) */
		} /* if(len > 0) */

#ifdef MORPHOS_AG_EXTENSION
                if (line->ln_Flags & LNF_LINK)
                    DrawLineBevel(td, rp, line, FALSE);
#endif

		/* Clear the right of the line */
		if (line->ln_Flags & LNF_LF)
		{
		    LONG x1 = td->left + line->ln_XOffset + line->ln_Width - td->horiz_top * td->horiz_unit;
		    LONG y1 = td->top + y_line;

		    if (mark_line2 == line) mark = FALSE;

//		    D(bug("x1: %ld  x2: %ld\n",x1,td->width + td->left - 1));

		    SetAPen(rp,mark?fillpen:line->ln_BgPen);
		    if (x1 <= td->width + td->left)
			RectFill(rp, x1, y1, td->width + td->left - 1, y1 + fonty - 1);
		}
	    } /* if(linenum >= minlinenum) */

	    if (line->ln_Flags & LNF_LF)
	    {
		if (mark_line2 == line) mark = FALSE;
		y_line += fonty;
	    }
	} /* if(linenum >= td->vert_top) */

	if (mark_line2 == line)
	{
	    mark = FALSE;
	}

	if (line->ln_Flags & LNF_LF)
	{
	    linenum++;
	    curlinelen = 0;
#ifdef MORPHOS_AG_EXTENSION
            abs_y_line += fonty;
#endif
	} else
	{
	    curlinelen += line->ln_TextLen + 1;
	}

	line = (struct Line *) Node_Next(line);
    } /* while(line) */

    /* erase the last part */
/*    y_line = fonty;
    SetAPen(rp, 0);
    if (y_line <= td->top + td->height - 1)
    {
    	RectFill(rp,td->left, y_line, td->left + td->width, td->top + td->height - 1);
    }*/

    SetABPenDrMd(rp, apen, bpen, mode);
    SetFont(rp, oldfont);

    installclipregion(rp->Layer,old_region);
    DisposeRegion(new_region);
    D(bug("text.datatype/DrawText: finished\n"));
}

/**************************************************************************
 Draw the text between the two markpoints
**************************************************************************/
static void DrawMarkedText(struct Text_Data *td, struct RastPort *rp, LONG marked)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
#ifdef MORPHOS_AG_EXTENSION
        LONG fonty = td->vert_unit;
        LONG abs_y_line = 0;
#else
    LONG fonty = td->font->tf_YSize;
#endif
    LONG baseline = td->font->tf_Baseline;

    LONG linenum = 0;
    LONG minlinenum = td->vert_top;
    LONG maxlinenum = td->vert_top + td->vert_visible;
    LONG y_line = 0;

    ULONG apen, bpen, mode;
    ULONG fillpen, filltextpen;
    struct TextFont *oldfont;

    struct Region *new_region;
    struct Region *old_region;
    struct Rectangle rect;

    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;
    LONG curlinelen = 0;

    fillpen = td->fillpen;
    filltextpen = td->filltextpen;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

    if (!(new_region = NewRegion()))
	return;

#ifdef MORPHOS_AG_EXTENSION
	PrepareAGExtension(td);
	/* if sub-class setup wrong vert unit adjust here at least to font height */
	if (fonty < td->font->tf_YSize)
            fonty = td->font->tf_YSize;
#endif

    rect.MinX = td->left;
    rect.MinY = td->top;
    rect.MaxX = td->left + td->width - 1;
    rect.MaxY = td->top + td->height - 1;

    OrRectRegion(new_region,&rect);
    old_region = installclipregion(rp->Layer,new_region);

    GetRPAttrs(rp,
	       RPTAG_APen, (IPTR) &apen,
	       RPTAG_BPen, (IPTR) &bpen,
	       RPTAG_DrMd, (IPTR) &mode,
	       RPTAG_Font, (IPTR) &oldfont,
	       TAG_DONE);

    SetFont(rp, td->font);

    D(bug("mark_line1 %lx  mark_line2 %lx\n",mark_line1,mark_line2));

    while (line)
    {
	if (mark_line1 == line)
	{
	    mark = TRUE;
	    D(bug("line: %ld   mark: %ld   lineptr: %lx\n",linenum,mark,line));
	}

	if (linenum >= td->vert_top)
	{
	    /* if end of page - no longer draw */
	    if (linenum >= maxlinenum)
		break;

	    if (linenum >= minlinenum)
	    {
		STRPTR text = line->ln_Text;
		LONG len = line->ln_TextLen;

		if (len > 0)
		{
		    LONG x_text = line->ln_XOffset - td->horiz_top * td->horiz_unit;
/*		    LONG width = td->width - x_text;*/

#ifdef MORPHOS_AG_EXTENSION
                    /* remember yoffset for rendering link boxes correctly */
                    line->ln_YOffset = abs_y_line;
                    /* clear possible link box rests */
                    if (fonty > td->font->tf_YSize)
                	EraseRect(rp, td->left + x_text, td->top + y_line + fonty-1,
                                      td->left + td->width - 1, td->top + y_line + fonty-1);
                    if (line->ln_Flags & LNF_LINK)
                	x_text += AG_LINK_ADDWIDTH;
#endif
		    Move(rp, td->left + x_text, td->top + y_line + baseline);
		    SetSoftStyle(rp, line->ln_Style, AskSoftStyle(rp));

		    if (mark)
		    {
			BOOL mark_start = FALSE, mark_end = FALSE;
			BOOL draw_rect = FALSE;

			if (mark_line1 == line) mark_start = TRUE;
			if (mark_line2 == line) mark_end = TRUE;

			if (mark_start)
			{
			    LONG start_len = mark_x1 - curlinelen - 1;

			    if (start_len > 0)
			    {
//			        SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
//			        Text(rp, text, start_len);
				Move(rp, rp->cp_x+TextLength(rp,text,start_len),rp->cp_y);
			        text += start_len;
			    } else
			    {
			    	if (start_len < 0) draw_rect = TRUE;
			    }

			    if (mark_end)
			    {
			    	len = mark_x2 - mark_x1;
			    	if (start_len < 0) len--;
			    }
			    else
			    {
			    	len -= mark_x1 - curlinelen - 1;
			    	if (start_len < 0) len--;
			    }
			}   else
			{
			    draw_rect = TRUE;
			    if (mark_end)
			    {
				len = mark_x2 - curlinelen - 1;
			    }
			}

			if (len > 0)
			{
			    if (marked) SetABPenDrMd(rp, filltextpen, fillpen, JAM2);
			    else  SetABPenDrMd(rp, line->ln_FgPen, line->ln_BgPen, JAM2);
			    Text(rp, text, len);
			}

			{
			    LONG x1 = GetLineStartX(line) + td->left - td->horiz_top * td->horiz_unit;
			    LONG x2 = line->ln_XOffset - 1 + td->left - td->horiz_top * td->horiz_unit;

			    if (x1 <= x2)
			    {
			    	if (draw_rect)
			    	{
				    SetAPen(rp,marked?3:line->ln_BgPen);
				    RectFill(rp,x1,td->top + y_line,x2,td->top + y_line + fonty - 1);
				}
			    }
			}
		    }
		} /* if(len > 0) */

#ifdef MORPHOS_AG_EXTENSION
        	if (line->ln_Flags & LNF_LINK)
                    DrawLineBevel(td, rp, line, FALSE);
#endif

		/* Clear the right of the line */
		if (line->ln_Flags & LNF_LF)
		{
		    LONG x1 = td->left + line->ln_XOffset + line->ln_Width - td->horiz_top * td->horiz_unit;
		    LONG y1 = td->top + y_line;

		    if (mark_line2 == line) mark = FALSE;

		    if (mark)
		    {
			SetAPen(rp,marked?3:line->ln_BgPen);
			if (x1 < td->width + td->left)
			    RectFill(rp, x1, y1, td->width + td->left - 1, y1 + fonty - 1);
		    }
		}
	    } /* if(linenum >= minlinenum) */

	    if (line->ln_Flags & LNF_LF)
	    {
		if (mark_line2 == line) mark = FALSE;
		y_line += fonty;
	    }
	} /* if(linenum >= td->vert_top) */

	if (mark_line2 == line)
	{
	    mark = FALSE;
	    D(bug("line: %ld   mark: %ld   lineptr: %lx\n",linenum,mark,line));
	}

	if (line->ln_Flags & LNF_LF)
	{
	    linenum++;
	    curlinelen = 0;
#ifdef MORPHOS_AG_EXTENSION
            abs_y_line += fonty;
#endif
	} else
	{
	    curlinelen += line->ln_TextLen + 1;
	}

	line = (struct Line *) Node_Next(line);
    } /* while(line) */

    SetABPenDrMd(rp, apen, bpen, mode);
    SetFont(rp, oldfont);

    installclipregion(rp->Layer,old_region);
    DisposeRegion(new_region);
}

/**************************************************************************
 Scroll the area vertical (depending on td->vert_diff)
**************************************************************************/
static void ScrollYText(struct Text_Data *td, struct RastPort *rp)
{
    LONG addx = td->left;
    LONG addy = td->top;
    LONG maxx = td->left + td->width - 1;
    LONG maxy = td->top + td->vert_visible * td->vert_unit - 1;//;td->height - 1;
    //struct Hook *old_hook;

/*
#ifdef 0 // is only called inside GM_RENDER
    LockLayerInfo (rp->Layer->LayerInfo);
#endif
*/
    if (addy <= maxy)
    {
	//old_hook = InstallLayerHook (rp->Layer, LAYERS_NOBACKFILL);
	ScrollRasterBF(rp, 0, td->vert_diff * td->vert_unit, addx, addy, maxx, maxy);
	//InstallLayerHook(rp->Layer,old_hook);
    }

/*
#ifdef 0
    UnlockLayerInfo(rp->Layer->LayerInfo);
#endif
*/
}

/**************************************************************************
 Scroll the area horizontal (depending on td->horiz_diff)
**************************************************************************/
static void ScrollXText(struct Text_Data *td, struct RastPort *rp)
{
    LONG addx = td->left;
    LONG addy = td->top;
    LONG maxx = td->left + td->width - 1;
    LONG maxy = td->top + td->height - 1;

    //struct Hook *old_hook;

/*
#ifdef 0
    LockLayerInfo(rp->Layer->LayerInfo);
#endif
*/
    //old_hook = InstallLayerHook (rp->Layer, LAYERS_NOBACKFILL);
    ScrollRasterBF(rp, td->horiz_diff *  td->horiz_unit, 0, addx, addy, maxx, maxy);
    //InstallLayerHook(rp->Layer,old_hook);

/*
#ifdef 0
    UnlockLayerInfo(rp->Layer->LayerInfo);
#endif
*/
}

/**************************************************************************
 ...
**************************************************************************/
static void SetTopText(struct Text_Data *td, struct RastPort *rp, LONG newy)
{
#if NO_GMORE_SCROLLRASTER
    BOOL refresh_old, refresh_new;
#endif

    if (newy < 0)
	newy = 0;

    if (rp)
    {
	if ((td->vert_diff = newy - td->vert_top))
	{
	#if NO_GMORE_SCROLLRASTER
	    refresh_old = (rp->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;
    	#endif
	
	    if (abs(td->vert_diff) < td->vert_visible)
	    {
		ScrollYText(td, rp);
		td->vert_top = newy;
		td->use_vert_diff = TRUE;
	    }
	    else
	    {
		EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		td->vert_top = newy;
	    }
	    DrawText(td, rp);

	#if NO_GMORE_SCROLLRASTER
	    if (!(rp->Layer->Flags & LAYERUPDATING))
	    {
		refresh_new = (rp->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;
		if (refresh_new)
		{
		    BOOL nocarerefresh = (((struct Window *)rp->Layer->Window)->Flags & WFLG_NOCAREREFRESH) ? TRUE : FALSE;

		    BeginUpdate(rp->Layer);
		    DrawText(td, rp);
		    EndUpdate(rp->Layer, !refresh_old || nocarerefresh);
    	    	}
	    }
	    
	#endif
	
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
#if NO_GMORE_SCROLLRASTER
    BOOL refresh_old, refresh_new;
#endif

    if (rp)
    {
	if ((td->horiz_diff = newx - td->horiz_top))
	{
	#if NO_GMORE_SCROLLRASTER
	    refresh_old = (rp->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;
    	#endif
	
	    if (abs(td->horiz_diff) < td->horiz_visible)
	    {
		ScrollXText(td, rp);
		td->horiz_top = newx;
		td->use_horiz_diff = TRUE;
	    }
	    else
	    {
		EraseRect(rp, td->left, td->top, td->left + td->width - 1, td->top + td->height - 1);
		td->horiz_top = newx;
	    }

	    DrawText(td, rp);
	    
	#if NO_GMORE_SCROLLRASTER
	    if (!(rp->Layer->Flags & LAYERUPDATING))
	    {
		refresh_new = (rp->Layer->Flags & LAYERREFRESH) ? TRUE : FALSE;
		if (refresh_new)
		{
		    BOOL nocarerefresh = (((struct Window *)rp->Layer->Window)->Flags & WFLG_NOCAREREFRESH) ? TRUE : FALSE;

		    BeginUpdate(rp->Layer);
		    DrawText(td, rp);
		    EndUpdate(rp->Layer, !refresh_old || nocarerefresh);
    	    	}
	    }
	    
	#endif
	
	}
    }
    else
	td->horiz_top = newx;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted.
*************************** ***********************************************/
#ifndef MORPHOS_AG_EXTENSION
static
#endif
struct Line *NewFindLine(struct Text_Data *td, LONG x, LONG y, LONG *xpos, LONG *line_xpos)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    LONG linenum = 0;

    LONG len = 0;
    LONG rel_len = 0; /* len within the line */

    while (line)
    {
    	struct Line *next_line = (struct Line*)Node_Next(line);

	if (y == linenum)
	{
	    LONG right_offset;

	    if (x >= GetLineStartX(line) && x < line->ln_XOffset)
	    {
	    	rel_len = -1;
		break;
	    }

	    len++;

	    if (next_line && !(line->ln_Flags & LNF_LF))
	    {
		right_offset = next_line->ln_XOffset;
	    } else right_offset = line->ln_XOffset + line->ln_Width;

	    if (x >= line->ln_XOffset && x < right_offset)
	    {
	    	rel_len = TextFit(&td->font_rp, line->ln_Text, line->ln_TextLen, &td->te, NULL, 1, x - line->ln_XOffset, td->font->tf_YSize);
	    	len += rel_len;
		break;
	    }   else
	    {
	    	len += line->ln_TextLen;
	    	rel_len = line->ln_TextLen; /* stimmt nicht ganz oder vielleicht doch? */
	    }
	}

	if (line->ln_Flags & LNF_LF)
	{
	    if (y == linenum) break;
	    linenum++;
	}
	line = next_line;
    }	/* while(line) */

    *xpos = len;
    if (line_xpos) *line_xpos = rel_len;
    return line;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted to the beginning of a word.
**************************************************************************/
#ifdef MORPHOS_AG_EXTENSION
static struct Line *NewFindWordBegin(struct Text_Data *td, LONG x, LONG y, LONG *xpos, LONG *lxpos)
#else
static struct Line *NewFindWordBegin(struct Text_Data *td, LONG x, LONG y, LONG *xpos)
#endif
{
    LONG line_xpos;
    struct Line *line = NewFindLine(td,x,y,xpos,&line_xpos);
#ifdef MORPHOS_AG_EXTENSION
        *lxpos = 0;
#endif
    if (!line) return NULL;

    if (line_xpos == -1) return line;
#ifdef MORPHOS_AG_EXTENSION
        *lxpos = line_xpos;
#endif
    if (stpchr(td->word_delim, line->ln_Text[line_xpos])) return line;


    D(bug("begin: %lc %ld %ld %ld\n",line->ln_Text[line_xpos],line->ln_Text[line_xpos],line_xpos,*xpos));

    if (line_xpos > 0)
    {
//    	line_xpos--;
    	while(line_xpos>=0)
    	{
	    D(bug("begin: %lc %ld %ld %ld\n",line->ln_Text[line_xpos],line->ln_Text[line_xpos],line_xpos,*xpos));
    	    if (stpchr(td->word_delim, line->ln_Text[line_xpos]))
    	    {
    	    	(*xpos)++;
                line_xpos++;
    	    	break;
    	    }
    	    if (line_xpos == 0) break;
    	    (*xpos)--;
    	    line_xpos--;
    	}
    }

#ifdef MORPHOS_AG_EXTENSION
        *lxpos = line_xpos;
#endif

    return line;
}

/**************************************************************************
 Returns the line at the specificed line number. The x coordinate will
 be adjusted to the end of a word.
**************************************************************************/
#ifdef MORPHOS_AG_EXTENSION
static struct Line *NewFindWordEnd(struct Text_Data *td, LONG x, LONG y, LONG *xpos, LONG *lxpos)
#else
static struct Line *NewFindWordEnd(struct Text_Data *td, LONG x, LONG y, LONG *xpos)
#endif
{
    LONG line_xpos;
    struct Line *line = NewFindLine(td,x,y,xpos,&line_xpos);
#ifdef MORPHOS_AG_EXTENSION
        *lxpos = 0;
#endif
    if (!line) return NULL;

    if (line_xpos == -1) return line;
#ifdef MORPHOS_AG_EXTENSION
        *lxpos = line_xpos;
#endif
    if (stpchr(td->word_delim, line->ln_Text[line_xpos])) return line;

    D(bug("end: %lc %ld %ld %ld\n",line->ln_Text[line_xpos],line->ln_Text[line_xpos],line_xpos,*xpos));

//    if (line_xpos > 0)
    {
//	if (stpchr(td->word_delim,line->ln_Text[line_xpos-1]))
//	    return line;

    	while(line_xpos < line->ln_TextLen)
    	{
    	    if (stpchr(td->word_delim, line->ln_Text[line_xpos]))
    	    {
    	    	break;
    	    }
    	    (*xpos)++;
    	    line_xpos++;
    	}
    }
#ifdef MORPHOS_AG_EXTENSION
        *lxpos = line_xpos;
#endif
    return line;
}

/**************************************************************************
 Handle Mouse Inputs especially the marking of the text
**************************************************************************/
static int HandleMouse(struct Text_Data *td, struct RastPort *rp, LONG x, LONG y, LONG code, ULONG secs, ULONG mics)
{
    LONG xcur;

    x += td->horiz_top * td->horiz_unit;
#ifdef MORPHOS_AG_EXTENSION
        y = y / td->vert_unit + td->vert_top;
#else
    y = y / td->font->tf_YSize + td->vert_top;
#endif

//    D(bug("x:%ld y:%ld\n",x,y));

    if (x < 0) x = 0;
    if (y < 0) y = 0;

#ifdef MORPHOS_AG_EXTENSION
        if (HandleMouseLink(td, rp, x, y, code))
            return 0;
#endif

    if (code == SELECTDOWN)
    {
	LONG old_dclick = td->doubleclick;

	ClearSelected(td, rp);
	td->mark_line1 = td->mark_line2 = NewFindLine(td, x, y, &xcur, NULL);
	td->mark_x1 = xcur;
	td->mark_x2 = xcur;
	td->mark_y1 = y;
	td->mark_y2 = y;
	td->pressed = TRUE;
	td->doubleclick = 0;

	if (DoubleClick(td->lastsecs, td->lastmics, secs, mics))
	{
	    if (old_dclick == 0)
	    {
		/* x wird an Grenzen angepaßt */
		LONG xstart;
		LONG xend;

#ifdef MORPHOS_AG_EXTENSION
        	LONG txtstart, txtend;
        	struct Line *newline = NewFindWordBegin(td, x, y, &xstart, &txtstart);
        	NewFindWordEnd(td, x, y, &xend, &txtend);

		if (newline != NULL) { /* if below all text, NewFindWordBegin returns \0 */
        	    if (newline->ln_Text[txtstart] != '\0')
        	    {
                        strcpy(td->word, "link ");
                        strncpy(&td->word[5], &newline->ln_Text[txtstart], txtend-txtstart);
                        td->word[5+txtend-txtstart] = 0;
                        D(bug("%ld %ld %s\n", txtstart, txtend, td->word));
                        TriggerWord(td, td->word);
        	    }
		}
#else
 
		struct Line *newline = NewFindWordBegin(td, x, y, &xstart);
		NewFindWordEnd(td, x, y, &xend);
#endif
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
		LONG xcur;

		td->doubleclick = 2;
		secs = mics = 0;

		td->mark_line1 = NewFindLine(td, x, y, &xcur,NULL);
		td->mark_x1 = xcur;
		td->mark_y1 = y;

		x = 0x7fffffff;
		td->mark_line2 = NewFindLine(td, x, y, &xcur,NULL);
		td->mark_x2 = xcur;
		td->mark_y2 = y;

		DrawMarkedText(td, rp, TRUE);
	    }
	} /* if (DoubleClick(td->lastsecs, td->lastmics, secs, mics)) */

	D(bug("mark_x1: %ld\n",td->mark_x1));

	td->lastsecs = secs;
	td->lastmics = mics;

    }	/* if (code == SELECTDOWN) */
    else
    {
	if (td->pressed)
	{
	    struct Line *newline = NewFindLine(td, x, y, &xcur, NULL);

	    struct Line *old_line = td->mark_line1;
	    LONG old_x = td->mark_x1;
	    LONG old_y = td->mark_y1;

	    BOOL mouse_up;
	    BOOL erase;
	    BOOL twotimes;

	    if (y < td->mark_y2 || (y == td->mark_y2 && (xcur < td->mark_x2 || (xcur == td->mark_x2 && Node_Prev(td->mark_line2) == (struct MinNode*)newline))))
		mouse_up = TRUE;
	    else
		mouse_up = FALSE;

	    if (td->doubleclick == 1)
	    {
		if ((td->mark_y1 == td->mark_y2 && td->mark_x1 > td->mark_x2) ||
		    (td->mark_y1 > td->mark_y2))
		{
#ifdef MORPHOS_AG_EXTENSION
                    LONG tmp;
                    NewFindWordBegin(td, x, y, &xcur, &tmp);
		    D(bug("Wordbegin  %ld  %ld  %ld\n",x,y,xcur));
#else
                    NewFindWordBegin(td, x, y, &xcur);
#endif
		}
		else
		{
#ifdef MORPHOS_AG_EXTENSION
                    LONG tmp;
                    NewFindWordEnd(td, x, y, &xcur, &tmp);
		    D(bug("Wordend  %ld  %ld  %ld\n",x,y,xcur));
#else
                    NewFindWordEnd(td, x, y, &xcur);
#endif

		}
	    }
	    else if (td->doubleclick == 2)
	    {
		if (td->mark_y1 > y)
		{
		    x = 0;
		    newline = NewFindLine(td, x, y, &xcur, NULL);
		}
		else
		{
		    x = 0x7fffffff;
		    newline = NewFindLine(td, x, y, &xcur, NULL);
		}
	    }

	    if (td->mark_x2 != xcur || td->mark_y2 != y || td->mark_line2 != newline)
	    {
		if (td->mark_y1 < td->mark_y2 || (td->mark_y1 == td->mark_y2 && (td->mark_x1 < td->mark_x2)))// || (td->mark_x1 == td->mark_x2 && Node_Prev(td->mark_line1) == (struct MinNode*)td->mark_line2))))
		{
		    if (!mouse_up)
			erase = TRUE;
		    else
			erase = FALSE;
	
		    if (td->mark_y1 < y || (td->mark_y1 == y && td->mark_x1 < xcur))
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
	
		    if (td->mark_y1 < y || (td->mark_y1 == y && td->mark_x1 < xcur))
			twotimes = TRUE;
		    else
			twotimes = FALSE;
		}

		D(bug("mouseup %ld  erase %ld  twotimes %ld   markx1: %ld  markx2: %ld x1: %ld   %lx  %lx\n",mouse_up,erase,twotimes,td->mark_x1,td->mark_x2,xcur,td->mark_line2,newline));
	
		if (twotimes)
		{
		    DrawMarkedText(td, rp, FALSE);
	
		    td->mark_x2 = xcur;
		    td->mark_y2 = y;
		    td->mark_line2 = newline;

		    DrawMarkedText(td, rp, TRUE);
		}
		else
		{
		    td->mark_x1 = td->mark_x2;
		    td->mark_y1 = td->mark_y2;
		    td->mark_line1 = td->mark_line2;
		    td->mark_x2 = xcur;
		    td->mark_y2 = y;
		    td->mark_line2 = newline;

		    DrawMarkedText(td, rp, erase);

		    td->mark_x1 = old_x;
		    td->mark_y1 = old_y;
		    td->mark_line1 = old_line;
		}

		td->mark_x2 = xcur;
		td->mark_y2 = y;
		td->mark_line2 = newline;
	    }

//	    DrawText(td, rp);

	    if (code == SELECTUP)
	    {
		td->pressed = FALSE;
		if (td->copy_text)
		{
		    CopyText(td);
		    ClearSelected(td,rp);
		}
	    }
	}	/* if(td->pressed) */

    }	/* if (code == SELECTDOWN) else ... */

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

    D(bug("text.datatype/ClearSelected: rp=0x%lx  %ld\n", rp, refresh));

    td->mark_x1 = -1;
    td->mark_x2 = -1;
    td->mark_y1 = -1;
    td->mark_y2 = -1;
    td->mark_line1 = td->mark_line2 = NULL;
    td->pressed = FALSE;

    if (rp && refresh)
	DrawText(td, rp);
}


struct CopyMsg
{
    struct Message cm_ExecMessage;
    ULONG cm_regA4;
    struct Text_Data *cm_TextData;
};

/**************************************************************************
 Move the selected text into the given iff
**************************************************************************/
static void CopyTextNowIFF(struct Text_Data *td, struct IFFHandle *iff)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;
    LONG curlinelen = 0;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);
    if (!mark_line1)
    {
    	mark_line1 = line;
    	mark_x1 = 0;
    }

    while (line)
    {
	STRPTR text = line->ln_Text;
	LONG len = line->ln_TextLen;

        if (mark_line1 == line)
	    mark = TRUE;

        if (mark)
        {
	     BOOL mark_start = FALSE, mark_end = FALSE;
	     BOOL draw_rect = FALSE;

	     if (mark_line1 == line) mark_start = TRUE;
	     if (mark_line2 == line) mark_end = TRUE;

	     if (mark_start)
	     {
		LONG start_len = mark_x1 - curlinelen - 1;

		if (start_len > 0)
		{
		   text += start_len;
	        } else
	        {
	    	    if (start_len < 0) draw_rect = TRUE;
	        }

	        if (mark_end)
	        {
	    	    len = mark_x2 - mark_x1;
		    if (start_len < 0) len--;
	        }
	        else
	        {
	    	    len -= mark_x1 - curlinelen - 1;
	    	    if (start_len < 0) len--;
	        }
	    }   else
	    {
	        draw_rect = TRUE;
	        if (mark_end)
	        {
		    len = mark_x2 - curlinelen - 1;
	        }
	    }
	    
	    if (draw_rect && (GetLineStartX(line) != line->ln_XOffset))
	        WriteChunkBytes(iff, "\t", 1);

	    if (len > 0)
	        WriteChunkBytes(iff, text, len);

	    /* Clear the right of the line */
	    if (line->ln_Flags & LNF_LF)
	    {
		if (mark_line2 == line) mark = FALSE;
		else  WriteChunkBytes(iff, "\n", 1);
	    }
	}

	if (line->ln_Flags & LNF_LF)
	{
	    curlinelen = 0;
	} else curlinelen += line->ln_TextLen + 1;

        if (mark_line2 == line)
        {
	    mark = FALSE;
	    break;
        }

        line = (struct Line *) Node_Next(line);
    }
}

/**************************************************************************
 Move the selected text into the given iff
**************************************************************************/
static void CopyTextNowDOS(struct Text_Data *td, BPTR handle)
{
    struct Line *line = (struct Line *) List_First(&td->line_list);
    BOOL mark = FALSE;
    LONG mark_y1, mark_x1;
    LONG mark_y2, mark_x2;
    struct Line *mark_line1, *mark_line2;
    LONG curlinelen = 0;

    PrepareMark(td, &mark_x1, &mark_y1, &mark_x2, &mark_y2, &mark_line1, &mark_line2);

    if (!handle) return;

    while (line)
    {
	STRPTR text = line->ln_Text;
	LONG len = line->ln_TextLen;

        if (mark_line1 == line)
	    mark = TRUE;

        if (mark)
        {
	     BOOL mark_start = FALSE, mark_end = FALSE;
	     BOOL draw_rect = FALSE;

	     if (mark_line1 == line) mark_start = TRUE;
	     if (mark_line2 == line) mark_end = TRUE;

	     if (mark_start)
	     {
		LONG start_len = mark_x1 - curlinelen - 1;

		if (start_len > 0)
		{
		   text += start_len;
	        } else
	        {
	    	    if (start_len < 0) draw_rect = TRUE;
	        }

	        if (mark_end)
	        {
	    	    len = mark_x2 - mark_x1;
		    if (start_len < 0) len--;
	        }
	        else
	        {
	    	    len -= mark_x1 - curlinelen - 1;
	    	    if (start_len < 0) len--;
	        }
	    }   else
	    {
	        draw_rect = TRUE;
	        if (mark_end)
	        {
		    len = mark_x2 - curlinelen - 1;
	        }
	    }

	    if (draw_rect && (GetLineStartX(line) != line->ln_XOffset))
	        FPutC(handle, '\t');

	    if (len > 0)
	        FWrite(handle,text,1,len);

	    /* Clear the right of the line */
	    if (line->ln_Flags & LNF_LF)
	    {
		if (mark_line2 == line) mark = FALSE;
		else  FPutC(handle,'\n');
	    }
	}

	if (line->ln_Flags & LNF_LF)
	{
	    curlinelen = 0;
	} else curlinelen += line->ln_TextLen + 1;

        if (mark_line2 == line)
        {
	    mark = FALSE;
	    break;
        }

        line = (struct Line *) Node_Next(line);
    }
}


/**************************************************************************
 Entrypoint for the copying (SAVEDS not needed)
**************************************************************************/
static void CopyTextEntry(void)
{
    struct Process *proc;
    struct CopyMsg *msg;
    struct IFFHandle *iff;

#ifndef __AROS__
    struct Library *SysBase = *((struct Library **) 4L);
#endif

    proc = (struct Process *) FindTask(NULL);
    WaitPort(&proc->pr_MsgPort);
    msg = (struct CopyMsg *) GetMsg(&proc->pr_MsgPort);
    putreg(REG_A4,msg->cm_regA4);

    if ((iff = PrepareClipboard()))
    {
	CopyTextNowIFF(msg->cm_TextData,iff);
	FreeClipboard(iff);
    }

    Forbid();
    ReplyMsg(&msg->cm_ExecMessage);
}

/**************************************************************************
 Copy the selected text passage
**************************************************************************/
static void CopyText(struct Text_Data *td)
{
    struct MsgPort *port = CreateMsgPort();
    if (port)
    {
    	struct CopyMsg *msg = (struct CopyMsg*)AllocVec(sizeof(struct CopyMsg),MEMF_PUBLIC|MEMF_CLEAR);
    	if (msg)
    	{
    	    struct Process *p;

    	    msg->cm_regA4 = getreg(REG_A4);
    	    msg->cm_TextData = td;
	    msg->cm_ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
	    msg->cm_ExecMessage.mn_ReplyPort    = port;

#ifdef __MORPHOS__
	    if ((p = CreateNewProcTags(NP_Entry,CopyTextEntry,
									   NP_CodeType, MACHINE_PPC,
	    			       NP_StackSize, 10000,
	    			       NP_Name,"text.datatype copy process",
				       TAG_DONE)))
#else
	    if ((p = CreateNewProcTags(NP_Entry,(IPTR)CopyTextEntry,
	    			       NP_StackSize, 10000,
	    			       NP_Name,(IPTR)"text.datatype copy process",
				       TAG_DONE)))
#endif
	    {
		PutMsg(&p->pr_MsgPort,&msg->cm_ExecMessage);
		WaitPort(port);
		while (GetMsg(port));
            }

	    FreeVec(msg);
    	}
    	DeleteMsgPort(port);
    }
}

/**************************************************************************
 Print the text (whole text only)
**************************************************************************/
static void PrintText(struct Text_Data *td, union printerIO *printer_io)
{
    D(bug("Printing Text...\n"));

    if (printer_io)
    {
	struct Line *line = (struct Line *) List_First(&td->line_list);
	LONG sig_mask = 1L << printer_io->ios.io_Message.mn_ReplyPort->mp_SigBit;
	LONG sigs;

	while (line->ln_Link.mln_Succ && !CheckSignal (SIGBREAKF_CTRL_C))
	{
	    LONG fontx = td->font->tf_XSize;
	    LONG space_len = GetRelativeOffset(line, fontx) / fontx;

	    while (space_len)
	    {
		printer_io->ios.io_Length = 1;
		printer_io->ios.io_Data = (APTR) " ";
		printer_io->ios.io_Command = CMD_WRITE;
	    	D(bug("DoIO1\n"));
		DoIO((struct IORequest *) printer_io);
		space_len--;
	    }

	    if (line->ln_TextLen)
	    {
		printer_io->ios.io_Length = line->ln_TextLen;
		printer_io->ios.io_Data = (APTR) line->ln_Text;
		printer_io->ios.io_Command = CMD_WRITE;
	    	D(bug("DoIO2\n"));
		DoIO((struct IORequest *) printer_io);
	    }

	    if (line->ln_Flags & LNF_LF)
	    {
		printer_io->ios.io_Length = 1;
		printer_io->ios.io_Data = (APTR) "\n";
		printer_io->ios.io_Command = CMD_WRITE;
	    	D(bug("Send\n"));
		SendIO((struct IORequest *) printer_io);

	    	D(bug("Wait\n"));

		sigs = Wait(SIGBREAKF_CTRL_C | sig_mask);
		if (sigs & SIGBREAKF_CTRL_C)
		{
		    if (!(sigs & sig_mask))
		    {
		    	D(bug("AbortIO\n"));
			AbortIO((struct IORequest *) printer_io);
		    	D(bug("WaitIO\n"));
			WaitIO((struct IORequest *) printer_io);
		    }
		    break;
		}
	    }
	    line = (struct Line *) line->ln_Link.mln_Succ;

	}	/* while(line) */

    }	/* if (printer_io) */

    D(bug("Printing Text finished...\n"));
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
	    if (buffer[i] == 10)	// && buffer[i+1]==10)

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

const static struct DTMethod trigger_methods[] =
{
  {"Search...","SEARCH",STM_SEARCH},
  {"Search next", "SEARCH_NEXT",STM_SEARCH_NEXT},
  {"Search previous", "SEARCH_PREV",STM_SEARCH_PREV},
  {NULL,NULL,0}
};

#ifdef __MORPHOS__
#ifdef MORPHOS_AG_EXTENSION
__varargs68k IPTR notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ...)
#else
static __varargs68k IPTR notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ...)
#endif
{
va_list va;
ULONG	result;
	va_start(va,flags);

	result = DoMethod(o, OM_NOTIFY, (struct TagItem*) va->overflow_arg_area, ginfo, flags);
	va_end(va);

	return(result);
}
#else

#ifndef MORPHOS_AG_EXTENSION
static
#endif
VARARGS IPTR notifyAttrChanges(Object * o, VOID * ginfo, ULONG flags, ULONG tag1,...)
{
	return DoMethod(o, OM_NOTIFY, (IPTR)&tag1, (IPTR)ginfo, flags);
}
#endif


#ifndef __AROS__

struct AsyncMethodMsg
{
    struct Message amm_ExecMessage;
    Object *amm_Object;
    struct IClass *amm_Class;
};


STATIC VOID DT_SearchString(Class *cl,Object *obj,LONG direction, struct GadgetInfo *ginfo, STRPTR text, LONG len);

/* SAVEDS not needed */
VOID getstring_entry(void)
{
    struct Library *SysBase;
    struct Process *proc;
    struct AsyncMethodMsg *amsg;
    struct IClass *cl;
    Object *obj;
    struct Text_Data *td;
    ULONG retval;//,search_method;
    LONG direction;

    SysBase = *((struct Library **) 4L);
    proc = (struct Process *) FindTask(NULL);
    WaitPort(&proc->pr_MsgPort);
    amsg = (struct AsyncMethodMsg *) GetMsg(&proc->pr_MsgPort);
    obj = amsg->amm_Object;
    cl = amsg->amm_Class;
    ReplyMsg(&amsg->amm_ExecMessage);

    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);	/* Small Data */

    td = (struct Text_Data *)INST_DATA(cl, obj);
    retval = GetSearchString(td->search_buffer,sizeof(td->search_buffer), &td->search_case, td->search_ginfo.gi_Window);

    Forbid();

    if (retval)
    {
	if (retval == 1 || retval == 2) direction = 1;
	else direction = -1;
	if (retval == 2 || retval == 3) td->search_line = -1;
	DT_SearchString(cl,obj,direction,&td->search_ginfo,td->search_buffer,strlen(td->search_buffer));
    }

    if (FindTask(NULL) == (struct Task*)td->search_proc)
    	td->search_proc = NULL;
    /* task ends here */
}

struct Process *CreateGetStringProcess(struct IClass *cl, Object *obj, struct GadgetInfo *gi)
{
    struct MsgPort *mport;
    struct Process *proc = NULL;

    if ((mport = CreateMsgPort()))
    {
    	struct AsyncMethodMsg *amsg = (struct AsyncMethodMsg*)AllocVec(sizeof(struct AsyncMethodMsg),MEMF_PUBLIC);
    	if (amsg)
    	{
	    struct Text_Data *td = (struct Text_Data *)INST_DATA(cl, obj);
	    amsg->amm_Object = obj;
	    amsg->amm_Class  = cl;
	    amsg->amm_ExecMessage.mn_Node.ln_Type = NT_MESSAGE;
	    amsg->amm_ExecMessage.mn_ReplyPort    = mport;
	    td->search_ginfo = *gi;
#ifdef __MORPHOS__
	    if ((proc = CreateNewProcTags(NP_Entry,getstring_entry,
										  NP_CodeType, MACHINE_PPC,
	    				  NP_StackSize, 10000,
	    				  NP_Name,"text.datatype getstring process",
					  TAG_DONE)))
#else
	    if ((proc = CreateNewProcTags(NP_Entry,getstring_entry,
	    				  NP_StackSize, 10000,
	    				  NP_Name,"text.datatype getstring process",
					  TAG_DONE)))
#endif
	    {
		PutMsg(&proc->pr_MsgPort,&amsg->amm_ExecMessage);
		WaitPort(mport);
		while (GetMsg(mport));
            }
            FreeVec(amsg);
        }
	DeleteMsgPort(mport);
    }
    return proc;

}

#endif

STATIC struct Gadget *_OM_NEW(struct IClass *cl, Object * o, struct opSet *msg)
{
    struct Gadget *g;
    struct TagItem *attrs = msg->ops_AttrList;
    ULONG st;

    st = GetTagData(DTA_SourceType, DTST_FILE, attrs);

    if (st != DTST_CLIPBOARD && st != DTST_RAM && st != DTST_FILE)
    {
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }

    if ((g = (struct Gadget *) DoSuperMethodA(cl, o, (Msg) msg)))
    {
	struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
	struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;
	struct TagItem *ti;

    #if !NO_GMORE_SCROLLRASTER
	/* We use ScrollRaster() so this tag must be set (see Autodocs ScrollWindowRaster()) */
	((struct ExtGadget*)g)->MoreFlags |= GMORE_SCROLLRASTER;
    #endif
	memset(td, 0, sizeof(struct Text_Data));
	if ((ti = FindTagItem(DTA_TextAttr, msg->ops_AttrList)))
	{
	    struct TextAttr *ta = (struct TextAttr *) (struct TextAttr *) ti->ti_Data;

	    td->attr.ta_Name = StrCopy(ta->ta_Name);
	    td->attr.ta_YSize = ta->ta_YSize;
	}
	InitText(td);
#ifdef MORPHOS_AG_EXTENSION
        /* remember our object pointer for internal use */
        td->obj = (Object *) g;
#endif
	td->vert_unit = si->si_VertUnit = td->font->tf_YSize;
	td->horiz_unit = si->si_HorizUnit = (td->font->tf_Flags & FPF_PROPORTIONAL) ? 1L : td->font->tf_XSize;

	SetFont(&td->font_rp,td->font);

//	D(bug("si->si_HorizUnit %ld\n",si->si_HorizUnit));

/*
	si->si_HorizUnit = 1;
	si->si_VertUnit = td->font->tf_YSize;
*/
	td->search_pos = -1;
	td->search_line = -1;
	if (st == DTST_CLIPBOARD || st == DTST_FILE)
	{
	    APTR handle;
	    if (GetDTAttrs((Object *) g, DTA_Handle, (IPTR) &handle, TAG_DONE) != 1)
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
		    LONG type;
		    STRPTR name;
		    struct DataType *dt;

		    if (GetDTAttrs((Object *) g, DTA_Name, (IPTR) &name, DTA_DataType, (IPTR) &dt, TAG_DONE) != 2)
		    {
			name = NULL;
			dt = NULL;
		    }

		    if (!name) name = "Unnamed";
		    if (dt) type = dt->dtn_Header->dth_Flags & DTF_TYPE_MASK;
		    else type = DTF_MISC;

		    D(bug("Handle: 0x%lx    type: %lx\n", handle, dt->dtn_Header->dth_Flags));

/*
		    td->oldmarkactivation = 0;
		    if (GetVar("TEXTDT_MENUMARKING", buf, sizeof(buf), LV_VAR) > 0)
		    {
			if (*buf == '1')
			    td->oldmarkactivation = 1;
		    }
*/
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
			    if (LoadText(td, name, (BPTR) handle))
				return g;
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

STATIC ULONG _OM_SET(struct IClass * cl, struct Gadget * g, struct opSet * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    const struct TagItem *tl = msg->ops_AttrList;
    struct TagItem *ti;

    LONG top_vert = td->vert_top;
    LONG top_horiz = td->horiz_top;
    BOOL new_top_vert = FALSE;
    BOOL new_top_horiz = FALSE;
    BOOL redraw_all = FALSE;
/*    BOOL layout = FALSE;*/

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
#ifdef MORPHOS_AG_EXTENSION
                {
		    new_top_vert = TRUE;
                    td->ginfo = msg->ops_GInfo;
                    CheckMarkedLink(td, top_vert);
                }
#else
                    new_top_vert = TRUE;
#endif
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
#ifdef MORPHOS_AG_EXTENSION
            td->ginfo = NULL;
            CheckMarkedLink(td, top_vert);
#endif
	    break;

	case DTA_TotalVert:
	    D(bug("text.datatype/DT_SetMethod: DTA_TotalVert  %ld\n", ti->ti_Data));
	    break;

	case DTA_NominalVert:
	    D(bug("text.datatype/DT_SetMethod: DTA_NominalVert  %ld\n", ti->ti_Data));
	    break;

	case DTA_VertUnit:
	    D(bug("text.datatype/DT_SetMethod: DTA_VertUnit  %ld\n", ti->ti_Data));
#ifdef MORPHOS_AG_EXTENSION
            td->vert_unit = ti->ti_Data;
#endif
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
	    if (ti->ti_Data) redraw_all = 1;
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

	case TDTA_WordWrap:
	    D(bug("text.datatype/DT_SetMethod: TDTA_WordWrap  0x%ld\n", ti->ti_Data));
	    td->word_wrap = ti->ti_Data;
	    break;

	case TDTA_WordDelim:
	    D(bug("text.datatype/DT_SetMethod: TDTA_WordDelim  0x%ld\n", ti->ti_Data));
	    td->word_delim = (char*) ti->ti_Data;
	    break;

	default:
	    D(bug("text.datatype/DT_SetMethod: Tag:0x%lx  %ld\n", ti->ti_Tag, ti->ti_Data));
	    break;

	} /* switch (ti->ti_Tag) */
    } /* while ((ti = NextTagItem(&tl))) */

/*    if (layout && msg->ops_GInfo)
    {
	DoMethod((Object *) g, GM_LAYOUT, msg->ops_GInfo, TRUE);
    }
*/
#ifdef MORPHOS_AG_EXTENSION
	/* adjust top values according to document and view dimension */
	if (new_top_vert)
	{
	    LONG visible;
	    LONG total;

	    if(GetDTAttrs((Object *) g, 
                          DTA_TotalVert, (ULONG) &total,
			  DTA_VisibleVert, (ULONG) &visible,
			  TAG_DONE) == 2)
	    {
		if(visible + top_vert > total)
		    top_vert = total - visible;
		if(top_vert < 0)
		    top_vert = 0;
	    }
	}
	if (new_top_horiz)
	{
	    LONG visible;
	    LONG total;

	    if(GetDTAttrs((Object *) g, 
                          DTA_TotalHoriz, (ULONG) &total,
			  DTA_VisibleHoriz, (ULONG) &visible,
			  TAG_DONE) == 2)
	    {
		if(visible + top_horiz > total)
		    top_horiz = total - visible;
		if(top_horiz < 0)
		    top_horiz = 0;
	    }
	}
#endif
    if (redraw_all || (new_top_horiz && new_top_vert))
    {
	td->update_type = 1;
	retval++;
	td->horiz_top = top_horiz;
	td->vert_top = top_vert;
    }
    else
    {
	if (new_top_horiz || new_top_vert)
	{
	    if (new_top_horiz)
	    {
		retval++;
		td->update_type = 2;
		td->update_arg = top_horiz;
	    }

	    if (new_top_vert)
	    {
 		retval++;
		td->update_type = 3;
		td->update_arg = top_vert;
	    }
	}
    }

    D(bug(" Set retval: %ld\n", retval));
    return retval;
}

ULONG Text__OM_NEW(struct IClass *cl, Object * o, struct opSet *msg)
{
    struct Gadget *retval = _OM_NEW(cl, o, msg);
#ifdef MORPHOS_AG_EXTENSION
    if (retval != NULL)
	_OM_SET(cl, retval, msg);
#endif
    return (ULONG)retval;
}

VOID Text__OM_DISPOSE(struct IClass * cl, Object * o, Msg msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

    DisposeText(td);
    DoSuperMethodA(cl, o, msg);
}

ULONG Text__OM_GET(struct IClass *cl, struct Gadget *g, struct opGet *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    switch (msg->opg_AttrID)
    {
    case TDTA_Buffer:
	D(bug("text.datatype/DT_GetMethod: TDTA_Buffer 0x%lx\n", td->buffer_allocated));
	*msg->opg_Storage = (ULONG) td->buffer_allocated;
	break;

    case TDTA_BufferLen:
	D(bug("text.datatype/DT_GetMethod: TDTA_BufferLen  %ld\n", td->buffer_allocated_len));
	*msg->opg_Storage = (ULONG) td->buffer_allocated_len;
	break;

    case TDTA_LineList:
	D(bug("text.datatype/DT_GetMethod: TDTA_LineList\n"));
	*msg->opg_Storage = (ULONG) &td->line_list;
	break;

    case TDTA_WordWrap:
	D(bug("text.datatype/DT_GetMethod: TDTA_WordWrap\n"));
	*msg->opg_Storage = td->word_wrap;
	break;

    case DTA_TextAttr:
	D(bug("text.datatype/DT_GetMethod: DTA_TextAttr (%s)\n", td->attr.ta_Name));
	*msg->opg_Storage = (ULONG) & td->attr;
	break;

    case DTA_TextFont:
	D(bug("text.datatype/DT_GetMethod: DTA_TextFont\n"));
	*msg->opg_Storage = (ULONG) td->font;
	break;

    case DTA_ObjName:
	D(bug("text.datatype/DT_GetMethod: DTA_ObjName\n"));
	*msg->opg_Storage = (ULONG) td->title;
	break;

    case DTA_Methods:
	D(bug("text.datatype/DT_GetMethod: DTA_Methods\n"));
	*msg->opg_Storage = (ULONG) supported_methods;
	break;

    case DTA_TriggerMethods:
    	D(bug("text.datatype/DT_GetMethod: DTA_TriggerMethods\n"));
    	*msg->opg_Storage = (ULONG) trigger_methods;
    	break;

    default:
	D(bug("text.datatype/DT_GetMethod: Tag ID: 0x%lx\n", msg->opg_AttrID));
	return DoSuperMethodA(cl, (Object *) g, (Msg) msg);
    }
    return 1;
}

ULONG Text__OM_SET(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    ULONG retval = DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    retval += _OM_SET(cl, g, msg);

    if (retval && (OCLASS (g) == cl))
    {
	struct RastPort *rp;
	D(bug("text.datatype: gadget should be redrawed\n"));
	/* Get a pointer to the rastport */
	if ((rp = ObtainGIRPort (((struct opSet *) msg)->ops_GInfo)))
	{
	    /* Force a redraw */
	    DoMethod ((Object *)g, GM_RENDER, (IPTR) ((struct opSet *) msg)->ops_GInfo, (IPTR) rp, GREDRAW_UPDATE);
	    /* Release the temporary rastport */
	    ReleaseGIRPort (rp);
	    retval = 0;
	}
    }

    return retval;
}
ULONG Text__OM_UPDATE(struct IClass *cl, struct Gadget *g, struct opSet *msg)
{
    return Text__OM_SET(cl, g, msg);
}

ULONG Text__GM_RENDER(struct IClass * cl, struct Gadget * g, struct gpRender * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;
    ULONG retval = (ULONG) DoSuperMethodA(cl, (Object *)g, (Msg)msg);
    
    D(bug("text.datatype/DT_Render: gpr_Redraw: %ld  si->si_Flags: %lx\n", msg->gpr_Redraw, si->si_Flags));

    if (!(si->si_Flags & DTSIF_LAYOUT))
    {
	struct IBox *domain;
	LONG vh, vv;

	if (GetDTAttrs((Object *) g,
		       DTA_Domain, (IPTR) &domain,
		       DTA_VisibleHoriz, (IPTR) &vh,
		       DTA_VisibleVert, (IPTR) &vv,
		       TAG_DONE) == 3)
	{
	    ULONG redraw_type;

	    if (!AttemptSemaphoreShared(&(si->si_Lock)))
	    {
		/* The datatype should be redrawn but can not get the lock, so redraw it the next time fully */
		D(bug("text.datatype/DT_Render: No semaphore\n"));
		td->redraw = 1;
		return retval;
	    }

            D(bug("redraw %ld, update_type %ld, update_arg %ld\n", td->redraw, td->update_type, td->update_arg));
	    if (td->redraw)
	    {
		/* The whole text should be redrawed */
		td->redraw = 0;
		redraw_type = GREDRAW_REDRAW;

#ifdef MORPHOS_AG_EXTENSION
                if (td->update_type == 2) td->horiz_top = td->update_arg;
                else if (td->update_type == 3) td->vert_top = td->update_arg;
#else
		if (msg->gpr_Redraw == GREDRAW_UPDATE)
		{
		    if (td->update_type == 2) td->horiz_top = td->update_arg;
		    else if (td->update_type == 3) td->vert_top = td->update_arg;
		}
#endif
	    } else redraw_type = msg->gpr_Redraw;

            D(bug("top vert %ld, %ld\n", td->vert_top, si->si_TopVert));

	    td->left = domain->Left;
	    td->top = domain->Top;
	    td->width = domain->Width;
	    td->height = domain->Height;

	    /* Needed for AmigaGuide */
	    td->horiz_visible = vh;
	    td->vert_visible = vv;

	    /* why this??? somebody seems to set a new horiz_unit whitout using the tags */
	    td->horiz_unit = si->si_HorizUnit;

/*	    td->horiz_top = si->si_TopHoriz;
	    td->horiz_visible = si->si_VisHoriz;
	    td->vert_top = si->si_TopVert;
	    td->vert_visible = si->si_VisVert;*/

	    if (redraw_type == GREDRAW_REDRAW)
	    {
		D(bug("text.datatype/DT_Render: Complete\n"));
		DrawText(td, msg->gpr_RPort);
	    }
	    else
	    {
		if (redraw_type == GREDRAW_UPDATE)
		{
		    D(bug("text.datatype/DT_Render: Update %ld\n", td->update_type));
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
	} else
	{
	    if (td->update_type == 2) td->horiz_top = td->update_arg;
	    else if (td->update_type == 3) td->vert_top = td->update_arg;
	    td->redraw = 1;
	    return retval;
	}
    } else
    {
    	if (td->update_type == 2) td->horiz_top = td->update_arg;
   	else if (td->update_type == 3) td->vert_top = td->update_arg;
    	td->redraw = 1;
    	return retval;
    }
    retval++;

    return retval;
}

LONG Text__GM_HANDLEINPUT(struct IClass * cl, struct Gadget * g, struct gpInput * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct InputEvent *ievent = msg->gpi_IEvent;
    struct DTSpecialInfo *si = (struct DTSpecialInfo *) g->SpecialInfo;
    LONG retval = GMR_MEACTIVE;

    if (!AttemptSemaphore(&(si->si_Lock)))
	return GMR_NOREUSE;

    if (si->si_Flags & DTSIF_LAYOUT)
    {
	ReleaseSemaphore(&(si->si_Lock));
	return GMR_NOREUSE;
    }

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

#ifdef MORPHOS_AG_EXTENSION
        if (td->mouse_pressed && !td->link_pressed)
#else
	if (td->mouse_pressed)
#endif
	{
	    LONG diff_x, diff_y;
	    LONG x = msg->gpi_Mouse.X, y = msg->gpi_Mouse.Y;
	    LONG newx, newy;

 	    if (x<0)
 		diff_x = (x - td->font->tf_XSize + 1) / td->font->tf_XSize;
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
#ifdef __AROS__
		IPTR val;
		LONG top, total, visible;

		GetDTAttrs((Object *)g, DTA_TopHoriz, (IPTR) &val, TAG_DONE); top = (LONG)val;
		GetDTAttrs((Object *)g, DTA_TotalHoriz, (IPTR) &val, TAG_DONE); total = (LONG)val;
		GetDTAttrs((Object *)g, DTA_VisibleHoriz, (IPTR) &val, TAG_DONE); visible = (LONG)val;

		newx = td->horiz_top + ((diff_x < 0) ? -1 : 1);

		if (newx + visible > total) newx = total - visible;
		if (newx < 0) newx = 0;

		if (newx != top)
		    notifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
				      GA_ID, g->GadgetID,
				      DTA_TopHoriz, newx,
				      TAG_DONE);
#else
 		newx = td->horiz_top + ((diff_x < 0) ? -1 : 1);
 		notifyAttrChanges((Object *) g, ((struct gpLayout *) msg)->gpl_GInfo, 0,
 				  GA_ID, g->GadgetID,
 				  DTA_TopHoriz, newx,
  				  TAG_DONE);
#endif
  	    }

	    if (diff_y)
	    {
#ifdef __AROS__
		IPTR val;
		LONG top, total, visible;

		GetDTAttrs((Object *)g, DTA_TopVert, (IPTR) &val, TAG_DONE); top = (LONG)val;
		GetDTAttrs((Object *)g, DTA_TotalVert, (IPTR) &val, TAG_DONE); total = (LONG)val;
		GetDTAttrs((Object *)g, DTA_VisibleVert, (IPTR) &val, TAG_DONE); visible = (LONG)val;

		newy = td->vert_top + ((diff_y < 0) ? -1 : 1);

		if (newy + visible > total) newy = total - visible;
		if (newy < 0) newy = 0;

		if (newy != top)
		    notifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
				      GA_ID, g->GadgetID,
				      DTA_TopVert, newy,
				      TAG_DONE);

#else
		newy = td->vert_top + ((diff_y < 0) ? -1 : 1);
		notifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
				  GA_ID, g->GadgetID,
				  DTA_TopVert, newy,
				  TAG_DONE);
#endif
	    }
	}
    }

    if (ievent->ie_Class == IECLASS_RAWMOUSE)
    {
	struct RastPort *rp;
	rp = ObtainGIRPort(msg->gpi_GInfo);
#ifdef MORPHOS_AG_EXTENSION
        /* remember object and gadgetinfo used by TriggerLink() */
        td->ginfo = msg->gpi_GInfo;
#endif

	if (ievent->ie_Code == SELECTUP)
	{
	    td->copy_text = (si->si_Flags & DTSIF_DRAGSELECT)?0:1;
	    HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, SELECTUP, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
	    retval = GMR_NOREUSE;
	}
	else
	{
	    if (ievent->ie_Code == SELECTDOWN)
	    {
	        HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, SELECTDOWN, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
	    }
	    else HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, 0, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
	}

	if (rp)
	    ReleaseGIRPort(rp);
    }
    else
    {
	if (ievent->ie_Class == IECLASS_TIMER)
	{
	    struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);

	    HandleMouse(td, rp, msg->gpi_Mouse.X, msg->gpi_Mouse.Y, 0, ievent->ie_TimeStamp.tv_secs, ievent->ie_TimeStamp.tv_micro);
	    if (rp)
	        ReleaseGIRPort(rp);
	} else
	{
	    if (ievent->ie_Class == IECLASS_RAWKEY)
	    {
		if (ievent->ie_Code == CURSORDOWN)
		{
		    notifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
				  GA_ID, g->GadgetID,
				  DTA_TopVert, td->vert_top + 1,
				  TAG_DONE);
			
		} else
		{
		    if (ievent->ie_Code == CURSORUP)
		    {
		        notifyAttrChanges((Object *) g, msg->gpi_GInfo, 0,
				  GA_ID, g->GadgetID,
				  DTA_TopVert, td->vert_top - 1,
				  TAG_DONE);

		    }
		}
	    }
	}
    }

    ReleaseSemaphore(&(si->si_Lock));

    return retval;
}
ULONG Text__GM_GOACTIVE(struct IClass *cl, struct Gadget *g, struct gpInput *msg)
{
    return Text__GM_HANDLEINPUT(cl, g, msg);
}

BOOL Text__DTM_WRITE(struct IClass * cl, struct Gadget * g, struct dtWrite * msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    // D(bug("%ld\n", msg->dtw_Mode));

    if (msg->dtw_Mode == DTWM_RAW && msg->dtw_FileHandle)
    {
	/* A NULL file handle is a NOP */
	if( msg->dtw_FileHandle )
	{
	    if (!td->mark_line1)
		Write(msg->dtw_FileHandle, td->buffer_allocated, td->buffer_allocated_len);
	    else CopyTextNowDOS(td,msg->dtw_FileHandle);
	}
    }  else
    if (msg->dtw_Mode == DTWM_IFF)
    {
    	struct IFFHandle *iff;
	    
	if ((iff = AllocIFF()))
    	{
	    D(bug(" got iff handle %08lx\n", (long)iff));
	    if ((iff->iff_Stream = (IPTR)(msg->dtw_FileHandle)))
	    {
		InitIFFasDOS(iff);
		D(bug(" init iff %08lx\n", (long)iff));
		if(!OpenIFF(iff,IFFF_WRITE))
		{
		    D(bug(" open iff %08lx\n", (long)iff));
		    if(!PushChunk(iff, MAKE_ID('F','T','X','T'), MAKE_ID('F','O','R','M'), IFFSIZE_UNKNOWN))
		    {
			D(bug(" push chunk FTXT %08lx\n", (long)iff));
			if(!PushChunk(iff, 0, MAKE_ID('C','H','R','S'), IFFSIZE_UNKNOWN))
			{
			    D(bug(" push chunk CHRS %08lx\n", (long)iff));
			    CopyTextNowIFF(td, iff);
			    D(bug(" copy text %08lx\n", (long)iff));
			    PopChunk(iff);
			}
			PopChunk(iff);
		    }
		    CloseIFF(iff);
		}
	    }
	    FreeIFF(iff);
	}
    }
    return TRUE;
}

VOID Text__DTM_PRINT(struct IClass *cl, struct Gadget *g, struct dtPrint *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);

    PrintText(td, msg->dtp_PIO);
}

#ifndef __AROS__

STATIC LONG strseg(struct Line *line, STRPTR str, LONG slen, LONG casesens, LONG offset)
{
    STRPTR text = line->ln_Text + offset;
    STRPTR ptr;
    LONG llen = line->ln_TextLen - offset;
    LONG len;

    while (llen >= slen)
    {
	ptr = str;
	len = slen;

	if (casesens)
	{
	    while (*text == *ptr)
	    {
	        text++;
	        ptr++;
	        llen--;
	       len--;
	    }
	} else
	{
	    while (ToLower(*text) == ToLower(*ptr))
	    {
	        text++;
	        ptr++;
	        llen--;
	       len--;
	    }
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

STATIC LONG strsegback(struct Line *line, STRPTR str, LONG slen, LONG casesens, LONG offset)
{
    STRPTR text = line->ln_Text + offset;// + line->ln_TextLen - 1;
    STRPTR ptr;
    LONG llen = /*line->ln_TextLen -*/ offset;
    LONG len;

    if (offset < 0) return -1;

    while (llen >= slen - 1)
    {
	ptr = str + slen - 1;
	len = slen;

	if (casesens)
	{
	    char *text_ptr = text;
	    while (len && (*text_ptr == *ptr))
	    {
	        text_ptr--;
	        ptr--;
	       len--;
	    }
	} else
	{
	    UBYTE *text_ptr = text;
	    while (len && (ToLower(*text_ptr) == ToLower(*ptr)))
	    {
	        text_ptr--;
	        ptr--;
	       len--;
	    }
	}

	if (len==0)
	{
	    D(bug("hiusdhis\n"));
	    return text - line->ln_Text + 1 - slen;
	}
	else
	{
	    text--;
	    llen--;
	}
    }
    return -1;
}

STATIC VOID DT_SearchString(Class *cl,Object *obj,LONG direction, struct GadgetInfo *ginfo, STRPTR text, LONG len)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, obj);
    struct List *list;

    if (GetAttr(TDTA_LineList,obj,(ULONG *) &list))
    {
	struct Line *line;
	LONG found = td->search_line;
	LONG found_x = 0;
	LONG y = 0;
	int search_offx = 0;

	line = (struct Line *)list->lh_Head;

	if (direction == -1)
	{
            if (found == -1)
	    {
		GetAttr(DTA_TotalVert,obj,(ULONG*)&found);
	    }
/*	    found++;*/
	}

	/* Find the correct line node */
	while (y < found && line->ln_Link.mln_Succ)
	{
	    if (line->ln_Flags & LNF_LF)
		y++;
	    line = (struct Line *) line->ln_Link.mln_Succ;
	}

	found = -1;

	if (direction == -1)
	{
	    if (line->ln_Link.mln_Succ)
	    {
		struct Line *l;
		D(bug("%lx   %ld\n",line,td->search_pos));
		if ((l = FindLineChar(line,td->search_pos)))
		{
		    LONG off =  td->search_pos - GetLineCharX(l);
		    D(bug("  off: %ld\n",off));
		    line = l;

		    found_x = strsegback(line, text, len, td->search_case,off-1);
		    D(bug("foundx: %lx %lx\n",found_x,l));


		    if (found_x >= 0)
		    {
		    	found = y;
		    	D(bug(" found: %ld\n",found));
		    }
		}
	    }


/*	    D(bug("td->search_line: %ld  y: %ld  %ld %ld\n",td->search_line,y,foundline, line->ln_Link.mln_Succ?1:0));*/

	    if (found < 0)
	    {
		if (line->ln_Link.mln_Pred)
		{
		    line = (struct Line *) line->ln_Link.mln_Pred;
		}


/*	    if (!line->ln_Link.mln_Succ)
	    {
	      line = (struct Line *) line->ln_Link.mln_Pred;
	    }*/

		while (line->ln_Link.mln_Pred && found == -1)
		{
		    if (line->ln_Flags & LNF_LF) y--;
		    if (line->ln_TextLen >= len)
		    {
		        found_x = strsegback(line, text, len, td->search_case,line->ln_TextLen-1);
		        D(bug("found_x: %ld\n",found_x));
		        if (found_x >= 0)
		        {
			    found = y;
			    D(bug("found_y: %ld\n",y));
			    break;
		        }
		    }
		    line = (struct Line *) line->ln_Link.mln_Pred;
	         }
	     }
	} else
	{
	    D(bug("searchline: %ld searchpos: %ld\n",td->search_line,td->search_pos));
	    if (y == td->search_line && td->search_line != -1)
	    {
	    	struct Line *l = FindLineChar(line,td->search_pos+1);
	    	LONG off =  td->search_pos - GetLineCharX(l);
	    	if (l)
	    	{
	    	    D(bug("searchpos: %ld off: %ld  linelen: %ld\n",td->search_pos,off,l->ln_TextLen));
	    	    line = l;
	    	    if (off >= 0)
	    	    {
	    	      off++;
	    	      found_x = strseg(l, text, len, td->search_case, off);
	    	      if (found_x >= 0) found = y;
	    	    }

		    if (found_x < 0)
		    {
		        if (line->ln_Flags & LNF_LF) y++;
		        line = (struct Line *) line->ln_Link.mln_Succ;
		    }
	        }
	    }

	    while (line->ln_Link.mln_Succ && found == -1)
	    {
	        if (line->ln_TextLen >= len)
	        {
	            found_x = strseg(line, text, len, td->search_case,0);
	            if (found_x >= 0)
	            {
	            	D(bug("found_x: %ld\n",found_x));
			found = y;
			break;
		    }
		}
		if (line->ln_Flags & LNF_LF) y++;
		line = (struct Line *) line->ln_Link.mln_Succ;
	    }
	}

	td->search_line = found;
	td->search_pos = GetLineCharX(line) + found_x;
	D(bug("searchline: %ld  searchpos: %ld  found_x: %ld\n",td->search_line,td->search_pos,found_x));

	if (found < 0)
	{
	    td->search_pos = -1;
	    DisplayBeep(ginfo->gi_Screen);
/*	    found = 0;*/
	} else
	{
	    int mark_x1 = found_x + 1;
	    struct Line *line_node;
	    struct RastPort *rp = ObtainGIRPort(ginfo);
	    if (td->mark_line1 && rp)
	    {
		DrawMarkedText(td,rp,FALSE);
		td->mark_line1 = td->mark_line2 = NULL;
	    }

	    D(bug("q %ld %ld\n",line->ln_Flags & LNF_LF, line->ln_TextLen));

	    line_node = (struct Line*)Node_Prev(line);
	    while (line_node)
	    {
	    	if (line_node->ln_Flags & LNF_LF) break;
	    	mark_x1 += line_node->ln_TextLen + 1;
		line_node = (struct Line*)Node_Prev(line_node);
	    }

	    {
	    	LONG visible_horiz;
	    	LONG visible_vert;
	    	LONG top_horiz;
	    	LONG top_vert;
	    	LONG new_x1 = line->ln_XOffset/td->horiz_unit + found_x;

		GetDTAttrs(obj,
			DTA_TopVert, &top_vert,
			DTA_TopHoriz, &top_horiz,
			DTA_VisibleVert, &visible_vert,
			DTA_VisibleHoriz, &visible_horiz,
			TAG_DONE);

		D(bug("new_x1: %ld  top_horiz: %ld  vis: %ld\n",new_x1,top_horiz,visible_horiz));

		if ((found <= top_vert || found >= top_vert + visible_vert) ||
		    (new_x1 < top_horiz || new_x1 >= top_horiz + visible_horiz))
		{

		    if (new_x1 < top_horiz || new_x1 >= top_horiz + visible_horiz)
		    {
			notifyAttrChanges(obj,ginfo,0,
			    DTA_TopVert,found,
			    DTA_TopHoriz,new_x1,
			    TAG_DONE);
		    } else
		    {
			notifyAttrChanges(obj,ginfo,0,
			    DTA_TopVert,found,
			    TAG_DONE);
		    }
		}
	    }

            td->mark_line1 = td->mark_line2 = line;
            td->mark_x1 = mark_x1;//found_x + line->ln_XOffset / td->font->tf_XSize;
            td->mark_x2 = td->mark_x1 + len;
            td->mark_y1 = td->mark_y2 = found;

	    D(bug("mark_x1: %ld  mark_x2: %ld  mark_y1: %ld\n",td->mark_x1,td->mark_x2,td->mark_y1));

	    if (rp)
	    {
	    	DrawMarkedText(td,rp,TRUE);
		ReleaseGIRPort(rp);
	    }
	}
    }
}
#endif /* !__AROS__ */

#ifndef MORPHOS_AG_EXTENSION
VOID Text__DTM_TRIGGER(struct IClass *cl, Object *o, struct dtTrigger *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);
    ULONG function = ((struct dtTrigger*)msg)->dtt_Function;

    D(bug(" Trigger function %ld\n",function));

    if (function == STM_ACTIVATE_FIELD || function == STM_RETRACE || function == STM_SEARCH)
    {
	if (!td->search_proc)
	{
	    td->search_proc = CreateGetStringProcess(cl,o,((struct dtTrigger*)msg)->dtt_GInfo);
	}
    } else
    {
	LONG direction;
	if (function == STM_PREV_FIELD || function == STM_BROWSE_PREV || function == STM_SEARCH_PREV) direction = -1;
	else direction = 1;

	if (td->search_buffer[0])
	{
	   DT_SearchString(cl,o,direction,((struct dtTrigger*)msg)->dtt_GInfo,
		     td->search_buffer,strlen(td->search_buffer));
	}
    }
}
#endif /* !MORPHOS_AG_EXTENSION */

ULONG Text__GM_LAYOUT(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, g);
    struct GadgetInfo *gi = msg->gpl_GInfo;

    td->redraw = 1;

    if (gi->gi_DrInfo)
    {
    	td->fillpen = gi->gi_DrInfo->dri_Pens[FILLPEN];
    	td->filltextpen = gi->gi_DrInfo->dri_Pens[FILLTEXTPEN];
#ifdef MORPHOS_AG_EXTENSION
                /* remember shine and shadow pen for bevel rendering */
                td->shinepen = gi->gi_DrInfo->dri_Pens[SHINEPEN];
                td->shadowpen = gi->gi_DrInfo->dri_Pens[SHADOWPEN];
#endif
    }

#ifndef MORPHOS_AG_EXTENSION
    notifyAttrChanges((Object*)g, gi, 0,
		      GA_ID, g->GadgetID,
		      DTA_Busy, TRUE,
		      TAG_DONE);
#endif

   return DoSuperMethodA(cl, (Object*)g, (Msg) msg);
}
ULONG Text__DTM_PROCLAYOUT(struct IClass *cl, struct Gadget *g, struct gpLayout *msg)
{
    return Text__GM_LAYOUT(cl, g, msg);
}

ULONG Text__DTM_CLEARSELECTED(struct IClass *cl, Object *o, Msg msg)
{
    struct RastPort *rp;
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

    rp = ObtainGIRPort(((struct dtGeneral *) msg)->dtg_GInfo);
    ClearSelected(td, rp);
    if (rp)
	ReleaseGIRPort(rp);

    return 1;
}

ULONG Text__DTM_COPY(struct IClass *cl, Object *o, Msg msg)
{
    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);

    CopyText(td);
    return 1;
}

#ifndef __AROS__

#ifdef __MORPHOS__
AROS_UFH3S(IPTR, DT_Dispatcher,
	   AROS_UFHA(Class *, cl, A0),
	   AROS_UFHA(Object *, o, A2),
	   AROS_UFHA(STACKULONG *, msg, A1))
#else
ASM ULONG DT_Dispatcher2(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 LONG * msg)
#endif
{
    AROS_USERFUNC_INIT
    
    ULONG retval;

    switch (*msg)
    {
    case OM_NEW: D(bug("text.datatype: Dispatcher called (MethodID: OM_NEW)!\n"));
	return Text__OM_NEW(cl, o, (struct opSet *) msg);

    case OM_DISPOSE: D(bug("text.datatype: Dispatcher called (MethodID: OM_DISPOSE)!\n"));
	Text__OM_DISPOSE(cl, o, (Msg) msg);
	break;

    case OM_UPDATE: D(bug("text.datatype: Dispatcher called (MethodID: OM_UPDATE)!\n"));
    case OM_SET: if (*msg == OM_SET) D(bug("text.datatype: Dispatcher called (MethodID: OM_SET)\n"));
	return Text__OM_SET(cl, o, (Msg)msg);

    case OM_GET: D(bug("text.datatype: Dispatcher called (MethodID: OM_GET)!\n"));
	return Text__OM_GET(cl, (struct Gadget *) o, (struct opGet *) msg);

    case GM_RENDER:
	D(bug("text.datatype: Dispatcher called (MethodID: GM_RENDER)!\n"));
	return Text__GM_RENDER(cl, (struct Gadget *) o, (struct gpRender *)msg);

    case DTM_PROCLAYOUT:
    case GM_LAYOUT:
	D(bug("text.datatype: Dispatcher called (MethodID: GM_LAYOUT)!\n"));
	return Text__GM_LAYOUT(cl, (struct Gadget *)o, (struct gpLayout *)msg);

    case GM_GOACTIVE:
	D(bug("text.datatype: Dispatcher called (MethodID: GM_GOACTIVE)!\n"));
	return (ULONG) DT_HandleInputMethod(cl, (struct Gadget *) o, (struct gpInput *) msg);

    case GM_HANDLEINPUT:
	D(bug("text.datatype: Dispatcher called (MethodID: GM_HANLDEINPUT)!\n"));
	return (ULONG) DT_HandleInputMethod(cl, (struct Gadget *) o, (struct gpInput *) msg);

    case DTM_CLEARSELECTED:
	D(bug("text.datatype: Dispatcher called (MethodID: DTM_CLEARSELECTED)!\n"));
	return Text__DTM_CLEARSELECTED(cl, o, msg);

    case DTM_COPY:
	D(bug("text.datatype: Dispatcher called (MethodID: DTM_COPY)!\n"));
	return Text__DTM_COPY(cl, o, msg);
	
    case DTM_SELECT: D(bug("text.datatype: Dispatcher called (MethodID: DTM_SELECT)!\n"));
	break;

    case DTM_WRITE:
	D(bug("text.datatype: Dispatcher called (MethodID: DTM_WRITE)!\n"));
	return (ULONG) Text__DTM_WRITE(cl, (struct Gadget *) o, (struct dtWrite *) msg);

    case DTM_PRINT:
	D(bug("text.datatype: Dispatcher called (MethodID: DTM_PRINT)!\n"));
	DT_Print(cl, (struct Gadget *) o, (struct dtPrint *) msg);
	break;

    case DTM_REMOVEDTOBJECT:
    	{
   	    struct Text_Data *td = (struct Text_Data *) INST_DATA(cl, o);
    	    D(bug("text.datatype: Dispatcher called (MethodID: DTM_REMOVEDTOBJECT\n"));
    	    Forbid();
    	    if (td->search_proc)
    	    	Signal(&td->search_proc->pr_Task,SIGBREAKF_CTRL_C);
    	    Permit();

    	    while(td->search_proc) Delay(2);
    	}
    	break;

    case DTM_TRIGGER:
	D(bug("text.datatype: Dispatcher called (MethodID: DTM_TRIGGER)!\n"));
	Text__DTM_TRIGGER(cl,o,(struct dtTrigger*)msg);
	break;

    default:
	D(bug("text.datatype: Dispatcher called (MethodID: %ld=0x%lx)!\n", *msg, *msg));
	return DoSuperMethodA(cl, o, (Msg) msg);
    }

    return 0;

    AROS_USERFUNC_EXIT
}

struct DispatchStruct
{
   struct StackSwapStruct *stk;
   struct IClass *cl;
   Object *o;
   LONG *msg;
};

ASM ULONG DT_Dispatcher3(register __a0 struct DispatchStruct *ds)
{
    register ULONG retval;

    StackSwap(ds->stk);
    retval = DT_Dispatcher2(ds->cl,ds->o,ds->msg);
    StackSwap(ds->stk);
    return retval;
}

ASM ULONG DT_Dispatcher(register __a0 struct IClass *cl, register __a2 Object * o, register __a1 LONG * msg)
{
    struct Task *task;
    putreg(REG_A4, (long) cl->cl_Dispatcher.h_SubEntry);

    task = FindTask(NULL);

    if ((ULONG)getreg(REG_A7) - (ULONG)task->tc_SPLower < 2000)
    {
	struct StackSwapStruct stk;
	struct DispatchStruct ds;
	ULONG retval;

	ds.stk = &stk;
	ds.cl = cl;
	ds.o = o;
	ds.msg = msg;

	if ((stk.stk_Lower = (APTR)AllocVec(12288,0x10000)))
	{
	    stk.stk_Upper = (ULONG)stk.stk_Lower + 12288;
	    stk.stk_Pointer = (APTR)stk.stk_Upper;
	    retval = DT_Dispatcher3(&ds);
	    FreeVec(stk.stk_Lower);
	} else retval = 0;
	return retval;
    } else return DT_Dispatcher2(cl, o, msg);
}



struct IClass *DT_MakeClass(LIBBASETYPEPTR	LIBBASE)
{
    struct IClass *cl = MakeClass("text.datatype", DATATYPESCLASS, NULL, sizeof(struct Text_Data), 0);

    if (cl)
    {
#if defined(__AROS__) || defined(__MORPHOS__)
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) AROS_ASMSYMNAME(DT_Dispatcher);
#else
	cl->cl_Dispatcher.h_Entry = (HOOKFUNC) DT_Dispatcher;
#endif
#ifndef __MORPHOS__
	cl->cl_Dispatcher.h_SubEntry = (HOOKFUNC) getreg(REG_A4);
#endif
	cl->cl_UserData = (ULONG)LIBBASE;	/* Required by datatypes */
    }

    return cl;
}
#endif /* !__AROS */

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
