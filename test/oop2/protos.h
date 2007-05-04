#ifndef PROTOS_H
#define PROTOS_H

/*
   Copyright © 1997-98, The AROS Development Team. All rights reserved.
   $Id$

   Desc: Demo of new OOP system
   Lang: english
*/

#include "types.h"
#include "oop.h"

/* Prototypes */
Class *MakeClass(STRPTR classID, STRPTR superID, APTR *mDescr, ULONG instDataSize,
ULONG numeNewMethods);
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
