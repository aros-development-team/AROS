#ifndef CLIB_ICON_PROTOS_H
#define CLIB_ICON_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for icon.library
    Lang: english
*/
#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Prototypes
*/
AROS_LP3(BOOL, AddFreeList,
    AROS_LPA(struct FreeList *, freelist, A0),
    AROS_LPA(APTR             , mem, A1),
    AROS_LPA(unsigned long    , size, A2),
    struct Library *, IconBase, 12, Icon)

AROS_LP2(UBYTE *, BumpRevision,
    AROS_LPA(UBYTE *, newname, A0),
    AROS_LPA(UBYTE *, oldname, A1),
    struct Library *, IconBase, 18, Icon)

AROS_LP1(BOOL, DeleteDiskObject,
    AROS_LPA(UBYTE *, name, A0),
    struct Library *, IconBase, 23, Icon)

AROS_LP2(UBYTE *, FindToolType,
    AROS_LPA(UBYTE **, toolTypeArray, A0),
    AROS_LPA(UBYTE  *, typeName, A1),
    struct Library *, IconBase, 16, Icon)

AROS_LP1(void, FreeDiskObject,
    AROS_LPA(struct DiskObject *, diskobj, A0),
    struct Library *, IconBase, 15, Icon)

AROS_LP1(void, FreeFreeList,
    AROS_LPA(struct FreeList *, freelist, A0),
    struct Library *, IconBase, 9, Icon)

AROS_LP1(struct DiskObject *, GetDefDiskObject,
    AROS_LPA(long, type, D0),
    struct Library *, IconBase, 20, Icon)

AROS_LP1(struct DiskObject *, GetDiskObject,
    AROS_LPA(UBYTE *, name, A0),
    struct Library *, IconBase, 13, Icon)

AROS_LP1(struct DiskObject *, GetDiskObjectNew,
    AROS_LPA(UBYTE *, name, A0),
    struct Library *, IconBase, 22, Icon)

AROS_LP2(BOOL, MatchToolValue,
    AROS_LPA(UBYTE *, typeString, A0),
    AROS_LPA(UBYTE *, value, A1),
    struct Library *, IconBase, 17, Icon)

AROS_LP1(BOOL, PutDefDiskObject,
    AROS_LPA(struct DiskObject *, diskObject, A0),
    struct Library *, IconBase, 21, Icon)

AROS_LP2(BOOL, PutDiskObject,
    AROS_LPA(UBYTE             *, name, A0),
    AROS_LPA(struct DiskObject *, diskobj, A1),
    struct Library *, IconBase, 14, Icon)


#endif /* CLIB_ICON_PROTOS_H */
