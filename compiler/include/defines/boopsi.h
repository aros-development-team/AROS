
/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $id: $

    Desc: Defines for boopsi.library
    Lang: english
*/

#ifndef DEFINES_BOOPSI_H
#define DEFINES_BOOPSI_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#ifdef AddClass
#   undef AddClass
#endif
#ifdef DisposeObject
#   undef DisposeObject
#endif
#ifdef FreeClass
#   undef FreeClass
#endif
#ifdef GetAttr
#   undef GetAttr
#endif
#ifdef MakeClass
#   undef MakeClass
#endif
#ifdef NewObjectA
#   undef NewObjectA
#endif
#ifdef NextObject
#   undef NextObject
#endif
#ifdef RemoveClass
#   undef RemoveClass
#endif
#ifdef SetAttrsA
#   undef SetAttrsA
#endif

/*
    Defines
*/
#define AddClass(classPtr) \
    AROS_LC1(void, AddClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct Library *, BOOPSIBase, 5, BOOPSI)

#define DisposeObject(object) \
    AROS_LC1(void, DisposeObject, \
    AROS_LCA(APTR, object, A0), \
    struct Library *, BOOPSIBase, 6, BOOPSI)

#define FindClass(classID) \
    AROS_LC1(struct IClass *, FindClass, \
    AROS_LCA(ClassID,	classID, A0), \
    struct Library *, BOOPSIBase, 7, BOOPSI)

#define FreeClass(classPtr) \
    AROS_LC1(BOOL, FreeClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct Library *, BOOPSIBase, 8, BOOPSI)

#define GetAttr(attrID, object, storagePtr) \
    AROS_LC3(ULONG, GetAttr, \
    AROS_LCA(ULONG   , attrID, D0), \
    AROS_LCA(Object *, object, A0), \
    AROS_LCA(IPTR *  , storagePtr, A1), \
    struct Library *, BOOPSIBase, 9, BOOPSI)

#define MakeClass(classID, superClassID, superClassPtr, instanceSize, flags) \
    AROS_LC5(struct IClass *, MakeClass, \
    AROS_LCA(UBYTE         *, classID, A0), \
    AROS_LCA(UBYTE         *, superClassID, A1), \
    AROS_LCA(struct IClass *, superClassPtr, A2), \
    AROS_LCA(ULONG          , instanceSize, D0), \
    AROS_LCA(ULONG          , flags, D1), \
    struct Library *, BOOPSIBase, 10, BOOPSI)

#define NewObjectA(classPtr, classID, tagList) \
    AROS_LC3(APTR, NewObjectA, \
    AROS_LCA(struct IClass  *, classPtr, A0), \
    AROS_LCA(UBYTE          *, classID, A1), \
    AROS_LCA(struct TagItem *, tagList, A2), \
    struct Library *, BOOPSIBase, 11, BOOPSI)

#define NextObject(objectPtrPtr) \
    AROS_LC1(APTR, NextObject, \
    AROS_LCA(APTR, objectPtrPtr, A0), \
    struct Library *, BOOPSIBase, 12, BOOPSI)

#define RemoveClass(classPtr) \
    AROS_LC1(void, RemoveClass, \
    AROS_LCA(struct IClass *, classPtr, A0), \
    struct Library *, BOOPSIBase, 13, BOOPSI)

#define SetAttrsA(object, tagList) \
    AROS_LC2(ULONG, SetAttrsA, \
    AROS_LCA(APTR            , object, A0), \
    AROS_LCA(struct TagItem *, tagList, A1), \
    struct Library *, BOOPSIBase, 14, BOOPSI)


#endif /* DEFINES_BOOPSI_H */
