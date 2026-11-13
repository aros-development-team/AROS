
#define DEBUG 1

#include <exec/types.h>

#include "VMM_libdefs.h"

#include "defs.h"

void  GM_UNIQUENAME(InitHandler)(struct ExecBase *sysBase)
{
	SysBase = sysBase;

	bug("[VMM-Handler] %s()\n", __func__);

	VM_Manager();
}

static int VMMAROSInitBase(struct VMMHBase *base)
{
#if defined(AROS_USE_LOGRES)
#ifdef LogResBase
#undef LogResBase
#endif
#ifdef LogResHandle
#undef LogResHandle
#endif
    APTR LogResBase;
#define LogHandle (base->hd_LogRHandle)
    base->vmmhb_LogResBase = OpenResource("log.resource");
    if (base->vmmhb_LogResBase)
    {
        LogResBase = base->vmmhb_LogResBase;
        base->vmmhb_LogRHandle = logInitialise(&base->vmmhb_LibNode.lib_Node);
    }
#endif

    return TRUE;
}


/* parthandler.asm */
void PartHandler()
{
}

/* switch_patch.asm */
void SwitchPatch()
{
}

void AddTaskPatch()
{
}

void WaitPatch()
{
}

void RemTaskPatch()
{
}

void StackSwapPatch()
{
}

/* loadseg_patch.asm */
ULONG OrigFuncs [60];
void LoadSegPatch()
{
	
}
void NewLoadSegPatch()
{
	
}

void CrashHandler()
{
	
}

void AlertPatch()
{
	
}

/* dma_patch.asm */
void CachePreDMAPatch()
{
	
}

void CachePostDMAPatch()
{
	
}

void OpenPatch()
{
	
}

/* wb_patch.asm */
void SetWindowTitlesPatch()
{
	
}

/* mem_trace.asm */
ULONG RootTableContents [NUM_PTR_TABLES];

void FreeMemPatch()
{
	
}

void AllocMemPatch()
{
	
}

void AvailMemPatch()
{
	
}

APTR DoOrigAllocMem (IPTR byteSize, ULONG requirements)
{
	APTR allocatedmem = NULL;
	if (OrigAllocMem)
	{
		allocatedmem = AROS_CALL2(APTR, OrigAllocMem,
                AROS_LCA(IPTR,     	byteSize,			D0),
                AROS_LCA(ULONG,	requirements,	D1),
                struct ExecBase *, SysBase);
	}
	else
		allocatedmem = AllocMem(byteSize,requirements);

	return allocatedmem;
}

IPTR DoOrigAvailMem (ULONG requirements)
{
	IPTR availmem = 0;
	if (OrigAvailMem)
	{
		availmem = AROS_CALL1(IPTR, OrigAvailMem,
                AROS_LCA(ULONG,	requirements,	D0),
                struct ExecBase *, SysBase);
	}
	else
		availmem = AvailMem(requirements);

	return availmem;
}

//ADD2INITLIB(VMMAROSInitBase, 10)
