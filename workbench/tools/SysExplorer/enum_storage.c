/*
    Copyright (C) 2015-2016, The AROS Development Team.
    $Id$
*/

#define DEBUG 0
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

#include <hidd/hidd.h>
#include <hidd/storage.h>
#include <hidd/ata.h>

#include "locale.h"
#include "classes.h"
#include "enums.h"

struct List storageList;

OOP_AttrBase HiddATABusAB;
OOP_AttrBase HiddATAUnitAB;

const struct OOP_ABDescr storage_abd[] =
{
    {IID_Hidd_ATABus ,  &HiddATABusAB },
    {IID_Hidd_ATAUnit,  &HiddATAUnitAB},
    {NULL            ,  NULL          }
};

static void addATAUnit(OOP_Object *dev, ULONG attrID, struct MUI_NListtree_TreeNode *parent)
{
    OOP_Object *unit = NULL;

    D(bug("[SysExplorer:Storage] ataunit: \n"));

    OOP_GetAttr(dev, attrID, (IPTR *)&unit);
    if (unit)
    {
        struct InsertObjectMsg msg =
        {
            .obj      = unit,
            .winClass = ATAUnitWindow_CLASS
        };
        CONST_STRPTR name;

        OOP_GetAttr(unit, aHidd_ATAUnit_Model, (IPTR *)&name);
        DoMethod(hidd_tree, MUIM_NListtree_Insert, name, &msg,
                 parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);
    }
}

static void ataBusEnum(OOP_Object *dev, struct MUI_NListtree_TreeNode *parent)
{
    D(bug("[SysExplorer:Storage] atabus: \n"));

    addATAUnit(dev, aHidd_ATABus_Master, parent);
    addATAUnit(dev, aHidd_ATABus_Slave , parent);
}

AROS_UFH3S(BOOL, storageenumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    BOOL objValid = TRUE;
    CONST_STRPTR name = NULL;
    struct MUI_NListtree_TreeNode *tn;
    ULONG flags = 0;
    struct InsertObjectMsg msg =
    {
        .obj = obj,
        .winClass = NULL
    };
    struct ClassHandlerNode *clHandlers = FindObjectHandler(obj, h->h_Data);

    D(bug("[SysExplorer:Storage] enum: list @ 0x%p\n", h->h_Data));
    D(bug("[SysExplorer:Storage] enum: handler @ 0x%p\n", clHandlers));

    if (clHandlers)
    {
        if (clHandlers->muiClass)
            msg.winClass = *(clHandlers->muiClass);
        if (clHandlers->enumFunc)
            flags = TNF_LIST|TNF_OPEN;
        if (clHandlers->validFunc)
            objValid = clHandlers->validFunc(obj, &flags);
    }

    if (objValid)
    {
        /* This is either HW or HIDD subclass */
        OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);
        if (!name)
            OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);

        tn = (APTR)DoMethod(hidd_tree, MUIM_NListtree_Insert, name, &msg,
                            parent, MUIV_NListtree_Insert_PrevNode_Sorted, flags);
        D(bug("Inserted TreeNode 0x%p <%s> UserData 0x%p\n", tn, tn->tn_Name, tn->tn_User));

        /* If we have enumerator for this class, call it now */
        if (clHandlers && clHandlers->enumFunc && (flags & TNF_LIST))
            clHandlers->enumFunc(obj, tn);
    }
    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

static struct Hook storageenum_hook =
{
    .h_Entry = storageenumFunc,
    .h_Data = &storageList
};

static void storageEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    D(bug("[SysExplorer:Storage] enumerating..\n"));
    D(bug("[SysExplorer:Storage] storage class list @ 0x%p\n", storageenum_hook.h_Data));
    HW_EnumDrivers(obj, &storageenum_hook, tn);
}

BOOL storageenum_init(void)
{
    struct ClassHandlerNode *storageclassNode;

    D(bug("[SysExplorer:Storage] Initialising..\n"));

    NEWLIST(&storageList);
    D(bug("[SysExplorer:Storage] class list @ 0x%p\n", &storageList));
    OOP_ObtainAttrBases(storage_abd);

    storageclassNode = AllocMem(sizeof(struct ClassHandlerNode), MEMF_CLEAR);
    storageclassNode->ch_Node.ln_Name = CLID_HW_ATA;
    storageclassNode->muiClass = &GenericWindow_CLASS;
    storageclassNode->enumFunc = storageEnum;
    Enqueue(&storageList, &storageclassNode->ch_Node);

    storageclassNode = AllocMem(sizeof(struct ClassHandlerNode), MEMF_CLEAR);
    storageclassNode->ch_Node.ln_Name = CLID_Hidd_ATABus;
    storageclassNode->muiClass = &ATAWindow_CLASS;
    storageclassNode->enumFunc = ataBusEnum;
    Enqueue(&storageList, &storageclassNode->ch_Node);

    RegisterClassHandler(CLID_Hidd_Storage, 90, NULL, storageEnum, NULL);
    return 2;
}

ADD2INIT(storageenum_init, 10);
