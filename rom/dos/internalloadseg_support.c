#include <aros/config.h>
#include <libraries/debug.h>
#include <proto/debug.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "dos_intern.h"
#include "internalloadseg.h"

void register_elf(BPTR file, BPTR hunks, struct elfheader *eh, struct sheader *sh, struct DosLibrary *DOSBase)
{
    if (DebugBase)
    {
        char *buffer = AllocMem(512, MEMF_ANY);

        if (buffer) {
            if (NameFromFH(file, buffer, 512))
            {
                char *nameptr = buffer;
                struct ELF_DebugInfo dbg = {eh, sh};

    /* gdb support needs full paths */
#if !AROS_MODULES_DEBUG
                /* First, go through the name, till end of the string */
                while(*nameptr++);
                /* Now, go back until either ":" or "/" is found */
                while(nameptr > buffer && nameptr[-1] != ':' && nameptr[-1] != '/')
                    nameptr--;
#endif
                RegisterModule(nameptr, hunks, DEBUG_ELF, &dbg);
            }
            FreeMem(buffer, 512);
        }
    }
}

void register_hunk(BPTR file, BPTR hunks, APTR header, struct DosLibrary *DOSBase)
{
    if (DebugBase)
    {
        char *buffer = AllocMem(512, MEMF_ANY);

        if (buffer) {
            if (NameFromFH(file, buffer, 512))
            {
                char *nameptr = buffer;
                struct HUNK_DebugInfo dbg = { header };


    /* gdb support needs full paths */
#if !AROS_MODULES_DEBUG
                /* First, go through the name, till end of the string */
                while(*nameptr++);
                /* Now, go back until either ":" or "/" is found */
                while(nameptr > buffer && nameptr[-1] != ':' && nameptr[-1] != '/')
                    nameptr--;
#endif
                RegisterModule(nameptr, hunks, DEBUG_HUNK, &dbg);
            }
            FreeMem(buffer, 512);
        }
    }
}
