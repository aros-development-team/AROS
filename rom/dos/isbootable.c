/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Can this disk boot on your architecture?
    Lang: english
*/

#include <aros/debug.h>
#include <exec/alerts.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "dos_intern.h"

/* Signature file, contains (for example) 'pc-i386' */
#define AROS_BOOT_CHECKSIG ":AROS.boot"
/* Alternate variant: check if Shell is loadable
#define AROS_BOOT_CHECKEXEC ":C/Shell" */

BOOL __dos_IsBootable(struct DosLibrary * DOSBase, BPTR lock)
{
#if defined(AROS_BOOT_CHECKSIG)
    BPTR fh;
    STRPTR buffer;
    LONG bufferLength;
    struct FileInfoBlock *abfile_fib;
    BOOL result = FALSE;

    D(bug("[__dos_IsBootable] __dos_IsBootable('%s')\n", AROS_BOOT_CHECKSIG));

    CurrentDir(lock);
    fh = Open(AROS_BOOT_CHECKSIG, MODE_OLDFILE);
    if (!fh)
    {
#ifdef __mc68000
        /*
         * Original Amiga disks don't contain this signature. They are obviously bootable on m68k.
         * However if the disk DOES contain a signature, we should check it. This can happen to be
         * a disk for another architecture. Attempting to boot from it will cause crash.
         */
        return TRUE;
#else
        return FALSE;
#endif
    }

    D(bug("[__dos_IsBootable] Opened succesfully\n"));

    abfile_fib = AllocDosObject(DOS_FIB, NULL);
    if (abfile_fib)
    {
        if (ExamineFH(fh, abfile_fib))
        {
            LONG readsize;

            bufferLength = abfile_fib->fib_Size + 1;

            buffer = AllocMem(bufferLength, MEMF_ANY);
            D(bug("[__dos_IsBootable] Allocated %d bytes for Buffer @ %p\n", bufferLength, buffer));

            if (!buffer)
            {
                Alert(AT_DeadEnd | AG_NoMemory | AN_DOSLib);
            }

            readsize = Read(fh, buffer, bufferLength - 1);
            if (readsize > 0)
            {
                char *sigptr = NULL;

                buffer[readsize] = '\0';
                D(bug("[__dos_IsBootable] Buffer contains '%s'\n", buffer));

                if ((sigptr = strstr(buffer, AROS_CPU)) != 0)
                {
                    D(bug("[__dos_IsBootable] Signature '%s' found\n", AROS_CPU));
                    result = TRUE;
                }
                else
                {
                    /* Something to more or less reflect "This disk is not bootable" */
                    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
                }
            }
        }
        FreeDosObject(DOS_FIB, abfile_fib);
    }

    Close(fh);

    D(bug("[__dos_IsBootable] returned %d\n", result));
    return result;

#elif defined(AROS_BOOT_CHECKEXEC)
    BPTR seg;

    D(bug("[__dos_IsBootable]: Trying to load '%s' as an executable\n", AROS_BOOT_CHECKEXEC));

    CurrentDir(lock);
    if ((seg = LoadSeg(AROS_BOOT_CHECKEXEC)))
    {
        UnLoadSeg(seg);

        D(bug("[__dos_IsBootable] Success!\n"));        
        return TRUE;
    }

#else
    return TRUE;
#endif
}
