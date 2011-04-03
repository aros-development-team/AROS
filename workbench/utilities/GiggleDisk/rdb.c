
/*
** rdb.c
*/

/*************************************************************************/

#define SOURCENAME "rdb.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "functions.h"
#include "header.h"
#include "locale_strings.h"
#include "macros.h"
#include "partitionentry.h"
#include "readargs.h"
#include "readwrite.h"
#include "version.h"

/*************************************************************************/

/* /// CreatePartition_RDB()
**
*/

/*************************************************************************/

ULONG CreatePartition_RDB( ULONG rdbblock, ULONG block )
{
struct RigidDiskBlock *rdb;
struct PartitionBlock *pb;
struct FileSysHeaderBlock *fs;
struct PartitionEntry *pe;

ULONG result;
UBYTE datardb[0x200];
UBYTE datapb[0x200];
UBYTE datafs[0x200];

ULONG sectorsize, partblock, fsblock;

    rdb = (APTR) datardb;
    pb  = (APTR) datapb;
    fs  = (APTR) datafs;

    if( !(result = Device_ReadBlock( (UBYTE *) rdb, rdbblock, 0x200)) ) {
        result = MSG_ERROR_PartitionTableIsInvalid;
		debug( APPLICATIONNAME ": RDB read (again)\n");
        sectorsize = rdb->rdb_BlockBytes;
        if( rdb->rdb_ID == IDNAME_RIGIDDISK  ) {
            partblock = rdb->rdb_PartitionList + block;

            do {

                if( !(result = Device_ReadBlock( (UBYTE *) pb, partblock, sectorsize)) ) {
                    //debug("partblock is %ld %512bh\n",partblock, pb);
                    result = MSG_ERROR_RDBPartitionIsInvalid;
                    if( pb->pb_ID == IDNAME_PARTITION ) {

                        result = MSG_ERROR_NotEnoughMemory;
                        if( (pe = Memory_AllocVec( sizeof( struct PartitionEntry) )) ) {

                            AddTail( &partitionlist, &pe->Node);

                            String_CopySBSTR( &pb->pb_DriveName[0], &pe->DriveName[0] );
							debug( APPLICATIONNAME ": DriveName: %s\n", pe->DriveName);

                            CopyMem( &pb->pb_Environment, &pe->ENV, sizeof( struct DosEnvec));

                            pe->ENV.de_LowCyl  += block; /* add offset just in case it's a virtual disk */
                            pe->ENV.de_HighCyl += block; /* add offset just in case it's a virtual disk */
                            pe->StackSize      = 16384;
                            pe->Device         = readargs_array[ARG_DEVICE];
                            pe->Unit           = readargs_array[ARG_UNIT];
                            pe->PartitionType  = pe->ENV.de_DosType;
                            pe->Flags          = pb->pb_Flags | (( block ) ? PBFF_VIRTUAL : 0L);

                            //PE_CheckSuperBlock( pe);

                            PE_ShrinkCycles( pe );

                            PE_CalculateSize( pe);

                            result = MSG_ERROR_NoError;
                            if( (rdb->rdb_FileSysHeaderList+1) ) {

                                fsblock = rdb->rdb_FileSysHeaderList + block;

                                do {
                                    if( !(result = Device_ReadBlock( (UBYTE *) fs, fsblock, sectorsize)) ) {
                                        //debug("Fsblock is %ld %512bh\n",fsblock, fs);
                                        result = MSG_ERROR_RDBFileSystemHeaderIsInvalid;
                                        if( fs->fhb_ID == IDNAME_FILESYSHEADER ) {
                                            result = MSG_ERROR_NoError;
                                            if( fs->fhb_DosType == pe->ENV.de_DosType ) {
                                                String_Copy( &fs->fhb_FileSysName[0], &pe->FileSysName[0] );
                                                break;
                                            }

                                        }


                                    }
                                    fsblock = fs->fhb_Next + block;
                                } while( !result && (fs->fhb_Next+1) );
                            } else {
                                PE_SetFileSystem( pe ); /* no filesystem in RDB, so best guess */
                            }
                        }
                    }
                }
                partblock = pb->pb_Next + block;

            } while( !result && pb->pb_Next && (pb->pb_Next+1) );
        }
    }
	return( result);
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      


/* \\\ */


