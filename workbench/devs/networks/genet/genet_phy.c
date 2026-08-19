/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet — MDIO/PHY access
    BCM54213PE PHY connected via internal MDIO at address 1.
*/

#include <exec/types.h>
#include <aros/macros.h>

#include "genet.h"

static inline ULONG genet_rd(struct GENETUnit *unit, ULONG off)
{
    return AROS_LE2LONG(*(volatile ULONG *)(unit->gn_RegBase + off));
}

static inline void genet_wr(struct GENETUnit *unit, ULONG off, ULONG val)
{
    *(volatile ULONG *)(unit->gn_RegBase + off) = AROS_LONG2LE(val);
}

static BOOL mdio_wait(struct GENETUnit *unit)
{
    int tries = 1000;
    while ((genet_rd(unit, MDIO_CMD) & MDIO_START_BUSY) && --tries)
        ;
    return tries > 0;
}

UWORD genet_MDIO_Read(struct GENETUnit *unit, ULONG phy, ULONG reg)
{
    ULONG val;

    val = MDIO_RD | (phy << MDIO_PMD_SHIFT) | (reg << MDIO_REG_SHIFT);
    genet_wr(unit, MDIO_CMD, val);
    genet_wr(unit, MDIO_CMD, val | MDIO_START_BUSY);

    if (!mdio_wait(unit))
        return 0xFFFF;

    val = genet_rd(unit, MDIO_CMD);
    if (val & MDIO_READ_FAIL)
        return 0xFFFF;

    return (UWORD)(val & 0xFFFF);
}

void genet_MDIO_Write(struct GENETUnit *unit, ULONG phy, ULONG reg, UWORD value)
{
    ULONG val;

    val = MDIO_WR | (phy << MDIO_PMD_SHIFT) | (reg << MDIO_REG_SHIFT) | value;
    genet_wr(unit, MDIO_CMD, val);
    genet_wr(unit, MDIO_CMD, val | MDIO_START_BUSY);

    mdio_wait(unit);
}

/*
 * Basic PHY initialization: reset, auto-negotiate, report link.
 * MII register definitions (IEEE 802.3).
 */
#define MII_BMCR        0x00
#define MII_BMSR        0x01
#define MII_PHYSID1     0x02
#define MII_PHYSID2     0x03
#define MII_ANAR       0x04
#define MII_ANLPAR     0x05
#define MII_GBCR       0x09
#define MII_GBSR       0x0A

#define BMCR_RESET      0x8000
#define BMCR_ANENABLE   0x1000
#define BMCR_ANRESTART  0x0200

#define BMSR_LSTATUS    0x0004
#define BMSR_ANEGCOMPLETE 0x0020

#define ANAR_100FULL    0x0100
#define ANAR_100HALF    0x0080
#define ANAR_10FULL     0x0040
#define ANAR_10HALF     0x0020
#define ANAR_CSMA       0x0001

#define GBCR_1000FULL   0x0200
#define GBCR_1000HALF   0x0100

#define GBSR_LP1000FULL 0x0800
#define GBSR_LP1000HALF 0x0400

BOOL genet_PHY_Init(struct GENETUnit *unit)
{
    UWORD bmsr, anar, gbcr, anlpar, gbsr;
    int tries;

    /* Reset PHY */
    genet_MDIO_Write(unit, unit->gn_PhyAddr, MII_BMCR, BMCR_RESET);

    tries = 1000;
    while ((genet_MDIO_Read(unit, unit->gn_PhyAddr, MII_BMCR) & BMCR_RESET) && --tries)
        ;
    if (!tries)
        return FALSE;

    /* Advertise all speeds */
    anar = ANAR_CSMA | ANAR_10HALF | ANAR_10FULL | ANAR_100HALF | ANAR_100FULL;
    genet_MDIO_Write(unit, unit->gn_PhyAddr, MII_ANAR, anar);

    gbcr = GBCR_1000FULL | GBCR_1000HALF;
    genet_MDIO_Write(unit, unit->gn_PhyAddr, MII_GBCR, gbcr);

    /* Start auto-negotiation */
    genet_MDIO_Write(unit, unit->gn_PhyAddr, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);

    /* Wait for link (up to 5 seconds) */
    tries = 5000;
    do {
        bmsr = genet_MDIO_Read(unit, unit->gn_PhyAddr, MII_BMSR);
        if (bmsr & BMSR_LSTATUS)
            break;
        /* ~1ms delay */
        genet_udelay(1000);
    } while (--tries);

    if (!(bmsr & BMSR_LSTATUS)) {
        unit->gn_LinkUp = FALSE;
        unit->gn_LinkSpeed = 0;
        return FALSE;
    }

    unit->gn_LinkUp = TRUE;

    /* Determine negotiated speed */
    gbsr = genet_MDIO_Read(unit, unit->gn_PhyAddr, MII_GBSR);
    anlpar = genet_MDIO_Read(unit, unit->gn_PhyAddr, MII_ANLPAR);

    if (gbsr & GBSR_LP1000FULL)
        unit->gn_LinkSpeed = 1000;
    else if (gbsr & GBSR_LP1000HALF)
        unit->gn_LinkSpeed = 1000;
    else if (anlpar & ANAR_100FULL)
        unit->gn_LinkSpeed = 100;
    else if (anlpar & ANAR_100HALF)
        unit->gn_LinkSpeed = 100;
    else
        unit->gn_LinkSpeed = 10;

    /* Update MAC speed register */
    {
        ULONG speed;
        switch (unit->gn_LinkSpeed) {
        case 1000: speed = UMAC_SPEED_1000; break;
        case 100:  speed = UMAC_SPEED_100; break;
        default:   speed = UMAC_SPEED_10; break;
        }
        genet_wr(unit, UMAC_CMD,
                 (genet_rd(unit, UMAC_CMD) & ~(CMD_SPEED_MASK << CMD_SPEED_SHIFT)) |
                 (speed << CMD_SPEED_SHIFT));
    }

    return TRUE;
}
