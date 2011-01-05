#ifndef INTERNALLOADSEG_H
#define INTERNALLOADSEG_H

BPTR InternalLoadSeg_AOS(BPTR file,
                         BPTR table,
                         SIPTR * FuncArray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF(BPTR file,
                         BPTR hunk_table,
                         SIPTR * FuncArray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF64(BPTR file,
                         BPTR hunk_table,
                         SIPTR * FuncArray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF_AROS(BPTR file,
                         BPTR hunk_table,
                         SIPTR * FuncArray,
                         SIPTR * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_AOUT(BPTR file,
                          BPTR hunk_table,
                          SIPTR * FuncArray,
                          SIPTR * stack,
                          struct DosLibrary * DOSBase);

void *loadseg_alloc(SIPTR *allocfunc, ULONG size, ULONG req);
void loadseg_free(SIPTR *freefunc, void *buf);
LONG loadseg_read(SIPTR *readfunc, BPTR fh, void *buf, LONG size, struct DosLibrary *DOSBase);

#endif
