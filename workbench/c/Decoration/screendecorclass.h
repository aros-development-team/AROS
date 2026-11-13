/*
    Copyright  2011-2026, The AROS Development Team.
*/

#ifndef SCREENDECORCLASS_H
#define SCREENDECORCLASS_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>

#include <libraries/decortheme.h>

struct ScreenData
{
    /* Theme instantiated for this screen; owns the screen matched
       images and the element descriptors over them */
    struct DecorThemeScreen *dts;

    /* These are default decorator images translated to depth/attributes of screen
       on which the decorator is used. WindowData and MenuData points into these
       objects (owned by dts) */
    struct DecorImages * di;
    
    struct DecorImage *img_sdepth;
    struct DecorImage *img_sbarlogo;
    struct DecorImage *img_stitlebar;
    
    /* XXX HACK */
    /* This is needed because a call to Menu::DrawSysImage gets passed ScreenData
       as UserBuffer instead of MenuData */

    struct DecorImage *img_amigakey;
    struct DecorImage *img_menucheck;
    struct DecorImage *img_submenu;
    /* XXX HACK */

    LONG            ActivePen;
    LONG            DeactivePen;
    BOOL            truecolor;
};

#define SDA_DecorImages     0x20003
#define SDA_DecorConfig     0x20004
#define SDA_TitleChild      0x20005
#define SDA_DecorTheme      0x20006

struct IClass * MakeScreenDecorClass();
struct IClass * GetScreenChildGClass();
#endif
