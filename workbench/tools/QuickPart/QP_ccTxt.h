/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCTXT_H
#define _QP_CCTXT_H

#include "QP_Intern.h"

struct QPTxt_DATA
{
    ULONG   qptd_TextPen;
};

#if !defined(_QP_CCTXT_C)
//Prototype for the dispatcher
extern IPTR QPTxt_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCTXT_H */
