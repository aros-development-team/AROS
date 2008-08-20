#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <windows.h>
#include <aros/hostboot.h>

#include "elfloader32.h"
#include "hostlib.h"
#include "../hostlib/hostlib_intern.h"

static unsigned char __bss_track[32768];

typedef int (*kernel_entry_fun_t)(struct HostBootInfo *);

#define BASE_ALIGNMENT 16

struct HostLibInterface HostLibIFace = {
    Kern_HostLib_Open,
    Kern_HostLib_Close,
    Kern_HostLib_GetPointer,
    Kern_HostLib_FreeErrorStr,
    Kern_HostLib_GetInterface
};

struct HostBootInfo bootinfo;
void *SysBase;

int main(int argc, char ** argv)
{
  char *error;
  unsigned long BadSyms;
  struct TagItem *t;
  char *kernel = "kernel";

  if (argc > 1)
      kernel = argv[1];

  //alloc mem
  unsigned int memSize = 100;  
  printf("[Bootstrap] allocating working mem: %iMb\n",memSize);
  void * memory = malloc((memSize << 20));

  //load elf-kernel and fill in the bootinfo
  void * file = fopen(argv[1], "rb");
  size_t ksize=0;
  if (file)
  {
	  fseek(file,0,SEEK_END);
    ksize = ftell(file);
    ksize += BASE_ALIGNMENT - ksize % BASE_ALIGNMENT;
	  printf("[Bootstrap] opened \"%s\"(%p) size:%p\n", kernel, file, ksize);
	  fseek(file,0,SEEK_SET);
  } else {
  	printf("[Bootstrap] unable to open kernel \"%s\"\n", kernel);
  	return -1;
  }
  unsigned int bufsize = 10*ksize;
  void * buf = malloc(bufsize);
  void * base = buf + ksize;
  printf("[Bootstrap] memory allocated: %p-%p kernelBase: %p\n",buf,buf+bufsize,base);
  set_base_address(base, __bss_track, &SysBase);
  load_elf_file(file,0);
  kernel_entry_fun_t kernel_entry_fun = (kernel_entry_fun_t)base;

  bootinfo.MemBase = memory;
  bootinfo.MemSize = memSize << 20;
  bootinfo.KernelBss = __bss_track;
  bootinfo.KernelLowest = kernel_lowest();
  bootinfo.KernelHighest = kernel_highest();
  bootinfo.HostLib_Interface = &HostLibIFace;

  printf("[Bootstrap] entering kernel@%p...\n",kernel_entry_fun);
  int retval = kernel_entry_fun(&bootinfo);

  printf("kernel returned %i\n",retval);
}  
