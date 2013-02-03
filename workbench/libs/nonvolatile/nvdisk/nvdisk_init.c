/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Nonvolatile disk based storage library initialization code.
    Lang: English
*/


#define  DEBUG  1

#include "nvdisk_intern.h"

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <devices/timer.h>
#include <aros/symbolsets.h>

#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/alerts.h>
#include LC_LIBDEFS_FILE


static int Init(LIBBASETYPEPTR LIBBASE)
{
    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    BOOL error = TRUE;    // Notice the initialization to 'TRUE' here.
    char *temp = NULL;
    BPTR locFile;

    LIBBASE->nvd_DOSBase = OpenLibrary("dos.library",0);
    if (LIBBASE->nvd_DOSBase == NULL)
        return 0;
    
    D(bug("Init nvdisk.library\n"));

    locFile = Open("SYS:prefs/env-archive/sys/nv_location", MODE_OLDFILE);

    D(bug("Getting location\n"));

    if(locFile != BNULL)
    {
        D(bug("Location file exists!\n"));
        
        temp = AllocVec(512, MEMF_CLEAR);
                
        if(temp != NULL)
        {
            int i = 0;         // Loop variable

            Read(locFile, temp, 512);

            // End string if a carriage return is encountered
            while(temp[i] != 0)
            {
                if(temp[i] == '\n')
                {
                    temp[i] = 0;
                    break;
                }

                i++;
            }

            nvdBase->nvd_location = Lock(temp, SHARED_LOCK);
                    
            D(bug("NV location = %s\n", temp));

            FreeVec(temp);
        }    
        Close(locFile);
    } else {
        /* Volatile mode, if we have no nv_location file */
        UnLock(CreateDir("RAM:NVRAM"));
        nvdBase->nvd_location = Lock("RAM:NVRAM", SHARED_LOCK);
    }

    D(bug("Got lock = %p\n", nvdBase->nvd_location));

    if(nvdBase->nvd_location != BNULL)
    {
        error = FALSE;
    }

    return !error;
}

static int Expunge(LIBBASETYPEPTR LIBBASE)
{
    /*
        This function is single-threaded by exec by calling Forbid.
        Never break the Forbid() or strange things might happen.
    */

    UnLock(nvdBase->nvd_location);

    CloseLibrary(nvdBase->nvd_DOSBase);
    
    return TRUE;
}

ADD2INITLIB(Init, 0);
ADD2EXPUNGELIB(Expunge, 0);
