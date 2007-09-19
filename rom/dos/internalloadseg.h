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

#endif
