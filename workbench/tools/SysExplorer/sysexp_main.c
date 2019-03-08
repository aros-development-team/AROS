/*
    Copyright (C) 2013-2018, The AROS Development Team.
    $Id$
*/

#include <aros/debug.h>

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/system.h>
#include <hidd/gfx.h>
#if (0)
#include <hidd/storage.h>
#endif
#include <libraries/asl.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/sysexp.h>

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

#include "enums.h"

#define APPNAME "SysExplorer"
#define VERSION "SysExplorer 0.5"
#define SysexpModuleDir	"PROGDIR:SysExpModules"

int __nocommandline = 1;

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

extern void sysexp_initlib(struct SysexpBase **SysexpBasePtr);

static Object *app, *main_window, *property_window;
static Object *property_menu, *expand_menu, *collapse_menu, *quit_menu;

OOP_AttrBase HiddAttrBase;
OOP_AttrBase HWAttrBase;

OOP_MethodID HWBase;

const struct OOP_ABDescr abd[] =
{
    {IID_Hidd        ,  &HiddAttrBase },
    {IID_HW          ,  &HWAttrBase   },
    {NULL            ,  NULL          }
};

AROS_UFH3S(BOOL, enumFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(OOP_Object*, obj, A2),
    AROS_UFHA(void *, parent, A1))
{
    AROS_USERFUNC_INIT

    BOOL objValid = TRUE;
    CONST_STRPTR name = NULL;
    struct MUI_NListtree_TreeNode *tn;
    ULONG flags = 0;
    struct SysexpEnum_data *edata = (struct SysexpEnum_data *)h->h_Data;
    struct InsertObjectMsg msg =
    {
        .obj = obj,
        .winClass = NULL
    };
    struct ClassHandlerNode *clHandlers;
    struct SysexpBase *SysexpBase;

    if (!edata)
        return TRUE;

    SysexpBase = edata->ed_sysexpbase;
    clHandlers = FindObjectHandler(obj, edata->ed_list);

    D(bug("[SysExplorer] %s: list @ 0x%p\n", __func__, edata->ed_list));
    D(bug("[SysExplorer] %s: handler @ 0x%p\n", __func__, clHandlers));

    if (clHandlers)
    {
        if (clHandlers->muiClass)
            msg.winClass = clHandlers->muiClass;
        if (clHandlers->enumFunc)
            flags = TNF_LIST|TNF_OPEN;
        if (clHandlers->validFunc)
            objValid = clHandlers->validFunc(obj, &flags);
    }

    if (objValid)
    {
        int objnum;

        /* This is either HW or HIDD subclass */
        OOP_GetAttr(obj, aHW_ClassName, (IPTR *)&name);
        if (!name)
            OOP_GetAttr(obj, aHidd_HardwareName, (IPTR *)&name);

        D(bug("[SysExplorer] %s: name = '%s'\n", __func__, name));

        objnum = ++SysexpBase->GlobalCount;
#if (1)
        tn = (APTR)DoMethod(SysexpBase->sesb_Tree, MUIM_NListtree_Insert, name, &msg,
                            parent, MUIV_NListtree_Insert_PrevNode_Sorted, flags);
        D(bug("[SysExplorer] %s: Inserted TreeNode 0x%p <%s> UserData 0x%p\n", __func__, tn, tn->tn_Name, tn->tn_User));

        /* If we have enumerator for this class, call it now */
        if (clHandlers && clHandlers->enumFunc && (flags & TNF_LIST))
            clHandlers->enumFunc(obj, tn);

        if (objnum == SysexpBase->GlobalCount)
        {
            tn->tn_Flags &= ~flags;
        }
#else
        DoMethod(_app(SysexpBase->sesb_Tree), MUIM_Application_PushMethod,
                SysexpBase->sesb_Tree, 6, MUIM_NListtree_Insert, name, &msg,
                        parent, MUIV_NListtree_Insert_PrevNode_Sorted, flags);
#endif
    }
    return FALSE; /* Continue enumeration */

    AROS_USERFUNC_EXIT
}

static struct SysexpEnum_data privatehookdata =
{
    NULL,
    NULL
};

static struct Hook enum_hook =
{
    .h_Entry = enumFunc,
    .h_Data = &privatehookdata
};

void hwEnum(OOP_Object *obj, struct MUI_NListtree_TreeNode *tn)
{
    HW_EnumDrivers(obj, &enum_hook, tn);
}

AROS_UFH3S(void, closeFunc,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(Object*, obj, A2),
    AROS_UFHA(struct ObjectUserData **, msg, A1))
{
    AROS_USERFUNC_INIT

    D(bug("[SysExplorer] %s: Close window 0x%p, ObjectUserData 0x%p\n", __func__, obj, *msg));

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

    D(bug("[SysExplorer] %s: insertMsg for '%s' @0x%p, ObjectUserData @ 0x%p\n", __func__, msg->Name, insertMsg, data));
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
    struct SysexpHook_data *hdata = (struct SysexpHook_data *)h->h_Data;
    struct SysexpBase *SysexpBase;

    if (!hdata)
        return;

    SysexpBase = hdata->hd_sysexpbase;

    if (node == NULL)
    {
        /* if we were called from menu we must first find the current entry */
        node = (struct MUI_NListtree_TreeNode *)XGET(SysexpBase->sesb_Tree, MUIA_NListtree_Active);
    }

    if (node == NULL)
    {
        /* Do nothing if still no current entry */
        return;
    }

    data = node->tn_User;
    D(bug("[SysExplorer] %s: propertyFunc called: TreeNode 0x%p <%s> UserData 0x%p\n", __func__, node, node->tn_Name, data));
    D(bug("[SysExplorer] %s: Window 0x%p\n", __func__, data->win));

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
        D(bug("[SysExplorer] %s: Created window 0x%p\n", __func__, data->win));

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
    .h_Entry = propertyFunc,
    .h_Data = &privatehookdata
};

static BOOL GUIinit(struct SysexpBase *SysexpBase)
{
    BOOL retval = FALSE;

    app = ApplicationObject,
        MUIA_Application_Title,         (IPTR)APPNAME,
        MUIA_Application_Version,       (IPTR)VERSION,
        MUIA_Application_Copyright,     (IPTR)"(C) 2013, The AROS Development Team",
        MUIA_Application_Author,        (IPTR)"Pavel Fedin",
        MUIA_Application_Base,          (IPTR)APPNAME,
        MUIA_Application_Description,   __(MSG_DESCRIPTION),

        MUIA_Application_Menustrip, (IPTR)(MenustripObject,
            MUIA_Family_Child, (IPTR)(MenuObject,
                MUIA_Menu_Title, __(MSG_MENU_PROJECT),
                MUIA_Family_Child, (IPTR)(property_menu = MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_PROPERTIES),
                End),
                MUIA_Family_Child, (IPTR)(expand_menu = MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_EXPANDALL),
                End),
                MUIA_Family_Child, (IPTR)(collapse_menu = MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_COLLAPSEALL),
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
                    MUIA_Background, MUII_ListBack,
                    MUIA_NListview_NList, (IPTR)(SysexpBase->sesb_Tree = NListtreeObject,
                        ReadListFrame,
                        MUIA_Background, MUII_ListBack,
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
                 SysexpBase->sesb_Tree, 4,
                 MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Root, MUIV_NListtree_Open_TreeNode_All, 0);

        DoMethod(collapse_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 SysexpBase->sesb_Tree, 4,
                 MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Root, MUIV_NListtree_Close_TreeNode_All, 0);

        DoMethod(quit_menu, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                 app, 2,
                 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

        DoMethod(SysexpBase->sesb_Tree, MUIM_Notify, MUIA_NListtree_DoubleClick, MUIV_EveryTime,
                 app, 3,
                 MUIM_CallHook, &property_hook, MUIV_TriggerValue);

        retval = TRUE;
    }

    return retval;
}

AROS_UFH3(void, SysExplorer__Proc_EnumerateHardware,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    OOP_Object *hwRoot = OOP_NewObject(NULL, CLID_HW_Root, NULL);
    if (hwRoot)
    {
        D(bug("[SysExplorer] %s: Enumerating Devices..\n", __func__));

        /* Kick the recursive enumeration into action */
        CALLHOOKPKT((struct Hook *)&enum_hook, hwRoot, MUIV_NListtree_Insert_ListNode_Root);
    }
    D(bug("[SysExplorer] %s: Finished..\n", __func__));

    return;

    AROS_USERFUNC_EXIT
}

int main(void)
{
    struct SysexpBase *SysexpBase = NULL;
    BPTR SysexpDirLock = BNULL;
    APTR SysExpModuleBase;
    struct SysexpIntModule *SysexpModule;

    sysexp_initlib(&SysexpBase);

    if (SysexpBase)
    {
        if (!Locale_Initialize())
            return 20;

        if (!OOP_ObtainAttrBases(abd))
        {
            Locale_Deinitialize();
            return 20;
        }

        privatehookdata.ed_sysexpbase = SysexpBase;
        privatehookdata.ed_list = &SysexpBase->sesb_ClassHandlers;

        SysexpDirLock = Lock(SysexpModuleDir, SHARED_LOCK);
        if (SysexpDirLock)
        {
            struct FileInfoBlock *SysexpFileFIB = AllocDosObject(DOS_FIB, NULL);
            if (SysexpFileFIB != NULL)
            {
                D(bug("[SysExplorer] %s: scanning for support modules ..\n", __func__));

                if (Examine(SysexpDirLock, SysexpFileFIB))
                {
                    while(ExNext(SysexpDirLock, SysexpFileFIB))
                    {
                        int namelen = strlen(SysexpFileFIB->fib_FileName);
                        char *SysExpModuleFilename;

                        if ((namelen > 4) && (strcmp(&SysexpFileFIB->fib_FileName[namelen - 4], ".dbg")))
                        {
                            SysExpModuleFilename = AllocVec((strlen(SysexpModuleDir) + 1 + namelen + 1), MEMF_CLEAR);

                            sprintf(SysExpModuleFilename, "%s/%s", SysexpModuleDir, SysexpFileFIB->fib_FileName);

                            D(bug("[SysExplorer] %s: Module '%s' ('%s')\n", __func__, SysexpFileFIB->fib_FileName, SysExpModuleFilename));

                            if ((SysExpModuleBase = (APTR)OpenLibrary(SysExpModuleFilename, 0)) != NULL)
                            {
                                D(bug("[SysExplorer] %s: '%s' Loaded @ 0x%p\n", __func__, SysexpFileFIB->fib_FileName, SysExpModuleBase));
                                    AROS_LC1(void, ModuleInit,
                                        AROS_LCA(void *, (SysexpBase), A0),
                                        struct Library *, SysExpModuleBase, 5, Module);
                            }
                            FreeVec(SysExpModuleFilename);
                        }
                    }
                }
                D(bug("[SysExplorer] %s: scan finished\n", __func__));

                FreeDosObject(DOS_FIB, SysexpFileFIB);
            }
            UnLock(SysexpDirLock);
        }

        SysexpBase->GlobalCount = 0;

        ForeachNode(&SysexpBase->sesb_Modules, SysexpModule)
        {
            if (SysexpModule->seim_Module.sem_Startup)
                SysexpModule->seim_Module.sem_Startup(SysexpBase);
        }

        if (GUIinit(SysexpBase))
        {
            SET(main_window, MUIA_Window_Open, TRUE);

            if (XGET(main_window, MUIA_Window_Open))
            {
#if (1)
                AROS_UFC3(void, SysExplorer__Proc_EnumerateHardware,
                                AROS_UFCA(STRPTR,              NULL, A0),
                                AROS_UFCA(ULONG,               0, D0),
                                AROS_UFCA(struct ExecBase *,   SysBase, A6) );

#else
                CreateNewProcTags(
                                NP_Entry,       (IPTR)SysExplorer__Proc_EnumerateHardware,
                                NP_Name,        (IPTR)"SysExplorer.Enumerator",
                                NP_Synchronous, FALSE,
                                NP_StackSize,   40000,
                                TAG_DONE);
#endif

                DoMethod(app, MUIM_Application_Execute);
                SET(main_window, MUIA_Window_Open, FALSE);
            }

            DisposeObject(app);
        }

        ForeachNode(&SysexpBase->sesb_Modules, SysexpModule)
        {
            if (SysexpModule->seim_Module.sem_Shutdown)
                SysexpModule->seim_Module.sem_Shutdown(SysexpBase);
            CloseLibrary(SysexpModule->seim_ModuleBase);
        }

        OOP_ReleaseAttrBases(abd);

        Locale_Deinitialize();
    }
    return 0;
}

BOOL sysexplorer_init(void)
{
    D(bug("[SysExplorer] Initialising..\n"));

    HWBase = OOP_GetMethodID(IID_HW, 0);

    D(bug("[SysExplorer] Init complete\n"));

    return TRUE;
}

BOOL sysexplorer_loadmodules(void)
{
    D(bug("[SysExplorer] Loading Support Modules..\n"));
    D(bug("[SysExplorer] Modules Loaded\n"));

    return TRUE;
}

ADD2INIT(sysexplorer_init, 0);
ADD2INIT(sysexplorer_loadmodules, 127);
