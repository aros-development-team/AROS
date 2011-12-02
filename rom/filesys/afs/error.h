#ifndef ERROR_H
#define ERROR_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/
/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk check option for broken disks
 */


#include "os.h"

/*
 * The various error codes, that can be used as index in texts[], below.
 */
enum {
        ERR_NONE,
        ERR_IOPORT,
        ERR_DEVICE,
        ERR_DOSENTRY,
        ERR_DISKNOTVALID,
        ERR_WRONG_DATA_BLOCK,
        ERR_CHECKSUM,                                                // block errors
        ERR_MISSING_BITMAP_BLOCKS,
        ERR_BLOCKTYPE,
        ERR_READWRITE,
        ERR_POSSIBLY_NOT_AFS,
        ERR_BLOCK_USED_TWICE,
        ERR_BLOCK_OUTSIDE_RANGE,
        ERR_DATA_LOSS_POSSIBLE,
        ERR_WRITEPROTECT,
        ERR_ALREADY_PRINTED,          // That is, this error has already been reported elsewhere.
        ERR_UNKNOWN
};

/*
 * showReqType matches adequate option[] in showPtrArgsText.
 */
enum showReqType 
{
    Req_Cancel = 0,
    Req_RetryCancel,
    Req_CheckCancel,
    Req_ContinueCancel,
    Req_Continue
};

#endif
