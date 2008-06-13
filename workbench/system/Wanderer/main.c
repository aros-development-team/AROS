/*
    Copyright � 2002-2008, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef __AROS__
#include "portable_macros.h"
#else
#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>
#endif

#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#ifdef __AROS__
#include <proto/workbench.h>
#endif
#include <dos/dos.h>

#include "locale.h"
#include "wanderer.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #define bug DebugPrintF
#else
  #define  D(...)
#endif
#endif

#ifndef __AROS__
struct Library          *MUIMasterBase = NULL;
struct Library          *CyberGfxBase = NULL;
#ifdef __amigaos4__
struct MUIMasterIFace   *IMUIMaster;
struct CyberGfxIFace    *ICyberGfx;
#endif
#endif

/* global variables */
Object 				 *_WandererIntern_AppObj = NULL;
Class 				 *_WandererIntern_CLASS = NULL;

/* Don't output errors to the console, open requesters instead */
int                  __forceerrorrequester = 1;


#ifndef __AROS__
struct MUI_CustomClass  *IconList_Class = NULL;
struct MUI_CustomClass  *IconDrawerList_Class = NULL;
struct MUI_CustomClass  *IconListview_Class = NULL;
struct MUI_CustomClass  *IconVolumeList_Class = NULL;

void deInitLibs(void)
{
  if (MUIMasterBase)
  {


    if (IconVolumeList_Class)
      MUI_DeleteCustomClass(IconVolumeList_Class);

    if (IconListview_Class)
      MUI_DeleteCustomClass(IconListview_Class);

    if (IconDrawerList_Class)
      MUI_DeleteCustomClass(IconDrawerList_Class);

    if (IconList_Class)
      MUI_DeleteCustomClass(IconList_Class);


D(bug("1\n"));
/*
    if (IconWindowIconNetworkBrowserList_CLASS)
      IconWindowIconNetworkBrowserList_Deinitialize();
D(bug("2\n"));*/
    if (IconWindowIconVolumeList_CLASS)
      IconWindowIconVolumeList_Deinitialize();

D(bug("3\n"));
    if (IconWindowIconDrawerList_CLASS)
      IconWindowIconDrawerList_Deinitialize();
D(bug("4\n"));

    if (WandererPrefs_CLASS)
      WandererPrefs_Deinitialize();
D(bug("5\n"));
    if (Wanderer_CLASS)
      Wanderer_Deinitialize();
D(bug("6\n"));
    if (IconWindow_CLASS)
      IconWindow_Deinitialize();

    #ifdef __amigaos4__
    if (IMUIMaster)
      DropInterface((struct Interface *)IMUIMaster);
    #endif

    CloseLibrary(MUIMasterBase);
  }

  Locale_Deinitialize();
   #ifdef __amigaos4__
   if (ICyberGfx)
    DropInterface((struct Interface *)ICyberGfx);
   #endif

   if (CyberGfxBase) CloseLibrary(CyberGfxBase);

}

int initLibs(void)
{

  if (!(MUIMasterBase = OpenLibrary((CONST_STRPTR)"muimaster.library",19))
    #ifdef __amigaos4__
      || !(IMUIMaster = (struct MUIMasterIFace *) GetInterface((struct Library *)MUIMasterBase,
                                                               "main",
                                                               1,
                                                               NULL))
    #endif
     )
    return 1;

  if(!(CyberGfxBase = OpenLibrary("cybergraphics.library",41))
  #ifdef __amigaos4__
  || !(ICyberGfx = (struct CyberGfxIFace *) GetInterface((struct Library *)CyberGfxBase,
                                                    "main",
                                                    1,
                                                    NULL))
  #endif
  )
    return 1;

  Locale_Initialize();

  if (!initIconWindowClass())
    return 1;

  if (!(IconList_Class = initIconListClass()))
    return 1;

  if (!(IconDrawerList_Class = initIconDrawerListClass()))
    return 1;

  if (!(IconListview_Class = initIconListviewClass()))
    return 1;

  if (!(IconVolumeList_Class = initIconVolumeListClass()))
    return 1;


  IconWindowIconDrawerList_Initialize();

  IconWindowIconVolumeList_Initialize();

//  IconWindowIconNetworkBrowserList_Initialize();


  Wanderer_Initialize();

  WandererPrefs_Initialize();

  return 0;

}
#endif


int main(void)
{
    LONG             retval = RETURN_ERROR;

    #ifndef __AROS__
    if (initLibs())
      return retval;
    #endif

D(bug("[Wanderer.EXE] Wanderer Initialising .. \n"));

    OpenWorkbenchObject("Wanderer:Tools/ExecuteStartup", 0, 0);
	
    if ((_WandererIntern_AppObj = WandererObject, End) != NULL)
    {
D(bug("[Wanderer.EXE] Handing control over to Zune .. \n"));
		retval = DoMethod(_WandererIntern_AppObj, MUIM_Application_Execute);
D(bug("[Wanderer.EXE] Returned from Zune's control .. \n"));
		MUI_DisposeObject(_WandererIntern_AppObj);
    }

D(bug("[Wanderer.EXE] Exiting .. \n"));

    #ifndef __AROS__
    deInitLibs();
    #endif

    return retval;
}
