/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_TEXTENGINE_H__
#define _MUI_TEXTENGINE_H__

/* This is the clone of the MUI text engine, used by the Text class
 * and all class needing to draw text strings.
 * 
 */

struct MUI_ImageSpec_intern;

typedef struct ZTextChunk {
    struct MinNode node; /* embedded node */
    char                 *str;
    CONST_STRPTR          spec;
    struct MUI_ImageSpec_intern *image;
    Object               *obj; /* Area subclass, see List_CreateImage */
    LONG                  dripen;
    LONG                  pen;
    UBYTE                 style; /* FSF_UNDERLINED, FSF_BOLD, ... */
    WORD                  cwidth;
    WORD                  cheight;
} ZTextChunk;

typedef struct ZTextLine {
    struct MinNode node;       /* embedded node */
    UBYTE  align;              /* ZTL_CENTER, ZTL_RIGHT, ZTL_LEFT */
    struct MinList chunklist;
    WORD   lwidth;
    WORD   lheight;
} ZTextLine;

struct ZText {
    WORD width; /* store calculated bounds - read only ! */
    WORD height;

/* private */
    struct MinList lines;
};

typedef struct ZText ZText;

/* zune_text_new argtypes */
enum {
    ZTEXT_ARG_NONE,
    ZTEXT_ARG_HICHAR,    /* following arg is the HiChar */
    ZTEXT_ARG_HICHARIDX, /* following arg is the HiCharIdx */
};

ZText *zune_text_new (CONST_STRPTR preparse, CONST_STRPTR content, int argtype, TEXT arg);
void zune_text_destroy (ZText *text);
void zune_text_get_bounds (ZText *text, Object *obj);
void zune_text_draw (ZText *text, Object *obj, WORD left, WORD right, WORD top);

#endif
