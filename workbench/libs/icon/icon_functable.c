/*
        (C) 1995-96 AROS - The Amiga Replacement OS
        *** Automatic generated file. Do not edit ***
        Desc: Funktion table for Icon
        Lang: english
*/
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef NULL
#define NULL ((void *)0)
#endif

void AROS_SLIB_ENTRY(open,Icon) (void);
void AROS_SLIB_ENTRY(close,Icon) (void);
void AROS_SLIB_ENTRY(expunge,Icon) (void);
void AROS_SLIB_ENTRY(null,Icon) (void);
void AROS_SLIB_ENTRY(FreeFreeList,Icon) (void);
void AROS_SLIB_ENTRY(AddFreeList,Icon) (void);
void AROS_SLIB_ENTRY(FindToolType,Icon) (void);
void AROS_SLIB_ENTRY(MatchToolValue,Icon) (void);
void AROS_SLIB_ENTRY(BumpRevision,Icon) (void);
void AROS_SLIB_ENTRY(GetDefDiskObject,Icon) (void);
void AROS_SLIB_ENTRY(PutDefDiskObject,Icon) (void);
void AROS_SLIB_ENTRY(GetDiskObjectNew,Icon) (void);
void AROS_SLIB_ENTRY(DeleteDiskObject,Icon) (void);

void *const Icon_functable[]=
{
    AROS_SLIB_ENTRY(open,Icon), /* 1 */
    AROS_SLIB_ENTRY(close,Icon), /* 2 */
    AROS_SLIB_ENTRY(expunge,Icon), /* 3 */
    AROS_SLIB_ENTRY(null,Icon), /* 4 */
    NULL, /* 5 */
    NULL, /* 6 */
    NULL, /* 7 */
    NULL, /* 8 */
    AROS_SLIB_ENTRY(FreeFreeList,Icon), /* 9 */
    NULL, /* 10 */
    NULL, /* 11 */
    AROS_SLIB_ENTRY(AddFreeList,Icon), /* 12 */
    NULL, /* 13 */
    NULL, /* 14 */
    NULL, /* 15 */
    AROS_SLIB_ENTRY(FindToolType,Icon), /* 16 */
    AROS_SLIB_ENTRY(MatchToolValue,Icon), /* 17 */
    AROS_SLIB_ENTRY(BumpRevision,Icon), /* 18 */
    NULL, /* 19 */
    AROS_SLIB_ENTRY(GetDefDiskObject,Icon), /* 20 */
    AROS_SLIB_ENTRY(PutDefDiskObject,Icon), /* 21 */
    AROS_SLIB_ENTRY(GetDiskObjectNew,Icon), /* 22 */
    AROS_SLIB_ENTRY(DeleteDiskObject,Icon), /* 23 */
    (void *)-1L
};

char Icon_end;
