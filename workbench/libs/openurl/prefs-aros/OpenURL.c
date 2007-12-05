/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
**  Main
*/


#include "OpenURL.h"
#define CATCOMP_NUMBERS
#include "loc.h"
#include <graphics/gfxbase.h>

#include <libraries/openurl.h>

/**************************************************************************/

LONG                   __stack = 32000; /* I think this is OK in every env */

struct IntuitionBase   *IntuitionBase = NULL;
struct GfxBase         *GfxBase = NULL;
struct Library         *MUIMasterBase = NULL;
struct Library         *UtilityBase = NULL;
struct Library         *IconBase = NULL;
struct Library         *OpenURLBase = NULL;
struct LocaleBase      *LocaleBase = NULL;

#if defined(__amigaos4__)
struct IntuitionIFace  *IIntuition = NULL;
struct GraphicsIFace   *IGraphics = NULL;
struct MUIMasterIFace  *IMUIMaster = NULL;
struct UtilityIFace    *IUtility = NULL;
struct IconIFace       *IIcon = NULL;
struct OpenURLIFace    *IOpenURL = NULL;
struct LocaleIFace     *ILocale = NULL;
#endif

struct MUI_CustomClass *g_appClass = NULL;
struct MUI_CustomClass *g_pensClass = NULL;
struct MUI_CustomClass *g_aboutClass = NULL;
struct MUI_CustomClass *g_winClass = NULL;
struct MUI_CustomClass *g_appListClass = NULL;
struct MUI_CustomClass *g_browserEditWinClass = NULL;
struct MUI_CustomClass *g_mailerEditWinClass = NULL;
struct MUI_CustomClass *g_FTPEditWinClass = NULL;
struct MUI_CustomClass *g_popportClass = NULL;
struct MUI_CustomClass *g_popphClass = NULL;

APTR                   g_pool = NULL;
struct Catalog         *g_cat = NULL;
//ULONG                  g_MUI4 = FALSE;

/**************************************************************************/

static ULONG
openStuff(ULONG *arg0,ULONG *arg1)
{
    *arg1 = 0;

    if (!(MUIMasterBase = OpenLibrary("muimaster.library",19)) || (MUIMasterBase->lib_Version<19))
    {
        *arg0 = 19;
        return MSG_Err_NoMUI;
    }
#if defined(__amigaos4__)
    if (!(IMUIMaster = (struct MUIMasterIFace *)GetInterface( MUIMasterBase, "main", 1L, NULL))) return MSG_Err_NoMUI;
#endif
    if (!(g_pool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR,8192,4196))) return MSG_Err_NoMem;

    *arg0 = 37;

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37))) return MSG_Err_NoIntuition;
    if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",37))) return MSG_Err_NoGfx;
    if (!(UtilityBase = OpenLibrary("utility.library",37))) return MSG_Err_NoUtility;
    if (!(IconBase = OpenLibrary("icon.library",37))) return MSG_Err_NoIcon;

#if defined(__amigaos4__)
    if (!(IIntuition = (struct IntuitionIFace *)GetInterface( (struct Library*)IntuitionBase, "main", 1L, NULL))) return MSG_Err_NoIntuition;
    if (!(IGraphics = (struct GraphicsIFace *)GetInterface( (struct Library*)GfxBase, "main", 1L, NULL))) return MSG_Err_NoGfx;
    if (!(IUtility = (struct UtilityIFace *)GetInterface( UtilityBase, "main", 1L, NULL))) return MSG_Err_NoUtility;
    if (!(IIcon = (struct IconIFace *)GetInterface( IconBase, "main", 1L, NULL))) return MSG_Err_NoIcon;
#endif

    if (!(OpenURLBase = OpenLibrary(OPENURLNAME,6)) /*||
        ((OpenURLBase->lib_Version==OPENURLREV) && (OpenURLBase->lib_Revision<OPENURLREV))*/)
    {
        *arg0 = OPENURLVER;
        *arg1 = OPENURLREV;

        return MSG_Err_NoOpenURL;
    }

#if defined(__amigaos4__)
    if (!(IOpenURL = (struct OpenURLIFace *)GetInterface( OpenURLBase, "main", 1L, NULL)))
    {
        *arg0 = OPENURLVER;
        *arg1 = OPENURLREV;

        return MSG_Err_NoOpenURL;
    }

    // setup the AmiUpdate variable
    SetAmiUpdateENVVariable( "Open URL" );
#endif

    return 0;
}

/**************************************************************************/

static void
closeStuff(void)
{
    if (LocaleBase)
    {
        uninitStrings();

        if (g_cat) CloseCatalog(g_cat);

        #if defined(__amigaos4__)
        if (ILocale)
        {
            DropInterface((struct Interface*)ILocale);
            ILocale = NULL;
        }
        #endif

        CloseLibrary((struct Library *)LocaleBase);
        LocaleBase = NULL;
    }

#if defined(__amigaos4__)
    if (IMUIMaster)
    {
        DropInterface((struct Interface*)IMUIMaster);
        IMUIMaster = NULL;
    }
    if (IIntuition)
    {
        DropInterface((struct Interface*)IIntuition);
        IIntuition = NULL;
    }
    if (IUtility)
    {
        DropInterface((struct Interface*)IUtility);
        IUtility = NULL;
    }
    if (IIcon)
    {
        DropInterface((struct Interface*)IIcon);
        IIcon = NULL;
    }
    if (IOpenURL)
    {
        DropInterface((struct Interface*)IOpenURL);
        IOpenURL = NULL;
    }
#endif

    if (OpenURLBase)   CloseLibrary(OpenURLBase);
    if (IconBase)      CloseLibrary(IconBase);
    if (UtilityBase)   CloseLibrary(UtilityBase);
    if (GfxBase)       CloseLibrary((struct Library *)GfxBase);
    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);

    if (g_pool) DeletePool(g_pool);
}

/**************************************************************************/

static ULONG
createClasses(void)
{
    if (!initPopphClass())          return MSG_Err_PopphClass;
    if (!initPopportClass())        return MSG_Err_PopupPortClass;
    if (!initFTPEditWinClass())     return MSG_Err_NoFTPEditWinClass;
    if (!initMailerEditWinClass())  return MSG_Err_NoMailerEditWinClass;
    if (!initBrowserEditWinClass()) return MSG_Err_NoBrowserEditWinClass;
    if (!initAppListClass())        return MSG_Err_NoAppListClass;
    if (!initPensClass())           return MSG_Err_NoPensClass;
    if (!initWinClass())            return MSG_Err_NoWinClass;
    if (!initAppClass())            return MSG_Err_NoAppClass;

    return 0;
}

/**************************************************************************/

static void
disposeClasses(void)
{
    if (g_popphClass)          disposePopphClass();
    if (g_popportClass)        disposePopportClass();
    if (g_FTPEditWinClass)     disposeFTPEditWinClass();
    if (g_mailerEditWinClass)  disposeMailerEditWinClass();
    if (g_browserEditWinClass) disposeBrowserEditWinClass();
    if (g_appListClass)        disposeAppListClass();
    if (g_winClass)            disposeWinClass();
    if (g_pensClass)           disposePensClass();
    if (g_aboutClass)          disposeAboutClass();
    if (g_appClass)            disposeAppClass();
}

/**************************************************************************/

#ifdef __MORPHOS__
int
realMain(int argc,char **argv)
#else
int
main(int argc,char **argv)
#endif
{
    ULONG error, arg0, arg1;
    int   res = RETURN_FAIL;

    initStrings();

    if (!(error = openStuff(&arg0,&arg1)))
    {
        if (!(error = createClasses()))
        {
            Object *app;

            if (app = appObject, End)
            {
                ULONG signals;

                for (signals = 0; DoMethod(app,MUIM_Application_NewInput,(ULONG)&signals)!=MUIV_Application_ReturnID_Quit; )
                    if (signals && ((signals = Wait(signals | SIGBREAKF_CTRL_C)) & SIGBREAKF_CTRL_C)) break;

                MUI_DisposeObject(app);

                res = RETURN_OK;
            }
            else error = MSG_Err_NoApp;

            disposeClasses();
        }
    }

    if (error)
    {
        TEXT buf[256];

        msnprintf(buf,sizeof(buf),getString(error),arg0,arg1);

        if (MUIMasterBase)
        {
            Object *app = ApplicationObject,
                MUIA_Application_UseCommodities, FALSE,
                MUIA_Application_UseRexx,        FALSE,
            End;

            MUI_RequestA(app,NULL,0,getString(MSG_ErrReqTitle),getString(MSG_ErrReqGadget),buf,NULL);

            if (app) MUI_DisposeObject(app);
        }
        else Printf("%s\n",(ULONG)buf);
    }

    closeStuff();

    return res;
}

/**************************************************************************/

#ifdef __MORPHOS__
#define MIN68KSTACKSIZE 16000

int
main(int argc,char **argv)
{
    struct Task *me = FindTask(NULL);
    ULONG       size;

    if (!NewGetTaskAttrsA(me,&size,sizeof(size),TASKINFOTYPE_STACKSIZE_M68K,NULL))
    return RETURN_ERROR;

    if (size<MIN68KSTACKSIZE)
    {
        struct StackSwapStruct *sss;

        if (sss = AllocMem(sizeof(*sss)+MIN68KSTACKSIZE,MEMF_PUBLIC))
        {
            int res;

            sss->stk_Lower   = sss+1;
            sss->stk_Upper   = (ULONG)(((UBYTE *)(sss + 1))+MIN68KSTACKSIZE);
            sss->stk_Pointer = (APTR)sss->stk_Upper;
            StackSwap(sss);
            res = realMain(argc,argv);
            StackSwap(sss);
            FreeMem(sss,sizeof(*sss)+MIN68KSTACKSIZE);

            return res;
        }
    }

    return realMain(argc,argv);
}

#endif

/***********************************************************************/

