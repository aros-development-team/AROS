/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Init of workbench.library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include "workbench_intern.h"
#include "libdefs.h"

#ifdef SysBase
#   undef SysBase
#endif
#ifdef ExecBase
#   undef ExecBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)	(((struct LIBBASETYPEPTR)(lib))->wb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct LIBBASETYPEPTR)(lib))->wb_SegList)
#define LC_RESIDENTNAME		Workbench_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		-120
#define LC_LIBBASESIZE		sizeof(struct LIBBASETYPE)
#define LC_LIBHEADERTYPEPTR	struct LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct LIBBASETYPEPTR)(lib))->LibNode)

#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

struct ExecBase   * SysBase; /* global variable */

ULONG SAVEDS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    SysBase = lh->wb_SysBase;

    if (lh->wb_AppReplyPort = CreateMsgPort())
    {
	return TRUE;
    }

    return FALSE;
} /* L_InitLib */



void SAVEDS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
    if (lh->wb_AppReplyPort)
	DeleteMsgPort(lh->wb_AppReplyPort);
}

