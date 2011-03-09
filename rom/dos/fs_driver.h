#ifndef DOS_FS_DRIVER_H
#define DOS_FS_DRIVER_H

/* Reference object types for fs_Open() */
#define REF_LOCK    0
#define REF_DEVICE  1
#define REF_CONSOLE 2

LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, STRPTR name, LONG accessMode, struct DosLibrary *DOSBase);
LONG fs_Open(struct FileHandle *fh, UBYTE reftype, APTR ref, LONG accessMode, STRPTR name, struct DosLibrary *DOSBase);
LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase);
BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase);
BYTE DosDoIO(struct IORequest *iORequest);

#endif
