/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Loader for shared libraries and devices.
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/ports.h>
#include <exec/alerts.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <stddef.h>
#include <string.h>

#include "lddemon.h"

#define CHECK_DEPENDENCY 1

/* Please leave them here! They are needed on Linux-M68K */
AROS_LD2(struct Library *, OpenLibrary,
    AROS_LDA(STRPTR, libname, A1),
    AROS_LDA(ULONG, version, D0),
    struct ExecBase *, SysBase, 0, Dos);
AROS_LD4(LONG, OpenDevice,
    AROS_LDA(STRPTR, devname, A0),
    AROS_LDA(IPTR, unitNumber, D0),
    AROS_LDA(struct IORequest *, iORequest, A1),
    AROS_LDA(ULONG, flags, D1),
    struct ExecBase *, SysBase, 0, Dos);
AROS_LD1(void, CloseLibrary,
    AROS_LDA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Dos);
AROS_LD1(void, CloseDevice,
    AROS_LDA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 0, Dos);
AROS_LD1(void, RemLibrary,
    AROS_LDA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Dos);

struct LDDMsg
{
    struct Message 	 ldd_Msg;	    /* Message link */
    struct MsgPort	 ldd_ReplyPort;	    /* Callers ReplyPort */

    STRPTR		 ldd_Name;	    /* Name of thing to load */

    STRPTR		 ldd_BaseDir;	    /* Base directory to load from */
    BPTR		 ldd_Return;	    /* Loaded seglist */
};

static const char ldDemonName[] = "Lib & Dev Loader Daemon";

/*
  BPTR LDLoad( caller, name, basedir, DOSBase )
    Try and load a segment from disk for the object <name>, relative
    to directory <basedir>. Will also try <caller>'s current and home
    directories.
*/
static BPTR LDLoad(struct Process *caller, STRPTR name, STRPTR basedir,
		   struct Library *DOSBase, struct ExecBase *SysBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR seglist = BNULL;
    STRPTR path;
    ULONG pathLen;
    int delimPos;

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

    if (strncmp(name, "PROGDIR:", 8) == 0)
    {
	/* Special case for explicit PROGDIR-based path */
	if (caller->pr_Task.tc_Node.ln_Type == NT_PROCESS)
	{
	    if (caller->pr_HomeDir != BNULL)
	    {
		BPTR oldHomeDir = me->pr_HomeDir;
		D(bug("[LDLoad] Trying homedir\n"));
		/* Temporarily override pr_HomeDir to let GetDeviceProc handle
		   PROGDIR: case correctly while opening library file */
		me->pr_HomeDir = caller->pr_HomeDir;
		seglist = LoadSeg(name);
		me->pr_HomeDir = oldHomeDir;
	    }	
	}
    }
    else if (!strstr(name, ":")) {
        delimPos = strlen(basedir);
	pathLen = delimPos + strlen(name) + 2;
        path = AllocMem(pathLen, MEMF_ANY);
	if (path) {
	    strcpy(path, basedir);
	    path[delimPos] = '/';
	    strcpy(&path[delimPos + 1], name);
	}
	if (caller->pr_Task.tc_Node.ln_Type == NT_PROCESS)
	{
    	    /* Try the current directory of the caller */

    	    D(bug("[LDLoad] Process\n"));
	    me->pr_CurrentDir = caller->pr_CurrentDir;
	    D(bug("[LDLoad] Trying currentdir\n"));
	    seglist = LoadSeg(name);
	    if ((!seglist) && path)
		seglist = LoadSeg(path);

	/* The the program directory of the caller */
	    if((!seglist) && (caller->pr_HomeDir != BNULL))
	    {
		D(bug("[LDLoad] Trying homedir\n"));
		me->pr_CurrentDir = caller->pr_HomeDir;
		seglist = LoadSeg(name);
		if ((!seglist) && path)
		    seglist = LoadSeg(path);
	    }
	}

	if (path) {
	    if (!seglist) {
		/* Nup, let's try the default directory as supplied. */
		D(bug("[LDLoad] Trying defaultdir\n"));
		path[delimPos] = ':';
		seglist = LoadSeg(path);
	    }
	    FreeMem(path, pathLen);
	}
    } else
	seglist = LoadSeg(name);

    return seglist;
}

/*
  Library *LDInit(seglist, DOSBase)
    Initialise the library.
*/
static struct Library *LDInit(BPTR seglist, struct List *list, struct ExecBase *SysBase)
{
    BPTR seg = seglist;

    /* we may not have any extension fields */ 
    const int sizeofresident = offsetof(struct Resident, rt_Init) + sizeof(APTR);

    while(seg)
    {
	STRPTR addr = (STRPTR)((IPTR)BADDR(seg) - sizeof(ULONG));
	ULONG size = *(ULONG *)addr;

	for(
	    addr += sizeof(BPTR) + sizeof(ULONG),
		size -= sizeof(BPTR) + sizeof(ULONG);
	    size >= sizeofresident;
	    size -= 2, addr += 2
//	    size -= AROS_PTRALIGN, addr += AROS_PTRALIGN
	)
	{
	    struct Resident *res = (struct Resident *)addr;
	    if(    res->rt_MatchWord == RTC_MATCHWORD
		&& res->rt_MatchTag == res )
	    {
		struct Node *node;

		D(bug("[LDInit] Calling InitResident(%p) on %s\n", res, res->rt_Name));
		/* AOS compatibility requirement. 
		 * Ramlib ignores InitResident() return code.
		 * After InitResident() it checks if lib/dev appeared
		 * in Exec lib/dev list via FindName().
		 *
		 * Evidently InitResident()'s return code was not
		 * reliable for some early AOS libraries.
		 */
		Forbid();
		InitResident(res, seglist);
		node = FindName(list, res->rt_Name);
		Permit();
		D(bug("[LDInit] Done calling InitResident(%p) on %s, seg %p, node %p\n", res, res->rt_Name, BADDR(seglist), node));

		return (struct Library*)node;
	    }
	}
	seg = *(BPTR *)BADDR(seg);
    }
    D(bug("[LDInit] Couldn't find Resident for %p\n", seglist));
    return NULL;
}

#define ExecOpenLibrary(libname, version)                         \
AROS_CALL2(struct Library *, ldBase->__OpenLibrary,               \
    AROS_LCA(STRPTR, libname, A1),                                \
    AROS_LCA(ULONG, version, D0),                                 \
    struct ExecBase *, SysBase)

#define ExecOpenDevice(devname, unitNumber, iORequest, flags)     \
AROS_CALL4(LONG, ldBase->__OpenDevice,                            \
    AROS_LCA(STRPTR, devname, A0),                                \
    AROS_LCA(IPTR, unitNumber, D0),                               \
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

static struct LDObjectNode *LDNewObjectNode(STRPTR name, struct ExecBase *SysBase)
{
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

static struct LDObjectNode *LDRequestObject(STRPTR libname, ULONG version, STRPTR dir, struct List *list, struct ExecBase *SysBase)
{
    /*  We use FilePart() because the liblist is built from resident IDs,
	and contain no path. Eg. The user can request gadgets/foo.gadget,
	but the resident only contains foo.gadget
    */
    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    STRPTR stripped_libname = FilePart(libname);
    struct Library *tmplib;
    struct LDObjectNode *object;

    /*
	We get the DOS semaphore to prevent the following:
	- task 1 tries to open foobar.library, needs to load it from disk...
	- task 1 Permit()'s (since it's not doing list things)
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
		  processes can attempt to open the same device/library. Instead of
		  locking a global semaphore until the opening is done, we lock a
		  per-object semaphore, so that others libraries/devices can be opened
		  in the meantime. Before, a deadlock could happen if there was a
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
    ObtainSemaphore(&ldBase->dl_LDObjectsListSigSem);

    object = (struct LDObjectNode *)FindName(&ldBase->dl_LDObjectsList, stripped_libname);
    if (!object)
    {
        object = LDNewObjectNode(stripped_libname, SysBase);
	if (object)
	{
	    AddTail(&ldBase->dl_LDObjectsList, (struct Node *)object);
	}
    }
 
    if (object)
    {
    	object->ldon_AccessCount += 1;
    }

    ReleaseSemaphore(&ldBase->dl_LDObjectsListSigSem);

    if (!object)
        return NULL;

    ObtainSemaphore(&object->ldon_SigSem);

    /* Try to find the resident in the list */
    tmplib = (struct Library *)FindName(list, stripped_libname);

    if (!tmplib)
    {
	/* Try to load from disk if not found */
	struct LDDMsg ldd;

	ldd.ldd_ReplyPort.mp_SigBit       = SIGB_SINGLE;
	ldd.ldd_ReplyPort.mp_SigTask      = FindTask(NULL);
	ldd.ldd_ReplyPort.mp_Flags        = PA_SIGNAL;
	ldd.ldd_ReplyPort.mp_Node.ln_Type = NT_MSGPORT;

	NEWLIST(&ldd.ldd_ReplyPort.mp_MsgList);

	ldd.ldd_Msg.mn_Node.ln_Type = NT_MESSAGE;
	ldd.ldd_Msg.mn_Length       = sizeof(struct LDDMsg);
	ldd.ldd_Msg.mn_ReplyPort    = &ldd.ldd_ReplyPort;

	ldd.ldd_Name    = libname;
	ldd.ldd_BaseDir = dir;

    	SetSignal(0, SIGF_SINGLE);
	D(bug("[LDCaller] Sending request for %s\n", stripped_libname));

	PutMsg(ldBase->dl_LDDemonPort, (struct Message *)&ldd);
	WaitPort(&ldd.ldd_ReplyPort);

	D(bug("[LDCaller] Returned 0x%p\n", ldd.ldd_Return));

	if (ldd.ldd_Return)
	{
	    tmplib = LDInit(ldd.ldd_Return, list, SysBase);
	    if (!tmplib)
	    	UnLoadSeg(ldd.ldd_Return);
	}
    }

    if (!tmplib)
    {
        /* 
         * The library is not on disk so check Resident List.
         * It can be there if it is resident but was flushed.
         */
	struct Resident *resident = FindResident(stripped_libname);

	/*
	 * Check if the resident is of required type and version is correct.
	 * This relies on the fact that lh_Type is set correctly for exec lists.
	 * In AROS this is true (see rom/exec/prepareexecbase.c).
	 */
	if (resident && (resident->rt_Type == list->lh_Type) && (resident->rt_Version >= version))
	    InitResident(resident, BNULL);
    }

    return object;
}

static void LDReleaseObject(struct LDObjectNode *object, struct ExecBase *SysBase)
{
    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;

    /*
	Release the semaphore here, after calling Open vector. This
	means that library open is singlethreaded by the semaphore.
	It also handles circular dependant libraries. (Won't deadlock),
	and recursive OpenLibrary calls (Semaphores nest when obtained
	several times in a row by the same task).
    */

    ObtainSemaphore(&ldBase->dl_LDObjectsListSigSem);

    if (--(object->ldon_AccessCount) == 0)
    {
        Remove((struct Node *)object);
        /*
         * CHECKME: In LDRequestObject() we obtain the object semaphore also on a new object.
         * So shouldn't we release it here too ?
         */
         
        FreeVec(object->ldon_Node.ln_Name);
    	FreeVec(object);
    }
    else
       ReleaseSemaphore(&object->ldon_SigSem);

    ReleaseSemaphore(&ldBase->dl_LDObjectsListSigSem);
}

AROS_LH2(struct Library *, OpenLibrary,
    AROS_LHA(STRPTR, libname, A1),
    AROS_LHA(ULONG, version, D0),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *library;
    struct LDObjectNode *object = LDRequestObject(libname, version, "libs", &SysBase->LibList, SysBase);

    if (!object)
    	return NULL;

    /* Call the EXEC's OpenLibrary function */
    library = ExecOpenLibrary(object->ldon_Node.ln_Name, version);

    LDReleaseObject(object, SysBase);

    return library;

    AROS_LIBFUNC_EXIT
}

AROS_LH4(LONG, OpenDevice,
    AROS_LHA(STRPTR, devname, A0),
    AROS_LHA(IPTR, unitNumber, D0),
    AROS_LHA(struct IORequest *, iORequest, A1),
    AROS_LHA(ULONG, flags, D1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct LDObjectNode *object;
    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;

    object = LDRequestObject(devname, 0, "devs", &SysBase->DeviceList, SysBase);
    if (object)
    {
    	/* Call exec.library/OpenDevice(), it will do the job */
    	ExecOpenDevice(object->ldon_Node.ln_Name, unitNumber, iORequest, flags);
    	LDReleaseObject(object, SysBase);
    }
    else
    {
        iORequest->io_Error  = IOERR_OPENFAIL;
	iORequest->io_Device = NULL;
	iORequest->io_Unit   = NULL;
    }

    D(bug("[LDCaller] Open result: %d\n", iORequest->io_Error));

    return iORequest->io_Error;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, CloseLibrary,
    AROS_LHA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Dos)
{
    AROS_LIBFUNC_INIT

    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist;

    if( library != NULL )
    {
	Forbid();
	seglist = AROS_LVO_CALL0(BPTR, struct Library *, library, 2, );
	if( seglist )
	{
	    ldBase->dl_LDReturn = MEM_TRY_AGAIN;

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
    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist = BNULL;

    Forbid();
    if( iORequest->io_Device != NULL )
    {
	seglist = AROS_LVO_CALL1(BPTR,
		    AROS_LCA(struct IORequest *, iORequest, A1),
		    struct Device *, iORequest->io_Device, 2, );
	iORequest->io_Device=(struct Device *)-1;
	if( seglist )
	{
	    ldBase->dl_LDReturn = MEM_TRY_AGAIN;
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

    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist;

    Forbid();
    /* calling ExpungeLib: library ends up in D0 and A6 for compatibility */
    seglist = AROS_CALL1(BPTR, __AROS_GETVECADDR(library, 3),
		AROS_LCA(struct Library *, library, D0),
		struct Library *, library
    );
    if( seglist )
    {
	ldBase->dl_LDReturn = MEM_TRY_AGAIN;
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

    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *library;

    D(bug("[LDDemon] Flush called\n"));
    ldBase->dl_LDReturn = MEM_DID_NOTHING;

    /* Forbid() is already done, but I don't want to rely on it. */
    Forbid();

    /* Follow the linked list of shared libraries. */
    library = (struct Library *)SysBase->LibList.lh_Head;
    while(library->lib_Node.ln_Succ != NULL)
    {
	/* Flush libraries with a 0 open count */
	if( ! library->lib_OpenCnt )
	{
	    /* the library list node will be wiped from memory */
	    struct Library *nextLib = (struct Library *)library->lib_Node.ln_Succ;
	    RemLibrary(library);
	    /* Did it really go away? */
	    if( ldBase->dl_LDReturn != MEM_DID_NOTHING )
	    {
		/* Yes! Return it. */
		Permit();
		return MEM_TRY_AGAIN;
	    }
	    library = nextLib;
	}
	else
	{
	    /* Go on to next library. */
	    library = (struct Library *)library->lib_Node.ln_Succ;
	}
    }

    /* Do the same with the device list. */
    library = (struct Library *)SysBase->DeviceList.lh_Head;
    while(library->lib_Node.ln_Succ != NULL)
    {
	/* Flush libraries with a 0 open count */
	if( ! library->lib_OpenCnt )
	{
	    struct Library *nextDev = (struct Library *)library->lib_Node.ln_Succ;
	    RemDevice((struct Device *)library);
	    /* Did it really go away? */
	    if( ldBase->dl_LDReturn != MEM_DID_NOTHING )
	    {
		/* Yes! Return it. */
		Permit();
		return MEM_TRY_AGAIN;
	    }
	    library = nextDev;
	}
	else
	{
	    /* Go on to next library. */
	    library = (struct Library *)library->lib_Node.ln_Succ;
	}
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
static AROS_ENTRY(VOID, LDDemon,
        AROS_UFHA(APTR, argptr, A0),
        AROS_UFHA(APTR, argsize, D0),
        struct ExecBase *, SysBase)
{
    AROS_USERFUNC_INIT

    struct LDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    struct LDDMsg *ldd;

    for(;;)
    {
	WaitPort(ldBase->dl_LDDemonPort);
	while( (ldd = (struct LDDMsg *)GetMsg(ldBase->dl_LDDemonPort)) )
	{
	    D(bug("[LDDemon] Got a request for %s in %s\n", ldd->ldd_Name, ldd->ldd_BaseDir));

	    ldd->ldd_Return = LDLoad(ldd->ldd_ReplyPort.mp_SigTask, ldd->ldd_Name, ldd->ldd_BaseDir, DOSBase, SysBase);

	    D(bug("[LDDemon] Replying with %p as result\n", ldd->ldd_Return));
	    ReplyMsg((struct Message *)ldd);
	} /* messages available */
    }

    AROS_USERFUNC_EXIT
}

static ULONG LDDemon_Init(struct LDDemonBase *ldBase)
{
    struct Library *DOSBase;
    struct TagItem tags[] =
    {
	{ NP_Entry, (IPTR)LDDemon },
	{ NP_Input, 0 },
	{ NP_Output, 0 },
	{ NP_WindowPtr, -1 },
	{ NP_Name, (IPTR)ldDemonName },
	{ NP_Priority, 5 },
	{ TAG_END , 0 }
    };

    DOSBase = TaggedOpenLibrary(TAGGEDOPEN_DOS);
    if (!DOSBase) {
	Alert( AN_RAMLib | AG_OpenLib | AO_DOSLib | AT_DeadEnd );
    }

    if ((ldBase->dl_LDDemonPort = CreateMsgPort()) == NULL )
    {
	Alert( AN_RAMLib | AG_NoMemory | AT_DeadEnd );
    }

    FreeSignal(ldBase->dl_LDDemonPort->mp_SigBit);
    ldBase->dl_LDDemonPort->mp_SigBit = SIGBREAKB_CTRL_F;

    ldBase->dl_LDHandler.is_Node.ln_Name = (STRPTR)ldDemonName;
    ldBase->dl_LDHandler.is_Node.ln_Pri = 0;
    ldBase->dl_LDHandler.is_Code = (void (*)())LDFlush;
    ldBase->dl_LDHandler.is_Data = NULL;

    ldBase->dl_DOSBase = DOSBase;

    NEWLIST(&ldBase->dl_LDObjectsList);
    InitSemaphore(&ldBase->dl_LDObjectsListSigSem);

    SysBase->ex_RamLibPrivate = ldBase;

    AddMemHandler(&ldBase->dl_LDHandler);

    /*
     *	Grab the semaphore ourself. The reason for this is that it will
     *	cause all other tasks to wait until we have finished initialising
     *	before they try and open something.
     */
    ObtainSemaphore(&ldBase->dl_LDObjectsListSigSem);

#define SetFunc(offs,ptr) \
    SetFunction(&SysBase->LibNode, (-offs)*(LONG)LIB_VECTSIZE, \
    			AROS_SLIB_ENTRY(ptr,Dos,0))

    /* Do not set the vectors until you have initialised everything else. */
    ldBase->__OpenLibrary = SetFunc(92, OpenLibrary);
    ldBase->__OpenDevice  = SetFunc(74, OpenDevice);
    (void)SetFunc(69, CloseLibrary);
    (void)SetFunc(75, CloseDevice);
    (void)SetFunc(67, RemLibrary);
    (void)SetFunc(73, RemLibrary);

    if( !(ldBase->dl_LDDemonTask = CreateNewProc((struct TagItem *)tags)) )
    {
	Alert( AT_DeadEnd | AN_RAMLib | AG_ProcCreate );
    }

    /* Fix up the MsgPort */

    ldBase->dl_LDDemonPort->mp_SigTask = ldBase->dl_LDDemonTask;

    /* Then unlock the semaphore to allow other processes to run. */
    ReleaseSemaphore(&ldBase->dl_LDObjectsListSigSem);

    return TRUE;
}

ADD2INITLIB(LDDemon_Init, 0)
