/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCOPERATION_H
#define _QP_CCOPERATION_H

#include "QP_Intern.h"

struct QPOp_DATA
{
    Object  *qpod_TmpObj;
    Object  *qpod_LabelObj;
    Object  *qpod_TargetObj;
    IPTR    qpod_Operation;
};

#define MUIA_QPart_ccOperation_Type           (MUIA_QPart_ccOperation_ABASE + 0x01)
#define MUIA_QPart_ccOperation_Target         (MUIA_QPart_ccOperation_ABASE + 0x02)

#define MUIV_QPart_ccOperation_Delete         (0x1)
#define MUIV_QPart_ccOperation_Create         (0x2)
#define MUIV_QPart_ccOperation_Alter          (0x3)

#if !defined(_QP_CCOPERATION_C)
//Prototype for the dispatcher
extern IPTR QPOp_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCOPERATION_H */

