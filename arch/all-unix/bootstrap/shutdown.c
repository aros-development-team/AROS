#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shutdown.h"

#define D(x) 

#ifdef __i386__
/*
 * MacOS X x86 ABI says that stack must be aligned at 16 bytes boundary.
 * Otherwise dyld's lazy binding (dyld_stub_binder) can crash at first.
 * See http://www.opensource.apple.com/source/dyld/dyld-132.13/src/dyld_stub_binder.s
 * Luckily this doesn't seem to happen when functions are linked in explicitly using dlsym().
 * On other systems this at least won't harm.
 */
#define __stackalign __attribute__((force_align_arg_pointer))
#else
#define __stackalign
#endif

static char **Kernel_ArgV;

void SaveArgs(char **argv)
{
    Kernel_ArgV = argv;
}

__stackalign void Host_Shutdown(unsigned char warm)
{
    /* Warm reboot is not implemented yet */
    if (warm)
	return;

    D(printf("[Shutdown] Cold reboot, dir: %s, name: %s\n", bootstrapdir, Kernel_ArgV[0]));
    if (chdir(bootstrapdir)==0)
    	execvp(Kernel_ArgV[0], Kernel_ArgV);

    D(printf("[Shutdown] Unable to re-run AROS\n"));
}
