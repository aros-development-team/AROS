/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <stdio.h>

#include "openurl.h"

#define CATCOMP_NUMBERS
#include "locale.h"

#include <graphics/gfxbase.h>

#include <libraries/openurl.h>

#include "macros.h"

#include "debug.h"

/**************************************************************************/

LONG                   __stack = 32000; /* I think this is OK for every env */

struct IntuitionBase   *IntuitionBase = NULL;
struct GfxBase         *GfxBase = NULL;
struct Library         *MUIMasterBase = NULL;
struct Library         *UtilityBase = NULL;
struct Library         *IconBase = NULL;
struct Library         *OpenURLBase = NULL;
#if !defined(__MORPHOS__)
struct LocaleBase      *LocaleBase = NULL;
#else
struct Library         *LocaleBase = NULL;
#endif

#if defined(__amigaos4__)
struct IntuitionIFace  *IIntuition = NULL;
struct GraphicsIFace   *IGraphics = NULL;
struct MUIMasterIFace  *IMUIMaster = NULL;
struct UtilityIFace    *IUtility = NULL;
struct IconIFace       *IIcon = NULL;
struct OpenURLIFace    *IOpenURL = NULL;
struct LocaleIFace     *ILocale = NULL;
#endif /* __amigaos4__ */

struct MUI_CustomClass *g_appClass = NULL;
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
BOOL                   g_MUI4 = FALSE;

/**************************************************************************/

static ULONG openStuff(ULONG *arg0, ULONG *arg1)
{
    *arg1 = 0;

    if((MUIMasterBase = OpenLibrary("muimaster.library",19)) == NULL ||
       !GETINTERFACE(IMUIMaster, MUIMasterBase))
    {
        *arg0 = 19;
        return MSG_Err_NoMUI;
    }

    if(MUIMasterBase->lib_Version < 20)
      g_MUI4 = FALSE;
    else if (MUIMasterBase->lib_Version==20)
      g_MUI4 = MUIMasterBase->lib_Revision>5341;
	else
	  g_MUI4 = TRUE;

    if(!(g_pool = CreatePool(MEMF_PUBLIC|MEMF_CLEAR,8192,4196)))
      return MSG_Err_NoMem;

    *arg0 = 37;

    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37)) == NULL ||
       !GETINTERFACE(IIntuition, IntuitionBase))
       return MSG_Err_NoIntuition;

    if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",37)) == NULL ||
       !GETINTERFACE(IGraphics, GfxBase))
       return MSG_Err_NoGfx;

    if((UtilityBase = OpenLibrary("utility.library",37)) == NULL ||
       !GETINTERFACE(IUtility, UtilityBase))
       return MSG_Err_NoUtility;

    if((IconBase = OpenLibrary("icon.library",37)) == NULL ||
       !GETINTERFACE(IIcon, IconBase))
       return MSG_Err_NoIcon;

    if((OpenURLBase = OpenLibrary(OPENURLNAME,OPENURLVER)) == NULL||
       !GETINTERFACE(IOpenURL, OpenURLBase) ||
       ((OpenURLBase->lib_Version==7) && (OpenURLBase->lib_Revision<1)))
        //((OpenURLBase->lib_Version==OPENURLREV) && (OpenURLBase->lib_Revision<OPENURLREV)))
    {
        *arg0 = OPENURLVER;
        *arg1 = OPENURLREV;

        return MSG_Err_NoOpenURL;
    }

    #if defined(__amigaos4__)
    // setup the AmiUpdate variable
    SetAmiUpdateENVVariable( "OpenURL" );
    #endif /* __amigaos4__ */

    return 0;
}

/**************************************************************************/

static void closeStuff(void)
{
    if (LocaleBase)
    {
        uninitStrings();

        if(g_cat)
          CloseCatalog(g_cat);

        DROPINTERFACE(ILocale);
        CloseLibrary((struct Library *)LocaleBase);
        LocaleBase = NULL;
    }

    if(OpenURLBase)
    {
      DROPINTERFACE(IOpenURL);
      CloseLibrary(OpenURLBase);
      OpenURLBase = NULL;
    }

    if(IconBase)
    {
      DROPINTERFACE(IIcon);
      CloseLibrary(IconBase);
      IconBase = NULL;
    }

    if(UtilityBase)
    {
      DROPINTERFACE(IUtility);
      CloseLibrary(UtilityBase);
      UtilityBase = NULL;
    }

    if(GfxBase)
    {
      DROPINTERFACE(IGraphics);
      CloseLibrary((struct Library *)GfxBase);
      GfxBase = NULL;
    }

    if(IntuitionBase)
    {
      DROPINTERFACE(IIntuition);
      CloseLibrary((struct Library *)IntuitionBase);
      IntuitionBase = NULL;
    }

    if(MUIMasterBase)
    {
      DROPINTERFACE(IMUIMaster);
      CloseLibrary(MUIMasterBase);
      MUIMasterBase = NULL;
    }

    if(g_pool)
      DeletePool(g_pool);
}

/**************************************************************************/

static ULONG createClasses(void)
{
    if (initPopphClass() == FALSE)          return MSG_Err_PopphClass;
    if (initPopportClass() == FALSE)        return MSG_Err_PopupPortClass;
    if (initFTPEditWinClass() == FALSE)     return MSG_Err_NoFTPEditWinClass;
    if (initMailerEditWinClass() == FALSE)  return MSG_Err_NoMailerEditWinClass;
    if (initBrowserEditWinClass() == FALSE) return MSG_Err_NoBrowserEditWinClass;
    if (initAppListClass() == FALSE)        return MSG_Err_NoAppListClass;
    if (initWinClass() == FALSE)            return MSG_Err_NoWinClass;
    if (initAppClass() == FALSE)            return MSG_Err_NoAppClass;

    return 0;
}

/**************************************************************************/

static void disposeClasses(void)
{
    if (g_popphClass)          disposePopphClass();
    if (g_popportClass)        disposePopportClass();
    if (g_FTPEditWinClass)     disposeFTPEditWinClass();
    if (g_mailerEditWinClass)  disposeMailerEditWinClass();
    if (g_browserEditWinClass) disposeBrowserEditWinClass();
    if (g_appListClass)        disposeAppListClass();
    if (g_winClass)            disposeWinClass();
    if (g_aboutClass)          disposeAboutClass();
    if (g_appClass)            disposeAppClass();
}

/**************************************************************************/

int main(void)
{
    ULONG error, arg0=0, arg1=0;
    int   res = RETURN_FAIL;

    // setup the debugging stuff
    #if defined(DEBUG)
    SetupDebug();
    #endif

    initStrings();

    if (!(error = openStuff(&arg0,&arg1)))
    {
        if (!(error = createClasses()))
        {
            Object *app;

            if((app = appObject, End) != NULL)
            {
                ULONG signals;

                for (signals = 0; (LONG)DoMethod(app,MUIM_Application_NewInput,(IPTR)&signals) != (LONG)MUIV_Application_ReturnID_Quit; )
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

        snprintf(buf, sizeof(buf), getString(error), arg0, arg1);

        if (MUIMasterBase)
        {
            Object *app = ApplicationObject,
                MUIA_Application_UseCommodities, FALSE,
                MUIA_Application_UseRexx,        FALSE,
            End;

            MUI_RequestA(app,NULL,0,getString(MSG_ErrReqTitle),getString(MSG_ErrReqGadget),buf,NULL);

            if (app) MUI_DisposeObject(app);
        }
        else printf("%s\n", buf);
    }

    closeStuff();

    return res;
}

/**************************************************************************/
