#define NATIVE
#include "elfloader32.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <dlfcn.h>
#include "../include/aros/kernel.h"

static unsigned char __bss_track[32768];
struct TagItem km[64];

typedef void  (*init_fun_t)();
typedef struct TagItem *  (*kmessage_fun_t)(struct TagItem *);
typedef int	  (*kernel_entry_fun_t)(struct TagItem *);
typedef struct TagItem *  (*hooklist_fun_t)();

#define BASE_ALIGNMENT 16

int main(int argc, char ** argv)
{
  

  //alloc mem
  unsigned int memSize = 100;  
  printf("[Bootstrap] allocating working mem: %iMb\n",memSize);
  void * memory = malloc((memSize << 20));

  //load native kernel
  printf("[Bootstrap] loading native kernel component...");
  void * kernel_native = dlopen("kernel_native.dylib",RTLD_NOW|RTLD_LOCAL);
  if (kernel_native)
	  printf("ok!\n");
  else {
	  printf("failed!\n");
	  return -1;
  }
	
  printf("[Bootstrap] peeking symbols for native kernel component...");
  init_fun_t kernel_native_init_fun = dlsym(kernel_native,"kernel_init_native");
//  hooklist_fun_t hooklist_fun = (hooklist_fun_t) dlsym(kernel_native,"kernel_get_native_hooks");
  kmessage_fun_t kernel_native_message_fun = (kmessage_fun_t) dlsym(kernel_native,"kernel_message");
  if (kernel_native_init_fun && kernel_native_message_fun)
	  printf("ok!\n");
  else {
	  printf("failed!\n");
	  return -1;
  }
  
  
  //fill in kernel message related to allocated ram regions
  struct TagItem *tag = km;

  tag->ti_Tag = KRN_KernelBss;
  tag->ti_Data = (unsigned long)__bss_track;
  tag++;

  tag->ti_Tag = KRN_MemBase;
  tag->ti_Data = (unsigned long)memory;
  tag++;
  
  tag->ti_Tag = KRN_MemSize;
  tag->ti_Data = (unsigned long)(memSize << 20);
  tag++;
  
  //init native kernel and fill in message
  printf("[Bootstrap] initializing native kernel component...\n");
  kernel_native_init_fun();
  printf("[Bootstrap] getting boot info from native kernel component...\n");
  tag = kernel_native_message_fun(tag);  
  
  //temporarily terminate message so it can be scanned
  tag->ti_Tag = TAG_DONE;
  
  void ** SysBasePtr = 0;
  for (struct TagItem * t = km; t->ti_Tag != TAG_DONE; ++t)
    if (t->ti_Tag == KRN_SysBasePtr)
      SysBasePtr = (void**)t->ti_Data;
      
  if (!SysBasePtr)
  {
    printf("[Bootstrap] could not get SysBasePtr!\n");
    return -1;
  } 
  else
    printf("[Bootstrap] SysBasePtr = %p\n",SysBasePtr);

  //load elf-kernel and finish message
  void * file = fopen(argv[1], "rb");
  size_t ksize=0;
  if (file)
  {
	  fseek(file,0,SEEK_END);
    ksize = ftell(file);
    ksize += BASE_ALIGNMENT - ksize % BASE_ALIGNMENT;
	  printf("[Bootstrap] opened \"%s\"(%p) size:%p\n", argv[1], file, ksize);
	  fseek(file,0,SEEK_SET);
  } else return -1;
  unsigned int bufsize = 10*ksize;
  void * buf = malloc(bufsize);
  void * base = buf + ksize;
  printf("[Bootstrap] memory allocated: %p-%p kernelBase: %p\n",buf,buf+bufsize,base);
  set_base_address(base, __bss_track, SysBasePtr);
  load_elf_file(file,0);
  kernel_entry_fun_t kernel_entry_fun = (kernel_entry_fun_t)base;

  tag->ti_Tag = KRN_KernelLowest;
  tag->ti_Data = (unsigned long)kernel_lowest();
  tag++;
    
  tag->ti_Tag = KRN_KernelHighest;
  tag->ti_Data = (unsigned long)kernel_highest();
  tag++;

  tag->ti_Tag = TAG_DONE;

  printf("[Bootstrap] entering kernel@%p...\n",kernel_entry_fun);
  int retval = kernel_entry_fun(km);

  printf("kernel returned %i\n",retval);
}  
