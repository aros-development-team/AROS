#ifndef DEFINES_ICON_H
#define DEFINES_ICON_H

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif

/*
    Defines
*/
#define AddFreeList(freelist, mem, size) \
    AROS_LC3(BOOL, AddFreeList, \
    AROS_LCA(struct FreeList *, freelist, A0), \
    AROS_LCA(APTR             , mem, A1), \
    AROS_LCA(unsigned long    , size, A2), \
    struct Library *, IconBase, 12, Icon)

#define BumpRevision(newname, oldname) \
    AROS_LC2(UBYTE *, BumpRevision, \
    AROS_LCA(UBYTE *, newname, A0), \
    AROS_LCA(UBYTE *, oldname, A1), \
    struct Library *, IconBase, 18, Icon)

#define DeleteDiskObject(name) \
    AROS_LC1(BOOL, DeleteDiskObject, \
    AROS_LCA(UBYTE *, name, A0), \
    struct Library *, IconBase, 23, Icon)

#define FindToolType(toolTypeArray, typeName) \
    AROS_LC2(UBYTE *, FindToolType, \
    AROS_LCA(UBYTE **, toolTypeArray, A0), \
    AROS_LCA(UBYTE  *, typeName, A1), \
    struct Library *, IconBase, 16, Icon)

#define FreeFreeList(freelist) \
    AROS_LC1(void, FreeFreeList, \
    AROS_LCA(struct FreeList *, freelist, A0), \
    struct Library *, IconBase, 9, Icon)

#define GetDefDiskObject(type) \
    AROS_LC1(struct DiskObject *, GetDefDiskObject, \
    AROS_LCA(long, type, D0), \
    struct Library *, IconBase, 20, Icon)

#define GetDiskObject(name) \
    AROS_LC1(struct DiskObject *, GetDiskObject, \
    AROS_LCA(UBYTE *, name, A0), \
    struct Library *, IconBase, 13, Icon)

#define GetDiskObjectNew(name) \
    AROS_LC1(struct DiskObject *, GetDiskObjectNew, \
    AROS_LCA(UBYTE *, name, A0), \
    struct Library *, IconBase, 22, Icon)

#define MatchToolValue(typeString, value) \
    AROS_LC2(BOOL, MatchToolValue, \
    AROS_LCA(UBYTE *, typeString, A0), \
    AROS_LCA(UBYTE *, value, A1), \
    struct Library *, IconBase, 17, Icon)

#define PutDefDiskObject(diskObject) \
    AROS_LC1(BOOL, PutDefDiskObject, \
    AROS_LCA(struct DiskObject *, diskObject, A0), \
    struct Library *, IconBase, 21, Icon)


#endif /* DEFINES_ICON_H */
