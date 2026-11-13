/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCFREE_H
#define _QP_CCFREE_H

#include "QP_Intern.h"

struct QPFree_DATA
{
    ULONG   qptd_TextPen;
};

#if !defined(_QP_CCFREE_C)
//Prototype for the dispatcher
extern IPTR QPFree_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCFREE_H */
