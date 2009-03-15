/*
    Copyright © 2002-2009, The AROS Development Team. All rights reserved.
    $Id$
*/
#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>
#endif

#include <libraries/mui.h>


#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
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
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

#ifndef __AROS__

#ifdef __MORPHOS__
struct Library          *LocaleBase = NULL;
struct Library          *UtilityBase = NULL;
#elif __amigaos4__
struct Library          *LocaleBase=NULL;
struct UtilityBase      *UtilityBase;
#else
struct LocaleBase       *LocaleBase = NULL;
struct UtilityBase      *UtilityBase = NULL;
#endif

struct GfxBase          *GfxBase = NULL;
struct IntuitionBase    *IntuitionBase = NULL;
struct Library          *IFFParseBase = NULL;
struct LayersBase       *LayersBase = NULL;
struct Library          *WorkbenchBase = NULL;
struct IconBase         *IconBase = NULL;
struct DataTypesBase    *DataTypesBase = NULL;
struct DiskfontBase     *DiskfontBase = NULL;

struct Library          *MUIMasterBase = NULL;
struct Library          *CyberGfxBase = NULL;


#ifdef __amigaos4__
struct UtilityIFace     *IUtility;
struct GraphicsIFace    *IGraphics;
struct LayersIFace      *ILayers;
struct DataTypesIFace   *IDataTypes;
struct IconIFace        *IIcon;
struct WorkbenchIFace   *IWorkbench;
struct DiskfontIFace    *IDiskfont;
struct IntuitionIFace   *IIntuition;
struct IFFParseIFace    *IIFFParse;
struct LocaleIFace      *ILocale;
struct MUIMasterIFace   *IMUIMaster;
struct CyberGfxIFace    *ICyberGfx;
#endif
#endif

/* global variables */
Object         *_WandererIntern_AppObj = NULL;
Class          *_WandererIntern_CLASS = NULL;

/* Don't output errors to the console, open requesters instead */
int                  __forceerrorrequester = 1;


#ifndef __AROS__
//struct MUI_CustomClass  *IconWindow_Class = NULL;

struct MUI_CustomClass  *IconList_Class = NULL;
struct MUI_CustomClass  *IconDrawerList_Class = NULL;
struct MUI_CustomClass  *IconListview_Class = NULL;
struct MUI_CustomClass  *IconVolumeList_Class = NULL;

///deInitLibs()
void deInitLibs(void)
{
    if (MUIMasterBase)
    {

/*
    if (IconWindowIconNetworkBrowserList_CLASS)
      IconWindowIconNetworkBrowserList_Deinitialize();
D(bug("2\n"));*/
        if (WandererPrefs_CLASS)
          WandererPrefs_Deinitialize();

        if (Wanderer_CLASS)
          Wanderer_Deinitialize();

        if (IconWindowIconVolumeList_CLASS)
          IconWindowIconVolumeList_Deinitialize();

        if (IconWindowIconDrawerList_CLASS)
          IconWindowIconDrawerList_Deinitialize();


        if (IconVolumeList_Class)
          MUI_DeleteCustomClass(IconVolumeList_Class);

        if (IconListview_Class)
          MUI_DeleteCustomClass(IconListview_Class);

        if (IconDrawerList_Class)
          MUI_DeleteCustomClass(IconDrawerList_Class);

        if (IconList_Class)
          MUI_DeleteCustomClass(IconList_Class);

        if (IconWindow_CLASS)
          IconWindow_Deinitialize();


        #ifdef __amigaos4__
        if (IMUIMaster)
        DropInterface((struct Interface *)IMUIMaster);
        #endif

        CloseLibrary(MUIMasterBase);
    }

    if (LocaleBase)
    {
        Locale_Deinitialize();
        #ifdef __amigaos4__
        if (ILocale)
            DropInterface((struct Interface *)ILocale);
        #endif
        CloseLibrary((struct Library *)LocaleBase);
    }

    #ifdef __amigaos4__
    if (IIFFParse)
        DropInterface((struct Interface *)IIFFParse);

    if (ICyberGfx)
        DropInterface((struct Interface *)ICyberGfx);

    if (IWorkbench)
        DropInterface((struct Interface *)IWorkbench);

    if (IIcon)
        DropInterface((struct Interface *)IIcon);


    if (IDataTypes)
        DropInterface((struct Interface *)IDataTypes);

    if (IDiskfont)
        DropInterface((struct Interface *)IDiskfont);

    if (IIntuition)
        DropInterface((struct Interface *)IIntuition);

    if (IUtility)
        DropInterface((struct Interface *)IUtility);

    if (ILayers)
        DropInterface((struct Interface *)ILayers);

    if (IGraphics)
        DropInterface((struct Interface *)IGraphics);
    #endif

    if (IFFParseBase)     CloseLibrary(IFFParseBase);

    

    if (WorkbenchBase)   CloseLibrary((struct Library *)WorkbenchBase);

    if (IconBase)   CloseLibrary((struct Library *)IconBase);

    if (DiskfontBase)   CloseLibrary((struct Library *)DiskfontBase);

    if (DataTypesBase)   CloseLibrary((struct Library *)DataTypesBase);

    if (CyberGfxBase) CloseLibrary(CyberGfxBase);

    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

    if (UtilityBase)   CloseLibrary((struct Library *)UtilityBase);

    if (LayersBase)   CloseLibrary((struct Library *)LayersBase);

    if (GfxBase) CloseLibrary((struct Library *)GfxBase);

}
///

///initLibs()
int initLibs(void)
{

    if (!(GfxBase =(struct GfxBase *) OpenLibrary("graphics.library",40))
    #ifdef __amigaos4__
        || !(IGraphics = (struct GraphicsIFace *) GetInterface((struct Library *)GfxBase,
                                                                 "main",
                                                                 1,
                                                                 NULL))
            #endif
             )
    return 1;

    if(!(LayersBase =(struct LayersBase *) OpenLibrary((CONST_STRPTR)"layers.library",40))
    #ifdef __amigaos4__
        || !(ILayers = (struct LayersIFace *) GetInterface((struct Library *)LayersBase,
                                                            "main",
                                                            1,
                                                            NULL))
    #endif
       )
        return 1;


    
    #ifdef __MORPHOS__
    if (!(UtilityBase = (struct Library *)OpenLibrary("utility.library",40))
    #else
    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library",40))
    #endif
    #ifdef __amigaos4__
    || !(IUtility = (struct UtilityIFace *) GetInterface((struct Library *)UtilityBase,
                                                         "main",
                                                         1,
                                                         NULL))
    #endif
     )
    return 1;



    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library",40))
        #ifdef __amigaos4__
            || !(IIntuition = (struct IntuitionIFace *) GetInterface((struct Library *)IntuitionBase,
                                                                     "main",
                                                                     1,
                                                                     NULL))
        #endif
         )
        return 1;


    if (!(MUIMasterBase = OpenLibrary((CONST_STRPTR)"muimaster.library",19))
    #ifdef __amigaos4__
      || !(IMUIMaster = (struct MUIMasterIFace *) GetInterface((struct Library *)MUIMasterBase,
                                                               "main",
                                                               1,
                                                               NULL))
    #endif
     )
    return 1;


    #ifdef __MORPHOS__
    if (!(LocaleBase = OpenLibrary((CONST_STRPTR)"locale.library",39))
    #elif __amigaos4__
    if (!(LocaleBase =(struct Library *) OpenLibrary("locale.library",39))
    #else
    if (!(LocaleBase =(struct LocaleBase *) OpenLibrary("locale.library",39))
    #endif
    #ifdef __amigaos4__
        || !(ILocale = (struct LocaleIFace *) GetInterface((struct Library *)LocaleBase,
                                                             "main",
                                                             1,
                                                             NULL))
    #endif
         )
         return 1;



  Locale_Initialize();

   if (!(IFFParseBase = OpenLibrary((CONST_STRPTR)"iffparse.library",40))
            #ifdef __amigaos4__
                || !(IIFFParse = (struct IFFParseIFace *) GetInterface((struct Library *)IFFParseBase,
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

    if(!(WorkbenchBase = OpenLibrary("workbench.library",40))
  #ifdef __amigaos4__
  || !(IWorkbench = (struct WorkbenchIFace *) GetInterface((struct Library *)WorkbenchBase,
                                                    "main",
                                                    1,
                                                    NULL))
  #endif
  )
    return 1;

    if(!(DiskfontBase = (struct DiskfontBase *)OpenLibrary((CONST_STRPTR)"diskfont.library",40))
  #ifdef __amigaos4__
  || !(IDiskfont = (struct DiskfontIFace *) GetInterface((struct Library *)DiskfontBase,
                                                    "main",
                                                    1,
                                                    NULL))
  #endif
  )
    return 1;

  if(!(IconBase = (struct IconBase *)OpenLibrary((CONST_STRPTR)"icon.library",41))
  #ifdef __amigaos4__
  || !(IIcon = (struct IconIFace *) GetInterface((struct Library *)IconBase,
                                                    "main",
                                                    1,
                                                    NULL))
  #endif
  )
    return 1;

  if(!(DataTypesBase = (struct DataTypesBase *)OpenLibrary((CONST_STRPTR)"datatypes.library",40))
  #ifdef __amigaos4__
  || !(IDataTypes = (struct DataTypesIFace *) GetInterface((struct Library *)DataTypesBase,
                                                    "main",
                                                    1,
                                                    NULL))
  #endif
  )
    return 1;

  

if (!initIconWindowClass())
    return 1;

//  if (!(IconWindow_Class = (struct MUI_CustomClass *) initIconWindowClass()))
//    return 1;

  if (!(IconList_Class = (struct MUI_CustomClass *) initIconListClass()))
    return 1;

  if (!(IconDrawerList_Class = (struct MUI_CustomClass *) initIconDrawerListClass()))
    return 1;

  if (!(IconListview_Class = (struct MUI_CustomClass *) initIconListviewClass()))
    return 1;

  if (!(IconVolumeList_Class = (struct MUI_CustomClass *) initIconVolumeListClass()))
    return 1;


  IconWindowIconDrawerList_Initialize();

  IconWindowIconVolumeList_Initialize();

//  IconWindowIconNetworkBrowserList_Initialize();


  Wanderer_Initialize();

  WandererPrefs_Initialize();

  return 0;

}
///
#endif

///main()
int main(void)
{
    LONG             retval = RETURN_ERROR;

#ifndef __AROS__
    if (initLibs())
        return retval;
#endif

D(bug("[Wanderer.EXE] Wanderer Initialising .. \n"));

    OpenWorkbenchObject("Wanderer:Tools/ExecuteStartup", 0, 0);
    if ((_WandererIntern_AppObj = NewObject(Wanderer_CLASS->mcc_Class, NULL, TAG_DONE)) != NULL)
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
///
