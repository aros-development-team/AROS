/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: FindAddr CLI command
*/

#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>

#include <dos/dos.h>
#include <libraries/debug.h>

#include <stdlib.h>

const TEXT version[] = "$VER: FindAddr 40.0 (31.5.2025)\n";

#define ARG_TEMPLATE "VERBOSE/S,ADDR/A"

enum
{
    ARG_VERBOSE = 0,
    ARG_ADDR,
    NOOFARGS
};

int __nocommandline = 1;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR)0, (IPTR)0 };
    struct RDArgs *rda;
    struct TagItem *bootMsg;
    struct ELF_ModuleInfo *kmod;
	APTR KernelBase;

    LONG return_code = RETURN_OK;
    LONG error = 0;

    KernelBase = OpenResource("kernel.resource");
    if (!KernelBase)
            return FALSE;

    rda = ReadArgs(ARG_TEMPLATE, args, NULL);
    if (rda != NULL)
    {
		BOOL found = FALSE;
		APTR addr = NULL;
		char *endptr;

#if (__WORDSIZE==64)
		addr = (APTR)strtoull((STRPTR)args[ARG_ADDR], &endptr, 0);
		if (args[ARG_VERBOSE])
			Printf("Address %s (0x%llx)\n", (STRPTR)args[ARG_ADDR], addr);
#else
		addr = (APTR)strtoul((STRPTR)args[ARG_ADDR], &endptr, 0);
		if (args[ARG_VERBOSE])
			Printf("Address %s (0x%lx)\n", (STRPTR)args[ARG_ADDR], addr);
#endif
		bootMsg = KrnGetBootInfo();
		kmod = (struct ELF_ModuleInfo *)GetTagData(KRN_DebugInfo, 0, bootMsg);

		for (; !found && kmod; kmod = kmod->Next)
		{
			struct sheader *sections = (struct sheader *)kmod->sh;
			ULONG shstr;
			if (kmod->eh->shstrndx == SHN_XINDEX)
					shstr = sections[0].link;
			else
				shstr = kmod->eh->shstrndx;

			for (int i=0; i < kmod->eh->shnum; i++)
			{
				if ((sections[i].size) && (sections[i].flags & SHF_ALLOC))
				{
					if (addr  >= sections[i].addr && addr <= (sections[i].addr + sections[i].size - 1))
					{
						if (args[ARG_VERBOSE])
						{
							Printf("Found in module %s\n", kmod->Name);
#if (__WORDSIZE==64)
							Printf(" * 0x%llx - 0x%llx\n", sections[i].addr, (sections[i].addr + sections[i].size - 1));
#else
							Printf(" * 0x%lx - 0x%lx\n", sections[i].addr, (sections[i].addr + sections[i].size - 1));
#endif
						}
						else
						{
							Printf("%s\n", kmod->Name);
						}
						found = TRUE;
						break;
					}
				}
			}
		}
		if ((!found)&&(args[ARG_VERBOSE]))
			Printf("Not found in module list\n");
    }
    else
    {
        error = IoErr();
        return_code = RETURN_FAIL;
    }
    
    if (error != 0)
        PrintFault(IoErr(), "FindAddr");

    return return_code;
}
