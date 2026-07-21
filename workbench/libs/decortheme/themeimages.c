/*
    Copyright (C) 2011-2026, The AROS Development Team.

    Desc: decortheme.library - theme image set loading/freeing

    Image containers are created and destroyed through the public
    decorator.library API so their ownership stays with the render
    engine.
*/

#include <proto/exec.h>
#include <proto/decorator.h>

#include <aros/libcall.h>

#include <libraries/decortheme.h>
#include "decortheme_intern.h"

#define DEBUG 0
#include <aros/debug.h>

struct DecorImages * NewImages(void)
{
    return AllocVec(sizeof(struct DecorImages), MEMF_ANY | MEMF_CLEAR);
}

struct DecorImages * LoadImages(struct DecorConfig * dc)
{
    ULONG   wgsubimagecols = 4; /* Default value of subimage cols in window gadget */
    STRPTR  path = dc->ThemePath;

    struct DecorImages * di = NewImages();

    if (!di)
        return NULL;

    if (dc->GadgetsThreeState) wgsubimagecols = 3;

    di->img_sdepth = DGetImageFromFile(path, "System/SDepth/Default", 2, 1);
    di->img_stitlebar = DGetImageFromFile(path, "System/STitlebar/Default", 1, 1);
    di->img_sbarlogo = DGetImageFromFile(path, "System/SBarLogo/Default", 1, 1);

    di->img_size = DGetImageFromFile(path, "System/Size/Default", wgsubimagecols, 1);
    di->img_close = DGetImageFromFile(path, "System/Close/Default", wgsubimagecols, 1);
    di->img_depth = DGetImageFromFile(path, "System/Depth/Default", wgsubimagecols, 1);
    di->img_zoom = DGetImageFromFile(path, "System/Zoom/Default", wgsubimagecols, 1);
    di->img_mui = DGetImageFromFile(path, "System/MUI/Default", wgsubimagecols, 1);
    di->img_popup = DGetImageFromFile(path, "System/PopUp/Default", wgsubimagecols, 1);
    di->img_snapshot = DGetImageFromFile(path, "System/Snapshot/Default", wgsubimagecols, 1);
    di->img_iconify = DGetImageFromFile(path, "System/Iconify/Default", wgsubimagecols, 1);
    di->img_lock = DGetImageFromFile(path, "System/Lock/Default", wgsubimagecols, 1);
    di->img_up = DGetImageFromFile(path, "System/ArrowUp/Default", wgsubimagecols, 1);
    di->img_down = DGetImageFromFile(path, "System/ArrowDown/Default", wgsubimagecols, 1);
    di->img_left = DGetImageFromFile(path, "System/ArrowLeft/Default", wgsubimagecols, 1);
    di->img_right = DGetImageFromFile(path, "System/ArrowRight/Default", wgsubimagecols, 1);
    di->img_winbar_normal = DGetImageFromFile(path, "System/Titlebar/Default", 1, 2);
    di->img_border_normal = DGetImageFromFile(path, "System/Borders/Default", 1, 1);
    di->img_border_deactivated = DGetImageFromFile(path, "System/Borders/Default_Deactivated", 1, 1);
    di->img_verticalcontainer = DGetImageFromFile(path, "System/Container/Vertical", 2, 1);
    di->img_verticalknob = DGetImageFromFile(path, "System/Knob/Vertical", 3, 1);
    di->img_horizontalcontainer = DGetImageFromFile(path, "System/Container/Horizontal", 1, 2);
    di->img_horizontalknob = DGetImageFromFile(path, "System/Knob/Horizontal", 1, 3);

    di->img_menu = DGetImageFromFile(path, "Menu/Background/Default", 1, 1);
    di->img_amigakey = DGetImageFromFile(path, "Menu/AmigaKey/Default", 1, 1);
    di->img_menucheck = DGetImageFromFile(path, "Menu/Checkmark/Default", 1, 1);
    di->img_submenu = DGetImageFromFile(path, "Menu/SubMenu/Default", 1, 1);

    return di;
}

void FreeImages(struct DecorImages * di)
{
    DDisposeImageContainer(di->img_sdepth);
    DDisposeImageContainer(di->img_sbarlogo);
    DDisposeImageContainer(di->img_stitlebar);

    DDisposeImageContainer(di->img_size);
    DDisposeImageContainer(di->img_close);
    DDisposeImageContainer(di->img_depth);
    DDisposeImageContainer(di->img_zoom);
    DDisposeImageContainer(di->img_up);
    DDisposeImageContainer(di->img_down);
    DDisposeImageContainer(di->img_left);
    DDisposeImageContainer(di->img_right);
    DDisposeImageContainer(di->img_mui);
    DDisposeImageContainer(di->img_popup);
    DDisposeImageContainer(di->img_snapshot);
    DDisposeImageContainer(di->img_iconify);
    DDisposeImageContainer(di->img_lock);
    DDisposeImageContainer(di->img_winbar_normal);
    DDisposeImageContainer(di->img_border_normal);
    DDisposeImageContainer(di->img_border_deactivated);
    DDisposeImageContainer(di->img_verticalcontainer);
    DDisposeImageContainer(di->img_verticalknob);
    DDisposeImageContainer(di->img_horizontalcontainer);
    DDisposeImageContainer(di->img_horizontalknob);

    DDisposeImageContainer(di->img_menu);
    DDisposeImageContainer(di->img_amigakey);
    DDisposeImageContainer(di->img_menucheck);
    DDisposeImageContainer(di->img_submenu);

    FreeVec(di);
}

/* ========== Library Entry Points ========== */

AROS_LH0(struct DecorImages *, DTNewImages,
    struct Library *, DecorThemeBase, 11, Decortheme)
{
    AROS_LIBFUNC_INIT
    return NewImages();
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, DTFreeImages,
    AROS_LHA(struct DecorImages *, di, A0),
    struct Library *, DecorThemeBase, 12, Decortheme)
{
    AROS_LIBFUNC_INIT
    FreeImages(di);
    AROS_LIBFUNC_EXIT
}
