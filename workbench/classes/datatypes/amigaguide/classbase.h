/*
** $PROJECT: class library template
**
** $VER: classbase.h 40.7 (22.08.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
** $COPYRIGHT: (C) Copyright 1996-2003 by Stefan Ruppert. All Rights Reserved!
**
** $HISTORY:
**
** 22.08.03 : 040.007 : added MorphOS support
** 08.11.98 : 040.006 : added BETA support
** 17.06.98 : 040.005 : added support for more system libraries
** 19.08.97 : 040.004 : now only USE_LIB#? defines
** 04.08.97 : 040.003 : added NO_#?LIB defines
** 09.11.96 : 040.002 : now uses ClassLibrary structure from the DevCD
** 15.09.96 : 040.001 : + removed some unnecessary includes
**                      + added Semaphore structure to ClassBase
*/

/* ------------------------- include system stuff ------------------------- */

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/icclass.h>
#include <dos/dos.h>

#include <clib/macros.h>
#include <clib/alib_protos.h>

#include <string.h>


#ifdef __MORPHOS__
#define CL_FUNCTYPE  ULONG
#else
#define CL_FUNCTYPE  APTR
#endif

/* ----------------------- compiler and class stuff ----------------------- */

#include "compiler.h"

#include "classdata.h"

/* ----------------------------- debug stuff ------------------------------ */

#include "debug.h"

/* ----------------------- needed system libraries ------------------------ */

#if defined(__GNUC__) && !defined(__AROS__)
#define __NOLIBBASE__
#endif

/* force SAS to use the SysBase pragmas instead of AbsSysBase pragmas */
#ifdef __SASC
#include <clib/exec_protos.h>
#include <pragmas/exec_sysbase_pragmas.h>
#else
#include <proto/exec.h>
#endif

#include <proto/intuition.h>
#include <proto/utility.h>

/* -------------------- include specific systems parts -------------------- */

#ifdef BETA
/* beta versions need dos.library to compare dates */
#undef USE_DOSLIB
#define USE_DOSLIB

#ifndef BETA_DATE
#error "You have to specify a date where this beta version expires using the BETA_DATE define!"
#endif
#endif

#ifdef USE_GFXLIB
#include <proto/graphics.h>
#endif
#ifdef USE_IFFPARSELIB
#include <proto/iffparse.h>
#endif
#ifdef USE_DATATYPESLIB
#include <intuition/gadgetclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <proto/datatypes.h>
#endif
#ifdef USE_DOSLIB
#include <proto/dos.h>
#endif
#ifdef USE_LOCALELIB
#include <proto/locale.h>
#endif
#ifdef USE_GADTOOLSLIB
#include <proto/gadtools.h>
#endif
#ifdef USE_DISKFONTLIB
#include <proto/diskfont.h>
#endif
#ifdef USE_LAYERSLIB
#include <proto/layers.h>
#endif
#ifdef USE_ICONLIB
#include <proto/icon.h>
#endif
#ifdef USE_ASLLIB
#include <proto/asl.h>
#endif
#ifdef USE_REXXSYSLIB
#include <proto/rexxsyslib.h>
#endif
#ifdef USE_WBLIB
#include <proto/wb.h>
#endif

/* ------------------------------ structures ------------------------------ */

#ifndef USERCLASSBASEDATA
#define USERCLASSBASEDATA
#endif

#ifndef USERCLASSBASEFUNCTABLE
#define USERCLASSBASEFUNCTABLE
#endif

struct ClassBase
{
   struct ClassLibrary cb_Lib;

   struct ExecBase *cb_SysBase;
   struct Library *cb_IntuitionBase;
   struct Library *cb_UtilityBase;

#ifndef __AROS__
#ifdef USE_DOSLIB
#define DOSBase                 cb->cb_DOSBase
   struct Library *cb_DOSBase;
#endif

#ifdef USE_GFXLIB
   struct GfxBase *cb_GfxBase;
#define GfxBase                 cb->cb_GfxBase
#endif

#ifdef USE_DATATYPESLIB
#define DataTypesBase           cb->cb_DataTypesBase
   struct Library *cb_DataTypesBase;
#endif

#ifdef USE_IFFPARSELIB
#define IFFParseBase            cb->cb_IFFParseBase
   struct Library *cb_IFFParseBase;
#endif

#ifdef USE_LOCALELIB
#define LocaleBase              cb->cb_LocaleBase
   struct Library *cb_LocaleBase;
#endif

#ifdef USE_GADTOOLSLIB
#define GadToolsBase            cb->cb_GadToolsBase
   struct Library *cb_GadToolsBase;
#endif

#ifdef USE_DISKFONTLIB
#define DiskFontBase            cb->cb_DiskFontBase
#define DiskfontBase            cb->cb_DiskFontBase
   struct Library *cb_DiskFontBase;
#endif

#ifdef USE_LAYERSLIB
#define LayersBase              cb->cb_LayersBase
   struct Library *cb_LayersBase;
#endif

#ifdef USE_ICONLIB
#define IconBase                cb->cb_IconBase
   struct Library *cb_IconBase;
#endif

#ifdef USE_ASLLIB
#define AslBase                 cb->cb_AslBase
   struct Library *cb_AslBase;
#endif

#ifdef USE_REXXSYSLIB
#define RexxSysBase             cb->cb_RexxSysBase
   struct Library *cb_RexxSysBase;
#endif

#ifdef USE_WBLIB
#define WorkbenchBase           cb->cb_WorkbenchBase
   struct Library *cb_WorkbenchBase;
#endif

#ifdef SUPERCLASSLIBRARY
#define SuperClassBase          cb->cb_SuperClassBase
   struct Library *cb_SuperClassBase;
#endif
#endif /* !__AROS__ */
    
   BPTR cb_SegList;

   struct SignalSemaphore cb_Lock;

   USERCLASSBASEDATA
};

#ifndef __AROS__
#define SysBase                 ((struct ExecBase *)cb->cb_SysBase)
#define UtilityBase             cb->cb_UtilityBase
#define IntuitionBase           cb->cb_IntuitionBase
#endif

#define CLASSBASE               struct ClassBase *cb = (struct ClassBase *) cl->cl_UserData

