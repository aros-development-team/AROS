#ifndef APPLICATION_PO_H
#define APPLICATION_PO_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: The PrefsObject behind the opaque PrefsObject pointer.
*/

#include <exec/types.h>

struct PObj
{
    ULONG po_Type;      /* ALPOT_ */
    LONG  po_Retain;
    union
    {
        struct { STRPTR s; ULONG len; } str;
        struct { ULONG type; LONG l; double d; } num;  /* l for Long and Bool */
        struct { ULONG secs; ULONG micro; } date;        /* since 1978 */
        struct { UBYTE *data; LONG size; } bin;
        struct { struct PObj **items; STRPTR *keys; LONG count, cap; } con;
    } u;
};

#define PO(x) ((struct PObj *)(x))

PrefsObject *po_New(ULONG type);
PrefsObject *po_Dispatch(ULONG want, PrefsObject *obj, ULONG *error, struct TagItem *tags);
void po_FormatLong(STRPTR buf, LONG v);
void po_FormatDouble(STRPTR buf, double d);
void po_FormatDate(STRPTR buf, ULONG secs);
BOOL po_ParseNumber(CONST_STRPTR s, LONG *l, double *d, BOOL *isdouble);
BOOL po_ParseDate(CONST_STRPTR s, ULONG *secs);

#endif /* APPLICATION_PO_H */
