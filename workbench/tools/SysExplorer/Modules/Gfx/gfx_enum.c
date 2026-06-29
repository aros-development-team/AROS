/*
    Copyright (C) 2015-2026, The AROS Development Team. All rights reserved.

    Desc: SysExplorer graphics subsystem enumerator.
*/

#include <aros/debug.h>

#include <proto/sysexp.h>

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

#include "gfx_intern.h"

extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HiddGfxAttrBase;

static struct SysexpBase *gfxSysexpBase = NULL;

static void addGfxDisplay(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent)
{
    struct SysexpBase *SysexpBase = gfxSysexpBase;
    struct InsertObjectMsg msg =
    {
        .obj      = obj,
        .winClass = MonitorWindow_CLASS
    };
    CONST_STRPTR name = NULL;

    OOP_GetAttr(obj, aHidd_Name, (IPTR *)&name);

    SysexpBase->GlobalCount++;
    DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
             parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);
}

void gfxEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    OOP_Object *display = NULL;
    struct List *displays = NULL;

    D(bug("[gfx.sysexp] %s: found '%s'\n", __func__, OOP_OCLASS(obj)->ClassNode.ln_Name));

    /* The base software rasterizer doesn't have an actual "display" */
    if (OOP_OCLASS(obj) == OOP_FindClass(CLID_Hidd_Gfx))
        return;

    OOP_GetAttr(obj, aHidd_Gfx_DisplayList, (IPTR *)&displays);
    if (displays)
    {
        ForeachNode(displays, display)
        {
            addGfxDisplay(display, tn);
        }
    }
    else
    {
        OOP_GetAttr(obj, aHidd_Gfx_DisplayDefault, (IPTR *)&display);
        if (display)
            addGfxDisplay(display, tn);
    }
}

BOOL gfxValid(OOP_Object *obj, ULONG *flags)
{
    /* The base software rasterizer has no displays, so it is a leaf node */
    if (OOP_OCLASS(obj) == OOP_FindClass(CLID_Hidd_Gfx))
        *flags &= ~TNF_LIST;

    return TRUE;
}

void GfxStartup(struct SysexpBase *SysexpBase)
{
    D(bug("[gfx.sysexp] %s(%p)\n", __func__, SysexpBase));

    gfxSysexpBase = SysexpBase;

    RegisterClassHandler(CLID_Hidd_Gfx, 60, GfxWindow_CLASS, gfxEnum, gfxValid);
}
