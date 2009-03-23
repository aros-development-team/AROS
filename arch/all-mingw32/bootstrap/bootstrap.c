#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <windows.h>
#include <sys/stat.h>
#include <aros/system.h>

#define __typedef_LONG /* LONG, ULONG, WORD, BYTE and BOOL are declared in Windows headers. Looks like everything  */
#define __typedef_WORD /* is the same except BOOL. It's defined to short on AROS and to int on Windows. This means */
#define __typedef_BYTE /* that you can't use it in OS-native part of the code and can't use any AROS structure     */
#define __typedef_BOOL /* definition that contains BOOL.                                                           */
typedef unsigned AROS_16BIT_TYPE UWORD;
typedef unsigned char UBYTE;

#include <aros/kernel.h>
#include <utility/tagitem.h>

#include "debug.h"
#include "elfloader32.h"
#include "hostlib.h"
#include "shutdown.h"
#include "../kernel/hostinterface.h"

#define D(x) x

static unsigned char __bss_track[32768];
struct TagItem km[64];
char bootstrapdir[MAX_PATH];
char *bootstrapname;
char *cmdline;

typedef int (*kernel_entry_fun_t)(struct TagItem *);

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
    Host_VKPrintF,
    Host_PutChar,
    Host_Shutdown
};

void *SysBase;

int main(int argc, char ** argv)
{
  char *error;
  unsigned long BadSyms;
  struct TagItem *t;
  int x;
  struct stat st;
  char _use_hostmem = 0;
  int i = 1;
  unsigned int memSize = 64;
  char *kernel = "boot\\aros-mingw32";
  char *KernelArgs = NULL;
  void *base;

  GetCurrentDirectory(MAX_PATH, bootstrapdir);
  bootstrapname = argv[0];
  cmdline = GetCommandLine();

  while (i < argc)
  {
      if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
      {
        printf
        (
            "AROS for Windows\n"
            "usage: %s [options] [kernel arguments]\n"
	    "Availible options:\n"
            " -h                 show this page\n"
            " -m <size>          allocate <size> Megabytes of memory for AROS\n"
            "                    (default is 64M)\n"
            " -k <file>          use <file> as a kernel\n"
            "                    (default is boot\\aros-mingw32)\n"
            " --help             same as '-h'\n"
            " --memsize <size>   same as '-m <size>'\n"
            " --kernel <file>    same as '-k'\n"
            " --hostmem          Let AROS use the host operating system's facilities to\n"
            "                    manage memory.\n"
            "\n"
            "Please report bugs to the AROS development team. http://www.aros.org/\n",
            argv[0]
        );
        return 0;
      }
      else if (!strcmp(argv[i], "--memsize") || !strcmp(argv[i], "-m"))
      {
        i++;
        x = 0;
        memSize = 0;
        while ((argv[i])[x] >= '0' && (argv[i])[x] <= '9')
        {
          memSize = memSize * 10 + (argv[i])[x] - '0';
          x++;
        }
        i++;
      }
      else if (!strcmp(argv[i], "--kernel") || !strcmp(argv[i], "-k"))
      {
        kernel = argv[++i];
        i++;
      }
      else if (!strcmp(argv[i], "--hostmem"))
      {
        _use_hostmem = 1;
        i++;
      }
      else
        break;
  }

  D(printf("[Bootstrap] %ld arguments processed\n", i));
  D(printf("[Bootstrap] Raw command line: %s\n", cmdline));
  if (i < argc) {
      KernelArgs = cmdline;
      while(isspace(*KernelArgs++));
      for (x = 0; x < i; x++) {
          while (!isspace(*KernelArgs++));
          while (isspace(*KernelArgs))
          	KernelArgs++;
      }
  }
  D(printf("[Bootstrap] Kernel arguments: %s\n", KernelArgs));
  
  if (!stat("..\\AROS.boot", &st)) {
      chdir("..");
  }

  //load elf-kernel and fill in the bootinfo
  void * file = fopen(kernel, "rb");

  if (!file)
  {
  	printf("[Bootstrap] unable to open kernel \"%s\"\n", kernel);
  	return -1;
  }
  set_base_address(__bss_track, &SysBase);
  i = load_elf_file(file,0);
  fclose(file);
  if (!i) {
      printf("[Bootstrap] Failed to load kernel \"%s\"\n", kernel);
      return -1;
  }
  D(printf("[Bootstrap] allocating working mem: %iMb\n",memSize));

  size_t memlen = memSize << 20;
  void * memory = malloc(memlen);

  if (!memory) {
      printf("[Bootstrap] Failed to allocate RAM!\n");
      return -1;
  }
  D(printf("[Bootstrap] RAM memory allocated: %p-%p (%lu bytes)\n", memory, memory + memlen, memlen));
  
  kernel_entry_fun_t kernel_entry_fun = kernel_lowest();

  //fill in kernel message related to allocated ram regions
  struct TagItem *tag = km;

/* FIXME: These tags should point to a memory map in PC BIOS format, not to memory itself.
   This is a temporary solution because hosted kernel should translate all AllocMem() requests
   to host's allocation requests. In future a full-featured memory map will be implemented in order
   to support loadable modules. */
  tag->ti_Tag = KRN_MMAPAddress;
  tag->ti_Data = (unsigned long)memory;
  tag++;
  
  tag->ti_Tag = KRN_MMAPLength;
  tag->ti_Data = memlen;
  tag++;

  tag->ti_Tag = KRN_KernelLowest;
  tag->ti_Data = kernel_entry_fun;
  tag++;
    
  tag->ti_Tag = KRN_KernelHighest;
  tag->ti_Data = kernel_highest();
  tag++;

  tag->ti_Tag = KRN_KernelBss;
  tag->ti_Data = (unsigned long)__bss_track;
  tag++;

  tag->ti_Tag = KRN_CmdLine;
  tag->ti_Data = KernelArgs;
  tag++;
  
  tag->ti_Tag = KRN_HostInterface;
  tag->ti_Data = &HostIFace;
  tag++;

  tag->ti_Tag = TAG_DONE;

  printf("[Bootstrap] entering kernel@%p...\n",kernel_entry_fun);
  int retval = kernel_entry_fun(km);

  printf("kernel returned %i\n",retval);
}  
