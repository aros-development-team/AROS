/*
    Copyright © 2003-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include	<libraries/mui.h>
#include   <libraries/partition.h>

#define LNT_Root                        (0)
#define LNT_Parent                      (1)
#define LNT_Device                      (2)
#define LNT_Harddisk                    (3)
#define LNT_Partition                   (4)

#define LNF_Listable                    (1<<0)
#define LNF_Unused1                     (1<<1)
#define LNF_ToSave                      (1<<2) /* Changed : must be saved */
#define LNF_Invalid                     (1<<3)
#define LNF_IntermedChange              (1<<4)

/**/

#define DEF_HDISK_IMAGE                 "SYS:Prefs/Presets/Icons/Gorilla/Small/Images/Harddisk"
#define DEF_LOGO_IMAGE                  "PROGDIR:gfx/quickpart.logo"

#define DEF_DRIVEPANE_BACK              MUII_SHINE

#define DEF_DRIVE_SELECTED              MUII_WindowBack
#define DEF_DRIVE_EMPTY                 MUII_SHINEBACK

#define MUIA_QPart_Custom_BASE                  (MUIB_AROS + 0x00ff0000)   

#define MUIA_QPart_Custom_ABASE                 (MUIA_QPart_Custom_BASE)
#define MUIA_QPart_Custom_MBASE                 (MUIA_QPart_Custom_BASE + 0x1000)

#define MUIA_QPart_ccApp_ABASE                  (MUIA_QPart_Custom_BASE + 0x100)
#define MUIA_QPart_ccApp_MBASE                  (MUIA_QPart_Custom_BASE + 0x1100)

#define MUIA_QPart_ccTxt_ABASE                  (MUIA_QPart_Custom_BASE + 0x200)
#define MUIA_QPart_ccTxt_MBASE                  (MUIA_QPart_Custom_BASE + 0x1200)

#define MUIA_QPart_ccDisk_ABASE                 (MUIA_QPart_Custom_BASE + 0x300)
#define MUIA_QPart_ccDisk_MBASE                 (MUIA_QPart_Custom_BASE + 0x1300)

#define MUIA_QPart_ccPartitionContainer_ABASE   (MUIA_QPart_Custom_BASE + 0x400)
#define MUIA_QPart_ccPartitionContainer_MBASE   (MUIA_QPart_Custom_BASE + 0x1400)

#define MUIA_QPart_ccPartition_ABASE            (MUIA_QPart_Custom_BASE + 0x500)
#define MUIA_QPart_ccPartition_MBASE            (MUIA_QPart_Custom_BASE + 0x1500)

#define MUIA_QPart_ccOperation_ABASE            (MUIA_QPart_Custom_BASE + 0x600)

extern APTR VolumeManagerResourceBase;

extern struct	MUI_CustomClass		*mcc_qpdisk;
extern struct	MUI_CustomClass		*mcc_qppartition;
extern struct	MUI_CustomClass		*mcc_qppartitioncontainer;

extern void GetPartitionDrawAttribs(struct PartitionType *PartType, IPTR *BGPtr, char **LabelPtr);
extern void GetPartitionContainerDrawAttribs(IPTR *ContainerType, IPTR *BGPtr, char **LabelPtr);