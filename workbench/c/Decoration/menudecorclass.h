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
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
    LONG   ActivePen;
    LONG   DeactivePen;
    BOOL   truecolor;

};

#define MDA_Configuration   0x10002
#define MDA_DecorImages     0x10003

struct IClass * MakeMenuDecorClass();
#endif
