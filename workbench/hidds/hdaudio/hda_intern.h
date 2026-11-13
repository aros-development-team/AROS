#ifndef HDA_INTERN_H
#define HDA_INTERN_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: HD Audio controller hidd, private definitions
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/interrupts.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <oop/oop.h>

#include <hidd/hda.h>

/* Controller register set (High Definition Audio specification 3.3) */
#define HD_GCAP 0x0
#define HD_GCAP_ISS_MASK 0x0F00
#define HD_GCAP_OSS_MASK 0xF000
#define HD_VMIN 0x2
#define HD_VMAJ 0x3
#define HD_GCTL 0x8
#define HD_WAKEEN 0xC
#define HD_STATESTS 0xE

#define HD_INTCTL 0x20
#define HD_INTCTL_GIE 0x80000000
#define HD_INTCTL_CIE 0x40000000
#define HD_INTSTS 0x24
#define HD_INTSTS_GIS 0x80000000
#define HD_INTSTS_CIS 0x40000000

#define HD_CORB_LOW 0x40
#define HD_CORB_HIGH 0x44
#define HD_CORBWP 0x48
#define HD_CORBRP 0x4A
#define HD_CORBRPRST 0x8000
#define HD_CORBCTL 0x4C
#define HD_CORBRUN 0x2
#define HD_CORBSIZE 0x4E

#define HD_RIRB_LOW 0x50
#define HD_RIRB_HIGH 0x54
#define HD_RIRBWP 0x58
#define HD_RIRBWPRST 0x8000
#define HD_RINTCNT 0x5A
#define HD_RIRBCTL 0x5C
#define HD_RIRBRUN 0x2
#define HD_RINTCTL 0x1
#define HD_RIRBSTS 0x5D
#define HD_RIRBSIZE 0x5E

#define HD_DPLBASE 0x70
#define HD_DPLBASE_ENABLE 0x1
#define HD_DPUBASE 0x74

#define HD_SD_BASE_OFFSET 0x80
#define HD_SD_DESCRIPTOR_SIZE 0x20

#define HD_SD_OFFSET_CONTROL 0x00
#define HD_SD_CONTROL_IOCE 0x4
#define HD_SD_CONTROL_STREAM_RUN 0x2
#define HD_SD_CONTROL_STREAM_RESET 0x1
#define HD_SD_OFFSET_STATUS  0x03
#define HD_SD_STATUS_MASK 0x1C
#define HD_SD_OFFSET_LINKPOS 0x04
#define HD_SD_OFFSET_CYCLIC_BUFFER_LEN 0x08
#define HD_SD_OFFSET_LAST_VALID_INDEX 0x0C
#define HD_SD_OFFSET_FIFO_SIZE 0x10
#define HD_SD_OFFSET_FORMAT 0x12
#define HD_SD_OFFSET_BDL_ADDR_LOW 0x18
#define HD_SD_OFFSET_BDL_ADDR_HIGH 0x1C

struct HDA_BDLE /* Buffer Descriptor List entry (3.6.2) */
{
    ULONG lower_address;
    ULONG upper_address;
    ULONG length;
    ULONG reserved_ioc; /* bit 0 is Interrupt on Completion */
};

struct HDAData;

struct HDAStream
{
    struct HDAData *hs_Ctrl;
    ULONG  hs_SDOffset;
    UBYTE  hs_Index;
    UBYTE  hs_Tag;
    UBYTE  hs_Direction;
    BOOL   hs_InUse;
    BOOL   hs_Running;
    struct Interrupt *hs_ClientInt;
    struct HDA_BDLE *hs_BDL;
    APTR   hs_BDLUnaligned;
    ULONG  hs_BufferCount;
    ULONG  hs_BufferSize;
    APTR   hs_Buffers[HDA_MAXSTREAMBUFS];
    APTR   hs_BuffersUnaligned[HDA_MAXSTREAMBUFS];
};

struct HDAData
{
    APTR   hc_DeviceData;

    BOOL   hc_HWInit;

    volatile UBYTE *hc_MMIO;

    struct Interrupt hc_Interrupt;
    BOOL   hc_InterruptAdded;
    struct Interrupt hc_ResetHandler;
    BOOL   hc_ResetHandlerAdded;

    UWORD  hc_CodecMask;

    struct SignalSemaphore hc_VerbLock;
    ULONG *hc_CORB;
    APTR   hc_CORBUnaligned;
    ULONG  hc_CORBEntries;
    ULONG *hc_RIRB;
    APTR   hc_RIRBUnaligned;
    ULONG  hc_RIRBEntries;
    ULONG  hc_RIRBReadPos;
    volatile LONG hc_RIRBIrq;

    UBYTE  hc_NumInStreams;
    UBYTE  hc_NumOutStreams;
    UBYTE  hc_NumStreams;
    struct HDAStream *hc_Streams;
};

struct hda_staticdata
{
    OOP_Class *hdaClass;

    struct Library *oopBase;
    struct Library *utilityBase;

    OOP_AttrBase hdaAttrBase;
};

struct HDABase
{
    struct Library lib;
    struct hda_staticdata hsd;
};

#define UtilityBase (hsd->utilityBase)

VOID hda_usleep(ULONG usec);

#endif /* HDA_INTERN_H */
