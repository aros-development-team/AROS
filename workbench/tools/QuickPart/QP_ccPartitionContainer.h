/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "QP_Intern.h"

struct PartCont_ChildIntern
{
    struct Node                 PCCN_Node;
    Object                      *PCCN_ChildObj;
    ULONG                       PCCN_Type;
    UQUAD                       PCCN_Start;
    UQUAD                       PCCN_End;
    IPTR                        PCCN_Weight;    
    BOOL                        PCCN_Delete;
};

#define MUIA_QPart_ccPartitionContainer_Type            (MUIA_QPart_ccPartitionContainer_ABASE + 0x01)
#define MUIA_QPart_ccPartitionContainer_PartObj         (MUIA_QPart_ccPartitionContainer_ABASE + 0x02)
#define MUIA_QPart_ccPartitionContainer_Start           (MUIA_QPart_ccPartitionContainer_ABASE + 0x03)
#define MUIA_QPart_ccPartitionContainer_End             (MUIA_QPart_ccPartitionContainer_ABASE + 0x04)
#define MUIA_QPart_ccPartitionContainer_Reserved        (MUIA_QPart_ccPartitionContainer_ABASE + 0x05)
#define MUIA_QPart_ccPartitionContainer_MaxParts        (MUIA_QPart_ccPartitionContainer_ABASE + 0x06)

#define MUIM_QPart_ccPartitionContainer_FindParts       (MUIA_QPart_ccPartitionContainer_MBASE + 0x01)
#define MUIM_QPart_ccPartitionContainer_UpdateWeights   (MUIA_QPart_ccPartitionContainer_MBASE + 0x02)
#define MUIM_QPart_ccPartitionContainer_UpdateParts     (MUIA_QPart_ccPartitionContainer_MBASE + 0x03)
#define MUIM_QPart_ccPartitionContainer_AddPart         (MUIA_QPart_ccPartitionContainer_MBASE + 0x04)
#define MUIM_QPart_ccPartitionContainer_AddFree         (MUIA_QPart_ccPartitionContainer_MBASE + 0x05)

struct MUIP_QPart_ccPartitionContainer_AddPart     {STACKED ULONG MethodID; STACKED struct PartitionHandle *PHToAdd; };
struct MUIP_QPart_ccPartitionContainer_AddFree     {STACKED ULONG MethodID; STACKED struct PartCont_ChildIntern *FSNode; };

struct QPPartitionContainer_DATA
{
    struct DriveGeometry        *qpcd_Geom;
    struct DosEnvec             qpcd_Part_DE;

    struct List                 qpcd_Part_Children; /* Private Records of partitions we "house" */
    Object                      *qpcd_Object_Disk;
    Object                      *qpcd_Object_Parent;
    Object                      *qpcd_Object_Contents;
    Object                      *qpcd_Object_Select;
    Object                      *qpcd_Object_Part;
   
    UQUAD                       qpcd_Start;
    UQUAD                       qpcd_End;

    /* Tmp strings & values */
    IPTR                        qpcd_Part_BGColor;
    IPTR                        qpcd_Weight;

    struct PartitionHandle      *qpcd_Part_PHandle;
    const struct PartitionAttribute   *qpcd_Part_TAttrlist; /* supported partition table attributes */
    const struct PartitionAttribute   *qpcd_Part_PAttrlist; /* supported partition attributes */
    ULONG                       qpcd_Part_Reserved;
    ULONG                       qpcd_Part_MaxPartitions;
    ULONG                       qpcd_Part_Type;
};


#define PCCN_TYPE_FREE      (0x01)
#define PCCN_TYPE_PARTITION (0x02)

#if !defined(_QP_CCPARTITIONCONTAINER_C)
//Prototype for the dispatcher
extern IPTR QPPartitionContainer_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif
