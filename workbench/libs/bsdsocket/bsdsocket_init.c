/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BSDSocket initialization code.
    Lang: English
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/libcall.h>

#include <proto/exec.h>

#include "bsdsocket_intern.h"
#include LC_LIBDEFS_FILE

/****************************************************************************************/

#undef SysBase

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (BSDSB(lib)->sb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (BSDSB(lib)->sb_SegList)
#define LC_LIBBASESIZE          sizeof(struct SocketBase)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (BSDSB(lib)->library)

/* #define LC_NO_INITLIB    */
/* #define LC_NO_OPENLIB    */
/* #define LC_NO_CLOSELIB   */
/* #define LC_NO_EXPUNGELIB */

#include <libcore/libheader.c>

#undef DEBUG
#define DEBUG 1
#include <aros/debug.h>

#define SysBase                 (LC_SYSBASE_FIELD(SocketBase))

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR SocketBase)
{
    D(bug("Inside Init func of bsdsocket.library\n"));

    if (!UtilityBase)
        (struct Library *)UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase)
        return FALSE;

    return TRUE;
}

/****************************************************************************************/

ULONG SAVEDS STDARGS LC_BUILDNAME(L_OpenLib) (LC_LIBHEADERTYPEPTR SocketBase)
{
    D(bug("Inside Open func of bsdsocket.library\n"));

    return TRUE;
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_CloseLib) (LC_LIBHEADERTYPEPTR SocketBase)
{
    D(bug("Inside Close func of bsdsocket.library\n"));
}

/****************************************************************************************/

void  SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR SocketBase)
{
    D(bug("Inside Expunge func of bsdsocket.library\n"));

    /* CloseLibrary() checks for NULL-pointers */

    CloseLibrary((struct Library *)UtilityBase);
    UtilityBase = NULL;
}

/****************************************************************************************/
