#define NATIVE
#include "elfloader32.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <windows.h>
#include "hostlib.h"
#include "../include/aros/kernel.h"
#include "../kernel/kernel_intern.h"

static unsigned char __bss_track[32768];
struct TagItem km[64];

typedef void  (*init_fun_t)();
typedef struct TagItem *  (*kmessage_fun_t)(struct TagItem *);
typedef int	  (*kernel_entry_fun_t)(struct TagItem *);
typedef struct TagItem *  (*hooklist_fun_t)();

#define BASE_ALIGNMENT 16

const char *kernel_names[] = {
    /* TODO */
    NULL
};

struct KernelInterface KernelIFace = {
    /* TODO */
    Kern_HostLib_Open,
    Kern_HostLib_Close,
    Kern_HostLib_GetPointer,
    Kern_HostLib_FreeErrorStr,
    Kern_HostLib_GetInterface
};

int main(int argc, char ** argv)
{
  char *error;
  unsigned long BadSyms;
  struct TagItem *t;

  //alloc mem
  unsigned int memSize = 100;  
  printf("[Bootstrap] allocating working mem: %iMb\n",memSize);
  void * memory = malloc((memSize << 20));

  //load native kernel
  printf("[Bootstrap] loading native kernel component... ");
  HMODULE kernel_native = Kern_HostLib_Open("kernel_native.dll", &error);
  if (kernel_native)
	  printf("ok!\n");
  else {
      	  CharToOem(error, error);
	  printf("%s\n", error);
	  Kern_HostLib_FreeErrorStr(error);
	  return -1;
  }
	
  printf("[Bootstrap] peeking symbols for native kernel component... ");
  BadSyms = Kern_HostLib_GetInterface(kernel_native, kernel_names, &KernelIFace);
  if (!BadSyms)
	  printf("ok!\n");
  else {
	  printf("failed (%lu unresolved symbols)!\n", BadSyms);
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
  printf("[Bootstrap] getting boot info from native kernel component...\n");
//  tag = kernel_native_message_fun(tag);  
  
  //temporarily terminate message so it can be scanned
  tag->ti_Tag = TAG_DONE;
  
  void ** SysBasePtr = 0;
  for (t = km; t->ti_Tag != TAG_DONE; ++t)
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
