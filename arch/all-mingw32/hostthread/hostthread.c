#define DEBUG 1

#include <aros/debug.h>
#include <aros/asmcall.h>
#include <aros/hostthread.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#include LC_LIBDEFS_FILE

/*
 * WARNING!
 *
 * This module is experimental and not finished. Everything here is
 * subject to change!
 */

/*****i***********************************************************************

    NAME */
#include <proto/hostthread.h>

	AROS_LH2(struct ThreadNode *, HT_CreateNewThread,

/*  SYNOPSIS */
        AROS_LHA(void *, entry, A0),
        AROS_LHA(void *, data, A1),

/*  LOCATION */
        struct HostThreadBase *, HostThreadBase, 1, HostThread)

/*  FUNCTION
	Creates a new host-side thread.

    INPUTS
	entry - Entry point of the thread
	data  - some value to be passed to the thread. Usually a pointer to a structure with some private data

    RESULT
	Thread handle or NULL upon error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	HT_KillThread()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ThreadNode *tn;
    
    tn = AllocMem(sizeof(struct ThreadNode), 0);
    if (tn) {
        if (HostThreadBase->HTIFace->CreateNewThread(entry, tn)) {
            NEWLIST(&tn->intservers);
            ObtainSemaphore(&HostThreadBase->sem);
            AddTail((struct List *)&HostThreadBase->threads_list, (struct Node *)tn);
            ReleaseSemaphore(&HostThreadBase->sem);
            return tn;
        }
        FreeMem(tn, sizeof(struct ThreadNode));
    }
    return NULL;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, HT_KillThread,
         AROS_LHA(struct ThreadNode *, tn, A0),
         struct HostThreadBase *, HostThreadBase, 2, HostThread)
{
    AROS_LIBFUNC_INIT
    
    ULONG res;
    
    res = HostThreadBase->HTIFace->KillThread(tn);
    if (res) {
    	ObtainSemaphore(&HostThreadBase->sem);
    	Remove((struct Node *)tn);
    	ReleaseSemaphore(&HostThreadBase->sem);
    }
    return res;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(ULONG, HT_PutMsg,
    	 AROS_LHA(struct ThreadNode *, tn, A0),
         AROS_LHA(APTR, msg, A1),
         struct HostThreadBase *, HostThreadBase, 3, HostThread)
{
    AROS_LIBFUNC_INIT
        
    return HostThreadBase->HTIFace->PutThreadMsg(tn, msg);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, HT_AddIntServer,
    	AROS_LHA(struct Interrupt *, interrupt, A0),
    	AROS_LHA(struct ThreadNode *, tn, A1),
    	struct HostThreadBase *, HostThreadBase, 4, HostThread)
{
    AROS_LIBFUNC_INIT
        
    Disable();
    Enqueue(&tn->intservers, (struct Node *)interrupt);
    Enable();
        
    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, HT_RemIntServer,
    	AROS_LHA(struct Interrupt *, interrupt, A0),
    	struct HostThreadBase *, HostThreadBase, 5, HostThread)
{
    AROS_LIBFUNC_INIT
        
    Disable();
    Remove((struct Node *)interrupt);
    Enable();
        
    AROS_LIBFUNC_EXIT
}

void IRQ0Handler(struct ThreadNode *tn, void *data)
{
    struct Interrupt * irq;

    ForeachNode(&tn->intservers, irq)
    {
	if( AROS_UFC3(int, irq->is_Code,
		AROS_UFCA(void *, data, A0),
		AROS_UFCA(APTR, irq->is_Data, A1),
		AROS_UFCA(APTR, irq->is_Code, A5)
	))
	    break;
    }
}

const char *Symbols[] = {
    "GetMsg",
    "CauseInterrupt",
    "CreateNewThread",
    "KillThread",
    "PutThreadMsg",
    NULL
};

/* auto init */
static int HostThread_Init(LIBBASETYPEPTR HostThreadBase)
{
    APTR HostLibBase;
    APTR KernelBase;
    ULONG r;

    HostLibBase = OpenResource("hostlib.resource");
    D(bug("[hostthread] HostLibBase = 0x%08lX\n", HostLibBase));
    if (HostLibBase) {
        HostThreadBase->HTLib = HostLib_Open("Libs\\Host\\hostthread.dll", NULL);
        if (HostThreadBase->HTLib) {
    	    HostThreadBase->HTIFace = (struct MyHTInterface *)HostLib_GetInterface(HostThreadBase->HTLib, Symbols, &r);
    	    D(bug("[hostthread] HTIFace = 0x%08lX\n", HostThreadBase->HTIFace));
    	    if (HostThreadBase->HTIFace) {
    	        if (!r) {
    	            KernelBase = OpenResource("kernel.resource");
    	            if (KernelBase) {
    	                D(bug("[hostthread] KernelBase = 0x%08lX\n", KernelBase));
    	                if (KrnAddIRQHandler(0, IRQ0Handler, NULL, NULL)) {
    	                    D(bug("[hostthread] IRQ 0 handler installed\n"));
    	            	    NEWLIST((struct List *)&HostThreadBase->threads_list);
    	            	    InitSemaphore(&HostThreadBase->sem);
    	            	    return 1;
    	            	}
    	            }
    	        }
    	        HostLib_DropInterface((APTR)HostThreadBase->HTIFace);
    	    }
    	    HostLib_Close(HostThreadBase->HTLib, NULL);
    	}
    }
    return 0;
}

ADD2INITLIB(HostThread_Init, 0)
