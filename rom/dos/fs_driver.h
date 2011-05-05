#ifndef DOS_FS_DRIVER_H
#define DOS_FS_DRIVER_H

/* Reference object types for fs_Open() */
#define REF_LOCK    0
#define REF_DEVICE  1
#define REF_CONSOLE 2

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, CONST_STRPTR name, LONG accessMode, struct DosLibrary *DOSBase);
LONG fs_Open(struct FileHandle *fh, UBYTE reftype, BPTR ref, LONG accessMode, CONST_STRPTR name, struct DosLibrary *DOSBase);
LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase);
LONG fs_AddNotify(struct NotifyRequest *notify, struct DevProc *dvp, BPTR lock, struct DosLibrary *DOSBase);
BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase);
BYTE DosDoIO(struct IORequest *iORequest);

#endif
