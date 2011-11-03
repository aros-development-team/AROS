#define __KERNEL_NOLIBBASE__

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <aros/kernel.h>

/* Early declaration for ictl functions */
struct KernelBase;

/* These two specify IRQ_COUNT and EXCEPTIONS_COUNT */
#include <kernel_arch.h>
#include <kernel_cpu.h>

/* Platform-specific stuff. Black box here. */
struct PlatformData;

/* kernel.resource base. Nothing spectacular, really. */
struct KernelBase
{
    struct Node            kb_Node;
    struct MinList         kb_Exceptions[EXCEPTIONS_COUNT];
    struct List            kb_Interrupts[IRQ_COUNT];
    ULONG		   kb_ContextFlags;	/* Hints for KrnCreateContext() */
    ULONG		   kb_ContextSize;	/* Total length of CPU context  */
    ULONG		   kb_PageSize;		/* Physical memory page size	*/
    struct PlatformData	  *kb_PlatformData;
};

/*
 * Some useful global variables. They are global because:
 * - BootMsg needs to be stored before KernelBase is allocated;
 * - KernelBase is needed by interrupt handling code
 */
extern struct TagItem *BootMsg;
extern struct KernelBase *KernelBase;

/* Allocation function */
struct KernelBase *AllocKernelBase(struct ExecBase *SysBase);

/* Utility function to clear BSS segments. Call it before storing any globals!!! */
void __clear_bss(const struct KernelBSS *bss);

/* Memory header initialization functions */
struct MemHeader *krnCreateROMHeader(CONST_STRPTR name, APTR start, APTR end);

/*
 * Create MemHeader structure for the specified RAM region.
 * The header will be placed in the beginning of the region itself.
 * The header will NOT be added to the memory list!
 */
static inline void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags)
{
    /* The MemHeader itself does not have to be aligned */
    struct MemHeader *mh = start;

    mh->mh_Node.ln_Succ    = NULL;
    mh->mh_Node.ln_Pred    = NULL;
    mh->mh_Node.ln_Type    = NT_MEMORY;
    mh->mh_Node.ln_Name    = (STRPTR)name;
    mh->mh_Node.ln_Pri     = pri;
    mh->mh_Attributes      = flags;
    /* The first MemChunk needs to be aligned. We do it by adding MEMHEADER_TOTAL. */
    mh->mh_First           = start + MEMHEADER_TOTAL;
    mh->mh_First->mc_Next  = NULL;
    mh->mh_First->mc_Bytes = size - MEMHEADER_TOTAL;

    /*
     * mh_Lower and mh_Upper are informational only. Since our MemHeader resides
     * inside the region it describes, the region includes MemHeader.
     */
    mh->mh_Lower           = start;
    mh->mh_Upper           = start + size;
    mh->mh_Free            = mh->mh_First->mc_Bytes;
}
