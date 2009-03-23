#define DEBUG 0

#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <memory.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/kernel.h>

#include <aros/kernel.h>
#include <aros/debug.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "cpucontext.h"
#include "kernel_intern.h"
#include LC_LIBDEFS_FILE

extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern ULONG ** Exec_RomTagScanner(struct ExecBase*,UWORD**);

static struct TagItem *BootMsg;
struct HostInterface *HostIFace;
struct KernelInterface KernelIFace;
APTR KernelBase = NULL;
/* static char cmdLine[200]; TODO */

#undef kprintf
#undef rkprintf
#undef vkprintf

int mykprintf(const UBYTE * fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);
    if (SysBase)
        Forbid();
    r = HostIFace->VKPrintF(fmt, args);
    if (SysBase)
        Permit();
    va_end(args);
    return r;
}

int myvkprintf (const UBYTE *fmt, va_list args)
{
    int res;
    
    if (SysBase)
        Forbid();
    res = HostIFace->VKPrintF(fmt, args);
    if (SysBase)
        Permit();
    return res;
}

int myrkprintf(const STRPTR foo, const STRPTR bar, int baz, const UBYTE * fmt, ...)
{
  va_list args;
  int r;

  va_start(args, fmt);
  if (SysBase)
      Forbid();
  r = HostIFace->VKPrintF(fmt, args);
  if (SysBase)
      Permit();
  va_end(args);
  return r;
}

void __clear_bss(struct TagItem *msg)
{
    struct KernelBSS *bss;

    bss = (struct KernelBSS *)krnGetTagData(KRN_KernelBss, 0, msg);
    
    if (bss)
    {
        while (bss->addr)
        {
		  bzero((void*)bss->addr, bss->len);
            bss++;
        }   
    }
}

AROS_LH0I(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 1, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;
    
    AROS_LIBFUNC_EXIT
}

AROS_LH0I(void *, KrnCreateContext,
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    return AllocMem(sizeof(struct AROSCPUContext), MEMF_PUBLIC|MEMF_CLEAR);

    AROS_LIBFUNC_EXIT
}

AROS_LH1I(void, KrnDeleteContext,
		AROS_LHA(void *, context, A0),
         struct KernelBase *, KernelBase, 10, Kernel)
{
    AROS_LIBFUNC_INIT

    FreeMem(context, sizeof(struct AROSCPUContext));

    AROS_LIBFUNC_EXIT
}

/* auto init */
static int Kernel_Init(LIBBASETYPEPTR kBase)
{
  int i;

  KernelBase = kBase;
  D(mykprintf("[Kernel] init (KernelBase=%p)\n", kBase));
  D(mykprintf("[Kernel] -1 : %p -2 : %p\n", *((APTR*)(((APTR*)kBase)-1)),*((APTR*)(((APTR*)kBase)-2))));
  for (i=0; i < EXCEPTIONS_NUM; i++)
        NEWLIST(&kBase->kb_Exceptions[i]);

  for (i=0; i < INTERRUPTS_NUM; i++)
        NEWLIST(&kBase->kb_Interrupts[i]);
  D(mykprintf("[Kernel] KrnGetBootInfo yields %p\n",Kernel_KrnGetBootInfo()));
  return 1;
}

ADD2INITLIB(Kernel_Init, 0)

char *kernel_functions[] = {
    "core_init",
    "core_intr_disable",
    "core_intr_enable",
    "core_syscall",
    "core_is_super",
    NULL
};

/* rom startup */


//make this the entry point
//int startup(struct TagItem *msg) __attribute__ ((section (".aros.init")));

int __startup startup(struct TagItem *msg)
{
  void *hostlib;
  char *errstr;
  unsigned long badsyms;
  struct MemHeader *mh;

  BootMsg = msg;
  
  void * klo = (void*)krnGetTagData(KRN_KernelLowest, 0, msg);
  void * khi = (void*)krnGetTagData(KRN_KernelHighest, 0, msg);
  void * memory = (void*)krnGetTagData(KRN_MMAPAddress, 0, msg); /* FIXME: These tags are used in non-conforming way */
  unsigned int memsize = krnGetTagData(KRN_MMAPLength, 0, msg);
  HostIFace = (struct HostInterface *)krnGetTagData(KRN_HostInterface, 0, msg);

  hostlib = HostIFace->HostLib_Open("Libs\\Host\\kernel.dll", &errstr);
  if (!hostlib) {
      mykprintf("[Kernel] failed to load host-side module: %s\n", errstr);
      HostIFace->HostLib_FreeErrorStr(errstr);
      return -1;
  }
  badsyms = HostIFace->HostLib_GetInterface(hostlib, kernel_functions, &KernelIFace);
  if (badsyms) {
      mykprintf("[Kernel] failed to resolve %lu symbols\n", badsyms);
      HostIFace->HostLib_Close(hostlib, NULL);
      return -1;
  }

  mykprintf("[Kernel] preparing first mem header\n");

  /* Prepare the first mem header and hand it to PrepareExecBase to take SysBase live */
  mh = memory;
  mh->mh_Node.ln_Type  = NT_MEMORY;
  mh->mh_Node.ln_Name = "chip memory";
  mh->mh_Node.ln_Pri = -5;
  mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC | MEMF_LOCAL | MEMF_24BITDMA | MEMF_KICK;
  mh->mh_First = memory + MEMHEADER_TOTAL;
  mh->mh_First->mc_Next = NULL;
  mh->mh_First->mc_Bytes = memsize - MEMHEADER_TOTAL;
  mh->mh_Lower = memory;
  mh->mh_Upper = memory + memsize;
  mh->mh_Free = mh->mh_First->mc_Bytes;

  mykprintf("[Kernel] calling PrepareExecBase@%p mh_First=%p\n",PrepareExecBase,mh->mh_First);
  /*
   * FIXME: This routine is part of exec.library, however it doesn't have an LVO
   * (it can't have one because exec.library is not initialized yet) and is called
   * only from here. Probably the code should be reorganized
   */
  SysBase = PrepareExecBase(mh);
  mykprintf("[Kernel] SysBase=%p mhFirst=%p\n",SysBase,mh->mh_First);
  
  /* ROM memory header. This special memory header covers all ROM code and data sections
   * so that TypeOfMem() will not return 0 for addresses pointing into the kernel.
   */
  if ((mh = (struct MemHeader *)AllocMem(sizeof(struct MemHeader), MEMF_PUBLIC)))
  {
      mh->mh_Node.ln_Type = NT_MEMORY;
      mh->mh_Node.ln_Name = "rom memory";
      mh->mh_Node.ln_Pri = -128;
      mh->mh_Attributes = MEMF_KICK;
      mh->mh_First = NULL;
      mh->mh_Lower = klo;
      mh->mh_Upper = khi;
      mh->mh_Free = 0;                        /* Never allocate from this chunk! */
      Enqueue(&SysBase->MemList, &mh->mh_Node);
   }

  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;
      
  mykprintf("[Kernel] calling Exec_RomTagScanner@%p\n",Exec_RomTagScanner);
  UWORD * ranges[] = {klo,khi,(UWORD *)~0};
  /*
   * FIXME: Cross-module call again
   */
  SysBase->ResModules = Exec_RomTagScanner(SysBase,ranges);

  mykprintf("[Kernel] initializing host-side kernel module\n");
  if (!KernelIFace.core_init(SysBase->VBlankFrequency, &SysBase, &KernelBase)) {
      mykprintf("[Kernel] Failed to initialize!\n");
      return -1;
  }

  mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
  InitCode(RTF_SINGLETASK, 0);

  mykprintf("leaving startup!\n");
  HostIFace->HostLib_Close(hostlib, NULL);
  return 1;
}


struct TagItem *krnNextTagItem(const struct TagItem **tagListPtr)
{
    if (!(*tagListPtr)) return 0;

    while(1)
    {
        switch((*tagListPtr)->ti_Tag)
        {
            case TAG_MORE:
                if (!((*tagListPtr) = (struct TagItem *)(*tagListPtr)->ti_Data))
                    return NULL;
                continue;
            case TAG_IGNORE:
                break;

            case TAG_END:
                (*tagListPtr) = 0;
                return NULL;

            case TAG_SKIP:
                (*tagListPtr) += (*tagListPtr)->ti_Data + 1;
                continue;

            default:
                return (struct TagItem *)(*tagListPtr)++;

        }

        (*tagListPtr)++;
    }
}

struct TagItem *krnFindTagItem(Tag tagValue, const struct TagItem *tagList)
{
    struct TagItem *tag;
    const struct TagItem *tagptr = tagList;

    while((tag = krnNextTagItem(&tagptr)))
    {
        if (tag->ti_Tag == tagValue)
            return tag;
    }

    return 0;
}

IPTR krnGetTagData(Tag tagValue, intptr_t defaultVal, const struct TagItem *tagList)
{
    struct TagItem *ti = 0;

    if (tagList && (ti = krnFindTagItem(tagValue, tagList)))
        return ti->ti_Data;

        return defaultVal;
}

