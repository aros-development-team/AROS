/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$

    Loader for shared libraries and devices.
*/

#include <aros/config.h>

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
#include <proto/execlock.h>
#include <proto/dos.h>

#include <proto/lddemon.h>

#include <resources/execlock.h>
#include <aros/types/spinlock_s.h>

#include <stddef.h>
#include <string.h>

#include "lddemon.h"

#ifdef __mc68000
#define INIT_IN_LDDEMON_CONTEXT 1
#else
#define INIT_IN_LDDEMON_CONTEXT 0
#endif

#define CHECK_DEPENDENCY 1

/* Please leave them here! They are needed on Linux-M68K */
AROS_LD2(struct Library *, OpenLibrary,
    AROS_LDA(STRPTR, libname, A1),
    AROS_LDA(ULONG, version, D0),
    struct ExecBase *, SysBase, 0, Lddemon);
AROS_LD4(LONG, OpenDevice,
    AROS_LDA(STRPTR, devname, A0),
    AROS_LDA(IPTR, unitNumber, D0),
    AROS_LDA(struct IORequest *, iORequest, A1),
    AROS_LDA(ULONG, flags, D1),
    struct ExecBase *, SysBase, 0, Lddemon);
AROS_LD1(void, CloseLibrary,
    AROS_LDA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Lddemon);
AROS_LD1(void, CloseDevice,
    AROS_LDA(struct IORequest *, iORequest, A1),
    struct ExecBase *, SysBase, 0, Lddemon);
AROS_LD1(void, RemLibrary,
    AROS_LDA(struct Library *, library, A1),
    struct ExecBase *, SysBase, 0, Lddemon);

struct LDDMsg
{
    struct Message 	 ldd_Msg;	    /* Message link */
    struct MsgPort	 ldd_ReplyPort;	    /* Callers ReplyPort */

    STRPTR		 ldd_Name;	    /* Name of thing to load */

    STRPTR		 ldd_BaseDir;	    /* Base directory to load from */
#if INIT_IN_LDDEMON_CONTEXT
    struct Library	 *ldd_Return;	    /* Loaded and initialized Library */
    struct List		 *ldd_List;	    /* List to add */
#else
    BPTR		 ldd_Return;	    /* Loaded seglist */
#endif
};

static const char ldDemonName[] = "Lib & Dev Loader Daemon";

/*
    overridable LoadSeg
*/
AROS_LH1(BPTR, LDLoadSeg,
        AROS_LHA(CONST_STRPTR, name, D1),
        struct IntLDDemonBase *, ldBase, 1, Lddemon)
{
    AROS_LIBFUNC_INIT
    struct Library *DOSBase = ldBase->dl_DOSBase;
    D(bug("[LDLoadSeg] name=%s\n", name));
    return LoadSeg(name);
    AROS_LIBFUNC_EXIT
}

/*
  BPTR LDLoad( caller, name, basedir, DOSBase )
    Try and load a segment from disk for the object <name>, relative
    to directory <basedir>. Will also try <caller>'s current and home
    directories.
*/
static BPTR LDLoad(struct IntLDDemonBase *ldBase, struct Process *caller, STRPTR name, STRPTR basedir, struct ExecBase *SysBase)
{
    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR seglist = BNULL;
    STRPTR path;
    ULONG pathLen;
    int delimPos;

    /*
        If the caller was a process, we have more scope for loading
        libraries. We can load them from the caller's current directory,
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
                seglist = LDLoadSeg(name);
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
            seglist = LDLoadSeg(name);
            if ((!seglist) && path)
                seglist = LDLoadSeg(path);

            /* The the program directory of the caller */
            if((!seglist) && (caller->pr_HomeDir != BNULL))
            {
                D(bug("[LDLoad] Trying homedir\n"));
                me->pr_CurrentDir = caller->pr_HomeDir;
                seglist = LDLoadSeg(name);
                if ((!seglist) && path)
                    seglist = LDLoadSeg(path);
            }
        }

        if (path) {
            if (!seglist) {
                /* Nup, let's try the default directory as supplied. */
                D(bug("[LDLoad] Trying defaultdir\n"));
                path[delimPos] = ':';
                seglist = LDLoadSeg(path);
            }
            FreeMem(path, pathLen);
        }
    } else
        seglist = LDLoadSeg(name);

    return seglist;
}

/*
  Library *LDInit(seglist, DOSBase)
    Initialise the library.
*/
static struct Library *LDInit(BPTR seglist, struct List *list, STRPTR resname, struct ExecBase *SysBase)
{
    struct Node *node = NULL;
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
#ifdef __mc68000
    /* If struct Resident was not found, just run the code. SegList in A0.
     * Required to load WB1.x devs:narrator.device. */
    Forbid();
    AROS_UFC1NR(void, BADDR(seglist) + sizeof(ULONG), AROS_UFCA(BPTR, seglist, A0));
    node = FindName(list, resname);
    Permit();
    D(bug("[LDInit] Done direct calling %s, seg %p, node %p\n", resname, BADDR(seglist), node));
#endif
    return (struct Library*)node;
}

static struct Library *CallLDInit(BPTR seglist, struct List *list, STRPTR resname, struct Library *DOSBase, struct ExecBase *SysBase)
{
    struct Library *tmplib;
    if (!seglist)
        return NULL;
    tmplib = LDInit(seglist, list, resname, SysBase);
    if (!tmplib)
        UnLoadSeg(seglist);
    return tmplib;
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

static void ProcessLDMessage(struct IntLDDemonBase *ldBase, struct LDDMsg *ldd, struct ExecBase *SysBase);

static struct LDObjectNode *LDRequestObject(STRPTR libname, ULONG version, STRPTR dir, struct List *list, struct ExecBase *SysBase)
{
    /*  We use FilePart() because the liblist is built from resident IDs,
        and contain no path. Eg. The user can request gadgets/foo.gadget,
        but the resident only contains foo.gadget
    */
    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
#if defined(__AROSEXEC_SMP__)
    void *ExecLockBase = ldBase->dl_ExecLockRes;
#endif    
    struct Library *DOSBase = ldBase->dl_DOSBase;
    STRPTR stripped_libname = FilePart(libname);
    struct Library *tmplib;
    struct LDObjectNode *object;

    D(bug("[LDDemon] %s()\n", __func__));

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
#if defined(__AROSEXEC_SMP__)
    ObtainSystemLock(list, SPINLOCK_MODE_READ, LOCKF_FORBID);
#endif
    tmplib = (struct Library *)FindName(list, stripped_libname);
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(list, LOCKF_FORBID);
#endif

    if (!tmplib)
    {
        /* Try to load from disk if not found */
        struct LDDMsg ldd;
        bzero(&ldd, sizeof(ldd));

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
#if INIT_IN_LDDEMON_CONTEXT
        ldd.ldd_List    = list;
#endif

        D(bug("[LDCaller] Sending request for %s, InLDProcess %d\n",
            stripped_libname, (struct Process*)FindTask(NULL) == ldBase->dl_LDDemonTask));

#if INIT_IN_LDDEMON_CONTEXT
        /* Direct call if already in LDDemon context */
        if ((struct Process*)FindTask(NULL) == ldBase->dl_LDDemonTask) {
            ProcessLDMessage(ldBase, &ldd, SysBase);
        } else
#endif
        {
            SetSignal(0, SIGF_SINGLE);
            PutMsg(ldBase->dl_LDDemonPort, (struct Message *)&ldd);
            WaitPort(&ldd.ldd_ReplyPort);
        }

        D(bug("[LDCaller] Returned 0x%p\n", ldd.ldd_Return));

#if INIT_IN_LDDEMON_CONTEXT
        tmplib = ldd.ldd_Return;
#else
        tmplib = CallLDInit(ldd.ldd_Return, list, stripped_libname, DOSBase, SysBase);
#endif
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
    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;

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
    struct ExecBase *, SysBase, 0, Lddemon)
{
    AROS_LIBFUNC_INIT

    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *library;
    struct LDObjectNode *object;

    D(bug("[LDDemon] %s()\n", __func__));

    object = LDRequestObject(libname, version, "libs", &SysBase->LibList, SysBase);

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
    struct ExecBase *, SysBase, 0, Lddemon)
{
    AROS_LIBFUNC_INIT

    struct LDObjectNode *object;
    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;

    D(bug("[LDDemon] %s()\n", __func__));

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
    struct ExecBase *, SysBase, 0, Lddemon)
{
    AROS_LIBFUNC_INIT

    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist;

    D(bug("[LDDemon] %s()\n", __func__));

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
    struct ExecBase *, SysBase, 0, Lddemon)
{
    AROS_LIBFUNC_INIT
    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist = BNULL;

    D(bug("[LDDemon] %s()\n", __func__));

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
    struct ExecBase *, SysBase, 0, Lddemon)
{
    AROS_LIBFUNC_INIT

    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct Library *DOSBase = ldBase->dl_DOSBase;
    BPTR seglist;

    D(bug("[LDDemon] %s()\n", __func__));

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

    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
#if defined(__AROSEXEC_SMP__)
    void *ExecLockBase = ldBase->dl_ExecLockRes;
#endif
    struct Library *library;

    D(bug("[LDDemon] %s()\n", __func__));
    ldBase->dl_LDReturn = MEM_DID_NOTHING;

    /* Forbid() is already done, but I don't want to rely on it. */
    Forbid();
#if defined(__AROSEXEC_SMP__)
    ObtainSystemLock(&SysBase->LibList, SPINLOCK_MODE_WRITE, 0);
#endif

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
#if defined(__AROSEXEC_SMP__)
                ReleaseSystemLock(&SysBase->LibList, 0);
#endif
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
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(&SysBase->LibList, 0);

    ObtainSystemLock(&SysBase->DeviceList, SPINLOCK_MODE_WRITE, 0);
#endif
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
#if defined(__AROSEXEC_SMP__)
            ReleaseSystemLock(&SysBase->DeviceList, 0);
#endif
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
#if defined(__AROSEXEC_SMP__)
    ReleaseSystemLock(&SysBase->DeviceList, 0);
#endif
    Permit();

    return MEM_DID_NOTHING;

    AROS_USERFUNC_EXIT
}

static void ProcessLDMessage(struct IntLDDemonBase *ldBase, struct LDDMsg *ldd, struct ExecBase *SysBase)
{
#if INIT_IN_LDDEMON_CONTEXT
    struct Library *DOSBase = ldBase->dl_DOSBase;
#endif
    BPTR seglist;
    D(bug("[LDDemon] Got a request for %s in %s\n", ldd->ldd_Name, ldd->ldd_BaseDir));

    seglist = LDLoad(ldBase, ldd->ldd_ReplyPort.mp_SigTask, ldd->ldd_Name, ldd->ldd_BaseDir, SysBase);

#if INIT_IN_LDDEMON_CONTEXT
    ldd->ldd_Return = CallLDInit(seglist, ldd->ldd_List, FilePart(ldd->ldd_Name), ldBase->dl_DOSBase, SysBase);
    D(bug("[LDDemon] Replying with %p as result, seglist was %p\n", ldd->ldd_Return, seglist));
#else
    ldd->ldd_Return = seglist;
    D(bug("[LDDemon] Replying with %p as result\n", ldd->ldd_Return));
#endif
}

/*
  void LDDemon()
    The LDDemon process entry. Sits around and does nothing until a
    request for a library comes, when it will then find the library
    and hopefully open it.
*/
static AROS_PROCH(LDDemon, argptr, argsize, SysBase)
{
    AROS_PROCFUNC_INIT

    struct IntLDDemonBase *ldBase = SysBase->ex_RamLibPrivate;
    struct LDDMsg *ldd;

    for(;;)
    {
        WaitPort(ldBase->dl_LDDemonPort);
        while( (ldd = (struct LDDMsg *)GetMsg(ldBase->dl_LDDemonPort)) )
        {
            ProcessLDMessage(ldBase, ldd, SysBase);
            ReplyMsg((struct Message *)ldd);
        } /* messages available */
    }

    /* lddemon died */
    return 0;

    AROS_PROCFUNC_EXIT
}

static ULONG LDDemon_Init(struct IntLDDemonBase *ldBase)
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

    NEWLIST(&ldBase->dl_Flavours);

    FreeSignal(ldBase->dl_LDDemonPort->mp_SigBit);
    ldBase->dl_LDDemonPort->mp_SigBit = SIGBREAKB_CTRL_F;

    ldBase->dl_LDHandler.is_Node.ln_Name = (STRPTR)ldDemonName;
    ldBase->dl_LDHandler.is_Node.ln_Pri = 0;
    ldBase->dl_LDHandler.is_Code = (VOID_FUNC)LDFlush;
    ldBase->dl_LDHandler.is_Data = NULL;

    ldBase->dl_DOSBase = DOSBase;

    NEWLIST(&ldBase->dl_LDObjectsList);
    InitSemaphore(&ldBase->dl_LDObjectsListSigSem);

    SysBase->ex_RamLibPrivate = ldBase;

    AddMemHandler(&ldBase->dl_LDHandler);

#if defined(__AROSEXEC_SMP__)
    ldBase->dl_ExecLockRes = OpenResource("execlock.resource");
#endif

    /*
     *	Grab the semaphore ourself. The reason for this is that it will
     *	cause all other tasks to wait until we have finished initialising
     *	before they try and open something.
     */
    ObtainSemaphore(&ldBase->dl_LDObjectsListSigSem);

#define SetFunc(offs,ptr) \
    SetFunction(&SysBase->LibNode, (-offs)*(LONG)LIB_VECTSIZE, \
                        AROS_SLIB_ENTRY(ptr,Lddemon,0))

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
