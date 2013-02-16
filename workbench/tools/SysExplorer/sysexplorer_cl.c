/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <libraries/asl.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <zune/customclasses.h>

#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "locale.h"
#include "sysexplorer_cl.h"

/*** Instance Data **********************************************************/
struct SysExplorer_DATA
{
    Object      *tree;
    Object      *instance_name_txt;
    Object      *hardware_description_txt;
    Object      *vendor_info_txt;
    struct Hook enum_hook;
    struct Hook selected_hook;
};

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;

const struct OOP_ABDescr abd[] =
{
    {IID_Hidd, &HiddAttrBase},
    {IID_HW  , &HWAttrBase  },
    {NULL    , NULL         }
};


AROS_UFH3S(void, enumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    struct SysExplorer_DATA *data = h->h_Data;

    CONST_STRPTR name = NULL;
    
    OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);

    if (name)
    {
        // node
        struct MUI_NListtree_TreeNode *tn;

        tn = (APTR)DoMethod(data->tree, MUIM_NListtree_Insert, name, obj,
                            parent, MUIV_NListtree_Insert_PrevNode_Tail,
                            TNF_LIST|TNF_OPEN);
        if (tn)
        {
            HW_EnumDrivers(obj, &data->enum_hook, tn);
        }
    }
    else
    {
        // leave
        OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);
        DoMethod(data->tree, MUIM_NListtree_Insert, name, obj,
                 parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, selectedFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct MUI_NListtree_TreeNode **, tn, A1))
{
    AROS_USERFUNC_INIT

    D(bug("selectedFunc called tn: %p name %s\n", *tn, (*tn)->tn_Name));

    struct SysExplorer_DATA *data = h->h_Data;

    if ((*tn)->tn_Flags & TNF_LIST)
    {
        // node
        SET(data->instance_name_txt, MUIA_Text_Contents, "");
        SET(data->hardware_description_txt, MUIA_Text_Contents, "");
        SET(data->vendor_info_txt, MUIA_Text_Contents, "");
    }
    else
    {
        // leave
        SET(data->instance_name_txt, MUIA_Text_Contents, (*tn)->tn_Name);
        SET(data->hardware_description_txt, MUIA_Text_Contents, "FOO");
        SET(data->vendor_info_txt, MUIA_Text_Contents, "BAR");
    }

    AROS_USERFUNC_EXIT
}

static Object *SysExplorer__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    Object *tree;
    Object *instance_name_txt;
    Object *hardware_description_txt;
    Object *vendor_info_txt;

    if (!OOP_ObtainAttrBases(abd))
    {
        // FIXME
        return NULL;
    }

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,

        MUIA_Group_Horiz, TRUE,
        Child, (IPTR)(HGroup,
            GroupFrame,
            MUIA_FrameTitle, __(MSG_HIDD_TREE),
            Child, (IPTR)(NListviewObject,
                MUIA_NListview_NList, (IPTR)(tree = NListtreeObject,
                    ReadListFrame,
                    MUIA_CycleChain, TRUE,
                End),
            End),
        End),
        Child, (IPTR)(VGroup,
            Child, (IPTR)(ColGroup(2),
                MUIA_FrameTitle, __(MSG_PROPERTIES),
                GroupFrame,
                Child, (IPTR)Label(_(MSG_LABEL_DRIVER)),
                Child, (IPTR)(instance_name_txt = TextObject,
                    MUIA_Text_Contents, (IPTR)"          ",
                    TextFrame,
                End),
                Child, (IPTR)Label(_(MSG_LABEL_HW_DESCRIPTION)),
                Child, (IPTR)(hardware_description_txt = TextObject,
                    MUIA_Text_Contents, (IPTR)"          ",
                    TextFrame,
                End),
                Child, (IPTR)Label(_(MSG_LABEL_VENDOR)),
                Child, (IPTR)(vendor_info_txt = TextObject,
                    MUIA_Text_Contents, (IPTR)"          ",
                    TextFrame,
                End),
            End),
            Child, (IPTR)HVSpace,
        End),
        TAG_MORE, (IPTR)msg->ops_AttrList
    );

    if (self)
    {
        struct SysExplorer_DATA *data = INST_DATA(cl, self);

        data->tree = tree;
        data->instance_name_txt = instance_name_txt;
        data->hardware_description_txt = hardware_description_txt;
        data->vendor_info_txt = vendor_info_txt;

        data->enum_hook.h_Entry = enumFunc;
        data->enum_hook.h_Data = data;

        data->selected_hook.h_Entry = selectedFunc;
        data->selected_hook.h_Data = data;

        OOP_Object *hwRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        if (hwRoot)
        {
            /* This will kick our recursive enumeration into action */
            CALLHOOKPKT(&data->enum_hook, hwRoot, MUIV_NListtree_Insert_ListNode_Root);
        }

        DoMethod(data->tree, MUIM_Notify, MUIA_NListtree_Active, MUIV_EveryTime,
                 self, 3,
                 MUIM_CallHook, &data->selected_hook, MUIV_TriggerValue);

    }

    return self;
}

static IPTR SysExplorer__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    // struct SysExplorer_DATA *data = INST_DATA(cl, obj);

    OOP_ReleaseAttrBases(abd);

    return DoSuperMethodA(cl, obj, msg);
}


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_2
(
    SysExplorer, NULL, MUIC_Group, NULL,
    OM_NEW,             struct opSet *,
    OM_DISPOSE,         Msg
);
