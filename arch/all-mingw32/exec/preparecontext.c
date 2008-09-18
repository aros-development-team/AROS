/*
 Copyright � 1995-2008, The AROS Development Team. All rights reserved.
 $Id$
 
 Desc: mingw32 version of PrepareContext().
 Lang: english
 */

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include "etask.h"
#include "exec_util.h"
#include "cpucontext.h"

#include <aros/libcall.h>
#include <proto/arossupport.h>

AROS_LH4(BOOL, PrepareContext,
		 AROS_LHA(struct Task *, task, A0),
		 AROS_LHA(APTR, entryPoint, A1),
		 AROS_LHA(APTR, fallBack, A2),
		 AROS_LHA(struct TagItem *, tagList, A3),
		 struct ExecBase *, SysBase, 6, Exec)
{
  AROS_LIBFUNC_INIT
  IPTR args[8] = {0};
  WORD numargs = 0;
  struct AROSCPUContext *ctx;

  D(kprintf("[PrepareContext] preparing task \"%s\" entry: %p fallback: %p\n",task->tc_Node.ln_Name,entryPoint,fallBack));
 
  if (!(task->tc_Flags & TF_ETASK) )
	  return FALSE;
  
  ctx = AllocTaskMem (task, sizeof(struct AROSCPUContext), MEMF_PUBLIC|MEMF_CLEAR);
  GetIntETask (task)->iet_Context = ctx;
  if (!ctx)
	  return FALSE;

  while(tagList)
  {
	switch(tagList->ti_Tag)
	{
	  case TAG_MORE:
		tagList = (struct TagItem *)tagList->ti_Data;
		continue;
		
	  case TAG_SKIP:
		tagList += tagList->ti_Data;
		break;
		
	  case TAG_DONE:
		tagList = NULL;
		break;
		
#define HANDLEARG(x) \
case TASKTAG_ARG ## x: \
args[x - 1] = (IPTR)tagList->ti_Data; \
if (x > numargs) numargs = x; \
break;
		
		HANDLEARG(1)
		HANDLEARG(2)
		HANDLEARG(3)
		HANDLEARG(4)
		HANDLEARG(5)
		HANDLEARG(6)
		HANDLEARG(7)
		HANDLEARG(8)
		
#undef HANDLEARG
	}
	
	if (tagList) tagList++;
  }
  if (numargs)
  {

	/* Assume C function gets all param on stack */
	
	while(numargs--)
	{
	  D(kprintf("  arg %i: %p\n",numargs, args[numargs]));
	  _PUSH(GetSP(task), args[numargs]);
	}
	
  }
  
  /* First we push the return address */
  _PUSH(GetSP(task), fallBack);
  
  /* Then set up the context */
  PREPARE_INITIAL_CONTEXT(ctx, GetSP(task), entryPoint);

  D(kprintf("Prepared task context: *****\n"));
  D(PRINT_CPUCONTEXT(ctx));

  return TRUE;
  
  AROS_LIBFUNC_EXIT
}
