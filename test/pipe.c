#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <dos/filesystem.h>
#include <dos/dos.h>
#include <exec/memory.h>

#include <assert.h>

static struct IOFileSys *CreateIOFS(ULONG type, struct MsgPort *port, struct FileHandle *fh)
{
    struct IOFileSys *iofs = (struct IOFileSys *)AllocMem(sizeof(struct IOFileSys), MEMF_PUBLIC|MEMF_CLEAR);

    if (iofs == NULL)
	return NULL;
  
    iofs->IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs->IOFS.io_Message.mn_ReplyPort    = port;
    iofs->IOFS.io_Message.mn_Length       = sizeof(struct IOFileSys);
    iofs->IOFS.io_Command                 = type;
    iofs->IOFS.io_Flags                   = 0;
    iofs->IOFS.io_Device                  = fh->fh_Device;
    iofs->IOFS.io_Unit                    = fh->fh_Unit;
  
    return iofs;
}

static BPTR DupFH(BPTR fh, LONG mode)
{
    BPTR ret = NULL;

    if (fh)
    {
        BPTR olddir = CurrentDir(fh);
        ret    = Open("", mode);

        CurrentDir(olddir);
    }

    return ret;
}

int main(void)
{
    struct FileHandle *fhin, *fhout;
    struct MsgPort *port;
    struct IOFileSys *iofs;
    
    fhin = Open("PIPEFS:__UNNAMED__", FMF_READ|FMF_NONBLOCK);
    fhout = DupFH(fhin, FMF_WRITE);
    assert(fhin!=NULL && fhout!=NULL);
    ChangeMode(CHANGE_FH, fhin, FMF_READ);
    
    port = CreatePort(NULL, 0);
    assert(port!=NULL);
    
    iofs = CreateIOFS(FSA_WRITE, port, fhout);
    iofs->io_Union.io_WRITE.io_Buffer = "Test\n";
    iofs->io_Union.io_WRITE.io_Length = 5;
    
    SendIO(&iofs->IOFS);
    
    Close(fhin);

    iofs = CreateIOFS(FSA_WRITE, port, fhout);
    iofs->io_Union.io_WRITE.io_Buffer = "Test\n";
    iofs->io_Union.io_WRITE.io_Length = 5;

    SendIO(&iofs->IOFS);

    Close(fhout);
    DeletePort(port);

    return 0;
}
