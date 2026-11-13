/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCAPP_H
#define _QP_CCAPP_H

#include "QP_Intern.h"

#define MUIA_QPart_ccApp_Parent           (MUIA_QPart_ccPartition_ABASE + 0x01)
#define MUIA_QPart_ccApp_Container        (MUIA_QPart_ccPartition_ABASE + 0x02)
#define MUIA_QPart_ccApp_ActivePart       (MUIA_QPart_ccPartition_ABASE + 0x03)

#define MUIM_QPart_ccApp_DeleteActive     (MUIA_QPart_ccPartition_MBASE + 0x01)


struct QPApp_DATA
{
    Object  *qpad_Active;
    ULONG   qpad_TextPen;
};

#if !defined(_QP_CCPARTITION_C)
//Prototype for the dispatcher
extern IPTR QPApp_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCAPP_H */
