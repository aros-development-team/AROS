#ifndef _MUI_CLASSES_TEXT_H
#define _MUI_CLASSES_TEXT_H

#ifndef _MUI_TEXTENGINE_H
#include "textengine.h"
#endif

struct MUI_TextData {
    ULONG  mtd_Flags;
    STRPTR contents;
    STRPTR preparse;
    TEXT   hichar;
    ZText *ztext;
    LONG xpos;
    LONG ypos;
    struct MUI_EventHandlerNode ehn;

    LONG update; /* type of update 1 - everything, 2 - insert char, no scroll */
    LONG update_arg1;
    LONG update_arg2;
};

#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)
#define MTDF_EDITABLE  (1<<5)
#define MTDF_MULTILINE (1<<6)
#define MTDF_ADVANCEONCR (1<<7)

#ifdef _DCC
extern char MUIC_Text[];
#else
#define MUIC_Text "Text.mui"
#endif

/* Attributes */

enum {
    MUIA_Text_Contents = 0x8042f8dc, /* V4  isg STRPTR            */
    MUIA_Text_HiChar =   0x804218ff, /* V4  i.. char              */
    MUIA_Text_PreParse = 0x8042566d, /* V4  isg STRPTR            */
    MUIA_Text_SetMax =   0x80424d0a, /* V4  i.. BOOL              */
    MUIA_Text_SetMin =   0x80424e10, /* V4  i.. BOOL              */
    MUIA_Text_SetVMax =  0x80420d8b, /* V11 i.. BOOL              */
};

/* Attributes */

#define MUIA_Text_HiCharIdx   0x804214f5
#define MUIA_Text_Editable    0x80420d8c  /* ZV1 i.. BOOL              */


extern const struct __MUIBuiltinClass _MUI_Text_desc;

#endif
