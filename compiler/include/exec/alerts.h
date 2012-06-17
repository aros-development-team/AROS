#ifndef EXEC_ALERTS_H
#define EXEC_ALERTS_H

/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: alert numbers
    Lang: english
*/

/* General Types */
#define AT_Recovery 0x00000000 /* Alert that returns */
#define AT_DeadEnd  0x80000000 /* Alert that crashes */

/* General errors */
#define AG_NoMemory   0x00010000
#define AG_MakeLib    0x00020000
#define AG_OpenLib    0x00030000
#define AG_OpenDev    0x00040000
#define AG_OpenRes    0x00050000
#define AG_IOError    0x00060000
#define AG_NoSignal   0x00070000
#define AG_BadParm    0x00080000
#define AG_CloseLib   0x00090000
#define AG_CloseDev   0x000A0000
#define AG_ProcCreate 0x000B0000

/* 680x0 */
#define ACPU_AbsExecBase 0x80000001     /* AbsExecBase changed */
#define ACPU_BusErr     0x80000002
#define ACPU_AddressErr 0x80000003
#define ACPU_InstErr    0x80000004
#define ACPU_DivZero    0x80000005
#define ACPU_CHK        0x80000006
#define ACPU_TRAPV      0x80000007
#define ACPU_PrivErr    0x80000008
#define ACPU_Trace      0x80000009
#define ACPU_LineA      0x8000000A
#define ACPU_LineF      0x8000000B
#define ACPU_Format     0x8000000E
#define ACPU_Spurious   0x80000018
#define ACPU_AutoVec1   0x80000019
#define ACPU_AutoVec2   0x8000001A
#define ACPU_AutoVec3   0x8000001B
#define ACPU_AutoVec4   0x8000001C
#define ACPU_AutoVec5   0x8000001D
#define ACPU_AutoVec6   0x8000001E
#define ACPU_AutoVec7   0x8000001F

/* Libraries */
#define AO_ExecLib      0x00008001
#define AO_GraphicsLib  0x00008002
#define AO_LayersLib    0x00008003
#define AO_Intuition    0x00008004
#define AO_MathLib      0x00008005
#define AO_DOSLib       0x00008007
#define AO_RAMLib       0x00008008
#define AO_IconLib      0x00008009
#define AO_ExpansionLib 0x0000800A
#define AO_DiskfontLib  0x0000800B
#define AO_UtilityLib   0x0000800C
#define AO_KeyMapLib    0x0000800D
/* Devices */
#define AO_AudioDev     0x00008010
#define AO_ConsoleDev   0x00008011
#define AO_GamePortDev  0x00008012
#define AO_KeyboardDev  0x00008013
#define AO_TrackDiskDev 0x00008014
#define AO_TimerDev     0x00008015
/* Resources */
#define AO_CIARsrc      0x00008020
#define AO_DiskRsrc     0x00008021
#define AO_MiscRsrc     0x00008022
/* Miscellaneous */
#define AO_BootStrap    0x00008030
#define AO_Workbench    0x00008031
#define AO_DiskCopy     0x00008032
#define AO_GadTools     0x00008033
#define AO_Unknown      0x00008035

/* AROS Additions, start at 0x40 */
#define AO_ArosLib	0x00008040
#define AO_OOPLib	0x00008041
#define AO_HiddLib	0x00008042
#define AO_PartitionLib	0x00008043

/* exec.library */
#define AN_ExecLib      0x01000000
#define AN_ExcptVect    0x01000001
#define AN_BaseChkSum   0x01000002
#define AN_LibChkSum    0x01000003
#define AN_MemCorrupt   0x81000005
#define AN_IntrMem      0x81000006
#define AN_InitAPtr     0x01000007
#define AN_SemCorrupt   0x01000008
#define AN_FreeTwice    0x01000009
#define AN_BogusExcpt   0x8100000A
#define AN_IOUsedTwice  0x0100000B
#define AN_MemoryInsane 0x0100000C
#define AN_IOAfterClose 0x0100000D
#define AN_StackProbe   0x0100000E /* stack has wrong size */
#define AN_BadFreeAddr  0x0100000F
#define AN_BadSemaphore 0x01000010

/* dos.library */
#define AN_DOSLib       0x07000000
#define AN_StartMem     0x07010001
#define AN_EndTask      0x07000002
#define AN_QPktFail     0x07000003
#define AN_AsyncPkt     0x07000004
#define AN_FreeVec      0x07000005
#define AN_DiskBlkSeq   0x07000006
#define AN_BitMap       0x07000007
#define AN_KeyFree      0x07000008
#define AN_BadChkSum    0x07000009
#define AN_DiskError    0x0700000A
#define AN_KeyRange     0x0700000B
#define AN_BadOverlay   0x0700000C
#define AN_BadInitFunc  0x0700000D
#define AN_FileReclosed 0x0700000E

/* graphics.library */
#define AN_GraphicsLib  0x02000000
#define AN_GfxNoMem     0x82010000
#define AN_GfxNoMemMspc 0x82010001
#define AN_LongFrame    0x82010006
#define AN_ShortFrame   0x82010007
#define AN_TextTmpRas   0x82010009
#define AN_BltBitMap    0x8201000A
#define AN_RegionMemory 0x8201000B
#define AN_MakeVPort    0x82010030
#define AN_GfxNewError  0x0200000C
#define AN_GfxFreeError 0x0200000D
#define AN_GfxNoLCM     0x82011234
#define AN_ObsoleteFont 0x02000401

/* intuition.library */
#define AN_Intuition    0x04000000
#define AN_GadgetType   0x84000001
#define AN_BadGadget    0x04000001
#define AN_CreatePort   0x84010002
#define AN_ItemAlloc    0x04010003
#define AN_SubAlloc     0x04010004
#define AN_PlaneAlloc   0x84010005
#define AN_ItemBoxTop   0x84000006
#define AN_OpenScreen   0x84010007
#define AN_OpenScrnRast 0x84010008
#define AN_SysScrnType  0x84000009
#define AN_AddSWGadget  0x8401000A
#define AN_OpenWindow   0x8401000B
#define AN_BadState     0x8400000C
#define AN_BadMessage   0x8400000D
#define AN_WeirdEcho    0x8400000E
#define AN_NoConsole    0x8400000F
#define AN_NoISem       0x04000010
#define AN_ISemOrder    0x04000011

/* utility.library */
#define AN_UtilityLib 0x34000000

/* layers.library */
#define AN_LayersLib   0x03000000
#define AN_LayersNoMem 0x83010000

/* math.library */
#define AN_MathLib 0x05000000

/* ramlib.library */
#define AN_RAMLib     0x08000000
#define AN_BadSegList 0x08000001

/* expansion.library */
#define AN_ExpansionLib     0x0A000000
#define AN_BadExpansionFree 0x0A000001

/* diskfont.library */
#define AN_DiskfontLib 0x0B000000

/* icon.library */
#define AN_IconLib 0x09000000

/* gadtools.library */
#define AN_GadTools 0x33000000

/* audio.device */
#define AN_AudioDev 0x10000000

/* console.device */
#define AN_ConsoleDev 0x11000000
#define AN_NoWindow   0x11000001

/* gameport.device */
#define AN_GamePortDev 0x12000000

/* keyboard.device */
#define AN_KeyboardDev 0x13000000

/* trackdisk.device */
#define AN_TrackDiskDev 0x14000000
#define AN_TDCalibSeek  0x14000001
#define AN_TDDelay      0x14000002

/* timer.device */
#define AN_TimerDev    0x15000000
#define AN_TMBadReq    0x15000001
#define AN_TMBadSupply 0x15000002

/* cia.resource */
#define AN_CIARsrc 0x20000000

/* disk.resource */
#define AN_DiskRsrc   0x21000000
#define AN_DRHasDisk  0x21000001
#define AN_DRIntNoAct 0x21000002

/* misc.resource */
#define AN_MiscRsrc 0x22000000

/* bootstrap */
#define AN_BootStrap 0x30000000
#define AN_BootError 0x30000001

/* Workbench */
#define AN_Workbench          0x31000000
#define AN_NoFonts            0xB1000001
#define AN_WBBadStartupMsg1   0x31000001
#define AN_WBBadStartupMsg2   0x31000002
#define AN_WBBadIOMsg         0x31000003
#define AN_WBReLayoutToolMenu 0xB1010009

/* DiskCopy */
#define AN_DiskCopy 0x32000000

#define AN_Unknown 0x35000000

/* AROS Additions */
#define AN_Aros		    0x40000000
#define AN_OOP		    0x41000000

/* Hidd Subsystem */
#define AN_Hidd		    0x42000000
#define AN_HiddNoRoot	    0xC2000001	/* Could not create root device */

#endif /* EXEC_ALERTS_H */
