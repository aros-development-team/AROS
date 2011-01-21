#define __KERNEL_NOLIBBASE__

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <aros/kernel.h>

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
    struct MinList         kb_Modules;
    dbg_seg_t		  *kb_KernelModules;
    struct SignalSemaphore kb_ModSem;
    unsigned char	   kb_VBlankEnable;
    unsigned int	   kb_VBlankTicks;
    unsigned int	   kb_TimerCount;
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

/* Utility function to clear BSS segments. Call it before storing any globals!!! */
void __clear_bss(const struct KernelBSS *bss);

/* Memory header initialization functions */
void krnCreateMemHeader(CONST_STRPTR name, BYTE pri, APTR start, IPTR size, ULONG flags);
struct MemHeader *krnCreateROMHeader(struct MemHeader *ram, CONST_STRPTR name, APTR start, APTR end);
