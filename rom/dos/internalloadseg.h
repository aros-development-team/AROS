#ifndef INTERNALLOADSEG_H
#define INTERNALLOADSEG_H

BPTR InternalLoadSeg_AOS(BPTR file,
                         BPTR table,
                         SIPTR * funcarray,
                         LONG  * stacksize,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF(BPTR file,
                         BPTR hunk_table,
                         SIPTR * funcarray,
                         LONG  * stacksize,
                         struct DosLibrary * DOSBase);

int read_block(BPTR file, APTR buffer, ULONG size, SIPTR * funcarray, struct DosLibrary * DOSBase);

/* AllocVec() simulation using allocation function from the supplied array */
APTR _ilsAllocVec(SIPTR *funcarray, ULONG size, ULONG flags);
void _ilsFreeVec(SIPTR *funcarray, void *buf);

#define ilsAllocVec(size, flags) _ilsAllocVec(funcarray, size, flags)
#define ilsFreeVec(ptr)          _ilsFreeVec(funcarray, ptr)

#define ilsRead(file, buf, size)     \
    AROS_UFC4                        \
    (                                \
        LONG, (APTR)(funcarray[0]),  \
        AROS_UFCA(BPTR,   file, D1), \
        AROS_UFCA(void *, buf,  D2), \
        AROS_UFCA(LONG,   size, D3), \
        AROS_UFCA(struct DosLibrary *, DOSBase, A6) \
    )

#define ilsAllocMem(size, flags)        \
    AROS_UFC3                        \
    (                                \
        void *, (APTR)(funcarray[1]),\
        AROS_UFCA(ULONG, size,  D0), \
        AROS_UFCA(ULONG, flags, D1), \
        AROS_UFCA(struct ExecBase *, SysBase, A6) \
    )				    

#define ilsFreeMem(addr, size)          \
    AROS_UFC3NR                      \
    (                                \
        void, (APTR)(funcarray[2]),  \
        AROS_UFCA(void *, addr, A1), \
        AROS_UFCA(ULONG,  size, D0), \
        AROS_UFCA(struct ExecBase *, SysBase, A6) \
    )

#define ilsSeek(file, pos, mode)     \
    AROS_UFC4                        \
    (                                \
        LONG, (APTR)(funcarray[3]),  \
        AROS_UFCA(BPTR,   file, D1), \
        AROS_UFCA(LONG,    pos, D2), \
        AROS_UFCA(LONG,   mode, D3), \
        AROS_UFCA(struct DosLibrary *, DOSBase, A6) \
    )

#endif
