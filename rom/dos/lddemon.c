/*
    Copyright (C) 1995-1998 AROS - The Amiga Research OS
    $Id$

    Desc: Loader for shared libraries and devices.
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <aros/asmcall.h>
/* #define DEBUG 1 */
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include "dos_intern.h"
#include "libdefs.h"

#undef SysBase

/* Please leave them here! They are needed on Linux-M68K */
struct Library * Dos_OpenLibrary();
BYTE Dos_OpenDevice();
void Dos_CloseLibrary();
void Dos_CloseDevice();
void Dos_RemLibrary();

struct LDDMsg
{
    struct Message 	 ldd_Msg;	    /* Message link */
    struct MsgPort	 ldd_ReplyPort;	    /* Callers ReplyPort */
    
    STRPTR		 ldd_Name;	    /* Name of thing to load */
    ULONG		 ldd_Version;	    /* Version of thing to load */

    STRPTR		 ldd_BaseDir;	    /* Base directory to load from */
    struct Library *	 ldd_Return;	    /* The result */
};

static const char name[];
static const char version[];
extern char LIBEND;
static ULONG AROS_SLIB_ENTRY(Init, LDDemon)();

const struct Resident LDDemon_resident =
{
    RTC_MATCHWORD,
    (struct Resident *)&LDDemon_resident,
    &LIBEND,
    RTF_AFTERDOS,
    VERSION_NUMBER,
    NT_PROCESS,
    -125,
    (STRPTR)name,
    (STRPTR)&version[6],
    AROS_SLIB_ENTRY(Init,LDDemon)
};

static const char name[] = "LDDemon";
static const char version[] = "$VER: LDDemon 41.1 (26.12.1997)\r\n";
static const char ldDemonName[] = "Lib & Dev Loader Daemon";

/*
  BPTR LDLoad( caller, name, basedir, DOSBase )
    Try and load a segment from disk for the object <name>, relative
    to directory <basedir>. Will also try <caller>'s current and home
    directories.
*/
static BPTR LDLoad(
    struct Process *caller, 
    STRPTR name,
    STRPTR basedir,	
    struct DosLibrary *DOSBase
)
{
    struct ExecBase *SysBase = DOSBase->dl_SysBase;
    struct Process *me = (struct Process *)FindTask(NULL);
    struct DevProc *dp = NULL;
    BPTR seglist = NULL;

    /*
	If the caller was a process, we have more scope for loading
	libraries. We can load them from the callers current directory,
	or from the PROGDIR: assign. These could both be the same
	though.
    */
    D(bug("LDLoad(caller=%P, name=%s, basedir=%s)\n", caller, name, basedir));
    if( caller->pr_Task.tc_Node.ln_Type == NT_PROCESS )
    {		
    	/* Try the current directory of the caller */
	
	D(bug("Process\n"));
	me->pr_CurrentDir = caller->pr_CurrentDir;
	D(bug("Trying currentdir\n"));
	seglist = LoadSeg(name);
	if( seglist )
	    return seglist;

	/* The the program directory of the caller */
	if( caller->pr_HomeDir != NULL )
	{
	    D(bug("Trying homedir\n"));
	    me->pr_CurrentDir = caller->pr_HomeDir;
	    seglist = LoadSeg(name);
	    if( seglist )
		return seglist;
	}
    }

    /* Nup, lets try the default directory as supplied. */
    while(    seglist == NULL
	   && (dp = GetDeviceProc( basedir, dp )) != NULL 
    )
    {
    	D(bug("Trying default dir, dp=%p\n", dp));
	/* XXX: There is something bad here if dvp_Lock == NULL */
	me->pr_CurrentDir = dp->dvp_Lock;
	seglist = LoadSeg(name);
    }

    FreeDeviceProc(dp);
    return seglist;
}

/*
  Library *LDInit(seglist, DOSBase)
    Initialise the library.
*/
static struct Library *LDInit(BPTR seglist, struct DosLibrary *DOSBase)
{
    struct ExecBase *SysBase = DOSBase->dl_SysBase;
    BPTR seg = seglist;

    while(seg)
    {
	STRPTR addr= (STRPTR)((LONG)BADDR(seg)-sizeof(ULONG));
	ULONG size = *(ULONG *)addr;
	
	for(
	    addr += sizeof(BPTR) + sizeof(ULONG),
		size -= sizeof(BPTR) - sizeof(ULONG);
	    size >= sizeof(struct Resident) ;
	    size -= AROS_PTRALIGN, addr += AROS_PTRALIGN
	)
	{
	    struct Resident *res = (struct Resident *)addr;
	    if(    res->rt_MatchWord == RTC_MATCHWORD
		&& res->rt_MatchTag == res )
	    {
		struct Library *lib;

		D(bug("Calling InitResident(%p) on %s\n", res, res->rt_Name));
		Forbid();
		lib = InitResident(res, seglist);
		Permit();
		if( lib == NULL )
		    UnLoadSeg(seglist);
		return lib;
	    }
	}
	seg = *(BPTR *)BADDR(seg);
    }
    D(bug("LD: Couldn't find Resident for %p\n", seglist));
    UnLoadSeg(seglist);
    return NULL;
}


AROS_LH2(struct Library *, OpenLibrary,
    AROS_LHA(STRPTR, libname, A1),
    AROS_LHA(ULONG, version, D0),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExecBase *,SysBase)


    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct Library *library, *tmplib;
    STRPTR stripped_libname;

    /*
	We get the DOS semaphore to prevent the following:
	- task 1 tries to open foobar.library, needs to load it from disk...
	- task 1 Permit()'s (since its not doing list things)
	- task switch (whilst LDDemon MAY get process next it might not)
	- task 2 tries to open foobar.library, needs to load it from disk...
	- it also requests LDDemon to open foobar.library, so it is now
	  trying to open it twice
    
	We block all OpenLibrary() callers from searching the list until
	all the other OpenLibrary() callers have returned. That way, 
	task #2 won't ask for foobar.library until task #1 has got its
	response back from the LDDemon process.

    */


   /* We use FilePart() because the liblist is built from resident IDs,
      and contain no path. Eg. The user can request gadgets/foo.gadget,
      but the resident only contains foo.gadget
   */    
   stripped_libname = FilePart(libname);

    ObtainSemaphore(&DOSBase->dl_LSigSem);

    /* See if the library is in the library list */
    Forbid();
   /* We use FilePart() because the liblist is built from resident IDs,
      and contain no path. Eg. The user can request gadgets/foo.gadget,
      but the resident only contains foo.gadget
   */    
    library = (struct Library *)FindName(&SysBase->LibList, stripped_libname); 
    Permit();

    if( library == NULL )
    {
	/* Use stack for now, this could be a security hole */
	struct LDDMsg ldd;

	ldd.ldd_ReplyPort.mp_SigBit = SIGB_SINGLE;
	ldd.ldd_ReplyPort.mp_SigTask = FindTask(NULL);
	NEWLIST(&ldd.ldd_ReplyPort.mp_MsgList);
	ldd.ldd_ReplyPort.mp_Flags = PA_SIGNAL;
	ldd.ldd_ReplyPort.mp_Node.ln_Type = NT_MSGPORT;

	ldd.ldd_Msg.mn_Node.ln_Type = NT_MESSAGE;

	ldd.ldd_Msg.mn_Length = sizeof(struct LDDMsg);
	ldd.ldd_Msg.mn_ReplyPort = &ldd.ldd_ReplyPort;

	ldd.ldd_Name = libname;
	ldd.ldd_Version = version;
	ldd.ldd_BaseDir = "libs:";

	D(bug("LDCaller: Sending request for %s v%ld\n", libname, version));
	PutMsg(DOSBase->dl_LDDemonPort, (struct Message *)&ldd);
	WaitPort(&ldd.ldd_ReplyPort);
/*	D(bug("Returned from LDDemon\n"));
*/
	library = LDInit(ldd.ldd_Return, DOSBase);
    }


    if( library != NULL )
    {
	/*
	    We have to Forbid() here because we need to look through the list
	    again, we also need to call the libOpen vector, which wants us
	    under a Forbidden state.
	*/
	Forbid();
	tmplib = (struct Library *)FindName(&SysBase->LibList, stripped_libname);
	if( tmplib != NULL )
	    library = tmplib;

	if( library && library->lib_Version >= version)
	{
	    D(bug("LDCaller: Calling libOpen() of %s\n",
    		    library->lib_Node.ln_Name));

	    library = AROS_LVO_CALL1(struct Library *,
		AROS_LCA(ULONG, version, D0),
		struct Library *, library, 1,
	    );
	}
	else
	    library = NULL;
	Permit();
    }
    
    /*
	Release the semaphore here, after calling Open vector. This
	means that library open is singlethreaded by the semaphore.
	It also handles circular dependant libraries. (Won't deadlock),
	and recursive OpenLibrary calls (Semaphores nest when obtained
	several times in a row by the same task).
    */
    ReleaseSemaphore(&DOSBase->dl_LSigSem);

    return library;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(BYTE, OpenDevice,
    AROS_LHA(STRPTR, devname, A0),
    AROS_LHA(ULONG, unitNumber, D0),
    AROS_LHA(struct IORequest *, iORequest, A1),
    AROS_LHA(ULONG, flags, D1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct Device *device, *tmpdev;
    UBYTE ret = IOERR_OPENFAIL;
    STRPTR stripped_devname;
    

    /* We use FilePart() because the liblist is built from resident IDs,
      and contain no path. Eg. The user can request gadgets/foo.gadget,
      but the resident only contains foo.gadget
    */    
    stripped_devname = FilePart(devname);
    /* Read the discussion in Dos_OpenLibrary() above for why we lock here */
    ObtainSemaphore(&DOSBase->dl_DSigSem);

    /* See if the device is in the device list */
    Forbid();
    device = (struct Device *)FindName(&SysBase->DeviceList, stripped_devname);
    Permit();

    if( device == NULL )
    {
	/* Use stack for now, this could be a security hole */
	struct LDDMsg ldd;

	ldd.ldd_ReplyPort.mp_SigBit = SIGB_SINGLE;
	ldd.ldd_ReplyPort.mp_SigTask = FindTask(NULL);
	NEWLIST(&ldd.ldd_ReplyPort.mp_MsgList);
	ldd.ldd_ReplyPort.mp_Flags = PA_SIGNAL;
	ldd.ldd_ReplyPort.mp_Node.ln_Type = NT_MSGPORT;

	ldd.ldd_Msg.mn_Node.ln_Type = NT_MESSAGE;
	ldd.ldd_Msg.mn_Length = sizeof(struct LDDMsg);
	ldd.ldd_Msg.mn_ReplyPort = &ldd.ldd_ReplyPort;

	ldd.ldd_Name = devname;
	ldd.ldd_BaseDir = "devs:";

	D(bug("LDCaller: Sending request for %s\n", devname));
	PutMsg(DOSBase->dl_LDDemonPort, (struct Message *)&ldd);
	WaitPort(&ldd.ldd_ReplyPort);

	device = (struct Device *)LDInit(ldd.ldd_Return, DOSBase);
    }

    ReleaseSemaphore(&DOSBase->dl_DSigSem);
    
    if( device != NULL )
    {
	Forbid();

	tmpdev = (struct Device *)FindName(&SysBase->DeviceList, stripped_devname);
	if(tmpdev != NULL)
	    device = tmpdev;

	iORequest->io_Error = 0;
	iORequest->io_Device = device;
	iORequest->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

	D(bug("LDCaller: Calling devOpen() of %s unit %ld\n", 
		device->dd_Library.lib_Node.ln_Name, unitNumber));

	AROS_LVO_CALL3(void,
	    AROS_LCA(struct IORequest *, iORequest, A1),
	    AROS_LCA(ULONG, unitNumber, D0),
	    AROS_LCA(ULONG, flags, D1),
	    struct Device *, device, 1,
	);
	Permit();

	ret = iORequest->io_Error;
	if( ret )
	    iORequest->io_Device = NULL;

    }
    return ret;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, CloseLibrary,
    AROS_LHA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    BPTR seglist;

    if( library != NULL )
    {
	Forbid();
	seglist = AROS_LVO_CALL0(BPTR, struct Library *, library, 2, );
	if( seglist )
	{
	    DOSBase->dl_LDReturn = MEM_TRY_AGAIN;

	    /* Safe to call from a Task */
	    UnLoadSeg(seglist);
	}
	Permit();
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, CloseDevice,
    AROS_LHA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT
    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    BPTR seglist = NULL;

    Forbid();
    if( iORequest->io_Device != NULL )
    {
	seglist = AROS_LVO_CALL1(BPTR, 
		    AROS_LCA(struct IORequest *, iORequest, A1),
		    struct Device, iORequest->io_Device, 2, );
	iORequest->io_Device=(struct Device *)-1;
	if( seglist )
	{
	    DOSBase->dl_LDReturn = MEM_TRY_AGAIN;
	    UnLoadSeg(seglist);
	}
    }
    Permit();
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, RemLibrary,
    AROS_LHA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    BPTR seglist;

    Forbid();
    seglist = AROS_LVO_CALL0(BPTR, struct Library *, library, 3, );
    if( seglist )
    {
	DOSBase->dl_LDReturn = MEM_TRY_AGAIN;
	UnLoadSeg(seglist);
    }
    Permit();

    AROS_LIBFUNC_EXIT
}

LONG LDFlush(void)
{
    extern struct ExecBase *SysBase;
    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct Library *library;
    
    DOSBase->dl_LDReturn = MEM_DID_NOTHING;

    /* Forbid() is already done, but I don't want to rely on it. */
    Forbid();

    /* Follow the linked list of shared libraries. */
    library = (struct Library *)SysBase->LibList.lh_Head;
    while(library->lib_Node.ln_Succ != NULL)
    {
	/* Flush libraries with a 0 open count */
	if( ! library->lib_OpenCnt )
	{
	    RemLibrary(library);
	    /* Did it really go away? */
	    if( DOSBase->dl_LDReturn != MEM_DID_NOTHING )
	    {
		/* Yes! Return it. */
		Permit();
		return MEM_TRY_AGAIN;
	    }
	}
	/* Go on to next library. */
	library = (struct Library *)library->lib_Node.ln_Succ;
    }

    /* Do the same with the device list. */
    library = (struct Library *)SysBase->DeviceList.lh_Head;
    while(library->lib_Node.ln_Succ != NULL)
    {
	/* Flush libraries with a 0 open count */
	if( ! library->lib_OpenCnt )
	{
	    RemDevice((struct Device *)library);
	    /* Did it really go away? */
	    if( DOSBase->dl_LDReturn != MEM_DID_NOTHING )
	    {
		/* Yes! Return it. */
		Permit();
		return MEM_TRY_AGAIN;
	    }
	}
	/* Go on to next library. */
	library = (struct Library *)library->lib_Node.ln_Succ;
    }
    Permit();
    return MEM_DID_NOTHING;
}

/*
  void LDDemon()
    The LDDemon process entry. Sits around and does nothing until a 
    request for a library comes, when it will then find the library
    and hopefully open it.
*/
AROS_UFH3(void, LDDemon,
    AROS_UFHA(STRPTR, argstr, A0),
    AROS_UFHA(ULONG, arglen, D0),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    struct Library *AROS_SLIB_ENTRY(OpenLibrary,Dos)();
    BYTE AROS_SLIB_ENTRY(OpenDevice,Dos)();
    void AROS_SLIB_ENTRY(CloseLibrary,Dos)();
    void AROS_SLIB_ENTRY(CloseDevice,Dos)();
    void AROS_SLIB_ENTRY(RemLibrary,Dos)();
    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct LDDMsg *ldd;
    
    struct Task *bootproc;

    /* Complete the initialisation. */
    if( (DOSBase->dl_LDDemonPort = CreateMsgPort()) == NULL )
    {
	Alert( AN_RAMLib | AG_NoMemory | AT_DeadEnd );
    }

#define SetFunc(offs,ptr) \
    (void)SetFunction(&SysBase->LibNode, (offs)*LIB_VECTSIZE, \
    			AROS_SLIB_ENTRY(ptr,Dos))

    SetFunc(-92, OpenLibrary);
    SetFunc(-74, OpenDevice);
    SetFunc(-69, CloseLibrary);
    SetFunc(-75, CloseDevice);
    SetFunc(-67, RemLibrary);
    SetFunc(-73, RemLibrary);

    DOSBase->dl_LDHandler.is_Node.ln_Name = (STRPTR)ldDemonName;
    DOSBase->dl_LDHandler.is_Node.ln_Pri = 0;
    DOSBase->dl_LDHandler.is_Code = (void (*)())LDFlush;

    InitSemaphore(&DOSBase->dl_LSigSem);
    InitSemaphore(&DOSBase->dl_DSigSem);
    AddMemHandler(&DOSBase->dl_LDHandler);
    
    /* Do syncronization with boot process (./dosboot.c):
       assure that LDDemon is initialize before the boot
       process, as the boot process might want to open
       disk-based libraries or devices.
    */
    
    Forbid(); /* To assure that if bootprocess is still not added to the system, it won't start here */
    bootproc = FindTask("Boot Process");
    if (bootproc)
    {
	Signal(bootproc, SIGBREAKF_CTRL_F);
    }
    else
    	kprintf("lddemon.c: LDDemon process scheduled after Boot process\nor name of Boot Process changed\nwhich causes Boot process to halt\n");
    Permit();
    
    for(;;)
    {
	WaitPort(DOSBase->dl_LDDemonPort);
	while( (ldd = (struct LDDMsg *)GetMsg(DOSBase->dl_LDDemonPort)) )
	{
	    D(bug("LDDemon: Got a request for %s in %s\n",
		    ldd->ldd_Name, ldd->ldd_BaseDir));

#if 0
	    seglist = LDLoad(
		ldd->ldd_ReplyPort.mp_SigTask,
		ldd->ldd_Name,
		ldd->ldd_BaseDir,
		DOSBase);
	    ldd->ldd_Return = LDInit(seglist, DOSBase);
#endif
	    ldd->ldd_Return = LDLoad(
		ldd->ldd_ReplyPort.mp_SigTask,
		ldd->ldd_Name,
		ldd->ldd_BaseDir,
		DOSBase);

	    D(bug("LDDemon: Replying with %p as result\n", ldd->ldd_Return));
	    ReplyMsg((struct Message *)ldd);
	} /* messages available */
    }
}

AROS_LH2(ULONG, Init,
    AROS_LHA(ULONG, dummy, D0),
    AROS_LHA(BPTR, seglist, A0),
    struct ExecBase *, SysBase, 0, LDDemon)
{
    AROS_LIBFUNC_INIT

    struct DosLibrary *DOSBase;
    struct TagItem tags[] =
    {
	{ NP_Entry, (IPTR)LDDemon },
	{ NP_Input, 0 },
	{ NP_Output, 0 },
	{ NP_Name, (IPTR)ldDemonName },
	{ NP_StackSize, AROS_STACKSIZE },
	{ TAG_END , 0 }
    };

    /* We want version v41 of DOS, since it corresponds to AROS atm... */
    if((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 41)) == NULL)
    {
	Alert(AT_DeadEnd | AN_RAMLib | AG_OpenLib | AO_DOSLib);
    }

    SysBase->ex_RamLibPrivate = DOSBase;

    if( !(DOSBase->dl_LDDemonTask = CreateNewProc((struct TagItem *)tags)) )
    {
	Alert( AT_DeadEnd | AN_RAMLib | AG_ProcCreate );
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}
