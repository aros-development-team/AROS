/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <dos/dosextens.h>
#include <proto/dos.h>

	AROS_LH3(void, SendPkt,

/*  SYNOPSIS */
	AROS_LHA(struct DosPacket *, dp, D1),
	AROS_LHA(struct MsgPort   *, port, D2),
	AROS_LHA(struct MsgPort   *, replyport, D3),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 41, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    /*
     * Trying to emulate the packet system by rewriting the
     * packets to IO Requests. Sometimes there are too many
     * parameters in the packet but thats fine. If there are
     * not enough parameters or the wrong type etc. then
     * it is more difficult to translate the packet.
     *
     */

    struct IOFileSys * iofs = AllocMem(sizeof(struct IOFileSys), MEMF_CLEAR);
    
    if (iofs)
    {
        /*
         * Have to rewrite this packet...
         */
        switch (dp->dp_Type) {
            case ACTION_END:
            break;
        
            case ACTION_READ:
              iofs->IOFS.io_Command = FSA_READ;
              // dp_Arg1 missing
              iofs->io_Union.io_READ.io_Buffer = (UBYTE *)dp->dp_Arg2;
              iofs->io_Union.io_READ.io_Length = dp->dp_Arg3;
            break;

            case ACTION_WRITE:
              iofs->IOFS.io_Command = FSA_WRITE;
              // dp_Arg1 missing
              iofs->io_Union.io_WRITE.io_Buffer = (UBYTE *)dp->dp_Arg2;
              iofs->io_Union.io_WRITE.io_Length = dp->dp_Arg3;
            break;

            case ACTION_SEEK:
              iofs->IOFS.io_Command = FSA_SEEK;
              // dp_Arg1 missing
              iofs->io_Union.io_SEEK.io_Offset   = dp->dp_Arg2;
              iofs->io_Union.io_SEEK.io_SeekMode = dp->dp_Arg3;
            break;
        
            case ACTION_CURRENT_VOLUME:
            break;
        
            case ACTION_SET_FILE_SIZE:
              //iofs->IOFS.
            break;
        
            case ACTION_SAME_LOCK:
              iofs->IOFS.io_Command = FSA_SAME_LOCK;
              // dp_Arg1 missing
              // !!! The following should be a BPTR to struct FileLock!
              iofs->io_Union.io_SAME_LOCK.io_Lock[0] = (char *)dp->dp_Arg1;
              iofs->io_Union.io_SAME_LOCK.io_Lock[1] = (char *)dp->dp_Arg2;
            break;
        
            case ACTION_EXAMINE_FH:
              iofs->IOFS.io_Command = FSA_EXAMINE;
              // dp_Arg1 missing
              // Cannot map the other parameters.
              //iofs->io_Union.io_EXAMINE.
            break;
        
            case ACTION_EXAMINE_NEXT:
              iofs->IOFS.io_Command = FSA_EXAMINE_NEXT;
              // dp_Arg1 missing (not needed so far.)
              iofs->io_Union.io_EXAMINE_NEXT.io_fib = (struct FileInfoBlock *)dp->dp_Arg2;
            break;
        
            case ACTION_EXAMINE_ALL:
              iofs->IOFS.io_Command = FSA_EXAMINE_ALL;
              iofs->io_Union.io_EXAMINE_ALL.io_ead  = (struct ExAllData *)dp->dp_Arg2;
              iofs->io_Union.io_EXAMINE_ALL.io_Size = dp->dp_Arg3;
              iofs->io_Union.io_EXAMINE_ALL.io_Mode = dp->dp_Arg4; // type? 
              // dp_Arg5 missing...
            break;
        
            case ACTION_CREATE_DIR:
              iofs->IOFS.io_Command = FSA_CREATE_DIR;
              // dp_Arg1 missing!
              iofs->io_Union.io_CREATE_DIR.io_Filename = (char *)dp->dp_Arg2;
            break;
        
            case ACTION_RENAME_OBJECT:
              iofs->IOFS.io_Command = FSA_RENAME;
              // dp_Arg1 missing
              iofs->io_Union.io_RENAME.io_Filename = (char *)dp->dp_Arg2;
              // dp_Arg2 missing
              iofs->io_Union.io_RENAME.io_NewName  = (char *)dp->dp_Arg3;
            break;
        }
    }

#warning Must find the io_Device and io_Unit somehow
//    iofs.IOFS.io_Device = 
//    iofs.IOFS.io_Unit = 

    /*
     * Also attach the packet to the io request.
     * Will remain untouched by the driver.
     *
     */
//    iofs.IOFS.io_oldpacket = dp;

//    iofs.IOFS.io_replyport = replyport; // device should use this for reply 

    PutMsg(port, &iofs->IOFS.io_Message);

    AROS_LIBFUNC_EXIT
} /* SendPkt */
