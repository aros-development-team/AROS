/*
** Trident V0.0 (08-Mar-02) by Chris Hodges <hodges@in.tum.de>
**
** Project started: 08-Mar-02
** Releases       :
*/

#include "debug.h"

#define USE_INLINE_STDARG
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/usbclass.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include <workbench/startup.h>

#include "Trident.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "DevWinClass.h"
#include "CfgListClass.h"

#define TMPL_SWITCH     1
#define TMPL_NUMERIC    2
#define TMPL_ARGUMENT   4


#include <stdlib.h>


/* defines */

struct WBStartup *_WBenchMsg;

extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Library *UtilityBase;
extern struct Library *MUIMasterBase;
struct Library *ps;
extern struct Library *IconBase;

struct MUI_CustomClass *ActionClass     = NULL;
struct MUI_CustomClass *IconListClass   = NULL;
struct MUI_CustomClass *DevWinClass     = NULL;
struct MUI_CustomClass *CfgListClass    = NULL;

Object *appobj = NULL;
Object *mainwinobj = NULL;
Object *actionobj = NULL;
BOOL registermode = FALSE;

static Object *mi_about;
static Object *mi_aboutmui;
static Object *mi_help;
static Object *mi_online;
static Object *mi_offline;
static Object *mi_iconify;
static Object *mi_quit;
static Object *mi_saveerrors;
static Object *mi_flusherrors;
static Object *mi_savedevlist;
static Object *mi_loadprefs;
static Object *mi_saveprefs;
static Object *mi_saveprefsas;
static Object *mi_muiprefs;

#define ARGS_REGMODE      0
#define ARGS_NOGUI        1
#define ARGS_SAVE         2
#define ARGS_SIZEOF       3

static char *template = "REGISTERMODE/S,NOGUI/S,SAVE/S";
static IPTR ArgsArray[ARGS_SIZEOF];
static struct RDArgs *ArgsHook = NULL;
static struct DiskObject *IconObject = NULL;
static LONG NumericArgsBuffer[32];

/* /// "GetToolTypes()" */
BOOL GetToolTypes(void)
{
    BPTR dir;
    struct WBArg *wba;
    char currtemp[512];
    char currkey[128];
    LONG currtype;
    char *templatepos = template;
    char *bufpos;
    char *bufpos2;
    BOOL valid = FALSE;
    IPTR *argsptr = ArgsArray;
    LONG *numargsptr = NumericArgsBuffer;

    if(!_WBenchMsg) return(0);
    wba = _WBenchMsg->sm_ArgList;

    dir = CurrentDir(wba->wa_Lock);
    if((IconObject = GetDiskObject(wba->wa_Name)))
    {
        valid = TRUE;
        if(IconObject->do_ToolTypes)
        {
            while(*templatepos)
            {
                bufpos = currtemp;
                while(*templatepos)
                {
                    if(*templatepos == ',')
                    {
                        templatepos++;
                        break;
                    }
                    *bufpos++ = *templatepos++;
                }
                *bufpos = 0;
                bufpos = currtemp;
                currtype = 0;
                while(*bufpos)
                {
                    if(*bufpos++ == '/')
                    {
                        switch(*bufpos++)
                        {
                            case 'S':
                                currtype |= TMPL_SWITCH;
                                break;
                            case 'A':
                                currtype |= TMPL_ARGUMENT;
                                break;
                            case 'N':
                                currtype |= TMPL_NUMERIC;
                                break;
                        }
                    }
                }
                bufpos2 = currtemp;
                *argsptr = 0L;
                while(*bufpos2)
                {
                    bufpos = currkey;
                    while(*bufpos2)
                    {
                        if(*bufpos2 == '/')
                        {
                            bufpos2 = currtemp + strlen(currtemp);
                            break;
                        }
                        if(*bufpos2 == '=')
                        {
                            bufpos2++;
                            break;
                        }
                        *bufpos++ = *bufpos2++;
                    }
                    *bufpos = 0;
                    if((bufpos = FindToolType(IconObject->do_ToolTypes, currkey)))
                    {
                        if(currtype & TMPL_SWITCH)
                        {
                            *argsptr = -1;
                        } else {
                            if(currtype & TMPL_NUMERIC)
                            {
                                *numargsptr = atoi(bufpos);
                                *argsptr = (LONG) numargsptr++;
                            } else {
                                *argsptr = (LONG) bufpos;
                            }
                        }
                    }
                }
                if((currtype & TMPL_ARGUMENT) && (*argsptr == 0L))
                {
                    valid = FALSE;
                    break;
                }
                argsptr++;
            }
        }
    }
    CurrentDir(dir);
    return(valid);
}
/* \\\ */

/* /// "CleanupClasses()" */
void CleanupClasses(void)
{
    if(DevWinClass)
    {
        MUI_DeleteCustomClass(DevWinClass);
        DevWinClass = NULL;
    }
    if(ActionClass)
    {
        MUI_DeleteCustomClass(ActionClass);
        ActionClass = NULL;
    }
    if(IconListClass)
    {
        MUI_DeleteCustomClass(IconListClass);
        IconListClass = NULL;
    }
    if(CfgListClass)
    {
        MUI_DeleteCustomClass(CfgListClass);
        CfgListClass = NULL;
    }
}
/* \\\ */

/* /// "SetupClasses()" */
BOOL SetupClasses(void)
{
    DevWinClass     = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct DevWinData)    , DevWinDispatcher);
    ActionClass     = MUI_CreateCustomClass(NULL, MUIC_Group , NULL, sizeof(struct ActionData)    , ActionDispatcher);
    IconListClass   = MUI_CreateCustomClass(NULL, MUIC_List  , NULL, sizeof(struct IconListData)  , IconListDispatcher);
    CfgListClass    = MUI_CreateCustomClass(NULL, MUIC_List  , NULL, sizeof(struct CfgListData)   , CfgListDispatcher);

    if(ActionClass && IconListClass && DevWinClass && CfgListClass)
        return(TRUE);

    return(FALSE);
}
/* \\\ */

/* /// "fail()" */
void fail(char *str)
{
    if(appobj)
    {
        MUI_DisposeObject(appobj);
        appobj = NULL;
    }
    CleanupClasses();

    if(ArgsHook)
    {
        FreeArgs(ArgsHook);
        ArgsHook = NULL;
    }

    if(ps)
    {
        CloseLibrary(ps);
        ps = NULL;
    }

    if(IconObject)
    {
        FreeDiskObject(IconObject);
        IconObject = NULL;
    }

    if(_WBenchMsg && str)
    {
        struct EasyStruct errorReq = { sizeof(struct EasyStruct), 0, "Trident", NULL, "Cancel" };
        errorReq.es_TextFormat = str;
        EasyRequest(NULL, &errorReq, NULL, NULL);
    }

    if(MUIMasterBase)
    {
        CloseLibrary(MUIMasterBase);
        MUIMasterBase = NULL;
    }

    if(str)
    {
        PutStr(str);
        exit(20);
    }
    exit(0);
}
/* \\\ */

/* /// "main()" */
int main(int argc, char *argv[])
{
    if(!argc)
    {
         _WBenchMsg = (struct WBStartup *) argv;
        if(!GetToolTypes())
        {
            fail("Wrong or missing ToolTypes!");
        }
    } else {
        if(!(ArgsHook = ReadArgs(template, ArgsArray, NULL)))
        {
            fail("Wrong arguments!\n");
        }
    }

    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        fail("Failed to open version 4 of poseidon.library.\n");
    }

    if(ArgsArray[ARGS_NOGUI])
    {
        if(ArgsArray[ARGS_SAVE])
        {
            InternalCreateConfig();
            psdSaveCfgToDisk(NULL, FALSE);
        } else {
            IPTR cfgread = FALSE;
            psdGetAttrs(PGA_STACK, NULL, PA_ConfigRead, &cfgread, TAG_DONE);
            if(!cfgread)
            {
                psdLoadCfgFromDisk(NULL);
            } else {
                psdParseCfg();
            }
        }
        fail(NULL);
    }

    if((ps->lib_Version == 4) && (ps->lib_Revision < 3))
    {
        fail("Sorry, this version of Trident requires at least version 4.3 of poseidon.library!\n");
    }

    {
        struct Task *thistask;
        IPTR stackfree;
        thistask = FindTask(NULL);
        stackfree = ((IPTR) thistask->tc_SPUpper) - ((IPTR) thistask->tc_SPLower);
        if(stackfree < 16000)
        {
            fail("Trident needs at least 16KB of stack! Please change the tooltypes accordingly!\n");
        }
    }

    {
        IPTR cfgread = FALSE;
        psdGetAttrs(PGA_STACK, NULL, PA_ConfigRead, &cfgread, TAG_DONE);
        if(!cfgread)
        {
            psdLoadCfgFromDisk(NULL);
        } else {
            psdParseCfg();
        }
    }
    registermode = ArgsArray[ARGS_REGMODE];

    if(!(MUIMasterBase = OpenLibrary(MUIMASTER_NAME,MUIMASTER_VMIN)))
        fail("Failed to open "MUIMASTER_NAME". This application requires MUI!\n");

    if(!SetupClasses())
        fail("Could not create custom classes.\n");

    appobj = ApplicationObject,
        MUIA_Application_Title      , "Trident",
        MUIA_Application_Version    , "4.3 (27-May-09)",
        MUIA_Application_Copyright  , "©2002-2009 Chris Hodges",
        MUIA_Application_Author     , "Chris Hodges <chrisly@platon42.de>",
        MUIA_Application_Description, "Poseidon USB Stack GUI",
        MUIA_Application_Base       , "TRIDENT",
        MUIA_Application_SingleTask , TRUE,
        MUIA_Application_DiskObject , IconObject,
        MUIA_Application_HelpFile   , "HELP:Poseidon.guide",
        MUIA_Application_Menustrip  , MenustripObject,
            Child, MenuObjectT("Project"),
                Child, mi_about = MenuitemObject,
                    MUIA_Menuitem_Title, "About...",
                    MUIA_Menuitem_Shortcut, "?",
                    End,
                Child, mi_aboutmui = MenuitemObject,
                    MUIA_Menuitem_Title, "About MUI...",
                    End,
                Child, mi_help = MenuitemObject,
                    MUIA_Menuitem_Title, "Help",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_online = MenuitemObject,
                    MUIA_Menuitem_Title, "Online",
                    MUIA_Menuitem_Shortcut, "O",
                    End,
                Child, mi_offline = MenuitemObject,
                    MUIA_Menuitem_Title, "Offline",
                    MUIA_Menuitem_Shortcut, "F",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_iconify = MenuitemObject,
                    MUIA_Menuitem_Title, "Iconify",
                    MUIA_Menuitem_Shortcut, "H",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_quit = MenuitemObject,
                    MUIA_Menuitem_Title, "Quit",
                    MUIA_Menuitem_Shortcut, "Q",
                    End,
                End,
            Child, MenuObjectT("Information"),
                Child, mi_saveerrors = MenuitemObject,
                    MUIA_Menuitem_Title, "Save error log...",
                    End,
                Child, mi_flusherrors = MenuitemObject,
                    MUIA_Menuitem_Title, "Flush errors",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_savedevlist = MenuitemObject,
                    MUIA_Menuitem_Title, "Save device list...",
                    End,
                End,
            Child, MenuObjectT("Settings"),
                Child, mi_loadprefs = MenuitemObject,
                    MUIA_Menuitem_Title, "Load...",
                    MUIA_Menuitem_Shortcut, "L",
                    End,
                Child, mi_saveprefs = MenuitemObject,
                    MUIA_Menuitem_Title, "Save",
                    MUIA_Menuitem_Shortcut, "S",
                    End,
                Child, mi_saveprefsas = MenuitemObject,
                    MUIA_Menuitem_Title, "Save as...",
                    MUIA_Menuitem_Shortcut, "A",
                    End,
                Child, MenuitemObject,
                    MUIA_Menuitem_Title, NM_BARLABEL,
                    End,
                Child, mi_muiprefs = MenuitemObject,
                    MUIA_Menuitem_Title, "MUI Settings",
                    MUIA_Menuitem_Shortcut, "M",
                    End,
                End,
            End,

        SubWindow, mainwinobj = WindowObject,
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, "Trident 4.3",
            MUIA_HelpNode, "usingtrident",

            WindowContents, actionobj = NewObject(ActionClass->mcc_Class, 0, TAG_END),
            End,
        End;

    if(!appobj) fail("Failed to create Application. Already running?\n");

/*
** Add notifications
*/
    DoMethod(mi_aboutmui, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             appobj, 2, MUIM_Application_AboutMUI, mainwinobj);
    DoMethod(mi_about, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_About);
    DoMethod(mi_online, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_Online);
    DoMethod(mi_offline, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_Offline);
    DoMethod(mi_help, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             appobj, 5, MUIM_Application_ShowHelp, mainwinobj, NULL, "main", 0);
    DoMethod(mi_iconify, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             appobj, 3, MUIM_Set, MUIA_Application_Iconified, TRUE);
    DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             appobj, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(mi_saveerrors, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_SaveErrors);
    DoMethod(mi_flusherrors, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_FlushErrors);
    DoMethod(mi_savedevlist, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_SaveDeviceList);

    DoMethod(mi_loadprefs, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_LoadPrefs);
    DoMethod(mi_saveprefs, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_SavePrefs);
    DoMethod(mi_saveprefsas, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_SavePrefsAs);
    DoMethod(mi_muiprefs, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
             appobj, 2, MUIM_Application_OpenConfigWindow, 0);

    DoMethod(mainwinobj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
             actionobj, 1, MUIM_Action_UseQuit);
    DoMethod(appobj, MUIM_Notify, MUIA_Application_DoubleStart, MUIV_EveryTime,
             actionobj, 1, MUIM_Action_WakeUp);

    DoMethod(appobj, MUIM_Application_Load, MUIV_Application_Load_ENVARC);

/*
** This is the ideal input loop for an object oriented MUI application.
** Everything is encapsulated in classes, no return ids need to be used,
** we just check if the program shall terminate.
** Note that MUIM_Application_NewInput expects sigs to contain the result
** from Wait() (or 0). This makes the input loop significantly faster.
*/

    {
        ULONG sigs = 0;
        ULONG sigmask;
        LONG isopen;
        LONG iconify;

        get(appobj, MUIA_Application_Iconified, &iconify);
        set(mainwinobj, MUIA_Window_Open, TRUE);
        get(mainwinobj, MUIA_Window_Open, &isopen);
        if(!(isopen || iconify))
                fail("Couldn't open window! Maybe screen is too small. Try a higher resolution!\n");

        sigmask = 0;// FIXME 1L<<eventmsgport->mp_SigBit;
        while(DoMethod(appobj, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
        {
            if(sigs)
            {
                sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                if(sigs & SIGBREAKF_CTRL_C)
                    break;
            }
        }
        set(mainwinobj, MUIA_Window_Open, FALSE);
    }
    DoMethod(appobj, MUIM_Application_Save, MUIV_Application_Save_ENVARC);

/*
** Shut down...
*/
    fail(NULL);
    return(0); // never gets here, just to shut the compiler up
}
/* \\\ */

