/*
   Copyright © 1995-2002, The AROS Development Team. All rights reserved.
   $Id$ 
 */

#ifndef OPERATIONCLASS_H
#    define OPERATIONCLASS_H

#    define OPM_Execute TAG_USER+5001

struct OperationClassData
{
    ULONG           dummy;
};

struct opExecute
{
    STACKED ULONG           MethodID;
    STACKED Object         *target;
    STACKED ULONG           operationCode;
};

#endif
