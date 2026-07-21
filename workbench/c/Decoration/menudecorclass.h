/*
    Copyright  2011-2026, The AROS Development Team.
*/

#ifndef MENUDECORCLASS_H
#define MENUDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include <libraries/decortheme.h>

struct MenuData
{
    struct DecorImage *ni;
    struct BitMap   *map;

    struct DecorImage *img_menu;
    struct TileInfo *img_menu_ti;
    struct DecorImage *img_amigakey;
    struct DecorImage *img_menucheck;
    struct DecorImage *img_submenu;
    LONG   ActivePen;
    LONG   DeactivePen;
    BOOL   truecolor;

};

#define MDA_DecorImages     0x10003
#define MDA_DecorConfig     0x10004
#define MDA_DecorTheme      0x10005

struct IClass * MakeMenuDecorClass();
#endif
