#define DEBUG 1

#include <asm/amcc440.h>
#include <inttypes.h>

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/lists.h>

#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <oop/oop.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "emac.h"
#include "mal.h"
#include LC_LIBDEFS_FILE

void rx_int(struct EMACUnit *unit, struct ExecBase *SysBase);

static void MALMRE(struct EMACBase *EMACBase, struct EMACUnit *unit)
{
    //Cause(&unit->eu_RXInt);
    rx_int(unit, SysBase);
}

static void MALIRQHandler(struct EMACBase *EMACBase, uint8_t inttype)
{
    uint32_t temp;

    switch (inttype)
    {
        case INTR_MTE:
            /* get the information about channel causing the interrupt */
            temp = rddcr(MAL0_TXEOBISR);
            /* Clear the interrupt flag */
            wrdcr(MAL0_TXEOBISR, temp);
            break;

        case INTR_MRE:
            /* get the information about channel causing the interrupt */
            temp = rddcr(MAL0_RXEOBISR);
            /* Clear the interrupt flag */
            wrdcr(MAL0_RXEOBISR, temp);
            if (temp & 0x80000000)
                MALMRE(EMACBase, EMACBase->emb_Units[0]);
            if (temp & 0x40000000)
                MALMRE(EMACBase, EMACBase->emb_Units[0]);
            break;

        case INTR_MTDE:
            D(bug("[EMAC ] MAL TXDE\n"));
            break;

        case INTR_MRDE:
            D(bug("[EMAC ] MAL RXDE\n"));
            break;

        case INTR_MS:
            D(bug("[EMAC ] MAL SERR\n"));
            break;
    }
}


void EMAC_MAL_Init(struct EMACBase *EMACBase)
{
    int i;
    void *stack;
    void *KernelBase;

    D(bug("[EMAC ] Memory Access Layer (MAL) Init\n"));

    /*
     * Add all interrupt handlers required. Oh boy, MAL serves us FIVE different
     * interrupt vectors!
     */

    KernelBase = OpenResource("kernel.resource");

    EMACBase->emb_MALHandlers[0] = KrnAddIRQHandler(INTR_MTE,   MALIRQHandler, EMACBase, (APTR)INTR_MTE);
    EMACBase->emb_MALHandlers[1] = KrnAddIRQHandler(INTR_MRE,   MALIRQHandler, EMACBase, (APTR)INTR_MRE);
    EMACBase->emb_MALHandlers[2] = KrnAddIRQHandler(INTR_MTDE,  MALIRQHandler, EMACBase, (APTR)INTR_MTDE);
    EMACBase->emb_MALHandlers[3] = KrnAddIRQHandler(INTR_MRDE,  MALIRQHandler, EMACBase, (APTR)INTR_MRDE);
    EMACBase->emb_MALHandlers[4] = KrnAddIRQHandler(INTR_MS,    MALIRQHandler, EMACBase, (APTR)INTR_MS);

    intptr_t buffers = (intptr_t)AllocPooled(EMACBase->emb_Pool,
                                            32 + 4 * (RX_RING_SIZE + TX_RING_SIZE) * ((RXTX_ALLOC_BUFSIZE+31)& ~31));

    D(bug("[EMAC ] Allocating %d bytes @ %p for buffers\n",
              32 + 4 * (RX_RING_SIZE + TX_RING_SIZE) * ((RXTX_ALLOC_BUFSIZE+31)& ~31), buffers));

    buffers = (buffers + 31) & ~31;

    /* Allocate memory for 2 RX channels */
    for (i=0; i < 2; i++)
    {
        int j;

        EMACBase->emb_MALRXChannels[i] = (void*)((intptr_t)AllocVecPooled(EMACBase->emb_Pool, 32 + RX_RING_SIZE * sizeof(mal_descriptor_t)) & ~31);

        for (j = 0; j < RX_RING_SIZE; j++)
        {
            EMACBase->emb_MALRXChannels[i][j].md_buffer = (char*)buffers;
            EMACBase->emb_MALRXChannels[i][j].md_length = RXTX_ALLOC_BUFSIZE;
            EMACBase->emb_MALRXChannels[i][j].md_ctrl = MAL_CTRL_RX_E | MAL_CTRL_RX_I;

            buffers += (RXTX_ALLOC_BUFSIZE+31) & ~31;
        }

        EMACBase->emb_MALRXChannels[i][RX_RING_SIZE-1].md_ctrl |= MAL_CTRL_RX_W;

        CacheClearE(EMACBase->emb_MALRXChannels[i], RX_RING_SIZE * sizeof(mal_descriptor_t), CACRF_ClearD);

        D(bug("[EMAC ] MAL RX Channel %d @ %p\n", i, EMACBase->emb_MALRXChannels[i]));
    }

    /* Allocate memory for 2 TX channels */
    for (i=0; i < 2; i++)
    {
        int j;

        EMACBase->emb_MALTXChannels[i] = (void*)((intptr_t)AllocVecPooled(EMACBase->emb_Pool, 32 + TX_RING_SIZE * sizeof(mal_descriptor_t)) & ~31);

        for (j = 0; j < TX_RING_SIZE; j++)
        {
            EMACBase->emb_MALTXChannels[i][j].md_buffer = (char*)buffers;
            EMACBase->emb_MALTXChannels[i][j].md_length = RXTX_ALLOC_BUFSIZE;
            EMACBase->emb_MALTXChannels[i][j].md_ctrl = 0;

            buffers += (RXTX_ALLOC_BUFSIZE+31) & ~31;
        }

        EMACBase->emb_MALTXChannels[i][TX_RING_SIZE - 1].md_ctrl |= MAL_CTRL_TX_W;

        CacheClearE(EMACBase->emb_MALTXChannels[i], RX_RING_SIZE * sizeof(mal_descriptor_t), CACRF_ClearD);

        D(bug("[EMAC ] MAL TX Channel %d @ %p\n", i, EMACBase->emb_MALTXChannels[i]));
    }


    /* Writing to DCR registers requires supervisor rights. */
    stack = SuperState();

    /* Set the MAL dcr's containing pointers to the transfer descriptors */
//    wrdcr(MAL0_TXCTP0R, (intptr_t)EMACBase->emb_MALTXChannels[0]);
//    wrdcr(MAL0_TXCTP1R, (intptr_t)EMACBase->emb_MALTXChannels[1]);
//    wrdcr(MAL0_TXCTP2R, (intptr_t)EMACBase->emb_MALTXChannels[2]);
//    wrdcr(MAL0_TXCTP3R, (intptr_t)EMACBase->emb_MALTXChannels[3]);
    wrdcr(MAL0_TXCTP0R, (intptr_t)EMACBase->emb_MALTXChannels[0]);
    wrdcr(MAL0_TXCTP2R, (intptr_t)EMACBase->emb_MALTXChannels[1]);
    wrdcr(MAL0_RXCTP0R, (intptr_t)EMACBase->emb_MALRXChannels[0]);
    wrdcr(MAL0_RXCTP1R, (intptr_t)EMACBase->emb_MALRXChannels[1]);

    /* Length of receive buffers */
    wrdcr(MAL0_RCBS0, (RXTX_ALLOC_BUFSIZE + 15) >> 4);
    wrdcr(MAL0_RCBS1, (RXTX_ALLOC_BUFSIZE + 15) >> 4);

    /* Enable MAL */
    wrdcr(MAL0_CFG, MAL_CR_PLBB | MAL_CR_OPBBL | MAL_CR_LEA | MAL_CR_PLBLT_DEFAULT);

    UserState(stack);

}
