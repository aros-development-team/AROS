#ifndef INTERNALLOADSEG_H
#define INTERNALLOADSEG_H

BPTR InternalLoadSeg_AOS(BPTR file,
                         BPTR table,
                         LONG * FuncArray,
                         LONG * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF(BPTR file,
                         BPTR hunk_table,
                         LONG * FuncArray,
                         LONG * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_ELF_relexe(BPTR file,
                         BPTR hunk_table,
                         LONG * FuncArray,
                         LONG * stack,
                         struct DosLibrary * DOSBase);

BPTR InternalLoadSeg_AOUT(BPTR file,
                          BPTR hunk_table,
                          LONG * FuncArray,
                          LONG * stack,
                          struct DosLibrary * DOSBase);

#endif
