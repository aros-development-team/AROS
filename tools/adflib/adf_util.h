#ifndef _ADF_UTIL_H
#define _ADF_UTIL_H 1

/*
 *  ADF Library. (C) 1997-1999 Laurent Clevy
 *
 *  adf_util.h
 *
 */

#include "adf_str.h"


void swLong(unsigned char* buf, ULONG val);
void swShort(unsigned char* buf, USHORT val);

PREFIX struct List* newCell(struct List* list, void* content);
PREFIX void freeList(struct List* list);
void adfDays2Date(ULONG days, int *yy, int *mm, int *dd);
BOOL adfIsLeap(int y);
    void
adfTime2AmigaTime(struct DateTime dt, ULONG *day, ULONG *min, ULONG *ticks );
    struct DateTime
adfGiveCurrentTime( void );

void dumpBlock(unsigned char *buf);

/*##########################################################################*/
#endif /* _ADF_UTIL_H */

