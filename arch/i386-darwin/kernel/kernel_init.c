#define DEBUG 1
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <memory.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

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

//make this the entry point
int startup(struct TagItem *msg) __attribute__ ((section (".aros.init")));

void prepare_host_hook(struct Hook * hook);

//exploiting the fact that the kernel is linked in one object
//so we can obtain exec funpointers
extern struct ExecBase * PrepareExecBase(struct MemHeader *);
extern ULONG ** Exec_RomTagScanner(struct ExecBase*,UWORD*);
extern void Exec_Exception();
extern void Exec_Dispatch();

struct Hook * PutcharHook = 0;
struct Hook * StartSchedulerHook = 0;
struct Hook * AllocHook = 0;
struct Hook * FreeHook = 0;

static struct TagItem *BootMsg;
static char cmdLine[200];

/* So we can examine the memory */
static struct MemHeaderExt mhe;
static struct MemHeader *mh = &mhe.mhe_MemHeader;
static struct MemHeaderExt rmhe;
static struct MemHeader *rmh = &rmhe.mhe_MemHeader;
BOOL use_hostmem = FALSE;
size_t memSize = 0;

#undef kprintf
#undef rkprintf
#undef vkprintf

int mykprintf(const UBYTE * fmt, ...)
{
  va_list args;
  int i;
  char str[1024];

  va_start (args, fmt);

  vsprintf (str, fmt, args);

  va_end (args);

  for (i = 0; str[i] != 0; ++i)
    CALLHOOKPKT(PutcharHook,0,str[i]);

  return 0;
}

int myvkprintf (const UBYTE * fmt, va_list args)
{
  UBYTE str[1024];

  vsprintf (str, fmt, args);

  return mykprintf(str);

}

int myrkprintf(const STRPTR foo, const STRPTR bar, int baz, const UBYTE * fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  return myvkprintf(fmt,args);
}

void __clear_bss(struct TagItem *msg)
{
  struct KernelBSS *bss;
  bss = krnGetTagData(KRN_KernelBss, 0, msg);

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
  AROS_LIBFUNC_INIT;

    return BootMsg;

  AROS_LIBFUNC_EXIT;
}


/* auto init */
static int Kernel_Init(LIBBASETYPEPTR LIBBASE)
{
  mykprintf("[Kernel] init (KernelBase=%p)\n",LIBBASE);
  mykprintf("[Kernel] -1 : %p -2 : %p\n", *((APTR*)(((APTR*)LIBBASE)-1)),*((APTR*)(((APTR*)LIBBASE)-2)));
  mykprintf("[Kernel] KrnGetBootInfo yields %p\n",Kernel_KrnGetBootInfo(KernelBase));
  return 1;
}


/* rom startup */



static APTR myAlloc(struct MemHeaderExt *mhe, ULONG size, ULONG *flags)
{
  APTR ret = 0;

  if (AllocHook)
  {
    ret = CALLHOOKPKT(FreeHook,(ULONG)size,0);
  }

  if (ret)
    mhe->mhe_MemHeader.mh_Free -= size;

  return ret;
}

static VOID myFree(struct MemHeaderExt *mhe, APTR mem, ULONG size)
{
  if (FreeHook)
  {
    mhe->mhe_MemHeader.mh_Free += size;
    CALLHOOKPKT(FreeHook,(ULONG)mem,0);
 }
}

static ULONG myAvail(struct MemHeaderExt *mhe, ULONG flags)
{
  if (flags & MEMF_TOTAL)
    return memSize;

  return mhe->mhe_MemHeader.mh_Free;
}

void prepareMemHeader(void * memory, size_t memsize)
{
  memSize = memsize;
  
  /* Prepare the first mem header and hand it to PrepareExecBase to take SysBase live */
  mh->mh_Node.ln_Name = "chip memory";
  mh->mh_Node.ln_Pri = -5;
  mh->mh_Attributes = MEMF_CHIP | MEMF_PUBLIC;

  if (memory == NULL)
  {
    mh->mh_Attributes |= MEMF_MANAGED;
    mh->mh_First       = NULL;
    mh->mh_Lower       = (char *)&_end + 1;
    mh->mh_Upper       = (APTR)(~(IPTR)0 / 2); /* Should use getrlimit here. */
    mh->mh_Free        = memsize;

    ((struct MemHeaderExt *)mh)->mhe_Alloc = myAlloc;
    ((struct MemHeaderExt *)mh)->mhe_Free  = myFree;
    ((struct MemHeaderExt *)mh)->mhe_Avail = myAvail;
  }
  else
  {
    mh->mh_First = (struct MemChunk *)(((IPTR)memory + MEMCHUNK_TOTAL-1) & ~(MEMCHUNK_TOTAL-1));
    mh->mh_First->mc_Next = NULL;
    mh->mh_First->mc_Bytes = memsize;
    mh->mh_Lower = memory;
    mh->mh_Upper = memory + MEMCHUNK_TOTAL + mh->mh_First->mc_Bytes;
    mh->mh_Free = mh->mh_First->mc_Bytes;
  }

}

void prepareRomHeader(APTR klo, APTR khi)
{
  rmh->mh_Node.ln_Type = NT_MEMORY;
  rmh->mh_Node.ln_Name = "rom memory";
  rmh->mh_Node.ln_Pri = -128;
  rmh->mh_Attributes = MEMF_KICK;
  rmh->mh_First = NULL;
  rmh->mh_Lower = klo;
  rmh->mh_Upper = khi;
  rmh->mh_Free = 0;                        /* Never allocate from this chunk! */
  Forbid();
  Enqueue(&SysBase->MemList, &rmh->mh_Node);
}

void prepareHooks(struct TagItem * hooklist)
{
  int i;
  for (i = 0; hooklist[i].ti_Tag != TAG_DONE; ++i)
  {
    struct Hook* hook = (struct Hook*)hooklist[i].ti_Data;
    prepare_host_hook(hook);
    if (hooklist[i].ti_Tag == KRNH_PutcharImpl) 
      PutcharHook = hook;
    else if (hooklist[i].ti_Tag == KRNH_StartSchedulerImpl)
      StartSchedulerHook = hook;
    else if (hooklist[i].ti_Tag == KRNH_AllocImpl)
      AllocHook = hook;
    else if (hooklist[i].ti_Tag == KRNH_FreeImpl)
      FreeHook = hook;
  }
}

int startup(struct TagItem *msg)
{
  BootMsg = msg;  

  //consume boot message
  void * klo = (void*)krnGetTagData(KRN_KernelLowest, 0, msg);
  void * khi = (void*)krnGetTagData(KRN_KernelHighest, 0, msg);
  void * memory = (void*)krnGetTagData(KRN_MemBase, 0, msg);
  unsigned int memsize = krnGetTagData(KRN_MemSize, 0, msg);
  void ** ksysbasep = (void*)krnGetTagData(KRN_SysBasePtr, 0, msg);
  struct TagItem * KernelHooks = (struct TagItem *)krnGetTagData(KRN_KernelHooks, 0, msg);
  void (**kexecexceptfunp)() = (void*)krnGetTagData(KRN_ExecExceptionFun, 0, msg);
  void (**kdispatchfunp)() = (void*)krnGetTagData(KRN_ExecDispatchFun, 0, msg);

  prepareHooks(KernelHooks);

  mykprintf("[Kernel] got Exec pointers Exception: %p Dispatch: %p\n",Exec_Exception);

  mykprintf("[Kernel] preparing first mem header\n",Exec_Exception);

  prepareMemHeader(memory,memsize);

  mykprintf("[Kernel] calling PrepareExecBase@%p mh_First=%p\n",PrepareExecBase,mh->mh_First);

  SysBase = PrepareExecBase(mh);

  mykprintf("[Kernel] SysBase=%p mhFirst=%p\n",SysBase,mh->mh_First);

  *ksysbasep = SysBase;
  //actually at this point we could properly use the library vectors
  *kexecexceptfunp = Exec_Exception;
  *kdispatchfunp = Exec_Dispatch;

  //this makes the rom region known to aros
  prepareRomHeader(klo,khi);

  //this makes kprintf and friends available from everywhere else in aros
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->kprintf = mykprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->rkprintf = myrkprintf;
  ((struct AROSSupportBase *)(SysBase->DebugAROSBase))->vkprintf = myvkprintf;

  mykprintf("[Kernel] calling Exec_RomTagScanner@%p\n",Exec_RomTagScanner);
  UWORD * ranges[] = {klo,khi,(UWORD *)~0};
  SysBase->ResModules = Exec_RomTagScanner(SysBase,ranges);

  mykprintf("[Kernel] starting native scheduler\n");
  CALLHOOKPKT(StartSchedulerHook,0,0);

  mykprintf("[Kernel] calling InitCode(RTF_SINGLETASK,0)\n");
  InitCode(RTF_SINGLETASK, 0);

#if 0



  mykprintf("[Kernel] Booting into kernel.resource...\n");
//    intptr_t addr = krnGetTagData(KRN_KernelBase, 0, msg);
//    intptr_t len = krnGetTagData(KRN_KernelHighest, 0, msg) - addr;


  mykprintf("[Kernel] calling initcode...\n");
  mykprintf("Returned from InitCode()\n");   
#endif

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

ADD2INITLIB(Kernel_Init, 0)
