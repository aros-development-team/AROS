
/*
** giggledisk.c
**
** (c) 1998 - 2011 Guido Mersmann
** (c) 2011 The AROS Development Team.
*/

/*************************************************************************/

#define SOURCENAME "giggledisk.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>

#include "create.h"
#include "createpartitionlist.h"
#include "functions.h"
#include "giggledisk.h"
#include "header.h"
#include "macros.h"
#include "main.h"
#include "mount.h"
#include "readargs.h"
#include "requester.h"
#include "version.h"

/*************************************************************************/

/* /// MD_ShowPartitions()
**
*/

/*************************************************************************/

static void MD_ShowPartitions( void )
{
    struct PartitionEntry *pe;
    IPTR args[15], i;
    IPTR *arg;

	VPrintf("\nGiggleDisk © 2005 - 2011 Guido Mersmann\n\n", args );

    i = 0;
    for( pe = (APTR) partitionlist.lh_Head ; pe->Node.ln_Succ ; pe = (APTR) pe->Node.ln_Succ ) {
        if( pe->PartitionType ) {
            arg = args;
            *arg++ = i++;
            *arg++ = pe->ENV.de_DosType;
            *arg++ = pe->ENV.de_LowCyl;
            *arg++ = pe->ENV.de_HighCyl;
            *arg++ = ((pe->PartitionSize < 1024) ? pe->PartitionSize : ((pe->PartitionSize & 1023) > 800) ? (pe->PartitionSize>>10)+1 : (pe->PartitionSize>>10));
            *arg++ = (IPTR)((pe->PartitionSize < 1024) ? "MB"              : "GB");
            *arg++ = (IPTR)&pe->FileSysName[0];
            *arg++ = (IPTR) &pe->FileSysName[0];
			VPrintf("%2ld, DosType: %08lx, LowCyl: %10ld HighCyl: %10ld, Size: %4ld %s, FileSystem: %s \n", args );
        }
    }
    VPrintf("\n\n", args);
}
/* \\\ */

/* /// GiggleDisk()
**
*/

/*************************************************************************/

ULONG GiggleDisk( void )
{
ULONG result;
	debug( "\n%76m#\n\n" APPLICATIONNAME ": GiggleDisk V%ld.%ld\n\n", VERSION, REVISION );

    if( !partitionlist.lh_Head ) {
		List_Init( &partitionlist );
    }

	if( !( result = GD_OpenDevice() ) ) {

		if( !(result = Create_PartitionList() ) ) {

/* List if required */

            if( readargs_array[ARG_LIST] || !readargs_array[ARG_TO] ) {
                MD_ShowPartitions();
            }
/* Mount if needed */

            GD_Mount();

/* write dos related files */
            if( (STRPTR) readargs_array[ARG_TO] ) {

                if( Dos_CheckExistsDrawer( (STRPTR) readargs_array[ARG_TO]) ) {
                    result = Create_DosDriver((STRPTR)  readargs_array[ARG_TO], NULL );
                } else {
                    result = Create_MountFile( (STRPTR) readargs_array[ARG_TO], NULL );
                }
            }
        }
    }

    Device_Close( &device );
	List_Free( &partitionlist );

	return( result );
}
/* \\\ */
