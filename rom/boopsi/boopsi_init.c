/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: BOOPSI Library
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <utility/utility.h>
#include <proto/boopsi.h>
#include "intern.h"
#include "libdefs.h"

/**********************************************************************************************/

#ifdef SysBase
#   undef SysBase
#endif

/* Customize libheader.c */
#define LC_SYSBASE_FIELD(lib)   (((struct IntBOOPSIBase *)(lib))->bb_SysBase)
#define LC_SEGLIST_FIELD(lib)   (((struct IntBOOPSIBase *)(lib))->bb_SegList)
#define LC_RESIDENTNAME 	BOOPSI_resident
#define LC_RESIDENTFLAGS	RTF_AUTOINIT|RTF_COLDSTART
#define LC_RESIDENTPRI		95
#define LC_LIBBASESIZE		sizeof(struct IntBOOPSIBase)
#define LC_LIBHEADERTYPEPTR	LIBBASETYPEPTR
#define LC_LIB_FIELD(lib)       (((struct IntBOOPSIBase *)(lib))->bb_LibNode)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_NO_EXPUNGELIB
#define LC_STATIC_INITLIB

#include <libcore/libheader.c>

/**********************************************************************************************/

#define SysBase (GetBBase(BOOPSIBase)->bb_SysBase)

/**********************************************************************************************/

static void FreeAllClasses(struct Library *BOOPSIBase)
{
    Class *cl;

    while((cl = (Class *)RemHead((struct List *)&GetBBase(BOOPSIBase)->bb_ClassList)))
    {
	/* We can't free this class, so lets not bother */
	if(cl != &rootclass)
	    FreeClass(cl);
    }
}

/**********************************************************************************************/

static ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LIBBASETYPEPTR LIBBASE)
{
    Class *cl;

    UtilityBase = OpenLibrary (UTILITYNAME, 0);

    if (!UtilityBase)
	return FALSE;

    /* All we have to do is to set up the pre-existing classes. */
    NEWLIST(&GetBBase(LIBBASE)->bb_ClassList);
    InitSemaphore(&GetBBase(LIBBASE)->bb_ClassListLock);

    rootclass.cl_UserData = (IPTR)LIBBASE;
    AddClass(&rootclass);

    if((cl = InitICClass(LIBBASE)) == 0)
    {
	CloseLibrary (UtilityBase);
	return FALSE;
    }

    if((cl = InitModelClass(LIBBASE)) == 0)
    {
	FreeAllClasses(LIBBASE);
	CloseLibrary (UtilityBase);
	return FALSE;
    }

    return TRUE;
}

/**********************************************************************************************/
