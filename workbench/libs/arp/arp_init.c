/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Init of mathtrans.library
    Lang: english
*/
#include <utility/utility.h> /* this must be before mathtrans_intern.h */
#include <proto/dos.h>
#include <proto/arp.h>

#define NO_LIB_DEFINES 1
#include "arp_intern.h"
#undef NO_LIB_DEFINES
#include "libdefs.h"

#include <proto/arp.h>

#define LC_RESIDENTPRI	    -120

#include <libcore/libheader.c>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

struct ArpBase ArpBase;   /* global variable */

#define SysBase 	ArpBase.ExecBase

ULONG SAVEDS L_InitLib (LC_LIBHEADERTYPEPTR lh)
{
    ArpBase.ExecBase = (struct Library *)lh->lh_SysBase;

    if (!ArpBase.GfxBase)
	ArpBase.GfxBase = (struct Library *)OpenLibrary ("graphics.library", 39);

    if (!ArpBase.GfxBase)
	return FALSE;

    if (!ArpBase.DosBase)
	ArpBase.DosBase = (struct Library *)OpenLibrary ("dos.library", 39);

    if (!ArpBase.DosBase)
	return FALSE;

    if (!ArpBase.IntuiBase)
	ArpBase.IntuiBase = (struct Library *)OpenLibrary ("intuition.library", 39);

    if (!ArpBase.IntuiBase)
	return FALSE;

    if (!ArpBase.AslBase)
	ArpBase.AslBase = (struct Library *)OpenLibrary ("asl.library", 39);

    if (!ArpBase.AslBase)
	return FALSE;

    if (!ArpBase.UtilityBase)
	ArpBase.UtilityBase = (struct Library *)OpenLibrary ("utility.library", 39);

    if (!ArpBase.UtilityBase)
	return FALSE;

    /* Do some initialization on my Base structure */
    ArpBase.ESCChar = 0x21;
    ArpBase.ResLists.mlh_TailPred = (struct MinNode *)&ArpBase.ResLists.mlh_Head;
    ArpBase.ResLists.mlh_Head     = (struct MinNode *)&ArpBase.ResLists.mlh_Tail;


    return TRUE;
} /* L_InitLib */

ULONG SAVEDS L_OpenLib (LC_LIBHEADERTYPEPTR lh)
{
    return TRUE;
} /* L_OpenLib */

void SAVEDS L_CloseLib(LC_LIBHEADERTYPEPTR lh)
{
  /* Free everything this task has allocated via arp */
#warning FIXME!
//while (TRUE == FreeTaskResList());
} /* L_CloseLib */

void SAVEDS L_ExpungeLib (LC_LIBHEADERTYPEPTR lh)
{
    if (ArpBase.AslBase)
	CloseLibrary ((struct Library *)ArpBase.AslBase);

    if (ArpBase.DosBase)
	CloseLibrary ((struct Library *)ArpBase.DosBase);

    if (ArpBase.GfxBase)
	CloseLibrary ((struct Library *)ArpBase.GfxBase);

    if (ArpBase.IntuiBase)
	CloseLibrary ((struct Library *)ArpBase.IntuiBase);

    if (ArpBase.UtilityBase)
	CloseLibrary ((struct Library *)ArpBase.UtilityBase);

} /* L_ExpungeLib */
