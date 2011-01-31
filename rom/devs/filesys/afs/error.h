#ifndef ERROR_H
#define ERROR_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
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

/*
 * showReqStruct: a group describing particular requester: 
 * - text - content to be displayed
 * - type - set of buttons for the requester
 */
struct showReqStruct 
{
        char* text;
        enum showReqType type;
} texts[] =
        {
                {NULL, Req_Cancel },
                {"No ioport", Req_Cancel },
                {"Couldn't open device %s", Req_Cancel },
                {"Couldn't add disk as dosentry", Req_Cancel },
                {"Disk is not validated!", Req_CheckCancel },
                {"Wrong data block %lu", Req_Cancel },
                {"Wrong checksum on block %lu", Req_CheckCancel },
                {"Missing some more bitmap blocks", Req_Cancel },
                {"Wrong blocktype on block %lu", Req_CheckCancel },
                {"Read/Write Error %ld accessing block %lu", Req_Cancel },
                {"*** This may be a non-AFS disk. ***\n"
                        "Any attempt to fix it in this case may render the original\n"
                        "file system invalid, and its contents unrecoverable.\n\n"
                        "Please select what to do", Req_ContinueCancel },        
                {"Block %lu used twice", Req_Cancel},
                {"Block %lu is located outside volume scope\nand will be removed.", Req_Continue},
                {"Repairing disk structure will lead to data loss.\n"
                        "It's best to make a backup before proceeding.\n\n"
                        "Please select what to do.", Req_ContinueCancel },
                {"Volume\n%s\nis write protected", Req_RetryCancel },
                {NULL, Req_Cancel },  // Error has already been reported.
                {"Unknown error", Req_Cancel}
        };



void showText(struct AFSBase *, char *, ...);
LONG showError(struct AFSBase *, ULONG, ...);
LONG showRetriableError(struct AFSBase *, TEXT *, ...);

#endif
