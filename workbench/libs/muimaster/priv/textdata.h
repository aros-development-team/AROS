/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __TEXTDATA_H__
#define __TEXTDATA_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#include <textengine.h>

struct MUI_TextData {
    ULONG  mtd_Flags;
    STRPTR contents;
    STRPTR preparse;
    TEXT   hichar;
    ZText *ztext;
};
 
#define MTDF_SETMIN    (1<<0)
#define MTDF_SETMAX    (1<<1)
#define MTDF_SETVMAX   (1<<2)
#define MTDF_HICHAR    (1<<3)
#define MTDF_HICHARIDX (1<<4)

#endif
