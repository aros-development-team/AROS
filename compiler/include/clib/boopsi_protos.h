#ifndef CLIB_BOOPSI_PROTOS_H
#define CLIB_BOOPSI_PROTOS_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Prototypes for boopsi.library
    Lang: english
*/

#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif

/* Prototypes for stubs in amiga.lib */
#ifndef CLIB_INTUITION_PROTOS_H /* Prevent name clashes */
ULONG SetAttrs (APTR obj, ULONG tag1, ...);
ULONG SetSuperAttrs (Class * cl, Object * obj, ULONG tag1, ...);
APTR NewObject (Class * classPtr, UBYTE * classID, ULONG tag1, ...);
IPTR DoMethodA (Object * obj, Msg message);
IPTR DoMethod (Object * obj, ULONG MethodID, ...);
IPTR DoSuperMethodA (Class  * cl, Object * obj, Msg message);
IPTR DoSuperMethod (Class * cl, Object * obj, ULONG MethodID, ...);
IPTR CoerceMethodA (Class * cl, Object * obj, Msg msg);
IPTR CoerceMethod (Class * cl, Object * obj, ULONG MethodID, ...);
#endif

/*
    Prototypes
*/
AROS_LP1(void, AddClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct Library *, BOOPSIBase, 5, BOOPSI)

AROS_LP1(void, DisposeObject,
    AROS_LPA(APTR, object, A0),
    struct Library *, BOOPSIBase, 6, BOOPSI)

AROS_LP1(struct IClass *, FindClass,
    AROS_LPA(ClassID,   classID, A0),
    struct Library *, BOOPSIBase, 7, BOOPSI)

AROS_LP1(BOOL, FreeClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct Library *, BOOPSIBase, 8, BOOPSI)

AROS_LP3(ULONG, GetAttr,
    AROS_LPA(ULONG   , attrID, D0),
    AROS_LPA(Object *, object, A0),
    AROS_LPA(IPTR *  , storagePtr, A1),
    struct Library *, BOOPSIBase, 9, BOOPSI)

AROS_LP5(struct IClass *, MakeClass,
    AROS_LPA(UBYTE         *, classID, A0),
    AROS_LPA(UBYTE         *, superClassID, A1),
    AROS_LPA(struct IClass *, superClassPtr, A2),
    AROS_LPA(ULONG          , instanceSize, D0),
    AROS_LPA(ULONG          , flags, D1),
    struct Library *, BOOPSIBase, 10, BOOPSI)

AROS_LP3(APTR, NewObjectA,
    AROS_LPA(struct IClass  *, classPtr, A0),
    AROS_LPA(UBYTE          *, classID, A1),
    AROS_LPA(struct TagItem *, tagList, A2),
    struct Library *, BOOPSIBase, 11, BOOPSI)

AROS_LP1(APTR, NextObject,
    AROS_LPA(APTR, objectPtrPtr, A0),
    struct Library *, BOOPSIBase, 12, BOOPSI)

AROS_LP1(void, RemoveClass,
    AROS_LPA(struct IClass *, classPtr, A0),
    struct Library *, BOOPSIBase, 13, BOOPSI)

AROS_LP2(ULONG, SetAttrsA,
    AROS_LPA(APTR            , object, A0),
    AROS_LPA(struct TagItem *, tagList, A1),
    struct Library *, BOOPSIBase, 14, BOOPSI)


#endif /* CLIB_BOOPSI_PROTOS_H */
