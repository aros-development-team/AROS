/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 register, mdio and PHY access.

    Port target: OpenBSD sys/dev/ic/bcmgenet.c (3-clause BSD). Function
    names below match the OpenBSD ones they replace, to make porting
    mechanical; the register/bit names come from bcmgenetreg.h, folded
    into bcmgenet.h in the plain shift/mask style the rest of this tree
    uses instead of the __BIT()/__BITS() macros OpenBSD/NetBSD define.
*/
#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>

#include "bcmgenet.h"

/*
 * On real aarch64 Raspberry Pi hardware (not just under QEMU, which
 * hides the difference) a plain volatile access to Device-mapped MMIO
 * still needs an explicit barrier around it, or accesses can be
 * reordered past whatever the caller does next. See RPiHDMI/RPiPWM's
 * rd32le()/wr32le() for the precedent this mirrors.
 */
static inline void __dsb(void) { asm volatile("dsb sy" ::: "memory"); }
static inline void __dmb(void) { asm volatile("dmb sy" ::: "memory"); }

ULONG BCMGENET_Read(struct bcmgenet_hw *hw, ULONG reg)
{
    ULONG val;

    __dmb();
    val = *(volatile ULONG *)(hw->base + reg);
    __dsb();

    return val;
}

void BCMGENET_Write(struct bcmgenet_hw *hw, ULONG reg, ULONG val)
{
    __dsb();
    *(volatile ULONG *)(hw->base + reg) = val;
    __dmb();
}

/*
 * TODO: port genet_mii_readreg() (bcmgenet.c:111). Write PMD/REG/READ
 * into GENET_MDIO_CMD | GENET_MDIO_START_BUSY, poll START_BUSY clear,
 * return the low 16 bits.
 */
LONG BCMGENET_MDIORead(struct bcmgenet_hw *hw, ULONG phy, ULONG reg)
{
    return -1;
}

/*
 * TODO: port genet_mii_writereg() (bcmgenet.c:132). Same as above with
 * GENET_MDIO_WRITE and the value to write in the low 16 bits.
 */
BOOL BCMGENET_MDIOWrite(struct bcmgenet_hw *hw, ULONG phy, ULONG reg, UWORD val)
{
    return FALSE;
}

/*
 * TODO: port genet_reset() (bcmgenet.c:469). UMAC_CMD_SW_RESET, flush
 * ctrl on RX/TX buffers, MIB reset, wait for the bits to self-clear.
 */
BOOL BCMGENET_HWReset(struct bcmgenet_hw *hw)
{
    return FALSE;
}

/*
 * TODO: port genet_init_rings() + genet_setup_dma() (bcmgenet.c:508,
 * 887) for the default queue (GENET_DMA_DEFAULT_QUEUE): ring buffer
 * size/desc count, start/end address, enable RX_DMA_CTRL / TX_DMA_CTRL.
 * Called once from BCMGENET_CreateUnit() after the rings are allocated.
 */
BOOL BCMGENET_HWInit(struct BCMGENETUnit *unit)
{
    return FALSE;
}

/*
 * TODO: write UMAC_MAC0 (bytes 0-3) / UMAC_MAC1 (bytes 4-5, high half)
 * and the matching MDF_ADDR0/1(0) exact-match filter entry.
 */
void BCMGENET_SetMACAddress(struct bcmgenet_hw *hw, const UBYTE *addr)
{
}

/*
 * Port of genet_lladdr_read() (bcmgenet.c:996): the bootloader/firmware
 * programs the station address into UMAC_MAC0/MAC1 before the OS ever
 * runs, so reading it back is normally enough - but do this *before*
 * BCMGENET_HWReset(), which may clear it. Prefer hw->macAddr from the
 * device tree ("local-mac-address") when BCMGENET_Discover() found one.
 */
BOOL BCMGENET_GetMACAddress(struct bcmgenet_hw *hw, UBYTE *addr)
{
    ULONG maclo, machi;

    maclo = BCMGENET_Read(hw, GENET_UMAC_MAC0);
    machi = BCMGENET_Read(hw, GENET_UMAC_MAC1);

    addr[0] = (maclo >> 24) & 0xff;
    addr[1] = (maclo >> 16) & 0xff;
    addr[2] = (maclo >> 8) & 0xff;
    addr[3] = (maclo >> 0) & 0xff;
    addr[4] = (machi >> 8) & 0xff;
    addr[5] = (machi >> 0) & 0xff;

    return TRUE;
}

/*
 * TODO: port the mii_attach()/genet_mii_statchg() side of genet_attach()
 * (bcmgenet.c:922, 183) without the generic mii(4) layer: probe
 * hw->phyAddr with MII_PHYSID1/2, then bring it up with BMCR_RESET and
 * BMCR_ANENABLE|BMCR_ANRESTART.
 */
BOOL BCMGENET_PHYInit(struct bcmgenet_hw *hw)
{
    return FALSE;
}

/*
 * TODO: port genet_update_link() (bcmgenet.c:152). Read BMSR for
 * link/autoneg-done, GBSR/ANLPAR for the negotiated speed/duplex, and
 * push EXT_RGMII_OOB_CTRL + UMAC_CMD's speed/duplex bits to match -
 * the MAC does not follow the PHY automatically.
 */
BOOL BCMGENET_PHYGetLink(struct bcmgenet_hw *hw, ULONG *mbps, BOOL *fullduplex)
{
    *mbps = 0;
    *fullduplex = FALSE;

    return FALSE;
}
