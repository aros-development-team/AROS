/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/09/13 17:50:06  digulla
    Use IPTR

    Revision 1.1  1996/09/11 12:54:45  digulla
    A couple of new DOS functions from M. Fleischer

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>
#include "dos_intern.h"

LONG DoName(struct IOFileSys *iofs, STRPTR name)
{
    extern struct DosLibrary *DOSBase;
    STRPTR volname, pathname, s1;
    BPTR cur;
    struct DosList *dl;
    struct Device *device;
    struct Unit *unit;
    struct FileHandle *fh;
    struct Process *me=(struct Process *)FindTask(NULL);

    if(!Strnicmp(name,"PROGDIR:",8))
    {
	cur=me->pr_HomeDir;
	volname=NULL;
	pathname=name+8;
    }else
    {
	/* Copy volume name */
	cur=me->pr_CurrentDir;
	s1=name;
	pathname=name;
	volname=NULL;
	while(*s1)
	    if(*s1++==':')
	    {
		volname=(STRPTR)AllocMem(s1-name,MEMF_ANY);
		if(volname==NULL)
		    return me->pr_Result2=ERROR_NO_FREE_STORE;
		CopyMem(name,volname,s1-name-1);
		volname[s1-name-1]=0;
		pathname=s1;
		break;
	    }
    }

    dl=LockDosList(LDF_ALL|LDF_READ);
    if(volname!=NULL)
    {
	/* Find logical device */
	dl=FindDosEntry(dl,volname,LDF_DEVICES|LDF_VOLUMES|LDF_ASSIGNS);
	if(dl==NULL)
	{
	    UnLockDosList(LDF_ALL|LDF_READ);
	    FreeMem(volname,s1-name);
	    return me->pr_Result2=ERROR_DEVICE_NOT_MOUNTED;
	}
	device=dl->dol_Device;
	unit  =dl->dol_Unit;
    }else if(cur)
    {
	fh=(struct FileHandle *)BADDR(cur);
	device=fh->fh_Device;
	unit  =fh->fh_Unit;
    }else
    {
	device=DOSBase->dl_NulHandler;
	unit  =DOSBase->dl_NulLock;
    }

    iofs->IOFS.io_Device =device;
    iofs->IOFS.io_Unit	 =unit;
    iofs->io_Args[0]=(IPTR)pathname;

    /* Send the request. */
    DoIO(&iofs->IOFS);

    if(dl!=NULL)
	UnLockDosList(LDF_ALL|LDF_READ);

    if(volname!=NULL)
	FreeMem(volname,s1-name);

    return me->pr_Result2=iofs->io_DosError;
}
