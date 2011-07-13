#ifndef INTERNALLOADSEG_H
#define INTERNALLOADSEG_H

BPTR LoadSegment_AOS(BPTR file, BPTR table,
                         SIPTR * funcarray,
                         LONG  * stacksize,
                         SIPTR * error,
                         struct Library *lib);

BPTR LoadSegment_ELF(BPTR file, BPTR table,
                         SIPTR * funcarray,
                         LONG  * stacksize,
                         SIPTR * error,
                         struct Library *lib);

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
        AROS_UFCA(struct Library *, lib, A6) \
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
    AROS_UFC3                        \
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
        AROS_UFCA(struct Library *, lib, A6) \
    )

#endif
