/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef LIBRARIES_PCCARD_H
#define LIBRARIES_PCCARD_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/* General */
/* ======= */

#define PCCARDNAME "pccard.library"

#define PCCARD_MAXTUPLESIZE 0xff


/* Tuple types */
/* =========== */

#define PCCARD_TPL_NULL           0x00
#define PCCARD_TPL_DEVICE         0x01
#define PCCARD_TPL_LONGLINKCB     0x02
#define PCCARD_TPL_CONFIGCB       0x04
#define PCCARD_TPL_CFTABLEENTRYCB 0x05
#define PCCARD_TPL_LONGLINKMFC    0x06
#define PCCARD_TPL_BAR            0x07
#define PCCARD_TPL_CHECKSUM       0x10
#define PCCARD_TPL_LONGLINKA      0x11
#define PCCARD_TPL_LONGLINKC      0x12
#define PCCARD_TPL_LINKTARGET     0x13
#define PCCARD_TPL_NOLINK         0x14
#define PCCARD_TPL_VERS1          0x15
#define PCCARD_TPL_ALTSTR         0x16
#define PCCARD_TPL_DEVICEA        0x17
#define PCCARD_TPL_JEDECC         0x18
#define PCCARD_TPL_JEDECA         0x19
#define PCCARD_TPL_CONFIG         0x1a
#define PCCARD_TPL_CFTABLEENTRY   0x1b
#define PCCARD_TPL_DEVICEOC       0x1c
#define PCCARD_TPL_DEVICEOA       0x1d
#define PCCARD_TPL_DEVICEGEO      0x1e
#define PCCARD_TPL_DEVICEGEOA     0x1f
#define PCCARD_TPL_MANFID         0x20
#define PCCARD_TPL_FUNCID         0x21
#define PCCARD_TPL_FUNCE          0x22
#define PCCARD_TPL_SWIL           0x23
#define PCCARD_TPL_END            0xff
#define PCCARD_TPL_VERS2          0x40
#define PCCARD_TPL_FORMAT         0x41
#define PCCARD_TPL_GEOMETRY       0x42
#define PCCARD_TPL_BYTEORDER      0x43
#define PCCARD_TPL_DATE           0x44
#define PCCARD_TPL_BATTERY        0x45
#define PCCARD_TPL_ORG            0x46


/* Tags for GetTupleTags() */
/* ======================= */

#define PCCARD_RegisterBase       (TAG_USER+0)
#define PCCARD_ModeCount          (TAG_USER+1)
#define PCCARD_ModeNo             (TAG_USER+2)
#define PCCARD_Flags              (TAG_USER+3)
#define PCCARD_IOFlags            (TAG_USER+4)
#define PCCARD_IOLineCount        (TAG_USER+5)
#define PCCARD_IOWinCount         (TAG_USER+6)
#define PCCARD_IOWinBases         (TAG_USER+7)    /* Array of ULONGs */
#define PCCARD_IOWinLengths       (TAG_USER+8)    /* Array of ULONGs */
#define PCCARD_VCCPowerTags       (TAG_USER+9)
#define PCCARD_VPP1PowerTags      (TAG_USER+10)
#define PCCARD_VPP2PowerTags      (TAG_USER+11)
#define PCCARD_NominalVoltage     (TAG_USER+12)
#define PCCARD_MinVoltage         (TAG_USER+13)
#define PCCARD_MaxVoltage         (TAG_USER+14)
#define PCCARD_StaticCurrent      (TAG_USER+15)
#define PCCARD_AverageCurrent     (TAG_USER+16)
#define PCCARD_PeakCurrent        (TAG_USER+17)
#define PCCARD_DownCurrent        (TAG_USER+18)
#define PCCARD_Type               (TAG_USER+19)
#define PCCARD_WaitTimingTags     (TAG_USER+20)
#define PCCARD_ReadyTimingTags    (TAG_USER+21)
#define PCCARD_ReservedTimingTags (TAG_USER+22)
#define PCCARD_Value              (TAG_USER+23)
#define PCCARD_Scale              (TAG_USER+24)
#define PCCARD_IRQFlags           (TAG_USER+25)
#define PCCARD_IRQMask            (TAG_USER+26)
#define PCCARD_MemWinCount        (TAG_USER+27)
#define PCCARD_MemWinBases        (TAG_USER+28)   /* Array of ULONGs */
#define PCCARD_MemWinHostBases    (TAG_USER+29)   /* Array of ULONGs */
#define PCCARD_MemWinLengths      (TAG_USER+30)   /* Array of ULONGs */
#define PCCARD_MajorVersion       (TAG_USER+31)
#define PCCARD_MinorVersion       (TAG_USER+32)
#define PCCARD_InfoStringCount    (TAG_USER+33)
#define PCCARD_InfoStrings        (TAG_USER+34)   /* Array of "TEXT *"s */
#define PCCARD_RegionCount        (TAG_USER+35)
#define PCCARD_RegionLists        (TAG_USER+36)   /* Array of TagLists */
#define PCCARD_Speed              (TAG_USER+37)
#define PCCARD_Base               (TAG_USER+38)
#define PCCARD_Length             (TAG_USER+39)
#define PCCARD_Maker              (TAG_USER+40)
#define PCCARD_Product            (TAG_USER+41)


/* Values used in tags */
/* =================== */

/* Types of memory region */

#define PCCARD_DTYPE_NULL     0x00
#define PCCARD_DTYPE_ROM      0x01
#define PCCARD_DTYPE_OTPROM   0x02
#define PCCARD_DTYPE_EPROM    0x03
#define PCCARD_DTYPE_EEPROM   0x04
#define PCCARD_DTYPE_FLASH    0x05
#define PCCARD_DTYPE_SRAM     0x06
#define PCCARD_DTYPE_DRAM     0x07
#define PCCARD_DTYPE_FUNCSPEC 0x0d
#define PCCARD_DTYPE_EXTEND   0x0e

/* Card Functions */

#define PCCARD_FUNC_MULTI    0x00
#define PCCARD_FUNC_MEMORY   0x01
#define PCCARD_FUNC_SERIAL   0x02
#define PCCARD_FUNC_PARALLEL 0x03
#define PCCARD_FUNC_FIXED    0x04
#define PCCARD_FUNC_VIDEO    0x05
#define PCCARD_FUNC_NETWORK  0x06
#define PCCARD_FUNC_AIMS     0x07
#define PCCARD_FUNC_SCSI     0x08

/* Flags in IRQFlags tag */

#define PCCARD_IRQB_NMI   0
#define PCCARD_IRQB_IOCK  1
#define PCCARD_IRQB_BERR  2
#define PCCARD_IRQB_VEND  3
#define PCCARD_IRQB_LEVEL 5
#define PCCARD_IRQB_PULSE 6
#define PCCARD_IRQB_SHARE 7

#define PCCARD_IRQF_NMI   (1<<PCCARD_IRQB_NMI)
#define PCCARD_IRQF_IOCK  (1<<PCCARD_IRQB_IOCK)
#define PCCARD_IRQF_BERR  (1<<PCCARD_IRQB_BERR)
#define PCCARD_IRQF_VEND  (1<<PCCARD_IRQB_VEND)
#define PCCARD_IRQF_LEVEL (1<<PCCARD_IRQB_LEVEL)
#define PCCARD_IRQF_PULSE (1<<PCCARD_IRQB_PULSE)
#define PCCARD_IRQF_SHARE (1<<PCCARD_IRQB_SHARE)

/* Flags for Device Regions */

#define PCCARD_REGIONB_WP 0

#define PCCARD_REGIONF_WP (1<<PCCARD_REGIONB_WP)

/* Flags in PCCARD_Flags tag for PCCARD_TPL_FUNCID */

#define PCCARD_SYSINITB_POST 0
#define PCCARD_SYSINITB_ROM  1

#define PCCARD_SYSINITF_POST (1<<PCCARD_SYSINITB_POST)
#define PCCARD_SYSINITF_ROM  (1<<PCCARD_SYSINITB_ROM)

/* Flags in PCCARD_Flags tag for power tag lists */

#define PCCARD_POWERB_HIGHZOK  0
#define PCCARD_POWERB_HIGHZREQ 1

#define PCCARD_POWERF_HIGHZOK  (1<<PCCARD_POWERB_HIGHZOK)
#define PCCARD_POWERF_HIGHZREQ (1<<PCCARD_POWERB_HIGHZREQ)

/* Flags in PCCARD_IOFlags tag */

#define PCCARD_IOB_8BIT  5
#define PCCARD_IOB_16BIT 6

#define PCCARD_IOF_8BIT  (1<<PCCARD_IOB_8BIT)
#define PCCARD_IOF_16BIT (1<<PCCARD_IOB_16BIT)

/* Flags in PCCARD_Flags tag for PCCARD_TPL_CFTABLEENTRY */

#define PCCARD_CFTABLEB_DEFAULT  0
#define PCCARD_CFTABLEB_BVDS     1
#define PCCARD_CFTABLEB_WP       2
#define PCCARD_CFTABLEB_RDYBSY   3
#define PCCARD_CFTABLEB_MWAIT    4
#define PCCARD_CFTABLEB_AUDIO    11
#define PCCARD_CFTABLEB_READONLY 12
#define PCCARD_CFTABLEB_PWRDOWN  13

#define PCCARD_CFTABLEF_DEFAULT  (1<<PCCARD_CFTABLEB_DEFAULT)
#define PCCARD_CFTABLEF_BVDS     (1<<PCCARD_CFTABLEB_BVDS)
#define PCCARD_CFTABLEF_WP       (1<<PCCARD_CFTABLEB_WP)
#define PCCARD_CFTABLEF_RDYBSY   (1<<PCCARD_CFTABLEB_RDYBSY)
#define PCCARD_CFTABLEF_MWAIT    (1<<PCCARD_CFTABLEB_MWAIT)
#define PCCARD_CFTABLEF_AUDIO    (1<<PCCARD_CFTABLEB_AUDIO)
#define PCCARD_CFTABLEF_READONLY (1<<PCCARD_CFTABLEB_READONLY)
#define PCCARD_CFTABLEF_PWRDOWN  (1<<PCCARD_CFTABLEB_PWRDOWN)


/* Registers */
/* ========= */

/* Register Offsets */

#define PCCARD_REG_COR      0x00
#define PCCARD_REG_CCSR     0x02
#define PCCARD_REG_PRR      0x04
#define PCCARD_REG_SCR      0x06
#define PCCARD_REG_ESR      0x08
#define PCCARD_REG_IOBASE0  0x0a
#define PCCARD_REG_IOBASE1  0x0c
#define PCCARD_REG_IOBASE2  0x0e
#define PCCARD_REG_IOBASE3  0x10
#define PCCARD_REG_IOSIZE   0x12

/* Configuration Option Register */

#define PCCARD_REG_CORB_FUNCENABLE 0
#define PCCARD_REG_CORB_ADDRDECODE 1
#define PCCARD_REG_CORB_IREQENABLE 2
#define PCCARD_REG_CORB_LEVELREQ   6
#define PCCARD_REG_CORB_SOFTRESET  7

#define PCCARD_REG_CORF_FUNCENABLE (1<<PCCARD_REG_CORB_FUNCENABLE)
#define PCCARD_REG_CORF_ADDRDECODE (1<<PCCARD_REG_CORB_ADDRDECODE)
#define PCCARD_REG_CORF_IREQENABLE (1<<PCCARD_REG_CORB_IREQENABLE)
#define PCCARD_REG_CORF_LEVELREQ   (1<<PCCARD_REG_CORB_LEVELREQ)
#define PCCARD_REG_CORF_SOFTRESET  (1<<PCCARD_REG_CORB_SOFTRESET)

#define PCCARD_REG_COR_CONFIGMASK    0x3f
#define PCCARD_REG_COR_MFCCONFIGMASK 0x38

/* Card Configuration and Status Register */

#define PCCARD_REG_CCSRB_INTACK       0
#define PCCARD_REG_CCSRB_INTPENDING   1
#define PCCARD_REG_CCSRB_POWERDOWN    2
#define PCCARD_REG_CCSRB_AUDIOENABLE  3
#define PCCARD_REG_CCSRB_IOIS8        5
#define PCCARD_REG_CCSRB_SIGCHGENABLE 6
#define PCCARD_REG_CCSRB_CHANGED      7

#define PCCARD_REG_CCSRF_INTACK       (1<<PCCARD_REG_CCSRB_INTACK)
#define PCCARD_REG_CCSRF_INTPENDING   (1<<PCCARD_REG_CCSRB_INTPENDING)
#define PCCARD_REG_CCSRF_POWERDOWN    (1<<PCCARD_REG_CCSRB_POWERDOWN)
#define PCCARD_REG_CCSRF_AUDIOENABLE  (1<<PCCARD_REG_CCSRB_AUDIOENABLE)
#define PCCARD_REG_CCSRF_IOIS8        (1<<PCCARD_REG_CCSRB_IOIS8)
#define PCCARD_REG_CCSRF_SIGCHGENABLE (1<<PCCARD_REG_CCSRB_SIGCHGENABLE)
#define PCCARD_REG_CCSRF_CHANGED      (1<<PCCARD_REG_CCSRB_CHANGED)

/* Pin Replacement Register */

#define PCCARD_REG_PRRB_WPSTATUS    0
#define PCCARD_REG_PRRB_READYSTATUS 1
#define PCCARD_REG_PRRB_BVD2STATUS  2
#define PCCARD_REG_PRRB_BVD1STATUS  3
#define PCCARD_REG_PRRB_WPEVENT     4
#define PCCARD_REG_PRRB_READYEVENT  5
#define PCCARD_REG_PRRB_BVD2EVENT   6
#define PCCARD_REG_PRRB_BVD1EVENT   7

#define PCCARD_REG_PRRF_WPSTATUS    (1<<PCCARD_REG_PRRB_WPSTATUS)
#define PCCARD_REG_PRRF_READYSTATUS (1<<PCCARD_REG_PRRB_READYSTATUS)
#define PCCARD_REG_PRRF_BVD2STATUS  (1<<PCCARD_REG_PRRB_BVD2STATUS)
#define PCCARD_REG_PRRF_BVD1STATUS  (1<<PCCARD_REG_PRRB_BVD1STATUS)
#define PCCARD_REG_PRRF_WPEVENT     (1<<PCCARD_REG_PRRB_WPEVENT)
#define PCCARD_REG_PRRF_READYEVENT  (1<<PCCARD_REG_PRRB_READYEVENT)
#define PCCARD_REG_PRRF_BVD2EVENT   (1<<PCCARD_REG_PRRB_BVD2EVENT)
#define PCCARD_REG_PRRF_BVD1EVENT   (1<<PCCARD_REG_PRRB_BVD1EVENT)

/* Socket and Copy Register */

#define PCCARD_REG_SCR_SOCKETNUMMASK 0x0f
#define PCCARD_REG_SCR_COPYNUMMASK   0x70

/* Extended Status Register */

#define PCCARD_REG_ESRB_REQATTNENABLE 0
#define PCCARD_REG_ESRB_REQATTN       4

#define PCCARD_REG_ESRF_REQATTNENABLE (1<<PCCARD_REG_ESRB_REQATTNENABLE)
#define PCCARD_REG_ESRF_REQATTN       (1<<PCCARD_REG_ESRB_REQATTN)


#endif
