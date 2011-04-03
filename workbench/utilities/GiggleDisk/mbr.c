
/*
** mbr.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "mbr.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "header.h"
#include "locale_strings.h"
#include "macros.h"
#include "mbr.h"
#include "partitionentry.h"
#include "rdb.h"
#include "readargs.h"
#include "readwrite.h"
#include "sprintf.h"
#include "version.h"

/*************************************************************************/

/* /// CreatePartition_MBR()
**
*/

/*************************************************************************/

ULONG CreatePartition_MBR( ULONG block )
{
struct MBRPartitionBlock pb;
struct PartitionEntry *pe;
ULONG i;
ULONG result;
ULONG args[2];

	debug( APPLICATIONNAME ": MBR read (again (%ld))\n", block);
    if( !(result = Device_ReadBlock( (UBYTE *) &pb, block, SECTORSIZE )) ) {
        result = MSG_ERROR_PartitionTableIsInvalid;
        if( pb.Partition_TableFlag == PART_TABLEFLAG ) {
            result = 0;
			debug( APPLICATIONNAME ": MBR is still valid\n");
			debug( APPLICATIONNAME ": MBr size %08lx\n", sizeof( struct MBRPartition));
			debug( APPLICATIONNAME ": MBR Dump %512bh\n", &pb);
            for( i = 0; i < 4 ; i++ ) {

				debug( APPLICATIONNAME ": checking partition: %ld type: %bx\n", i, pb.Partition[i].PartitionType);
				debug( APPLICATIONNAME ": Partition field: %16bh\n", &pb.Partition[i]);

                result = MSG_ERROR_NotEnoughMemory;
                if( (pe = Memory_AllocVec( sizeof( struct PartitionEntry) )) ) {
                    AddTail( &partitionlist, &pe->Node);

/* first we create a drive name for this partition */

                    args[0] = readargs_array[ARG_PREFIX];
                    args[1] = partitionnumber++;     /* increase partition number */
                    SPrintf( "%s%ld", &pe->DriveName[0], args );

/* now we fill the entire structure */

                    pe->SectorSize       = 0x200;

                    pe->StartHead       = pb.Partition[i].StartingHead;
                    pe->StartSector     = SECTOR(   pb.Partition[i].StartingSector);
                    pe->StartCylinder   = CYLINDER( pb.Partition[i].EndingSector,pb.Partition[i].StartingCylinder);

                    pe->EndHead         = pb.Partition[i].EndingHead;
                    pe->EndSector       = SECTOR(   pb.Partition[i].EndingSector);
                    pe->EndCylinder     = CYLINDER( pb.Partition[i].EndingSector,pb.Partition[i].EndingCylinder);

                    pe->FirstSector      = swap32( pb.Partition[i].StartSector );
                    pe->FirstSector     += ( pe->FirstSector ) ? block : 0;

                    pe->NumberOfSectors  = swap32( pb.Partition[i].NumberOfSectors );

				debug( APPLICATIONNAME ": First: %08lx (%ld), Mum: %08lx (%ld)\n", pe->FirstSector, pe->FirstSector, pe->NumberOfSectors, pe->NumberOfSectors);

                    pe->PartitionSize    = pe->NumberOfSectors / (0x100000/ pe->SectorSize);
                    pe->PartitionType    = pb.Partition[i].PartitionType;

                    pe->Flags           |= (pb.Partition[i].BootIndicator == PART_BOOTINDICATOR ) ? PBFF_BOOTABLE : 0L;
                    pe->Flags           |= ( block ) ? PBFF_VIRTUAL : 0L;

                    pe->Device                 = readargs_array[ARG_DEVICE];
                    pe->Unit                   = readargs_array[ARG_UNIT];
                    pe->StackSize              = 16384;
                    pe->ENV.de_SizeBlock       = 0x200 / 4; /* block size in longs */
                    pe->ENV.de_Surfaces        = 1;
                    pe->ENV.de_SectorPerBlock  = 1;
                    pe->ENV.de_BlocksPerTrack  = 1;
/* alloc is clearing memory, so skip this
                    pe->ENV.de_Reserved        = 0;
                    pe->ENV.de_PreAlloc        = 0;
                    pe->ENV.de_Interleave      = 0;
*/
                    pe->ENV.de_LowCyl          = pe->FirstSector;
                    pe->ENV.de_HighCyl         = pe->FirstSector + pe->NumberOfSectors;
                    pe->ENV.de_NumBuffers      = 50;
                    pe->ENV.de_BufMemType      = MEMF_ANY|MEMF_PUBLIC;
                    pe->ENV.de_MaxTransfer     = readargs_array[ARG_MAXTRANSFER];
                    pe->ENV.de_Mask            = 0x7ffffffe;
                    pe->ENV.de_BootPri         = 10;

                    pe->ENV.de_DosType         = pe->PartitionType;

					PE_CheckSuperBlock( pe);

                    PE_CalculateSize( pe );
                    PE_SetFileSystem( pe );
                    PE_ShrinkCycles( pe );

                    if( (pe->PartitionType == FSID_AVDH) ) {
						debug( APPLICATIONNAME ": Reading next RDB\n");
                        if( (result = CreatePartition_RDB( pe->FirstSector, pe->FirstSector )) ) {
                            break;
                        }
                    }
                    if( (pe->PartitionType == FSID_EXTENDED) ||
                        (pe->PartitionType == FSID_EXTENDED32) ) {

						debug( APPLICATIONNAME ": Reading next MBR\n");
                        if( (result = CreatePartition_MBR( pe->FirstSector ))) {
                            break;
                        }
                    }


                    result = MSG_ERROR_NoError;
                }
            }
        }
    }
debug( APPLICATIONNAME ": List MBR result: %ld \n", result);
return( result);
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      






/* \\\ */

/* /// MBR_Check()
**
** NOTE: It's required to check if Block is 0, for 100% testing.
*/

/*************************************************************************/

BOOL MBR_Check( struct MBRPartitionBlock *mbr )
{

    if( (mbr->Partition_TableFlag == PART_TABLEFLAG) &&
        (((mbr->Partition[0].BootIndicator & 0xf0) == 0x00) || ((mbr->Partition[0].BootIndicator & 0xf0) == 0x80)) &&
        (((mbr->Partition[1].BootIndicator & 0xf0) == 0x00) || ((mbr->Partition[1].BootIndicator & 0xf0) == 0x80)) &&
        (((mbr->Partition[2].BootIndicator & 0xf0) == 0x00) || ((mbr->Partition[2].BootIndicator & 0xf0) == 0x80)) &&
        (((mbr->Partition[3].BootIndicator & 0xf0) == 0x00) || ((mbr->Partition[3].BootIndicator & 0xf0) == 0x80)) ) {

        return( TRUE );
    } else {
        return( FALSE );
    }
}
/* \\\ */

