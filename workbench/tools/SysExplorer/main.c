/*
    Copyright (C) 2013, The AROS Development Team.
    $Id: main.c 41537 2011-09-22 08:33:28Z sonic $
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
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

#define DEBUG 1
#include <aros/debug.h>

#define APPNAME "SysExplorer"
#define VERSION "SysExplorer 0.2"

struct ClassDisplay
{
    CONST_STRPTR classID;
    struct MUI_CustomClass **muiClass;
};

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

static Object *app, *main_window, *property_window, *hidd_tree;
static Object *property_menu, *expand_menu, *collapse_menu, *quit_menu;

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;

const struct OOP_ABDescr abd[] =
{
    {IID_Hidd, &HiddAttrBase},
    {IID_HW  , &HWAttrBase  },
    {NULL    , NULL         }
};

static const struct ClassDisplay classWindows[] =
{
    {CLID_HW_Root, &ComputerWindow_CLASS},
    {CLID_Hidd   , &GenericWindow_CLASS },
    {NULL        , NULL                }
};

static struct Hook enum_hook;
static struct Hook property_hook;


AROS_UFH3S(BOOL, enumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    CONST_STRPTR name = NULL;
    
    OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);

    if (name)
    {
        // node
        struct MUI_NListtree_TreeNode *tn;

        tn = (APTR)DoMethod(hidd_tree, MUIM_NListtree_Insert, name, obj,
                            parent, MUIV_NListtree_Insert_PrevNode_Tail,
                            TNF_LIST|TNF_OPEN);
        if (tn)
        {
            HW_EnumDrivers(obj, &enum_hook, tn);
        }
    }
    else
    {
        // leaf
        // we're storing the device handle as userdata in the tree node
        OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);
        DoMethod(hidd_tree, MUIM_NListtree_Insert, name, obj,
                 parent, MUIV_NListtree_Insert_PrevNode_Tail, 0);
    }

    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, closeFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    SET(obj, MUIA_Window_Open, FALSE);
    DoMethod(app, OM_REMMEMBER, obj);
    DisposeObject(obj);
    
    AROS_USERFUNC_EXIT
};

static const struct Hook close_hook =
{
    .h_Entry = closeFunc
};

AROS_UFH3S(void, propertyFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct MUI_NListtree_TreeNode **, tn, A1))
{
    AROS_USERFUNC_INIT

    D(bug("propertyFunc called tn: %p\n", *tn));

    struct MUI_NListtree_TreeNode *node = *tn;
    OOP_Object *obj;
    unsigned int i;

    if (node == NULL)
    {
        /* if we were called from menu we must 1st find the current entry */
        node = (struct MUI_NListtree_TreeNode *)XGET(hidd_tree, MUIA_NListtree_Active);
    }

    if (node == NULL)
        return;

    /*
     * TODO: Do not allow to open properties window for the same object
     * multiple times.
     */
    obj = node->tn_User;
    for (i = 0; classWindows[i].classID; i++)
    {
        OOP_Class *cl;

        for (cl = OOP_OCLASS(obj); cl ; cl = cl->superclass)
        {
            if (!strcmp(cl->ClassNode.ln_Name, classWindows[i].classID))
            {
                Object *window = NewObject((*classWindows[i].muiClass)->mcc_Class, NULL,
                                     MUIA_PropertyWin_Object, (IPTR)node->tn_User,
                                 TAG_DONE);

                if (window)
                {
                    DoMethod(app, OM_ADDMEMBER, window);
                    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                             window, 3, 
                             MUIM_CallHook, &close_hook, 0);
                    SET(window, MUIA_Window_Open, TRUE);
                }                
                return;
            }
        }   
    }

    AROS_USERFUNC_EXIT
}

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
                    GroupFrame,
                    MUIA_FrameTitle, __(MSG_HIDD_TREE),
                    MUIA_NListview_NList, (IPTR)(hidd_tree = NListtreeObject,
                        ReadListFrame,
                        MUIA_CycleChain, TRUE,
                        MUIA_NListtree_DragDropSort, FALSE, /* forbid sorting by drag'n'drop */
                    End),
                End),
            End),
        End),
    End;

    if (app)
    {
        enum_hook.h_Entry = enumFunc;
        property_hook.h_Entry = propertyFunc;

        OOP_Object *hwRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        if (hwRoot)
        {
            /* This will kick our recursive enumeration into action */
            CALLHOOKPKT(&enum_hook, hwRoot, MUIV_NListtree_Insert_ListNode_Root);
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
