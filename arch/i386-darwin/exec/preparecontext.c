/*
 Copyright � 1995-2001, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: i386unix version of PrepareContext().
 Lang: english
 */

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include "etask.h"
#include "exec_util.h"

#include <aros/libcall.h>
#include <proto/arossupport.h>

#ifdef SIZEOF_ALL_REGISTERS
 #warning SIZEOF_ALL_REGISTERS already defined!
#endif
#undef SIZEOF_ALL_REGISTERS

/* bit awkward, until I can include sigcore.h */
#define SIZEOF_ALL_REGISTERS	568

AROS_LH4(BOOL, PrepareContext,
		 AROS_LHA(struct Task *, task, A0),
		 AROS_LHA(APTR, entryPoint, A1),
		 AROS_LHA(APTR, fallBack, A2),
		 AROS_LHA(struct TagItem *, tagList, A3),
		 struct ExecBase *, SysBase, 6, Exec)
{
  AROS_LIBFUNC_INIT

  kprintf("[PrepareContext] preparing task \"%s\" entry: %p fallback: %p\n",task->tc_Node.ln_Name,entryPoint,fallBack);
 
  if (!(task->tc_Flags & TF_ETASK) )
	  return FALSE;
  
  GetIntETask (task)->iet_Context = AllocTaskMem (task
												  , SIZEOF_ALL_REGISTERS
												  , MEMF_PUBLIC|MEMF_CLEAR
												  );
  
  if (!GetIntETask (task)->iet_Context)
	  return FALSE;
  
  
  KRNWireImpl(PrepareContext);
  
  CALLHOOKPKT(krnPrepareContextImpl,task,TAGLIST(TAG_USER,entryPoint,TAG_USER+1,fallBack,TAG_USER+2,tagList,TAG_DONE));
  
  return TRUE;
  
  AROS_LIBFUNC_EXIT
}
