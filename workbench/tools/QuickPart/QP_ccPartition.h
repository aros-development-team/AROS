/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/
#ifndef _QP_CCPARTITION_H
#define _QP_CCPARTITION_H

#include "QP_Intern.h"

#define MUIA_QPart_ccPartition_Parent       (MUIA_QPart_ccPartition_ABASE + 0x01)
#define MUIA_QPart_ccPartition_Container    (MUIA_QPart_ccPartition_ABASE + 0x02)
#define MUIA_QPart_ccPartition_PartObj      (MUIA_QPart_ccPartition_ABASE + 0x03)

#define	MUIA_QPart_ccPartition_Handle	    (MUIA_QPart_ccPartition_ABASE + 0x05)
#define	MUIA_QPart_ccPartition_Geometry	    (MUIA_QPart_ccPartition_ABASE + 0x06)

#define	MUIA_QPart_ccPartition_Type   	    (MUIA_QPart_ccPartition_ABASE + 0x07)
#define	MUIA_QPart_ccPartition_TypeStr	    (MUIA_QPart_ccPartition_ABASE + 0x08)
#define	MUIA_QPart_ccPartition_SizeStr	    (MUIA_QPart_ccPartition_ABASE + 0x09)
#define	MUIA_QPart_ccPartition_DOSDevStr    (MUIA_QPart_ccPartition_ABASE + 0x0a)

#define	MUIA_QPart_ccPartition_Start 	    (MUIA_QPart_ccPartition_ABASE + 0x10)
#define	MUIA_QPart_ccPartition_End    	    (MUIA_QPart_ccPartition_ABASE + 0x11)
#define	MUIA_QPart_ccPartition_Position	    (MUIA_QPart_ccPartition_ABASE + 0x12)

struct QPPartition_DATA
{
    struct DriveGeometry        *qppd_Geom;
    struct DosEnvec             qppd_Part_DE;

    /* Object Pointers Used to represent our "Partition" */
    Object                      *qpdd_Object_Disk;
    Object                      *qppd_Object_Parent; /* The PartitionContainer Object we are part of .. */
    Object                      *qppd_Object_Part;
    Object                      *qppd_Object_Contents;

    char                        *qppd_Str_Label;
    char                        *qppd_Str_Type;
    char                        *qppd_Str_Size;
    char                        *qppd_Str_DOSDev;
    
    /* strings & values */
    IPTR                        qppd_Part_BGColor;
    IPTR                        qppd_Weight;

    struct PartitionHandle      *qppd_Part_PH;
    struct PartitionType        qppd_Part_Type;
    struct PartitionTable       *qppd_Part_Table;
    ULONG                       qppd_Part_Flags;
    ULONG                       qppd_Part_Pos;
    
    struct MUI_EventHandlerNode qppd_PatEH;
    UWORD                       qppd_Pattern;
};

#if !defined(_QP_CCPARTITION_C)
//Prototype for the dispatcher
extern IPTR QPPartition_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif

#endif /* _QP_CCPARTITION_H */
