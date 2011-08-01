#ifndef DOS_FS_DRIVER_H
#define DOS_FS_DRIVER_H

#include <dos/notify.h>

LONG fs_LocateObject(BPTR *ret, struct MsgPort *port, BPTR parent, CONST_STRPTR name, LONG accessMode, struct DosLibrary *DOSBase);
LONG fs_Open(struct FileHandle *fh, struct MsgPort *port, BPTR lock, LONG accessMode, CONST_STRPTR name, struct DosLibrary *DOSBase);
LONG fs_ReadLink(BPTR parent, struct DevProc *dvp, CONST_STRPTR path, STRPTR buffer, ULONG size, struct DosLibrary *DOSBase);
LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase);
LONG fs_AddNotify(struct NotifyRequest *notify, struct DevProc *dvp, BPTR lock, struct DosLibrary *DOSBase);
BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase);
BYTE DosDoIO(struct IORequest *iORequest);

#endif
