/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef OPERATIONCLASS_H
#define OPERATIONCLASS_H

#define OPM_Execute TAG_USER+5001

struct OperationClassData
{
    ULONG dummy;
};

struct opExecute
{
    ULONG MethodID;
    Object *target;
    ULONG operationCode;
};

#endif
