/*
**  openurl.library - universal URL display and browser
**  launcher library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
*/


#define __NOLIBBASE__
#define __USE_SYSBASE

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/rexxsyslib.h>

#include <clib/alib_protos.h>
#include <clib/debug_protos.h>

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
#define URL_GetPrefs_Default URL_GetPrefs_Mode

/**************************************************************************/

struct startMsg
{
    struct Message link;
    UBYTE          *port;
    UBYTE          *cmd;
    ULONG          res;
    ULONG          flags;
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

