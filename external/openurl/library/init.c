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

#include "lib.h"

#include "debug.h"

#if defined(__amigaos4__)
struct Library *DOSBase = NULL;
struct Library *UtilityBase = NULL;
struct Library *RexxSysBase = NULL;
struct Library *IFFParseBase = NULL;

struct DOSIFace*      IDOS = NULL;
struct UtilityIFace*  IUtility = NULL;
struct RexxSysIFace*  IRexxSys = NULL;
struct IFFParseIFace* IIFFParse = NULL;

#if !defined(__NEWLIB__)
extern struct Library *__UtilityBase;
extern struct UtilityIFace*  __IUtility;
#endif

#else
struct DosLibrary *DOSBase = NULL;
#if defined(__AROS__)
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
struct Library *__UtilityBase = NULL; // required by clib2 & libnix
#endif
#if defined(__MORPHOS__)
struct Library *RexxSysBase = NULL;
#else
struct RxsLib *RexxSysBase = NULL;
#endif
struct Library *IFFParseBase = NULL;
#endif

/***********************************************************************/

ULONG
freeBase(struct LibraryHeader *lib)
{
  ENTER();

  D(DBF_STARTUP, "freeing all resources of openurl.library");

  if(lib->prefs != NULL)
  {
    URL_FreePrefsA(lib->prefs,NULL);
    lib->prefs = NULL;
  }

  if(RexxSysBase)
  {
    DROPINTERFACE(IRexxSys);
    CloseLibrary((struct Library *)RexxSysBase);
    RexxSysBase = NULL;
  }

  if(IFFParseBase)
  {
    DROPINTERFACE(IIFFParse);
    CloseLibrary(IFFParseBase);
    IFFParseBase = NULL;
  }

  // delete our private memory pool
  if(lib->pool != NULL)
  {
    #if defined(__amigaos4__)
    FreeSysObject(ASOT_MEMPOOL, lib->pool);
    #else
    DeletePool(lib->pool);
    #endif
    lib->pool = NULL;
  }

  if(UtilityBase)
  {
    DROPINTERFACE(IUtility);
    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
  }

  if(DOSBase)
  {
    DROPINTERFACE(IDOS);
    CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
  }

  CLEAR_FLAG(lib->flags, BASEFLG_Init);

  RETURN(TRUE);
  return TRUE;
}

/***********************************************************************/

ULONG
initBase(struct LibraryHeader *lib)
{
  ENTER();

  if((DOSBase = (APTR)OpenLibrary("dos.library", 37)) &&
     GETINTERFACE(IDOS, DOSBase))
  {
    if((UtilityBase = (APTR)OpenLibrary("utility.library", 37)) &&
       GETINTERFACE(IUtility, UtilityBase))
    {
      // we have to please the internal utilitybase
      // pointers of libnix and clib2
      #if !defined(__NEWLIB__) && !defined(__AROS__)
        __UtilityBase = (APTR)UtilityBase;
        #if defined(__amigaos4__)
        __IUtility = IUtility;
        #endif
      #endif

      // setup the debugging stuff
      #if defined(DEBUG)
      SetupDebug();
      #endif

      if((IFFParseBase = OpenLibrary("iffparse.library", 37)) &&
         GETINTERFACE(IIFFParse, IFFParseBase))
      if((RexxSysBase = (APTR)OpenLibrary("rexxsyslib.library", 36)) &&
         GETINTERFACE(IRexxSys, RexxSysBase))
      {
        #if defined(__amigaos4__)
        lib->pool = AllocSysObjectTags(ASOT_MEMPOOL, ASOPOOL_MFlags,    MEMF_SHARED|MEMF_CLEAR,
                                                     ASOPOOL_Puddle,    4096,
                                                     ASOPOOL_Threshold, 512,
                                                     ASOPOOL_Name,      "openurl.library pool",
                                                     ASOPOOL_LockMem,   FALSE,
                                                     TAG_DONE);
        #else
        lib->pool = CreatePool(MEMF_CLEAR, 4096, 512);
        #endif
        if(lib->pool != NULL)
        {
          lib->prefs = loadPrefsNotFail();

          if(lib->prefs != NULL)
          {
            SET_FLAG(lib->flags, BASEFLG_Init);

            RETURN(TRUE);
            return TRUE;
          }
        }
      }
    }
  }

  freeBase(lib);

  RETURN(FALSE);
  return FALSE;
}

/***********************************************************************/
