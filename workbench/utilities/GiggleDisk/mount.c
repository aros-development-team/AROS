
/*
** mount.c
*/

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>

#include "header.h"
#include "readargs.h"
#include "locale_strings.h"
#include "create.h"
#include "sprintf.h"
#include "version.h"

ULONG Do_Mount( struct PartitionEntry *pe, STRPTR mountfile );

/*************************************************************************/

/* /// GD_Mount()
**
*/

/*************************************************************************/

ULONG GD_Mount( void )
{
struct PartitionEntry *pe;
ULONG result;
#define MOUNTFILE_SIZEOF 0x200

char mountfile[ MOUNTFILE_SIZEOF ];


	SPrintf( "t:tmp_giggle%08lx", mountfile, (ULONG *) &pe );

	debug( APPLICATIONNAME ": mount file: %s\n", mountfile );

	if( !(result = Create_MountFile( mountfile, NULL ) ) ) {
        for( pe = (APTR) partitionlist.lh_Head ; pe->Node.ln_Succ ; pe = (APTR) pe->Node.ln_Succ ) {

            if(
                (((pe->ENV.de_DosType & 0xffffff00) == ID_FAT0_DISK)   && readargs_array[ARG_MOUNTDOS ] ) ||
                (((pe->ENV.de_DosType             ) == ID_NTFS_DISK)   && readargs_array[ARG_MOUNTNTFS] ) ||
                (((pe->ENV.de_DosType             ) == ID_EXT2_DISK)   && readargs_array[ARG_MOUNTEXT ] ) ||
                (((pe->ENV.de_DosType             ) == ID_XFS0_DISK)   && readargs_array[ARG_MOUNTSGIX] ) ||
                (((pe->ENV.de_DosType             ) == ID_MSH_DISK)    && readargs_array[ARG_MOUNTDOS ] ) ||
                (((pe->ENV.de_DosType             ) == ID_MSDOS_DISK)  && readargs_array[ARG_MOUNTDOS ] ) ) {

//            debug("mount: %s\n", &pe->DriveName[0]);
                Do_Mount( pe, mountfile );
            }
        }
    }

    DeleteFile( mountfile );
	return( result );
}
/* \\\ */

/* /// Do_Mount()
**
*/

/*************************************************************************/

ULONG Do_Mount( struct PartitionEntry *pe, STRPTR mountfile )
{
#define COMMAND_SIZEOF 0x200
char command[ COMMAND_SIZEOF ];
ULONG args[2];

    args[0] = (ULONG) &pe->DriveName[0];
    args[1] = (ULONG) mountfile;


	SPrintf("mount %s: from %s", &command[0], args );
    VPrintf("Mounting '%s:'\n", args );

    SystemTags( command, TAG_DONE );

	return( 0L );
}
/* \\\ */
