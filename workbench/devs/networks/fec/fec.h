/*
 * fec.h
 *
 *  Created on: May 15, 2009
 *      Author: misc
 */

#ifndef FEC_H_
#define FEC_H_

#include <inttypes.h>

#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/lists.h>
#include <exec/interrupts.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

enum {
    WRITE_QUEUE,
    ADOPT_QUEUE,
    EVENT_QUEUE,
    GENERAL_QUEUE,
    REQUEST_QUEUE_COUNT
};

/* Bits and pieces taken from linux */

typedef struct {
    uint32_t fec_id;                     /* FEC + 0x000 */
    uint32_t ievent;                     /* FEC + 0x004 */
    uint32_t imask;                      /* FEC + 0x008 */

    uint32_t reserved0[1];               /* FEC + 0x00C */
    uint32_t r_des_active;               /* FEC + 0x010 */
    uint32_t x_des_active;               /* FEC + 0x014 */
    uint32_t r_des_active_cl;            /* FEC + 0x018 */
    uint32_t x_des_active_cl;            /* FEC + 0x01C */
    uint32_t ivent_set;                  /* FEC + 0x020 */
    uint32_t ecntrl;                     /* FEC + 0x024 */

    uint32_t reserved1[6];               /* FEC + 0x028-03C */
    uint32_t mii_data;                   /* FEC + 0x040 */
    uint32_t mii_speed;                  /* FEC + 0x044 */
    uint32_t mii_status;                 /* FEC + 0x048 */

    uint32_t reserved2[5];               /* FEC + 0x04C-05C */
    uint32_t mib_data;                   /* FEC + 0x060 */
    uint32_t mib_control;                /* FEC + 0x064 */

    uint32_t reserved3[6];               /* FEC + 0x068-7C */
    uint32_t r_activate;                 /* FEC + 0x080 */
    uint32_t r_cntrl;                    /* FEC + 0x084 */
    uint32_t r_hash;                     /* FEC + 0x088 */
    uint32_t r_data;                     /* FEC + 0x08C */
    uint32_t ar_done;                    /* FEC + 0x090 */
    uint32_t r_test;                     /* FEC + 0x094 */
    uint32_t r_mib;                      /* FEC + 0x098 */
    uint32_t r_da_low;                   /* FEC + 0x09C */
    uint32_t r_da_high;                  /* FEC + 0x0A0 */

    uint32_t reserved4[7];               /* FEC + 0x0A4-0BC */
    uint32_t x_activate;                 /* FEC + 0x0C0 */
    uint32_t x_cntrl;                    /* FEC + 0x0C4 */
    uint32_t backoff;                    /* FEC + 0x0C8 */
    uint32_t x_data;                     /* FEC + 0x0CC */
    uint32_t x_status;                   /* FEC + 0x0D0 */
    uint32_t x_mib;                      /* FEC + 0x0D4 */
    uint32_t x_test;                     /* FEC + 0x0D8 */
    uint32_t fdxfc_da1;                  /* FEC + 0x0DC */
    uint32_t fdxfc_da2;                  /* FEC + 0x0E0 */
    uint32_t paddr1;                     /* FEC + 0x0E4 */
    uint32_t paddr2;                     /* FEC + 0x0E8 */
    uint32_t op_pause;                   /* FEC + 0x0EC */

    uint32_t reserved5[4];               /* FEC + 0x0F0-0FC */
    uint32_t instr_reg;                  /* FEC + 0x100 */
    uint32_t context_reg;                /* FEC + 0x104 */
    uint32_t test_cntrl;                 /* FEC + 0x108 */
    uint32_t acc_reg;                    /* FEC + 0x10C */
    uint32_t ones;                       /* FEC + 0x110 */
    uint32_t zeros;                      /* FEC + 0x114 */
    uint32_t iaddr1;                     /* FEC + 0x118 */
    uint32_t iaddr2;                     /* FEC + 0x11C */
    uint32_t gaddr1;                     /* FEC + 0x120 */
    uint32_t gaddr2;                     /* FEC + 0x124 */
    uint32_t random;                     /* FEC + 0x128 */
    uint32_t rand1;                      /* FEC + 0x12C */
    uint32_t tmp;                        /* FEC + 0x130 */

    uint32_t reserved6[3];               /* FEC + 0x134-13C */
    uint32_t fifo_id;                    /* FEC + 0x140 */
    uint32_t x_wmrk;                     /* FEC + 0x144 */
    uint32_t fcntrl;                     /* FEC + 0x148 */
    uint32_t r_bound;                    /* FEC + 0x14C */
    uint32_t r_fstart;                   /* FEC + 0x150 */
    uint32_t r_count;                    /* FEC + 0x154 */
    uint32_t r_lag;                      /* FEC + 0x158 */
    uint32_t r_read;                     /* FEC + 0x15C */
    uint32_t r_write;                    /* FEC + 0x160 */
    uint32_t x_count;                    /* FEC + 0x164 */
    uint32_t x_lag;                      /* FEC + 0x168 */
    uint32_t x_retry;                    /* FEC + 0x16C */
    uint32_t x_write;                    /* FEC + 0x170 */
    uint32_t x_read;                     /* FEC + 0x174 */

    uint32_t reserved7[2];               /* FEC + 0x178-17C */
    uint32_t fm_cntrl;                   /* FEC + 0x180 */
    uint32_t rfifo_data;                 /* FEC + 0x184 */
    uint32_t rfifo_status;               /* FEC + 0x188 */
    uint32_t rfifo_cntrl;                /* FEC + 0x18C */
    uint32_t rfifo_lrf_ptr;              /* FEC + 0x190 */
    uint32_t rfifo_lwf_ptr;              /* FEC + 0x194 */
    uint32_t rfifo_alarm;                /* FEC + 0x198 */
    uint32_t rfifo_rdptr;                /* FEC + 0x19C */
    uint32_t rfifo_wrptr;                /* FEC + 0x1A0 */
    uint32_t tfifo_data;                 /* FEC + 0x1A4 */
    uint32_t tfifo_status;               /* FEC + 0x1A8 */
    uint32_t tfifo_cntrl;                /* FEC + 0x1AC */
    uint32_t tfifo_lrf_ptr;              /* FEC + 0x1B0 */
    uint32_t tfifo_lwf_ptr;              /* FEC + 0x1B4 */
    uint32_t tfifo_alarm;                /* FEC + 0x1B8 */
    uint32_t tfifo_rdptr;                /* FEC + 0x1BC */
    uint32_t tfifo_wrptr;                /* FEC + 0x1C0 */

    uint32_t reset_cntrl;                /* FEC + 0x1C4 */
    uint32_t xmit_fsm;                   /* FEC + 0x1C8 */

    uint32_t reserved8[3];               /* FEC + 0x1CC-1D4 */
    uint32_t rdes_data0;                 /* FEC + 0x1D8 */
    uint32_t rdes_data1;                 /* FEC + 0x1DC */
    uint32_t r_length;                   /* FEC + 0x1E0 */
    uint32_t x_length;                   /* FEC + 0x1E4 */
    uint32_t x_addr;                     /* FEC + 0x1E8 */
    uint32_t cdes_data;                  /* FEC + 0x1EC */
    uint32_t status;                     /* FEC + 0x1F0 */
    uint32_t dma_control;                /* FEC + 0x1F4 */
    uint32_t des_cmnd;                   /* FEC + 0x1F8 */
    uint32_t data;                       /* FEC + 0x1FC */

    uint32_t rmon_t_drop;                /* FEC + 0x200 */
    uint32_t rmon_t_packets;             /* FEC + 0x204 */
    uint32_t rmon_t_bc_pkt;              /* FEC + 0x208 */
    uint32_t rmon_t_mc_pkt;              /* FEC + 0x20C */
    uint32_t rmon_t_crc_align;           /* FEC + 0x210 */
    uint32_t rmon_t_undersize;           /* FEC + 0x214 */
    uint32_t rmon_t_oversize;            /* FEC + 0x218 */
    uint32_t rmon_t_frag;                /* FEC + 0x21C */
    uint32_t rmon_t_jab;                 /* FEC + 0x220 */
    uint32_t rmon_t_col;                 /* FEC + 0x224 */
    uint32_t rmon_t_p64;                 /* FEC + 0x228 */
    uint32_t rmon_t_p65to127;            /* FEC + 0x22C */
    uint32_t rmon_t_p128to255;           /* FEC + 0x230 */
    uint32_t rmon_t_p256to511;           /* FEC + 0x234 */
    uint32_t rmon_t_p512to1023;          /* FEC + 0x238 */
    uint32_t rmon_t_p1024to2047;         /* FEC + 0x23C */
    uint32_t rmon_t_p_gte2048;           /* FEC + 0x240 */
    uint32_t rmon_t_octets;              /* FEC + 0x244 */
    uint32_t ieee_t_drop;                /* FEC + 0x248 */
    uint32_t ieee_t_frame_ok;            /* FEC + 0x24C */
    uint32_t ieee_t_1col;                /* FEC + 0x250 */
    uint32_t ieee_t_mcol;                /* FEC + 0x254 */
    uint32_t ieee_t_def;                 /* FEC + 0x258 */
    uint32_t ieee_t_lcol;                /* FEC + 0x25C */
    uint32_t ieee_t_excol;               /* FEC + 0x260 */
    uint32_t ieee_t_macerr;              /* FEC + 0x264 */
    uint32_t ieee_t_cserr;               /* FEC + 0x268 */
    uint32_t ieee_t_sqe;                 /* FEC + 0x26C */
    uint32_t t_fdxfc;                    /* FEC + 0x270 */
    uint32_t ieee_t_octets_ok;           /* FEC + 0x274 */

    uint32_t reserved9[2];               /* FEC + 0x278-27C */
    uint32_t rmon_r_drop;                /* FEC + 0x280 */
    uint32_t rmon_r_packets;             /* FEC + 0x284 */
    uint32_t rmon_r_bc_pkt;              /* FEC + 0x288 */
    uint32_t rmon_r_mc_pkt;              /* FEC + 0x28C */
    uint32_t rmon_r_crc_align;           /* FEC + 0x290 */
    uint32_t rmon_r_undersize;           /* FEC + 0x294 */
    uint32_t rmon_r_oversize;            /* FEC + 0x298 */
    uint32_t rmon_r_frag;                /* FEC + 0x29C */
    uint32_t rmon_r_jab;                 /* FEC + 0x2A0 */

    uint32_t rmon_r_resvd_0;             /* FEC + 0x2A4 */

    uint32_t rmon_r_p64;                 /* FEC + 0x2A8 */
    uint32_t rmon_r_p65to127;            /* FEC + 0x2AC */
    uint32_t rmon_r_p128to255;           /* FEC + 0x2B0 */
    uint32_t rmon_r_p256to511;           /* FEC + 0x2B4 */
    uint32_t rmon_r_p512to1023;          /* FEC + 0x2B8 */
    uint32_t rmon_r_p1024to2047;         /* FEC + 0x2BC */
    uint32_t rmon_r_p_gte2048;           /* FEC + 0x2C0 */
    uint32_t rmon_r_octets;              /* FEC + 0x2C4 */
    uint32_t ieee_r_drop;                /* FEC + 0x2C8 */
    uint32_t ieee_r_frame_ok;            /* FEC + 0x2CC */
    uint32_t ieee_r_crc;                 /* FEC + 0x2D0 */
    uint32_t ieee_r_align;               /* FEC + 0x2D4 */
    uint32_t r_macerr;                   /* FEC + 0x2D8 */
    uint32_t r_fdxfc;                    /* FEC + 0x2DC */
    uint32_t ieee_r_octets_ok;           /* FEC + 0x2E0 */

    uint32_t reserved10[7];              /* FEC + 0x2E4-2FC */

    uint32_t reserved11[64];             /* FEC + 0x300-3FF */
} fec_t;

#define FEC_MIB_DISABLE                 0x80000000

#define FEC_IEVENT_HBERR                0x80000000
#define FEC_IEVENT_BABR                 0x40000000
#define FEC_IEVENT_BABT                 0x20000000
#define FEC_IEVENT_GRA                  0x10000000
#define FEC_IEVENT_TFINT                0x08000000
#define FEC_IEVENT_MII                  0x00800000
#define FEC_IEVENT_LATE_COL             0x00200000
#define FEC_IEVENT_COL_RETRY_LIM        0x00100000
#define FEC_IEVENT_XFIFO_UN             0x00080000
#define FEC_IEVENT_XFIFO_ERROR          0x00040000
#define FEC_IEVENT_RFIFO_ERROR          0x00020000

#define FEC_IMASK_HBERR                 0x80000000
#define FEC_IMASK_BABR                  0x40000000
#define FEC_IMASK_BABT                  0x20000000
#define FEC_IMASK_GRA                   0x10000000
#define FEC_IMASK_MII                   0x00800000
#define FEC_IMASK_LATE_COL              0x00200000
#define FEC_IMASK_COL_RETRY_LIM         0x00100000
#define FEC_IMASK_XFIFO_UN              0x00080000
#define FEC_IMASK_XFIFO_ERROR           0x00040000
#define FEC_IMASK_RFIFO_ERROR           0x00020000

/* all but MII, which is enabled separately */
#define FEC_IMASK_ENABLE        (FEC_IMASK_HBERR | FEC_IMASK_BABR | \
                FEC_IMASK_BABT | FEC_IMASK_GRA | FEC_IMASK_LATE_COL | \
                FEC_IMASK_COL_RETRY_LIM | FEC_IMASK_XFIFO_UN | \
                FEC_IMASK_XFIFO_ERROR | FEC_IMASK_RFIFO_ERROR)

#define FEC_RCNTRL_MAX_FL_SHIFT         16
#define FEC_RCNTRL_LOOP                 0x01
#define FEC_RCNTRL_DRT                  0x02
#define FEC_RCNTRL_MII_MODE             0x04
#define FEC_RCNTRL_PROM                 0x08
#define FEC_RCNTRL_BC_REJ               0x10
#define FEC_RCNTRL_FCE                  0x20

#define FEC_TCNTRL_GTS                  0x00000001
#define FEC_TCNTRL_HBC                  0x00000002
#define FEC_TCNTRL_FDEN                 0x00000004
#define FEC_TCNTRL_TFC_PAUSE            0x00000008
#define FEC_TCNTRL_RFC_PAUSE            0x00000010

#define FEC_ECNTRL_RESET                0x00000001
#define FEC_ECNTRL_ETHER_EN             0x00000002

#define FEC_MII_DATA_ST                 0x40000000      /* Start frame */
#define FEC_MII_DATA_OP_RD              0x20000000      /* Perform read */
#define FEC_MII_DATA_OP_WR              0x10000000      /* Perform write */
#define FEC_MII_DATA_PA_MSK             0x0f800000      /* PHY Address mask */
#define FEC_MII_DATA_RA_MSK             0x007c0000      /* PHY Register mask */
#define FEC_MII_DATA_TA                 0x00020000      /* Turnaround */
#define FEC_MII_DATA_DATAMSK            0x0000ffff      /* PHY data mask */

#define FEC_MII_READ_FRAME      (FEC_MII_DATA_ST | FEC_MII_DATA_OP_RD | FEC_MII_DATA_TA)
#define FEC_MII_WRITE_FRAME     (FEC_MII_DATA_ST | FEC_MII_DATA_OP_WR | FEC_MII_DATA_TA)

#define FEC_MII_DATA_RA_SHIFT           0x12            /* MII reg addr bits */
#define FEC_MII_DATA_PA_SHIFT           0x17            /* MII PHY addr bits */

#define FEC_PHYADDR_NONE				-1
#define FEC_PHYADDR_7WIRE				-2

#define FEC_PADDR2_TYPE                 0x8808

#define FEC_OP_PAUSE_OPCODE             0x00010000

#define FEC_FIFO_WMRK_256B              0x3

#define FEC_FIFO_STATUS_ERR             0x00400000
#define FEC_FIFO_STATUS_UF              0x00200000
#define FEC_FIFO_STATUS_OF              0x00100000

#define FEC_FIFO_CNTRL_FRAME            0x08000000
#define FEC_FIFO_CNTRL_LTG_7            0x07000000

#define FEC_RESET_CNTRL_RESET_FIFO      0x02000000
#define FEC_RESET_CNTRL_ENABLE_IS_RESET 0x01000000

#define FEC_XMIT_FSM_APPEND_CRC         0x02000000
#define FEC_XMIT_FSM_ENABLE_CRC         0x01000000

struct Opener
{
    struct MinNode  node;
    struct MsgPort  read_port;
    BOOL            (*rx_function)(APTR, APTR, ULONG);
    BOOL            (*tx_function)(APTR, APTR, ULONG);
    struct Hook     *filter_hook;
    struct MinList  initial_stats;
};

struct TypeStats
{
    struct MinNode node;
    ULONG packet_type;
    struct Sana2PacketTypeStats stats;
};

struct TypeTracker
{
    struct MinNode node;
    ULONG packet_type;
    struct Sana2PacketTypeStats stats;
    ULONG user_count;
};

struct AddressRange
{
    struct MinNode node;
    ULONG add_count;
    ULONG lower_bound_left;
    ULONG upper_bound_left;
    UWORD lower_bound_right;
    UWORD upper_bound_right;
};


struct FECUnit;

struct FECBase {
	struct Device               feb_Device;
    struct Sana2DeviceQuery     feb_Sana2Info;
    void                        *feb_Pool;

    struct FECUnit				*feb_Unit;
};

/* Standard interface flags (netdevice->flags). */
#define IFF_UP          0x1             /* interface is up              */
#define IFF_BROADCAST   0x2             /* broadcast address valid      */
#define IFF_DEBUG       0x4             /* turn on debugging            */
#define IFF_LOOPBACK    0x8             /* is a loopback net            */
#define IFF_POINTOPOINT 0x10            /* interface is has p-p link    */
#define IFF_NOTRAILERS  0x20            /* avoid use of trailers        */
#define IFF_RUNNING     0x40            /* resources allocated          */
#define IFF_NOARP       0x80            /* no ARP protocol              */
#define IFF_PROMISC     0x100           /* receive all packets          */
#define IFF_ALLMULTI    0x200           /* receive all multicast packets*/

#define IFF_MASTER      0x400           /* master of a load balancer    */
#define IFF_SLAVE       0x800           /* slave of a load balancer     */

#define IFF_MULTICAST   0x1000          /* Supports multicast           */

#define IFF_VOLATILE    (IFF_LOOPBACK|IFF_POINTOPOINT|IFF_BROADCAST|IFF_MASTER|IFF_SLAVE|IFF_RUNNING)

#define IFF_PORTSEL     0x2000          /* can set media type           */
#define IFF_AUTOMEDIA   0x4000          /* auto media select active     */
#define IFF_DYNAMIC     0x8000          /* dialup device with changing addresses*/
#define IFF_SHARED      0x10000         /* interface may be shared */
#define IFF_CONFIGURED  0x20000         /* interface already configured */


#define ETH_DATA_LEN        1500
#define ETH_ADDRESSSIZE     6
#define ETH_HEADERSIZE      14
#define ETH_CRCSIZE         4
#define ETH_MTU             (ETH_DATA_LEN)
#define ETH_MAXPACKETSIZE   ((ETH_HEADERSIZE) + (ETH_MTU) + (ETH_CRCSIZE))

#define ETH_PACKET_DEST     0
#define ETH_PACKET_SOURCE   6
#define ETH_PACKET_TYPE     12
#define ETH_PACKET_IEEELEN  12
#define ETH_PACKET_SNAPTYPE 20
#define ETH_PACKET_DATA     14
#define ETH_PACKET_CRC      (ETH_PACKET_DATA + ETH_MTU)

#define RXTX_ALLOC_BUFSIZE  (ETH_MAXPACKETSIZE + 26)

/* PHY definitions */

/* phy seed setup */
#define AUTO                    99
#define _1000BASET              1000
#define _100BASET               100
#define _10BASET                10
#define HALF                    22
#define FULL                    44

/* phy register offsets */
#define PHY_BMCR                0x00
#define PHY_BMSR                0x01
#define PHY_PHYIDR1             0x02
#define PHY_PHYIDR2             0x03
#define PHY_ANAR                0x04
#define PHY_ANLPAR              0x05
#define PHY_ANER                0x06
#define PHY_ANNPTR              0x07
#define PHY_ANLPNP              0x08
#define PHY_1000BTCR            0x09
#define PHY_1000BTSR            0x0A
#define PHY_EXSR                0x0F
#define PHY_PHYSTS              0x10
#define PHY_MIPSCR              0x11
#define PHY_MIPGSR              0x12
#define PHY_DCR                 0x13
#define PHY_FCSCR               0x14
#define PHY_RECR                0x15
#define PHY_PCSR                0x16
#define PHY_LBR                 0x17
#define PHY_10BTSCR             0x18
#define PHY_PHYCTRL             0x19

/* PHY BMCR */
#define PHY_BMCR_RESET          0x8000
#define PHY_BMCR_LOOP           0x4000
#define PHY_BMCR_100MB          0x2000
#define PHY_BMCR_AUTON          0x1000
#define PHY_BMCR_POWD           0x0800
#define PHY_BMCR_ISO            0x0400
#define PHY_BMCR_RST_NEG        0x0200
#define PHY_BMCR_DPLX           0x0100
#define PHY_BMCR_COL_TST        0x0080

#define PHY_BMCR_SPEED_MASK     0x2040
#define PHY_BMCR_1000_MBPS      0x0040
#define PHY_BMCR_100_MBPS       0x2000
#define PHY_BMCR_10_MBPS        0x0000

/* phy BMSR */
#define PHY_BMSR_100T4          0x8000
#define PHY_BMSR_100TXF         0x4000
#define PHY_BMSR_100TXH         0x2000
#define PHY_BMSR_10TF           0x1000
#define PHY_BMSR_10TH           0x0800
#define PHY_BMSR_EXT_STAT       0x0100
#define PHY_BMSR_PRE_SUP        0x0040
#define PHY_BMSR_AUTN_COMP      0x0020
#define PHY_BMSR_RF             0x0010
#define PHY_BMSR_AUTN_ABLE      0x0008
#define PHY_BMSR_LS             0x0004
#define PHY_BMSR_JD             0x0002
#define PHY_BMSR_EXT            0x0001

/*phy ANLPAR */
#define PHY_ANLPAR_NP           0x8000
#define PHY_ANLPAR_ACK          0x4000
#define PHY_ANLPAR_RF           0x2000
#define PHY_ANLPAR_ASYMP        0x0800
#define PHY_ANLPAR_PAUSE        0x0400
#define PHY_ANLPAR_T4           0x0200
#define PHY_ANLPAR_TXFD         0x0100
#define PHY_ANLPAR_TX           0x0080
#define PHY_ANLPAR_10FD         0x0040
#define PHY_ANLPAR_10           0x0020
#define PHY_ANLPAR_100          0x0380  /* we can run at 100 */
/* phy ANLPAR 1000BASE-X */
#define PHY_X_ANLPAR_NP         0x8000
#define PHY_X_ANLPAR_ACK        0x4000
#define PHY_X_ANLPAR_RF_MASK    0x3000
#define PHY_X_ANLPAR_PAUSE_MASK 0x0180
#define PHY_X_ANLPAR_HD         0x0040
#define PHY_X_ANLPAR_FD         0x0020

#define PHY_ANLPAR_PSB_MASK     0x001f
#define PHY_ANLPAR_PSB_802_3    0x0001
#define PHY_ANLPAR_PSB_802_9    0x0002

/* phy 1000BTCR */
#define PHY_1000BTCR_1000FD     0x0200
#define PHY_1000BTCR_1000HD     0x0100

/* phy 1000BTSR */
#define PHY_1000BTSR_MSCF       0x8000
#define PHY_1000BTSR_MSCR       0x4000
#define PHY_1000BTSR_LRS        0x2000
#define PHY_1000BTSR_RRS        0x1000
#define PHY_1000BTSR_1000FD     0x0800
#define PHY_1000BTSR_1000HD     0x0400

/* phy EXSR */
#define PHY_EXSR_1000XF         0x8000
#define PHY_EXSR_1000XH         0x4000
#define PHY_EXSR_1000TF         0x2000
#define PHY_EXSR_1000TH         0x1000

struct FECUnit {
	struct Unit				feu_Unit;
	struct SignalSemaphore	feu_Lock;
	struct MinList      	feu_Openers;
	struct MinList      	feu_MulticastRanges;
	struct MinList      	feu_TypeTrackers;
	struct Process      	*feu_Process;
	struct FECBase     		*feu_FECBase;
	void                	*feu_IRQHandler;

	struct Interrupt    	feu_TXInt;
	struct Interrupt    	feu_RXInt;
	struct Interrupt    	feu_TXEndInt;

	fec_t					*feu_regs;

	uint32_t				feu_phy_speed;
	int8_t					feu_phy_id;

	uint16_t				feu_speed;
	uint8_t					feu_duplex;
	uint8_t					feu_link;

    uint8_t             	feu_DevAddr[ETH_ADDRESSSIZE];
    uint8_t             	feu_OrgAddr[ETH_ADDRESSSIZE];

    uint32_t            	feu_Flags;
    uint32_t            	feu_OpenCount;
    int32_t             	feu_RangeCount;

    struct MsgPort      	*feu_RequestPorts[REQUEST_QUEUE_COUNT];
    struct MsgPort      	*feu_InputPort;

    struct MsgPort      	feu_TimerPort;
    struct timerequest  	feu_TimerRequest;

    int                		(*start)(struct FECUnit *);
    int                		(*stop)(struct FECUnit *);
    void               		(*set_multicast)(struct FECUnit *);
    void               		(*set_mac_address)(struct FECUnit *);
};


int FEC_CreateUnit(struct FECBase *FECBase, fec_t *regs);
void handle_request(struct FECBase *FECBase, struct IOSana2Req *request);
void FEC_UDelay(struct FECUnit *unit, uint32_t usec);
int FEC_MDIO_Read(struct FECUnit *unit, int32_t phy_id, int32_t reg);
int FEC_MDIO_Write(struct FECUnit *unit, int32_t phy_id, int32_t reg, uint16_t data);
void FEC_HW_Init(struct FECUnit *unit);
void FEC_PHY_Init(struct FECUnit *unit);
void FEC_Reset_Stats(struct FECUnit *unit);
int8_t FEC_PHY_Find(struct FECUnit *unit);
int FEC_PHY_Reset(struct FECUnit *unit);
int FEC_PHY_Link(struct FECUnit *unit);
int FEC_PHY_Speed(struct FECUnit *unit);
int FEC_PHY_Duplex(struct FECUnit *unit);
void FEC_PHY_Setup_Autonegotiation(struct FECUnit *unit);

#endif /* FEC_H_ */
