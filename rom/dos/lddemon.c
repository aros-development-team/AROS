/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Loader for shared libraries and devices
    Lang: english
*/
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include "dos_intern.h"

static BPTR LDLoad(STRPTR name, STRPTR basedir, struct DosLibrary *DOSBase)
{
    BPTR seglist;
    struct Process *me=(struct Process *)FindTask(NULL);
    struct DosList *dl1, *dl2;
    struct Process *caller=DOSBase->dl_LDCaller;

    if(caller->pr_Task.tc_Node.ln_Type==NT_PROCESS)
    {
	/* Try the caller's current dir */
	me->pr_CurrentDir=caller->pr_CurrentDir;
	seglist=LoadSeg(name);
	if(seglist)
	    return seglist;
    }
    /* Try the system's default directory. */
    dl1=LockDosList(LDF_ALL|LDF_READ);
    dl2=FindDosEntry(dl1,basedir,LDF_VOLUMES);
    if(dl2==NULL)
	dl2=FindDosEntry(dl1,basedir,LDF_DEVICES|LDF_ASSIGNS);
    if(dl2!=NULL)
    {
	struct FileHandle fh;
	fh.fh_Unit  =dl2->dol_Unit;
	fh.fh_Device=dl2->dol_Device;
	me->pr_CurrentDir=MKBADDR(&fh);
	seglist=LoadSeg(name);
    }
    else
	seglist=NULL;
    UnLockDosList(LDF_ALL|LDF_READ);
    return seglist;
}

static struct Library *LDInit(BPTR seglist, struct DosLibrary *DOSBase)
{
    BPTR seg=seglist;
    while(seg)
    {
	STRPTR addr=(STRPTR)BADDR(seg)-AROS_ALIGN(sizeof(ULONG));
	ULONG size=*(ULONG *)addr;
	for(;size>=sizeof(struct Resident);size-=AROS_PTRALIGN,addr+=AROS_PTRALIGN)
	{
	    struct Resident *res=(struct Resident *)addr;
	    if(res->rt_MatchWord==RTC_MATCHWORD&&res->rt_MatchTag==res)
	    {
		struct Library *lib=InitResident(res,seglist);
		if(lib==NULL)
		    UnLoadSeg(seglist);
		return lib;
	    }
	}
	seg=*(BPTR *)BADDR(seg);
    }
    UnLoadSeg(seglist);
    return NULL;
}

void LDDemon(void)
{
    extern struct DosLibrary *DOSBase;
    BPTR seglist;
    for(;;)
    {
	Wait(SIGF_DOS);
	seglist=LDLoad(DOSBase->dl_LDName,(STRPTR)DOSBase->dl_LDPtr,DOSBase);
	DOSBase->dl_LDPtr=LDInit(seglist,DOSBase);
	Signal(&DOSBase->dl_LDCaller->pr_Task,SIGF_DOS);
    }
}

AROS_LH2(struct Library *,OpenLibrary,
AROS_LHA(STRPTR,libName,A1),AROS_LHA(ULONG,version,D0),
struct ExecBase *,sysbase,0,Dos)
{
    AROS_LIBFUNC_INIT
    extern struct DosLibrary *DOSBase;
    struct Library *library;
    Forbid();
    library=(struct Library *)FindName(&SysBase->LibList,libName);
    if(library==NULL)
    {
	ObtainSemaphore(&DOSBase->dl_LDSigSem);
	DOSBase->dl_LDCaller=(struct Process *)FindTask(NULL);
	DOSBase->dl_LDName  =libName;
	DOSBase->dl_LDPtr   ="libs:";
	Signal((struct Task *)DOSBase->dl_LDDemon,SIGF_DOS);
	Wait(SIGF_DOS);
	library=(struct Library *)DOSBase->dl_LDPtr;
	ReleaseSemaphore(&DOSBase->dl_LDSigSem);
    }
    if(library!=NULL)
    {
	if(library->lib_Version>=version)
	    library=AROS_LVO_CALL1(struct Library *,
		AROS_LCA(ULONG,version,D0),
		struct Library *,library,1,
	    );
	else
	    library=NULL;
    }
    Permit();
    return library;
    AROS_LIBFUNC_EXIT
}

AROS_LH4(BYTE,OpenDevice,
    AROS_LHA(STRPTR,devName,A0),
    AROS_LHA(ULONG,unitNumber,D0),
    AROS_LHA(struct IORequest *,iORequest,A1),
    AROS_LHA(ULONG,flags,D1),
    struct ExecBase *,sysbase,0,Dos)
{
    AROS_LIBFUNC_INIT
    extern struct DosLibrary *DOSBase;
    struct Device *device;
    UBYTE ret=IOERR_OPENFAIL;
    Forbid();
    device=(struct Device *)FindName(&SysBase->DeviceList,devName);
    if(device==NULL)
    {
	ObtainSemaphore(&DOSBase->dl_LDSigSem);
	DOSBase->dl_LDCaller=(struct Process *)FindTask(NULL);
	DOSBase->dl_LDName  =devName;
	DOSBase->dl_LDPtr   ="devs:";
	Signal((struct Task *)DOSBase->dl_LDDemon,SIGF_DOS);
	Wait(SIGF_DOS);
	device=(struct Device *)DOSBase->dl_LDPtr;
	ReleaseSemaphore(&DOSBase->dl_LDSigSem);
    }
    if(device!=NULL)
    {
	iORequest->io_Error=0;
	iORequest->io_Device=device;
	iORequest->io_Flags=flags;
	iORequest->io_Message.mn_Node.ln_Type=NT_REPLYMSG;
	AROS_LVO_CALL3(void,
	    AROS_LCA(struct IORequest *,iORequest,A1),
	    AROS_LCA(ULONG,unitNumber,D0),
	    AROS_LCA(ULONG,flags,D1),
	    struct Device *,device,1,
	);
	ret=iORequest->io_Error;
	if(ret)
	    iORequest->io_Device=NULL;
    }
    Permit();
    return ret;
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void,CloseLibrary,
    AROS_LHA(struct Library *,library,A1),
    struct ExecBase *,sysbase,0,Dos)
{
    AROS_LIBFUNC_INIT
    extern struct DosLibrary *DOSBase;
    BPTR seglist;
    if(library!=NULL)
    {
	Forbid();
	seglist=AROS_LVO_CALL0(BPTR,
	    struct Library *,library,2,);
	if(seglist)
	{
	    DOSBase->dl_LDReturn=MEM_TRY_AGAIN;
	    UnLoadSeg(seglist);
	}
	Permit();
    }
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void,CloseDevice,
AROS_LHA(struct IORequest *,iORequest,A1),
struct ExecBase *,sysbase,0,Dos)
{
    AROS_LIBFUNC_INIT
    extern struct DosLibrary *DOSBase;
    BPTR seglist;
    Forbid();
    if(iORequest->io_Device!=NULL)
    {
	seglist=AROS_LVO_CALL1(BPTR,
	    AROS_LHA(struct IORequest *,iORequest,A1),
	    struct Device *,iORequest->io_Device,2,
	);
	iORequest->io_Device=(struct Device *)-1;
	if(seglist)
	{
	    DOSBase->dl_LDReturn=MEM_TRY_AGAIN;
	    UnLoadSeg(seglist);
	}
    }
    Permit();
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void,RemLibrary,
AROS_LHA(struct Library *,library,A1),
struct ExecBase *,sysbase,0,Dos)
{
    AROS_LIBFUNC_INIT
    extern struct DosLibrary *DOSBase;
    BPTR seglist;
    Forbid();
    seglist=AROS_LVO_CALL0(BPTR,
	struct Library *,library,3,
    );
    if(seglist)
    {
	DOSBase->dl_LDReturn=MEM_TRY_AGAIN;
	UnLoadSeg(seglist);
    }
    Permit();
    AROS_LIBFUNC_EXIT
}

LONG LDFlush(void)
{
    extern struct DosLibrary *DOSBase;
    struct Library *library;

    DOSBase->dl_LDReturn=MEM_DID_NOTHING;

    /* Forbid() is already done, but I don't want to rely on it. */
    Forbid();

    /* Follow the linked list of shared libraries. */
    library=(struct Library *)SysBase->LibList.lh_Head;
    while(library->lib_Node.ln_Succ!=NULL)
    {
	/* Flush libraries with a 0 open count */
	if(!library->lib_OpenCnt)
	{
	    RemLibrary(library);
	    /* Did it really go away? */
	    if(DOSBase->dl_LDReturn!=MEM_DID_NOTHING)
	    {
		/* Yes. Return it. */
		Permit();
		return DOSBase->dl_LDReturn;
	    }
	}
	/* Go to next. */
	library=(struct Library *)library->lib_Node.ln_Succ;
    }
    /* Do the same with the device list. */
    library=(struct Library *)SysBase->DeviceList.lh_Head;
    while(library->lib_Node.ln_Succ!=NULL)
    {
	if(!library->lib_OpenCnt)
	{
	    RemLibrary(library);
	    if(DOSBase->dl_LDReturn!=MEM_DID_NOTHING)
	    {
		Permit();
		return DOSBase->dl_LDReturn;
	    }
	}
	library=(struct Library *)library->lib_Node.ln_Succ;
    }
    Permit();
    return MEM_DID_NOTHING;
}
