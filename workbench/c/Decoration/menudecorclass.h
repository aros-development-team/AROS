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

struct menudecor_data
{
    struct scrdecor_data *sd;

    struct DrawInfo *dri;
    struct Screen *scr;
    struct NewImage *img_menu;
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
};

struct MenuData
{
    struct  NewImage    *ni;
    struct  BitMap      *map;

    struct NewImage *img_menu;
    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
    LONG   ActivePen;
    LONG   DeactivePen;
    BOOL   truecolor;

};

#define MDA_Configuration   0x10002
#define MDA_ScreenData      0x10003

IPTR MenuDecor_Dispatcher(struct IClass *cl, Object *obj, Msg msg);
#endif
