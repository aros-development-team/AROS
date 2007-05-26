#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>

int main(int argc, char **argv) {
    struct DevProc *dvp;
    struct IOFileSys iofs;
    struct FileHandle *fh;
    UBYTE buf[256];
    
    if (argc != 2) {
        printf("usage: %s path\n", argv[0]);
        return 1;
    }

    iofs.IOFS.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    iofs.IOFS.io_Message.mn_ReplyPort = &(((struct Process *) FindTask(NULL))->pr_MsgPort);
    iofs.IOFS.io_Message.mn_Length = sizeof(struct IOFileSys);
    iofs.IOFS.io_Flags = 0;

    iofs.io_Union.io_OPEN.io_Filename = "";
    iofs.io_Union.io_OPEN.io_FileMode = FMF_READ;

    dvp = NULL;
    while ((dvp = GetDeviceProc(argv[1], dvp)) != NULL) {
        iofs.IOFS.io_Command = FSA_OPEN;
        iofs.IOFS.io_Device = (struct Device *) dvp->dvp_Port;

        if (dvp->dvp_Lock != NULL)
            iofs.IOFS.io_Unit = ((struct FileHandle *) BADDR(dvp->dvp_Lock))->fh_Unit;
        else
            iofs.IOFS.io_Unit = dvp->dvp_DevNode->dol_Ext.dol_AROS.dol_Unit;

        DoIO(&iofs.IOFS);

        if (iofs.io_DosError == 0) {
            fh = AllocDosObject(DOS_FILEHANDLE, NULL);
            fh->fh_Device = iofs.IOFS.io_Device;
            fh->fh_Unit = iofs.IOFS.io_Unit;

            if (NameFromLock(MKBADDR(fh), buf, sizeof(buf)) == DOSFALSE) {
                Fault(IoErr(), NULL, buf, sizeof(buf));
                printf("found file, but couldn't get lock name: %s\n", buf);
            }
            else
                printf("found in: %s\n", buf);

            Close(MKBADDR(fh));
        }
        
        else if (iofs.io_DosError != ERROR_OBJECT_NOT_FOUND) {
            Fault(IoErr(), NULL, buf, sizeof(buf));
            printf("error expanding name: %s\n", buf);
            FreeDeviceProc(dvp);
            return 1;
        }
    }

    FreeDeviceProc(dvp);

    return 0;
}
