#ifndef EMAC_H_
#define EMAC_H_

#include <inttypes.h>
#include <exec/libraries.h>
#include <exec/devices.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/dosextens.h>
#include <devices/timer.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include "mal.h"

#define EMAC_TASK1_NAME      "IBM EMAC0 task"
#define EMAC_TASK2_NAME      "IBM EMAC1 task"
#define EMAC_PORT_NAME       "IBM EMAC port"

enum {
    WRITE_QUEUE,
    ADOPT_QUEUE,
    EVENT_QUEUE,
    GENERAL_QUEUE,
    REQUEST_QUEUE_COUNT
};

struct EMACUnit;

struct EMACBase {
    struct Device               emb_Device;
    struct Sana2DeviceQuery     emb_Sana2Info;
    void                        *emb_Pool;

    struct EMACUnit             *emb_Units[2];

    mal_descriptor_t            *emb_MALRXChannels[2];
    mal_descriptor_t            *emb_MALTXChannels[4];

    void                        *emb_MALHandlers[5];
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

struct eth_frame {
    uint8_t     eth_packet_dest[6];
    uint8_t     eth_packet_source[6];
    uint16_t    eth_packet_type;
    uint8_t     eth_packet_data[ETH_MTU];
    uint8_t     eth_packet_crc[4];
    uint8_t     eth_pad[RXTX_ALLOC_BUFSIZE - ETH_MAXPACKETSIZE];
} __attribute__((packed));

#define TX_RING_SIZE      32    /* 256 max */
#define RX_RING_SIZE      64    /* 256 max */

#define STAT_COUNT 3

struct EMACUnit {
    struct SignalSemaphore      eu_Lock;
    struct MinList      eu_Openers;
    struct MinList      eu_MulticastRanges;
    struct MinList      eu_TypeTrackers;
    struct Process      *eu_Process;
    struct EMACBase     *eu_EMACBase;
    void                *eu_IRQHandler;

    struct Interrupt    eu_TXInt;
    struct Interrupt    eu_RXInt;
    struct Interrupt    eu_TXEndInt;

    mal_packet_t        *eu_RXChannel;
    mal_packet_t        *eu_TXChannel;
    uint8_t             eu_LastRXSlot;
    uint8_t             eu_LastTXSlot;

    intptr_t            eu_IOBase;
    uint32_t            eu_Flags;
    uint32_t            eu_OpenCount;
    uint32_t            eu_IER;
    int32_t             eu_RangeCount;

    uint16_t            eu_MTU;

    uint8_t             eu_DevAddr[ETH_ADDRESSSIZE];
    uint8_t             eu_OrgAddr[ETH_ADDRESSSIZE];

    uint8_t             eu_UnitNum;
    uint8_t             eu_PHYAddr;

    struct Sana2DeviceStats eu_Stats;
    uint32_t            eu_SpecialStats[STAT_COUNT];

    int                (*start)(struct EMACUnit *);
    int                (*stop)(struct EMACUnit *);
    void               (*udelay)(struct EMACUnit *, uint32_t usec);
    void               (*set_multicast)(struct EMACUnit *);
    void               (*set_mac_address)(struct EMACUnit *);

    struct MsgPort      *eu_RequestPorts[REQUEST_QUEUE_COUNT];
    struct MsgPort      *eu_InputPort;

    struct MsgPort      eu_TimerPort;
    struct timerequest  eu_TimerRequest;
};

void EMAC_Startup(struct EMACUnit *unit);
void EMACIRQHandler(struct EMACBase *EMACBase, struct EMACUnit *Unit);
struct EMACUnit *CreateUnit(struct EMACBase *EMACBase, uint8_t num);
void handle_request(struct EMACBase *EMACBase, struct IOSana2Req *request);
BOOL AddMulticastRange(struct EMACBase *EMACBase, struct EMACUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound);
BOOL RemMulticastRange(struct EMACBase *EMACBase, struct EMACUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound);
struct TypeStats *FindTypeStats(struct EMACBase *EMACBase, struct EMACUnit *unit,
                    struct MinList *list, ULONG packet_type);

int EMAC_miiphy_read(struct EMACUnit *unit, uint8_t reg, uint16_t *value);
int EMAC_miiphy_write(struct EMACUnit *unit, uint8_t reg, uint16_t value);
int EMAC_miiphy_reset(struct EMACUnit *unit);
int EMAC_miiphy_speed(struct EMACUnit *unit);
int EMAC_miiphy_duplex(struct EMACUnit *unit);
int EMAC_miiphy_link(struct EMACUnit *unit);
int EMAC_phy_setup_aneg (struct EMACUnit *unit);

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


#endif /*EMAC_H_*/
