/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/alib_protos.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include "mui.h"
#include "textengine.h"
#include "support.h"

#include "muimaster_intern.h"

extern struct Library *MUIMasterBase;

static struct MinNode *Node_Next(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Succ->mln_Succ == NULL)
		return NULL;
    return ((struct MinNode*)node)->mln_Succ;
}

static struct MinNode *Node_Prev(APTR node)
{
    if(node == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred == NULL) return NULL;
    if(((struct MinNode*)node)->mln_Pred->mln_Pred == NULL)
	return NULL;
    return ((struct MinNode*)node)->mln_Pred;
}

static struct MinNode *List_First(APTR list)
{
    if( !((struct MinList*)list)->mlh_Head) return NULL;
    if(((struct MinList*)list)->mlh_Head->mln_Succ == NULL) return NULL;
    return ((struct MinList*)list)->mlh_Head;
}

static struct MinNode *List_Last(APTR list)
{
    if( !((struct MinList*)list)->mlh_TailPred) return NULL;
    if(((struct MinList*)list)->mlh_TailPred->mln_Pred == NULL) return NULL;
    return ((struct MinList*)list)->mlh_TailPred;
}

#define MAX(a,b) ((a)>(b)?(a):(b))

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

#if 0

typedef struct ZTextLine {
    struct MinNode node; /* embedded node */
    UBYTE  align;
    struct MinList chunklist;
    WORD   lwidth;
    WORD   lheight;
} ZTextLine;

#endif

#define ZTL_LEFT    1
#define ZTL_CENTER  2
#define ZTL_RIGHT   3

#define DEFAULT_TEXT_ALIGN ZTL_LEFT

/*-------------*/

#if 0

typedef struct ZTextChunk {
    struct MinNode node; /* embedded node */
    char                 *str;
//    struct MUI_ImageSpec *image;
    LONG                  dripen;
    UBYTE                 style;

    UBYTE                 longer_underline;

    WORD                  deltabearing; /* width bearing - right bearing */
    WORD                  cwidth;
    WORD                  cheight;
    WORD                  lbearing;
    WORD                  lbear2;
} ZTextChunk;

#endif

#define ZTC_STYLE_BOLD      (1<<0)
#define ZTC_STYLE_ITALIC    (1<<1)
#define ZTC_STYLE_UNDERLINE (1<<2)
#define ZTC_STYLE_NORMAL    (1<<3)

#define ITALIC_RATIO 5

#define NEED_LONG_UNDERLINE 10000

/*-------------*/

struct line_bounds_datas {
    ZText *text;
    Object *obj;
};

struct chunk_bounds_datas {
    ZTextLine *line;
    struct line_bounds_datas *lbd;
};

/*-------------*/

struct line_draw_datas {
    ZText *text;
    Object *obj;
    WORD left;
    WORD right;
    WORD top;
    WORD yoffset; /* incremented by each line draw */
};

struct chunk_draw_datas {
    ZTextLine *line;
    struct line_draw_datas *ldd;
    WORD xoffset; /* incremented by each chunk draw */
};

/*-------------*/

struct line_pos_datas {
    ZText *text;
    Object *obj;
    WORD left;
    WORD right;
    WORD *realleft;
    WORD *realright;
};

/** Variables **********************/

//static GMemChunk *textMemChunk = NULL;
//static GMemChunk *lineMemChunk = NULL;
//static GMemChunk *chunkMemChunk = NULL;

/** Declarations **********************/

struct zune_context;

static ZTextLine *zune_text_parse_line (STRPTR *s, struct zune_context *zc,
					int *argtype, int arg);


/************************/

#if 0
static void
destroy_memchunk (void)
{
    if (textMemChunk)
    {
	g_mem_chunk_destroy(textMemChunk);
	textMemChunk = NULL;
    }

    if (lineMemChunk)
    {
	g_mem_chunk_destroy(lineMemChunk);
	lineMemChunk = NULL;
    }

    if (chunkMemChunk)
    {
	g_mem_chunk_destroy(chunkMemChunk);
	chunkMemChunk = NULL;
    }
}
#endif

struct zune_context
{
    LONG pen;
    ULONG style;
    UBYTE align;

    ZTextLine *line;
    char *text_start;
    char *text;
};

/**************************************************************************
 ...
**************************************************************************/
ZText *zune_text_new (STRPTR preparse, STRPTR content, int argtype, TEXT argbyte)
{
    ZText *text;
    /* STRPTR *lines; */
    /* int i; */
    char *dup_content, *buf;
    int preparse_len;
    struct zune_context zc;
    int arg;

    if (!(text = mui_alloc_struct(ZText))) return NULL;
    text->style = ZTC_STYLE_NORMAL;
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

    zc.pen = TEXTPEN;
    zc.style = ZTC_STYLE_NORMAL;
    zc.align = ZTL_LEFT;

    if (argtype == ZTEXT_ARG_HICHAR)
    {
    	/* store lower and upper case in arg */
    	arg = ToLower(argbyte)|(ToUpper(argbyte)<<8);
    } else arg = argbyte;

    /* the other elements are done by other functions */

    while (1)
    {
	struct ZTextLine *ztl = zune_text_parse_line((STRPTR *)&buf, &zc, &argtype, arg);
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
**************************************************************************/
void zune_text_chunk_new(struct zune_context *zc)
{
    ZTextChunk *ztc;

    /* No char has been processed so we needn't to allocate anything */
    if (zc->text == zc->text_start) return;

    if (!(ztc = mui_alloc_struct(ZTextChunk))) return;

    ztc->style = zc->style;
    ztc->dripen = zc->pen;
    if ((ztc->str = (char*)mui_alloc(zc->text - zc->text_start + 1)))
    {
	strncpy(ztc->str, zc->text_start, zc->text - zc->text_start + 1);
	ztc->str[zc->text - zc->text_start] = 0;

	AddTail((struct List*)&zc->line->chunklist,(struct Node*)ztc);
    }
}


/**************************************************************************
 Calculates the length of the string exluding line feed or 0 byte
**************************************************************************/
static int strlenlf(char *str)
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
static STRPTR parse_escape_code (ZTextLine *ztl, struct zune_context *zc, STRPTR s)
{
    unsigned char c;
    c = *s++;

    zune_text_chunk_new(zc);
    zc->text_start = zc->text = s;

    switch (c)
    {
	case 'c': zc->align = ztl->align = ZTL_CENTER; break;
	case 'r': zc->align = ztl->align = ZTL_RIGHT;  break;
	case 'l': zc->align = ztl->align = ZTL_LEFT; break;
	case 'n': zc->style = ZTC_STYLE_NORMAL; break;
	case 'u': zc->style |= ZTC_STYLE_UNDERLINE; break;
	case 'b': zc->style |= ZTC_STYLE_BOLD; break;
	case 'i': zc->style |= ZTC_STYLE_ITALIC; break;
	case 'I': /*  *(s+1) = '['  */ break;
	case '-': zc->text += strlenlf(s); break; /* disable engine */

	default: /* some other ESC code ? */
	    if (isdigit(c)) /* pen */
	    {
		zc->pen = c - '0';
	    }
	    break;
    }
    return zc->text;
}



#if 0
	    default:
		if (*argtype == ZTEXT_ARG_HICHAR && arg == *s)
		{
		    /* underline this char */
		    int styleback = *current_style;

		    if (ztc) *current_style = ztc->style | ZTC_STYLE_UNDERLINE;
		    else *current_style |= ZTC_STYLE_UNDERLINE;
		    *current_style &= ~ZTC_STYLE_NORMAL;

		    if ((ztc = zune_text_chunk_new (*current_style, pen)))
		    {
			AddTail((struct List*)&ztl->chunklist,(struct Node*)ztc);

		    g_string_append_c (ztc->str, *s);
		    ztc = NULL;
		    *current_style = styleback;
		    *argtype = ZTEXT_ARG_NONE;
		    break;
		}
		else if (*argtype == ZTEXT_ARG_HICHARIDX && arg == *s)
		{
		    /* underline next char */
		    s++;
		    if (*s)
		    {
			int styleback = *current_style;
			if (ztc)
			{
			    styleback = ztc->style;
			    *current_style = ztc->style | ZTC_STYLE_UNDERLINE;
			}
			else
			    *current_style |= ZTC_STYLE_UNDERLINE;
			*current_style &= ~ZTC_STYLE_NORMAL;
			ztc = zune_text_chunk_new (*current_style, pen);
			ztl->chunks = g_list_append(ztl->chunks, ztc);
			g_string_append_c (ztc->str, *s);
			ztc = NULL;
			*current_style = styleback;			
		    }
		    *argtype = ZTEXT_ARG_NONE;
		    break;
		}

		if (!ztc)
		{
		    ztc = zune_text_chunk_new (*current_style, pen);
		    ztl->chunks = g_list_append(ztl->chunks, ztc);
		}
		g_string_append_c (ztc->str, *s);

		if (ztc->style & ZTC_STYLE_BOLD)
		{
/* bold chunks have an additional width, so we must have a chunk
 * for each character to avoid eating character interspacing
 * when writing a bold word.
 */
/*  		    g_print("single bold chunk <%c>\n", *s); */
		    ztc = NULL;
		}
		break;
	} /* switch */
#endif


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
static ZTextLine *zune_text_parse_line (STRPTR *s_ptr, struct zune_context *zc, int *argtype, int arg)
{
    STRPTR s;
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
	    zc->style |= ZTC_STYLE_UNDERLINE;
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
	    zc->style |= ZTC_STYLE_UNDERLINE;
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

/* Oh boy... why does TextExtent() require a RastPort
 * when it only needs the TextFont? */

#if 0

#if defined(_AROS) || defined(_AMIGA)
void myTextExtent(struct TextFont *tf, STRPTR string, ULONG count,
                  struct TextExtent *te)
{
    WORD len;

    if (tf->tf_Flags & FPF_PROPORTIONAL)
    {
    	WORD  idx;
	WORD  defaultidx = ((tf->tf_HiChar - tf->tf_LoChar) + 1); /* Last glyph is the default glyph */
	UBYTE c;
	
	for (len = 0; count; count--)
	{
	    c = *string++;
	    
	    if ( c < tf->tf_LoChar || c > tf->tf_HiChar)
	    {
		idx = defaultidx;
	    }
	    else
	    {
		idx = c - tf->tf_LoChar;
	    }
	    	    
   	    len += ((WORD *)tf->tf_CharKern)[idx];
	    len += ((WORD *)tf->tf_CharSpace)[idx];
	}
    }
    else
    {
    	len = count * tf->tf_XSize;
    }

    te->te_Width  = len;
    te->te_Height = tf->tf_YSize;
    te->te_Extent.MinX = 0;
    te->te_Extent.MinY = -tf->tf_Baseline;
    te->te_Extent.MaxX = te->te_Width  - 1;
    te->te_Extent.MaxY = te->te_Height - 1 - tf->tf_Baseline;
}
#endif /* _AROS */


/* trailing spaces are not correctly handled, thus the hack which
 * adds a final dot for the calculation  .
 */
static void calculate_chunk_bounds (gpointer ch, gpointer udata)
{
    struct chunk_bounds_datas *cbd = (struct chunk_bounds_datas *)udata;
    ZTextChunk *chunk = (ZTextChunk *)ch;
    struct TextExtent te;
    int lbearing;
    int rbearing;
    int width;

    chunk->cwidth = 0;
    chunk->cheight = 0;
    if (chunk->str)
    {
        GList *next;
	GList *prev;
        GList *me = g_list_find(cbd->line->chunks, chunk);
        g_return_if_fail(me != NULL);
        next = g_list_next(me);
	prev = g_list_previous(me);

	cbd->lbd->text->style = chunk->style;
/*  	g_print("chunk <%s>\n", chunk->str->str); */

#ifndef _AROS
	chunk->cheight += font->ascent + font->descent;

	gdk_text_extents (font,
			  chunk->str->str, chunk->str->len,
			  &lbearing, &rbearing, &width, NULL, NULL);
#else
kprintf(">>> calculate_chunk_bounds: TextExtent(...)\n");
kprintf(" >>  obj=%lx font=%lx\n", cbd->lbd->obj, _font(cbd->lbd->obj));
	myTextExtent(_font(cbd->lbd->obj),
		chunk->str->str, chunk->str->len, &te);

	chunk->cheight += te.te_Height;
	lbearing = te.te_Extent.MinX;
	rbearing = te.te_Extent.MaxX;
	width    = te.te_Width;
#endif

	chunk->lbear2 = lbearing;

	if (!next) /* last chunk of a line */
	{
	    chunk->cwidth += rbearing;
	    if (cbd->lbd->text->style & ZTC_STYLE_ITALIC)
		chunk->cwidth += chunk->cheight / ITALIC_RATIO;
	}
	else
	{
	    chunk->cwidth += width;
	}

	if (cbd->lbd->text->style & ZTC_STYLE_BOLD)
	    chunk->cwidth += 1;

	if (cbd->lbd->text->style & ZTC_STYLE_UNDERLINE)
	{
	    chunk->deltabearing = width - rbearing;
	    if (next)
	    {
		if (((ZTextChunk *)next->data)->style != ZTC_STYLE_NORMAL
		    && (((ZTextChunk *)next->data)->style == 0
			|| ((ZTextChunk *)next->data)->style & ZTC_STYLE_UNDERLINE))
		    chunk->longer_underline = TRUE;
	    }
	}


	if (!prev && lbearing > 0) /* get rid extra space on line beginning */
	{
	    chunk->cwidth -= lbearing;
	    chunk->lbearing = lbearing;
	}
    }
#ifndef _AROS
    else if (chunk->image)
    {
    }
#endif

    cbd->line->lheight = MAX(cbd->line->lheight, chunk->cheight);
    cbd->line->lwidth += chunk->cwidth;
}

static void calculate_line_bounds (gpointer l, gpointer udata)
{
    struct chunk_bounds_datas data;

    struct line_bounds_datas *lbd = (struct line_bounds_datas *)udata;
    ZTextLine *line = (ZTextLine *)l;

    line->lwidth = 0;
    line->lheight = 0;
    data.line = line;
    data.lbd = lbd;

    g_list_foreach(line->chunks, calculate_chunk_bounds, &data);
    lbd->text->height += line->lheight;
    lbd->text->width = MAX(lbd->text->width, line->lwidth);
}
#endif

void zune_text_get_bounds (ZText *text, Object *obj)
{
    struct RastPort rp;
    struct TextFont *font;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    text->width = 0;
    text->height = 0;

    text->align = DEFAULT_TEXT_ALIGN;
    text->style = ZTC_STYLE_NORMAL;
    text->dripen = 0;

    font = _font(obj);
    InitRastPort(&rp);
    rp.Font = font;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	line_node->lheight = font->tf_YSize;
	line_node->lwidth = 0;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    if (chunk_node->str)
	        chunk_node->cwidth = TextLength(&rp,chunk_node->str,strlen(chunk_node->str));
	    line_node->lwidth += chunk_node->cwidth;
	}

	text->height += line_node->lheight;
	text->width = MAX(text->width,line_node->lwidth);
    }
}

#if 0
static void draw_text_string (struct chunk_draw_datas *cdd, ZTextChunk *chunk)
{
    Object *obj = cdd->ldd->obj;
    struct RastPort *rp = _rp(obj);
    int xtext = cdd->ldd->left + cdd->xoffset - chunk->lbearing;
    int ytext = cdd->ldd->top + cdd->ldd->yoffset + _font(obj)->tf_Baseline;

    cdd->ldd->text->style = chunk->style;
    if (chunk->dripen != -1)
    {
	cdd->ldd->text->dripen = chunk->dripen;
	SetAPen(gc, _dri(obj)->dri_Pens[cdd->ldd->text->dripen]);
    }

#ifndef _AROS
    if (cdd->ldd->text->style & ZTC_STYLE_ITALIC)
    {
	xback = xtext;
	xtext = 0;
	yback = ytext;
	ytext = _font(obj)->ascent;

	if (chunk->im)
	{
	    draw_italic (drawable, gc, cdd, obj, chunk, xback);
	    return;
	}
	prepare_italic_pixmap(&drawable, &gc, obj, chunk);
    }

    gdk_draw_text(drawable, _font(obj), gc,
		  xtext,
		  ytext,
		  chunk->str->str, chunk->str->len);

    if (cdd->ldd->text->style & ZTC_STYLE_UNDERLINE)
    {
	int endu = xtext + chunk->lbear2
	    + MIN(chunk->cwidth, chunk->cwidth - chunk->deltabearing) - 1;

	if (chunk->longer_underline)
	    endu = xtext + chunk->lbear2 + chunk->cwidth;

	gdk_draw_line(drawable, gc, xtext + chunk->lbear2,
		      ytext + MIN(_font(obj)->descent - 1, 1),
		      endu, ytext + MIN(_font(obj)->descent - 1, 1));
    }
    if (cdd->ldd->text->style & ZTC_STYLE_BOLD)
    {
	gdk_draw_text(drawable, _font(obj), gc, xtext + 1, ytext,
		      chunk->str->str, chunk->str->len);
    }

    if (cdd->ldd->text->style & ZTC_STYLE_ITALIC)
    {
	draw_italic (drawable, gc, cdd, obj, chunk, xback);
    }
#else
#warning FIXME: set appropriate style using SetSoftStyle()
    SetDrMd(_rp(obj), JAM1);
    Move(_rp(obj), xtext, ytext);
    Text(_rp(obj), chunk->str->str, chunk->str->len);
#endif
}


/* 
 * first, you have to hide the real graphic environment, to draw
 * either on the real place, or in a new pixmap if doing italic.
 * Means hiding gc, drawable and origin behind new variables.
 * How is done italic ?
 * the normal text is rendered to a pixmap with
 * a special background color (which will be 'transparent').
 * The pixmap is converted to an imlib image. This image is sheared
 * (lines are scrolled right according to their y-coordinate)
 * Then image is pasted to the window.
 */
static void draw_text_chunk (gpointer ch, gpointer d)
{
    struct chunk_draw_datas *cdd = (struct chunk_draw_datas *)d;
    ZTextChunk *chunk = (ZTextChunk *)ch;

    if (chunk->str)
    {
	draw_text_string(cdd, chunk);
    }
#ifndef _AROS
    else if (chunk->image)
    {
    }
#endif

    cdd->xoffset += chunk->cwidth;
}


static void draw_text_line (gpointer l, gpointer d)
{
    struct line_draw_datas *ldd = (struct line_draw_datas *)d;
    ZTextLine *line = (ZTextLine *)l;
    struct chunk_draw_datas data;

    data.line = line;
    data.xoffset = 0;
    data.ldd = ldd;

    if ((ldd->right - ldd->left + 1) < line->lwidth)
	g_warning("draw_text_line: allocated space is smaller than needed space");

    if (line->align > 0)
	ldd->text->align = line->align;

    if (ldd->text->align == ZTL_CENTER)
        data.xoffset = ((ldd->right - ldd->left + 1) - line->lwidth) / 2;
    else if (ldd->text->align == ZTL_RIGHT)
        data.xoffset = (ldd->right - ldd->left + 1) - line->lwidth;

    g_list_foreach(line->chunks, draw_text_chunk, &data);    
    ldd->yoffset += line->lheight;
}

#endif

void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top)
{
    struct RastPort *rp;
    ULONG style = FS_NORMAL;

    ZTextLine *line_node;
    ZTextChunk *chunk_node;

    if (!text || !obj) return;

    rp = _rp(obj);
    SetFont(rp,_font(obj));

    top += _font(obj)->tf_Baseline + text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth;
	else x = left;

	x += text->xscroll;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    if (chunk_node->str)
	    {
	    	ULONG newstyle = FS_NORMAL;

		if (chunk_node->style & ZTC_STYLE_BOLD) newstyle |= FSF_BOLD;
		if (chunk_node->style & ZTC_STYLE_UNDERLINE) newstyle |= FSF_UNDERLINED;
		if (newstyle != style)
		{
		    SetSoftStyle(rp, newstyle, 0xff);
		    style = newstyle;
		}

	    	Move(rp,x,top);
	    	SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);

	    	Text(rp,chunk_node->str,strlen(chunk_node->str));
	    	x += chunk_node->cwidth;
	    }
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

    top += _font(obj)->tf_Baseline + text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth;
	else x = left;

	x += text->xscroll;

	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    char *str = chunk_node->str;
	    ULONG newstyle = FS_NORMAL;
	    if (!str) str = "";


	    if (chunk_node->style & ZTC_STYLE_BOLD) newstyle |= FSF_BOLD;
	    if (chunk_node->style & ZTC_STYLE_UNDERLINE) newstyle |= FSF_UNDERLINED;
	    if (newstyle != style)
	    {
		SetSoftStyle(rp, newstyle, 0xff);
		style = newstyle;
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
		    RectFill(_rp(obj), x + offx, top - _font(obj)->tf_Baseline, x + offx + cursor_width - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
		    cursorx = -1;
		} else cursorx -= strlen(chunk_node->str);
	    }
	    
	    Move(rp,x,top);
	    SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);

	    Text(rp,chunk_node->str,strlen(chunk_node->str));
	    x += chunk_node->cwidth;
	}

	if (!cursory && cursorx != -1)
	{
	    /* Cursor has not been drawn yet */
	    SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
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

    top += _font(obj)->tf_Baseline + text->yscroll;

    for (line_node = (ZTextLine *)text->lines.mlh_Head; line_node->node.mln_Succ ; line_node = (ZTextLine*)line_node->node.mln_Succ)
    {
	LONG x;

	if (line_node->align == ZTL_CENTER) x = (left + right - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth;
	else x = left;

	x += text->xscroll;

	if (!ypos)
	for (chunk_node = (ZTextChunk *)line_node->chunklist.mlh_Head; chunk_node->node.mln_Succ ; chunk_node = (ZTextChunk*)chunk_node->node.mln_Succ)
	{
	    char *str = chunk_node->str;
	    ULONG newstyle = FS_NORMAL;
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

	    if (chunk_node->style & ZTC_STYLE_BOLD) newstyle |= FSF_BOLD;
	    if (chunk_node->style & ZTC_STYLE_UNDERLINE) newstyle |= FSF_UNDERLINED;
	    if (newstyle != style)
	    {
		SetSoftStyle(rp, newstyle, 0xff);
		style = newstyle;
	    }

	    if (cursor)
	    {
		SetAPen(_rp(obj), _dri(obj)->dri_Pens[FILLPEN]);
		RectFill(_rp(obj), x + offx, top - _font(obj)->tf_Baseline, x + offx + cursor_width - 1, top - _font(obj)->tf_Baseline + _font(obj)->tf_YSize-1);
	    } else
	    {
		DoMethod(obj, MUIM_DrawBackground, x + offx, top - _font(obj)->tf_Baseline, cursor_width, _font(obj)->tf_YSize);
	    }
	    
	    Move(rp,x + offx, top);
	    SetABPenDrMd(rp, _dri(obj)->dri_Pens[chunk_node->dripen],0,JAM1);

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
    *chunk_ptr = NULL;
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
	*chunk_ptr = chunk;
	InitRastPort(&rp);
	SetFont(&rp,_font(obj));
	*len_ptr = x;
	*offset_ptr += TextLength(&rp,chunk->str,x);
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

	if (line_node->align == ZTL_CENTER) x = (left + right - line_node->lwidth) / 2;
	else if (line_node->align == ZTL_RIGHT) x = right - line_node->lwidth;
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

    return (text->xscroll != oldxscroll || text->yscroll != oldyscroll);
}

int zune_text_merge(ZText *text, Object *obj, int x, int y, ZText *tomerge)
{
    int i, offset, len;
    ZTextLine *line, *line_tomerge;
    ZTextChunk *chunk, *chunk_tomerge;
    ZTextChunk *chunk_new;
    struct MinList store;

    if (!zune_text_get_char_pos(text, obj, x, y, &line, &chunk, &offset, &len)) return 0;
    if (!(line_tomerge = (ZTextLine*)List_First(&tomerge->lines))) return 0;
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

