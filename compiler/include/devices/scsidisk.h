#ifndef DEVICES_SCSIDISK_H
#define DEVICES_SCSIDISK_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for SCSI exec-level device command
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#define HD_WIDESCSI 8
#define HD_SCSICMD  28

struct SCSICmd
{
    UWORD *scsi_Data;
    ULONG  scsi_Length;
    ULONG  scsi_Actual;
    UBYTE *scsi_Command;
    UWORD  scsi_CmdLength;
    UWORD  scsi_CmdActual;
    UBYTE  scsi_Flags;
    UBYTE  scsi_Status;
    UBYTE *scsi_SenseData;
    UWORD  scsi_SenseLength;
    UWORD  scsi_SenseActual;
};

/* scsi_Flags */

#define SCSIF_WRITE 	    0
#define SCSIF_READ  	    1
#define SCSIB_READ_WRITE    0

#define SCSIF_NOSENSE	    0
#define SCSIF_AUTOSENSE     2

#define SCSIF_OLDAUTOSENSE  6

#define SCSIB_AUTOSENSE     1
#define SCSIB_OLDAUTOSENSE  2

/* SCSI io_Error values */

#define HFERR_SelfUnit	    40
#define HFERR_DMA   	    41
#define HFERR_Phase 	    42
#define HFERR_Parity	    43
#define HFERR_SelTimeout    44
#define HFERR_BadStatus     45

/* OpenDevice io_Error values */

#define HFERR_NoBoard	    50

#endif /* DEVICES_SCSIDISK_H */
