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
void zune_text_destroy (ZText *text);
void zune_text_get_bounds (ZText *text, Object *obj);
void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top);
void zune_text_draw_cursor (ZText *text, Object *obj, WORD left, WORD right, WORD top, LONG cursorx, LONG cursory);
void zune_text_get_real_pos (ZText *text, Object *obj, WORD *left, WORD *right);

int zune_text_get_char_pos(ZText *text, Object *obj, LONG x, LONG y, struct ZTextLine **line_ptr, struct ZTextChunk **chunk_ptr, int *offset_ptr, int *len_ptr);
int zune_text_get_line_len(ZText *text, Object *obj, LONG y);

#endif
