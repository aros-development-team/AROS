
#if (0)
/*
 *      Replacement for the dos.library LoadSeg() function
 */
AROS_LH1(BPTR, NEWLoadSeg,
        AROS_LHA(CONST_STRPTR, name, D1),
        struct DosLibrary *, DOSBase, 25, Dos)
{
    AROS_LIBFUNC_INIT

    BPTR fl;
    struct FileInfoBlock *fib;
    ULONG owner = secOWNER_NOBODY;
    BPTR seglist;

    if (name && (fl = Lock(name, ACCESS_READ))) {
        if (CheckmuFSVolume(((struct FileLock *)BADDR(fl))->fl_Task) &&
             (fib = AllocDosObject(DOS_FIB, NULL))) {
            if (Examine(fl, fib) && (fib->fib_Protection & muFIBF_SET_UID))
                owner = (fib->fib_OwnerUID<<16) | fib->fib_OwnerGID;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(fl);
    }
    seglist = secBase->OLDLoadSeg(name, dosbase);
    if ((owner & secMASK_UID) != (secNOBODY_UID << 16)) {
        ObtainSemaphore(&secBase->SegOwnerSem);
        AddSegNode(seglist, owner);
        ReleaseSemaphore(&secBase->SegOwnerSem);
    }
    return(seglist);

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library NewLoadSeg() function
 */
AROS_LH2(BPTR, NEWNewLoadSeg,
        AROS_LHA(CONST_STRPTR, file, D1),
        AROS_LHA(const struct TagItem *, tags, D2),
        struct DosLibrary *, DOSBase, 128, Dos)
{
    AROS_LIBFUNC_INIT

    BPTR fl;
    struct FileInfoBlock *fib;
    ULONG owner = secOWNER_NOBODY;
    BPTR seglist;

    if (name && (fl = Lock(name, ACCESS_READ))) {
        if (CheckmuFSVolume(((struct FileLock *)BADDR(fl))->fl_Task) &&
             (fib = AllocDosObject(DOS_FIB, NULL))) {
            if (Examine(fl, fib) && (fib->fib_Protection & muFIBF_SET_UID))
                owner = (fib->fib_OwnerUID<<16) | fib->fib_OwnerGID;
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(fl);
    }
    seglist = secBase->OLDNewLoadSeg(name, tags, dosbase);
    if ((owner & secMASK_UID) != (secNOBODY_UID << 16)) {
        ObtainSemaphore(&secBase->SegOwnerSem);
        AddSegNode(seglist, owner);
        ReleaseSemaphore(&secBase->SegOwnerSem);
    }
    return(seglist);

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library UnLoadSeg() function
 */
AROS_LH1(BOOL, NEWUnLoadSeg,
        AROS_LHA(BPTR, seglist, D1),
        struct DosLibrary *, DOSBase, 26, Dos)
{
    AROS_LIBFUNC_INIT

    struct muSegNode *snode;

    ObtainSemaphoreShared(&secBase->SegOwnerSem);
    if ((snode = FindSegNode(seglist)))
        RemSegNode(snode);
    ReleaseSemaphore(&secBase->SegOwnerSem);
    return(secBase->OLDUnLoadSeg(seglist, dosbase));

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library InternalLoadSeg() function
 */
AROS_LH4(BPTR, NEWInternalLoadSeg,
        AROS_LHA(BPTR       , fh           , D0),
        AROS_LHA(BPTR       , table        , A0),
        AROS_LHA(LONG_FUNC *, funcarray    , A1),
        AROS_LHA(LONG *     , stack        , A2),
        struct DosLibrary *, DOSBase, 126, Dos)
{
    AROS_LIBFUNC_INIT

    struct FileInfoBlock *fib;
    ULONG owner = secOWNER_NOBODY;
    BPTR seglist;

    D(kprintf("InternalLoadSeg()\n"));
    if (fh && CheckmuFSVolume(((struct FileHandle *)BADDR(fh))->fh_Type) &&
         (fib = AllocDosObject(DOS_FIB, NULL))) {
        if (ExamineFH(fh, fib) && (fib->fib_Protection & muFIBF_SET_UID))
            owner = (fib->fib_OwnerUID<<16) | fib->fib_OwnerGID;
        FreeDosObject(DOS_FIB, fib);
    }
    seglist = secBase->OLDInternalLoadSeg(fh, table, functionarray, stack, dosbase);
    if ((owner & secMASK_UID) != (secNOBODY_UID << 16)) {
        ObtainSemaphore(&secBase->SegOwnerSem);
        AddSegNode(seglist, owner);
        ReleaseSemaphore(&secBase->SegOwnerSem);
    }
    D(kprintf("InternalLoadSeg() done\n"));
    return(seglist);

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library InternalUnLoadSeg() function
 */
AROS_LH2(BOOL, NEWInternalUnLoadSeg,
        AROS_LHA(BPTR     , seglist , D1),
        AROS_LHA(VOID_FUNC, freefunc, A1),
        struct DosLibrary *, DOSBase, 127, Dos)
{
    AROS_LIBFUNC_INIT

    struct muSegNode *snode;

    D(kprintf("InternalUnLoadSeg()\n"));
    ObtainSemaphoreShared(&secBase->SegOwnerSem);
    if ((snode = FindSegNode(seglist)))
            RemSegNode(snode);
    ReleaseSemaphore(&secBase->SegOwnerSem);
    D(kprintf("InternalUnLoadSeg() done\n"));

    return(secBase->OLDInternalUnLoadSeg(seglist, freefunc, dosbase));

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library CreateProc() function
 */
AROS_LH4(struct MsgPort *, NEWCreateProc,
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(LONG, pri, D2),
        AROS_LHA(BPTR, segList, D3),
        AROS_LHA(LONG, stackSize, D4),
        struct DosLibrary *, DOSBase, 23, Dos)
{
    AROS_LIBFUNC_INIT

    struct muExtOwner *owner;
    struct muTaskNode *tasknode = NULL;
    struct Process *proc;

    D(kprintf("CreateProc()\n"));
    if ((owner = GetSegOwner(seglist)))
    {
        ObtainSemaphore(&secBase->TaskOwnerSem);
        if ( (tasknode = FindTaskNode(FindTask(NULL))) )
        {
            if (tasknode->Owner)
            {
                if (tasknode->Owner->uid != owner->uid)
                {
                    tasknode->SavedUID = tasknode->Owner->uid;
                    tasknode->Owner->uid = owner->uid;
                }
            }
            else
            {
                if ( (tasknode->Owner = CloneExtOwner(&RootExtOwner)) )
                {
                    tasknode->SavedUID = secNOBODY_UID;
                    tasknode->Owner->uid = owner->uid;
                    tasknode->Owner->gid = secNOBODY_UID;
                }
            }
        }
    }
    proc = secBase->OLDCreateProc(name, pri, seglist, stacksize, dosbase);
    if (owner)
    {
        if (tasknode && tasknode->Owner->uid != tasknode->SavedUID)
            if (tasknode->SavedUID == secNOBODY_UID)
                secFreeExtOwner(tasknode->Owner);
            else
                tasknode->Owner->uid = tasknode->SavedUID;
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    D(kprintf("CreateProc() done\n"));
    return proc;

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library CreateNewProc() function
 */
AROS_LH1(struct Process *, NEWCreateNewProc,
        AROS_LHA(const struct TagItem *, tags, D1),
        struct DosLibrary *, DOSBase, 83, Dos)
{
    AROS_LIBFUNC_INIT

    struct muTaskNode *tasknode = NULL;
    struct muExtOwner *owner = NULL;
    struct Process *proc;
    BPTR seglist;

    D(kprintf("CreateNewProc()\n"));
    if (tags && (seglist = (BPTR)GetTagData(NP_Seglist, NULL, tags)) && (owner = GetSegOwner(seglist)))
    {
        ObtainSemaphore(&secBase->TaskOwnerSem);
        if ( (tasknode = FindTaskNode(FindTask(NULL))) )
        {
            if (tasknode->Owner)
            {
                if (tasknode->Owner->uid != owner->uid)
                {
                    tasknode->SavedUID = tasknode->Owner->uid;
                    tasknode->Owner->uid = owner->uid;
                }
            }
            else
            {
                tasknode->Owner = CloneExtOwner(&RootExtOwner);
                tasknode->SavedUID = secNOBODY_UID;
                tasknode->Owner->uid = owner->uid;
                tasknode->Owner->gid = secNOBODY_UID;
            }
        }
    }
    proc = secBase->OLDCreateNewProc(tags, dosbase);
    if (owner)
    {
        if (tasknode && tasknode->Owner->uid != tasknode->SavedUID)
            if (tasknode->SavedUID == secNOBODY_UID)
                secFreeExtOwner(tasknode->Owner);
            else
                tasknode->Owner->uid = tasknode->SavedUID;
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    D(kprintf("CreateNewProc done()\n"));
    return(proc);

    AROS_LIBFUNC_EXIT
}

/*
 *      Replacement for the dos.library RunCommand() function
 */
AROS_LH4(LONG, NEWRunCommand,
        AROS_LHA(BPTR,   segList,   D1),
        AROS_LHA(ULONG,  stacksize, D2),
        AROS_LHA(CONST_STRPTR, argptr,    D3),
        AROS_LHA(ULONG,  argsize,   D4),
        struct DosLibrary *, DOSBase, 84, Dos)
{
    AROS_LIBFUNC_INIT

    struct muTaskNode *tasknode = NULL;
    struct muExtOwner *owner;
    LONG rc;

    D(kprintf("RunCommand()\n"));
    if ((owner = GetSegOwner(seglist)))
    {
        D(kprintf("SegOwner found\n"));
        ObtainSemaphore(&secBase->TaskOwnerSem);
        if ( (tasknode = FindTaskNode(FindTask(NULL))) )
        {
            if (tasknode->Owner)
            {
                if (tasknode->Owner->uid != owner->uid)
                {
                    tasknode->SavedUID = tasknode->Owner->uid;
                    tasknode->Owner->uid = owner->uid;
                }
            }
            else
            {
                tasknode->Owner = CloneExtOwner(&RootExtOwner);
                tasknode->SavedUID = secNOBODY_UID;
                tasknode->Owner->uid = owner->uid;
                tasknode->Owner->gid = secNOBODY_UID;
            }
        }
    }
    rc = secBase->OLDRunCommand(seglist, stacksize, argptr, argsize, dosbase);
    if (owner)
    {
        if (tasknode && tasknode->Owner->uid != tasknode->SavedUID)
            if (tasknode->SavedUID == secNOBODY_UID)
                secFreeExtOwner(tasknode->Owner);
            else
                tasknode->Owner->uid = tasknode->SavedUID;
        ReleaseSemaphore(&secBase->TaskOwnerSem);
    }
    D(kprintf("RunCommand() done\n"));
    return(rc);

    AROS_LIBFUNC_EXIT
}
#endif
