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

#ifndef _LIB_H
#define _LIB_H

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/rexxsyslib.h>

#include <clib/alib_protos.h>

#if !defined(__AROS__)
#include <clib/debug_protos.h>
#endif

#include <libraries/openurl.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "base.h"
#include <macros.h>

/**************************************************************************/

#define DEF_ENV             "ENV:OpenURL.prefs"
#define DEF_ENVARC          "ENVARC:OpenURL.prefs"

#define DEF_FLAGS           (UPF_ISDEFAULTS|UPF_PREPENDHTTP|UPF_DOMAILTO)

#define DEF_DefShow         TRUE
#define DEF_DefBringToFront TRUE
#define DEF_DefNewWindow    FALSE
#define DEF_DefLaunch       TRUE

// ABA, TO BE ABLE TO COMPILE, DON'T KNOW WHY, SEEMS SOMETHING MESSED IN OPENURL.H
//#define URL_GetPrefs_Default URL_GetPrefs_Mode

/**************************************************************************/

struct startMsg
{
    struct Message link;
    STRPTR port;
    STRPTR cmd;
    BOOL res;
};

/**************************************************************************/

enum
{
    LOADPREFS_ENV,
    LOADPREFS_ENVARC,

    LOADPREFS_LAST
};

/**************************************************************************/

#include "lib_protos.h"

/**************************************************************************/

#endif /* _LIB_H */
