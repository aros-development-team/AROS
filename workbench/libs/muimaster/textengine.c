/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "textengine.h"
#include "imspec.h"
#include "support.h"
#include "listimage.h"

#include "muimaster_intern.h"

//#define MYDEBUG 1
#include "debug.h"

extern struct Library *MUIMasterBase;

/* A bit of explanation:
 * The most important thing, after the datastructure, is the bounds
 * calculation (especially chunk width).
 * It will determine where to draw. Drawing is then straightforward.
 * From the beginning:
 * the input string is parsed line by line. Each line is formed of chunks
 * of symbols having the same rendering style. As a special case,
 * HiChar is a single letter chunk with underline style.
 * Size calculation is done by calculating for each line the width and
 * height of each chunk, then line width is the sum of widths and line height
 * is the max height. Text width will be max line width, and text height
 * will be the sum of line heights. Remember group layout.
 * Chunk width is affected by soft style of text, and maybe other params.
 * Drawing is done line by line, chunk by chunk, incrementing each time
 * the offsets with chunk width or line height.
 *
 * Italic is looking ugly, no ?
 */

#define ZTL_LEFT    1
#define ZTL_CENTER  2
#define ZTL_RIGHT   3

struct zune_context
{
    LONG dripen;
    LONG pen;
    ULONG style;
    UBYTE align;

    ZTextLine *line;
    CONST_STRPTR text_start;
    CONST_STRPTR text;
    CONST_STRPTR imspec;
    Object       *obj; /* Area subclass, see List_CreateImage */
};

ZText *zune_text_new (CONST_STRPTR preparse, CONST_STRPTR content, int argtype, TEXT argbyte);
char *zune_text_iso_string(ZText *text);
void zune_text_destroy (ZText *text);
void zune_text_chunk_new(struct zune_context *zc);
static int strlenlf(const char *str);
static CONST_STRPTR parse_escape_code (ZTextLine *ztl, struct zune_context *zc, CONST_STRPTR s);
static ZTextLine *zune_text_parse_line (CONST_STRPTR *s, struct zune_context *zc,
					int *argtype, int arg);
void zune_text_get_bounds (ZText *text, Object *obj);
void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top);
void zune_text_draw_cursor (ZText *text, Object *obj, WORD left, WORD right, WORD top,
			    LONG cursorx, LONG cursory);
void zune_text_draw_single (ZText *text, Object *obj, WORD left, WORD right, WORD top,
			    LONG xpos, LONG ypos, BOOL cursor);
int zune_text_get_char_pos(ZText *text, Object *obj, LONG x, LONG y,
			   struct ZTextLine **line_ptr, struct ZTextChunk **chunk_ptr,
			   int *offset_ptr, int *len_ptr);
int zune_text_get_line_len(ZText *text, Object *obj, LONG y);
int zune_get_xpos_of_line(ZText *text, Object *obj, LONG y, LONG xpixel);
int zune_text_get_lines(ZText *text);
int zune_make_cursor_visible(ZText *text, Object *obj, LONG cursorx, LONG cursory,
			     LONG left, LONG top, LONG right, LONG bottom);
int zune_text_merge(ZText *text, Object *obj, int x, int y, ZText *tomerge);


/**************************************************************************
 ...
**************************************************************************/
ZText *zune_text_new (CONST_STRPTR preparse, CONST_STRPTR content, int argtype, TEXT argbyte)
{
    ZText *text;
    /* STRPTR *lines; */
    /* int i; */
    STRPTR dup_content;
    CONST_STRPTR buf;
    int preparse_len;
    struct zune_context zc;
    int arg;

    if (!(text = mui_alloc_struct(ZText))) return NULL;
    NewList((struct List*)&text->lines);

    if (!content) content = "";
    preparse_len = preparse?strlen(preparse):0;
    if (!(dup_content = mui_alloc(preparse_len + strlen(content)+1)))
    {
    	mui_free(text);
	return NULL;
    }

   if (preparse_len) strcpy(dup_content,preparse);
    strcpy(&dup_content[preparse_len],content);

    buf = dup_content;

    zc.pen = -1;
    zc.dripen = -1;
    zc.style = FS_NORMAL;
    zc.align = ZTL_LEFT;
    zc.imspec = NULL;
    zc.obj = NULL;

    if (argtype == ZTEXT_ARG_HICHAR)
    {
    	/* store lower and upper case in arg */
    	arg = ToLower(argbyte)|(ToUpper(argbyte)<<8);
    } else arg = argbyte;

    /* the other elements are done by other functions */

    while (1)
    {
	struct ZTextLine *ztl = zune_text_parse_line(&buf, &zc, &argtype, arg);
	if (ztl) AddTail((struct List*)&text->lines,(struct Node*)ztl);
	else break;
	if (*buf == '\n')
	{
	    buf++;
	    continue;
	} 
	if (!(*buf)) break;
    }

    mui_free(dup_content);
    return text;
}

/**************************************************************************
 Converts a ztext to plain text (iso latin 1) allocated via AllocVec().
 Calling this with NULL returns NULL
**************************************************************************/
char *zune_text_iso_string(ZText *text)
{
    char *iso_text, *buf;
    int len = 0;
    struct ZTextLine *line;
    struct ZTextChunk *chunk;

    if (!text) return NULL;

    /* Count the number needed chars first */
    for (line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ; line = (ZTextLine*)line->node.mln_Succ)
    {
	for (chunk = (ZTextChunk*)line->chunklist.mlh_Head; chunk->node.mln_Succ; chunk = (ZTextChunk*)chunk->node.mln_Succ)
	{
	    len += chunk->str?(strlen(chunk->str)):0;
	}
	len++;
    }

    if (!(iso_text = (char*)AllocVec(len+2,0))) return NULL;
    buf = iso_text;

    /* Now copy the stuff */
    for (line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ; line = (ZTextLine*)line->node.mln_Succ)
    {
	for (chunk = (ZTextChunk*)line->chunklist.mlh_Head; chunk->node.mln_Succ; chunk = (ZTextChunk*)chunk->node.mln_Succ)
	{
	    if (chunk->str)
	    {
	    	strcpy(buf,chunk->str);
	    	buf += strlen(chunk->str);
	    }
	}
	*buf++ = '\n';
    }

    /* remove the last newline */
    if (buf > iso_text) buf[-1] = 0;
    else *buf = 0;

    return iso_text;
}

/**************************************************************************
 Completly frees a ZText
**************************************************************************/
void zune_text_destroy (ZText *text)
{
    struct ZTextLine *ztl;
    struct ZTextChunk *ztc;

    while ((ztl = (struct ZTextLine *)RemTail((struct List*)&text->lines)))
    {
	while ((ztc = (struct ZTextChunk*)RemTail((struct List*)&ztl->chunklist)))
	{
	    if (ztc->str) mui_free(ztc->str);
/*
 *
 *
 *
 *
 *
 * TBF : text_cleanup
 *
 *
 *
 *
 *
 *
 */
#warning do text_cleanup, dammit
	    if (ztc->spec) FreeVec((APTR)ztc->spec);
	    mui_free(ztc);
	}
	mui_free(ztl);
    }
    mui_free(text);	
}

/************************************************************/
/* Parsing */
/************************************************************/


/**************************************************************************
 Allocated and initialize a new text chunk and add it to the list
 The context contains the values for the chunk to be created.
**************************************************************************/
void zune_text_chunk_new(struct zune_context *zc)
{
    ZTextChunk *ztc;

    /* No char has been processed so we needn't to allocate anything */
    if (zc->text == zc->text_start) return;

    if (!(ztc = mui_alloc_struct(ZTextChunk))) return;

    ztc->style = zc->style;
    ztc->dripen = zc->dripen;
    ztc->pen = zc->pen;
    if (zc->imspec)
    {
	D(bug("zune_text_chunk_new: imspec %s\n", zc->imspec));
	ztc->spec = zc->imspec;
	zc->imspec = NULL;

	AddTail((struct List*)&zc->line->chunklist,(struct Node*)ztc);
    }
    else if (zc->obj)
    {
	ztc->obj = zc->obj;
	zc->obj = NULL;

	AddTail((struct List*)&zc->line->chunklist,(struct Node*)ztc);
    }
    else if ((ztc->str = (char*)mui_alloc(zc->text - zc->text_start + 1)))
    {
	strncpy(ztc->str, zc->text_start, zc->text - zc->text_start + 1);
	ztc->str[zc->text - zc->text_start] = 0;
	D(bug("zune_text_chunk_new: string %s\n", ztc->str));

	AddTail((struct List*)&zc->line->chunklist,(struct Node*)ztc);
    }
    else
    {
	mui_free(ztc);
    }
}


/**************************************************************************
 Calculates the length of the string exluding line feed or 0 byte
**************************************************************************/
static int strlenlf(const char *str)
{
    char c;
    int len = 0;
    while ((c = *str))
    {
	if (c=='\n') break;
	len++;
    }
    return len;
}

/**************************************************************************
 Note: Only aligments at the beginning of a line should affect this line
 (tested in MUI)
**************************************************************************/
static CONST_STRPTR parse_escape_code (ZTextLine *ztl, struct zune_context *zc, CONST_STRPTR s)
{
    unsigned char c;
    c = *s;

    zune_text_chunk_new(zc);
    s++; /* s now points after the command */
    zc->text_start = zc->text = s;

    switch (c)
    {
	case 'c': zc->align = ztl->align = ZTL_CENTER; break;
	case 'r': zc->align = ztl->align = ZTL_RIGHT;  break;
	case 'l': zc->align = ztl->align = ZTL_LEFT; break;
	case 'n': zc->style = FS_NORMAL; break;
	case 'u': zc->style |= FSF_UNDERLINED; break;
	case 'b': zc->style |= FSF_BOLD; break;
	case 'i': zc->style |= FSF_ITALIC; break;
	case 'I': /* image spec */
	{
	    char *t;

	    if (*s != '[')
		break;
	    s++;
	    /* s points on the first char of imagespec.
	     *  Extract it to the trailing ']'.
	     */
	    t = strchr(s, ']');
	    if (t == NULL)
		break;
	    *t = 0;
	    D(bug("imspec = %s\n", s));
	    zc->imspec = StrDup(s);
	    *t = ']';
	    zc->text = t;
	    zune_text_chunk_new(zc);
	    zc->text_start = t + 1;	    
	}   
	case 'O': /* pointer from List_CreateImage */
	{
	    struct ListImage *li;
	    ULONG tmp;
	    char *t;

	    if (*s != '[')
		break;
	    s++;
	    /* s points on the first char of pointer printed as %08lx.
	     *  Extract it to the trailing ']'.
	     */
	    t = strchr(s, ']');
	    if (t == NULL)
		break;
	    *t = 0;
	    if (sscanf(s, "%lx", &tmp) == 1)
	    {
		li = (struct ListImage *)tmp;
		D(bug("listimage = %lx\n", li));
		zc->obj = li->obj;
	    }
	    *t = ']';
	    zc->text = t;
	    zune_text_chunk_new(zc);
	    zc->text_start = t + 1;	    
	    break;
	}
	case 'P': /* pen number */
	{
	    LONG pen;
	    char *t;

	    if (*s != '[')
		break;
	    s++;
	    /* s points on the first char of pen from ObtainPen printed as %ld.
	     *  Extract it to the trailing ']'.
	     */
	    t = strchr(s, ']');
	    if (t == NULL)
		break;
	    *t = 0;
	    if (sscanf(s, "%ld", &pen) == 1)
	    {
		D(bug("pen = %ld\n", pen));
		zc->pen = pen;
	    }
	    *t = ']';
	    zc->text = t;
	    zune_text_chunk_new(zc);
	    zc->text_start = t + 1;	    
	    break;
	}
	case '-': zc->text += strlenlf(s); break; /* disable engine */

	default: /* some other ESC code ? */
	    if (isdigit(c)) /* pen */
	    {
		zc->dripen = c - '0';
	    }
	    break;
    }
    return zc->text;
}

/**************************************************************************
 Parse a text line, and create text chunks.
 Whenever an escape code is encountered, the current chunk
 is terminated, and the escape parameters are stacked until
 a normal character is read. Then a new chunk begins with this char.

 s_ptr is a pointer to the text string. After this function has been
 executed it is changed to point to the end of the string (eighter the '\n'
 or 0 byte)

 This is probably a function to rewrite to deal with wide chars.

 Note that the contents in s_ptr is overwritten.
**************************************************************************/
static ZTextLine *zune_text_parse_line (CONST_STRPTR *s_ptr, struct zune_context *zc, int *argtype, int arg)
{
    CONST_STRPTR s;
    UBYTE c;

    ZTextLine *ztl;

    if (!s_ptr) return NULL;
    if (!(s = *s_ptr)) return NULL;
    if (!(ztl = mui_alloc_struct(ZTextLine))) return NULL;
    NewList((struct List*)&ztl->chunklist);

    ztl->align = zc->align;
    zc->text_start = zc->text = s;
    zc->line = ztl;

    while ((c = *s))
    {
    	if (c == '\n') break;

    	if (c == '\33')
    	{
    	    s++;
	    if (*s == 0) break;
	    s = parse_escape_code(ztl, zc, s);
	    continue;
	}

        if (*argtype == ZTEXT_ARG_HICHAR && (((arg & 0xff) == c) || (((arg >> 8)&0xff) == c)))
        {
	    ULONG styleback = zc->style;
	    zune_text_chunk_new(zc);
	    zc->style |= FSF_UNDERLINED;
	    zc->text_start = s;
	    zc->text = ++s;
	    zune_text_chunk_new(zc);
	    zc->text_start = s;
	    zc->style = styleback;
	    *argtype = ZTEXT_ARG_NONE;
	    continue;
	}

	if (*argtype == ZTEXT_ARG_HICHARIDX && arg == c)
	{
	    ULONG styleback = zc->style;
	    /* underline next char */
	    zune_text_chunk_new(zc);
	    zc->style |= FSF_UNDERLINED;
	    zc->text_start = ++s;
	    zc->text = ++s;
	    zune_text_chunk_new(zc);
	    zc->text_start = s;
	    zc->style = styleback;
	    *argtype = ZTEXT_ARG_NONE;
	}

	zc->text = ++s;
    } /* while */
    zune_text_chunk_new(zc);
    *s_ptr = s;
    return ztl;
}


/************************************************************/
/* Bounds */
/************************************************************/

void zune_text_get_bounds (ZText *text, Object *obj)
{
    struct RastPort rp;
    struct TextFont *font;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    text->width = 0;
    text->height = 0;

    font = _font(obj);
    InitRastPort(&rp);
    SetFont(&rp, font);

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	line_node->lheight = font->tf_YSize;
	line_node->lwidth = 0;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    if (chunk_node->spec)
	    {
		struct MUI_MinMax minmax;

		chunk_node->image = zune_imspec_setup((IPTR)chunk_node->spec, muiRenderInfo(obj));
		if (!chunk_node->image)
		    return;
		zune_imspec_askminmax(chunk_node->image, &minmax);
		chunk_node->cwidth = minmax.DefWidth;
		chunk_node->cheight = minmax.DefHeight;
		line_node->lheight = MAX(line_node->lheight, chunk_node->cheight);
	    }
	    else if (chunk_node->obj)
	    {
		struct MUI_MinMax MinMax;

		DoMethod(chunk_node->obj, MUIM_AskMinMax, (IPTR)&MinMax);
		chunk_node->cwidth = MinMax.DefWidth;
		chunk_node->cheight = MinMax.DefHeight;
		line_node->lheight = MAX(line_node->lheight, chunk_node->cheight);
	    }
	    else if (chunk_node->str)
	    {
		chunk_node->cheight = font->tf_YSize;
	        chunk_node->cwidth = TextLength(&rp,chunk_node->str,strlen(chunk_node->str));
		D(bug("zune_text_get_bounds(%s,%x) => cwidth=%d\n", chunk_node->str, obj, chunk_node->cwidth));
	    }
	    line_node->lwidth += chunk_node->cwidth;
	}

	text->height += line_node->lheight;
	text->width = MAX(text->width,line_node->lwidth);
    }
    
    DeinitRastPort(&rp);
}

/************************************************************/
/* Drawing                                                  */
/************************************************************/

// problem is cheight being seldom 0

void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top)
{
    struct RastPort *rp;
    ULONG pensave;
    ULONG style = FS_NORMAL;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    D(bug("zune_text_draw(%p) %d %d %d [ys=%d]\n", obj, left, right, top, text->yscroll));

    rp = _rp(obj);
    SetFont(rp,_font(obj));
    SetSoftStyle(rp, style, AskSoftStyle(rp));
    
    top += text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right + 1 - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth + 1;
	else x = left;
/*  	D(bug("zune_text_draw(%x,%d,%d,%d, align=%d) : x = %d\n", obj, left, right, line_node->lwidth, line_node->align, x)); */
	x += text->xscroll;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    /*
	     * Show/Hide stuff should be put in new api calls
	     */
	    if (chunk_node->image)
	    {
		WORD top_im = top;
		top_im += (line_node->lheight - chunk_node->cheight) / 2;
		zune_imspec_show(chunk_node->image, obj);
		pensave = GetAPen(rp);
		zune_imspec_draw(chunk_node->image, muiRenderInfo(obj), x,
				top_im, chunk_node->cwidth,
				chunk_node->cheight, 0, 0, 0);
		SetAPen(rp, pensave);
		zune_imspec_hide(chunk_node->image);
	    }
	    else if (chunk_node->obj)
	    {
		_left(chunk_node->obj) = x;
		_top(chunk_node->obj) = top + (line_node->lheight - chunk_node->cheight) / 2;
		_width(chunk_node->obj) = chunk_node->cwidth;
		_height(chunk_node->obj) = chunk_node->cheight;
		DoMethod(chunk_node->obj, MUIM_Show);
		pensave = GetAPen(rp);
		MUI_Redraw(chunk_node->obj, MADF_DRAWOBJECT);
		SetAPen(rp, pensave);
		DoMethod(chunk_node->obj, MUIM_Hide);
	    }
	    else if (chunk_node->str)
	    {
                if (chunk_node->style != style)
		{
		    SetSoftStyle(rp, chunk_node->style, 0xff);
		    style = chunk_node->style;
		}
		if (chunk_node->cheight == 0)
		    Move(rp,x,_font(obj)->tf_Baseline + top);
		else
		    Move(rp,x,_font(obj)->tf_Baseline + (line_node->lheight - chunk_node->cheight) / 2 + top);
#if 0
		D(bug("zune_text_draw(%p) Moved to %d (baseline=%d, lh=%d, ch=%d, top=%d)\n",
		      obj, _font(obj)->tf_Baseline + (line_node->lheight - chunk_node->cheight) / 2 + top,
		      _font(obj)->tf_Baseline, line_node->lheight, chunk_node->cheight, top));
#endif
		if (chunk_node->dripen != -1)
		{
		    D(bug("chunk_node->dripen == %d\n", chunk_node->dripen));
		    SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);
		}
		else if (chunk_node->pen != -1)
		{
		    D(bug("chunk_node->pen == %d\n", chunk_node->pen));
		    SetABPenDrMd(rp, chunk_node->pen, 0, JAM1);
		}
		else
		{
		    SetDrMd(rp, JAM1);
		}
	    	Text(rp,chunk_node->str,strlen(chunk_node->str));
	    }
	    x += chunk_node->cwidth;
	}
	top += line_node->lheight;
    }
}

void zune_text_draw_cursor (ZText *text, Object *obj, WORD left, WORD right, WORD top, LONG cursorx, LONG cursory)
{
    struct RastPort *rp;
    ULONG style = FS_NORMAL;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    rp = _rp(obj);
    SetFont(rp,_font(obj));
    SetSoftStyle(rp, style, AskSoftStyle(rp));

    top += _font(obj)->tf_Baseline + text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right + 1 - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth + 1;
	else x = left;

	x += text->xscroll;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    char *str = chunk_node->str;

	    if (str == NULL)
		str = "";

	    if (chunk_node->style != style)
	    {
		SetSoftStyle(rp, chunk_node->style, 0xff);
		style = chunk_node->style;
	    }

	    if (!cursory && cursorx != -1)
	    {
	    	if (cursorx < strlen(str) && cursorx >= 0)
		{
	    	    int offx = TextLength(_rp(obj),str,cursorx);
	    	    int cursor_width;

		    if (str[cursorx]) cursor_width = TextLength(_rp(obj),&str[cursorx],1);
		    else cursor_width = _font(obj)->tf_XSize;

		    SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
		    D(bug("set FILLPEN a\n"));
		    RectFill(_rp(obj), x + offx, top - _font(obj)->tf_Baseline, x + offx + cursor_width - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
		    cursorx = -1;
		} else cursorx -= strlen(chunk_node->str);
	    }
	    
	    Move(rp,x,top);
	    if (chunk_node->dripen != -1)
	    {
		D(bug("chunk_node->dripen == %d\n", chunk_node->dripen));
		SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);
	    }
	    else if (chunk_node->pen != -1)
	    {
		D(bug("chunk_node->pen == %d\n", chunk_node->pen));
		SetABPenDrMd(rp, chunk_node->pen, 0, JAM1);
	    }
	    else
	    {
		SetABPenDrMd(rp, _dri(obj)->dri_Pens[TEXTPEN], 0, JAM1);
	    }
	    Text(rp,chunk_node->str,strlen(chunk_node->str));
	    x += chunk_node->cwidth;
	}

	if (!cursory && cursorx != -1)
	{
	    /* Cursor has not been drawn yet */
	    SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
	    D(bug("set FILLPEN\n"));
	    RectFill(_rp(obj), x, top - _font(obj)->tf_Baseline, x + _font(obj)->tf_XSize - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
	}

	top += line_node->lheight;
	cursory--;
    }
}

void zune_text_draw_single (ZText *text, Object *obj, WORD left, WORD right, WORD top, LONG xpos, LONG ypos, BOOL cursor)
{
    struct RastPort *rp;
    ULONG style = FS_NORMAL;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    rp = _rp(obj);
    SetFont(rp,_font(obj));
    SetSoftStyle(rp, style, AskSoftStyle(rp));

    top += _font(obj)->tf_Baseline + text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right + 1 - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth + 1;
	else x = left;

	x += text->xscroll;

	if (!ypos)
	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    char *str = chunk_node->str;
	    int offx;
	    int cursor_width;

	    if (!str) str = "";

	    if (xpos >= strlen(str))
	    {
		x += chunk_node->cwidth;
		xpos -= strlen(str);
		continue;
	    }

	    offx = TextLength(_rp(obj),str,xpos);
	    cursor_width = TextLength(rp,&chunk_node->str[xpos],1);

	    if (chunk_node->style != style)
	    {
		SetSoftStyle(rp, chunk_node->style, 0xff);
		style = chunk_node->style;
	    }

	    if (cursor)
	    {
		SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
		D(bug("set FILLPEN\n"));
		RectFill(_rp(obj), x + offx, top - _font(obj)->tf_Baseline, x + offx + cursor_width - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
	    } else
	    {
		DoMethod(obj, MUIM_DrawBackground, x + offx, top - _font(obj)->tf_Baseline, cursor_width, _font(obj)->tf_YSize);
	    }
	    
	    Move(rp,x + offx, top);

	    if (chunk_node->dripen != -1)
	    {
		D(bug("chunk_node->dripen == %d\n", chunk_node->dripen));
		SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);
	    }
	    else if (chunk_node->pen != -1)
	    {
		D(bug("chunk_node->pen == %d\n", chunk_node->pen));
		SetABPenDrMd(rp, chunk_node->pen, 0, JAM1);
	    }
	    else
	    {
		SetABPenDrMd(rp, _dri(obj)->dri_Pens[TEXTPEN], 0, JAM1);
	    }

	    Text(rp,&chunk_node->str[xpos],1);
	    return;
	}

#if 0
	if (!cursory && cursorx != -1)
	{
	    /* Cursor has not been drawn yet */
	    SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
	    RectFill(_rp(obj), x, top - _font(obj)->tf_Baseline, x + _font(obj)->tf_XSize - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
	}
#endif

	top += line_node->lheight;
	ypos--;
    }
}

int zune_text_get_char_pos(ZText *text, Object *obj, LONG x, LONG y, struct ZTextLine **line_ptr, struct ZTextChunk **chunk_ptr, int *offset_ptr, int *len_ptr)
{
    int i;
    struct ZTextLine *line;
    struct ZTextChunk *chunk;

    /* find the line */
    for (i=0,line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ && i<y; line = (ZTextLine*)line->node.mln_Succ,i++);

    if (!line->node.mln_Succ) return 0;
    *line_ptr = line;
    if (chunk_ptr) *chunk_ptr = NULL;
    *offset_ptr = 0;
    *len_ptr = 0;

    for (chunk = (ZTextChunk*)line->chunklist.mlh_Head; chunk->node.mln_Succ; chunk = (ZTextChunk*)chunk->node.mln_Succ)
    {
    	int len = chunk->str?(strlen(chunk->str)):0;
    	if (len >= x)
    	{
    	    break;
    	} else
    	{
	    x -= len;
	    *offset_ptr += chunk->cwidth;
    	}
    }

    if (chunk->node.mln_Succ)
    {
	struct RastPort rp;
	if (chunk_ptr) *chunk_ptr = chunk;
	InitRastPort(&rp);
	SetFont(&rp,_font(obj));
	*len_ptr = x;
	*offset_ptr += TextLength(&rp,chunk->str,x);
	DeinitRastPort(&rp);
    }

    *offset_ptr += text->xscroll;

    return 1;
}

int zune_text_get_line_len(ZText *text, Object *obj, LONG y)
{
    int i,len=0;
    struct ZTextLine *line;
    struct ZTextChunk *chunk;

    /* find the line */
    for (i=0,line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ && i<y; line = (ZTextLine*)line->node.mln_Succ,i++);

    if (!line->node.mln_Succ) return 0;

    for (chunk = (ZTextChunk*)line->chunklist.mlh_Head; chunk->node.mln_Succ; chunk = (ZTextChunk*)chunk->node.mln_Succ)
    {
    	len += chunk->str?(strlen(chunk->str)):0;
    }
    return len;
}

int zune_get_xpos_of_line(ZText *text, Object *obj, LONG y, LONG xpixel)
{
    int i,xpos=0;
    struct ZTextLine *line;
    struct ZTextChunk *chunk;

    /* find the line */
    for (i=0,line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ && i<y; line = (ZTextLine*)line->node.mln_Succ,i++);

    if (!line->node.mln_Succ) return -1;

    for (chunk = (ZTextChunk*)line->chunklist.mlh_Head; chunk->node.mln_Succ; chunk = (ZTextChunk*)chunk->node.mln_Succ)
    {
    	if (xpixel < chunk->cwidth)
    	{
	    struct RastPort rp;
	    struct TextExtent te;
	    InitRastPort(&rp);
	    SetFont(&rp,_font(obj));

	    xpos += TextFit(&rp,chunk->str,strlen(chunk->str),&te,NULL,1,xpixel,_font(obj)->tf_YSize);
	    DeinitRastPort(&rp);
	    return xpos;
    	}
    	xpixel -= chunk->cwidth;
    	xpos += strlen(chunk->str);
    }
    return xpos;
}

int zune_text_get_lines(ZText *text)
{
    int i;
    struct ZTextLine *line;

    for (i=0,line = (ZTextLine *)text->lines.mlh_Head; line->node.mln_Succ; line = (ZTextLine*)line->node.mln_Succ,i++);
    return i;
}

int zune_make_cursor_visible(ZText *text, Object *obj, LONG cursorx, LONG cursory, LONG left, LONG top, LONG right, LONG bottom)
{
    struct RastPort rp;
    LONG oldxscroll,oldyscroll;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return 0;

    oldxscroll = text->xscroll;
    oldyscroll = text->yscroll;

    InitRastPort(&rp);
    SetFont(&rp,_font(obj));

    top += _font(obj)->tf_Baseline;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right + 1 - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth + 1;
	else x = left;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    char *str = chunk_node->str;
	    if (!str) str = "";

	    if (!cursory && (cursorx <= strlen(str) && cursorx >= 0))
	    {
	    	int offx = TextLength(&rp,str,cursorx);
	    	int cursor_width;

		if (str[cursorx]) cursor_width = TextLength(&rp,&str[cursorx],1);
		else cursor_width = _font(obj)->tf_XSize;

		if ((x + offx + text->xscroll) < left) text->xscroll = left - x - offx;
		else if ((x + offx + cursor_width - 1 + text->xscroll) > right) text->xscroll = right - x - offx - cursor_width + 1;

		cursorx = -1;
	    }
	    
	    x += chunk_node->cwidth;
	}

	if (!cursory && cursorx != -1)
	{
	    int cursor_width = _font(obj)->tf_XSize;
	    /* Cursor has not been drawn yet */

	    if ((x  + text->xscroll) < left) text->xscroll = left - x;
	    else if ((x + cursor_width - 1 + text->xscroll) > right) text->xscroll = right - x - cursor_width + 1;
	}

	top += line_node->lheight;
	cursory--;
    }
    
    DeinitRastPort(&rp);
    
    return (text->xscroll != oldxscroll || text->yscroll != oldyscroll);
}

int zune_text_merge(ZText *text, Object *obj, int x, int y, ZText *tomerge)
{
    int offset, len;
    ZTextLine *line, *line_tomerge;
    ZTextChunk *chunk, *chunk_tomerge;
    ZTextChunk *chunk_new;
    struct MinList store;

    if (!zune_text_get_char_pos(text, obj, x, y, &line, &chunk, &offset, &len)) return 0;
    if (!(line_tomerge = List_First(&tomerge->lines))) return 0;
    if (!(chunk_new = mui_alloc_struct(struct ZTextChunk))) return 0;
    memset(chunk_new,0,sizeof(struct ZTextChunk));

    /* Split chunk at (x,y) into two */
    if (!(chunk_new->str = mui_alloc(strlen(&chunk->str[len])+1)))
    {
	mui_free(chunk_new);
	return 0;
    }
    strcpy(chunk_new->str,&chunk->str[len]);
    chunk_new->dripen = chunk->dripen;
    chunk_new->pen = chunk->pen;
    chunk_new->style = chunk->style;
    chunk->str[len] = 0; /* set new string end */

    NewList((struct List*)&store);
    AddTail((struct List*)&store,(struct Node*)chunk_new);

    /* Append all following chunks at the newly created chunk */
    chunk = (ZTextChunk*)Node_Next(chunk);
    while (chunk)
    {
	ZTextChunk *chunk_next;
	chunk_next = (ZTextChunk*)Node_Next(chunk);
	Remove((struct Node*)chunk);
	AddTail((struct List*)&store,(struct Node*)chunk);
	chunk = chunk_next;
    }

    while ((chunk_tomerge = (ZTextChunk*)RemHead((struct List*)&line_tomerge->chunklist)))
	AddTail((struct List*)&line->chunklist,(struct Node*)chunk_tomerge);

    Remove((struct Node*)line_tomerge);

    while ((line_tomerge = (ZTextLine*)RemHead((struct List*)&tomerge->lines)))
    {
	Insert((struct List*)&text->lines,(struct Node*)line_tomerge,(struct Node*)line);
	line = line_tomerge;
    }

    while ((chunk = (ZTextChunk*)RemHead((struct List*)&store)))
    {
	AddTail((struct List*)&line->chunklist,(struct Node*)chunk);
    }

    zune_text_get_bounds(text,obj);

    return 1;
}

