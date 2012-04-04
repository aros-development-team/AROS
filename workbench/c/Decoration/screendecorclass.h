/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef SCREENDECORCLASS_H
#define SCREENDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include "newimage.h"

struct ScreenTitleChild
{
    LONG ChildWidth;
    UWORD ChildBgPen;
    void (*ChildRender)(struct RastPort *rp, UWORD *pens, struct Rectangle *bounds);
    void (*ChildInput)(ULONG x, ULONG y);
};

struct ScreenData
{
    /* These are default decorator images translated to depth/attributes of screen
       on which the decorator is used. WindowData and MenuData points into these
       objects */
    struct DecorImages * di;
    
    struct NewImage *img_sdepth;
    struct NewImage *img_sbarlogo;
    struct NewImage *img_stitlebar;
    
    /* XXX HACK */
    /* This is needed because a call to Menu::DrawSysImage gets passed ScreenData
       as UserBuffer instead of MenuData */

    struct NewImage *img_amigakey;
    struct NewImage *img_menucheck;
    struct NewImage *img_submenu;
    /* XXX HACK */

    LONG            ActivePen;
    LONG            DeactivePen;
    BOOL            truecolor;
};

#define SDA_DecorImages     0x20003
#define SDA_DecorConfig     0x20004
#define SDA_TitleChild     0x20005

struct IClass * MakeScreenDecorClass();
#endif
