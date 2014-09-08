/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef BASE_H
#define BASE_H 1

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

/***************************************************************************/

struct LibraryHeader
{
  struct Library          libBase;
  struct Library          *sysBase;
  BPTR                    segList;
  struct SignalSemaphore  libSem;
  APTR                    pool;
  struct SignalSemaphore  poolSem;
  struct URL_Prefs        *prefs;
  struct SignalSemaphore  prefsSem;
  ULONG                   flags;
  ULONG                   rexx_use;
};

#define __NOLIBBASE__
#include <proto/openurl.h>

/***************************************************************************/

#if defined(__amigaos4__)
extern struct Library *SysBase;
#else
extern struct ExecBase *SysBase;
#endif

extern struct LibraryHeader *OpenURLBase;

#if defined(__amigaos4__)
#define __BASE_OR_IFACE_TYPE	struct OpenURLIFace *
#define __BASE_OR_IFACE_VAR		IOpenURL
#else
#define __BASE_OR_IFACE_TYPE	struct LibraryHeader *
#define __BASE_OR_IFACE_VAR		OpenURLBase
#endif
#define __BASE_OR_IFACE			__BASE_OR_IFACE_TYPE __BASE_OR_IFACE_VAR

/***************************************************************************/

enum
{
  BASEFLG_Init  = 1<<0,
  BASEFLG_Trans = 1<<1,
};

/***************************************************************************/

#endif /* BASE_H */
