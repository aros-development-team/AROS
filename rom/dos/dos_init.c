/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:49  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <aros/libcall.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <clib/dos_protos.h>
#include <utility/tagitem.h>
#include "dos_intern.h"

static const char name[];
static const char version[];
static const APTR Dos_inittabl[4];
static void *const Dos_functable[];
struct DosLibrary *Dos_init();
extern const char Dos_end;

int Dos_entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

/* CreateNewProc needs a global DosBase variable */
struct DosLibrary *DOSBase;

const struct Resident Dos_resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&Dos_resident,
    (APTR)&Dos_end,
    RTF_AUTOINIT,
    39,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)Dos_inittabl
};

static const char name[]="dos.library";

static const char version[]="$VER: dos 39.0 (28.3.96)\n\015";

static const APTR Dos_inittabl[4]=
{
    (APTR)sizeof(struct DosLibrary),
    (APTR)Dos_functable,
    NULL,
    &Dos_init
};

void LDDemon();
void Dos_OpenLibrary();
void Dos_OpenDevice();
void Dos_CloseLibrary();
void Dos_CloseDevice();
void Dos_RemLibrary();
void LDFlush();

__AROS_LH2(struct DosLibrary *, init,
 __AROS_LA(struct DosLibrary *, dosBase, D0),
 __AROS_LA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, Dos)
{
    __AROS_FUNC_INIT
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    DOSBase=dosBase;
    dosBase->dl_SysBase=sysBase;
    dosBase->dl_SegList=segList;
    
    InitSemaphore(&dosBase->dl_DosListLock);
    InitSemaphore(&dosBase->dl_LDSigSem);
    
    dosBase->dl_UtilityBase=OpenLibrary("utility.library",39);
    if(dosBase->dl_UtilityBase!=NULL)
    {
        static const struct TagItem tags[]=
        {
            { NP_Entry, (LONG)LDDemon }, { NP_Input, 0 }, { NP_Output, 0 },
            { NP_Name, (LONG)"lib & dev loader demon" }, { TAG_END, 0 }
        };
        dosBase->dl_LDDemon=CreateNewProc((struct TagItem *)tags);
        if(dosBase->dl_LDDemon!=NULL)
        {
            (void)SetFunction(&SysBase->LibNode,-92*sizeof(struct JumpVec),Dos_OpenLibrary);
            (void)SetFunction(&SysBase->LibNode,-74*sizeof(struct JumpVec),Dos_OpenDevice);
            (void)SetFunction(&SysBase->LibNode,-69*sizeof(struct JumpVec),Dos_CloseLibrary);
            (void)SetFunction(&SysBase->LibNode,-75*sizeof(struct JumpVec),Dos_CloseDevice);
            (void)SetFunction(&SysBase->LibNode,-67*sizeof(struct JumpVec),Dos_RemLibrary);
            (void)SetFunction(&SysBase->LibNode,-73*sizeof(struct JumpVec),Dos_RemLibrary);
            dosBase->dl_LDHandler.is_Node.ln_Name="lib & dev loader demon";
            dosBase->dl_LDHandler.is_Node.ln_Pri=0;
            dosBase->dl_LDHandler.is_Code=LDFlush;
            AddMemHandler(&dosBase->dl_LDHandler);
	    return dosBase;
	}
	CloseLibrary(dosBase->dl_UtilityBase);
    }

    return NULL;
    __AROS_FUNC_EXIT
}

__AROS_LH1(struct DosLibrary *, open,
 __AROS_LA(ULONG, version, D0),
	   struct DosLibrary *, DOSBase, 1, Dos)
{
    __AROS_FUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* Keep compiler happy */
    version=0;

    /* I have one more opener. */
    DOSBase->dl_lib.lib_OpenCnt++;
    DOSBase->dl_lib.lib_Flags&=~LIBF_DELEXP;

    /* You would return NULL if the open failed. */
    return DOSBase;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, close,
           struct DosLibrary *, DOSBase, 2, Dos)
{
    __AROS_FUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    /* I have one fewer opener. */
    if(!--DOSBase->dl_lib.lib_OpenCnt)
    {
	/* Delayed expunge pending? */
	if(DOSBase->dl_lib.lib_Flags&LIBF_DELEXP)
	    /* Then expunge the library */
	    return expunge();
    }
    return 0;
    __AROS_FUNC_EXIT
}

__AROS_LH0(BPTR, expunge,
           struct DosLibrary *, DOSBase, 3, Dos)
{
    __AROS_FUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
    if(DOSBase->dl_lib.lib_OpenCnt)
    {
	/* Set the delayed expunge flag and return. */
	DOSBase->dl_lib.lib_Flags|=LIBF_DELEXP;
	return 0;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&DOSBase->dl_lib.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret=DOSBase->dl_SegList;

    /* Free the memory. */
    FreeMem((char *)DOSBase-DOSBase->dl_lib.lib_NegSize,
	    DOSBase->dl_lib.lib_NegSize+DOSBase->dl_lib.lib_PosSize);

    return ret;
    __AROS_FUNC_EXIT
}

__AROS_LH0I(int, null,
            struct DosLibrary *, DOSBase, 4, Dos)
{
    __AROS_FUNC_INIT
    return 0;
    __AROS_FUNC_EXIT
}
