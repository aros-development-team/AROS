#ifndef PROTOS_H
#define PROTOS_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#ifndef TYPES_H
#   include "types.h"
#endif
#ifndef OOP_H
#   include "oop.h"
#endif
#ifndef SYSDEP_SYSDEP_H
#   include "sysdep/sysdep.h"
#endif

/* Prototypes */
Class *MakeClass(STRPTR classID, STRPTR superID, struct InterfaceDescr *ifDescr, ULONG instDataSize);
VOID FreeClass(Class *cl);
VOID AddClass(Class *cl);
VOID RemoveClass(Class *cl);
Object *NewObject(Class *cl, STRPTR classID, Msg msg);
VOID DisposeObject(Object *obj);

BOOL InitOOP();
VOID CleanupOOP();

IPTR CoerceMethodA(Class *cl, Object *o, Msg msg);
IPTR DoMethodA(Object *o, Msg msg);
IPTR DoSuperMethodA(Class *cl, Object *o, Msg msg);
BOOL GetMethod(Object *o, ULONG methodID, IPTR (**methodPtrPtr)(), Class **classPtrPtr);

#endif /* PROTOS_H */
