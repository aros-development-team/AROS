/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef MENUDECORCLASS_H
#define MENUDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include "newimage.h"

struct MenuData
{
    struct NewImage *ni;
    struct BitMap   *map;

    struct NewImage *img_menu;
    struct TileInfo *img_menu_ti;
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
    LONG   ActivePen;
    LONG   DeactivePen;
    BOOL   truecolor;

};

#define MDA_DecorImages     0x10003
#define MDA_DecorConfig     0x10004

struct IClass * MakeMenuDecorClass();
#endif
