/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/ata.h>
#include <hidd/hidd.h>
#include <libraries/asl.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

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
#include "classes.h"

#include <aros/debug.h>

#define APPNAME "SysExplorer"
#define VERSION "SysExplorer 0.2"

struct ObjectUserData
{
    OOP_Object *obj;
    struct MUI_CustomClass *winClass;
    Object *win;
};

struct InsertObjectMsg
{
    OOP_Object *obj;
    struct MUI_CustomClass *winClass;
};

struct ClassHandlers
{
    CONST_STRPTR classID;
    struct MUI_CustomClass **muiClass;
    void (*enumFunc)(OOP_Object *obj, struct MUI_NListtree_TreeNode *parent);
};

static const struct ClassHandlers *FindClassHandler(OOP_Object *obj);

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

static Object *app, *main_window, *property_window, *hidd_tree;
static Object *property_menu, *expand_menu, *collapse_menu, *quit_menu;

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;
OOP_AttrBase HiddATABusAB;
OOP_AttrBase HiddATAUnitAB;

const struct OOP_ABDescr abd[] =
{
    {IID_Hidd        , &HiddAttrBase },
    {IID_HW          , &HWAttrBase   },
    {IID_Hidd_ATABus , &HiddATABusAB },
    {IID_Hidd_ATAUnit, &HiddATAUnitAB},
    {NULL            , NULL          }
};

/* Here we have enumerators for different device and subsystem classes */

static void addATAUnit(OOP_Object *dev, ULONG attrID, struct MUI_NListtree_TreeNode *parent)
{
    OOP_Object *unit = NULL;

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

static void ataUnitsEnum(OOP_Object *dev, struct MUI_NListtree_TreeNode *parent)
{
    addATAUnit(dev, aHidd_ATABus_Master, parent);
    addATAUnit(dev, aHidd_ATABus_Slave , parent);
}

AROS_UFH3S(BOOL, enumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    CONST_STRPTR name = NULL;
    struct MUI_NListtree_TreeNode *tn;
    ULONG flags = 0;
    struct InsertObjectMsg msg =
    {
        .obj = obj,
        .winClass = NULL
    };
    const struct ClassHandlers *clHandlers = FindClassHandler(obj);

    /* This is either HW or HIDD subclass */
    OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);
    if (!name)
        OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);

    if (clHandlers)
    {
        if (clHandlers->muiClass)
            msg.winClass = *(clHandlers->muiClass);
        if (clHandlers->enumFunc)
            flags = TNF_LIST|TNF_OPEN;
    }

    tn = (APTR)DoMethod(hidd_tree, MUIM_NListtree_Insert, name, &msg,
                        parent, MUIV_NListtree_Insert_PrevNode_Tail, flags);
    D(bug("Inserted TreeNode 0x%p <%s> UserData 0x%p\n", tn, tn->tn_Name, tn->tn_User));

    /* If we have enumerator for this class, call it now */
    if (clHandlers && clHandlers->enumFunc)
        clHandlers->enumFunc(obj, tn);

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

static const struct Hook enum_hook =
{
    .h_Entry = enumFunc
};

static void hwEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    HW_EnumDrivers(obj, (struct Hook *)&enum_hook, tn);
}

/*
 * This table lists handlers for known public classes.
 * It specifies information window class, as well as function
 * to enumerate children objects for the class.
 *
 * For proper operation CLIDs in this table should
 * be sorted from subclasses to superclasses.
 */
static const struct ClassHandlers classHandlers[] =
{
    {CLID_HW_Root    , &ComputerWindow_CLASS, hwEnum      },
    {CLID_HW         , NULL                 , hwEnum      },
    {CLID_Hidd_ATABus, &ATAWindow_CLASS     , ataUnitsEnum},
    {CLID_Hidd       , &GenericWindow_CLASS , NULL        },
    {NULL            , NULL                 , NULL        }
};

static const struct ClassHandlers *FindClassHandler(OOP_Object *obj)
{
    unsigned int i;
 
    for (i = 0; classHandlers[i].classID; i++)
    {
        OOP_Class *cl;

        for (cl = OOP_OCLASS(obj); cl ; cl = cl->superclass)
        {
            if (!strcmp(cl->ClassNode.ln_Name, classHandlers[i].classID))
                return &classHandlers[i];
        }
    }
    return NULL;
}

AROS_UFH3S(void, closeFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct ObjectUserData **, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("closeFunc address 0x%p\n", closeFunc));
    D(bug("Close window 0x%p, ObjectUserData 0x%p\n", obj, *msg));

    SET(obj, MUIA_Window_Open, FALSE);
    DoMethod(app, OM_REMMEMBER, obj);
    DoMethod(app, MUIM_Application_PushMethod, obj, 1, OM_DISPOSE);

    (*msg)->win = NULL;

    AROS_USERFUNC_EXIT
};

static const struct Hook close_hook =
{
    .h_Entry = closeFunc
};

AROS_UFH3S(APTR, constructFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct MUIP_NListtree_ConstructMessage *, msg, A1))
{
    AROS_USERFUNC_INIT

    struct InsertObjectMsg *insertMsg = msg->UserData;
    struct ObjectUserData *data = AllocPooled(msg->MemPool, sizeof(struct ObjectUserData));

    D(bug("%s: insertMsg 0x%p ObjectUserData 0x%p\n", msg->Name, insertMsg, data));
    if (data)
    {
        data->obj      = insertMsg->obj;
        data->winClass = insertMsg->winClass;
        data->win      = NULL;
    }
    return data;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, destructFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct MUIP_NListtree_DestructMessage *, msg, A1))
{
    AROS_USERFUNC_INIT

    FreePooled(msg->MemPool, msg->UserData, sizeof(struct ObjectUserData));

    AROS_USERFUNC_EXIT
}

static const struct Hook constructHook =
{
    .h_Entry = constructFunc
};

static const struct Hook destructHook =
{
    .h_Entry = destructFunc
};

AROS_UFH3S(void, propertyFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct MUI_NListtree_TreeNode **, tn, A1))
{
    AROS_USERFUNC_INIT

    struct MUI_NListtree_TreeNode *node = *tn;
    struct ObjectUserData *data;

    if (node == NULL)
    {
        /* if we were called from menu we must 1st find the current entry */
        node = (struct MUI_NListtree_TreeNode *)XGET(hidd_tree, MUIA_NListtree_Active);
    }

    if (node == NULL)
    {
        /* Do nothing if still no current entry */
        return;
    }

    data = node->tn_User;
    D(bug("propertyFunc called: TreeNode 0x%p <%s> UserData 0x%p\n", node, node->tn_Name, data));
    D(bug("Window 0x%p\n", data->win));

    if (data->win)
    {
        /* The window is already open, show it to the user */
        DoMethod(data->win, MUIM_Window_ToFront);
        SET(data->win, MUIA_Window_Activate, TRUE);
        return;
    }

    if (data->winClass)
    {
        /* We have information window class. Open the window. */
        data->win = NewObject(data->winClass->mcc_Class, NULL,
                              MUIA_PropertyWin_Object, (IPTR)data->obj,
                              TAG_DONE);
        D(bug("Created window 0x%p\n", data->win));

        if (data->win)
        {
            DoMethod(app, OM_ADDMEMBER, data->win);
            DoMethod(data->win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                     data->win, 3,
                     MUIM_CallHook, &close_hook, data);
            SET(data->win, MUIA_Window_Open, TRUE);
        }
    }

    AROS_USERFUNC_EXIT
}

static const struct Hook property_hook =
{
    .h_Entry = propertyFunc
};

static BOOL GUIinit()
{
    BOOL retval = FALSE;

    app = ApplicationObject,
        MUIA_Application_Title,         (IPTR)APPNAME,
        MUIA_Application_Version,       (IPTR)VERSION,
        MUIA_Application_Copyright,     (IPTR)"(C) 2013, The AROS Development Team",
        MUIA_Application_Author,        (IPTR)"Pavel Fedin",
        MUIA_Application_Base,          (IPTR)APPNAME,
        MUIA_Application_Description,   __(MSG_DESCRIPTION),

        MUIA_Application_Menustrip, (IPTR)(MenuitemObject,
            MUIA_Family_Child, (IPTR)(MenuitemObject,
                MUIA_Menuitem_Title, __(MSG_MENU_PROJECT),
                MUIA_Family_Child, (IPTR)(property_menu = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Properties",
                End),
                MUIA_Family_Child, (IPTR)(expand_menu = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Expand All",
                End),
                MUIA_Family_Child, (IPTR)(collapse_menu = MenuitemObject,
                    MUIA_Menuitem_Title, (IPTR)"Collapse All",
                End),
                MUIA_Family_Child, (IPTR)(quit_menu = MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_QUIT),
                End),
            End),
        End),

        SubWindow, (IPTR)(main_window = WindowObject,
            MUIA_Window_Title,	__(MSG_WINTITLE),
            MUIA_Window_ID, MAKE_ID('S', 'Y', 'E', 'X'),
            WindowContents, (IPTR)(HGroup,
                Child, (IPTR)(NListviewObject,
                    MUIA_NListview_NList, (IPTR)(hidd_tree = NListtreeObject,
                        ReadListFrame,
                        MUIA_CycleChain, TRUE,
                        MUIA_NListtree_ConstructHook, (IPTR)&constructHook,
                        MUIA_NListtree_DestructHook, (IPTR)&destructHook,
                        MUIA_NListtree_DragDropSort, FALSE, /* forbid sorting by drag'n'drop */
                    End),
                End),
            End),
        End),
    End;

    if (app)
    {
        OOP_Object *hwRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        if (hwRoot)
        {
            /* This will kick our recursive enumeration into action */
            CALLHOOKPKT((struct Hook *)&enum_hook, hwRoot, MUIV_NListtree_Insert_ListNode_Root);
        }

        /* Quit application if the main window's closegadget or the esc key is pressed. */
        DoMethod(main_window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                 app, 2, 
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        DoMethod(property_window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                 property_window, 3, 
                 MUIM_Set, MUIA_Window_Open, FALSE);


        DoMethod(property_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 app, 3,
                 MUIM_CallHook, &property_hook, 0);

        DoMethod(expand_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 hidd_tree, 4,
                 MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Root, MUIV_NListtree_Open_TreeNode_All, 0);

        DoMethod(collapse_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 hidd_tree, 4,
                 MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Root, MUIV_NListtree_Close_TreeNode_All, 0);

        DoMethod(quit_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        DoMethod(hidd_tree, MUIM_Notify, MUIA_NListtree_DoubleClick, MUIV_EveryTime,
                 app, 3,
                 MUIM_CallHook, &property_hook, MUIV_TriggerValue);

        retval = TRUE;
    }

    return retval;
}

int __nocommandline = 1;

int main(void)
{
    if (!Locale_Initialize())
        return 20;

    if (!OOP_ObtainAttrBases(abd))
    {
        Locale_Deinitialize();
        return 20;
    }

    if (GUIinit())
    {
        SET(main_window, MUIA_Window_Open, TRUE);

        if (XGET(main_window, MUIA_Window_Open))
        {
            DoMethod(app, MUIM_Application_Execute);
            SET(main_window, MUIA_Window_Open, FALSE);
        }

        DisposeObject(app);
    }

    OOP_ReleaseAttrBases(abd);

    Locale_Deinitialize();
    return 0;
}
