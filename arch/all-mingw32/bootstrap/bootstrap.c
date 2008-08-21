#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <windows.h>
#include <aros/system.h>

#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <aros/kernel.h>
#include <utility/tagitem.h>

#include "elfloader32.h"
#include "hostlib.h"

static unsigned char __bss_track[32768];
struct TagItem km[64];

typedef int (*kernel_entry_fun_t)(struct TagItem *, struct HostInterface *);

#define BASE_ALIGNMENT 16

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
struct HostInterface HostIFace = {
    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    Host_HostLib_FreeErrorStr,
    Host_HostLib_GetInterface,
    Host_VKPrintF
};

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

  //fill in kernel message related to allocated ram regions
  struct TagItem *tag = km;

  tag->ti_Tag = KRN_KernelBss;
  tag->ti_Data = (unsigned long)__bss_track;
  tag++;

/* FIXME: These tags should point to a memory map in PC BIOS format, not to memory itself.
   This is a temporary solution because hosted kernel should translate all AllocMem() requests
   to host's allocation requests. In future a full-featured memory map will be implemented in order
   to support loadable modules. */
  tag->ti_Tag = KRN_MMAPAddress;
  tag->ti_Data = (unsigned long)memory;
  tag++;
  
  tag->ti_Tag = KRN_MMAPLength;
  tag->ti_Data = (unsigned long)(memSize << 20);
  tag++;

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

  tag->ti_Tag = KRN_KernelLowest;
  tag->ti_Data = (unsigned long)kernel_lowest();
  tag++;
    
  tag->ti_Tag = KRN_KernelHighest;
  tag->ti_Data = (unsigned long)kernel_highest();
  tag++;

  tag->ti_Tag = TAG_DONE;

  printf("[Bootstrap] entering kernel@%p...\n",kernel_entry_fun);
  int retval = kernel_entry_fun(km, &HostIFace);

  printf("kernel returned %i\n",retval);
}  
