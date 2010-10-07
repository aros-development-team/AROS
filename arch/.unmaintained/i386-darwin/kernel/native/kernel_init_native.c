#include "../include/aros/kernel.h"
#include <stdio.h>
#include <stdlib.h>

struct ExecBase;

struct TagItem * kernel_native_hooks = 0;
struct ExecBase * SysBase;

void (*Exec_Exception)(struct ExecBase*);
void (*Exec_Dispatch)(struct ExecBase*);

struct TagItem * kernel_message(struct TagItem *tag)
{
  printf("[Kernel] filling in boot info...\n");
  tag->ti_Tag = KRN_SysBasePtr;
  tag->ti_Data = (unsigned long)&SysBase;
  tag++;
  
  tag->ti_Tag = KRN_ExecExceptionFun;
  tag->ti_Data = (unsigned long)&Exec_Exception;
  tag++;
  
  tag->ti_Tag = KRN_ExecDispatchFun;
  tag->ti_Data = (unsigned long)&Exec_Dispatch;
  tag++;
  
  tag->ti_Tag = KRN_KernelHooks;
  tag->ti_Data = (unsigned long)kernel_native_hooks;
  tag++;  
  
  return tag;
}

void * kernelAlloc (struct Hook * hook, long object, long message)
{
  return malloc((size_t)object);
}

void * kernelFree (struct Hook * hook, long object, long message)
{
  free((void*)object);
  return NULL;
}

void kernel_init_native()
{
  printf("[Kernel] preparing hooks...\n");
  static int initialized = 0;
  if (!initialized) {
	void InitCore();
//	InitCore();
	BeginHookList(kernel_native_hooks,12);
	Add2HookList(KRNH,kernel,Putchar);
	Add2HookList(KRNH,kernel,Disable);
	Add2HookList(KRNH,kernel,Enable);
	Add2HookList(KRNH,kernel,SoftDisable);
	Add2HookList(KRNH,kernel,SoftEnable);
	Add2HookList(KRNH,kernel,SoftCause);
	Add2HookList(KRNH,kernel,Alert);
	Add2HookList(KRNH,kernel,IdleTask);
	Add2HookList(KRNH,kernel,PrepareContext);
	Add2HookList(KRNH,kernel,LoadNativeLib);
	Add2HookList(KRNH,kernel,UnloadNativeLib);
    Add2HookList(KRNH,kernel,StartScheduler);
    Add2HookList(KRNH,kernel,Alloc);
    Add2HookList(KRNH,kernel,Free);
	EndHookList;
  }
  initialized = 1;
}

struct TagItem * kernel_get_native_hooks()
{
  return kernel_native_hooks;
}