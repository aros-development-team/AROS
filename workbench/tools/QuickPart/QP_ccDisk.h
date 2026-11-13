/*
    Copyright © 2012-2025, The AROS Development Team. All rights reserved.
    $Id$
*/

#include	<libraries/mui.h>

#define __BSD_VISIBLE
#include	<sys/types.h>
#undef __BSD_VISIBLE

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
typedef unsigned int u_int;

#include   <devices/scsidisk.h>
#include   <devices/scsicmds.h>
#include   <devices/atascsi.h>

#include	"QP_Intern.h"

/* QuickPart DISK Custom Class */

#define	MUIA_QPart_Disk_Handler		    (MUIA_QPart_ccDisk_ABASE + 0x01) 
#define	MUIA_QPart_Disk_Unit  		    (MUIA_QPart_ccDisk_ABASE + 0x02)
#define	MUIA_QPart_Disk_Object 		    (MUIA_QPart_ccDisk_ABASE + 0x03)

#define	MUIA_QPart_Disk_Geometry        (MUIA_QPart_ccDisk_ABASE + 0x06)
#define	MUIA_QPart_Disk_Name     	    (MUIA_QPart_ccDisk_ABASE + 0x07)
#define	MUIA_QPart_Disk_NameVerbose     (MUIA_QPart_ccDisk_ABASE + 0x08)
#define	MUIA_QPart_Disk_Capacity        (MUIA_QPart_ccDisk_ABASE + 0x09) /* drive size in bytes */

#define	MUIA_QPart_Disk_Spacing		    (MUIA_QPart_ccDisk_ABASE + 0x10)
#define	MUIA_QPart_Disk_Scale  		    (MUIA_QPart_ccDisk_ABASE + 0x11) /* % of width drive should occupy */

/* Allways the LAST attribute ! */
#define	MUIA_QPart_Disk_Changed         (MUIA_QPart_ccDisk_ABASE + 0xff)

#define	MUIM_QPart_Disk_ScanForParts    (MUIA_QPart_ccDisk_MBASE + 0x01) /* Scan the disk this object represents and update its display   */
#define	MUIM_QPart_Disk_SaveDisk        (MUIA_QPart_ccDisk_MBASE + 0x02) /* !! Warning - causes the current data to be written to disk !! */

struct QPDisk_DATA
{
    struct DriveGeometry qpd_Geom;

    /* Object Pointers Used to represent our "Disk" */
    Object            *qpd_Object_HardDiskArea;
    Object            *qpd_Object_txt_DriveName;
    Object            *qpd_Object_txt_DriveCapacity;
    Object            *qpd_Object_txt_DriveUnit;
    Object            *qpd_Object_VOID_spacer;
    /*** The Drive Image (rectangle) **/
    Object            *qpd_Object_HardDisk_LayoutGrp;
    Object            *qpd_Object_HardDisk_Rectangle;
    Object            *qpd_Object_HardDisk_Contents;
    Object            *qpd_Object_VOID_weight;

    /* strings & values */
    IPTR              qpd_Disk_BGColor;
    char              *qpd_Disk_Handler;
    LONG              qpd_Disk_Unit;

    char              *qpd_Disk_str_DriveName;
    char              *qpd_Disk_str_DriveNameVerbose;
    char              *qpd_Disk_str_DriveUnit; // Concatonated string of "qpd_Disk_Handler Unit qpd_Disk_Unit"
    UQUAD           qpd_Disk_val_DriveCapacity;
    char              *qpd_Disk_str_DriveCapacity;

    struct   Hook     *qpd_Disk_hook_LocaliseNumber;
    int                qpd_Disk_hook_LocaliseNumber_ConvCnt;
    /* Disk Access */
    struct IOStdReq   *qpd_Disk_IOReq;
    struct MsgPort               *qpd_Disk_MP;

    struct PartitionHandle       *qpd_Disk_PH;
};

#if !defined(_QP_CCDISK_C)
//Prototype for the dispatcher
extern IPTR QPDisk_Dispatcher(Class *CLASS, Object *self, Msg message);
#endif
