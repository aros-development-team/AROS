/* 
    Copyright � 1999, David Le Corfec.
    Copyright � 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_TEXTENGINE_H__
#define _MUI_TEXTENGINE_H__

/* This is the clone of the MUI text engine, used by the Text class
 * and all class needing to draw text strings.
 * 
 */

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

typedef struct ZTextLine {
    struct MinNode node; /* embedded node */
    UBYTE  align;
    struct MinList chunklist;
    WORD   lwidth;
    WORD   lheight;
} ZTextLine;

struct ZText {
	  LONG xscroll; /* number of pixel which are not visible */
    LONG yscroll;

    WORD width; /* store calculated bounds - read only ! */
    WORD height;

/* private */
    struct MinList lines;
    UBYTE align; /* used in parsing */
    ULONG style;
    LONG  dripen;
};

typedef struct ZText ZText;

/* zune_text_new argtypes */
enum {
    ZTEXT_ARG_NONE,
    ZTEXT_ARG_HICHAR,    /* following arg is the HiChar */
    ZTEXT_ARG_HICHARIDX, /* following arg is the HiCharIdx */
};

ZText *zune_text_new (STRPTR preparse, STRPTR content, int argtype, TEXT arg);
char *zune_text_iso_string(ZText *text);
void zune_text_destroy (ZText *text);
void zune_text_get_bounds (ZText *text, Object *obj);
void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top);
void zune_text_draw_cursor (ZText *text, Object *obj, WORD left, WORD right, WORD top, LONG cursorx, LONG cursory);
void zune_text_draw_single (ZText *text, Object *obj, WORD left, WORD right, WORD top, LONG xpos, LONG ypos, BOOL cursor);

int zune_get_xpos_of_line(ZText *text, Object *obj, LONG y, LONG xpixel);
//int zune_text_horiz_pixel_to_coord(ZText *text, Object *obj, struct ZTextLine *line
int zune_text_get_char_pos(ZText *text, Object *obj, LONG x, LONG y, struct ZTextLine **line_ptr, struct ZTextChunk **chunk_ptr, int *offset_ptr, int *len_ptr);
int zune_text_get_line_len(ZText *text, Object *obj, LONG y);
int zune_text_get_lines(ZText *text);
int zune_make_cursor_visible(ZText *text, Object *obj, LONG cursorx, LONG cursory, LONG left, LONG top, LONG right, LONG bottom);

int zune_text_merge(ZText *text, Object *obj, int x, int y, ZText *tomerge);

#endif
