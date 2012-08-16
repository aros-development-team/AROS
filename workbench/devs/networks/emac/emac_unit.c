#define DEBUG 0
#include <aros/debug.h>
#include <aros/macros.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include <inttypes.h>

#include <proto/kernel.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/processor.h>

#include <utility/utility.h>
#include <utility/hooks.h>
#include <utility/tagitem.h>

#include <resources/processor.h>

#include "emac.h"
#include LC_LIBDEFS_FILE

// TODO: Implement CmdFlush!!!!!!!!!

static void EMAC_UDelay(struct EMACUnit *unit, uint32_t usec)
{
    unit->eu_TimerPort.mp_SigTask = FindTask(NULL);
    unit->eu_TimerRequest.tr_node.io_Command = TR_ADDREQUEST;
    unit->eu_TimerRequest.tr_time.tv_secs = usec / 1000000;
    unit->eu_TimerRequest.tr_time.tv_micro = usec % 1000000;

    DoIO((struct IORequest *)&unit->eu_TimerRequest);
}

static int EMAC_Start(struct EMACUnit *unit)
{
    void *stack;
    uint16_t reg_short;
    uint32_t reg, mode_reg, speed, duplex;
    int i;

    D(bug("[EMAC%d] start()\n", unit->eu_UnitNum));

    /*
     *  Enable MAL channels:
     * TX Channels 0 and 1 are assigned to unit 0, Channels 2 and 3 are assigned to unit 1.
     * RX Channel 0 is assigned to unit 0, channel 1 is assigned to unit 1.
     */
    stack = SuperState();
    D(bug("[EMAC%d] Enable MAL\n", unit->eu_UnitNum));
    wrdcr(MAL0_RXCASR, 0x80000000 >> unit->eu_UnitNum);
    wrdcr(MAL0_TXCASR, 0x80000000 >> (2*unit->eu_UnitNum));
    UserState(stack);

    /* set RMII mode */
    outl(0, ZMII_FER);
    unit->udelay(unit, 100);
    outl((ZMII_FER_RMII | ZMII_FER_MDI) << ZMII_FER_V (unit->eu_UnitNum), ZMII_FER);
    outl(ZMII_SSR_SP << ZMII_SSR_V(unit->eu_UnitNum), ZMII_SSR);

    /* Reset EMAC */
    outl(EMAC_M0_SRST, EMAC_M0 + unit->eu_IOBase);
    while(inl(EMAC_M0 + unit->eu_IOBase) & EMAC_M0_SRST)
        unit->udelay(unit, 1000);

    EMAC_miiphy_reset(unit);

    /* Start/Restart autonegotiation */
    EMAC_phy_setup_aneg(unit);
    unit->udelay(unit, 1000);

    EMAC_miiphy_read (unit, PHY_BMSR, &reg_short);

    /*
     * Wait if PHY is capable of autonegotiation and autonegotiation is not complete
     */
    if ((reg_short & PHY_BMSR_AUTN_ABLE)
         && !(reg_short & PHY_BMSR_AUTN_COMP)) {
             D(bug("[EMAC%d] Waiting for PHY auto negotiation to complete", unit->eu_UnitNum));
             i = 0;
             while (!(reg_short & PHY_BMSR_AUTN_COMP)) {
                     /*
                      * Timeout reached ?
                      */
                     if (i > 10000) {
                             D(bug(" TIMEOUT !\n"));
                             break;
                     }

                     if ((i++ % 1000) == 0) {
                             D(bug("."));
                     }
                     unit->udelay (unit, 1000);  /* 1 ms */
                     EMAC_miiphy_read (unit, PHY_BMSR, &reg_short);

             }
             D(bug(" done\n"));
             unit->udelay (unit, 500000);        /* another 500 ms (results in faster booting) */
     }

    speed = EMAC_miiphy_speed (unit);
    duplex = EMAC_miiphy_duplex (unit);

    D(bug("[EMAC%d] Speed is %d Mbps - %s duplex connection\n", unit->eu_UnitNum,
                    (int) speed, (duplex == HALF) ? "HALF" : "FULL"));

    stack = SuperState();
    wrdcr(SDR0_CFGADDR, SDR0_MFR);
    reg = rddcr(SDR0_CFGDATA);
    if (speed == 100) {
        reg = (reg & ~SDR0_MFR_ZMII_MODE_MASK) | SDR0_MFR_ZMII_MODE_RMII_100M;
    } else {
        reg = (reg & ~SDR0_MFR_ZMII_MODE_MASK) | SDR0_MFR_ZMII_MODE_RMII_10M;
    }
    wrdcr(SDR0_CFGADDR, SDR0_MFR);
    wrdcr(SDR0_CFGDATA, reg);
    UserState(stack);

    /* Set ZMII/RGMII speed according to the phy link speed */
    reg = inl(ZMII_SSR);
    if ( (speed == 100) || (speed == 1000) )
            outl (reg | (ZMII_SSR_SP << ZMII_SSR_V (unit->eu_UnitNum)), ZMII_SSR);
    else
            outl (reg & (~(ZMII_SSR_SP << ZMII_SSR_V (unit->eu_UnitNum))), ZMII_SSR);

    /* set transmit enable & receive enable */
    outl (EMAC_M0_TXE | EMAC_M0_RXE, EMAC_M0 + unit->eu_IOBase);

    /* set receive fifo to 4k and tx fifo to 2k */
    mode_reg = inl (EMAC_M1 + unit->eu_IOBase);
    mode_reg |= EMAC_M1_RFS_4K | EMAC_M1_TX_FIFO_2K;

    /* set speed */
    if (speed == _1000BASET) {
            mode_reg = mode_reg | EMAC_M1_MF_1000MBPS | EMAC_M1_IST;
    } else if (speed == _100BASET)
            mode_reg = mode_reg | EMAC_M1_MF_100MBPS | EMAC_M1_IST;
    else
            mode_reg = mode_reg & ~0x00C00000;      /* 10 MBPS */
    if (duplex == FULL)
            mode_reg = mode_reg | 0x80000000 | EMAC_M1_IST;

    outl (mode_reg, EMAC_M1 + unit->eu_IOBase);

    /* Enable broadcast and indvidual address */
    /* TBS: enabling runts as some misbehaved nics will send runts */
    outl (EMAC_RMR_BAE | EMAC_RMR_IAE, EMAC_RXM + unit->eu_IOBase);

    /* we probably need to set the tx mode1 reg? maybe at tx time */

    /* set transmit request threshold register */
    outl (0x18000000, EMAC_TRTR + unit->eu_IOBase);  /* 256 byte threshold */

    /* set receive  low/high water mark register */
    /* 440s has a 64 byte burst length */
    outl (0x80009000, EMAC_RX_HI_LO_WMARK + unit->eu_IOBase);

    outl (0xf8640000, EMAC_TXM1 + unit->eu_IOBase);

    /* Set fifo limit entry in tx mode 0 */
    outl (0x00000003, EMAC_TXM0 + unit->eu_IOBase);
    /* Frame gap set */
    outl (0x00000008, EMAC_I_FRAME_GAP_REG + unit->eu_IOBase);

    /* Set EMAC IER */
    unit->eu_IER = EMAC_ISR_PTLE | EMAC_ISR_BFCS | EMAC_ISR_ORE | EMAC_ISR_IRE;
    if (speed == _100BASET)
            unit->eu_IER = unit->eu_IER | EMAC_ISR_SYE;

    outl (0xffffffff, EMAC_ISR + unit->eu_IOBase);   /* clear pending interrupts */
    outl (unit->eu_IER, EMAC_IER + unit->eu_IOBase);

    unit->eu_Flags |= IFF_UP;

    return 0;
}

static int EMAC_Stop(struct EMACUnit *unit)
{
    void *stack;
    uint32_t casr;

    D(bug("[EMAC%d] stop()\n", unit->eu_UnitNum));

    unit->eu_Flags &= ~IFF_UP;

    /*  Stop MAL channels */
    stack = SuperState();

    D(bug("[EMAC%d] Disable and reset MAL\n", unit->eu_UnitNum));
    wrdcr(MAL0_RXCARR, 0x80000000 >> unit->eu_UnitNum);
    wrdcr(MAL0_TXCARR, 0xc0000000 >> (2*unit->eu_UnitNum));

    casr = rddcr(MAL0_RXCASR);
    UserState(stack);

    /* Wait for Reset to complete */
    while(casr & (0x80000000 >> unit->eu_UnitNum))
    {
        unit->udelay(unit, 1000);

        stack = SuperState();
        casr = rddcr(MAL0_RXCASR);
        UserState(stack);
    }

    /* Reset the EMAC */
    outl(EMAC_M0_SRST, EMAC_M0 + unit->eu_IOBase);

    D(bug("[EMAC%d] stopped\n", unit->eu_UnitNum));

    return 1;
}

static void EMAC_SetMacAddress(struct EMACUnit *unit)
{
    uint32_t reg;

    D(bug("[EMAC%d] set_mac_address()\n", unit->eu_UnitNum));
    D(bug("[EMAC%d] New addr=%02x:%02x:%02x:%02x:%02x:%02x\n", unit->eu_UnitNum,
          unit->eu_DevAddr[0],unit->eu_DevAddr[1],
          unit->eu_DevAddr[2],unit->eu_DevAddr[3],
          unit->eu_DevAddr[4],unit->eu_DevAddr[5]));

    reg = 0;

    reg |= unit->eu_DevAddr[0];
    reg = reg << 8;
    reg |= unit->eu_DevAddr[1];

    outl (reg, EMAC_IAH + unit->eu_IOBase);

    reg = 0;

    reg |= unit->eu_DevAddr[2];
    reg = reg << 8;
    reg |= unit->eu_DevAddr[3];
    reg = reg << 8;
    reg |= unit->eu_DevAddr[4];
    reg = reg << 8;
    reg |= unit->eu_DevAddr[5];

    outl (reg, EMAC_IAL + unit->eu_IOBase);
}

/*
 * Report incoming events to all hyphotetical event receivers
 */
VOID ReportEvents(struct EMACBase *EMACBase, struct EMACUnit *unit, ULONG events)
{
    struct IOSana2Req *request, *tail, *next_request;
    struct List *list;

    list = &unit->eu_RequestPorts[EVENT_QUEUE]->mp_MsgList;
    next_request = (APTR)list->lh_Head;
    tail = (APTR)&list->lh_Tail;

    /* Go through list of event listeners. If send messages to receivers if event found */
    Disable();
    while(next_request != tail)
    {
        request = next_request;
        next_request = (APTR)request->ios2_Req.io_Message.mn_Node.ln_Succ;

        if((request->ios2_WireError&events) != 0)
        {
            request->ios2_WireError = events;
            Remove((APTR)request);
            ReplyMsg((APTR)request);
        }
    }
    Enable();

    return;
}

VOID CopyPacket(struct EMACBase *EMACBase, struct EMACUnit *unit,
                struct IOSana2Req *request, UWORD packet_size, UWORD packet_type,
                struct eth_frame *buffer)
{
    struct Opener *opener;
    BOOL filtered = FALSE;
    UBYTE *ptr;

    D(bug("[EMAC%d] CopyPacket(packet @ %x, len = %d)\n", unit->eu_UnitNum, buffer, packet_size));

    /* Set multicast and broadcast flags */

    request->ios2_Req.io_Flags &= ~(SANA2IOF_BCAST | SANA2IOF_MCAST);
    if((*((ULONG *)(buffer->eth_packet_dest)) == 0xffffffff) &&
            (*((UWORD *)(buffer->eth_packet_dest + 4)) == 0xffff))
    {
        request->ios2_Req.io_Flags |= SANA2IOF_BCAST;
        D(bug("[EMAC%d] CopyPacket: BROADCAST Flag set\n", unit->eu_UnitNum));
    }
    else if((buffer->eth_packet_dest[0] & 0x1) != 0)
    {
        request->ios2_Req.io_Flags |= SANA2IOF_MCAST;
        D(bug("[EMAC%d] CopyPacket: MULTICAST Flag set\n", unit->eu_UnitNum));
    }

    /* Set source and destination addresses and packet type */
    CopyMem(buffer->eth_packet_source, request->ios2_SrcAddr, ETH_ADDRESSSIZE);
    CopyMem(buffer->eth_packet_dest, request->ios2_DstAddr, ETH_ADDRESSSIZE);
    request->ios2_PacketType = packet_type;

    /* Adjust for cooked packet request */

    if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
    {
        packet_size -= ETH_PACKET_DATA;
        ptr = (UBYTE*)&buffer->eth_packet_data[0];
    }
    else
    {
        ptr = (UBYTE*)buffer;
    }

    request->ios2_DataLength = packet_size;
    D(bug("[EMAC%d] CopyPacket: packet @ %x (%d bytes)\n", unit->eu_UnitNum, ptr, packet_size));

    /* Filter packet */

    opener = request->ios2_BufferManagement;
    if((request->ios2_Req.io_Command == CMD_READ) &&
            (opener->filter_hook != NULL))
        if(!CallHookPkt(opener->filter_hook, request, ptr))
        {
            D(bug("[EMAC%d] CopyPacket: packet filtered\n", unit->eu_UnitNum));
            filtered = TRUE;
        }

    if(!filtered)
    {
        /* Copy packet into opener's buffer and reply packet */
        D(bug("[EMAC%d] CopyPacket: opener recieve packet .. ", unit->eu_UnitNum));
        if(!opener->rx_function(request->ios2_Data, ptr, packet_size))
        {
            D(bug("ERROR occured!!\n"));
            request->ios2_Req.io_Error = S2ERR_NO_RESOURCES;
            request->ios2_WireError = S2WERR_BUFF_ERROR;
            ReportEvents(EMACBase, unit, S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF | S2EVENT_RX);
        }
        else
        {
            D(bug("SUCCESS!!\n"));
        }
        Disable();
        Remove((APTR)request);
        Enable();
        ReplyMsg((APTR)request);
        D(bug("[EMAC%d] CopyPacket: opener notified.\n", unit->eu_UnitNum));
    }
}

BOOL AddressFilter(struct EMACBase *EMACBase, struct EMACUnit *unit, UBYTE *address)
{
    struct AddressRange *range, *tail;
    BOOL accept = TRUE;
    ULONG address_left;
    UWORD address_right;

    /* Check whether address is unicast/broadcast or multicast */

    address_left = AROS_BE2LONG(*((ULONG *)address));
    address_right = AROS_BE2WORD(*((UWORD *)(address + 4)));

    if((address_left & 0x01000000) != 0 &&
        !(address_left == 0xffffffff && address_right == 0xffff))
    {
        /* Check if this multicast address is wanted */

        range = (APTR)unit->eu_MulticastRanges.mlh_Head;
        tail = (APTR)&unit->eu_MulticastRanges.mlh_Tail;
        accept = FALSE;

        while((range != tail) && !accept)
        {
            if((address_left > range->lower_bound_left ||
                (address_left == range->lower_bound_left &&
                address_right >= range->lower_bound_right)) &&
                (address_left < range->upper_bound_left ||
                (address_left == range->upper_bound_left &&
                address_right <= range->upper_bound_right)))
                accept = TRUE;
            range = (APTR)range->node.mln_Succ;
        }
        if(!accept)
            unit->eu_SpecialStats[S2SS_ETHERNET_BADMULTICAST & 0xffff]++;
    }
    return accept;
}

void rx_int(struct EMACUnit *unit, struct ExecBase *SysBase)
{
    mal_descriptor_t descr;
    int i;
    int last_slot = unit->eu_LastRXSlot;
    struct TypeStats *tracker;
    ULONG packet_type;
    struct Opener *opener, *opener_tail;
    struct IOSana2Req *request, *request_tail;
    BOOL accepted, is_orphan;


    D(bug("[EMAC%d] RX Int\n", unit->eu_UnitNum));
    D(bug("[EMAC%d] Starting at packet %d", unit->eu_UnitNum, (unit->eu_LastRXSlot + 1) % RX_RING_SIZE));
    for (i=1; i <= RX_RING_SIZE; i++)
    {
        int packet_pos = ((i + last_slot) % RX_RING_SIZE) >> 2 ;
        int sub_pos = ((i + last_slot) % RX_RING_SIZE) % 4;
        mal_packet_t mal_packet;

        /* Invalidate each cache line - four mal descriptors at once - the very special case is
         * the first run of interrupt handler - it has to fetch the cache line unconditionally */
        if (!sub_pos  || i == 1)
        {
            /* Invalidate memory containing MAL descriptor */
            CacheClearE(&unit->eu_RXChannel[packet_pos], sizeof(mal_packet), CACRF_InvalidateD);
            mal_packet = unit->eu_RXChannel[packet_pos];
        }

        /* Work on local descriptor's copy */
        descr = mal_packet.descr[sub_pos];

        if (!(descr.md_ctrl & MAL_CTRL_RX_E))
        {
            struct eth_frame *frame;
            is_orphan = TRUE;

            unit->eu_LastRXSlot = (packet_pos << 2) + sub_pos;

            D(bug("[EMAC%d] MAL descriptor %d filled with %d bytes\n", unit->eu_UnitNum, (packet_pos << 2) + sub_pos, descr.md_length));

            /* Invalidate memory containing MAL descriptor */
            CacheClearE(descr.md_buffer, descr.md_length, CACRF_InvalidateD);
            frame = (struct eth_frame *)descr.md_buffer;

            /* Dump contents of frame if DEBUG enabled */
            D({
                int j;
                for (j=0; j<64; j++) {
                    if ((j%16) == 0)
                        D(bug("\n%03x:", j));
                    D(bug(" %02x", ((unsigned char*)frame)[j]));
                }
                D(bug("\n"));
            });

            /* Check for address validity */
            if(AddressFilter(unit->eu_EMACBase, unit, frame->eth_packet_dest))
            {
                /* Packet is addressed to this driver */
                packet_type = frame->eth_packet_type;
    D(bug("[EMAC%d] Packet IP accepted with type = %d\n", unit->eu_UnitNum, packet_type));

                opener = (APTR)unit->eu_Openers.mlh_Head;
                opener_tail = (APTR)&unit->eu_Openers.mlh_Tail;
                /* Offer packet to every opener */

                while(opener != opener_tail)
                {
                   request = (APTR)opener->read_port.mp_MsgList.lh_Head;
                   request_tail = (APTR)&opener->read_port.mp_MsgList.lh_Tail;
                   accepted = FALSE;

                   /* Offer packet to each request until it's accepted */
                   while((request != request_tail) && !accepted)
                   {
                      if((request->ios2_PacketType == packet_type)
                         || ((request->ios2_PacketType <= ETH_MTU)
                              && (packet_type <= ETH_MTU)))
                      {
    D(bug("[EMAC%d] copy packet for opener ..\n", unit->eu_UnitNum));
                         CopyPacket(unit->eu_EMACBase, unit, request, descr.md_length, packet_type, frame);
                         accepted = TRUE;
                      }
                      request =
                         (struct IOSana2Req *)request->ios2_Req.io_Message.mn_Node.ln_Succ;
                   }

                   if(accepted)
                      is_orphan = FALSE;

                   opener = (APTR)opener->node.mln_Succ;
                }

                /* If packet was unwanted, give it to S2_READORPHAN request */
                if(is_orphan)
                {
                    unit->eu_Stats.UnknownTypesReceived++;

                    if(!IsMsgPortEmpty(unit->eu_RequestPorts[ADOPT_QUEUE]))
                    {
                        CopyPacket(unit->eu_EMACBase, unit,
                            (APTR)unit->eu_RequestPorts[ADOPT_QUEUE]->
                            mp_MsgList.lh_Head, descr.md_length, packet_type, frame);
    D(bug("[EMAC%d] packet copied to orphan queue\n", unit->eu_UnitNum));
                    }
                }

                /* Update remaining statistics */

                tracker =
                    FindTypeStats(unit->eu_EMACBase, unit, &unit->eu_TypeTrackers, packet_type);
                if(tracker != NULL)
                {
                    tracker->stats.PacketsReceived++;
                    tracker->stats.BytesReceived += descr.md_length;
                }
            }


            /* Set the descriptor back as free */
            descr.md_ctrl |= MAL_CTRL_RX_E;
            descr.md_length = 0;

            /* Save local copy and flush data cache */
            /* Invalidate memory containing MAL descriptor */
            CacheClearE(&unit->eu_RXChannel[packet_pos], sizeof(mal_packet), CACRF_InvalidateD);
            mal_packet = unit->eu_RXChannel[packet_pos];

            mal_packet.descr[sub_pos] = descr;
            unit->eu_RXChannel[packet_pos] = mal_packet;
            CacheClearE(&unit->eu_RXChannel[packet_pos], sizeof(mal_packet), CACRF_ClearD);
        }
    }
}

static AROS_INTH1(EMAC_RX_Int, struct EMACUnit *, unit)
{
    AROS_INTFUNC_INIT

    D(bug("[EMAC%d] RX Int\n", unit->eu_UnitNum));
    rx_int(unit, SysBase);

    return 0;

    AROS_INTFUNC_EXIT
}

void tx_int(struct EMACUnit *unit, struct ExecBase *SysBase)
{
    struct EMACBase *EMACBase = unit->eu_EMACBase;
    mal_descriptor_t descr;
    int nr, try_count = 1;
    int last_slot = unit->eu_LastTXSlot;
    UWORD packet_size, data_size;
    struct IOSana2Req *request;
    struct Opener *opener;
    UBYTE *buffer;
    ULONG wire_error=0;
    BYTE error;
    struct MsgPort *port;
    struct TypeStats *tracker;
    int packet_pos;
    int sub_pos;
    BOOL proceed = TRUE; /* Success by default */

    port = unit->eu_RequestPorts[WRITE_QUEUE];

    D(bug("[EMAC%d] TX Int\n", unit->eu_UnitNum));
    D(bug("[EMAC%d] Starting at packet %d\d", unit->eu_UnitNum, (last_slot + 1) % TX_RING_SIZE));

//    for (nr=0; nr < TX_RING_SIZE; nr++)
//    {
//        CacheClearE(&unit->eu_TXChannel[nr>>2], sizeof(mal_packet_t), CACRF_InvalidateD);
//        D(bug("%04x ",unit->eu_TXChannel[nr>>2].descr[nr%4].md_ctrl));
//    }
    /* Still no error and there are packets to be sent? */
    while(proceed && (!IsMsgPortEmpty(port)))
    {
        mal_packet_t mal_packet;

        nr = (last_slot + 1) % TX_RING_SIZE;

        packet_pos = nr >> 2 ;
        sub_pos = nr % 4;

        error = 0;

        /* Invalidate memory containing MAL descriptor */
        CacheClearE(&unit->eu_TXChannel[packet_pos], sizeof(mal_packet), CACRF_InvalidateD);
        mal_packet = unit->eu_TXChannel[packet_pos];
        /* Work on local descriptor's copy */
        descr = mal_packet.descr[sub_pos];

        if (!(descr.md_ctrl & MAL_CTRL_TX_R))
        {
            struct eth_frame *eth = (struct eth_frame *)descr.md_buffer;
            request = (APTR)port->mp_MsgList.lh_Head;
            data_size = packet_size = request->ios2_DataLength;

            opener = (APTR)request->ios2_BufferManagement;

            if((request->ios2_Req.io_Flags & SANA2IOF_RAW) == 0)
            {
               packet_size += ETH_PACKET_DATA;
               CopyMem(request->ios2_DstAddr, eth->eth_packet_dest, ETH_ADDRESSSIZE);
               CopyMem(unit->eu_DevAddr, eth->eth_packet_source, ETH_ADDRESSSIZE);
               eth->eth_packet_type = request->ios2_PacketType;

               buffer = eth->eth_packet_data;
            }
            else
               buffer = descr.md_buffer;

            if (!opener->tx_function(buffer, request->ios2_Data, data_size))
            {
               error = S2ERR_NO_RESOURCES;
               wire_error = S2WERR_BUFF_ERROR;
               ReportEvents(EMACBase, unit,
                  S2EVENT_ERROR | S2EVENT_SOFTWARE | S2EVENT_BUFF
                  | S2EVENT_TX);
            }

            if (error == 0)
            {
                D(bug("[EMAC%d] packet %d:%d [type = %d] queued for transmission.", unit->eu_UnitNum, last_slot, nr, eth->eth_packet_type));

                /* Dump contents of frame if DEBUG enabled */
                D({
                    int j;
                    for (j=0; j<64; j++) {
                        if ((j%16) == 0)
                            D(bug("\n%03x:", j));
                        D(bug(" %02x", ((unsigned char*)eth)[j]));
                    }
                    D(bug("\n"));
                });

                /* Update the descriptor */
                descr.md_length = packet_size;
                descr.md_ctrl |= MAL_CTRL_TX_R | MAL_CTRL_TX_I | MAL_CTRL_TX_L | EMAC_CTRL_TX_GFCS | EMAC_CTRL_TX_GP;
                CacheClearE(descr.md_buffer, descr.md_length, CACRF_ClearD);

                CacheClearE(&unit->eu_TXChannel[packet_pos], sizeof(mal_packet), CACRF_InvalidateD);
                mal_packet = unit->eu_TXChannel[packet_pos];
                mal_packet.descr[sub_pos] = descr;
                unit->eu_TXChannel[packet_pos] = mal_packet;
                CacheClearE(&unit->eu_TXChannel[packet_pos], sizeof(mal_packet), CACRF_ClearD);
            }

            /* Reply packet */

            request->ios2_Req.io_Error = error;
            request->ios2_WireError = wire_error;
            Disable();
            Remove((APTR)request);
            Enable();
            ReplyMsg((APTR)request);

            /* Update statistics */

            if(error == 0)
            {
                tracker = FindTypeStats(EMACBase, unit, &unit->eu_TypeTrackers,
                    request->ios2_PacketType);
                if(tracker != NULL)
                {
                    tracker->stats.PacketsSent++;
                    tracker->stats.BytesSent += packet_size;
                }
            }
            try_count=0;
        }

        unit->eu_LastTXSlot = ++last_slot;
        try_count++;

        /*
         * If we've just run out of free space on the TX queue, stop
         * it and give up pushing further frames
         */
        if ( (try_count + 1) >= TX_RING_SIZE)
        {
            D(bug("[EMAC%d] output queue full!. Stopping [count = %d, TX_RING_SIZE = %d\n", unit->eu_UnitNum, try_count, TX_RING_SIZE));
            proceed = FALSE;
        }
    }

    /* Tell EMAC that it has new packets to process */
    outl(inl(EMAC_TXM0 + unit->eu_IOBase) | EMAC_TXM0_GNP0, EMAC_TXM0 + unit->eu_IOBase);

    /* Was there success? Enable incomming of new packets */
    if(proceed)
        unit->eu_RequestPorts[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;
    else
        unit->eu_RequestPorts[WRITE_QUEUE]->mp_Flags = PA_IGNORE;

}

static AROS_INTH1(EMAC_TX_Int, struct EMACUnit *, unit)
{
    AROS_INTFUNC_INIT

    D(bug("[EMAC%d] TX Int\n", unit->eu_UnitNum));
    tx_int(unit, SysBase);

    return 0;

    AROS_INTFUNC_EXIT
}

static AROS_INTH1(EMAC_TXEnd_Int, struct EMACUnit *, unit)
{
    AROS_INTFUNC_INIT

    D(bug("[EMAC%d] TX End Int\n", unit->eu_UnitNum));

    return FALSE;

    AROS_INTFUNC_EXIT
}

struct TypeStats *FindTypeStats(struct EMACBase *EMACBase, struct EMACUnit *unit,
                    struct MinList *list, ULONG packet_type)
{
    struct TypeStats *stats, *tail;
    BOOL found = FALSE;

    stats = (APTR)list->mlh_Head;
    tail = (APTR)&list->mlh_Tail;

    while(stats != tail && !found)
    {
        if(stats->packet_type == packet_type)
            found = TRUE;
        else
            stats = (APTR)stats->node.mln_Succ;
    }

    if(!found)
        stats = NULL;

    return stats;
}

static
AROS_UFH3(void, EMAC_UnitProcess,
      AROS_UFHA(char *,                 argPtr,         A0),
      AROS_UFHA(ULONG,                  argSize,        D0),
      AROS_UFHA(struct ExecBase *,      SysBase,        A6))
{
    AROS_USERFUNC_INIT

    struct MsgPort *iport;
    struct EMACUnit *unit = (struct EMACUnit *)(FindTask(NULL)->tc_UserData);
    struct Process *parent = unit->eu_Process;

    unit->eu_Process = (struct Process *)FindTask(NULL);

    D(bug("[EMAC%d] Hello there.\n", unit->eu_UnitNum));
    D(bug("[EMAC%d] Process @ %p\n", unit->eu_UnitNum, unit->eu_Process));

    unit->eu_Flags = 0;
    unit->eu_OpenCount = 0;
    unit->eu_RangeCount = 0;

    iport = CreateMsgPort();

    unit->eu_InputPort = iport;

    unit->eu_TimerPort.mp_SigBit = SIGB_SINGLE;
    unit->eu_TimerPort.mp_Flags = PA_SIGNAL;
    unit->eu_TimerPort.mp_SigTask = FindTask(NULL);
    unit->eu_TimerPort.mp_Node.ln_Type = NT_MSGPORT;
    NEWLIST(&unit->eu_TimerPort.mp_MsgList);

    unit->eu_TimerRequest.tr_node.io_Message.mn_ReplyPort = &unit->eu_TimerPort;
    unit->eu_TimerRequest.tr_node.io_Message.mn_Length = sizeof(unit->eu_TimerRequest);

    OpenDevice((STRPTR)"timer.device", UNIT_MICROHZ, (struct IORequest *)&unit->eu_TimerRequest, 0);

    EMAC_Startup(unit);

    Signal((struct Task *)parent, SIGF_SINGLE);

    do {
        uint32_t sigset = 1 << iport->mp_SigBit |
                          SIGBREAKF_CTRL_C;

        uint32_t rcvd = Wait(sigset);

        if (rcvd & SIGBREAKF_CTRL_C)
        {
            D(bug("[EMAC%d] CTRL_C signal\n", unit->eu_UnitNum));
        }
        else if (rcvd & (1 << iport->mp_SigBit))
        {
            struct IOSana2Req *io;

            /* Handle incoming transactions */
            while ((io = (struct IOSana2Req *)GetMsg(iport))!= NULL);
            {
                D(bug("[EMAC%d] Handle incomming transaction.\n", unit->eu_UnitNum));
                ObtainSemaphore(&unit->eu_Lock);
                handle_request(unit->eu_EMACBase, io);
            }
        }

    } while(1);

    AROS_USERFUNC_EXIT
}

static const struct UnitInfo {
    uint8_t     ui_IrqNum;
    char       *ui_TaskName;
    intptr_t    ui_IOBase;
    uint8_t     ui_PHYAddr;
} EMAC_Units_sam440[2] = {
        { INTR_ETH0, EMAC_TASK1_NAME, EMAC0_BASE, 24 },
        { INTR_ETH1, EMAC_TASK2_NAME, EMAC1_BASE, 25 },
}, EMAC_Units_sam460[2] = {
        { INTR_UIC2_BASE + INTR_UIC2_EMAC0, EMAC_TASK1_NAME, EMAC0_BASE, 24 },
        { INTR_UIC2_BASE + INTR_UIC2_EMAC0, EMAC_TASK2_NAME, EMAC1_BASE, 25 },
};

static inline ULONG GetPVR(void)
{
    struct Library *ProcessorBase = OpenResource(PROCESSORNAME);
    ULONG pvr = 0;

    if (ProcessorBase) {
        struct TagItem tags[] = {
            { GCIT_Model, (IPTR)&pvr },
            { TAG_END }
        };
        GetCPUInfo(tags);
    }

    return pvr;
}

struct EMACUnit *CreateUnit(struct EMACBase *EMACBase, uint8_t num)
{
    void *KernelBase = OpenResource("kernel.resource");

    D(bug("[EMAC ] CreateUnit(%d)\n", num));

    struct EMACUnit *unit = AllocPooled(EMACBase->emb_Pool, sizeof(struct EMACUnit));

    if (unit)
    {
        int i;
        const struct UnitInfo *EMAC_Units;

        if (GetPVR() == PVR_PPC460EX_B) {
            EMAC_Units = &EMAC_Units_sam460[0];
        } else {
            EMAC_Units = &EMAC_Units_sam440[0];
        }

        InitSemaphore(&unit->eu_Lock);

        NEWLIST(&unit->eu_Openers);
        NEWLIST(&unit->eu_MulticastRanges);
        NEWLIST(&unit->eu_TypeTrackers);

        unit->eu_UnitNum = num;
        unit->eu_IRQHandler = KrnAddIRQHandler(EMAC_Units[num].ui_IrqNum, EMACIRQHandler, EMACBase, unit);
        unit->eu_EMACBase = EMACBase;
        unit->eu_IOBase = EMAC_Units[num].ui_IOBase;
        unit->eu_PHYAddr = EMAC_Units[num].ui_PHYAddr;
        unit->eu_MTU = ETH_MTU;

        unit->eu_LastTXSlot = TX_RING_SIZE - 1;
        unit->eu_LastRXSlot = RX_RING_SIZE - 1;

        unit->start = EMAC_Start;
        unit->stop = EMAC_Stop;
        unit->set_mac_address = EMAC_SetMacAddress;
        unit->udelay = EMAC_UDelay;

        unit->eu_RXInt.is_Node.ln_Name = "EMAC RX Int";
        unit->eu_RXInt.is_Code = (VOID_FUNC)EMAC_RX_Int;
        unit->eu_RXInt.is_Data = unit;

        unit->eu_TXInt.is_Node.ln_Name = "EMAC TX Int";
        unit->eu_TXInt.is_Code = (VOID_FUNC)EMAC_TX_Int;
        unit->eu_TXInt.is_Data = unit;

        unit->eu_TXEndInt.is_Node.ln_Name = "EMAC TX Int";
        unit->eu_TXEndInt.is_Code = (VOID_FUNC)EMAC_TXEnd_Int;
        unit->eu_TXEndInt.is_Data = unit;

        unit->eu_RXChannel = (mal_packet_t *)EMACBase->emb_MALRXChannels[num];
        unit->eu_TXChannel = (mal_packet_t *)EMACBase->emb_MALTXChannels[num];
        //unit->eu_TXChannel[1] = (mal_packet_t *)EMACBase->emb_MALTXChannels[2*num+1];

        for (i=0; i < REQUEST_QUEUE_COUNT; i++)
        {
            struct MsgPort *port = AllocPooled(EMACBase->emb_Pool, sizeof(struct MsgPort));
            unit->eu_RequestPorts[i] = port;

            if (port)
            {
                NEWLIST(&port->mp_MsgList);
                port->mp_Flags = PA_IGNORE;
                port->mp_SigTask = &unit->eu_TXInt;
            }
        }

        unit->eu_RequestPorts[WRITE_QUEUE]->mp_Flags = PA_SOFTINT;

        /* Create the unit's process */

        /* Unit's process pointer will temporarly contain the parent */
        unit->eu_Process = (struct Process *)FindTask(NULL);
        CreateNewProcTags(
                         NP_Entry, (IPTR)EMAC_UnitProcess,
                         NP_Name, EMAC_Units[num].ui_TaskName,
                         NP_Priority, 0,
                         NP_UserData, (IPTR)unit,
                         NP_StackSize, 40960,
                         TAG_DONE);

        /* Wait for synchronisation signal */
        Wait(SIGF_SINGLE);

        D(bug("[EMAC ] Unit %d up and running\n", num));
    }

    return unit;
}

static struct AddressRange *FindMulticastRange(struct EMACBase *EMACBase, struct EMACUnit *unit,
   ULONG lower_bound_left, UWORD lower_bound_right, ULONG upper_bound_left, UWORD upper_bound_right)
{
    struct AddressRange *range, *tail;
    BOOL found = FALSE;

    range = (APTR)unit->eu_MulticastRanges.mlh_Head;
    tail = (APTR)&unit->eu_MulticastRanges.mlh_Tail;

    while((range != tail) && !found)
    {
        if((lower_bound_left == range->lower_bound_left) &&
            (lower_bound_right == range->lower_bound_right) &&
            (upper_bound_left == range->upper_bound_left) &&
            (upper_bound_right == range->upper_bound_right))
            found = TRUE;
        else
            range = (APTR)range->node.mln_Succ;
    }

    if(!found)
        range = NULL;

    return range;
}

BOOL AddMulticastRange(struct EMACBase *EMACBase, struct EMACUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(EMACBase, unit, lower_bound_left, lower_bound_right,
        upper_bound_left, upper_bound_right);

    if(range != NULL)
        range->add_count++;
    else
    {
        range = AllocPooled(EMACBase->emb_Pool, sizeof(struct AddressRange));
        if(range != NULL)
        {
            range->lower_bound_left = lower_bound_left;
            range->lower_bound_right = lower_bound_right;
            range->upper_bound_left = upper_bound_left;
            range->upper_bound_right = upper_bound_right;
            range->add_count = 1;

            Disable();
            AddTail((APTR)&unit->eu_MulticastRanges, (APTR)range);
            Enable();

            if (unit->eu_RangeCount++ == 0)
            {
                unit->eu_Flags |= IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }

    return range != NULL;
}

BOOL RemMulticastRange(struct EMACBase *EMACBase, struct EMACUnit *unit, const UBYTE *lower_bound, const UBYTE *upper_bound)
{
    struct AddressRange *range;
    ULONG lower_bound_left, upper_bound_left;
    UWORD lower_bound_right, upper_bound_right;

    lower_bound_left = AROS_BE2LONG(*((ULONG *)lower_bound));
    lower_bound_right = AROS_BE2WORD(*((UWORD *)(lower_bound + 4)));
    upper_bound_left = AROS_BE2LONG(*((ULONG *)upper_bound));
    upper_bound_right = AROS_BE2WORD(*((UWORD *)(upper_bound + 4)));

    range = FindMulticastRange(EMACBase, unit, lower_bound_left, lower_bound_right,
        upper_bound_left, upper_bound_right);

    if(range != NULL)
    {
        if(--range->add_count == 0)
        {
            Disable();
            Remove((APTR)range);
            Enable();
            FreePooled(EMACBase->emb_Pool, range, sizeof(struct AddressRange));

            if (--unit->eu_RangeCount == 0)
            {
                unit->eu_Flags &= ~IFF_ALLMULTI;
                unit->set_multicast(unit);
            }
        }
    }
    return range != NULL;
}
