/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private data and register layout of the DesignWare i2c driver.
*/

#ifndef DWI2C_INTERN_H
#define DWI2C_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>

#include <oop/oop.h>

#include "dwi2c.h"

#include LC_LIBDEFS_FILE

/*
 * Register block, offsets from the controller's base. Only the ones
 * this driver touches are listed; the rest of the block is either
 * interrupt plumbing we have no use for while polling, or slave-mode
 * state we never enter.
 */
#define IC_CON              0x00    /* what mode and speed to run in    */
#define IC_TAR              0x04    /* who the next transaction is with */
#define IC_DATA_CMD         0x10    /* the command/data FIFO, both ways */
#define IC_SS_SCL_HCNT      0x14    /* clock shape, standard mode       */
#define IC_SS_SCL_LCNT      0x18
#define IC_FS_SCL_HCNT      0x1c    /* clock shape, fast mode           */
#define IC_FS_SCL_LCNT      0x20
#define IC_INTR_STAT        0x2c
#define IC_INTR_MASK        0x30
#define IC_RAW_INTR_STAT    0x34    /* the events, masked or not        */
#define IC_RX_TL            0x38
#define IC_TX_TL            0x3c
#define IC_CLR_INTR         0x40    /* read to clear everything         */
#define IC_CLR_TX_ABRT      0x54    /* read to clear an abort           */
#define IC_ENABLE           0x6c
#define IC_STATUS           0x70
#define IC_TXFLR            0x74    /* bytes waiting to go out          */
#define IC_RXFLR            0x78    /* bytes waiting to be read         */
#define IC_SDA_HOLD         0x7c
#define IC_TX_ABRT_SOURCE   0x80    /* why the last transaction died    */
#define IC_ENABLE_STATUS    0x9c
#define IC_COMP_PARAM_1     0xf4    /* how the block was synthesised    */

/* IC_CON */
#define IC_CON_MASTER           0x0001
#define IC_CON_SPEED_STD        0x0002
#define IC_CON_SPEED_FAST       0x0004
#define IC_CON_SPEED_MASK       0x0006
#define IC_CON_10BITADDR_MASTER 0x0010
#define IC_CON_RESTART_EN       0x0020
#define IC_CON_SLAVE_DISABLE    0x0040

/* IC_DATA_CMD - the byte is in the low bits, the rest says what to do */
#define IC_DATA_CMD_DAT         0x00ff
#define IC_DATA_CMD_READ        0x0100
#define IC_DATA_CMD_STOP        0x0200
#define IC_DATA_CMD_RESTART     0x0400

/* IC_STATUS */
#define IC_STATUS_ACTIVITY      0x0001
#define IC_STATUS_TFNF          0x0002  /* tx FIFO not full             */
#define IC_STATUS_TFE           0x0004  /* tx FIFO empty                */
#define IC_STATUS_RFNE          0x0008  /* rx FIFO not empty            */
#define IC_STATUS_RFF           0x0010  /* rx FIFO full                 */
#define IC_STATUS_MST_ACTIVITY  0x0020

/* IC_RAW_INTR_STAT / IC_INTR_STAT / IC_INTR_MASK */
#define IC_INTR_RX_UNDER        0x0001
#define IC_INTR_RX_OVER         0x0002
#define IC_INTR_RX_FULL         0x0004
#define IC_INTR_TX_OVER         0x0008
#define IC_INTR_TX_EMPTY        0x0010
#define IC_INTR_RD_REQ          0x0020
#define IC_INTR_TX_ABRT         0x0040
#define IC_INTR_RX_DONE         0x0080
#define IC_INTR_ACTIVITY        0x0100
#define IC_INTR_STOP_DET        0x0200
#define IC_INTR_START_DET       0x0400

/* IC_TX_ABRT_SOURCE - the reasons worth telling apart */
#define IC_ABRT_7B_ADDR_NOACK   0x0001  /* nobody there                 */
#define IC_ABRT_10ADDR1_NOACK   0x0002
#define IC_ABRT_10ADDR2_NOACK   0x0004
#define IC_ABRT_TXDATA_NOACK    0x0008  /* there, then stopped answering*/
#define IC_ABRT_GCALL_NOACK     0x0010
#define IC_ABRT_ARB_LOST        0x1000

/*
 * An abort with no bit set in IC_TX_ABRT_SOURCE still has to be
 * reported as a failure, so stand in for it with a value no real
 * source register can produce.
 */
#define DWI2C_ABRT_UNKNOWN      0xffffffff

/*
 * How long to spin before declaring the bus wedged. This counts polls
 * of a memory mapped register rather than time, which is all a driver
 * that must work before timer.device exists can measure. A byte at
 * 100kHz takes some 90us, so a limit this size is several thousand
 * times longer than any single step should need - it exists to stop a
 * shorted or unpowered bus hanging the machine, not to time anything.
 */
#define DWI2C_SPIN_LIMIT        1000000UL

/* The enable/disable handshake is documented as settling in a few
   cycles; the databook suggests giving up after a hundred tries */
#define DWI2C_ENABLE_TRIES      100

/* More than this many controllers on one board is not worth planning for */
#define DWI2C_MAX_CTRL          8

/* FIFO depth to assume when IC_COMP_PARAM_1 reads back as nothing */
#define DWI2C_DEFAULT_FIFO      8

/*
 * One controller.
 *
 * The register block is mapped once at discovery and the timing is
 * worked out then, because none of it can change afterwards. What a
 * transfer does is program the controller from these numbers, run the
 * transaction and disable it again - leaving it disabled between
 * transactions is what lets the target address be changed at all.
 */
struct dwi2c_ctrl
{
    struct SignalSemaphore  lock;       /* one transaction at a time    */
    IPTR                    base;       /* mapped register block        */
    IPTR                    size;
    ULONG                   busSpeed;   /* Hz, from "clock-frequency"   */
    ULONG                   inputClock; /* Hz the block itself is fed   */
    ULONG                   sdaHoldNs;  /* 0 when the tree says nothing */
    ULONG                   irq;        /* interrupt source, unused     */
    ULONG                   ssHcnt;     /* 0 when we could not work the */
    ULONG                   ssLcnt;     /*   timing out, in which case  */
    ULONG                   fsHcnt;     /*   whatever the firmware left */
    ULONG                   fsLcnt;     /*   in the registers stands    */
    ULONG                   sdaHold;    /* in input clock cycles        */
    ULONG                   rxDepth;
    ULONG                   txDepth;
    char                    name[12];
};

struct dwi2c_staticdata
{
    struct Library      *OOPBase;
    struct Library      *utilityBase;
    APTR                 kernelBase;
    struct Library      *i2cBase;       /* the hidd.i2c base class      */

    OOP_AttrBase         hiddAB;
    OOP_AttrBase         hiddI2CAB;
    OOP_AttrBase         hiddI2CDeviceAB;
    OOP_AttrBase         hiddI2CDWAB;

    OOP_Class           *i2cDrvClass;

    struct dwi2c_ctrl    ctrl[DWI2C_MAX_CTRL];
    ULONG                ctrlCount;
};

struct DWI2CBase
{
    struct Library          LibNode;
    struct dwi2c_staticdata psd;
};

#define PSD(cl) (&((struct DWI2CBase *)cl->UserData)->psd)

/* Per-instance data of a bus object */
struct dwi2c_busdata
{
    struct dwi2c_ctrl   *ctrl;
};

#undef HiddAttrBase
#undef HiddI2CAttrBase
#undef HiddI2CDeviceAttrBase
#undef HiddI2CDWAttrBase

#define HiddAttrBase            (PSD(cl)->hiddAB)
#define HiddI2CAttrBase         (PSD(cl)->hiddI2CAB)
#define HiddI2CDeviceAttrBase   (PSD(cl)->hiddI2CDeviceAB)
#define HiddI2CDWAttrBase       (PSD(cl)->hiddI2CDWAB)

/*
 * These resolve through `cl`, so any function that has no class pointer
 * - the discovery code and the library entry points - has to #undef
 * them and use a local of the same name, or the psd field directly.
 */
#define UtilityBase             (PSD(cl)->utilityBase)
#define OOPBase                 (PSD(cl)->OOPBase)

/* The hardware, in dwi2c_hw.c. None of it needs a library base. */
void DWI2C_HWProbeFIFO(struct dwi2c_ctrl *ctrl);
void DWI2C_HWCalcTiming(struct dwi2c_ctrl *ctrl);
BOOL DWI2C_HWTransfer(struct dwi2c_ctrl *ctrl, UWORD address,
                      CONST_APTR writeBuffer, ULONG writeLength,
                      APTR readBuffer, ULONG readLength);

#endif /* DWI2C_INTERN_H */
