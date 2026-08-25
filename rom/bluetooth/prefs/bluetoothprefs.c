/*
** Bluetooth preferences -- a Trident-style front end for bluetooth.library.
**
** Structured the same way Poseidon's Trident is: this file is just the shell
** (open libraries, create the custom classes, build the application, run the
** input loop). All of the panel and its logic live in ActionClass.c, with the
** navigation list in IconListClass.c.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <libraries/iffparse.h>

#define MUIMASTER_YES_INLINE_STDARG
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/icon.h>

#include "bluetoothprefs.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "ScanWinClass.h"
#include "DevWinClass.h"
#include "debug.h"

#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

struct Library      *MUIMasterBase = NULL;
struct Library      *BluetoothBase = NULL;
struct Library      *IconBase      = NULL;

struct MUI_CustomClass *IconListClass = NULL;
struct MUI_CustomClass *ActionClass   = NULL;
struct MUI_CustomClass *ScanWinClass  = NULL;
struct MUI_CustomClass *DevWinClass   = NULL;

const char *version = "$VER: Bluetooth 1.1 (25.08.2026) The AROS Development Team";

static void CloseClasses(void)
{
    if(ActionClass)   { MUI_DeleteCustomClass(ActionClass);   ActionClass = NULL; }
    if(DevWinClass)   { MUI_DeleteCustomClass(DevWinClass);   DevWinClass = NULL; }
    if(ScanWinClass)  { MUI_DeleteCustomClass(ScanWinClass);  ScanWinClass = NULL; }
    if(IconListClass) { MUI_DeleteCustomClass(IconListClass); IconListClass = NULL; }
}

static BOOL OpenClasses(void)
{
    IconListClass = MUI_CreateCustomClass(NULL, MUIC_List,   NULL, sizeof(struct IconListData), IconListDispatcher);
    ScanWinClass  = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct ScanWinData),  ScanWinDispatcher);
    DevWinClass   = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct DevWinData),   DevWinDispatcher);
    ActionClass   = MUI_CreateCustomClass(NULL, MUIC_Group,  NULL, sizeof(struct BtActionData), ActionDispatcher);
    return (BOOL)(IconListClass && ScanWinClass && DevWinClass && ActionClass);
}

int main(int argc, char **argv)
{
    Object *app, *win, *actionobj;
    Object *mi_about, *mi_quit, *mi_iconify, *mi_save, *mi_use, *mi_flush, *mi_savelog;

    (void)argc; (void)argv;

    if(!(BluetoothBase = OpenLibrary("bluetooth.library", 1)))
    {
        if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
        {
            struct EasyStruct es = { sizeof(struct EasyStruct), 0, "Bluetooth", "Could not open bluetooth.library.", "OK" };
            EasyRequestArgs(NULL, &es, NULL, NULL);
            CloseLibrary((struct Library *)IntuitionBase);
        }
        return 20;
    }
    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN))) { CloseLibrary(BluetoothBase); return 20; }
    IconBase = OpenLibrary("icon.library", 0);

    if(!OpenClasses())
    {
        CloseClasses();
        if(IconBase) CloseLibrary(IconBase);
        CloseLibrary(MUIMasterBase);
        CloseLibrary(BluetoothBase);
        return 20;
    }

    app = ApplicationObject,
        MUIA_Application_Title,       (IPTR)"Bluetooth",
        MUIA_Application_Version,     (IPTR)version,
        MUIA_Application_Copyright,   (IPTR)"(c)2026 The AROS Development Team",
        MUIA_Application_Author,      (IPTR)"The AROS Development Team",
        MUIA_Application_Description, (IPTR)"Bluetooth preferences",
        MUIA_Application_Base,        (IPTR)"BTPREFS",
        MUIA_Application_SingleTask,  TRUE,
        MUIA_Application_Menustrip, MenustripObject,
            Child, MenuObjectT((IPTR)"Project"),
                Child, mi_about = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"About MUI...", End,
                Child, MenuitemObject, MUIA_Menuitem_Title, (IPTR)NM_BARLABEL, End,
                Child, mi_iconify = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Iconify", MUIA_Menuitem_Shortcut, (IPTR)"I", End,
                Child, mi_quit = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Quit", MUIA_Menuitem_Shortcut, (IPTR)"Q", End,
                End,
            Child, MenuObjectT((IPTR)"Log"),
                Child, mi_savelog = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Save log...", End,
                Child, mi_flush = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Flush all", End,
                End,
            Child, MenuObjectT((IPTR)"Settings"),
                Child, mi_save = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Save", MUIA_Menuitem_Shortcut, (IPTR)"S", End,
                Child, mi_use = MenuitemObject, MUIA_Menuitem_Title, (IPTR)"Use", End,
                End,
            End,
        SubWindow, win = WindowObject,
            MUIA_Window_Title, (IPTR)"Bluetooth",
            MUIA_Window_ID,    MAKE_ID('B','T','P','R'),
            WindowContents, actionobj = NewObject(ActionClass->mcc_Class, NULL, TAG_END),
            End,
        End;

    if(!app)
    {
        CloseClasses();
        if(IconBase) CloseLibrary(IconBase);
        CloseLibrary(MUIMasterBase);
        CloseLibrary(BluetoothBase);
        return 20;
    }

    DoMethod(win, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(mi_about, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             app, 2, MUIM_Application_AboutMUI, win);
    DoMethod(mi_iconify, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             app, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
    DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(mi_savelog, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_BtA_SaveLog);
    DoMethod(mi_flush, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_BtA_FlushLog);
    DoMethod(mi_save, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_BtA_Save);
    DoMethod(mi_use, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_BtA_Use);

    set(win, MUIA_Window_Open, TRUE);

    {
        ULONG sigs = 0;
        while(DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs) != MUIV_Application_ReturnID_Quit)
        {
            if(sigs)
            {
                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C) break;
            }
        }
    }

    set(win, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);

    CloseClasses();
    if(IconBase) CloseLibrary(IconBase);
    CloseLibrary(MUIMasterBase);
    CloseLibrary(BluetoothBase);
    return 0;
}
