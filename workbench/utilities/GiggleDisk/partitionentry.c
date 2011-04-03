
/*
** partitionentry.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "partitionentry.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <exec/io.h>

#include "functions.h"
#include "header.h"
#include "locale_strings.h"
#include "macros.h"
#include "partitionentry.h"
#include "readargs.h"
#include "readwrite.h"
#include "version.h"

/*************************************************************************/

/* /// PE_FindPartitionName()
**
*/

/*************************************************************************/

STRPTR PE_FindPartitionName( ULONG type )
{
ULONG i = 0;

    while( FSIDARRAY[i].Type != FSID_TERMINATED && FSIDARRAY[i].Type != type) { i++; }

return( FSIDARRAY[i].Name );

}
/* \\\ */

/* /// PE_ShrinkCycles()
**
** Lower the values of lowcyl and highcyl.
**
** Some file-systems/devices can't handle high values for the cylinders
** (probably higher than 65536). So we try to keep them low, by increasing
** the number of cylinders.
*/

/*************************************************************************/

void PE_ShrinkCycles( struct PartitionEntry *pe )
{
    if( readargs_array[ARG_LOWERCYL] ) {

        while( !(pe->ENV.de_LowCyl & 1) && !(pe->ENV.de_HighCyl & 1) && (pe->ENV.de_BlocksPerTrack < 0x0800) ) {
            pe->ENV.de_BlocksPerTrack += pe->ENV.de_BlocksPerTrack;
            pe->ENV.de_LowCyl          = pe->ENV.de_LowCyl>>1;
            pe->ENV.de_HighCyl         = pe->ENV.de_HighCyl>>1;
        }
    }
}
/* \\\ */

/* /// PE_SetFileSystem()
**
*/

/*************************************************************************/

void PE_SetFileSystem( struct PartitionEntry *pe )
{
STRPTR name;

    switch( pe->ENV.de_DosType ) {

        case ID_MSDOS_DISK:
        case ID_MSH_DISK:
            name = "L:CrossDOSFileSystem";
            break;

        case ID_FAT0_DISK:
        case ID_FAT1_DISK:
        case ID_FAT2_DISK:
        case ID_FAT3_DISK:
        case ID_FAT4_DISK:
        case ID_FAT5_DISK:
        case ID_FAT6_DISK:
        case ID_FAT7_DISK:
            name = "L:fat95";
            break;

        case ID_NTFS_DISK:
            name = "L:NTFileSystem";
            break;

        case ID_EXT2_DISK:
            name = "L:EXT2FileSystem";
            break;

        case ID_XFS0_DISK:
            name = "L:SGIXFileSystem";
            break;

        case ID_SFS_DISK:
        case ID_CFS_DISK:
            name = "L:SmartFilesystem";
            break;

        case ID_DOS_DISK:
        case ID_FFS_DISK:
        case ID_INTER_DOS_DISK:
        case ID_INTER_FFS_DISK:
        case ID_FASTDIR_DOS_DISK:
        case ID_FASTDIR_FFS_DISK:
        case ID_FASTDIRINTER_DOS_DISK:
        case ID_FASTDIRINTER_FFS_DISK:
            name = "L:FastFileSystem";
            break;

        default:

            name = PE_FindPartitionName( pe->ENV.de_DosType);
            break;
    }
    String_Copy( name, &pe->FileSysName[0]);

}
/* \\\ */

/* /// PE_CalculateSize()
**
*/

/*************************************************************************/

void PE_CalculateSize( struct PartitionEntry *pe )
{
/*
** Here we calculate the partition size, which is (HighCyl-LowCyl+1) * (SizeBlock*4) * BlocksPerTrack * Surfaces / (1024 * 1024).
** The reason for the whiles is, that we may get huge values during multiplication. The simple way would be to divide somewhere
** in the middle, but this is dropping the precision. So we multiply shift as long as bit 0 is clear and we number of shifts is < 20
** This garantees maximum precision without causing the partition size to overrun.
*/

ULONG i, blocksize, blockspertrack;

    i = 0; /* shifts */

    blocksize = pe->ENV.de_SizeBlock*4;
    while( !(blocksize & 1) && i < 20 ) {
        blocksize = blocksize>>1;
        i++;
    }
    blockspertrack = pe->ENV.de_BlocksPerTrack;
    while( !(blockspertrack & 1) && i < 20 ) {
        blockspertrack = blockspertrack>>1;
        i++;
    }
    pe->PartitionSize = (1 + pe->ENV.de_HighCyl - pe->ENV.de_LowCyl);
    while( !(pe->PartitionSize & 1) && i < 20 ) {
        pe->PartitionSize = pe->PartitionSize>>1;
        i++;
    }
    pe->PartitionSize *= blocksize;
    while( !(pe->PartitionSize & 1) && i < 20 ) {
        pe->PartitionSize = pe->PartitionSize>>1;
        i++;
    }
    pe->PartitionSize *= blockspertrack;
    while( !(pe->PartitionSize & 1) && i < 20 ) {
        pe->PartitionSize = pe->PartitionSize>>1;
        i++;
    }
    pe->PartitionSize *= pe->ENV.de_Surfaces;
    while( i < 20 ) {
        pe->PartitionSize = pe->PartitionSize>>1;
        i++;
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      


/* \\\ */

/* /// PE_CheckSuperBlock()
**
*/

/*************************************************************************/

ULONG PE_CheckSuperBlock( struct PartitionEntry *pe )
{
BYTE data[0x800];
ULONG result = 0L;
ULONG newid;

newid = pe->ENV.de_DosType;

/* Superblock check only if there is an ID and it's not EXTENDED and not an RDSK Block */
    if( newid && newid != FSID_EXTENDED && newid != FSID_EXTENDED32 && newid != FSID_AVDH ) {

		debug(APPLICATIONNAME ": PE_CheckSuperBlock: %08lx at ( %ld ) \n", newid, pe->ENV.de_LowCyl + pe->ENV.de_Reserved );
        if( !(result = Device_ReadBlock( &data[0], pe->ENV.de_LowCyl + pe->ENV.de_Reserved , 0x800)) ) {

            newid = PE_IdentifySuperblock( data, newid );


            pe->PartitionType  = newid;
            pe->ENV.de_DosType = newid;

            if( newid == ID_EXT2_DISK ) {
				debug(APPLICATIONNAME ": PE_CheckSuperBlock: EXT2: Blocksize: %08lx\n", swap32(((struct ext2_super_block *) &data[1024])->s_log_block_size));
                pe->ENV.de_SizeBlock  = 1 << (swap32(((struct ext2_super_block *) &data[1024])->s_log_block_size)+8);
				debug(APPLICATIONNAME ": PE_CheckSuperBlock: Updating BlockSize: %08lx\n",pe->ENV.de_SizeBlock  );
            }

        }

    }

/*************************************************************************/
/* Testing AREA */
#ifdef TESTINGAREA
#include <dos/dos.h>
#include <dos/filehandler.h>

#include <proto/dos.h>
#include <proto/exec.h>

ULONG i;
struct DosList *dol;
struct FileSysStartupMsg *fssm;
struct DosEnvec *env;

    if( dol = LockDosList( LDF_DEVICES|LDF_READ)) {

        if( dol = FindDosEntry(dol, "VDH0", LDF_DEVICES|LDF_READ)) {

            fssm = ((struct FileSysStartupMsg *) ( (ULONG) (dol->dol_misc.dol_handler.dol_Startup)<<2L));

            env = (APTR) ((fssm->fssm_Environ)<<2L);

            debug("Surfaces: %ld\n", env->de_Surfaces);
            debug("SectorsPerBlock: %ld\n", env->de_SectorPerBlock);
            debug("BlocksPerTrack: %ld\n", env->de_BlocksPerTrack);
            debug("Reserved: %ld\n", env->de_Reserved);
            debug("PreAlloc: %ld\n", env->de_PreAlloc);
            debug("Interleave: %ld\n", env->de_Interleave);
            debug("LowCyl: %ld\n", env->de_LowCyl);
            debug("HighCyl: %ld\n", env->de_HighCyl);

#ifdef djdjd
struct DosEnvec {
    ULONG de_TableSize;	     /* Size of Environment vector */
    ULONG de_SizeBlock;	     /* in longwords: Physical disk block size */
    ULONG de_SecOrg;	     /* not used; must be 0 */
    ULONG de_Surfaces;	     /* # of heads (surfaces). drive specific */
    ULONG de_SectorPerBlock; /* N de_SizeBlock sectors per logical block */
    ULONG de_BlocksPerTrack; /* blocks per track. drive specific */
    ULONG de_Reserved;	     /* DOS reserved blocks at start of partition. */
    ULONG de_PreAlloc;	     /* DOS reserved blocks at end of partition */
    ULONG de_Interleave;     /* usually 0 */
    ULONG de_LowCyl;	     /* starting cylinder. typically 0 */
    ULONG de_HighCyl;	     /* max cylinder. drive specific */
    ULONG de_NumBuffers;     /* Initial # DOS of buffers.  */
    ULONG de_BufMemType;     /* type of mem to allocate for buffers */
    ULONG de_MaxTransfer;    /* Max number of bytes to transfer at a time */
    ULONG de_Mask;	     /* Address Mask to block out certain memory */
    LONG  de_BootPri;	     /* Boot priority for autoboot */
    ULONG de_DosType;	     /* ASCII (HEX) string showing filesystem type;
			      * 0X444F5300 is old filesystem,
			      * 0X444F5301 is fast file system */
    ULONG de_Baud;	     /* Baud rate for serial handler */
    ULONG de_Control;	     /* Control word for handler/filesystem */
    ULONG de_BootBlocks;     /* Number of blocks containing boot code */

#endif
        }
    }
    UnLockDosList(LDF_DEVICES|LDF_READ);

#endif


/*************************************************************************/

return( result );
}
/* \\\ */

/* /// PE_IdentifySuperblock()
**
*/

/*************************************************************************/

ULONG PE_IdentifySuperblock( BYTE *data, ULONG id )
{

   /* There is a quite big chance to identify e.g. EXT2 instead of another FS, because
    ** it's only using a word sized identifier, so we would check it a last to prevent
    ** detection problems.
    **
    ** BUT linux is stupid. They do not fill the first bytes, so there may be any other
    ** ID + the ext2 ID at the same time.
    **
    ** DAM LINUX SHIT
    */
    if( swap16(((struct ext2_super_block *) &data[1024])->s_magic) == EXT2_SB_MAGIC  ) {
		debug( APPLICATIONNAME ": SUPERBLOCK: EXT2 detected!\n");
        id = ID_EXT2_DISK;
    } else {

        if( !Strnicmp( (STRPTR) &(((struct ntfs_super_block *) data)->magic_id), NTFS_SB_MAGIC, 8) ) {
			debug( APPLICATIONNAME ": SUPERBLOCK: NTFS detected!\n");
            id = ID_NTFS_DISK;
        } else {

            if( !Strnicmp( (STRPTR) &(((struct msdos_super_block *) data)->magic_id), MSWIN_SB_MAGIC, 5) &&
                                     (((struct msdos_super_block *) data)->magic_id)[6] == '.' ) {
				debug( APPLICATIONNAME ": SUPERBLOCK: MSWIN detected!\n");
                id = ID_FAT1_DISK;

            } else {

                if( !Strnicmp( (STRPTR) &(((struct msdos_super_block *) data)->magic_id), MSDOS_SB_MAGIC, 5) &&
                                         (((struct msdos_super_block *) data)->magic_id)[6] == '.' ) {
					debug( APPLICATIONNAME ": SUPERBLOCK: MSDOS detected!\n");
                    id = ID_FAT1_DISK;

                } else {

                    if( !Strnicmp( (STRPTR) &(((struct fat16_super_block *) data)->magic_id), FAT16_SB_MAGIC, 8) ) {
						debug( APPLICATIONNAME ": SUPERBLOCK: FAT16 detected!\n");
                        id = ID_FAT1_DISK;

                    } else {

                        if( ((struct xfs_super_block *) data)->sb_magicnum == XFS_SB_MAGIC ) {
							debug( APPLICATIONNAME ": SUPERBLOCK: EXT2 detected!\n");
                            id = ID_XFS0_DISK;
                        } else {
							 debug( APPLICATIONNAME ": SUPERBLOCK: %512h\n", &data[0]);
                        }
                    }
                }
            }
        }
    }
return( id );
}
/* \\\ */

/* /// PE_GetGeometry()
**
*/

/*************************************************************************/

ULONG PE_GetGeometry( struct PartitionEntry *pe )
{
struct DriveGeometry dg;

    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Command = TD_GETGEOMETRY;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Data = (APTR) &dg;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Length = sizeof( struct DriveGeometry );

    if( !DoIO( device.IORequest )) {
/* first the standard stuff */
        pe->Device                 = readargs_array[ARG_DEVICE];
        pe->Unit                   = readargs_array[ARG_UNIT];
        pe->StackSize              = 16384;
        pe->ENV.de_NumBuffers      = 50;
        pe->ENV.de_MaxTransfer     = readargs_array[ARG_MAXTRANSFER];
        pe->ENV.de_Mask            = 0x7ffffffe;
        pe->ENV.de_BootPri         = 10;
/* and now the geometry stuff */

        pe->ENV.de_SizeBlock      = dg.dg_SectorSize/4;
        pe->ENV.de_LowCyl         = 0L;
        pe->ENV.de_HighCyl        = dg.dg_Cylinders-1;
        pe->ENV.de_Surfaces       = dg.dg_Heads;
        pe->ENV.de_BlocksPerTrack = dg.dg_TrackSectors;
        pe->ENV.de_SectorPerBlock = dg.dg_CylSectors;
        pe->ENV.de_BufMemType     = dg.dg_BufMemType;

		debug( APPLICATIONNAME ": SizeBlock = %ld\n", pe->ENV.de_SizeBlock *4);
		debug( APPLICATIONNAME ": LowCyl = %ld\n", pe->ENV.de_LowCyl);
		debug( APPLICATIONNAME ": HighCyl = %ld\n", pe->ENV.de_HighCyl);
		debug( APPLICATIONNAME ": Surfaces = %ld\n", pe->ENV.de_Surfaces);
		debug( APPLICATIONNAME ": BlocksPerTrack = %ld\n", pe->ENV.de_BlocksPerTrack);
		debug( APPLICATIONNAME ": SectorsPerBlock = %ld\n", pe->ENV.de_SectorPerBlock);
		debug( APPLICATIONNAME ": BufMemType = %ld\n", pe->ENV.de_BufMemType);

    }
    return( 0L);
}
/* \\\ */

#ifdef JustToSeeStructureNames
struct DriveGeometry {
	ULONG	dg_SectorSize;		/* in bytes */
	ULONG	dg_TotalSectors;	/* total # of sectors on drive */
	ULONG	dg_Cylinders;		/* number of cylinders */
	ULONG	dg_CylSectors;		/* number of sectors/cylinder */
	ULONG	dg_Heads;		/* number of surfaces */
	ULONG	dg_TrackSectors;	/* number of sectors/track */
	ULONG	dg_BufMemType;		/* preferred buffer memory type */
					/* (usually MEMF_PUBLIC) */
	UBYTE	dg_DeviceType;		/* codes as defined in the SCSI-2 spec*/
	UBYTE	dg_Flags;		/* flags, including removable */
	UWORD	dg_Reserved;
};

struct DosEnvec {
    ULONG de_TableSize;	     /* Size of Environment vector */
    ULONG de_SizeBlock;	     /* in longwords: Physical disk block size */
    ULONG de_SecOrg;	     /* not used; must be 0 */
    ULONG de_Surfaces;	     /* # of heads (surfaces). drive specific */
    ULONG de_SectorPerBlock; /* N de_SizeBlock sectors per logical block */
    ULONG de_BlocksPerTrack; /* blocks per track. drive specific */
    ULONG de_Reserved;	     /* DOS reserved blocks at start of partition. */
    ULONG de_PreAlloc;	     /* DOS reserved blocks at end of partition */
    ULONG de_Interleave;     /* usually 0 */
    ULONG de_LowCyl;	     /* starting cylinder. typically 0 */
    ULONG de_HighCyl;	     /* max cylinder. drive specific */
    ULONG de_NumBuffers;     /* Initial # DOS of buffers.  */
    ULONG de_BufMemType;     /* type of mem to allocate for buffers */
    ULONG de_MaxTransfer;    /* Max number of bytes to transfer at a time */
    ULONG de_Mask;	     /* Address Mask to block out certain memory */
    LONG  de_BootPri;	     /* Boot priority for autoboot */
    ULONG de_DosType;	     /* ASCII (HEX) string showing filesystem type;
			      * 0X444F5300 is old filesystem,
			      * 0X444F5301 is fast file system */
    ULONG de_Baud;	     /* Baud rate for serial handler */
    ULONG de_Control;	     /* Control word for handler/filesystem */
    ULONG de_BootBlocks;     /* Number of blocks containing boot code */

};
#endif
