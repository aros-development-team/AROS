/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/exall.h>
#include <dos/rdargs.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(void, FreeDosObject,

/*  SYNOPSIS */
        AROS_LHA(ULONG, type, D1),
        AROS_LHA(APTR,  ptr,  D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 39, Dos)

/*  FUNCTION
        Frees an object allocated with AllocDosObject.

    INPUTS
        type - object type. The same parameter as given to AllocDosObject().
        ptr  - Pointer to object.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (ptr == NULL)
        return;

    switch(type)
    {
        case DOS_FILEHANDLE:
        {
            struct FileHandle *fh=(struct FileHandle *)ptr;
            if (fh->fh_Flags & FHF_OWNBUF)
                FreeMem(BADDR(fh->fh_OrigBuf),fh->fh_BufSize);
            FreeVec(fh);
            break;
        }
        case DOS_FIB:
            FreeVec(ptr);
            break;
            
        case DOS_STDPKT:
            freedospacket(ptr);
            break;
            
        case DOS_EXALLCONTROL:
            if (((struct InternalExAllControl *)ptr)->fib)
                FreeDosObject(DOS_FIB, ((struct InternalExAllControl *)ptr)->fib);
            
            FreeVec(ptr);
            break;
            
        case DOS_CLI:
        {
            struct CommandLineInterface *cli=(struct CommandLineInterface *)ptr;
            BPTR *cur, *next;
            cur=(BPTR *)BADDR(cli->cli_CommandDir);
            FreeVec(BADDR(cli->cli_SetName));
            FreeVec(BADDR(cli->cli_CommandName));
            FreeVec(BADDR(cli->cli_CommandFile));
            FreeVec(BADDR(cli->cli_Prompt));
            FreeVec(ptr);
            while(cur!=NULL)
            {
                next=(BPTR *)BADDR(cur[0]);
                UnLock(cur[1]);
                FreeVec(cur);
                cur=next;
            }
            break;

        /*
            FreeArgs() will not free a RDArgs without a RDA_DAList, 
            see that function for more information as to why...
        */
        case DOS_RDARGS:
            if(((struct RDArgs *)ptr)->RDA_DAList != 0)
                FreeArgs(ptr);
            else
                FreeVec(ptr);

            break;
        }
    }
    AROS_LIBFUNC_EXIT
} /* FreeDosObject */

#undef offsetof
#undef container_of

#define offsetof(TYPE, MEMBER) ((IPTR) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({          \
    const typeof(((type *)0)->member) *__mptr = (ptr);    \
             (type *)((char *)__mptr - offsetof(type, member)); })

void freedospacket(struct DosPacket *dp)
{
    FreeVec(container_of(dp, struct StandardPacket, sp_Pkt));
}
