/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Amiga bootloader -- InternalLoadSeg support routines
    Lang: english
*/

extern LONG ils_read(BPTR __d1, void * __d2, LONG __d3, struct DosLibrary * __a6);
extern void *ils_alloc(ULONG __d0, ULONG __d1, struct ExecBase * __a6);
extern void ils_free(void * __a1, ULONG __d0, struct ExecBase * __a6);
