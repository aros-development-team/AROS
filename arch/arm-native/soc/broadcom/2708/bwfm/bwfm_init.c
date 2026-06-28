/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Portions ported from OpenBSD's sys/dev/ic/bwfm.c and
    sys/dev/sdmmc/if_bwfm_sdio.c, which carry the ISC licence:

      Copyright (c) 2010-2016 Broadcom Corporation
      Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>

      Permission to use, copy, modify, and/or distribute this software for any
      purpose with or without fee is hereby granted, provided that the above
      copyright notice and this permission notice appear in all copies.

      THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
      WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
      MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
      ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
      WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
      ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
      OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    bwfm.resource - Broadcom FullMAC WiFi chip bring-up over SDIO.

    Phase 2: chip bring-up - identify the chip (ChipCommon CHIPID over the
    silicon backplane), enumerate the backplane cores from the EROM, halt
    them (set_passive) and size the chip RAM. Ported from OpenBSD's bwfm.c /
    if_bwfm_sdio.c (see the ISC notice above), reworked to drive the chip
    through sdio.resource (CMD52/CMD53) instead of the OpenBSD sdmmc layer;
    AI interconnect only.

    Firmware download (needs DOS/filesystem) and the SANA-II network device
    (bwfm.device) sit on top of this chip layer in later phases.
*/

#define DEBUG 0

#include <aros/macros.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <devices/timer.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/sdio.h>

#include "bwfmreg.h"
#include "bwfm_sdio.h"
#include "bwfm_scan.h"
#include "bwfm_private.h"

APTR SDIOBase __attribute__((used)) = NULL;     /* sdio.resource (proto/sdio.h base) */
APTR KernelBase __attribute__((used)) = NULL;

/* Busy-wait microseconds off the BCM2835 1 MHz system timer (CLO). */
static void bwfm_udelay(struct BWFMBase *BWFMBase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *)(BWFMBase->bwfm_periiobase + 0x3004);
    ULONG start = AROS_LE2LONG(*clo);

    while ((AROS_LE2LONG(*clo) - start) < us)
        ;
}

/*
 * Zero a buffer of run-time length. The volatile store stops gcc from
 * synthesising a memset() call out of the loop - this resource is resident
 * in the kickstart where stdc.library cannot be opened, so any implicit
 * memset/memcpy reference makes the module's autoinit fail at boot.
 */
static void bwfm_zero(UBYTE *p, ULONG n)
{
    volatile UBYTE *d = p;

    while (n--)
        *d++ = 0;
}

/* ----------------------------------------------------------------------- */
/* SDIO register access (ported from if_bwfm_sdio.c)                       */

/*
 * Address-range based function selection:
 *   0x00000-0x007FF : function 0 (CCCR / FBR)
 *   0x10000-0x1FFFF : function 1 miscellaneous registers
 *   otherwise       : function 1 silicon-backplane window
 */
static inline uint32_t bwfm_func(uint32_t addr)
{
    return ((addr & ~0x7ff) == 0) ? 0 : 1;
}

static uint8_t bwfm_read_1(uint32_t addr)
{
    return SDIOReadByte(bwfm_func(addr), addr);
}

static void bwfm_write_1(uint32_t addr, uint8_t val)
{
    SDIOWriteByte(bwfm_func(addr), addr, val);
}

static void bwfm_backplane(struct BWFMBase *BWFMBase, uint32_t bar0)
{
    if (BWFMBase->bwfm_bar0 == bar0)
        return;

    bwfm_write_1(BWFM_SDIO_FUNC1_SBADDRLOW,  (bar0 >> 8) & 0x80);
    bwfm_write_1(BWFM_SDIO_FUNC1_SBADDRMID,  (bar0 >> 16) & 0xff);
    bwfm_write_1(BWFM_SDIO_FUNC1_SBADDRHIGH, (bar0 >> 24) & 0xff);
    BWFMBase->bwfm_bar0 = bar0;
}

static uint32_t bwfm_read_4(struct BWFMBase *BWFMBase, uint32_t addr)
{
    uint32_t bar0 = addr & ~BWFM_SDIO_SB_OFT_ADDR_MASK;
    uint32_t val = 0xffffffff;

    bwfm_backplane(BWFMBase, bar0);

    addr &= BWFM_SDIO_SB_OFT_ADDR_MASK;
    addr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

    if (SDIOReadExt(bwfm_func(addr), addr, &val, 4, 1))
        return 0xffffffff;
    return val;
}

static void bwfm_write_4(struct BWFMBase *BWFMBase, uint32_t addr, uint32_t val)
{
    uint32_t bar0 = addr & ~BWFM_SDIO_SB_OFT_ADDR_MASK;

    bwfm_backplane(BWFMBase, bar0);

    addr &= BWFM_SDIO_SB_OFT_ADDR_MASK;
    addr |= BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

    SDIOWriteExt(bwfm_func(addr), addr, &val, 4, 1);
}

/*
 * Bring up the ALP clock so the backplane is accessible (ported from
 * bwfm_sdio_buscore_prepare).
 */
static int bwfm_buscore_prepare(struct BWFMBase *BWFMBase)
{
    uint8_t clkset, clkmask, clkval;
    int i;

    clkset = BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ |
             BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF;
    bwfm_write_1(BWFM_SDIO_FUNC1_CHIPCLKCSR, clkset);

    clkmask = BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL |
              BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL;
    clkval = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);

    if ((clkval & ~clkmask) != clkset)
    {
        D(bug("[bwfm] CHIPCLKCSR wrote 0x%02x read 0x%02x\n", clkset, clkval));
        return 1;
    }

    for (i = 1000; i > 0; i--)
    {
        clkval = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);
        if (clkval & clkmask)
            break;
        bwfm_udelay(BWFMBase, 1);       /* bound the wait in time, not read count */
    }
    if (i == 0)
    {
        D(bug("[bwfm] timeout waiting for ALP, clkval 0x%02x\n", clkval));
        return 1;
    }

    clkset = BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HW_CLKREQ_OFF |
             BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_ALP;
    bwfm_write_1(BWFM_SDIO_FUNC1_CHIPCLKCSR, clkset);
    bwfm_udelay(BWFMBase, 65);          /* let ALP settle (matches reference) */

    bwfm_write_1(BWFM_SDIO_FUNC1_SDIOPULLUP, 0);

    return 0;
}

/* ----------------------------------------------------------------------- */
/* Core enumeration and reset (ported from bwfm.c, AI interconnect only)   */

static struct bwfm_core *bwfm_get_core_idx(struct BWFMBase *BWFMBase, int id, int idx)
{
    int i;

    for (i = 0; i < BWFMBase->bwfm_ncores; i++)
        if (BWFMBase->bwfm_cores[i].co_id == id && idx-- == 0)
            return &BWFMBase->bwfm_cores[i];
    return NULL;
}

static struct bwfm_core *bwfm_get_core(struct BWFMBase *BWFMBase, int id)
{
    return bwfm_get_core_idx(BWFMBase, id, 0);
}

static struct bwfm_core *bwfm_get_pmu(struct BWFMBase *BWFMBase)
{
    struct bwfm_core *cc, *pmu;

    cc = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_CHIPCOMMON);
    if (cc->co_rev >= 35 &&
        (BWFMBase->bwfm_cc_caps_ext & BWFM_CHIP_REG_CAPABILITIES_EXT_AOB_PRESENT))
    {
        pmu = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_PMU);
        if (pmu)
            return pmu;
    }
    return cc;
}

static int bwfm_ai_isup(struct BWFMBase *BWFMBase, struct bwfm_core *core)
{
    uint32_t ioctl, reset;

    ioctl = bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL);
    reset = bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL);

    if (((ioctl & (BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK)) == BWFM_AGENT_IOCTL_CLK) &&
        ((reset & BWFM_AGENT_RESET_CTL_RESET) == 0))
        return 1;
    return 0;
}

static void bwfm_ai_disable(struct BWFMBase *BWFMBase, struct bwfm_core *core,
                            uint32_t prereset, uint32_t reset)
{
    uint32_t val;
    int i;

    val = bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL);
    if ((val & BWFM_AGENT_RESET_CTL_RESET) == 0)
    {
        bwfm_write_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL,
                     prereset | BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK);
        bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL);

        bwfm_write_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL,
                     BWFM_AGENT_RESET_CTL_RESET);
        bwfm_udelay(BWFMBase, 20);

        for (i = 300; i > 0; i--)
            if (bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL) ==
                BWFM_AGENT_RESET_CTL_RESET)
                break;
        if (i == 0)
            D(bug("[bwfm] timeout on core disable\n"));
    }

    bwfm_write_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL,
                 reset | BWFM_AGENT_IOCTL_FGC | BWFM_AGENT_IOCTL_CLK);
    bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL);
}

static void bwfm_ai_reset(struct BWFMBase *BWFMBase, struct bwfm_core *core,
                          uint32_t prereset, uint32_t reset, uint32_t postreset)
{
    int i;

    bwfm_ai_disable(BWFMBase, core, prereset, reset);

    for (i = 50; i > 0; i--)
    {
        if ((bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL) &
             BWFM_AGENT_RESET_CTL_RESET) == 0)
            break;
        bwfm_write_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_RESET_CTL, 0);
        bwfm_udelay(BWFMBase, 60);
    }
    if (i == 0)
        D(bug("[bwfm] timeout on core reset\n"));

    bwfm_write_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL,
                 postreset | BWFM_AGENT_IOCTL_CLK);
    bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL);
}

static int bwfm_dmp_get_regaddr(struct BWFMBase *BWFMBase, uint32_t *erom,
                                uint32_t *base, uint32_t *wrap)
{
    uint8_t type = 0, stype, wraptype;
    uint32_t val;

    *base = 0;
    *wrap = 0;

    val = bwfm_read_4(BWFMBase, *erom);
    type = val & BWFM_DMP_DESC_MASK;
    if (type == BWFM_DMP_DESC_MASTER_PORT)
    {
        wraptype = BWFM_DMP_SLAVE_TYPE_MWRAP;
        *erom += 4;
    }
    else if ((type & ~BWFM_DMP_DESC_ADDRSIZE_GT32) == BWFM_DMP_DESC_ADDRESS)
        wraptype = BWFM_DMP_SLAVE_TYPE_SWRAP;
    else
        return 1;

    do
    {
        uint8_t sztype;

        do
        {
            val = bwfm_read_4(BWFMBase, *erom);
            type = val & BWFM_DMP_DESC_MASK;
            if (type == BWFM_DMP_DESC_COMPONENT)
                return 0;
            if (type == BWFM_DMP_DESC_EOT)
                return 1;
            *erom += 4;
        }
        while ((type & ~BWFM_DMP_DESC_ADDRSIZE_GT32) != BWFM_DMP_DESC_ADDRESS);

        if (type & BWFM_DMP_DESC_ADDRSIZE_GT32)
            *erom += 4;

        sztype = (val & BWFM_DMP_SLAVE_SIZE_TYPE) >> BWFM_DMP_SLAVE_SIZE_TYPE_S;
        if (sztype == BWFM_DMP_SLAVE_SIZE_DESC)
        {
            uint32_t v2 = bwfm_read_4(BWFMBase, *erom);
            if ((v2 & BWFM_DMP_DESC_MASK) & BWFM_DMP_DESC_ADDRSIZE_GT32)
                *erom += 8;
            else
                *erom += 4;
        }
        if (sztype != BWFM_DMP_SLAVE_SIZE_4K && sztype != BWFM_DMP_SLAVE_SIZE_8K)
            continue;

        stype = (val & BWFM_DMP_SLAVE_TYPE) >> BWFM_DMP_SLAVE_TYPE_S;
        if (*base == 0 && stype == BWFM_DMP_SLAVE_TYPE_SLAVE)
            *base = val & BWFM_DMP_SLAVE_ADDR_BASE;
        if (*wrap == 0 && stype == wraptype)
            *wrap = val & BWFM_DMP_SLAVE_ADDR_BASE;
    }
    while (*base == 0 || *wrap == 0);

    return 0;
}

static void bwfm_erom_scan(struct BWFMBase *BWFMBase)
{
    uint32_t erom, val, base, wrap;
    uint8_t type = 0, nmw, nsw, rev;
    uint16_t id;

    BWFMBase->bwfm_ncores = 0;

    erom = bwfm_read_4(BWFMBase, BWFM_CHIP_BASE + BWFM_CHIP_REG_EROMPTR);
    while (type != BWFM_DMP_DESC_EOT)
    {
        val = bwfm_read_4(BWFMBase, erom);
        type = val & BWFM_DMP_DESC_MASK;
        erom += 4;

        if (type != BWFM_DMP_DESC_COMPONENT)
            continue;

        id = (val & BWFM_DMP_COMP_PARTNUM) >> BWFM_DMP_COMP_PARTNUM_S;

        val = bwfm_read_4(BWFMBase, erom);
        type = val & BWFM_DMP_DESC_MASK;
        erom += 4;
        if (type != BWFM_DMP_DESC_COMPONENT)
        {
            D(bug("[bwfm] EROM: not a component descriptor\n"));
            return;
        }

        nmw = (val & BWFM_DMP_COMP_NUM_MWRAP) >> BWFM_DMP_COMP_NUM_MWRAP_S;
        nsw = (val & BWFM_DMP_COMP_NUM_SWRAP) >> BWFM_DMP_COMP_NUM_SWRAP_S;
        rev = (val & BWFM_DMP_COMP_REVISION) >> BWFM_DMP_COMP_REVISION_S;

        if (nmw + nsw == 0 && id != BWFM_AGENT_CORE_PMU && id != BWFM_AGENT_CORE_GCI)
            continue;

        if (bwfm_dmp_get_regaddr(BWFMBase, &erom, &base, &wrap))
            continue;

        if (BWFMBase->bwfm_ncores >= BWFM_MAX_CORES)
        {
            D(bug("[bwfm] EROM: core table full\n"));
            return;
        }
        BWFMBase->bwfm_cores[BWFMBase->bwfm_ncores].co_id = id;
        BWFMBase->bwfm_cores[BWFMBase->bwfm_ncores].co_rev = rev;
        BWFMBase->bwfm_cores[BWFMBase->bwfm_ncores].co_base = base;
        BWFMBase->bwfm_cores[BWFMBase->bwfm_ncores].co_wrapbase = wrap;
        BWFMBase->bwfm_ncores++;
    }
}

/* Put the CPU/802.11/memory cores into a known halted (passive) state. */
static void bwfm_set_passive(struct BWFMBase *BWFMBase)
{
    struct bwfm_core *core;

    if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_ARM_CR4)) != NULL)
    {
        uint32_t val = bwfm_read_4(BWFMBase, core->co_wrapbase + BWFM_AGENT_IOCTL);
        int i = 0;

        bwfm_ai_reset(BWFMBase, core, val & BWFM_AGENT_IOCTL_ARMCR4_CPUHALT,
                      BWFM_AGENT_IOCTL_ARMCR4_CPUHALT, BWFM_AGENT_IOCTL_ARMCR4_CPUHALT);
        while ((core = bwfm_get_core_idx(BWFMBase, BWFM_AGENT_CORE_80211, i++)))
            bwfm_ai_disable(BWFMBase, core,
                            BWFM_AGENT_D11_IOCTL_PHYRESET | BWFM_AGENT_D11_IOCTL_PHYCLOCKEN,
                            BWFM_AGENT_D11_IOCTL_PHYCLOCKEN);
        return;
    }

    if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_ARM_CM3)) != NULL)
    {
        bwfm_ai_disable(BWFMBase, core, 0, 0);
        core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_80211);
        bwfm_ai_reset(BWFMBase, core,
                      BWFM_AGENT_D11_IOCTL_PHYRESET | BWFM_AGENT_D11_IOCTL_PHYCLOCKEN,
                      BWFM_AGENT_D11_IOCTL_PHYCLOCKEN, BWFM_AGENT_D11_IOCTL_PHYCLOCKEN);
        core = bwfm_get_core(BWFMBase, BWFM_AGENT_INTERNAL_MEM);
        bwfm_ai_reset(BWFMBase, core, 0, 0, 0);

        if (BWFMBase->bwfm_chip == BRCM_CC_43430_CHIP_ID)
        {
            bwfm_write_4(BWFMBase, core->co_base + BWFM_SOCRAM_BANKIDX, 3);
            bwfm_write_4(BWFMBase, core->co_base + BWFM_SOCRAM_BANKPDA, 0);
        }
        return;
    }
}

static void bwfm_socram_ramsize(struct BWFMBase *BWFMBase, struct bwfm_core *core)
{
    uint32_t coreinfo, nb, lss, banksize, bankinfo;
    uint32_t ramsize = 0, srsize = 0;
    int i;

    if (!bwfm_ai_isup(BWFMBase, core))
        bwfm_ai_reset(BWFMBase, core, 0, 0, 0);

    coreinfo = bwfm_read_4(BWFMBase, core->co_base + BWFM_SOCRAM_COREINFO);
    nb = (coreinfo & BWFM_SOCRAM_COREINFO_SRNB_MASK) >> BWFM_SOCRAM_COREINFO_SRNB_SHIFT;

    if (core->co_rev <= 7 || core->co_rev == 12)
    {
        banksize = coreinfo & BWFM_SOCRAM_COREINFO_SRBSZ_MASK;
        lss = (coreinfo & BWFM_SOCRAM_COREINFO_LSS_MASK) >> BWFM_SOCRAM_COREINFO_LSS_SHIFT;
        if (lss != 0)
            nb--;
        ramsize = nb * (1 << (banksize + BWFM_SOCRAM_COREINFO_SRBSZ_BASE));
        if (lss != 0)
            ramsize += (1 << ((lss - 1) + BWFM_SOCRAM_COREINFO_SRBSZ_BASE));
    }
    else
    {
        for (i = 0; i < nb; i++)
        {
            bwfm_write_4(BWFMBase, core->co_base + BWFM_SOCRAM_BANKIDX,
                         (BWFM_SOCRAM_BANKIDX_MEMTYPE_RAM << BWFM_SOCRAM_BANKIDX_MEMTYPE_SHIFT) | i);
            bankinfo = bwfm_read_4(BWFMBase, core->co_base + BWFM_SOCRAM_BANKINFO);
            banksize = ((bankinfo & BWFM_SOCRAM_BANKINFO_SZMASK) + 1) * BWFM_SOCRAM_BANKINFO_SZBASE;
            ramsize += banksize;
            if (bankinfo & BWFM_SOCRAM_BANKINFO_RETNTRAM_MASK)
                srsize += banksize;
        }
    }

    if (BWFMBase->bwfm_chip == BRCM_CC_43430_CHIP_ID)
        srsize = 64 * 1024;

    BWFMBase->bwfm_ramsize = ramsize;
    BWFMBase->bwfm_srsize = srsize;
}

static void bwfm_tcm_ramsize(struct BWFMBase *BWFMBase, struct bwfm_core *core)
{
    uint32_t cap, nab, nbb, totb, bxinfo, blksize, ramsize = 0;
    int i;

    cap = bwfm_read_4(BWFMBase, core->co_base + BWFM_ARMCR4_CAP);
    nab = (cap & BWFM_ARMCR4_CAP_TCBANB_MASK) >> BWFM_ARMCR4_CAP_TCBANB_SHIFT;
    nbb = (cap & BWFM_ARMCR4_CAP_TCBBNB_MASK) >> BWFM_ARMCR4_CAP_TCBBNB_SHIFT;
    totb = nab + nbb;

    for (i = 0; i < totb; i++)
    {
        bwfm_write_4(BWFMBase, core->co_base + BWFM_ARMCR4_BANKIDX, i);
        bxinfo = bwfm_read_4(BWFMBase, core->co_base + BWFM_ARMCR4_BANKINFO);
        blksize = (bxinfo & BWFM_ARMCR4_BANKINFO_BLK_1K_MASK) ? 1024 : 8192;
        ramsize += ((bxinfo & BWFM_ARMCR4_BANKINFO_BSZ_MASK) + 1) * blksize;
    }

    BWFMBase->bwfm_ramsize = ramsize;
}

static void bwfm_tcm_rambase(struct BWFMBase *BWFMBase)
{
    switch (BWFMBase->bwfm_chip)
    {
    case BRCM_CC_4345_CHIP_ID:          /* Pi 3B+ / Pi 4 (BCM43455) */
        BWFMBase->bwfm_rambase = 0x198000;
        break;
    case BRCM_CC_4339_CHIP_ID:
        BWFMBase->bwfm_rambase = 0x180000;
        break;
    default:
        D(bug("[bwfm] unknown TCM rambase for chip %x\n", BWFMBase->bwfm_chip));
        break;
    }
}

/* ----------------------------------------------------------------------- */
/* Chip identification                                                     */

static int bwfm_do_attach(struct BWFMBase *BWFMBase)
{
    uint32_t val;
    int type;

    BWFMBase->bwfm_attached = FALSE;
    BWFMBase->bwfm_bar0 = ~0u;          /* force first backplane write */

    if (!SDIOBase)
        return FALSE;

    /* Drive the lazy SDIO bring-up: the first SDIOProbe() powers the WiFi chip
     * and runs CMD5/CMD3/CMD7; later calls return the cached result (it is
     * idempotent on sdio_Present), so this is safe to call on every attach. */
    if (!SDIOProbe())
    {
        D(bug("[bwfm] no SDIO device present\n"));
        return FALSE;
    }

    /* Block sizes: F1 (backplane) 64, F2 (data) 512; enable F1 */
    SDIOSetBlockSize(1, 64);
    SDIOSetBlockSize(2, 512);
    if (SDIOEnableFunction(1))
    {
        D(bug("[bwfm] cannot enable function 1\n"));
        return FALSE;
    }

    if (bwfm_buscore_prepare(BWFMBase))
        return FALSE;

    val = bwfm_read_4(BWFMBase, BWFM_CHIP_BASE + BWFM_CHIP_REG_CHIPID);
    BWFMBase->bwfm_chip = BWFM_CHIP_CHIPID_ID(val);
    BWFMBase->bwfm_chiprev = BWFM_CHIP_CHIPID_REV(val);
    type = BWFM_CHIP_CHIPID_TYPE(val);

    /* Print like OpenBSD: decimal for "43xxx" parts, hex for "0x4xxx" parts */
    if (BWFMBase->bwfm_chip > 0xa000 || BWFMBase->bwfm_chip < 0x4000)
        D(bug("[bwfm] CHIPID 0x%08x -> BCM%u rev %d (type %d)\n",
              val, BWFMBase->bwfm_chip, BWFMBase->bwfm_chiprev, type));
    else
        D(bug("[bwfm] CHIPID 0x%08x -> BCM%x rev %d (type %d)\n",
              val, BWFMBase->bwfm_chip, BWFMBase->bwfm_chiprev, type));

    if (type != BWFM_CHIP_CHIPID_TYPE_SOCI_AI)
    {
        D(bug("[bwfm] unsupported SoC interconnect type %d\n", type));
        return FALSE;
    }

    /* Enumerate the silicon-backplane cores from the EROM */
    bwfm_erom_scan(BWFMBase);
    {
        int i, cpu_found = 0, need_socram = 0, has_socram = 0;
        int cc_found = 0, d11_found = 0, sdiodev_found = 0;

        for (i = 0; i < BWFMBase->bwfm_ncores; i++)
        {
            struct bwfm_core *core = &BWFMBase->bwfm_cores[i];

            D(bug("[bwfm]   core 0x%03x rev %d base 0x%08x wrap 0x%08x\n",
                  core->co_id, core->co_rev, core->co_base, core->co_wrapbase));

            switch (core->co_id)
            {
            case BWFM_AGENT_CORE_ARM_CM3:
                need_socram = 1;
                /* fallthrough */
            case BWFM_AGENT_CORE_ARM_CR4:
            case BWFM_AGENT_CORE_ARM_CA7:
                cpu_found = 1;
                break;
            case BWFM_AGENT_INTERNAL_MEM:
                has_socram = 1;
                break;
            case BWFM_AGENT_CORE_CHIPCOMMON:
                cc_found = 1;
                break;
            case BWFM_AGENT_CORE_80211:
                d11_found = 1;
                break;
            case BWFM_AGENT_CORE_SDIO_DEV:
                sdiodev_found = 1;
                break;
            }
        }

        if (!cpu_found)
        {
            D(bug("[bwfm] no CPU core detected\n"));
            return FALSE;
        }
        if (need_socram && !has_socram)
        {
            D(bug("[bwfm] RAM core not provided\n"));
            return FALSE;
        }
        /* ChipCommon, the 802.11 MAC and the SDIO device core are dereferenced
         * unconditionally below (bwfm_set_passive / bwfm_get_pmu / bwfm_dev_* /
         * bwfm_frame_rw); a malformed EROM that omits any of them would fault,
         * so bail here instead. The datapath only runs once bwfm_attached, so
         * this one check makes every later bwfm_get_core() of these safe. */
        if (!cc_found || !d11_found || !sdiodev_found)
        {
            D(bug("[bwfm] required core missing (cc %d d11 %d sdiodev %d)\n",
                  cc_found, d11_found, sdiodev_found));
            return FALSE;
        }
    }

    /* Halt the cores, then size the chip RAM */
    bwfm_set_passive(BWFMBase);

    {
        struct bwfm_core *core, *cc, *pmu;

        if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_ARM_CR4)) != NULL)
        {
            bwfm_tcm_ramsize(BWFMBase, core);
            bwfm_tcm_rambase(BWFMBase);
        }
        else if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_INTERNAL_MEM)) != NULL)
        {
            bwfm_socram_ramsize(BWFMBase, core);
        }

        cc = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_CHIPCOMMON);
        BWFMBase->bwfm_cc_caps = bwfm_read_4(BWFMBase,
            cc->co_base + BWFM_CHIP_REG_CAPABILITIES);
        BWFMBase->bwfm_cc_caps_ext = bwfm_read_4(BWFMBase,
            cc->co_base + BWFM_CHIP_REG_CAPABILITIES_EXT);

        pmu = bwfm_get_pmu(BWFMBase);
        if (BWFMBase->bwfm_cc_caps & BWFM_CHIP_REG_CAPABILITIES_PMU)
            BWFMBase->bwfm_pmurev = bwfm_read_4(BWFMBase,
                pmu->co_base + BWFM_CHIP_REG_PMUCAPABILITIES) &
                BWFM_CHIP_REG_PMUCAPABILITIES_REV_MASK;
    }

    D(bug("[bwfm] %d cores, RAM base 0x%06x size %d KB (sr %d KB), PMU rev %d\n",
          BWFMBase->bwfm_ncores, BWFMBase->bwfm_rambase,
          BWFMBase->bwfm_ramsize / 1024, BWFMBase->bwfm_srsize / 1024,
          BWFMBase->bwfm_pmurev));

    /*
     * Post-enumeration chip setup (from bwfm_sdio_attach):
     *  - KSO ("keep SDIO on") for SDIO_DEV core rev >= 12 (BCM43455/3B+/4):
     *    without it the chip may drop the SDIO clock and go silent;
     *  - assert the WLAN reset bit in the Broadcom CARDCTRL CCCR register;
     *  - trigger a PMU resource reload.
     */
    {
        struct bwfm_core *sdiodev = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_SDIO_DEV);
        struct bwfm_core *pmu = bwfm_get_pmu(BWFMBase);

        if (sdiodev && sdiodev->co_rev >= 12)
        {
            uint8_t cs = bwfm_read_1(BWFM_SDIO_FUNC1_SLEEPCSR);
            if (!(cs & BWFM_SDIO_FUNC1_SLEEPCSR_KSO))
                bwfm_write_1(BWFM_SDIO_FUNC1_SLEEPCSR,
                             cs | BWFM_SDIO_FUNC1_SLEEPCSR_KSO);
        }

        bwfm_write_1(BWFM_SDIO_CCCR_CARDCTRL,
                     bwfm_read_1(BWFM_SDIO_CCCR_CARDCTRL) |
                     BWFM_SDIO_CCCR_CARDCTRL_WLANRESET);

        if (pmu)
            bwfm_write_4(BWFMBase, pmu->co_base + BWFM_CHIP_REG_PMUCONTROL,
                         bwfm_read_4(BWFMBase, pmu->co_base + BWFM_CHIP_REG_PMUCONTROL) |
                         (BWFM_CHIP_REG_PMUCONTROL_RES_RELOAD <<
                          BWFM_CHIP_REG_PMUCONTROL_RES_SHIFT));
    }

    BWFMBase->bwfm_attached = TRUE;
    return TRUE;
}

/* ----------------------------------------------------------------------- */
/* Firmware download (ported from if_bwfm_sdio.c bwfm_sdio_load_microcode)  */

/* Write a buffer into chip RAM through the backplane, paging at 32 KB
 * windows and chunking each window into <=512-byte CMD53 byte-mode writes
 * (sdio.resource's extended transfer limit). Lengths are rounded up to 4. */
static int bwfm_ram_write(struct BWFMBase *BWFMBase, uint32_t ramaddr,
                          const UBYTE *data, ULONG len)
{
    UBYTE bounce[512];
    ULONG off = 0;

    while (len > 0)
    {
        uint32_t sbaddr = ramaddr + off;
        uint32_t sdoff = sbaddr & BWFM_SDIO_SB_OFT_ADDR_MASK;
        ULONG winleft = BWFM_SDIO_SB_OFT_ADDR_PAGE - sdoff;
        ULONG n = (len < winleft) ? len : winleft;
        ULONG done = 0;

        bwfm_backplane(BWFMBase, sbaddr & ~BWFM_SDIO_SB_OFT_ADDR_MASK);

        while (done < n)
        {
            ULONG chunk = (n - done < 512) ? (n - done) : 512;
            /* Round up to the func1 block size (64) so sdio.resource uses
             * block-mode CMD53 (a large byte-mode access returns OUT_OF_RANGE).
             * The few pad bytes land in chip RAM beyond the image - harmless. */
            ULONG wlen = (chunk + 63) & ~63UL;
            const UBYTE *src = data + off + done;

            if (wlen != chunk)
            {
                CopyMem((APTR)src, bounce, chunk);
                bwfm_zero(bounce + chunk, wlen - chunk);
                src = bounce;
            }

            if (SDIOWriteExt(1, (sdoff + done) | BWFM_SDIO_SB_ACCESS_2_4B_FLAG,
                             (APTR)src, wlen, 1))
                return -1;
            done += chunk;
        }

        off += n;
        len -= n;
    }
    return 0;
}

/*
 * Request and wait for the ALP clock. Firmware is uploaded on ALP - the
 * HT/PLL clock is not available until the firmware configures the PMU, so
 * waiting for HT here would just time out. HT is forced only after the
 * firmware signals ready (see BWFMStartFirmware). Matches OpenBSD/brcmfmac.
 */
static int bwfm_alpclk_on(struct BWFMBase *BWFMBase)
{
    uint8_t clk = 0;
    int i;

    bwfm_write_1(BWFM_SDIO_FUNC1_CHIPCLKCSR, BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL_REQ);
    for (i = 0; i < 5000; i++)
    {
        clk = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);
        if (clk & (BWFM_SDIO_FUNC1_CHIPCLKCSR_ALP_AVAIL |
                   BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL))
            return 0;
        bwfm_udelay(BWFMBase, 1000);
    }
    D(bug("[bwfm] ALP clock timeout (csr 0x%02x)\n", clk));
    return -1;
}

/* SDPCM (SDIO_DEV core) register access */
static uint32_t bwfm_dev_read(struct BWFMBase *BWFMBase, uint32_t reg)
{
    struct bwfm_core *core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_SDIO_DEV);
    return bwfm_read_4(BWFMBase, core->co_base + reg);
}

static void bwfm_dev_write(struct BWFMBase *BWFMBase, uint32_t reg, uint32_t val)
{
    struct bwfm_core *core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_SDIO_DEV);
    bwfm_write_4(BWFMBase, core->co_base + reg, val);
}

static void bwfm_buscore_activate(struct BWFMBase *BWFMBase, uint32_t rstvec)
{
    bwfm_dev_write(BWFMBase, SDPCMD_INTSTATUS, 0xFFFFFFFF);
    if (rstvec)
        bwfm_write_4(BWFMBase, 0, rstvec);      /* exactly 4 bytes, like OpenBSD;
                                                 * bwfm_ram_write would block-mode
                                                 * pad this to 64 and clobber RAM
                                                 * at backplane addr 4..63 */
}

/* Kick the CPU core out of reset to run the just-loaded firmware. */
static int bwfm_set_active(struct BWFMBase *BWFMBase, uint32_t rstvec)
{
    struct bwfm_core *core, *mem;

    if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_ARM_CR4)) != NULL)
    {
        bwfm_buscore_activate(BWFMBase, rstvec);
        bwfm_ai_reset(BWFMBase, core, BWFM_AGENT_IOCTL_ARMCR4_CPUHALT, 0, 0);
        return 0;
    }

    if ((core = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_ARM_CM3)) != NULL)
    {
        mem = bwfm_get_core(BWFMBase, BWFM_AGENT_INTERNAL_MEM);
        if (!bwfm_ai_isup(BWFMBase, mem))
            return -1;
        bwfm_buscore_activate(BWFMBase, 0);
        bwfm_ai_reset(BWFMBase, core, 0, 0, 0);
        return 0;
    }

    return -1;
}

/* ----------------------------------------------------------------------- */
/* SDPCM control channel + BCDC commands                                    */

/* Read/write one SDPCM frame through function 2 at the ChipCommon window.
 * Control frames are small, so a single (<=512 byte) CMD53 suffices. */
static int bwfm_frame_rw(struct BWFMBase *BWFMBase, APTR buf, ULONG len, int write)
{
    struct bwfm_core *cc = bwfm_get_core(BWFMBase, BWFM_AGENT_CORE_CHIPCOMMON);
    uint32_t addr = cc->co_base;

    bwfm_backplane(BWFMBase, addr & ~BWFM_SDIO_SB_OFT_ADDR_MASK);
    addr = (addr & BWFM_SDIO_SB_OFT_ADDR_MASK) | BWFM_SDIO_SB_ACCESS_2_4B_FLAG;

    /* Function 2 is the frame FIFO: writes increment the address, but reads
     * use a FIXED address (incr=0) so successive header/body reads pull from
     * the FIFO - matching OpenBSD's read_multi_1 vs write_region_1 split. */
    if (write)
        return SDIOWriteExt(2, addr, buf, len, 1);
    return SDIOReadExt(2, addr, buf, len, 0);
}

/* Forward decls: bwfm_dcmd salvages frames that arrive while it polls (below). */
static int bwfm_frame_eth(UBYTE *frame, ULONG flen, ULONG *ehoff, ULONG *ethlen);
static void bwfm_glom_salvage(struct BWFMBase *BWFMBase, UBYTE *desc, ULONG flen);

/*
 * Push one already-extracted 802.3 event frame into the hand-off ring for an
 * in-progress join/scan. Caller holds bwfm_Sem. Factored out of BWFMEventPost
 * so bwfm_dcmd can reuse it from its event-salvage path.
 */
static void bwfm_event_push(struct BWFMBase *BWFMBase, const UBYTE *ev, ULONG len)
{
    struct bwfm_evslot *s;

    if (!BWFMBase->bwfm_evlisten || ev == NULL || len == 0)
        return;
    if (len > BWFM_EVSLOT_SIZE)
        len = BWFM_EVSLOT_SIZE;
    if ((UWORD)(BWFMBase->bwfm_evhead - BWFMBase->bwfm_evtail) >= BWFM_EVRING_SLOTS)
        BWFMBase->bwfm_evtail++;             /* ring full: drop the oldest */
    s = &BWFMBase->bwfm_evring[BWFMBase->bwfm_evhead % BWFM_EVRING_SLOTS];
    s->ev_len = (UWORD)len;
    CopyMem((APTR)ev, s->ev_data, len);
    BWFMBase->bwfm_evhead++;
}

/*
 * Issue one BCDC command over the SDPCM control channel and wait for the
 * matching response (synchronous, polled - no glomming/flow-control, which
 * is fine for control transactions). For GET, the response payload is
 * copied back into data. Returns 0 on success, firmware status on error,
 * -1 on transport failure/timeout.
 */
static int bwfm_dcmd(struct BWFMBase *BWFMBase, int cmd, int set, void *data, ULONG dlen)
{
    UBYTE frame[512];
    struct bwfm_sdio_hwhdr *hw = (struct bwfm_sdio_hwhdr *)frame;
    struct bwfm_sdio_swhdr *sw = (struct bwfm_sdio_swhdr *)(frame + 4);
    struct bwfm_bcdc_dcmd_hdr *dc = (struct bwfm_bcdc_dcmd_hdr *)(frame + 12);
    ULONG total, padded;
    int reqid = (++BWFMBase->bwfm_reqid) & 0xffff;
    int tries;

    if (dlen > sizeof(frame) - (12 + 16))
        return -1;

    dc->cmd = AROS_LONG2LE(cmd);
    dc->len = AROS_LONG2LE(dlen);
    dc->flags = AROS_LONG2LE((set ? BWFM_BCDC_DCMD_SET : 0) | BWFM_BCDC_DCMD_ID_SET(reqid));
    dc->status = 0;
    if (dlen)
        CopyMem(data, frame + 12 + 16, dlen);

    total = 12 + 16 + dlen;
    hw->frmlen = AROS_WORD2LE(total);
    hw->cksum = AROS_WORD2LE((UWORD)~total);
    sw->seqnr = BWFMBase->bwfm_txseq++;
    sw->chanflag = BWFM_SDIO_SWHDR_CHANNEL_CONTROL;
    sw->nextlen = 0;
    sw->dataoff = 12;
    sw->flowctl = 0;
    sw->maxseqnr = 0;
    sw->res0 = 0;

    padded = (total + 3) & ~3UL;
    bwfm_zero(frame + total, padded - total);

    if (bwfm_frame_rw(BWFMBase, frame, padded, 1))
        return -1;

    for (tries = 0; tries < 1000; tries++)
    {
        UWORD frmlen, cksum;
        ULONG flen, rlen;

        if (bwfm_frame_rw(BWFMBase, frame, 12, 0))
        {
            bwfm_udelay(BWFMBase, 1000);
            continue;
        }
        frmlen = AROS_LE2WORD(hw->frmlen);
        cksum = AROS_LE2WORD(hw->cksum);
        if (frmlen == 0 && cksum == 0)
        {
            bwfm_udelay(BWFMBase, 1000);
            continue;
        }
        if ((UWORD)(frmlen ^ cksum) != 0xffff || frmlen < 12)
            return -1;

        flen = frmlen - 12;
        rlen = (flen + 3) & ~3UL;
        if (12 + rlen > sizeof(frame))
            return -1;          /* oversized frame - cannot buffer (rare) */
        if (flen && bwfm_frame_rw(BWFMBase, frame + 12, rlen, 0))
            return -1;

        BWFMBase->bwfm_txmax = sw->maxseqnr;    /* update the tx-credit window */

        if ((sw->chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK) == BWFM_SDIO_SWHDR_CHANNEL_CONTROL)
        {
            struct bwfm_bcdc_dcmd_hdr *rdc;
            ULONG rflags;

            /* Validate dataoff before dereferencing - a malformed frame
             * must not push the BCDC header past the bounce buffer. */
            if (sw->dataoff < 12 || (ULONG)sw->dataoff + sizeof(*rdc) > 12 + flen)
                return -1;

            rdc = (struct bwfm_bcdc_dcmd_hdr *)(frame + sw->dataoff);
            rflags = AROS_LE2LONG(rdc->flags);

            if (BWFM_BCDC_DCMD_ID_GET(rflags) != (ULONG)reqid)
                continue;       /* not our response */
            if (rflags & BWFM_BCDC_DCMD_ERROR)
                return (int)AROS_LE2LONG(rdc->status);
            if (data && dlen)
            {
                ULONG rdlen = AROS_LE2LONG(rdc->len);
                ULONG avail = (12 + flen) - ((ULONG)sw->dataoff + 16);
                ULONG n = (rdlen < dlen) ? rdlen : dlen;
                if (n > avail)          /* never read past the received payload */
                    n = avail;
                CopyMem((UBYTE *)rdc + 16, data, n);
            }
            return 0;
        }
        /* Non-control frame arriving while we poll for our control reply. */
        {
            int ch = sw->chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK;
            ULONG ehoff, ethlen;

            if (ch == BWFM_SDIO_SWHDR_CHANNEL_GLOM)
            {
                /* A GLOM superframe leaves its sub-frames sitting in F2 after
                 * the descriptor we just read - they MUST be deaggregated even
                 * if no one is listening, or every later read desyncs. Salvage
                 * any events to the ring (no-op when not listening), drop data. */
                bwfm_glom_salvage(BWFMBase, frame, flen);
            }
            else if (BWFMBase->bwfm_evlisten &&
                     (ch == BWFM_SDIO_SWHDR_CHANNEL_EVENT ||
                      ch == BWFM_SDIO_SWHDR_CHANNEL_DATA) &&
                     bwfm_frame_eth(frame, flen, &ehoff, &ethlen))
            {
                /* Don't lose a firmware event (LINK_CTL) that lands while we
                 * poll: hand it to the ring so the join/scan still sees it. A
                 * swallowed E_LINK/E_SET_SSID here made join flaky. The full
                 * EVENT/DATA frame was already read, so F2 is not left desynced. */
                struct bwfm_event *e = (struct bwfm_event *)(frame + ehoff);

                if (ethlen >= sizeof(*e) &&
                    AROS_BE2WORD(e->ehdr.ether_type) == BWFM_ETHERTYPE_LINK_CTL)
                    bwfm_event_push(BWFMBase, frame + ehoff, ethlen);
            }
            if (BWFMBase->bwfm_evlisten)
                D(bug("[bwfm] dcmd salvaged chan %d frame (len %u) while polling reply\n",
                      ch, (unsigned)flen));
        }
    }
    return -1;
}

/* Set a named firmware variable (iovar). Caller holds bwfm_Sem. */
static int bwfm_iovar_set(struct BWFMBase *BWFMBase, const char *name,
                          const void *data, ULONG len)
{
    UBYTE tmp[300];
    ULONG nlen = 0;

    while (name[nlen])
        nlen++;
    if (nlen + 1 + len > sizeof(tmp))
        return -1;

    CopyMem((APTR)name, tmp, nlen + 1);
    if (len)
        CopyMem((APTR)data, tmp + nlen + 1, len);

    return bwfm_dcmd(BWFMBase, BWFM_C_SET_VAR, 1, tmp, nlen + 1 + len);
}

/* Get a named firmware variable (iovar). Caller holds bwfm_Sem. */
static int bwfm_iovar_get(struct BWFMBase *BWFMBase, const char *name,
                          void *data, ULONG len)
{
    UBYTE tmp[300];
    ULONG nlen = 0;
    int err;

    while (name[nlen])
        nlen++;
    if (nlen + 1 + len > sizeof(tmp))
        return -1;

    CopyMem((APTR)name, tmp, nlen + 1);
    if (len)
        bwfm_zero(tmp + nlen + 1, len);

    err = bwfm_dcmd(BWFMBase, BWFM_C_GET_VAR, 0, tmp, nlen + 1 + len);
    if (!err && len)
        CopyMem(tmp, data, len);
    return err;
}

/* Issue an integer firmware ioctl (the payload is a single little-endian u32). */
static int bwfm_set_int(struct BWFMBase *BWFMBase, int cmd, uint32_t val)
{
    uint32_t le = AROS_LONG2LE(val);

    return bwfm_dcmd(BWFMBase, cmd, 1, &le, sizeof(le));
}

/* Set a named firmware variable to a little-endian u32. */
static int bwfm_iovar_set_int(struct BWFMBase *BWFMBase, const char *name, uint32_t val)
{
    uint32_t le = AROS_LONG2LE(val);

    return bwfm_iovar_set(BWFMBase, name, &le, sizeof(le));
}

/*
 * Read one raw SDPCM frame off the func2 FIFO into `frame` (max framesz bytes).
 * On success returns 1, sets *flenp to the payload length after the 12-byte
 * SDPCM hw/sw header and *chanp to the SDPCM channel. Returns 0 if nothing is
 * pending or the frame is malformed. Caller holds bwfm_Sem.
 */
static int bwfm_rx_read(struct BWFMBase *BWFMBase, UBYTE *frame, ULONG framesz,
                        ULONG *flenp, int *chanp)
{
    struct bwfm_sdio_hwhdr *hw = (struct bwfm_sdio_hwhdr *)frame;
    struct bwfm_sdio_swhdr *sw = (struct bwfm_sdio_swhdr *)(frame + 4);
    UWORD frmlen, cksum;
    ULONG flen, rlen;

    if (bwfm_frame_rw(BWFMBase, frame, 12, 0))
        return 0;
    frmlen = AROS_LE2WORD(hw->frmlen);
    cksum = AROS_LE2WORD(hw->cksum);
    if (frmlen == 0 && cksum == 0)
        return 0;
    if ((UWORD)(frmlen ^ cksum) != 0xffff || frmlen < 12)
        return 0;

    flen = frmlen - 12;
    rlen = (flen + 3) & ~3UL;
    if (flen == 0 || 12 + rlen > framesz)
        return 0;
    if (bwfm_frame_rw(BWFMBase, frame + 12, rlen, 0))
        return 0;

    BWFMBase->bwfm_txmax = sw->maxseqnr;     /* tx-credit window from every frame */
    *flenp = flen;
    *chanp = sw->chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK;
    return 1;
}

/*
 * Locate the 802.3 ethernet frame inside an already-read SDPCM EVENT/DATA frame
 * (`frame` = 12-byte hw/sw header + flen payload). Sets *ehoff (offset within
 * frame, past the SDPCM headers and the BCDC/BDC data header) and *ethlen.
 * Returns 1 on a well-formed EVENT/DATA frame, 0 otherwise.
 */
static int bwfm_frame_eth(UBYTE *frame, ULONG flen, ULONG *ehoff, ULONG *ethlen)
{
    struct bwfm_sdio_swhdr *sw = (struct bwfm_sdio_swhdr *)(frame + 4);
    struct bwfm_bcdc_data_hdr *bcdc;
    ULONG off, eo;

    if (sw->dataoff < 12)
        return 0;
    off = sw->dataoff;
    if (off + sizeof(*bcdc) > 12 + flen)
        return 0;
    bcdc = (struct bwfm_bcdc_data_hdr *)(frame + off);
    eo = off + sizeof(*bcdc) + (ULONG)bcdc->data_offset * 4;
    if (eo > 12 + flen)
        return 0;
    *ehoff = eo;
    *ethlen = (12 + flen) - eo;
    return 1;
}

/*
 * Encode the *info word BWFMRxFrame hands the pump: SDPCM channel in the low
 * byte, BWFM_RX_EVENT if the 802.3 frame is a firmware event (LINK_CTL
 * ethertype), and the event type in the high half.
 */
static ULONG bwfm_rx_info(UBYTE *eth, ULONG ethlen, int chan)
{
    struct bwfm_event *e = (struct bwfm_event *)eth;
    int is_event = (ethlen >= sizeof(*e) &&
                    AROS_BE2WORD(e->ehdr.ether_type) == BWFM_ETHERTYPE_LINK_CTL);
    ULONG etype = is_event ? (AROS_BE2LONG(e->msg.event_type) & 0xffff) : 0;

    return ((ULONG)chan & 0xff) | (is_event ? BWFM_RX_EVENT : 0) | (etype << 16);
}

/* ----------------------------------------------------------------------- */
/* GLOM (aggregated RX) deaggregation                                       */

#define BWFM_GLOM_MAXSUB        16
#define BWFM_GLOM_BUFSZ         (BWFM_GLOM_MAXSUB * 2048)   /* 32 KB: hold a full
                                * data glom (was 8 KB - too small once rxglom is
                                * on, dropped sub-frames past ~5 full data frames) */

/* Sub-frames split out of the last GLOM superframe, awaiting delivery. Only the
 * pump's BWFMRxFrame touches these, serialized by bwfm_Sem. */
static UBYTE bwfm_glom_buf[BWFM_GLOM_BUFSZ];
static struct
{
    UWORD   ehoff;          /* 802.3 frame offset within bwfm_glom_buf */
    UWORD   ethlen;
    UBYTE   chan;
} bwfm_glom_q[BWFM_GLOM_MAXSUB];
static int bwfm_glom_n, bwfm_glom_i;

/*
 * Deaggregate a GLOM superframe. `desc` is the just-read GLOM-channel descriptor
 * frame; right after its 12-byte SDPCM header it carries an array of uint16
 * sub-frame lengths (flen bytes => nsub = flen/2). The nsub sub-frames follow
 * back-to-back on F2; the first carries an extra 12-byte superframe header, the
 * rest are plain SDPCM frames. Each EVENT/DATA sub-frame is parsed to its 802.3
 * frame and queued in bwfm_glom_q for BWFMRxFrame to return one at a time.
 *
 * CRITICAL: every announced sub-frame MUST be read off F2, in order, with its
 * announced length - even one we cannot buffer or do not want - or the F2 FIFO
 * desyncs and every later read returns garbage (RX dies; e.g. a bulk transfer
 * stalls partway). So surplus/oversize sub-frames are drained into a throwaway
 * and dropped (a dropped frame is recoverable via TCP; a desync is not).
 * Ported from OpenBSD bwfm_sdio_rx_glom(). Caller holds bwfm_Sem.
 */
static void bwfm_rx_glom(struct BWFMBase *BWFMBase, UBYTE *desc, ULONG flen)
{
    static UBYTE discard[2048];
    ULONG nsub = flen / 2, i, off = 0, dropped = 0;
    int bail = 0;

    bwfm_glom_n = bwfm_glom_i = 0;
    if (nsub == 0)
        return;

    if (BWFMBase->bwfm_evlisten)
        D(bug("[bwfm] glom: %u sub-frame(s)\n", (unsigned)nsub));

    for (i = 0; i < nsub; i++)
    {
        UBYTE *sp = desc + 12 + i * 2;          /* byte-wise: avoid an unaligned */
        ULONG sl = (ULONG)(sp[0] | (sp[1] << 8));    /* 16-bit load (gcc->memcpy) */
        ULONG rl = (sl + 3) & ~3UL;
        ULONG subtot, subflen, ehoff, ethlen;
        struct bwfm_sdio_hwhdr *hw;
        struct bwfm_sdio_swhdr *sw;
        UBYTE *sub;
        UWORD frmlen, cksum;
        int ch, keep;

        if (rl == 0 || rl > sizeof(discard))    /* nonsense length: cannot resync */
        {
            bail = 1;
            break;
        }

        /* Keep it in the glom buffer only if it fits alongside what is already
         * queued AND the queue has room; otherwise drain it off F2 and drop. */
        keep = (off + rl <= sizeof(bwfm_glom_buf)) && (bwfm_glom_n < BWFM_GLOM_MAXSUB);
        if (bwfm_frame_rw(BWFMBase, keep ? (bwfm_glom_buf + off) : discard, rl, 0))
        {
            bail = 1;
            break;
        }
        if (!keep)
        {
            dropped++;
            continue;
        }

        sub = bwfm_glom_buf + off;
        subtot = sl;

        /* The first sub-frame is prefixed with a 12-byte superframe header. */
        if (i == 0)
        {
            sub += 12;
            subtot -= 12;
        }
        if (subtot < 12)
            continue;

        hw = (struct bwfm_sdio_hwhdr *)sub;
        sw = (struct bwfm_sdio_swhdr *)(sub + 4);
        frmlen = AROS_LE2WORD(hw->frmlen);
        cksum = AROS_LE2WORD(hw->cksum);
        if ((UWORD)(frmlen ^ cksum) != 0xffff || frmlen < 12)
        {
            if (BWFMBase->bwfm_evlisten)
                D(bug("[bwfm] glom sub %u bad hdr (frmlen %u)\n", (unsigned)i, frmlen));
            continue;
        }
        BWFMBase->bwfm_txmax = sw->maxseqnr;     /* refresh the tx-credit window
                                                  * from EVERY glommed sub-frame,
                                                  * not just the descriptor - under a
                                                  * bulk RX glom the descriptor's
                                                  * maxseqnr lags and TX stalls at
                                                  * seq==max (matches OpenBSD) */
        ch = sw->chanflag & BWFM_SDIO_SWHDR_CHANNEL_MASK;
        subflen = frmlen - 12;
        if (BWFMBase->bwfm_evlisten)
            D(bug("[bwfm] glom sub %u chan %d flen %u\n", (unsigned)i, ch, (unsigned)subflen));
        if (ch != BWFM_SDIO_SWHDR_CHANNEL_EVENT && ch != BWFM_SDIO_SWHDR_CHANNEL_DATA)
            continue;
        if (12 + subflen > subtot)
            continue;
        if (!bwfm_frame_eth(sub, subflen, &ehoff, &ethlen))
            continue;

        bwfm_glom_q[bwfm_glom_n].ehoff = (UWORD)((sub - bwfm_glom_buf) + ehoff);
        bwfm_glom_q[bwfm_glom_n].ethlen = (UWORD)ethlen;
        bwfm_glom_q[bwfm_glom_n].chan = (UBYTE)ch;
        bwfm_glom_n++;
        off += rl;          /* keep this sub-frame; next reads land after it */
    }

    /* Rare-path diagnostic (un-gated): a glom we could not fully buffer, or an
     * F2 read failure mid-superframe, is the prime suspect for a bulk-transfer
     * stall - surface it. bail => F2 is likely now desynced (read nothing more). */
    if (bail || dropped)
        D(bug("[bwfm] glom: %u subs, %d queued, %u dropped%s\n", (unsigned)nsub,
              bwfm_glom_n, (unsigned)dropped, bail ? ", BAILED (F2 desync risk)" : ""));
}

/* Pop the next deaggregated glom sub-frame into buf; returns its length (0 if
 * the queue is empty). Caller holds bwfm_Sem. */
static int bwfm_glom_pop(UBYTE *buf, ULONG bufsize, uint32_t *info)
{
    ULONG ehoff, ethlen;
    int chan;

    if (bwfm_glom_i >= bwfm_glom_n)
        return 0;
    ehoff = bwfm_glom_q[bwfm_glom_i].ehoff;
    ethlen = bwfm_glom_q[bwfm_glom_i].ethlen;
    chan = bwfm_glom_q[bwfm_glom_i].chan;
    bwfm_glom_i++;
    if (ethlen > bufsize)
        ethlen = bufsize;
    CopyMem(bwfm_glom_buf + ehoff, buf, ethlen);
    if (info)
        *info = bwfm_rx_info(bwfm_glom_buf + ehoff, ethlen, chan);
    return (int)ethlen;
}

/*
 * Deaggregate a GLOM superframe that landed inside a synchronous bwfm_dcmd
 * (i.e. while polling for a control reply) and salvage its firmware events to
 * the hand-off ring. bwfm_rx_glom reads the sub-frames off F2 - the essential
 * part, so the control read does not desync - then we push any event (LINK_CTL)
 * sub-frame to the ring (bwfm_event_push is a no-op when nobody is listening).
 * Data sub-frames are dropped: the datapath is the pump's job, and the unit is
 * not online during the control ops (join) where this fires. Caller holds
 * bwfm_Sem. Shares the global glom queue with the pump, which is safe because
 * the pump blocks on bwfm_Sem while we hold it.
 */
static void bwfm_glom_salvage(struct BWFMBase *BWFMBase, UBYTE *desc, ULONG flen)
{
    int i;

    bwfm_rx_glom(BWFMBase, desc, flen);         /* fills bwfm_glom_q from F2 */

    for (i = 0; i < bwfm_glom_n; i++)
    {
        UBYTE *eth = bwfm_glom_buf + bwfm_glom_q[i].ehoff;
        struct bwfm_event *e = (struct bwfm_event *)eth;
        ULONG ethlen = bwfm_glom_q[i].ethlen;

        if (ethlen >= sizeof(*e) &&
            AROS_BE2WORD(e->ehdr.ether_type) == BWFM_ETHERTYPE_LINK_CTL)
            bwfm_event_push(BWFMBase, eth, ethlen);
    }
    bwfm_glom_n = bwfm_glom_i = 0;              /* consumed here, not by the pump */
}

/* ----------------------------------------------------------------------- */
/* Firmware-event hand-off ring (single FIFO reader = the device RX pump)    */

/*
 * Enable or disable event capture for a control op (join/scan). Enabling
 * discards any events left over from a previous op. Safe to call while
 * already holding bwfm_Sem - the semaphore nests.
 */
static void bwfm_event_listen(struct BWFMBase *BWFMBase, int on)
{
    ObtainSemaphore(&BWFMBase->bwfm_Sem);
    BWFMBase->bwfm_evlisten = on;
    if (on)
        BWFMBase->bwfm_evtail = BWFMBase->bwfm_evhead;
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
}

/* Pop the oldest queued event frame into buf; returns its length (0 if none). */
static ULONG bwfm_event_pop(struct BWFMBase *BWFMBase, UBYTE *buf, ULONG bufsize)
{
    ULONG n = 0;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);
    if (BWFMBase->bwfm_evhead != BWFMBase->bwfm_evtail)
    {
        struct bwfm_evslot *s =
            &BWFMBase->bwfm_evring[BWFMBase->bwfm_evtail % BWFM_EVRING_SLOTS];

        n = s->ev_len;
        if (n > bufsize)
            n = bufsize;
        CopyMem(s->ev_data, buf, n);
        BWFMBase->bwfm_evtail++;
    }
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return n;
}

/*
 * A timer.device handle a control op uses to sleep (and bound its wait) while
 * the RX pump runs. Opened on demand at runtime - never at kickstart init.
 */
struct bwfm_timer
{
    struct MsgPort     *bt_port;
    struct timerequest *bt_req;
    int                 bt_ok;
};

static void bwfm_timer_open(struct bwfm_timer *t)
{
    t->bt_ok = 0;
    t->bt_req = NULL;
    t->bt_port = CreateMsgPort();
    if (t->bt_port == NULL)
        return;
    t->bt_req = (struct timerequest *)CreateIORequest(t->bt_port,
                                                      sizeof(struct timerequest));
    if (t->bt_req != NULL &&
        OpenDevice((CONST_STRPTR)"timer.device", UNIT_MICROHZ,
                   (struct IORequest *)t->bt_req, 0) == 0)
        t->bt_ok = 1;
}

static void bwfm_timer_close(struct bwfm_timer *t)
{
    if (t->bt_ok)
        CloseDevice((struct IORequest *)t->bt_req);
    if (t->bt_req)
        DeleteIORequest((struct IORequest *)t->bt_req);
    if (t->bt_port)
        DeleteMsgPort(t->bt_port);
}

/*
 * Sleep ~micros, yielding the CPU so the RX pump can read the FIFO and post
 * events. Falls back to a busy-wait only if the timer could not be opened
 * (which would re-starve the pump, but should never happen post-DOS).
 */
static void bwfm_timer_wait(struct BWFMBase *BWFMBase, struct bwfm_timer *t, ULONG micros)
{
    if (!t->bt_ok)
    {
        bwfm_udelay(BWFMBase, micros);
        return;
    }
    t->bt_req->tr_node.io_Command = TR_ADDREQUEST;
    t->bt_req->tr_time.tv_secs = micros / 1000000;
    t->bt_req->tr_time.tv_micro = micros % 1000000;
    DoIO((struct IORequest *)t->bt_req);
}

/* ----------------------------------------------------------------------- */
/* Resource init                                                           */

static int bwfm_init(struct BWFMBase *BWFMBase)
{
    D(bug("[bwfm] %s()\n", __PRETTY_FUNCTION__));

    InitSemaphore(&BWFMBase->bwfm_Sem);
    BWFMBase->bwfm_bar0 = ~0u;

    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
        BWFMBase->bwfm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    SDIOBase = OpenResource("sdio.resource");
    if (!SDIOBase)
        D(bug("[bwfm] sdio.resource not available\n"));

    /* Chip bring-up (power + enumeration) is deferred to the first BWFMAttach(),
     * which bwfm.device calls when it is opened - so the WiFi chip stays
     * unpowered until something actually uses it. */
    return TRUE;
}

ADD2INITLIB(bwfm_init, 0)

/* ----------------------------------------------------------------------- */
/* Public API                                                              */

AROS_LH0(int, BWFMAttach,
                struct BWFMBase *, BWFMBase, 1, Bwfm)
{
    AROS_LIBFUNC_INIT

    int ok;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);
    ok = BWFMBase->bwfm_attached ? TRUE : bwfm_do_attach(BWFMBase);
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);

    return ok;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(uint32_t, BWFMChipID,
                struct BWFMBase *, BWFMBase, 2, Bwfm)
{
    AROS_LIBFUNC_INIT

    return BWFMBase->bwfm_attached ? BWFMBase->bwfm_chip : 0;

    AROS_LIBFUNC_EXIT
}

/*
 * Load firmware + nvram into chip RAM and start the CPU. The caller
 * (bwfm.device) supplies the blobs read from disk; nvram must already be
 * in the converted binary form (with its trailing size token). Returns 0
 * once the firmware signals ready. The upload is NOT read back for verification:
 * large block-mode reads of the backplane RAM are unreliable on this chip (the
 * nvram region's non-64-aligned address returned a data CRC), while the writes
 * themselves are reliable.
 */
AROS_LH4(int, BWFMStartFirmware,
                AROS_LHA(APTR, fw, A0),
                AROS_LHA(uint32_t, fwlen, D0),
                AROS_LHA(APTR, nvram, A1),
                AROS_LHA(uint32_t, nvlen, D1),
                struct BWFMBase *, BWFMBase, 3, Bwfm)
{
    AROS_LIBFUNC_INIT

    int err = -1, i;
    uint32_t rstvec;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    if (!BWFMBase->bwfm_attached || fw == NULL || fwlen < 4)
        goto done;

    /* Guard the RAM layout: a zero ramsize (unsized chip) or an over-large
     * firmware would make the nvram address underflow (rambase+ramsize-nvlen)
     * and scribble a wild backplane window. rambase==0 is valid (SOCRAM). */
    if (BWFMBase->bwfm_ramsize == 0 || fwlen > BWFMBase->bwfm_ramsize)
    {
        D(bug("[bwfm] bad RAM layout (size %u, fw %u) - refusing upload\n",
              BWFMBase->bwfm_ramsize, fwlen));
        goto done;
    }

    if (bwfm_alpclk_on(BWFMBase))   /* upload on ALP; HT forced after FWREADY */
        goto done;

    rstvec = *(uint32_t *)fw;       /* reset vector = first firmware word */

    D(bug("[bwfm] uploading firmware (%u bytes) to RAM 0x%06x\n",
          fwlen, BWFMBase->bwfm_rambase));
    if (bwfm_ram_write(BWFMBase, BWFMBase->bwfm_rambase, fw, fwlen))
        goto done;

    if (nvram && nvlen)
    {
        uint32_t nvaddr;

        if (nvlen >= BWFMBase->bwfm_ramsize)
        {
            D(bug("[bwfm] nvram (%u bytes) too large for RAM (%u)\n",
                  nvlen, BWFMBase->bwfm_ramsize));
            goto done;
        }
        nvaddr = BWFMBase->bwfm_rambase + BWFMBase->bwfm_ramsize - nvlen;
        D(bug("[bwfm] uploading nvram (%u bytes) to RAM 0x%06x\n", nvlen, nvaddr));
        if (bwfm_ram_write(BWFMBase, nvaddr, nvram, nvlen))
            goto done;
    }

    if (bwfm_set_active(BWFMBase, rstvec))
    {
        D(bug("[bwfm] failed to start CPU core\n"));
        goto done;
    }

    for (i = 0; i < 2000; i++)
    {
        uint32_t mb = bwfm_dev_read(BWFMBase, SDPCMD_TOHOSTMAILBOXDATA);
        if (mb & (SDPCMD_TOHOSTMAILBOXDATA_FWREADY | SDPCMD_TOHOSTMAILBOXDATA_DEVREADY))
        {
            bwfm_dev_write(BWFMBase, SDPCMD_TOSBMAILBOX, SDPCMD_TOSBMAILBOX_INT_ACK);
            err = 0;
            break;
        }
        bwfm_udelay(BWFMBase, 1000);
    }

    D(bug("[bwfm] firmware %s\n", err ? "ready timeout" : "ready"));

    if (!err)
    {
        /* Post-firmware bus bring-up: force HT, advertise the SDPCM
         * protocol version, enable the data function and set up the
         * host interrupt mask + FIFO watermark. */
        uint8_t clk = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);
        bwfm_write_1(BWFM_SDIO_FUNC1_CHIPCLKCSR,
                     clk | BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HT);
        bwfm_dev_write(BWFMBase, SDPCMD_TOSBMAILBOXDATA,
                       SDPCM_PROT_VERSION << SDPCM_PROT_VERSION_SHIFT);
        SDIOEnableFunction(2);
        bwfm_dev_write(BWFMBase, SDPCMD_HOSTINTMASK,
                       SDPCMD_INTSTATUS_HMB_SW_MASK | SDPCMD_INTSTATUS_CHIPACTIVE);
        bwfm_write_1(BWFM_SDIO_WATERMARK, 8);
        BWFMBase->bwfm_txseq = 0xff;     /* matches OpenBSD; firmware grants */
        BWFMBase->bwfm_txmax = 1;        /* credit from RX maxseqnr; seed 1 */
        BWFMBase->bwfm_reqid = 0;

        /* Keep RX frame glomming OFF. Turning it on (rxglom=1) broke the
         * synchronous control channel on this firmware: every dcmd right after
         * firmware-ready (GetMAC, clmload, C_UP, join) timed out -1. With glom on
         * the firmware also aggregates the BCDC control *responses* onto the GLOM
         * channel (chan 3), and our deaggregation drops control sub-frames
         * (bwfm_rx_glom keeps only EVENT/DATA), so the polled dcmd never sees its
         * reply. rxglom=0 keeps control replies on their own channel. The firmware
         * still glomms EVENT/DATA under load regardless - that is handled by the
         * pump (bwfm_rx_glom) and by bwfm_dcmd's glom drain, so those paths stay
         * robust; we just must not ask for glom on the control path. */
        bwfm_iovar_set_int(BWFMBase, "bus:rxglom", 0);
    }

done:
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Read the chip's current MAC address via the BCDC "cur_etheraddr" iovar.
 * Valid only after BWFMStartFirmware has succeeded. mac must hold 6 bytes.
 */
AROS_LH1(int, BWFMGetMAC,
                AROS_LHA(UBYTE *, mac, A0),
                struct BWFMBase *, BWFMBase, 4, Bwfm)
{
    AROS_LIBFUNC_INIT

    static const char var[] = "cur_etheraddr";
    UBYTE buf[16];
    int err, i;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    for (i = 0; var[i]; i++)
        buf[i] = var[i];
    buf[i++] = '\0';            /* name + NUL = 14 bytes (>= 6 result bytes) */

    err = bwfm_dcmd(BWFMBase, BWFM_C_GET_VAR, 0, buf, i);
    if (!err)
        CopyMem(buf, mac, 6);

    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Raw firmware ioctl. For GET, the response is copied back into buf; for
 * SET, buf holds the argument (may be NULL/0). Returns 0 / firmware status.
 */
AROS_LH4(int, BWFMIoctl,
                AROS_LHA(uint32_t, cmd, D0),
                AROS_LHA(int, set, D1),
                AROS_LHA(void *, buf, A0),
                AROS_LHA(uint32_t, len, D2),
                struct BWFMBase *, BWFMBase, 5, Bwfm)
{
    AROS_LIBFUNC_INIT

    int err;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);
    err = bwfm_dcmd(BWFMBase, cmd, set, buf, len);
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Named firmware variable (iovar) access. Encodes "name\0" + payload and
 * issues GET_VAR/SET_VAR. For GET, the value is copied back into buf.
 */
AROS_LH4(int, BWFMIovar,
                AROS_LHA(uint8_t *, name, A0),
                AROS_LHA(int, set, D0),
                AROS_LHA(void *, buf, A1),
                AROS_LHA(uint32_t, len, D1),
                struct BWFMBase *, BWFMBase, 6, Bwfm)
{
    UBYTE tmp[256];
    ULONG nlen = 0;
    int err;

    AROS_LIBFUNC_INIT

    while (name[nlen])
        nlen++;
    if (nlen + 1 + len > sizeof(tmp))
        return -1;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    CopyMem(name, tmp, nlen + 1);               /* name + NUL */
    if (len)
        CopyMem(buf, tmp + nlen + 1, len);

    err = bwfm_dcmd(BWFMBase, set ? BWFM_C_SET_VAR : BWFM_C_GET_VAR, set,
                    tmp, nlen + 1 + len);
    if (!err && !set && len)
        CopyMem(tmp, buf, len);

    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Per-result beacon IEs captured by the last BWFMScan, fetched afterwards via
 * BWFMScanIE() (parallel to the scanresult array). Kept out of struct
 * bwfm_scanresult so the device/test don't have to mirror a variable-size blob.
 */
#define BWFM_SCAN_IE_MAX        32      /* results we keep IEs for */
#define BWFM_SCAN_IE_SIZE       256     /* IE bytes captured per result */
static UBYTE bwfm_scan_ie[BWFM_SCAN_IE_MAX][BWFM_SCAN_IE_SIZE];
static UWORD bwfm_scan_ie_len[BWFM_SCAN_IE_MAX];

/*
 * Run a broadcast escan and collect results. `outp` is an array of
 * struct bwfm_scanresult (max entries); returns the number filled. Triggers
 * an escan, then polls the SDPCM event channel for ESCAN_RESULT events,
 * parsing each bss_info, until the firmware reports scan complete or ~5s pass.
 */
AROS_LH2(int, BWFMScan,
                AROS_LHA(void *, outp, A0),
                AROS_LHA(int, max, D0),
                struct BWFMBase *, BWFMBase, 7, Bwfm)
{
    AROS_LIBFUNC_INIT

    static UBYTE frame[2048];
    struct bwfm_scanresult *out = (struct bwfm_scanresult *)outp;
    struct bwfm_escan_params_v0 p;
    struct bwfm_timer tmr;
    UBYTE evmask[BWFM_EVENT_MASK_LEN];
    int count = 0, done = 0, tries, i;

    if (!BWFMBase->bwfm_attached || out == NULL || max <= 0)
        return 0;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    /*
     * The escan request layout depends on the firmware's scan version.
     * OpenBSD queries "scan_ver" (default 0 = v0 if the iovar is absent).
     * We only build v0 params; warn if the firmware expects a newer version
     * so a wrong-layout escan is diagnosable rather than silently failing.
     */
    {
        uint32_t scan_ver = 0;
        if (bwfm_iovar_get(BWFMBase, "scan_ver", &scan_ver, sizeof(scan_ver)) == 0)
        {
            scan_ver = AROS_LE2LONG(scan_ver);
            D(bug("[bwfm] scan_ver = %u\n", scan_ver));
            if (scan_ver != 0)
                D(bug("[bwfm] WARNING: firmware escan v%u, we send v0\n", scan_ver));
        }
        else
            D(bug("[bwfm] scan_ver iovar absent (escan v0)\n"));
    }

    /* Enable the events we care about (STA mode set) */
    bwfm_zero(evmask, sizeof(evmask));
#define EV(e) (evmask[(e) / 8] |= (1 << ((e) % 8)))
    EV(BWFM_E_IF); EV(BWFM_E_LINK); EV(BWFM_E_AUTH); EV(BWFM_E_ASSOC);
    EV(BWFM_E_EAPOL_MSG); EV(BWFM_E_DEAUTH); EV(BWFM_E_DISASSOC);
    EV(BWFM_E_ESCAN_RESULT);
#undef EV
    bwfm_iovar_set(BWFMBase, "event_msgs", evmask, sizeof(evmask));

    /*
     * Bring the STA interface up the way the reference does before scanning.
     * A bare C_UP returns success but escan still fails with NOTUP (-4): the
     * firmware needs the operating mode configured (infra STA, not AP) and the
     * scan timing / power-management set first. Mirror OpenBSD bwfm_init()'s
     * order: scan times, PM, C_UP, then INFRA/AP. mpc=0 keeps the radio awake.
     */
    {
        int e;
        bwfm_set_int(BWFMBase, BWFM_C_SET_SCAN_CHANNEL_TIME, 40);
        bwfm_set_int(BWFMBase, BWFM_C_SET_SCAN_UNASSOC_TIME, 40);
        bwfm_set_int(BWFMBase, BWFM_C_SET_SCAN_PASSIVE_TIME, 120);
        bwfm_set_int(BWFMBase, BWFM_C_SET_PM, BWFM_PM_FAST_PS);
        {
            uint32_t mpc = 0;
            bwfm_iovar_set(BWFMBase, "mpc", &mpc, sizeof(mpc));
        }
        e = bwfm_dcmd(BWFMBase, BWFM_C_UP, 1, NULL, 0);
        bwfm_set_int(BWFMBase, BWFM_C_SET_INFRA, 1);
        bwfm_set_int(BWFMBase, BWFM_C_SET_AP, 0);
        bwfm_udelay(BWFMBase, 50000);
        D(bug("[bwfm] pre-scan: interface up (C_UP err %d)\n", e));
    }

    /* Broadcast passive escan, all channels */
    bwfm_zero((UBYTE *)&p, sizeof(p));
    for (i = 0; i < ETHER_ADDR_LEN; i++)
        p.scan_params.bssid[i] = 0xff;
    p.scan_params.bss_type = DOT11_BSSTYPE_ANY;
    p.scan_params.scan_type = BWFM_SCANTYPE_PASSIVE;
    p.scan_params.nprobes = AROS_LONG2LE(0xffffffff);
    p.scan_params.active_time = AROS_LONG2LE(0xffffffff);
    p.scan_params.passive_time = AROS_LONG2LE(0xffffffff);
    p.scan_params.home_time = AROS_LONG2LE(0xffffffff);
    p.scan_params.channel_num = 0;
    p.version = AROS_LONG2LE(BWFM_ESCAN_REQ_VERSION);
    p.action = AROS_WORD2LE(WL_ESCAN_ACTION_START);
    p.sync_id = AROS_WORD2LE(0x1234);

    /* Match OpenBSD's size exactly: the trailing channel_list[] is empty
     * for an all-channel scan (channel_num == 0), so exclude it. */
    {
        ULONG psize = sizeof(p) - sizeof(p.scan_params.channel_list);
        int err;

        /* Collect events from here on, then release the bus: the RX pump (the
         * single FIFO reader) reads the escan results and posts them to us. */
        bwfm_event_listen(BWFMBase, 1);
        err = bwfm_iovar_set(BWFMBase, "escan", &p, psize);
        ReleaseSemaphore(&BWFMBase->bwfm_Sem);
        if (err)
        {
            D(bug("[bwfm] escan trigger failed (err %d, size %u)\n", err, psize));
            bwfm_event_listen(BWFMBase, 0);
            return 0;
        }
    }

    bwfm_timer_open(&tmr);

    /* ~5s window: 100 ticks of 50ms, yielding so the pump keeps reading. */
    for (tries = 0; tries < 100 && !done; tries++)
    {
        ULONG n;

        while ((n = bwfm_event_pop(BWFMBase, frame, sizeof(frame))) > 0)
        {
            struct bwfm_event *e = (struct bwfm_event *)frame;
            struct bwfm_escan_results *res;
            ULONG evlen, status, remain;
            UBYTE *bp;
            UWORD bcount;

            if (n < sizeof(*e))
                continue;
            evlen = n - sizeof(*e);
            if (AROS_BE2LONG(e->msg.event_type) != BWFM_E_ESCAN_RESULT)
                continue;

            status = AROS_BE2LONG(e->msg.status);
            if (status == BWFM_E_STATUS_SUCCESS)
            {
                done = 1;
                continue;
            }
            if (status != BWFM_E_STATUS_PARTIAL || evlen < 12)
                continue;

            /* escan_results header is 12 bytes; bss_info[] follows */
            res = (struct bwfm_escan_results *)(e + 1);
            bcount = AROS_LE2WORD(res->bss_count);
            bp = (UBYTE *)&res->bss_info[0];
            remain = evlen - 12;

            for (i = 0; i < bcount; i++)
            {
                struct bwfm_bss_info *bi = (struct bwfm_bss_info *)bp;
                ULONG bilen, sl;
                int16_t rssi;
                int j, dup = -1;

                if (remain < sizeof(struct bwfm_bss_info))
                    break;
                bilen = AROS_LE2LONG(bi->length);
                if (bilen < sizeof(struct bwfm_bss_info) || bilen > remain)
                    break;

                rssi = (int16_t)AROS_LE2WORD(bi->rssi);
                sl = bi->ssid_len;
                if (sl > BWFM_MAX_SSID_LEN)
                    sl = BWFM_MAX_SSID_LEN;

                /*
                 * Dedupe by BSSID: escan emits a PARTIAL event per beacon
                 * heard, so the same BSS recurs across the scan window. Keep
                 * one entry per BSSID with the strongest RSSI; fill in the
                 * SSID if an earlier (hidden) sighting had none.
                 */
                for (j = 0; j < count; j++)
                {
                    UBYTE *a = out[j].bssid;
                    UBYTE *b = bi->bssid;

                    if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2] &&
                        a[3] == b[3] && a[4] == b[4] && a[5] == b[5])
                    {
                        dup = j;
                        break;
                    }
                }

                if (dup >= 0)
                {
                    if (rssi > out[dup].rssi)
                        out[dup].rssi = rssi;
                    if (out[dup].ssid_len == 0 && sl > 0)
                    {
                        out[dup].ssid_len = sl;
                        CopyMem(bi->ssid, out[dup].ssid, sl);
                    }
                    out[dup].chanspec = AROS_LE2WORD(bi->chanspec);
                }
                else if (count < max)
                {
                    CopyMem(bi->bssid, out[count].bssid, ETHER_ADDR_LEN);
                    out[count].ssid_len = sl;
                    CopyMem(bi->ssid, out[count].ssid, sl);
                    out[count].rssi = rssi;
                    out[count].chanspec = AROS_LE2WORD(bi->chanspec);

                    /* Capture this BSS's beacon IEs (SSID, rates, RSN/WPA, ...)
                     * so the SANA-II scan can hand them to wpa_supplicant, which
                     * needs the RSN/WPA element to match a WPA2 network. */
                    if (count < BWFM_SCAN_IE_MAX)
                    {
                        ULONG ieoff = AROS_LE2WORD(bi->ie_offset);
                        ULONG ielen = AROS_LE2LONG(bi->ie_length);
                        UBYTE *ies = (UBYTE *)bi + ieoff;

                        bwfm_scan_ie_len[count] = 0;
                        /* Validate the bss_info layout before trusting ie_offset:
                         * the first beacon IE is always the SSID element (id 0,
                         * length == ssid_len). If that doesn't hold, ie_offset is
                         * wrong for this firmware - skip rather than feed
                         * wpa_supplicant garbage (which crashed it). */
                        if (ielen && ieoff + 2 <= bilen && ieoff + ielen <= bilen &&
                            ies[0] == 0 && ies[1] == bi->ssid_len)
                        {
                            if (ielen > BWFM_SCAN_IE_SIZE)
                                ielen = BWFM_SCAN_IE_SIZE;
                            CopyMem(ies, bwfm_scan_ie[count], ielen);
                            bwfm_scan_ie_len[count] = (UWORD)ielen;
                        }
                        else
                            D(bug("[bwfm] scan IE skip: off=%u len=%u first=%02x/%02x"
                                  " ssidlen=%u bilen=%u\n", (unsigned)ieoff,
                                  (unsigned)ielen, ies[0], ies[1],
                                  (unsigned)bi->ssid_len, (unsigned)bilen));
                    }
                    count++;
                }

                bp += bilen;
                remain -= bilen;
            }
        }

        if (!done)
            bwfm_timer_wait(BWFMBase, &tmr, 50000);
    }

    bwfm_timer_close(&tmr);
    bwfm_event_listen(BWFMBase, 0);

    D(bug("[bwfm] scan done: %d result(s)\n", count));
    return count;

    AROS_LIBFUNC_EXIT
}

/*
 * Upload the regulatory CLM (Country Locale Matrix) blob to the running
 * firmware via the "clmload" iovar. The firmware for this chip ships without
 * built-in regulatory data, so without the blob the radio has no valid
 * channels and "wl up" leaves the interface NOTUP (escan fails). The blob is
 * sent in chunks small enough to fit the synchronous BCDC control frame, the
 * first marked BEGIN and the last END. Ported from OpenBSD bwfm_process_blob().
 * A NULL/empty blob is a no-op (firmware falls back to its built-in regulatory).
 */
#define BWFM_CLM_CHUNK  256

AROS_LH2(int, BWFMLoadCLM,
                AROS_LHA(void *, clm, A0),
                AROS_LHA(uint32_t, clmlen, D0),
                struct BWFMBase *, BWFMBase, 8, Bwfm)
{
    AROS_LIBFUNC_INIT

    UBYTE buf[sizeof(struct bwfm_dload_data) + BWFM_CLM_CHUNK];
    struct bwfm_dload_data *d = (struct bwfm_dload_data *)buf;
    UBYTE *src = (UBYTE *)clm;
    ULONG off = 0, remain = clmlen;
    int err = 0;

    if (!BWFMBase->bwfm_attached || clm == NULL || clmlen == 0)
        return 0;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    while (remain)
    {
        ULONG len = (remain < BWFM_CLM_CHUNK) ? remain : BWFM_CLM_CHUNK;
        UWORD flag = BWFM_DLOAD_FLAG_HANDLER_VER_1;

        if (off == 0)
            flag |= BWFM_DLOAD_FLAG_BEGIN;
        if (remain <= BWFM_CLM_CHUNK)
            flag |= BWFM_DLOAD_FLAG_END;

        d->flag = AROS_WORD2LE(flag);
        d->type = AROS_WORD2LE(BWFM_DLOAD_TYPE_CLM);
        d->len = AROS_LONG2LE(len);
        d->crc = 0;
        CopyMem(src + off, buf + sizeof(*d), len);

        err = bwfm_iovar_set(BWFMBase, "clmload", buf, sizeof(*d) + len);
        if (err)
        {
            D(bug("[bwfm] clmload failed at %u/%u (err %d)\n", off, clmlen, err));
            break;
        }

        off += len;
        remain -= len;
    }

    if (!err)
        D(bug("[bwfm] CLM loaded (%u bytes)\n", clmlen));

    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Send one 802.3 data frame to the firmware: wrap it in an SDPCM hw/sw header
 * (DATA channel) plus a BCDC/BDC data header, zero-pad to 4 bytes (or the
 * block size for >512B frames) and write it to the func2 FIFO. Mirrors
 * OpenBSD bwfm_sdio_tx_dataframe(). Called from the device's CMD_WRITE path.
 */
AROS_LH2(int, BWFMTxData,
                AROS_LHA(void *, eth, A0),
                AROS_LHA(uint32_t, ethlen, D0),
                struct BWFMBase *, BWFMBase, 9, Bwfm)
{
    AROS_LIBFUNC_INIT

    static UBYTE frame[2048];
    struct bwfm_sdio_hwhdr *hw = (struct bwfm_sdio_hwhdr *)frame;
    struct bwfm_sdio_swhdr *sw = (struct bwfm_sdio_swhdr *)(frame + 4);
    struct bwfm_bcdc_data_hdr *bcdc = (struct bwfm_bcdc_data_hdr *)(frame + 12);
    ULONG total, padded;
    int err;

    if (!BWFMBase->bwfm_attached || eth == NULL || ethlen == 0)
        return -1;

    total = 12 + sizeof(*bcdc) + ethlen;        /* hw+sw = 12, bcdc = 4 */
    padded = (total > 512 && (total % 512)) ? ((total + 511) & ~511UL)
                                            : ((total + 3) & ~3UL);
    if (padded > sizeof(frame))
        return -1;

    /*
     * Data frames are gated by the firmware's tx-credit window: we may only
     * send while (txmax - txseq) is non-zero with its top bit clear (OpenBSD
     * bwfm_sdio_tx_ok). Control frames are exempt. The window is refreshed from
     * every received frame's maxseqnr, so wait (releasing the bus so the RX
     * pump can run and update it) until a credit appears or we give up.
     */
    {
        int waited = 0;
        struct bwfm_timer tmr;
        int have_timer = 0;

        ObtainSemaphore(&BWFMBase->bwfm_Sem);
        while (1)
        {
            uint8_t space = (uint8_t)(BWFMBase->bwfm_txmax - BWFMBase->bwfm_txseq);

            if (space != 0 && (space & 0x80) == 0)
                break;
            ReleaseSemaphore(&BWFMBase->bwfm_Sem);
            if (++waited > 200)         /* ~200ms - the window should be open */
            {
                D(bug("[bwfm] TxData: no tx credit (seq %u max %u)\n",
                      BWFMBase->bwfm_txseq, BWFMBase->bwfm_txmax));
                if (have_timer)
                    bwfm_timer_close(&tmr);
                return -1;
            }
            /* Yield via timer.device rather than busy-spinning so the RX pump
             * can run and refresh the credit window. The timer is opened lazily
             * - the common path has credit at once and never reaches here. */
            if (!have_timer)
            {
                bwfm_timer_open(&tmr);
                have_timer = 1;
            }
            bwfm_timer_wait(BWFMBase, &tmr, 1000);
            ObtainSemaphore(&BWFMBase->bwfm_Sem);
        }
        if (have_timer)
            bwfm_timer_close(&tmr);
    }

    hw->frmlen = AROS_WORD2LE((UWORD)total);
    hw->cksum = AROS_WORD2LE((UWORD)~total);
    sw->seqnr = BWFMBase->bwfm_txseq++;
    sw->chanflag = BWFM_SDIO_SWHDR_CHANNEL_DATA;
    sw->nextlen = 0;
    sw->dataoff = 12;
    sw->flowctl = 0;
    sw->maxseqnr = 0;
    sw->res0 = 0;

    bcdc->flags = BWFM_BCDC_FLAG_VER(BWFM_BCDC_FLAG_PROTO_VER);
    bcdc->priority = 0;
    bcdc->flags2 = 0;
    bcdc->data_offset = 0;

    CopyMem(eth, frame + 12 + sizeof(*bcdc), ethlen);
    bwfm_zero(frame + total, padded - total);

    err = bwfm_frame_rw(BWFMBase, frame, padded, 1);

    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return err;

    AROS_LIBFUNC_EXIT
}

/*
 * Read one pending SDPCM frame for the device's RX pump. Copies the 802.3
 * ethernet frame into `buf` and returns its length (0 if nothing pending).
 * *info gets the SDPCM channel in the low byte plus BWFM_RX_EVENT if the
 * frame is a firmware event (LINK_CTL ethertype) rather than network data.
 */
AROS_LH3(int, BWFMRxFrame,
                AROS_LHA(void *, buf, A0),
                AROS_LHA(uint32_t, bufsize, D0),
                AROS_LHA(uint32_t *, info, A1),
                struct BWFMBase *, BWFMBase, 10, Bwfm)
{
    AROS_LIBFUNC_INIT

    static UBYTE frame[2048];
    ULONG flen = 0, ehoff = 0, ethlen = 0;
    int chan = 0, ret = 0;

    if (!BWFMBase->bwfm_attached)
        return 0;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    /* Deliver any queued GLOM sub-frames before pulling more off F2. */
    ret = bwfm_glom_pop(buf, bufsize, info);
    if (ret == 0 && bwfm_rx_read(BWFMBase, frame, sizeof(frame), &flen, &chan))
    {
        if (BWFMBase->bwfm_evlisten)
            D(bug("[bwfm] rx frame chan %d len %u\n", chan, (unsigned)flen));

        if (chan == BWFM_SDIO_SWHDR_CHANNEL_GLOM)
        {
            bwfm_rx_glom(BWFMBase, frame, flen);
            ret = bwfm_glom_pop(buf, bufsize, info);
        }
        else if (chan == BWFM_SDIO_SWHDR_CHANNEL_EVENT ||
                 chan == BWFM_SDIO_SWHDR_CHANNEL_DATA)
        {
            if (bwfm_frame_eth(frame, flen, &ehoff, &ethlen))
            {
                if (ethlen > bufsize)
                    ethlen = bufsize;
                CopyMem(frame + ehoff, buf, ethlen);
                if (info)
                    *info = bwfm_rx_info(frame + ehoff, ethlen, chan);
                ret = (int)ethlen;
            }
        }
        /* CONTROL or malformed: nothing to deliver */
    }

    ReleaseSemaphore(&BWFMBase->bwfm_Sem);
    return ret;

    AROS_LIBFUNC_EXIT
}

/*
 * Associate to an access point by SSID. With no passphrase this is an OPEN
 * join; with one, WPA2-PSK is configured and the passphrase is handed to the
 * firmware's internal supplicant (sup_wpa=1), which runs the 4-way handshake
 * for us - we have no host supplicant (OpenBSD relies on net80211 for this and
 * keeps sup_wpa=0; we do the opposite). Sets the security iovars + the "join"
 * iovar (ext_join_params, any BSSID/any channel), then polls the event channel
 * for the result. Returns 0 when linked, <0 on failure/timeout. Caller (the
 * device) must NOT hold an open CMD_READ on the bus; the RX pump waits on
 * bwfm_Sem while we run.
 */
AROS_LH4(int, BWFMJoin,
                AROS_LHA(uint8_t *, ssid, A0),
                AROS_LHA(uint32_t, ssidlen, D0),
                AROS_LHA(uint8_t *, pass, A1),
                AROS_LHA(uint32_t, passlen, D1),
                struct BWFMBase *, BWFMBase, 11, Bwfm)
{
    AROS_LIBFUNC_INIT

    static UBYTE frame[2048];
    struct bwfm_ext_join_params jp;
    struct bwfm_timer tmr;
    UBYTE evmask[BWFM_EVENT_MASK_LEN];
    ULONG psize;
    int i, tries, attempt, result = -1;

    if (!BWFMBase->bwfm_attached || ssid == NULL || ssidlen == 0 ||
        ssidlen > BWFM_MAX_SSID_LEN)
        return -1;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    /*
     * Enable the association events we wait on. The firmware's default event
     * mask delivers E_IF but NOT the assoc events (E_LINK/E_SET_SSID/E_AUTH/...),
     * so without this the join would never see a link-up event and always time
     * out. (BWFMScan sets the same mask; join must not depend on a prior scan
     * having run.) Mirrors OpenBSD's pre-join event_msgs setup.
     */
    bwfm_zero(evmask, sizeof(evmask));
#define EV(e) (evmask[(e) / 8] |= (1 << ((e) % 8)))
    EV(BWFM_E_IF); EV(BWFM_E_LINK); EV(BWFM_E_AUTH); EV(BWFM_E_ASSOC);
    EV(BWFM_E_EAPOL_MSG); EV(BWFM_E_DEAUTH); EV(BWFM_E_DISASSOC);
    EV(BWFM_E_SET_SSID);
#undef EV
    bwfm_iovar_set(BWFMBase, "event_msgs", evmask, sizeof(evmask));

    /*
     * Put the interface in infrastructure-STA mode and keep the radio powered.
     * BWFMScan sets these as part of its pre-scan bwfm_init() sequence; doing
     * them here makes a join self-contained so the datapath works without a
     * prior scan. mpc=0 is the key one: with the default (mpc on) the firmware
     * powers the MAC down when briefly idle and drops unsolicited RX like the
     * DHCP offer, so association succeeds but no IP ever arrives.
     */
    bwfm_set_int(BWFMBase, BWFM_C_SET_INFRA, 1);
    bwfm_set_int(BWFMBase, BWFM_C_SET_AP, 0);
    bwfm_iovar_set_int(BWFMBase, "mpc", 0);
    bwfm_set_int(BWFMBase, BWFM_C_SET_PM, BWFM_PM_FAST_PS);

    if (passlen > 0)
    {
        struct bwfm_wsec_pmk pmk;

        bwfm_zero((UBYTE *)&pmk, sizeof(pmk));
        if (passlen > sizeof(pmk.key))
            passlen = sizeof(pmk.key);
        pmk.key_len = AROS_WORD2LE((UWORD)passlen);
        pmk.flags = AROS_WORD2LE(BWFM_WSEC_PASSPHRASE);
        CopyMem(pass, pmk.key, passlen);

        bwfm_iovar_set_int(BWFMBase, "sup_wpa", 1);
        bwfm_iovar_set_int(BWFMBase, "wsec", BWFM_WSEC_AES);
        bwfm_iovar_set_int(BWFMBase, "wpa_auth", BWFM_WPA_AUTH_WPA2_PSK);
        bwfm_dcmd(BWFMBase, BWFM_C_SET_WSEC_PMK, 1, &pmk, sizeof(pmk));
    }
    else
    {
        bwfm_iovar_set_int(BWFMBase, "sup_wpa", 0);
        bwfm_iovar_set_int(BWFMBase, "wpa_auth", BWFM_WPA_AUTH_DISABLED);
        bwfm_iovar_set_int(BWFMBase, "wsec", BWFM_WSEC_NONE);
    }
    bwfm_iovar_set_int(BWFMBase, "auth", BWFM_AUTH_OPEN);
    bwfm_iovar_set_int(BWFMBase, "mfp", BWFM_MFP_NONE);

    /* Build the join request: SSID, any BSSID, any channel. */
    bwfm_zero((UBYTE *)&jp, sizeof(jp));
    jp.ssid.len = AROS_LONG2LE(ssidlen);
    CopyMem(ssid, jp.ssid.ssid, ssidlen);
    for (i = 0; i < ETHER_ADDR_LEN; i++)
        jp.assoc.bssid[i] = 0xff;
    jp.assoc.chanspec_num = 0;
    jp.scan.scan_type = 0xff;                   /* default scan */
    jp.scan.nprobes = AROS_LONG2LE(0xffffffff);
    jp.scan.active_time = AROS_LONG2LE(0xffffffff);
    jp.scan.passive_time = AROS_LONG2LE(0xffffffff);
    jp.scan.home_time = AROS_LONG2LE(0xffffffff);

    psize = sizeof(jp) - sizeof(jp.assoc.chanspec_list);

    /* Collect events from here on, then release the bus so the RX pump (the
     * single FIFO reader) can deliver the association events to us. */
    bwfm_event_listen(BWFMBase, 1);
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);

    bwfm_timer_open(&tmr);

    /*
     * Issue the join and wait for the link. The first association attempt
     * often returns E_AUTH timeout (status 2) on this firmware, so retry a
     * couple of times before giving up.
     */
    for (attempt = 0; attempt < 3 && result != 0; attempt++)
    {
        result = -1;

        ObtainSemaphore(&BWFMBase->bwfm_Sem);
        {
            int jerr = bwfm_iovar_set(BWFMBase, "join", &jp, psize);

            if (jerr)
            {
                /* Older firmware: fall back to BWFM_C_SET_SSID with join_params. */
                struct bwfm_join_params jpp;
                int serr;

                bwfm_zero((UBYTE *)&jpp, sizeof(jpp));
                jpp.ssid.len = AROS_LONG2LE(ssidlen);
                CopyMem(ssid, jpp.ssid.ssid, ssidlen);
                for (i = 0; i < ETHER_ADDR_LEN; i++)
                    jpp.assoc.bssid[i] = 0xff;
                serr = bwfm_dcmd(BWFMBase, BWFM_C_SET_SSID, 1, &jpp,
                                 sizeof(jpp) - sizeof(jpp.assoc.chanspec_list));
                D(bug("[bwfm] join iovar err %d, SET_SSID fallback err %d\n", jerr, serr));
            }
            else
                D(bug("[bwfm] join iovar accepted\n"));
        }
        ReleaseSemaphore(&BWFMBase->bwfm_Sem);

        D(bug("[bwfm] join \"%s\" (%s) attempt %d - waiting for link\n", ssid,
              passlen ? "WPA2-PSK" : "open", attempt + 1));

        /* Consume association events from the pump (~10s, 50ms ticks). */
        for (tries = 0; tries < 200 && result < 0; tries++)
        {
            ULONG etype, status, flags, n;

            while ((n = bwfm_event_pop(BWFMBase, frame, sizeof(frame))) > 0)
            {
                struct bwfm_event *e = (struct bwfm_event *)frame;

                if (n < sizeof(*e))
                    continue;

                etype = AROS_BE2LONG(e->msg.event_type);
                status = AROS_BE2LONG(e->msg.status);
                flags = AROS_BE2WORD(e->msg.flags);
                D(bug("[bwfm] join event type %u status %u flags 0x%x\n",
                      etype, status, flags));

                if (etype == BWFM_E_LINK)
                    result = (flags & BWFM_EVENT_FLAG_LINK_UP) ? 0 : -2;
                else if (etype == BWFM_E_SET_SSID)
                {
                    if (status != BWFM_E_STATUS_SUCCESS)
                        result = -3;            /* association rejected */
                    /* success: keep polling for the E_LINK up that follows */
                }
                else if (etype == BWFM_E_DEAUTH || etype == BWFM_E_DISASSOC)
                    result = -4;                /* kicked off (e.g. wrong passphrase) */
            }

            /*
             * The E_LINK up event is sometimes glommed (SDPCM channel 3) and
             * dropped by our one-frame-at-a-time RX path, so confirm the link
             * directly every few ticks: GET_BSSID returns the AP's BSSID once
             * associated. This also makes re-joining an already-connected
             * network succeed immediately (idempotent).
             */
            if (result < 0 && (tries % 4) == 3)
            {
                uint8_t bssid[ETHER_ADDR_LEN];

                bwfm_zero(bssid, sizeof(bssid));
                ObtainSemaphore(&BWFMBase->bwfm_Sem);
                if (bwfm_dcmd(BWFMBase, BWFM_C_GET_BSSID, 0, bssid, sizeof(bssid)) == 0 &&
                    (bssid[0] | bssid[1] | bssid[2] |
                     bssid[3] | bssid[4] | bssid[5]) != 0)
                    result = 0;
                ReleaseSemaphore(&BWFMBase->bwfm_Sem);
            }

            if (result < 0)
                bwfm_timer_wait(BWFMBase, &tmr, 50000);
        }

        D(bug("[bwfm] join attempt %d %s (result %d)\n", attempt + 1,
              result == 0 ? "OK" : "FAILED", result));
    }

    bwfm_timer_close(&tmr);
    bwfm_event_listen(BWFMBase, 0);
    return result;

    AROS_LIBFUNC_EXIT
}

/*
 * Route the SDIO card interrupt to the device's RX pump: `task` is signalled
 * with `sigmask` when the chip has a frame. Thin wrapper over sdio.resource so
 * the device never touches the SDIO layer directly.
 */
AROS_LH2(int, BWFMSetRxSignal,
                AROS_LHA(struct Task *, task, A0),
                AROS_LHA(uint32_t, sigmask, D0),
                struct BWFMBase *, BWFMBase, 12, Bwfm)
{
    AROS_LIBFUNC_INIT

    return SDIOSetInterrupt(task, sigmask);

    AROS_LIBFUNC_EXIT
}

/* Re-arm the card interrupt after the pump has drained pending frames. */
AROS_LH0(void, BWFMAckRx,
                struct BWFMBase *, BWFMBase, 13, Bwfm)
{
    AROS_LIBFUNC_INIT

    SDIOAckInterrupt();

    AROS_LIBFUNC_EXIT
}

/*
 * Clear the chip's SDPCM interrupt status (the func1/card-interrupt source) so
 * DAT1 deasserts. The RX pump calls this before draining + re-arming, so a
 * re-armed card interrupt does not immediately re-fire on a still-asserted
 * line. Mirrors OpenBSD bwfm_sdio_task()'s INTSTATUS read + W1C.
 */
AROS_LH0(void, BWFMClearInt,
                struct BWFMBase *, BWFMBase, 14, Bwfm)
{
    AROS_LIBFUNC_INIT

    uint32_t ist, raw;
    uint8_t clk, devctl;
    int i;

    if (!BWFMBase->bwfm_attached)
        return;

    ObtainSemaphore(&BWFMBase->bwfm_Sem);

    /*
     * Make the chip clock available before touching frames. When the card
     * interrupt is enabled the chip may doze and only wake the host via the
     * interrupt with DEVICE_CTL_CA_INT_ONLY set - in that state it signals
     * HMB_FRAME_IND but does NOT DMA the frame into the F2 FIFO until the host
     * requests HT and clears CA_INT_ONLY. This explains the symptom: FRAME_IND
     * set but F2 empty. Mirrors OpenBSD bwfm_sdio_clkctl(CLK_AVAIL).
     */
    clk = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);
    if (!(clk & BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL))
    {
        bwfm_write_1(BWFM_SDIO_FUNC1_CHIPCLKCSR,
                     clk | BWFM_SDIO_FUNC1_CHIPCLKCSR_FORCE_HT);
        for (i = 0; i < 100; i++)
        {
            clk = bwfm_read_1(BWFM_SDIO_FUNC1_CHIPCLKCSR);
            if (clk & BWFM_SDIO_FUNC1_CHIPCLKCSR_HT_AVAIL)
                break;
            bwfm_udelay(BWFMBase, 100);
        }
    }
    devctl = bwfm_read_1(BWFM_SDIO_DEVICE_CTL);
    if (devctl & BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY)
        bwfm_write_1(BWFM_SDIO_DEVICE_CTL,
                     devctl & ~BWFM_SDIO_DEVICE_CTL_CA_INT_ONLY);

    raw = bwfm_dev_read(BWFMBase, SDPCMD_INTSTATUS);
    ist = raw & (SDPCMD_INTSTATUS_HMB_SW_MASK | SDPCMD_INTSTATUS_CHIPACTIVE);

    /* Diagnostic: dump interrupt + clock state during a join/scan. */
    if (BWFMBase->bwfm_evlisten)
        D(bug("[bwfm] clrint INTSTATUS 0x%08x clk 0x%02x devctl 0x%02x\n",
              raw, clk, devctl));

    /* The host-mailbox interrupt needs the SDPCM handshake before its status
     * bit may be cleared - read the mailbox data and ACK it, or the chip's
     * mailbox protocol wedges and it stops delivering events/frames (this
     * broke join when the pump cleared it bare). Mirrors OpenBSD. */
    if (ist & SDPCMD_INTSTATUS_HMB_HOST_INT)
    {
        bwfm_dev_read(BWFMBase, SDPCMD_TOHOSTMAILBOXDATA);
        bwfm_dev_write(BWFMBase, SDPCMD_TOSBMAILBOX, SDPCMD_TOSBMAILBOX_INT_ACK);
    }

    /* W1C the latched status bits (incl. HMB_FRAME_IND). The pump calls us
     * AFTER draining F2, so clearing the frame-ready bit with the FIFO empty
     * deasserts DAT1 and lets the next frame re-trigger the edge-sensitive
     * card interrupt. (The earlier "clearing halts the firmware" was really the
     * missing event_msgs mask, since fixed - clearing here is correct.) */
    if (ist)
        bwfm_dev_write(BWFMBase, SDPCMD_INTSTATUS, ist);
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);

    AROS_LIBFUNC_EXIT
}

/*
 * Queue a firmware event frame for an in-progress control op (join/scan). The
 * device's RX pump - the single SDPCM FIFO reader - calls this for every event
 * frame it reads. If no control op is listening the frame is dropped (events
 * outside join/scan are not acted on yet). On overflow the oldest entry is
 * dropped so the pump never blocks.
 */
AROS_LH2(void, BWFMEventPost,
                AROS_LHA(void *, ev, A0),
                AROS_LHA(uint32_t, len, D0),
                struct BWFMBase *, BWFMBase, 15, Bwfm)
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&BWFMBase->bwfm_Sem);
    bwfm_event_push(BWFMBase, ev, len);
    ReleaseSemaphore(&BWFMBase->bwfm_Sem);

    AROS_LIBFUNC_EXIT
}

/*
 * Copy the beacon IEs captured for scan result `idx` (by the last BWFMScan)
 * into buf; returns the length (0 if none / out of range). The SANA-II device's
 * S2_GETNETWORKS hands these to wpa_supplicant as S2INFO_InfoElements.
 */
AROS_LH3(int, BWFMScanIE,
                AROS_LHA(int, idx, D0),
                AROS_LHA(void *, buf, A0),
                AROS_LHA(uint32_t, bufsize, D1),
                struct BWFMBase *, BWFMBase, 16, Bwfm)
{
    AROS_LIBFUNC_INIT

    ULONG n;

    if (idx < 0 || idx >= BWFM_SCAN_IE_MAX || buf == NULL)
        return 0;
    n = bwfm_scan_ie_len[idx];
    if (n > bufsize)
        n = bufsize;
    if (n)
        CopyMem(bwfm_scan_ie[idx], buf, n);
    return (int)n;

    AROS_LIBFUNC_EXIT
}
