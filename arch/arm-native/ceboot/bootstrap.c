/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Native AROS softkicker for Windows CE
    Lang: english
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <hardware/vbe.h>
#include <elfloader.h>
#include <runtime.h>

/*
 * These tricks prevent conflicts between Windows types and exec/types.h
 * WARNING!!! After this WORD and BYTE become unsigned, and BOOL becomes 32-bit!
 * Use with care!
 */
#define __typedef_LONG
#define __typedef_WORD
#define __typedef_BYTE
#define __typedef_BOOL

typedef unsigned short UWORD;
typedef unsigned char  UBYTE;

#include <aros/kernel.h>

#include "bootmem.h"
#include "bootstrap.h"
#include "elf_io.h"
#include "filesystem.h"
#include "hardware.h"
#include "winapi.h"

#define D(x)

char *bootstrapdir;
char buf[BUFFER_SIZE];

static const char version[] = "$VER: CEBoot v1.0 (" ADATE ")";

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

int main(int argc, char **argv)
{
    char *name;
    int i;
    FILE *file;
    unsigned long ro_size, rw_size, bss_size;
    void *ro_addr, *rw_addr, *bss_addr;
    kernel_entry_fun_t kernel_entry;
    struct ELF_ModuleInfo *Debug_KickList;
    unsigned long physbase;
    void *phys_end;
    ULONG_PTR boot_phys, bss_phys, vmode_phys, taglist_phys;
    ULONG_PTR cmd_phys = 0;
    struct vbe_mode *vmode;
    struct TagItem *tag;

    name = namepart(argv[0]);
    i = name - argv[0];
    bootstrapdir = malloc(i + 1);
    if (!bootstrapdir)
    {
        DisplayError("Failed to locate home directory!");
        return -1;
    }
    memcpy(bootstrapdir, argv[0], i);
    bootstrapdir[i] = 0;

    /*
     * Now prepare memory for boot information.
     * Our bootinfo is allocated in a specific way because we need to know both virtual
     * and physical addresses. All pointers inside it must be physical.
     * This will be a single contiguous chunk, so we won't force the kernel to copy boot information
     * once more.
     */
    boot_phys = InitBootMem();
    if (!boot_phys)
    {
        DisplayError("Failed to initialize bootinfo storage");
        return -1;
    }

    file = file_open("CEBoot.conf", "r");
    if (!file)
    {
        DisplayError("Failed to load configuration file!");
        return -1;
    }

    /* Parse the configuration file */
    while (fgets(buf, sizeof(buf), file))
    {
        char *c = strchr(buf, '\r');
      
        if (!c)
            c = strchr(buf, '\n');
        if (c)
            *c = 0;

        c = GetConfigArg(buf, "module");
        if (c)
        {
            AddKernelFile(c);
            continue;
        }
        
        c = GetConfigArg(buf, "logfile");
        if (c)
        {
            i = SetLog(c);
            if (i)
            {
                DisplayError("Failed to redirect debug output to %s", c);
                return -1;
            }
            fprintf(stderr, "----\n"); /* Separation marker */
        }

        c = GetConfigArg(buf, "arguments");
        if (c)
        {
            char *cmd_virt;

            cmd_phys = bootmem_Phys;
            cmd_virt = AllocBootMem(strlen(c) + 1);

            strcpy(cmd_virt, c);
            fprintf(stderr, "Got command line: %s\n", cmd_virt);
        }
    }
    fclose(file);

    if (!GetKernelSize(FirstELF, &ro_size, &rw_size, &bss_size))
        return -1;

    /* Page-align rw_size to assist furure memory protection */
    rw_size = AROS_ROUNDUP2(rw_size, PAGE_SIZE);

    /* Allocate the memory in a single chunk. Read-only region follows read-write. */
    rw_addr = AllocPhysMem(rw_size + ro_size, PAGE_READWRITE, 0, 0, &physbase);
    if (!rw_addr)
    {
        DisplayError("Failed to allocate %u bytes for the kickstart!", rw_size + ro_size);
        return -1;
    }

    bss_phys = bootmem_Phys;
    bss_addr = AllocBootMem(bss_size);

    ro_addr = rw_addr + rw_size;
    D(fprintf(stderr, "[Boot] Logical : Read-write %p, Read-only %p BSS %p\n", rw_addr, ro_addr, bss_addr));
    D(fprintf(stderr, "[Boot] Physical: Read-write %p, Read-only %p BSS %p\n", physbase, physbase + rw_size, bss_phys));

    /* kernel_entry and Debug_KickList are returned as PHYSICAL POINTERS!!! */
    if (!LoadKernel(FirstELF, ro_addr, rw_addr, bss_addr, 4, &phys_end, &kernel_entry, &Debug_KickList))
        return -1;

    FreeKernelList();
    D(fprintf(stderr, "[Boot] Physical: Entry %p, Debug info %p\n", kernel_entry, Debug_KickList));

    /* Get framebuffer information */
    vmode_phys = bootmem_Phys;
    vmode = AllocBootMem(sizeof(struct vbe_mode));

    if (GetFBInfo(vmode))
        return -1;

    /* Remember beginning of the taglist */
    taglist_phys = bootmem_Phys;

    AddTag(KRN_KernelLowest , physbase);
    AddTag(KRN_KernelBase   , physbase + rw_size);
    AddTag(KRN_KernelHighest, (ULONG_PTR)phys_end);
    AddTag(KRN_KernelBss    , bss_phys);
    AddTag(KRN_DebugInfo    , (ULONG_PTR)Debug_KickList);
    if (cmd_phys)
        AddTag(KRN_CmdLine, cmd_phys);

    AddTag(KRN_ProtAreaStart, boot_phys);
    tag = AddTag(KRN_ProtAreaEnd, 0);
    AddTag(TAG_DONE         , 0);

    /* Set the end of protected area */
    tag->ti_Data = bootmem_Phys;
    D(fprintf(stderr, "[Boot] Bootinfo physical %p - %p, taglist %p\n", boot_phys, bootmem_Phys, taglist_phys));

    /*
     * Become a supervisor.
     * SetKMode() returns original state, we use the second call to check result of the first one.
     */
    SetKMode(TRUE);
    if (!SetKMode(TRUE))
    {
        DisplayError("Failed to enter supervisor mode");
        return -1;
    }

    DisplayError("Launching kickstart... Not implemented yet! :)");

    return 0;
}
