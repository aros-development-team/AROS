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

/***************************************************************************/

#if defined(__amigaos4__)
extern struct Library *SysBase;
#else
extern struct ExecBase *SysBase;
#endif

extern struct LibraryHeader *OpenURLBase;

/***************************************************************************/

enum
{
  BASEFLG_Init  = 1<<0,
  BASEFLG_Trans = 1<<1,
};

/***************************************************************************/
