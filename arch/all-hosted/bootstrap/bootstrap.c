#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <utility/tagitem.h>

#include "bootstrap.h"
#include "elfloader32.h"
#include "filesystem.h"
#include "memory.h"
#include "support.h"
#include "shutdown.h"
#include "ui.h"

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

#define D(x)

extern void *HostIFace;

char bootstrapdir[PATH_MAX];
char buf[256];

static struct mb_mmap MemoryMap = {
    sizeof(struct mb_mmap),
    0,
    0,
    0,
    0,
    MMAP_TYPE_RAM
};

static struct KernelBSS __bss_track[256];

/* Kernel message */
static struct TagItem km[] = {
    {KRN_KernelLowest , 0                },
    {KRN_KernelHighest, 0                },
    {KRN_KernelBss    , 0		 },
    {KRN_BootLoader   , 0                },
    {KRN_CmdLine      , 0                },
    {KRN_DebugInfo    , 0                },
    {KRN_HostInterface, 0                },
    {KRN_MMAPAddress  , 0		 },
    {KRN_MMAPLength   , sizeof(MemoryMap)},
    {TAG_DONE         , 0                }
};

static char *GetConfigArg(char *str, char *option)
{
    size_t l = strlen(option);

    /* First check option name */
    if (strncasecmp(str, option, l))
        return NULL;

    /* Skip option name */
    str += l;

    /* First character must be space */
    if (!isspace(*str++))
        return NULL;
    /* Skip the rest of spaces */
    while(isspace(*str))
	str++;

    return str;
}

char *join_string(int argc, char **argv)
{
    char *str, *s;
    int j;
    int x = 0;

    for (j = 0; j < argc; j++)
	x += (strlen(argv[j]) + 1);
    D(printf("[Init] Allocating %u bytes for string\n", x));
    str = malloc(x);
    if (str) {
	s = str;
	for (j = 0; j < argc; j++) {
	    strcpy(s, argv[j]);
	    s += strlen(s);
	    *s++ = ' ';
	}
	s[-1] = 0;
	D(printf("[Init] Joined line: %s\n", str));
    }
    return str;
}

int bootstrap(int argc, char ** argv)
{
    int i = 1;
    unsigned int memSize = 64;
    int def_memSize = 1;
    char *config = DefaultConfig;
    char *KernelArgs = NULL;
    char *SystemVersion;
    FILE *file;
    kernel_entry_fun_t kernel_entry;
    void *debug_addr;
    void *ro_addr, *rw_addr;
    size_t ro_size, rw_size;

    /* This makes national characters to be output properly into
       the debug log under Windows */
    setlocale(LC_ALL, "");
    getcwd(bootstrapdir, sizeof(bootstrapdir));;
    SaveArgs(argv);

    while (i < argc) {
	if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf ("Hosted AROS bootstrap\n"
		    "usage: %s [options] [kernel arguments]\n"
		    "Availible options:\n"
		    " -h                 show this page\n"
		    " -m <size>          allocate <size> Megabytes of memory for AROS\n"
		    "                    (default is 64M)\n"
		    " -c <file>          read configuration from <file>\n"
		    "                    (default is %s)\n"
		    " --help             same as '-h'\n"
		    " --memsize <size>   same as '-m <size>'\n"
		    " --config <file>    same as '-c'\n"
		    "\n"
		    "Please report bugs to the AROS development team. http://www.aros.org/\n", argv[0], DefaultConfig);
            return 0;
        } else if (!strcmp(argv[i], "--memsize") || !strcmp(argv[i], "-m")) {
	    memSize = atoi(argv[++i]);
	    def_memSize = 0;
	    i++;
	} else if (!strcmp(argv[i], "--config") || !strcmp(argv[i], "-c")) {
	    config = argv[++i];
            i++;
        } else
	    break;
    }
    D(printf("[Bootstrap] %u arguments processed\n", i));

    if (i < argc) {
	KernelArgs = join_string(argc - i, &argv[i]);
	D(printf("[Bootstrap] Kernel arguments: %s\n", KernelArgs));
    }

    SystemVersion = getosversion();
    D(printf("[Bootstrap] OS version: %s\n", SystemVersion));

    if (SetRootDirectory())
    {
	DisplayError("Failed to locate root directory!");
	return -1;
    }

    file = fopen(config, "r");
    if (!file) {
	DisplayError("Failed to load configuration file %s!", config);
	return -1;
    }

    /* Parse the configuration file */
    while (fgets(buf, sizeof(buf), file)) {
	char *c = strchr(buf, '\r');
      
	if (!c)
            c = strchr(buf, '\n');
	if (c)
	    *c = 0;

	c = GetConfigArg(buf, "module");
	if (c) {
	    AddKernelFile(c);
	    continue;
	}

	/* Command line argument overrides this */
	if (def_memSize) {
	    c = GetConfigArg(buf, "memory");
	    if (c) {
		memSize = atoi(c);
		continue;
	    }
	}
    }
    fclose(file);

    if (!GetKernelSize(&ro_size, &rw_size))
	return -1;
    D(printf("[Bootstrap] Kernel size %u\n", ro_size));

    ro_addr = AllocateRO(ro_size);
    if (!ro_addr) {
	DisplayError("Failed to allocate %u bytes for the kernel!", ro_size);
	return -1;
    }

    rw_addr = AllocateRW(rw_size);
    if (!rw_addr) {
	DisplayError("Failed to allocate %u bytes for the kernel!", rw_size);
	return -1;
    }

    if (!LoadKernel(ro_addr, rw_addr, __bss_track, &kernel_entry, &debug_addr))
	return -1;
    D(printf("[Bootstrap] Read-only 0x%p - 0x%p, Read-write 0x%p - 0x%p, Entry 0x%p, Debug info 0x%p\n",
	     ro_addr, ro_addr + ro_size - 1, rw_addr, rw_addr + rw_size - 1, kernel_entry, debug_addr));

    FreeKernelList();

    D(printf("[Bootstrap] allocating working mem: %iMb\n",memSize));

    MemoryMap.len = memSize << 20;
    MemoryMap.addr = (IPTR)AllocateRAM(MemoryMap.len);

    if (!MemoryMap.addr) {
	DisplayError("[Bootstrap] Failed to allocate %i Mb of RAM for AROS!\n", memSize);
	return -1;
    }
    D(printf("[Bootstrap] RAM memory allocated: 0x%p - 0x%p (%u bytes)\n", (void *)MemoryMap.addr, (void *)MemoryMap.addr + MemoryMap.len, MemoryMap.len));

    km[0].ti_Data = (IPTR)ro_addr;
    km[1].ti_Data = (IPTR)ro_addr + ro_size - 1;
    km[2].ti_Data = (IPTR)__bss_track;
    km[3].ti_Data = (IPTR)SystemVersion;
    km[4].ti_Data = (IPTR)KernelArgs;
    km[5].ti_Data = (IPTR)debug_addr;
    km[6].ti_Data = (IPTR)HostIFace;
    km[7].ti_Data = (IPTR)&MemoryMap;

    printf("[Bootstrap] entering kernel@%p...\n", kernel_entry);
    i = kernel_entry(km);
    
    DisplayError("Kernel exited with code %d\n", i);
    return i;
}
