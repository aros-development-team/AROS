/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Loader for shared libraries and devices.
*/

#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <aros/asmcall.h>
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include "dos_intern.h"
#include LC_LIBDEFS_FILE

#include <string.h>

#undef SysBase

#define CHECK_DEPENDENCY 1

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

#ifdef LIBEND
#undef LIBEND
#endif
#define LIBEND LDDemon_end

#include <libcore/compiler.h>

static const char name[];
static const char version[];
extern const int LIBEND TEXT_SECTION;
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
static const char version[] = "$VER: LDDemon 41.2 (17.3.2001)\r\n";
static const char ldDemonName[] = "Lib & Dev Loader Daemon";

/*
  BPTR LDLoad( caller, name, basedir, DOSBase )
    Try and load a segment from disk for the object <name>, relative
    to directory <basedir>. Will also try <caller>'s current and home
    directories.
*/
static BPTR
LDLoad(
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
    D(bug(
	"[LDLoad] caller=(%p) %s, name=%s, basedir=%s\n",
	caller, caller->pr_Task.tc_Node.ln_Name, name, basedir
    ));

    if (__is_process(caller))
    {
    	/* Try the current directory of the caller */

	D(bug("[LDLoad] Process\n"));
	me->pr_CurrentDir = caller->pr_CurrentDir;
	D(bug("[LDLoad] Trying currentdir\n"));
	seglist = LoadSeg(name);
	if( seglist )
	    return seglist;

	/* The the program directory of the caller */
	if( caller->pr_HomeDir != NULL )
	{
	    D(bug("[LDLoad] Trying homedir\n"));
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
    	D(bug("[LDLoad] Trying default dir, dp=%p\n", dp));
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
static struct Library *
LDInit(BPTR seglist, struct DosLibrary *DOSBase)
{
    struct ExecBase *SysBase = DOSBase->dl_SysBase;
    BPTR seg = seglist;

    while(seg)
    {
	STRPTR addr= (STRPTR)((LONG)BADDR(seg)-sizeof(ULONG));
	ULONG size = *(ULONG *)addr;

	for(
	    addr += sizeof(BPTR) + sizeof(ULONG),
		size -= sizeof(BPTR) + sizeof(ULONG);
	    size >= sizeof(struct Resident) ;
	    size -= AROS_PTRALIGN, addr += AROS_PTRALIGN
	)
	{
	    struct Resident *res = (struct Resident *)addr;
	    if(    res->rt_MatchWord == RTC_MATCHWORD
		&& res->rt_MatchTag == res )
	    {
		struct Library *lib;

		D(bug("[LDInit] Calling InitResident(%p) on %s\n", res, res->rt_Name));
		Forbid();
		lib = InitResident(res, seglist);
		Permit();
		D(bug("[LDInit] Done calling InitResident(%p) on %s\n", res, res->rt_Name));
		if( lib == NULL )
		    UnLoadSeg(seglist);
		return lib;
	    }
	}
	seg = *(BPTR *)BADDR(seg);
    }
    D(bug("[LDInit] Couldn't find Resident for %p\n", seglist));
    UnLoadSeg(seglist);
    return NULL;
}

struct Library *AROS_SLIB_ENTRY(OpenLibrary, Exec)();
BYTE            AROS_SLIB_ENTRY(OpenDevice,  Exec)();

#define ExecOpenLibrary(libname, version)                         \
AROS_CALL2(struct Library *, &AROS_SLIB_ENTRY(OpenLibrary, Exec), \
    AROS_LCA(STRPTR, libname, A1),                                \
    AROS_LCA(ULONG, version, D0),                                 \
    struct ExecBase *, SysBase)

#define ExecOpenDevice(devname, unitNumber, iORequest, flags)     \
AROS_CALL4(BYTE, &AROS_SLIB_ENTRY(OpenDevice, Exec),              \
    AROS_LCA(STRPTR, devname, A0),                                \
    AROS_LCA(ULONG, unitNumber, D0),                              \
    AROS_LCA(struct IORequest *, iORequest, A1),                  \
    AROS_LCA(ULONG, flags, D1),                                   \
    struct ExecBase *, SysBase)

struct LDObjectNode
{
    struct Node            ldon_Node;
    struct SignalSemaphore ldon_SigSem;
    ULONG                  ldon_AccessCount;
#if CHECK_DEPENDENCY
    struct Task           *ldon_FirstLocker;
#endif
};

struct LDObjectNode *LDNewObjectNode(STRPTR name, struct DosLibrary *DOSBase)
{
    struct ExecBase *SysBase = DOSBase->dl_SysBase;

    struct LDObjectNode *ret = AllocVec(sizeof(struct LDObjectNode), MEMF_ANY);
    if (ret)
    {
	ULONG  len = strlen(name);
	STRPTR dupname = AllocVec(len+1, MEMF_ANY);
 	if (dupname)
	{
	    CopyMem(name, dupname, len);
	    dupname[len] = '\0';
	    ret->ldon_Node.ln_Name = dupname;
            InitSemaphore(&ret->ldon_SigSem);
	    ret->ldon_AccessCount = 0;

	    #if CHECK_DEPENDENCY
	    ret->ldon_FirstLocker = FindTask(0);
            #endif

	    return ret;
        }
	FreeVec(ret);
    }

    return NULL;
}

VOID LDDestroyObjectNode(struct LDObjectNode *object, struct DosLibrary *DOSBase)
{
    struct ExecBase *SysBase = DOSBase->dl_SysBase;

    FreeVec(object->ldon_Node.ln_Name);
    FreeVec(object);
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
    struct LDObjectNode *object;

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

  	falemagn: I changed the implementation of all that.
	          There's a list of "LDObjectNodes", that contain the name
		  of the object being opened. Since the problem is that more
		  processes can attempt to open the same device/library Instead of
		  locking a global semaphore until the opening is done, we lock a
		  per-object semaphore, so that others libraries/devices can be opened
		  in the meantime. Beofore a deadlock could happen if there was a
		  situation like this:

		  Process A opens L --------> LDDemon loads L and locks sem S
		                              /                        \
					     /                          \
					  1 /                            \ 3
					   /                              \
					  /                   2            \
				L spawns a process B and ----------> The process opens
   				waits for it to respond             a library but gets loked
				to a message            <----/---- because sem S is locked
							    /\
							    ||
							    ||
							Proces B will never
							respond to L.

                 Hopefully this won't happen anymore now.
    */


    /*  We use FilePart() because the liblist is built from resident IDs,
	and contain no path. Eg. The user can request gadgets/foo.gadget,
	but the resident only contains foo.gadget
    */
    stripped_libname = FilePart(libname);
    ObtainSemaphore(&DOSBase->dl_LDObjectsListSigSem);
    object = (struct LDObjectNode *)FindName(&DOSBase->dl_LDObjectsList, stripped_libname);
    if (!object)
    {
        object = LDNewObjectNode(stripped_libname, DOSBase);
	if (object)
	{
	    AddTail(&DOSBase->dl_LDObjectsList, (struct Node *)object);
	}
    }
 
    if (object)
    {
    	object->ldon_AccessCount += 1;
    }
#if CHECK_DEPENDENCY
    else
    {
	struct Task  *curtask = FindTask(0);
	struct ETask *et      = GetETask(curtask);

	D(bug("Checking for circular dependency\n"));
	if (et)
	{
	    while (curtask && curtask != object->ldon_FirstLocker)
     		curtask = et->et_Parent;

	    if (curtask)
	    {
		bug("Circular dependency found!\n");
	        object = NULL;
	    }
        }
    }
#endif
    ReleaseSemaphore(&DOSBase->dl_LDObjectsListSigSem);

    if (!object)
        return NULL;


    ObtainSemaphore(&object->ldon_SigSem);

    /* Call the EXEC's OpenLibrary function */
    library = ExecOpenLibrary(stripped_libname, version);

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

    	SetSignal(0, SIGF_SINGLE);
	D(bug("[LDCaller] Sending request for %s v%ld\n", libname, version));
	PutMsg(DOSBase->dl_LDDemonPort, (struct Message *)&ldd);
	WaitPort(&ldd.ldd_ReplyPort);
	D(bug("[LDCaller] Returned\n"));

	library = LDInit(ldd.ldd_Return, DOSBase);

        if( library != NULL )
        {
	    /*
	        We have to Forbid() here because we need to look through the list
	        again, we also need to call the libOpen vector, which wants us
	        under a Forbidden state.

		falemagn: well, it doesn't want us under a Forbidden state, it just
		          wants to be single threaded, and it is, in fact, so no
			  need for Forbid()/Permit() around open. I Hope... :)

	    */
	    Forbid();
	    tmplib = (struct Library *)FindName(&SysBase->LibList, stripped_libname);
	    Permit();

	    if( tmplib != NULL )
	        library = tmplib;

	    if(library->lib_Version >= version)
	    {
	        D(bug("[LDCaller] Calling libOpen() of %s\n",
    		        library->lib_Node.ln_Name));

	        library = AROS_LVO_CALL1(struct Library *,
		    AROS_LCA(ULONG, version, D0),
		    struct Library *, library, 1,
	        );

	        D(bug("[LDCaller] libOpen() returned\n"));
	    }
	    else
	       library = NULL;
	}
    }

    if (library == NULL)
    {
        /*
	    the library is not on disk so
	    check Resident List
        */

	struct Resident *resident;

	resident = FindResident(stripped_libname);
	if (resident)
	{
	    if (resident->rt_Version >= version)
	    {
		if (InitResident(resident, NULL))
		    library = ExecOpenLibrary(stripped_libname, version);
	    }
	}
    }

    /*
	Release the semaphore here, after calling Open vector. This
	means that library open is singlethreaded by the semaphore.
	It also handles circular dependant libraries. (Won't deadlock),
	and recursive OpenLibrary calls (Semaphores nest when obtained
	several times in a row by the same task).
    */
    ObtainSemaphore(&DOSBase->dl_LDObjectsListSigSem);
    if (--(object->ldon_AccessCount) == 0)
    {
        Remove((struct Node *)object);
        LDDestroyObjectNode(object, DOSBase);
    }
    else
       ReleaseSemaphore(&object->ldon_SigSem);
    ReleaseSemaphore(&DOSBase->dl_LDObjectsListSigSem);

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
    struct Device *tmpdev;
    STRPTR stripped_devname;
    struct LDObjectNode *object;

    iORequest->io_Error  = IOERR_OPENFAIL;
    iORequest->io_Device = NULL;

    /*	We use FilePart() because the liblist is built from resident IDs,
	and contain no path. Eg. The user can request gadgets/foo.gadget,
	but the resident only contains foo.gadget
    */

    stripped_devname = FilePart(devname);

    ObtainSemaphore(&DOSBase->dl_LDObjectsListSigSem);
    object = (struct LDObjectNode *)FindName(&DOSBase->dl_LDObjectsList, stripped_devname);

    if (!object)
    {
        object = LDNewObjectNode(stripped_devname, DOSBase);
	if (object)
	{
	    AddTail(&DOSBase->dl_LDObjectsList, (struct Node*)object);
	}
    }

    if (object)
    {
    	object->ldon_AccessCount += 1;
    }
#if CHECK_DEPENDENCY
    else
    {
	struct Task  *curtask = FindTask(0);
	struct ETask *et      = GetETask(curtask);

	if (et)
	{
	    while (curtask && curtask != object->ldon_FirstLocker)
     		curtask = et->et_Parent;

	    if (curtask)
	    {
		bug("[LDCaller] Circular dependency found!\n");
	        object = NULL;
	    }
        }
    }
#endif
    ReleaseSemaphore(&DOSBase->dl_LDObjectsListSigSem);

    if (!object)
        return IOERR_OPENFAIL;

    ObtainSemaphore(&object->ldon_SigSem);

    ExecOpenDevice(stripped_devname, unitNumber, iORequest, flags);

    if (iORequest->io_Error)
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

	SetSignal(0, SIGF_SINGLE);
	D(bug("[LDCaller] Sending request for %s\n", devname));
	PutMsg(DOSBase->dl_LDDemonPort, (struct Message *)&ldd);
	WaitPort(&ldd.ldd_ReplyPort);
	D(bug("[LDCaller] Returned\n"));

	iORequest->io_Device = (struct Device *)LDInit(ldd.ldd_Return, DOSBase);

	if(iORequest->io_Device)
        {
	    Forbid();
	    tmpdev = (struct Device *)FindName(&SysBase->DeviceList, stripped_devname);
	    Permit();

	    if(tmpdev != NULL)
	        iORequest->io_Device = tmpdev;

	    iORequest->io_Error = 0;
	    iORequest->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

  	    D(bug("[LDCaller] Calling devOpen() of %s unit %ld\n",
		    iORequest->io_Device->dd_Library.lib_Node.ln_Name, unitNumber));

	    AROS_LVO_CALL3(void,
	        AROS_LCA(struct IORequest *, iORequest, A1),
	        AROS_LCA(ULONG, unitNumber, D0),
	        AROS_LCA(ULONG, flags, D1),
	        struct Device *, iORequest->io_Device, 1,
	    );

	    D(bug("[LDCaller] devOpen() returned\n"));

	    if (iORequest->io_Error)
	        iORequest->io_Device = NULL;
        }
    }

    ObtainSemaphore(&DOSBase->dl_LDObjectsListSigSem);
    if (--(object->ldon_AccessCount) == 0)
    {
        Remove((struct Node *)object);
        LDDestroyObjectNode(object, DOSBase);
    }
    else
       ReleaseSemaphore(&object->ldon_SigSem);
    ReleaseSemaphore(&DOSBase->dl_LDObjectsListSigSem);

    D(bug("%s", iORequest->io_Error?"[LDCaller] Couldn't open the device\n":""));

    return iORequest->io_Error;

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

AROS_UFH3(LONG, LDFlush,
    AROS_UFHA(struct MemHandlerData *, lmhd, A0),
    AROS_UFHA(APTR, data, A1),
    AROS_UFHA(struct ExecBase *, SysBase, A6)
)
{
    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct Library *library;

    D(bug("[LDDemon] Flush called\n"));
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

    AROS_USERFUNC_EXIT
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
    AROS_USERFUNC_INIT

    struct DosLibrary *DOSBase = SysBase->ex_RamLibPrivate;
    struct LDDMsg *ldd;

    for(;;)
    {
	WaitPort(DOSBase->dl_LDDemonPort);
	while( (ldd = (struct LDDMsg *)GetMsg(DOSBase->dl_LDDemonPort)) )
	{
	    D(bug("[LDDemon] Got a request for %s in %s\n",
		    ldd->ldd_Name, ldd->ldd_BaseDir));

	    ldd->ldd_Return = LDLoad(
		ldd->ldd_ReplyPort.mp_SigTask,
		ldd->ldd_Name,
		ldd->ldd_BaseDir,
		DOSBase);

	    if (ldd->ldd_Return == 0)
		bug("[LDDemon] Could not open %s\n",ldd->ldd_Name);
	    else
	    	D(bug("[LDDemon] Replying with %p as result\n", ldd->ldd_Return));
	    ReplyMsg((struct Message *)ldd);
	} /* messages available */
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3(ULONG, AROS_SLIB_ENTRY(Init, LDDemon),
 AROS_UFHA(ULONG,              dummy,   D0),
 AROS_UFHA(BPTR,               segList, A0),
 AROS_UFHA(struct ExecBase *,  sysBase, A6)
)
{
    AROS_USERFUNC_INIT
    AROS_GET_SYSBASE

    struct Library *AROS_SLIB_ENTRY(OpenLibrary,Dos)();
    BYTE AROS_SLIB_ENTRY(OpenDevice,Dos)();
    void AROS_SLIB_ENTRY(CloseLibrary,Dos)();
    void AROS_SLIB_ENTRY(CloseDevice,Dos)();
    void AROS_SLIB_ENTRY(RemLibrary,Dos)();

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

    if( (DOSBase->dl_LDDemonPort = CreateMsgPort()) == NULL )
    {
	Alert( AN_RAMLib | AG_NoMemory | AT_DeadEnd );
    }

    FreeSignal(DOSBase->dl_LDDemonPort->mp_SigBit);
    DOSBase->dl_LDDemonPort->mp_SigBit = SIGBREAKB_CTRL_F;
    
    DOSBase->dl_LDHandler.is_Node.ln_Name = (STRPTR)ldDemonName;
    DOSBase->dl_LDHandler.is_Node.ln_Pri = 0;
    DOSBase->dl_LDHandler.is_Code = (void (*)())LDFlush;
    DOSBase->dl_LDHandler.is_Data = NULL;

    NEWLIST(&DOSBase->dl_LDObjectsList);
    InitSemaphore(&DOSBase->dl_LDObjectsListSigSem);
    AddMemHandler(&DOSBase->dl_LDHandler);

    /*
     *	Grab the semaphore ourself. The reason for this is that it will
     *	cause all other tasks to wait until we have finished initialising
     *	before they try and open something.
     */
    ObtainSemaphore(&DOSBase->dl_LDObjectsListSigSem);

#define SetFunc(offs,ptr) \
    (void)SetFunction(&SysBase->LibNode, (offs)*LIB_VECTSIZE, \
    			AROS_SLIB_ENTRY(ptr,Dos))

    /* Do not set the vectors until you have initialised everything else. */
    SetFunc(-92, OpenLibrary);
    SetFunc(-74, OpenDevice);
    SetFunc(-69, CloseLibrary);
    SetFunc(-75, CloseDevice);
    SetFunc(-67, RemLibrary);
    SetFunc(-73, RemLibrary);

    if( !(DOSBase->dl_LDDemonTask = CreateNewProc((struct TagItem *)tags)) )
    {
	Alert( AT_DeadEnd | AN_RAMLib | AG_ProcCreate );
    }

    /* Fix up the MsgPort */

    DOSBase->dl_LDDemonPort->mp_SigTask = DOSBase->dl_LDDemonTask;

    /* Then unlock the semaphore to allow other processes to run. */
    ReleaseSemaphore(&DOSBase->dl_LDObjectsListSigSem);

    return 0;

    AROS_USERFUNC_EXIT
}

const int LIBEND TEXT_SECTION = 1;
