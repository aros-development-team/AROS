#ifndef ERRSTRINGS_H
#define ERRSTRINGS_H


/*
 * Shares between aros and unix the array texts[], with provides
 *  error strings and requester types.
 *  The data is in this header file to avoid further splitting
 *  up into a header file and a code file. This is possible,
 *  here, as this file should always be included in just one
 *  file, the source code for error handling.
 */


/*
 * showReqStruct: a group describing particular requester: 
 * - text - content to be displayed
 * - type - set of buttons for the requester
 */
const struct showReqStruct 
{
        const char* text;
        enum showReqType type;
} const texts[] =
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
/*
 * This array can be accessed with an enum of error codes. These
 *  are included in error.h.
 */

#endif
