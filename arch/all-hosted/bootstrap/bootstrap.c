#include <ctype.h>
#include <dirent.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <sys/stat.h>

#include <aros/kernel.h>
#include <aros/multiboot.h>
#include <utility/tagitem.h>

#include "debug.h"
#include "elfloader32.h"
#include "support.h"
#include "shutdown.h"

#define D(x)

extern void *HostIFace;

char bootstrapdir[MAX_PATH];
char buf[256];

typedef int (*kernel_entry_fun_t)(struct TagItem *);

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
    {KRN_KernelBss    , (IPTR)__bss_track},
    {KRN_BootLoader   , 0                },
    {KRN_CmdLine      , 0                },
    {KRN_DebugInfo    , 0                },
    {KRN_HostInterface, 0                },
    {KRN_MMAPAddress  , (IPTR)&MemoryMap },
    {KRN_MMAPLength   , sizeof(MemoryMap)},
    {TAG_DONE         , 0                }
};

static char *GetConfigArg(char *str, char *option)
{
    size_t l = strlen(option);

    /* First check option name */
    if (strnicmp(str, option, l))
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
    D(printf("[Init] Allocating %lu bytes for string\n", x));
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

int main(int argc, char ** argv)
{
    struct stat st;
    int i = 1;
    unsigned int memSize = 64;
    int def_memSize = 1;
    char *config = DefaultConfig;
    char *KernelArgs = NULL;
    char *SystemVersion;
    FILE *file;
    kernel_entry_fun_t kernel_addr;
    void *debug_addr;
    size_t kernel_size, debug_size;

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

    /* If AROSBootstrap.exe is found in the current directory, this means the bootstrap
       was started in its own dir. Go one level up in order to reach the root */
    if (!stat("AROSBootstrap.exe", &st))
	chdir("..");

    file = fopen(config, "r");
    if (!file) {
	printf("Failed to load configuration file %s!\n", config);
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

    if (!GetKernelSize(&kernel_size, &debug_size))
	return -1;
    D(printf("[Bootstrap] Kernel size %u, debug information size %u\n", kernel_size, debug_size));

    kernel_addr = malloc(kernel_size + debug_size);
    if (!kernel_addr) {
	printf("Failed to allocate %u bytes for the kernel!\n", kernel_size);
	return -1;
    }

    debug_addr = (void *)kernel_addr + kernel_size;
    D(printf("[Bootstrap] Code 0x%p, debug info 0x%p, End 0x%p\n", kernel_addr, debug_addr, debug_addr + debug_size));
    
    if (!LoadKernel(kernel_addr, debug_addr, __bss_track))
	return -1;

    FreeKernelList();

    D(printf("[Bootstrap] allocating working mem: %iMb\n",memSize));

    size_t memlen = memSize << 20;
    void * memory = malloc(memlen);

    if (!memory) {
	printf("[Bootstrap] Failed to allocate %i Mb of RAM for AROS!\n", memSize);
	return -1;
    }
    D(printf("[Bootstrap] RAM memory allocated: 0x%p - 0x%p (%u bytes)\n", memory, memory + memlen, memlen));

    MemoryMap.addr = (IPTR)memory;
    MemoryMap.len  = memlen;

    km[0].ti_Data = (IPTR)kernel_addr;
    km[1].ti_Data = (IPTR)debug_addr + debug_size - 1;
    km[3].ti_Data = (IPTR)SystemVersion;
    km[4].ti_Data = (IPTR)KernelArgs;
    km[5].ti_Data = (IPTR)debug_addr;
    km[6].ti_Data = (IPTR)HostIFace;

    printf("[Bootstrap] entering kernel@%p...\n", kernel_addr);
    return kernel_addr(km);
}
