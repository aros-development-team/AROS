#define DEBUG 1
#include <inttypes.h>
#include <aros/debug.h>
#include <asm/amcc440.h>
#include <asm/io.h>
#include "emac.h"

void EMACIRQHandler(struct EMACBase *EMACBase, struct EMACUnit *unit)
{
    if (unit)
    {
        D(bug("[EMAC%d] IRQ Handler\n", unit->eu_UnitNum));
    }
}

void EMAC_Startup(struct EMACUnit *unit)
{
    uint32_t tmp;

    D(bug("[EMAC%d] Startup\n", unit->eu_UnitNum));

    tmp = inl((uint32_t*)(unit->eu_IOBase + EMAC_IAH));

    unit->eu_DevAddr[0] = unit->eu_OrgAddr[0] = (tmp >> 8) & 0xff;
    unit->eu_DevAddr[1] = unit->eu_OrgAddr[1] = (tmp >> 0) & 0xff;

    tmp = inl((uint32_t*)(unit->eu_IOBase + EMAC_IAL));

    unit->eu_DevAddr[2] = unit->eu_OrgAddr[2] = (tmp >> 24) & 0xff;
    unit->eu_DevAddr[3] = unit->eu_OrgAddr[3] = (tmp >> 16) & 0xff;
    unit->eu_DevAddr[4] = unit->eu_OrgAddr[4] = (tmp >> 8) & 0xff;
    unit->eu_DevAddr[5] = unit->eu_OrgAddr[5] = (tmp >> 0) & 0xff;
    D(bug("[EMAC%d] HW addr=%02x:%02x:%02x:%02x:%02x:%02x\n", unit->eu_UnitNum,
              unit->eu_DevAddr[0],unit->eu_DevAddr[1],
              unit->eu_DevAddr[2],unit->eu_DevAddr[3],
              unit->eu_DevAddr[4],unit->eu_DevAddr[5]));
}

int EMAC_miiphy_read(struct EMACUnit *unit, uint8_t reg, uint16_t *value)
{
    unsigned long sta_reg;  /* STA scratch area */
    unsigned long i;

    /* see if it is ready for 1000 nsec */
    i = 0;

    /* see if it is ready for  sec */
    while ((inl (EMAC_STACR + unit->eu_IOBase) & EMAC_STACR_OC) == EMAC_STACR_OC_MASK) {
            unit->udelay (unit, 7);
            if (i > 5) {
D({
                    sta_reg = inl (EMAC_STACR + unit->eu_IOBase);
                    bug ("[EMAC%d] read : EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg);  /* test-only */
                    bug ("[EMAC%d] read err 1\n", unit->eu_UnitNum);
})
                    return -1;
            }
            i++;
    }
    sta_reg = reg;          /* reg address */

    sta_reg = (sta_reg | EMAC_STACR_READ) & ~EMAC_STACR_CLK_100MHZ;

    sta_reg = sta_reg | (unit->eu_PHYAddr << 5);        /* Phy address */
    sta_reg = sta_reg | EMAC_STACR_OC_MASK; /* new IBM emac v4 */
    outl (sta_reg, EMAC_STACR + unit->eu_IOBase);
    D(bug("[EMAC%d] a2: write: EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg));

    sta_reg = inl (EMAC_STACR + unit->eu_IOBase);
    D(bug("[EMAC%d] a21: read: EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg));
    i = 0;
    while ((sta_reg & EMAC_STACR_OC) == EMAC_STACR_OC_MASK) {
            unit->udelay (unit, 7);
            if (i > 5) {
                    return -1;
            }
            i++;
            sta_reg = inl (EMAC_STACR + unit->eu_IOBase);
            D(bug("[EMAC%d] a22: read: EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg));
    }
    if ((sta_reg & EMAC_STACR_PHYE) != 0) {
            return -1;
    }

//    *value = *(uint16_t *) (&sta_reg);
    *value = sta_reg >> 16;
    return 0;
}

int EMAC_miiphy_write(struct EMACUnit *unit, uint8_t reg, uint16_t value)
{
    unsigned long sta_reg;  /* STA scratch area */
    unsigned long i;

    /* see if it is ready for 1000 nsec */
     i = 0;

     while ((inl (EMAC_STACR + unit->eu_IOBase) & EMAC_STACR_OC) == EMAC_STACR_OC_MASK) {
             if (i > 5)
                     return -1;
             unit->udelay (unit, 7);
             i++;
     }
     sta_reg = 0;
     sta_reg = reg;          /* reg address */

     sta_reg = (sta_reg | EMAC_STACR_WRITE) & ~EMAC_STACR_CLK_100MHZ;

     sta_reg = sta_reg | ((unsigned long) unit->eu_PHYAddr << 5);/* Phy address */
     sta_reg = sta_reg | EMAC_STACR_OC_MASK;         /* new IBM emac v4 */
     sta_reg = ((uint32_t)(value)) << 16 | sta_reg;
//     memcpy (&sta_reg, &value, 2);   /* put in data */

     outl (sta_reg, EMAC_STACR + unit->eu_IOBase);

     /* wait for completion */
     i = 0;
     sta_reg = inl (EMAC_STACR + unit->eu_IOBase);
     D(bug("[EMAC%d] a31: read : EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg));

     while ((sta_reg & EMAC_STACR_OC) == EMAC_STACR_OC_MASK) {
             unit->udelay (unit, 7);
             if (i > 5)
                     return -1;
             i++;
             sta_reg = inl (EMAC_STACR + unit->eu_IOBase);
             D(bug("[EMAC%d] a32: read : EMAC_STACR=0x%0x\n", unit->eu_UnitNum, sta_reg));
     }

     if ((sta_reg & EMAC_STACR_PHYE) != 0)
             return -1;
     return 0;
}

int EMAC_miiphy_reset(struct EMACUnit *unit)
{
    uint16_t reg;
    uint32_t loop_cnt;

    if (EMAC_miiphy_read (unit, PHY_BMCR, &reg) != 0) {
        D(bug("[EMAC%d] PHY status read failed\n", unit->eu_UnitNum));
        return -1;
    }
    if (EMAC_miiphy_write (unit, PHY_BMCR, reg | 0x8000) != 0) {
        D(bug("[EMAC%d] PHY reset failed\n", unit->eu_UnitNum));
        return (-1);
    }

    unit->udelay(unit, 1000);

    /*
     * Poll the control register for the reset bit to go to 0 (it is
     * auto-clearing).  This should happen within 0.5 seconds per the
     * IEEE spec.
     */
    loop_cnt = 0;
    reg = 0x8000;
    while (((reg & 0x8000) != 0) && (loop_cnt++ < 1000000)) {
        if (EMAC_miiphy_read (unit, PHY_BMCR, &reg) != 0) {
            D(bug("[EMAC%d] PHY status read failed\n", unit->eu_UnitNum));
            return (-1);
        }
    }
    if ((reg & 0x8000) == 0) {
        return (0);
    } else {
        D(bug("[EMAC%d] PHY reset timed out\n", unit->eu_UnitNum));
        return (-1);
    }
    return (0);
}

int EMAC_miiphy_speed(struct EMACUnit *unit)
{
    uint16_t bmcr, anlpar;

    /* Check Basic Management Control Register first. */
    if (EMAC_miiphy_read (unit, PHY_BMCR, &bmcr)) {
            D(bug("[EMAC%d] PHY speed", unit->eu_UnitNum));
            goto miiphy_read_failed;
    }
    /* Check if auto-negotiation is on. */
    if (bmcr & PHY_BMCR_AUTON) {
            /* Get auto-negotiation results. */
            if (EMAC_miiphy_read (unit, PHY_ANLPAR, &anlpar)) {
                    D(bug("[EMAC%d] PHY AN speed", unit->eu_UnitNum));
                    goto miiphy_read_failed;
            }
            return (anlpar & PHY_ANLPAR_100) ? _100BASET : _10BASET;
    }
    /* Get speed from basic control settings. */
    return (bmcr & PHY_BMCR_100MB) ? _100BASET : _10BASET;

  miiphy_read_failed:
    D(bug(" read failed, assuming 10BASE-T\n"));
    return _10BASET;
}

int EMAC_miiphy_duplex(struct EMACUnit *unit)
{
    uint16_t bmcr, anlpar;

    /* Check Basic Management Control Register first. */
    if (EMAC_miiphy_read(unit, PHY_BMCR, &bmcr)) {
            D(bug("[EMAC%d] PHY duplex", unit->eu_UnitNum));
            goto miiphy_read_failed;
    }
    /* Check if auto-negotiation is on. */
    if (bmcr & PHY_BMCR_AUTON) {
            /* Get auto-negotiation results. */
            if (EMAC_miiphy_read(unit, PHY_ANLPAR, &anlpar)) {
                    D(bug("[EMAC%d] PHY AN duplex", unit->eu_UnitNum));
                    goto miiphy_read_failed;
            }
            return (anlpar & (PHY_ANLPAR_10FD | PHY_ANLPAR_TXFD)) ?
                FULL : HALF;
    }
    /* Get speed from basic control settings. */
    return (bmcr & PHY_BMCR_DPLX) ? FULL : HALF;

  miiphy_read_failed:
    D(bug(" read failed, assuming half duplex\n"));
    return HALF;
}

int EMAC_miiphy_link(struct EMACUnit *unit)
{
    uint16_t reg;

    EMAC_miiphy_read(unit, PHY_BMSR, &reg);
    if (EMAC_miiphy_read(unit, PHY_BMSR, &reg))
    {
        D(bug("[EMAC%d] PHY_BMSR read failed\n", unit->eu_UnitNum));
        return 0;
    }

    if (reg & PHY_BMSR_LS)
        return 1;
    else
        return 0;
}

int EMAC_phy_setup_aneg (struct EMACUnit *unit)
{
        unsigned short ctl, adv;

        /* Setup standard advertise */
        EMAC_miiphy_read (unit, PHY_ANAR, &adv);
        adv |= (PHY_ANLPAR_ACK | PHY_ANLPAR_RF | PHY_ANLPAR_T4 |
                PHY_ANLPAR_TXFD | PHY_ANLPAR_TX | PHY_ANLPAR_10FD |
                PHY_ANLPAR_10);
        EMAC_miiphy_write (unit, PHY_ANAR, adv);

        EMAC_miiphy_read (unit, PHY_1000BTCR, &adv);
        adv |= (0x0300);
        EMAC_miiphy_write (unit, PHY_1000BTCR, adv);

        /* Start/Restart aneg */
        EMAC_miiphy_read (unit, PHY_BMCR, &ctl);
        ctl |= (PHY_BMCR_AUTON | PHY_BMCR_RST_NEG);
        EMAC_miiphy_write (unit, PHY_BMCR, ctl);

        return 0;
}
