/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>
#include <libraries/debug.h>
#include <proto/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "internalloadseg.h"

static char *getname(BPTR file, char **bufferp, struct DosLibrary *DOSBase)
{
    /* Some applications pass a non-filehandle to
     * Dos/InternalLoadSeg, so don't try to register
     * the hunks if this is not a real FileHandle
     *
     * A real-life example of this is C:AddDataTypes
     * from AmigaOS 3.9
     */
    if (ISFILEHANDLE(file) && DOSBase) {
        char *buffer = AllocMem(512, MEMF_ANY);
        if (buffer) {
            *bufferp = buffer;
            if (NameFromFH(file, buffer, 512)) {
                char *nameptr = buffer;
                /* gdb support needs full paths */
#if !AROS_MODULES_DEBUG
                /* First, go through the name, till end of the string */
                while(*nameptr++);
                /* Now, go back until either ":" or "/" is found */
                while(nameptr > buffer && nameptr[-1] != ':' && nameptr[-1] != '/')
                    nameptr--;
#endif
                return nameptr;
            }
        }
    }
    return "unknown";
}

void register_elf(BPTR file, BPTR hunks, struct elfheader *eh, struct sheader *sh, struct DosLibrary *DOSBase)
{
    if (DOSBase && DebugBase)
    {
        char *buffer = NULL;
        char *nameptr = getname(file, &buffer, DOSBase);
        struct ELF_DebugInfo dbg = {eh, sh};
        RegisterModule(nameptr, hunks, DEBUG_ELF, &dbg);
        FreeMem(buffer, 512);
    }
}

#undef DebugBase
/* DOSBase is NULL if called by m68k RDB filesystem loader.
 * No DosBase = Open debug.library manually, this enables
 * us to have RDB segments in debug.library list.
 */
void register_hunk(BPTR file, BPTR hunks, APTR header, struct DosLibrary *DOSBase)
{
    struct Library *DebugBase;
    if (DOSBase)
        DebugBase = IDosBase(DOSBase)->debugBase;
    else
        DebugBase = OpenLibrary("debug.library", 0);
    if (DebugBase)
    {
        char *buffer = NULL;
        char *nameptr = getname(file, &buffer, DOSBase);
        struct HUNK_DebugInfo dbg = { header };
        RegisterModule(nameptr, hunks, DEBUG_HUNK, &dbg);
        FreeMem(buffer, 512);
    }
    if (!DOSBase)
        CloseLibrary(DebugBase);
}
