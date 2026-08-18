/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet device definitions
*/

#ifndef GENET_H
#define GENET_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <exec/devices.h>
#include <exec/interrupts.h>
#include <devices/timer.h>
#include <devices/sana2.h>

#include <oop/oop.h>

#define GENET_TASK_NAME     "BCM GENET Ethernet"
#define GENET_PORT_NAME     "BCM GENET I/O"

/* BCM2711 GENET base address (from DTB, typically 0xFD580000) */
#define GENET_BASE_DEFAULT  0xFD580000  /* RPi4; RPi5 uses RP1 */
#define GENET_SIZE          0x10000

/* Max units (only 1 GENET on RPi4) */
#define MAX_UNITS           1

/* Buffer sizes */
#define ETH_MTU             1500
#define ETH_HEADERSIZE      14
#define ETH_MAXPACKETSIZE   (ETH_MTU + ETH_HEADERSIZE + 4)
#define ETH_ADDRSIZE        6

#define RX_BUF_LENGTH       2048
#define TOTAL_DESCS         256
#define RX_DESCS            TOTAL_DESCS
#define TX_DESCS            TOTAL_DESCS

/* ============================================================
 * Register offsets (GENETv5 as on BCM2711)
 * From U-Boot drivers/net/bcmgenet.c
 * ============================================================ */

#define SYS_REV_CTRL            0x0000
#define SYS_PORT_CTRL           0x0004
#define PORT_MODE_EXT_GPHY      3

#define SYS_RBUF_FLUSH_CTRL     0x0008
#define SYS_TBUF_FLUSH_CTRL     0x000C

#define GENET_EXT_OFF           0x0080
#define EXT_RGMII_OOB_CTRL      (GENET_EXT_OFF + 0x0C)
#define RGMII_LINK              (1 << 4)
#define OOB_DISABLE             (1 << 5)
#define RGMII_MODE_EN           (1 << 6)
#define ID_MODE_DIS             (1 << 16)

#define GENET_RBUF_OFF          0x0300
#define RBUF_CTRL               (GENET_RBUF_OFF + 0x00)
#define RBUF_ALIGN_2B           (1 << 1)
#define RBUF_TBUF_SIZE_CTRL     (GENET_RBUF_OFF + 0xB4)

#define GENET_UMAC_OFF          0x0800
#define UMAC_CMD                (GENET_UMAC_OFF + 0x008)
#define UMAC_MAC0               (GENET_UMAC_OFF + 0x00C)
#define UMAC_MAC1               (GENET_UMAC_OFF + 0x010)
#define UMAC_MAX_FRAME_LEN      (GENET_UMAC_OFF + 0x014)
#define UMAC_TX_FLUSH           (GENET_UMAC_OFF + 0x334)
#define UMAC_MIB_CTRL           (GENET_UMAC_OFF + 0x580)
#define MDIO_CMD                (GENET_UMAC_OFF + 0x614)

#define CMD_TX_EN               (1 << 0)
#define CMD_RX_EN               (1 << 1)
#define CMD_SPEED_SHIFT         2
#define CMD_SPEED_MASK          3
#define CMD_SW_RESET            (1 << 13)
#define CMD_LCL_LOOP_EN         (1 << 15)

#define UMAC_SPEED_10           0
#define UMAC_SPEED_100          1
#define UMAC_SPEED_1000         2

#define MIB_RESET_RX            (1 << 0)
#define MIB_RESET_RUNT          (1 << 1)
#define MIB_RESET_TX            (1 << 2)

/* MDIO */
#define MDIO_START_BUSY         (1 << 29)
#define MDIO_READ_FAIL          (1 << 28)
#define MDIO_RD                 (2 << 26)
#define MDIO_WR                 (1 << 26)
#define MDIO_PMD_SHIFT          21
#define MDIO_REG_SHIFT          16

/* DMA descriptors */
#define DMA_DESC_LENGTH_STATUS  0x00
#define DMA_DESC_ADDRESS_LO     0x04
#define DMA_DESC_ADDRESS_HI     0x08
#define DMA_DESC_SIZE           12

#define DMA_EN                  (1 << 0)
#define DMA_RING_BUF_EN_SHIFT   1
#define DMA_BUFLENGTH_MASK      0x0FFF
#define DMA_BUFLENGTH_SHIFT     16
#define DMA_RING_SIZE_SHIFT     16
#define DMA_OWN                 0x8000
#define DMA_EOP                 0x4000
#define DMA_SOP                 0x2000
#define DMA_WRAP                0x1000
#define DMA_MAX_BURST_LENGTH    0x08

/* TX descriptor bits */
#define DMA_TX_APPEND_CRC       0x0040
#define DMA_TX_QTAG_SHIFT       7

/* DMA ring layout */
#define DMA_RING_SIZE           0x40
#define DEFAULT_Q               0x10
#define DMA_RINGS_SIZE          (DMA_RING_SIZE * (DEFAULT_Q + 1))

#define GENET_RX_OFF            0x2000
#define GENET_TX_OFF            0x4000
#define GENET_RDMA_REG_OFF      (GENET_RX_OFF + TOTAL_DESCS * DMA_DESC_SIZE)
#define GENET_TDMA_REG_OFF      (GENET_TX_OFF + TOTAL_DESCS * DMA_DESC_SIZE)

#define TDMA_RING_REG_BASE      (GENET_TDMA_REG_OFF + DEFAULT_Q * DMA_RING_SIZE)
#define TDMA_PROD_INDEX         (TDMA_RING_REG_BASE + 0x0C)
#define TDMA_CONS_INDEX         (TDMA_RING_REG_BASE + 0x08)
#define TDMA_READ_PTR           (TDMA_RING_REG_BASE + 0x00)
#define TDMA_WRITE_PTR          (TDMA_RING_REG_BASE + 0x2C)
#define TDMA_FLOW_PERIOD        (TDMA_RING_REG_BASE + 0x28)

#define RDMA_RING_REG_BASE      (GENET_RDMA_REG_OFF + DEFAULT_Q * DMA_RING_SIZE)
#define RDMA_PROD_INDEX         (RDMA_RING_REG_BASE + 0x08)
#define RDMA_CONS_INDEX         (RDMA_RING_REG_BASE + 0x0C)
#define RDMA_READ_PTR           (RDMA_RING_REG_BASE + 0x2C)
#define RDMA_WRITE_PTR          (RDMA_RING_REG_BASE + 0x00)
#define RDMA_XON_XOFF_THRESH    (RDMA_RING_REG_BASE + 0x28)

#define TDMA_REG_BASE           (GENET_TDMA_REG_OFF + DMA_RINGS_SIZE)
#define RDMA_REG_BASE           (GENET_RDMA_REG_OFF + DMA_RINGS_SIZE)

#define DMA_RING_CFG            0x00
#define DMA_CTRL                0x04
#define DMA_SCB_BURST_SIZE      0x0C

#define DMA_START_ADDR          0x14
#define DMA_END_ADDR            0x1C
#define DMA_MBUF_DONE_THRESH    0x24
#define DMA_RING_BUF_SIZE       0x10

#define DMA_FC_THRESH_HI        (RX_DESCS >> 4)
#define DMA_FC_THRESH_LO        5

/* PHY address on RPi4 (BCM54213PE) */
#define GENET_PHY_ADDR          1

/* IRQ numbers for BCM2711 GENET (GIC SPI) */
#define GENET_IRQ_0             157  /* Ring/default queue completion */
#define GENET_IRQ_1             158  /* Link/PHY events */

/* ============================================================
 * Driver structures
 * ============================================================ */

struct GENETUnit {
    struct Node         gn_Node;
    ULONG               gn_UnitNum;
    ULONG               gn_Flags;

    struct GENETBase    *gn_Device;
    IPTR                gn_RegBase;     /* MMIO base */

    /* MAC address */
    UBYTE               gn_DevAddr[ETH_ADDRSIZE];
    UBYTE               gn_OrgAddr[ETH_ADDRSIZE];

    /* DMA state */
    IPTR                gn_TxDescBase;
    IPTR                gn_RxDescBase;
    ULONG               gn_TxProdIdx;
    ULONG               gn_TxConsIdx;
    ULONG               gn_RxConsIdx;
    ULONG               gn_RxIdx;

    /* RX buffers (statically allocated ring) */
    UBYTE               *gn_RxBuffer;   /* RX_DESCS * RX_BUF_LENGTH */

    /* Task and signals */
    struct Task         *gn_Task;
    ULONG               gn_IntSig;
    APTR                gn_IRQHandle;

    /* SANA-II request queues */
    struct MinList      gn_ReadList;
    struct MinList      gn_WriteList;
    struct MinList      gn_EventList;
    struct MinList      gn_ReadOrphanList;
    struct MinList      gn_TypeTrackers;

    struct SignalSemaphore gn_Lock;

    /* Statistics */
    struct Sana2DeviceStats gn_Stats;

    /* PHY state */
    ULONG               gn_PhyAddr;
    ULONG               gn_LinkSpeed;   /* 10/100/1000 */
    BOOL                gn_LinkUp;
};

struct GENETBase {
    struct Device       gn_Device;
    struct GENETUnit    *gn_Units[MAX_UNITS];
    ULONG               gn_UnitCount;

    APTR                gn_KernelBase;
    struct Library      *gn_UtilityBase;

    struct SignalSemaphore gn_Lock;
};

/* Unit flags */
#define GNF_ONLINE      (1 << 0)
#define GNF_CONFIGURED  (1 << 1)

/* Opener tracking (per-opener state for SANA-II) */
struct Opener {
    struct MinNode      node;
    struct MsgPort      read_port;
    BOOL                (*rx_function)(APTR, APTR, ULONG);
    BOOL                (*tx_function)(APTR, APTR, ULONG);
    struct Hook         *filter_hook;
    struct MinList      initial_stats;
};

/* Type tracker for SANA-II S2_TRACKTYPE */
struct TypeTracker {
    struct MinNode      node;
    ULONG               packet_type;
    struct Sana2PacketTypeStats stats;
};

/* Function prototypes */
struct GENETUnit *genet_CreateUnit(struct GENETBase *base, ULONG unitnum);
void genet_DeleteUnit(struct GENETBase *base, struct GENETUnit *unit);

/* Hardware */
void genet_HW_Init(struct GENETUnit *unit);
void genet_HW_Stop(struct GENETUnit *unit);
int  genet_HW_Send(struct GENETUnit *unit, UBYTE *data, ULONG length);
void genet_HW_SetMAC(struct GENETUnit *unit, UBYTE *addr);

/* MDIO/PHY */
UWORD genet_MDIO_Read(struct GENETUnit *unit, ULONG phy, ULONG reg);
void  genet_MDIO_Write(struct GENETUnit *unit, ULONG phy, ULONG reg, UWORD val);
BOOL  genet_PHY_Init(struct GENETUnit *unit);

/* Unit task */
void genet_UnitTask(void);

/* Microsecond delay */
static inline void genet_udelay(ULONG us)
{
#if defined(__aarch64__)
    uint64_t frq, start, now;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(frq));
    asm volatile("mrs %0, cntpct_el0" : "=r"(start));
    uint64_t ticks = ((uint64_t)us * frq) / 1000000;
    do {
        asm volatile("mrs %0, cntpct_el0" : "=r"(now));
    } while ((now - start) < ticks);
#else
    volatile ULONG i;
    for (i = 0; i < us * 50; i++)
        asm volatile("" ::: "memory");
#endif
}

#endif /* GENET_H */
