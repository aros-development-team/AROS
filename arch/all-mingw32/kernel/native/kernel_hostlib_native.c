#include <aros/kernel.h>
#include <windows.h>
#include <stdio.h>

typedef void (*init_fun_t)();
typedef struct TagItem* (*hooklist_fun_t)();

struct TagItem * kernelLoadNativeLib(struct Hook * hook, unsigned long object, unsigned long message)
{
  struct TagItem * hooks = 0;
  const char * name = (const char *) object;
  const char buf[1024];
  sprintf(buf,"%s_native.dll",name);

  printf("[Kernel] loading native lib: %s\n",buf);
  
  HMODULE dlhandle = LoadLibrary(buf);
  
  if (dlhandle)
  {
    printf("[Kernel] got dlhandle @%p\n",dlhandle);
    sprintf(buf,"%s_init_native",name);
    init_fun_t init_fun = (init_fun_t)GetProcAddress(dlhandle,buf);
    printf("[Kernel] got symbol %s@%p\n",buf,init_fun);
    if (init_fun) init_fun(); /* not required */
    sprintf(buf,"%s_get_native_hooks",name);
    hooklist_fun_t hooklist_fun = (hooklist_fun_t) GetProcAddress(dlhandle,buf);
    printf("[Kernel] got symbol %s@%p\n",buf,hooklist_fun);
    hooks = hooklist_fun ? hooklist_fun() : 0;
    if (hooks)
    {
	    int i;
	    for (i = 0; hooks[i].ti_Tag != TAG_DONE; ++i);
	    hooks[i].ti_Data = dlhandle;
      printf("[Kernel] got %i hooks\n",i);
    }
  }
  
  if (dlhandle != 0 && hooks == 0)
    FreeLibrary(dlhandle);
  
  return hooks;
}

struct TagItem * kernelUnloadNativeLib(struct Hook * hook, unsigned long object, unsigned long message)
{
  struct TagItem * hooklist = (struct TagItem *) object;
  while (hooklist->ti_Tag != TAG_DONE) ++hooklist;
  FreeLibrary(hooklist->ti_Data);
}
