
/*
** createpartitionlist.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "createpartitionlist.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include "header.h"
#include "mbr.h"
#include "partitionentry.h"
#include "rdb.h"
#include "readargs.h"
#include "readwrite.h"
#include "sprintf.h"
#include "version.h"

/*************************************************************************/

/* /// Create_Partition_List()
**
*/

/*************************************************************************/

ULONG Create_PartitionList( void )
{
    struct PartitionEntry *pe;
    BYTE data[0x800];
    ULONG result, block, i, id;
    IPTR args[2];

    i = 0;
    for( block = 0; block < RDB_LOCATION_LIMIT ; block ++ ) {

        if( !(result = Device_ReadBlock( (UBYTE *) data, block, 2048 )) ) {
            if( !block ) {

            }
            if( !block && MBR_Check( (struct MBRPartitionBlock *) data) ) {
				debug( APPLICATIONNAME ": MBR detected\n");
                result = CreatePartition_MBR( 0L );
                i++;
                /* break;  28.04.2005 removed break to support mixed mode MBR followed by RDB */
            }
            if( ((struct RigidDiskBlock *) data)->rdb_ID == IDNAME_RIGIDDISK ) {
				debug( APPLICATIONNAME ": RDB detected\n");
                result = CreatePartition_RDB( block, 0L );
                i++;
                break;
            }
        }
    }
/* no error and no error, then it is probably a drive formatted without MBR or RDB */

    if( !result && !i ) {

		if( !(result = Device_ReadBlock( (UBYTE *) data, 0L, 2048 ) ) ) {

            if( (id = PE_IdentifySuperblock( &data[0], 0L)) ) {

                result = MSG_ERROR_NotEnoughMemory;
				if( (pe = Memory_AllocVec( sizeof( struct PartitionEntry ) ) ) ) {
					AddTail( &partitionlist, &pe->Node );

                    pe->ENV.de_DosType = id;
                    pe->PartitionType  = id;

                    result = PE_GetGeometry( pe );
					PE_CheckSuperBlock( pe );   /* added 01.07.2005 */

                    PE_CalculateSize( pe );
                    PE_SetFileSystem( pe );
                    PE_ShrinkCycles( pe );
/* setup mount name */
                    args[0] = readargs_array[ARG_PREFIX];
                    args[1] = partitionnumber++;     /* increase partition number */
                    SPrintf( "%s%ld", &pe->DriveName[0], args );


					debug( APPLICATIONNAME ": Non MBR and non RDB found! Superblock says: %08lx\n", PE_IdentifySuperblock( &data[0], 0L ) );
                    debug("%2048lh\n", &data[0]);



                }
            }
        }
    }
	debug( APPLICATIONNAME ": Create_Partitionlist Result: %ld\n", result );

/* is there any partition in list? No? Then return error */

	if( !result && !(partitionlist.lh_Head->ln_Succ ) ) {
        result = MSG_ERROR_NothingUseful;
    }

return( result );
}
/* \\\ */
