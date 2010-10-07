#include <aros/debug.h>
#include <inttypes.h>
#include <aros/symbolsets.h>
#include <exec/lists.h>
#include <memory.h>
#include <exec/resident.h>
#include <exec/memheaderext.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>
#include <proto/arossupport.h>

#include <clib/alib_protos.h>

#include "kernel_intern.h"

void prepare_host_hook(struct Hook * hook)
{
  hook->h_Entry = HookEntry;
}


AROS_LH1(struct TagItem *, KrnGetHooks,
		 
/*  SYNOPSIS */
		 AROS_LHA(UBYTE *, name, A1),
		 
/*  LOCATION */
         struct KernelBase *, KernelBase, 2, Kernel)
{
  AROS_LIBFUNC_INIT
  
  
  struct TagItem * hooks = 0;
  int i;
  
  
  if (name == NULL)
  {
	struct TagItem * msg = KrnGetBootInfo();
	hooks = (struct TagItem *)krnGetTagData(KRN_KernelHooks, 0, msg);
  } else {
	KRNWireImpl(LoadNativeLib);
	hooks = CALLHOOKPKT(krnLoadNativeLibImpl,name,0);
  }

  for (i = 0; hooks[i].ti_Tag != TAG_DONE; ++i)
  {
	  struct Hook* hook = (struct Hook*)hooks[i].ti_Data;
	  prepare_host_hook(hook);
  }
  
  return hooks;

  AROS_LIBFUNC_EXIT
}  

AROS_LH1(struct TagItem *, KrnRelaseHooks,
		 
/*  SYNOPSIS */
		 AROS_LHA(struct TagItem *, handle, A1),
		 
/*  LOCATION */
         struct KernelBase *, KernelBase, 2, Kernel)
{
  AROS_LIBFUNC_INIT
  if (handle != NULL)
  {
	KRNWireImpl(UnloadNativeLib);
	CALLHOOKPKT(krnUnloadNativeLibImpl,handle,0);
  }
  AROS_LIBFUNC_EXIT
}
#if 0



AROS_LH2(void, KrnImportHookListToHost,

/*  SYNOPSIS */
  AROS_LHA(struct TagItem *, hooks, A1),
  AROS_LHA(const char*, name, A2),

/*  LOCATION */
  struct KernelBase *, KernelBase, 20, Kernel)
{
  AROS_LIBFUNC_INIT

    struct TagItem[2] tags;
    
  tags[0].ti_tag = KRN_HookList;
  tags[0].ti_data = (ULONG)hooks;
  tags[1].ti_tag = KRN_HookListName;
  tags[1].ti_data = (ULONG)name;
  
  AROS_LIBFUNC_EXIT
}

#endif