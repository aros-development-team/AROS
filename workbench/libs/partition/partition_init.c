/*
   (C) 2001 AROS - The Amiga Research OS
   $Id$

   Desc: Partition initialization code
   Lang: English
*/


#define DEBUG 1
#include <aros/debug.h>

#include "partition_intern.h"
#include "partition_support.h"
#include LC_LIBDEFS_FILE

#undef SysBase

/* customize libheader.c */
#define LC_RESIDENTPRI 104
#define LC_LIBBASESIZE sizeof(LIBBASETYPE)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB
#define LC_RESIDENTFLAGS (RTF_AUTOINIT | RTF_COLDSTART)

#include <libcore/libheader.c>

#define PartitionBase ((LIBBASETYPEPTR) lh)

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh)
{
    PartitionBase->tables = (struct PartitionTableInfo **)PartitionSupport;
   return TRUE;
}

void SAVEDS STDARGS LC_BUILDNAME(L_ExpungeLib) (LC_LIBHEADERTYPEPTR lh)
{
}

