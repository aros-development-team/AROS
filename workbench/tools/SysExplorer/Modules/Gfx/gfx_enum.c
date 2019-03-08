/*
    Copyright (C) 2015-2018, The AROS Development Team.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/gfx.h>

#include "locale.h"
#include "gfx_classes.h"
#include "enums.h"

OOP_AttrBase HiddGfxAttrBase;

const struct OOP_ABDescr gfx_abd[] =
{
    {IID_Hidd_Gfx,      &HiddGfxAttrBase},
    {NULL            ,  NULL          }
};

static void addGgfxDisplay(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent)
{
    struct InsertObjectMsg msg =
    {
        .obj      = obj,
        .winClass = MonitorWindow_CLASS
    };
    CONST_STRPTR name;

    OOP_GetAttr(obj, aHidd_Name, (IPTR *)&name);

    sysExplGlobalCount++;
    DoMethod(hidd_tree, MUIM_NListtree_Insert, name, &msg,
             parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);
}

void gfxEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    OOP_Object *display = NULL;
    struct List *displays = NULL;

    D(bug("[SysExplorer:Gfx] %s: found '%s'\n", __PRETTY_FUNCTION__, OOP_OCLASS(obj)->ClassNode.ln_Name));

    /* software rasterizer doesnt have an actual "sidplay" */
    if (OOP_OCLASS(obj) == OOP_FindClass(CLID_Hidd_Gfx))
        return;

    OOP_GetAttr(obj, aHidd_Gfx_DisplayList, (IPTR *)&displays);
    if (displays)
    {
        ForeachNode(displays, display)
        {
            addGgfxDisplay(display, tn);
        }
    }
    else
    {
        OOP_GetAttr(obj, aHidd_Gfx_DisplayDefault, (IPTR *)&display);
        if (display)
            addGgfxDisplay(display, tn);
    }
}

BOOL gfxValid(OOP_Object *obj, ULONG *flags)
{
    if (OOP_OCLASS(obj) == OOP_FindClass(CLID_Hidd_Gfx))
    {
        ULONG _flags = *flags;
        _flags &= ~ TNF_LIST;
        *flags = _flags;
    }
    return TRUE;
}

BOOL gfxenum_init(void)
{
    D(bug("[SysExplorer:Gfx] Initialising..\n"));

    OOP_ObtainAttrBases(gfx_abd);
    RegisterClassHandler(CLID_Hidd_Gfx, 60, &GfxWindow_CLASS, gfxEnum, gfxValid);

    return TRUE;
}

ADD2INIT(gfxenum_init, 10);
