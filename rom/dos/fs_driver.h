LONG fs_LocateObject(BPTR *ret, BPTR parent, struct DevProc *dvp, STRPTR name, LONG accessMode, struct DosLibrary *DOSBase);
LONG fs_ChangeSignal(BPTR handle, struct Process *task, struct DosLibrary *DOSBase);
BPTR DupFH(BPTR fh, LONG mode, struct DosLibrary * DOSBase);
BYTE DosDoIO(struct IORequest *iORequest);
