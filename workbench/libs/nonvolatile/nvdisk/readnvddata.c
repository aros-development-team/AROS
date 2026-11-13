/*
    Copyright (C) 1995-2001, The AROS Development Team. All rights reserved.

    Desc:
*/



#include "nvdisk_intern.h"
#include <nvdisk_arch.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <exec/memory.h>


/*****************************************************************************

    NAME */

        AROS_LH2(APTR, ReadNVDData,

/*  SYNOPSIS */

        AROS_LHA(STRPTR, appName,  A0),
        AROS_LHA(STRPTR, itemName, A1),

/*  LOCATION */

        struct Library *, nvdBase, 5, NVDisk)

/*  FUNCTION

    Retrieve an application's nonvolatile data item.

    INPUTS

    appName   --  name of the application owning the item
    itemName  --  name of the item to retrieve

    RESULT

    Pointer to an allocated data buffer, or NULL if the item cannot be
    retrieved. The caller owns a successful result and must release it
    using nonvolatile.library/FreeNVData().

    NOTES

    The filesystem-backed implementation locates the item in the
    application's directory beneath the configured storage root. An
    active native backend handles the request through NVDisk_ArchRead().

    The public GetCopyNV() interface validates names and provides the
    documented application-facing entry point.

    SEE ALSO

    nonvolatile.library/GetCopyNV(), nonvolatile.library/FreeNVData()

    HISTORY

    November 2000, SDuvan -- implemented

******************************************************************************/

{
    AROS_LIBFUNC_INIT

    if (NVDisk_ArchActive(GPB(nvdBase)))
        return NVDisk_ArchRead(GPB(nvdBase), appName, itemName);

    APTR  mem = NULL;
    BPTR  lock, lock2;
    BPTR  oldCDir;

    /* Allocate a FileInfoBlock to be able to determine the size of the
       file containing the data. */
    struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

    if(fib != NULL)
    {
        oldCDir = CurrentDir(GPB(nvdBase)->nvd_location);
        lock = Lock(appName, SHARED_LOCK);

        if(lock)
        {
            CurrentDir(lock);

            lock2 = Lock(itemName, SHARED_LOCK);

            if(lock2 != BNULL)
            {
                // We have now found the file -- check how big it is
                Examine(lock2, fib);

                mem = AllocVec(fib->fib_Size, MEMF_ANY);

                if(mem != NULL)
                {
                    if(Read(lock2, mem, fib->fib_Size) != fib->fib_Size)
                    {
                        FreeVec(mem);
                        mem = NULL;
                    }
                    
                } /* if(mem) */
                
                UnLock(lock2);
                
            } /* if(lock2) */
            
            CurrentDir(oldCDir);
            UnLock(lock);
            
        } /* if(lock) */
        
        CurrentDir(oldCDir);
        FreeDosObject(DOS_FIB, fib);
        
    } /* if(fib) */
    
    return mem;

    AROS_LIBFUNC_EXIT
} /* ReadData */
