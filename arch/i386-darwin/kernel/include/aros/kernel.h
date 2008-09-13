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


typedef char BYTE;
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef short WORD;
typedef unsigned long ULONG;
typedef unsigned long STACKULONG;
typedef long LONG;
typedef void VOID;
typedef void * APTR;
typedef int * IPTR;
typedef char * STRPTR;
#define FALSE 0
#define TRUE 1

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

#define TAG_DONE   (0L)   /* terminates array of TagItems. ti_Data unused */
#define TAG_END    (0L)   /* synonym for TAG_DONE                         */
#define TAG_IGNORE (1L)   /* ignore this item, not end of array           */
#define TAG_MORE   (2L)   /* ti_Data is pointer to another array of TagItems
			     note that this tag terminates the current array */
#define TAG_SKIP   (3L)   /* skip this and the next ti_Data items         */

/* What separates user tags from system tags */
#define TAG_USER    ((STACKULONG)(1L<<31))
#define TAG_OS	    (16L)   /* The first tag used by the OS */

/* Tag-Offsets for the OS */
#define DOS_TAGBASE	    (TAG_OS)        /* Reserve 16k tags for DOS */
#define INTUITION_TAGBASE   (TAG_OS | 0x2000) /* Reserve 16k tags for Intuition */

/* Tag filter for FilterTagItems() */
#define TAGFILTER_AND 0 	/* exclude everything but filter hits	*/
#define TAGFILTER_NOT 1 	/* exclude only filter hits		*/

/* Mapping types for MapTags() */
#define MAP_REMOVE_NOT_FOUND 0	/* remove tags that aren't in mapList */
#define MAP_KEEP_NOT_FOUND   1	/* keep tags that aren't in mapList   */

/* Macro for syntactic sugar (and a little extra bug-resiliance) */
#define TAGLIST(args...) ((struct TagItem *)(IPTR []){ args, TAG_DONE })

struct MsgPort
{
    struct Node mp_Node;
    UBYTE	mp_Flags;
    UBYTE	mp_SigBit;  /* Signal bit number */
    void      * mp_SigTask; /* Object to be signalled */
    struct List mp_MsgList; /* Linked list of messages */
};

struct Message
{
    struct Node      mn_Node;
    struct MsgPort * mn_ReplyPort;  /* message reply port */
    UWORD	     mn_Length;     /* total message length, in bytes */
				    /* (include the size of the Message
				       structure in the length) */
};

struct Task
{
    struct Node tc_Node;
    UBYTE	tc_Flags;
    UBYTE	tc_State;
    BYTE	tc_IDNestCnt;	/* Interrupt disabled nesting */
    BYTE	tc_TDNestCnt;	/* Task disabled nesting */
    ULONG	tc_SigAlloc;	/* Allocated signals */
    ULONG	tc_SigWait;	/* Signals we are waiting for */
    ULONG	tc_SigRecvd;	/* Received signals */
    ULONG	tc_SigExcept;	/* Signals we will take exceptions for */
    union
    {
	struct
	{
	    UWORD tc_ETrapAlloc;   /* Allocated traps */
	    UWORD tc_ETrapAble;    /* Enabled traps */
	} tc_ETrap;
	APTR	  tc_ETask;	   /* Valid if TF_ETASK is set */
    }		tc_UnionETask;
    APTR	tc_ExceptData;	/* Exception data */
    APTR	tc_ExceptCode;	/* Exception code */
    APTR	tc_TrapData;	/* Trap data */
    APTR	tc_TrapCode;	/* Trap code */
    APTR	tc_SPReg;	/* Stack pointer */
    APTR	tc_SPLower;	/* Stack lower bound */
    APTR	tc_SPUpper;	/* Stack upper bound */
    VOID     (* tc_Switch)();   /* Task loses CPU */
    VOID     (* tc_Launch)();   /* Task gets CPU */
    struct List tc_MemEntry;	/* Allocated memory. Freed by RemTask(). */
    APTR	tc_UserData;	/* For use by the task; no restrictions! */
};

struct ETask
{
    struct Message et_Message;
    APTR	   et_Parent;	    /* Pointer to task */
    ULONG	   et_UniqueID;
    struct MinList et_Children;     /* List of children */
    UWORD	   et_TrapAlloc;
    UWORD	   et_TrapAble;
    ULONG	   et_Result1;	    /* First result */
    APTR	   et_Result2;	    /* Result data pointer (AllocVec) */
    struct MsgPort et_TaskMsgPort;

    /* Internal fields follow */
};

struct IntETask
{
    struct ETask iet_ETask;
#ifdef DEBUG_ETASK
    STRPTR	 iet_Me;
#endif
    APTR	 iet_RT;	/* Structure for resource tracking */
    APTR	 iet_Context;	/* Structure to store CPU registers */
    APTR         iet_acpd;      /* Structure to store shared clib's data */
    APTR	 iet_startup;   /* Structure to store startup code stuff */
};



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
#define KRNH_AllocImpl          (KRNH_Dummy + 14)
#define KRNH_FreeImpl          (KRNH_Dummy + 15)


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
	void * HPrefix ## HName(struct Hook*,long,long);\
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
