/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCOPGRP_H
#define _QP_CCOPGRP_H

#include "QP_Intern.h"

struct QPOpGrp_DATA
{
    Object *qpogd_Spacer;
    ULONG   qpogd_OpCnt;
};

#if !defined(_QP_CCOPGRP_C)
//Prototype for the dispatcher
extern IPTR QPOpGrp_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCOPGRP_H */
