#define DEBUG 1
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

#include "kernel_intern.h"
#include LC_LIBDEFS_FILE

extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern ULONG ** Exec_RomTagScanner(struct ExecBase*,UWORD**);
extern void Exec_Exception();
extern void Exec_Dispatch();

static struct TagItem *BootMsg;
struct HostInterface *HostIFace;
/* static char cmdLine[200]; TODO */

/* So we can examine the memory */
static struct MemHeaderExt mhe;
static struct MemHeader *mh = &mhe.mhe_MemHeader;

#undef kprintf
#undef rkprintf
#undef vkprintf

int mykprintf(const UBYTE * fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);
    r = HostIFace->VKPrintF(fmt, args);
    va_end(args);
    return r;
}

int myvkprintf (const UBYTE *fmt, va_list args)
{
    return HostIFace->VKPrintF(fmt, args);
}

int myrkprintf(const STRPTR foo, const STRPTR bar, int baz, const UBYTE * fmt, ...)
{
  va_list args;
  int r;

  va_start(args, fmt);
  r = HostIFace->VKPrintF(fmt, args);
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

AROS_LH0(struct TagItem *, KrnGetBootInfo,
         struct KernelBase *, KernelBase, 1, Kernel)
{
    AROS_LIBFUNC_INIT

    return BootMsg;
    
    AROS_LIBFUNC_EXIT
}


/* auto init */
static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
  mykprintf("[Kernel] init (KernelBase=%p)\n",LIBBASE);
  mykprintf("[Kernel] -1 : %p -2 : %p\n", *((APTR*)(((APTR*)LIBBASE)-1)),*((APTR*)(((APTR*)LIBBASE)-2)));
  mykprintf("[Kernel] KrnGetBootInfo yields %p\n",Kernel_KrnGetBootInfo(KernelBase));
  return 1;
}

ADD2INITLIB(Kernel_Init, 0)

/* rom startup */


//make this the entry point
int startup(struct TagItem *msg, struct HostInterface *hif) __attribute__ ((section (".aros.init")));
void prepare_host_hook(struct Hook * hook);


int startup(struct TagItem *msg, struct HostInterface *hif)
{
  BootMsg = msg;
  HostIFace = hif;
  
  void * klo = (void*)krnGetTagData(KRN_KernelLowest, 0, msg);
  void * khi = (void*)krnGetTagData(KRN_KernelHighest, 0, msg);
  void * memory = (void*)krnGetTagData(KRN_MMAPAddress, 0, msg); /* FIXME: These tags are used in non-conforming way */
  unsigned int memsize = krnGetTagData(KRN_MMAPLength, 0, msg);

/*
  struct TagItem * KernelHooks = (struct TagItem *)krnGetTagData(KRN_KernelHooks, 0, msg);
  struct Hook * hook;
  int i;
  
  for (i = 0; KernelHooks[i].ti_Tag != TAG_DONE; ++i)
  {
	  struct Hook* hook = (struct Hook*)KernelHooks[i].ti_Data;
	  prepare_host_hook(hook);
	  if (KernelHooks[i].ti_Tag == KRNH_PutcharImpl)
	    PutcharHook = hook;
    else if (KernelHooks[i].ti_Tag == KRNH_StartSchedulerImpl)
      StartSchedulerHook = hook;
  }

  void (**kexecexceptfunp)() = (void*)krnGetTagData(KRN_ExecExceptionFun, 0, msg);
  *kexecexceptfunp = Exec_Exception;
  void (**kdispatchfunp)() = (void*)krnGetTagData(KRN_ExecDispatchFun, 0, msg);
  *kdispatchfunp = Exec_Dispatch;

  mykprintf("[Kernel] got Exec pointers Exception: %p Dispatch: %p\n",Exec_Exception);
*/
  mykprintf("[Kernel] preparing first mem header\n");

  /* Prepare the first mem header and hand it to PrepareExecBase to take SysBase live */
  mh->mh_Node.ln_Name = "chip memory";
  mh->mh_Node.ln_Pri = -5;
  mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC;
  mh->mh_First = (struct MemChunk *)
          (((IPTR)memory + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1));
  mh->mh_First->mc_Next = NULL;
  mh->mh_First->mc_Bytes = memsize;
  mh->mh_Lower = memory;
  mh->mh_Upper = memory + MEMCHUNK_TOTAL + mh->mh_First->mc_Bytes;
  mh->mh_Free = mh->mh_First->mc_Bytes;

  mykprintf("[Kernel] calling PrepareExecBase@%p mh_First=%p\n",PrepareExecBase,mh->mh_First);
  /*
   * FIXME: This routine is part of exec.library, however it doesn't have an LVO
   * (it can't have one because exec.library is not initialized yet) and is called
   * only from here. Probably it should be moved into kernel.resource
   */
  SysBase = PrepareExecBase(mh);
  mykprintf("[Kernel] SysBase=%p mhFirst=%p\n",SysBase,mh->mh_First);

  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;
      
  mykprintf("[Kernel] calling Exec_RomTagScanner@%p\n",Exec_RomTagScanner);
  UWORD * ranges[] = {klo,khi,(UWORD *)~0};
  /*
   * FIXME: Cross-module call again
   */
  SysBase->ResModules = Exec_RomTagScanner(SysBase,ranges);

  mykprintf("[Kernel] starting native scheduler\n");
/*CALLHOOKPKT(StartSchedulerHook,0,0);*/

  mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
  InitCode(RTF_SINGLETASK, 0);

  mykprintf("leaving startup!\n");   
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

