/*
   (C) 2001 AROS - The Amiga Research OS
   $Id$

   Desc: Partition initialization code
   Lang: English
*/

#define AROS_ALMOST_COMPATIBLE

#define DEBUG 1
#include <aros/debug.h>

#include "partition_intern.h"
#include "partition_support.h"
#include "libdefs.h"

#undef SysBase

/* customize libheader.c */
#define LC_RESIDENTPRI 104
#define LC_LIBBASESIZE sizeof(LIBBASETYPE)
#define LC_NO_OPENLIB
#define LC_NO_CLOSELIB

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

