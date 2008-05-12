#ifndef AROS_KERNEL_H
#define AROS_KERNEL_H

#ifndef NATIVE

#include <inttypes.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>
#include <stdio.h>

struct KernelBSS {
    uint64_t    addr;
    uint64_t    len;
};

#else

/* need to define needed structs in a host friendly way */

struct MinNode
{
  struct MinNode  * mln_Succ,
				  * mln_Pred;
};

#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
struct Node
{
  struct Node * ln_Succ,
  * ln_Pred;
  unsigned char	  ln_Type;
  char	  ln_Pri;
  /* AROS: pointer should be 32bit aligned, but we cannot do this on
   the native machine because of binary compatibility.
   */
  char	* ln_Name;
};

#else
struct Node
{
  struct Node * ln_Succ,
  * ln_Pred;
  /* AROS: pointer should be 32bit aligned */
  char	* ln_Name;
  unsigned char	  ln_Type;
  char	  ln_Pri;
};
#endif /* AROS_FLAVOUR */

struct List
{
  struct Node * lh_Head,
  * lh_Tail,
  * lh_TailPred;
  unsigned char	  lh_Type;
  unsigned char	  l_pad;
};

/* Minimal list */
struct MinList
{
  struct MinNode * mlh_Head,
  * mlh_Tail,
  * mlh_TailPred;
};

struct Hook
{
  struct MinNode  h_MinNode;
  void *	  (*h_Entry)();     /* Main entry point, was IPTR */
  void *  (*h_SubEntry)();  /* Secondary entry point, was IPTR */
  void *    h_Data;	    /* Whatever you want, was APTR */
};

struct TagItem{
    unsigned long ti_Tag;
    unsigned long ti_Data;
};

#define TAG_DONE        0x00000000ULL
#define TAG_USER        0x80000000ULL


#endif


/* taglist stuff */

#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)
#define KRN_KernelLowest        (KRN_Dummy + 2)
#define KRN_KernelHighest       (KRN_Dummy + 3)
#define KRN_KernelBss           (KRN_Dummy + 4)
#define KRN_CmdLine             (KRN_Dummy + 5)
#define KRN_MemBase             (KRN_Dummy + 6)
#define KRN_MemSize             (KRN_Dummy + 7)
#define KRN_SysBasePtr          (KRN_Dummy + 8)
#define KRN_ExecHooksPtr        (KRN_Dummy + 9)
#define KRN_KernelHooks         (KRN_Dummy + 10)
#define KRN_ExecExceptionFun	(KRN_Dummy + 11)
#define KRN_ExecDispatchFun	    (KRN_Dummy + 12)


#define KRNH_Dummy              (TAG_USER + 0x03e00000)
#define KRNH_PutcharImpl		(KRNH_Dummy + 1)
#define KRNH_EnableImpl			(KRNH_Dummy + 2)
#define KRNH_DisableImpl		(KRNH_Dummy + 3)
#define KRNH_AlertImpl			(KRNH_Dummy + 4)
#define KRNH_SoftEnableImpl		(KRNH_Dummy + 5)
#define KRNH_SoftDisableImpl	(KRNH_Dummy + 6)
#define KRNH_SoftCauseImpl		(KRNH_Dummy + 7)
#define KRNH_IdleTaskImpl		(KRNH_Dummy + 9)
#define KRNH_PrepareContextImpl	(KRNH_Dummy + 10)
#define KRNH_LoadNativeLibImpl	(KRNH_Dummy + 11)
#define KRNH_UnloadNativeLibImpl	(KRNH_Dummy + 12)
#define KRNH_StartSchedulerImpl	(KRNH_Dummy + 13)

#define EXECH_Dummy              (TAG_USER + 0x03f00000)
#define EXECH_Exception			 (EXECH_Dummy + 1)
#define EXECH_Dispatch			 (EXECH_Dummy + 2)


#define KRNWireImpl(ImplName) \
static struct Hook * krn ## ImplName ## Impl = 0;\
if (krn ## ImplName ## Impl == 0)\
{\
  struct TagItem * listp;\
  APTR KernelBase = OpenResource("kernel.resource"); \
  kprintf("KRNWireImpl got KernelBase = %p\n",KernelBase); \
  krn ## ImplName ## Impl = 0;\
  if (KernelBase) \
  {\
    for (listp = KrnGetHooks(0);\
	    listp != 0 && listp->ti_Tag != TAG_DONE && listp->ti_Tag !=  KRNH_ ## ImplName ## Impl;\
	    ++listp);\
    if (listp->ti_Tag == KRNH_ ## ImplName ## Impl)\
    {\
      krn ## ImplName ## Impl = (struct Hook*) listp->ti_Data;\
    }\
  }\
  kprintf("KRNWireImpl got hook = %p\n",krn ## ImplName ## Impl); \
}


#define BeginHookList(HookList,n) \
{ \
  HookList = (struct TagItem*)malloc((n+1)*sizeof(struct TagItem));\
  struct TagItem * _hooklist = HookList;
  
  #define Add2HookList(TPrefix,HPrefix,HName)\
  {\
	void * HPrefix ## HName(void*,long,long);\
	static struct Hook HPrefix ## _ ## HName ## _hook = { \
	  {0, 0}, \
	  0, HPrefix ## HName, 0 \
	}; \
	_hooklist->ti_Tag = TPrefix ## _ ## HName ## Impl;\
	_hooklist->ti_Data = (unsigned long)& HPrefix ## _ ## HName ## _hook; \
  } \
  ++_hooklist;

  #define EndHookList \
  _hooklist->ti_Tag = TAG_DONE; \
}

//void mykprintf(const char * s);
//void myrkprintf(const char *fmt, ...);

#endif /*AROS_  KERNEL_H*/
