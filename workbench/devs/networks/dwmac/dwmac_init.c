/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: DesignWare MAC SANA-II driver, initialisation.
*/

#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>

#include <devices/sana2.h>

#include LC_LIBDEFS_FILE

#include "dwmac.h"

#undef UtilityBase

/*
 * Ask the PHY who it is. A bus with nothing on it reads back all ones
 * or all zeroes, which is how an empty address is told from a real one.
 */
static BOOL DWMAC_ProbePHY(struct dwmac_hw *hw)
{
    LONG id1, id2;
    ULONG phy = hw->phyAddr;

    id1 = DWMAC_MDIORead(hw, phy, MII_PHYSID1);
    id2 = DWMAC_MDIORead(hw, phy, MII_PHYSID2);

    if (id1 < 0 || id2 < 0)
        return FALSE;

    if ((id1 == 0xffff && id2 == 0xffff) || (id1 == 0 && id2 == 0))
    {
        D(bug("[dwmac] nothing answering at mdio address %u\n", phy);)
        return FALSE;
    }

    D(bug("[dwmac] phy %u: id %04x%04x, bmsr %04x\n", phy,
          (UWORD)id1, (UWORD)id2,
          (UWORD)DWMAC_MDIORead(hw, phy, MII_BMSR));)

    return TRUE;
}

static int DWMAC_Init(LIBBASETYPEPTR LIBBASE)
{
    struct dwmac_hw *hw = &LIBBASE->dwm_HW;
    ULONG version;

    LIBBASE->dwm_Found = FALSE;
    LIBBASE->dwm_Unit = NULL;
    InitSemaphore(&LIBBASE->dwm_UnitSem);

    LIBBASE->dwm_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->dwm_KernelBase)
    {
        D(bug("[dwmac] no kernel.resource\n");)
        return FALSE;
    }

    LIBBASE->dwm_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->dwm_UtilityBase)
    {
        D(bug("[dwmac] no utility.library\n");)
        return FALSE;
    }

    if (!DWMAC_Discover(LIBBASE, hw))
        return FALSE;

    /*
     * The version register is the first thing worth reading: it says
     * the mapping reached the block at all, and which generation of
     * register map is behind it.
     */
    version = DWMAC_Read(hw, DWMAC_MAC_VERSION);

    if (version == 0 || version == 0xffffffff)
    {
        D(bug("[dwmac] register block reads %08x - not there\n", version);)
        return FALSE;
    }

    D(bug("[dwmac] core %u.%u (version %08x)\n",
          DWMAC_VERSION_CORE(version) >> 4,
          DWMAC_VERSION_CORE(version) & 0xf, version);)

    if (DWMAC_VERSION_CORE(version) < 0x40)
    {
        D(bug("[dwmac] core predates the register map this drives\n");)
        return FALSE;
    }

    if (!DWMAC_ProbePHY(hw))
        return FALSE;

    LIBBASE->dwm_Found = TRUE;

    return TRUE;
}

static int DWMAC_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->dwm_Unit)
    {
        DWMAC_DeleteUnit(LIBBASE, LIBBASE->dwm_Unit);
        LIBBASE->dwm_Unit = NULL;
    }

    if (LIBBASE->dwm_UtilityBase)
    {
        CloseLibrary(LIBBASE->dwm_UtilityBase);
        LIBBASE->dwm_UtilityBase = NULL;
    }

    return TRUE;
}

AROS_LH1(void, DWMAC_BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, DWMAC)
{
    AROS_LIBFUNC_INIT

    struct DWMACUnit *unit = (APTR)req->ios2_Req.io_Unit;

    req->ios2_Req.io_Error = 0;

    if (!unit || !unit->dwu_InputPort)
    {
        req->ios2_Req.io_Error = IOERR_OPENFAIL;
        if (!(req->ios2_Req.io_Flags & IOF_QUICK))
            ReplyMsg(&req->ios2_Req.io_Message);
        return;
    }

    if (AttemptSemaphore(&unit->dwu_Lock))
        DWMAC_HandleRequest(LIBBASE, req);
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->dwu_InputPort, &req->ios2_Req.io_Message);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, DWMAC_AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, DWMAC)
{
    AROS_LIBFUNC_INIT

    /*
     * Still NT_MESSAGE means still sitting on some queue; anything else
     * is either being processed right now or already replied, and in
     * neither case may it be unlinked here.
     */
    Disable();
    if (req->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE &&
        (req->ios2_Req.io_Flags & IOF_QUICK) == 0)
    {
        Remove((struct Node *)req);
        req->ios2_Req.io_Error = IOERR_ABORTED;
        req->ios2_WireError = S2WERR_GENERIC_ERROR;
        ReplyMsg(&req->ios2_Req.io_Message);
    }
    Enable();

    return 0;

    AROS_LIBFUNC_EXIT
}

static const IPTR dwmac_rx_tags[] =
{
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const IPTR dwmac_tx_tags[] =
{
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

static int DWMAC_Open(LIBBASETYPEPTR LIBBASE, struct IORequest *io,
                      ULONG unitnum, ULONG flags)
{
    struct Library *UtilityBase = LIBBASE->dwm_UtilityBase;
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct DWMACUnit *unit;
    struct Opener *opener;
    struct TagItem *tags;
    ULONG i;

    io->io_Error = IOERR_OPENFAIL;
    io->io_Unit = NULL;

    if (!LIBBASE->dwm_Found || unitnum != 0)
        return FALSE;

    if (io->io_Message.mn_Length < sizeof(struct IOSana2Req))
    {
        D(bug("[dwmac] open request is not a sana2 one\n");)
        return FALSE;
    }

    /* The hardware side comes up on first open and then stays up */
    ObtainSemaphore(&LIBBASE->dwm_UnitSem);
    if (!LIBBASE->dwm_Unit)
        LIBBASE->dwm_Unit = DWMAC_CreateUnit(LIBBASE);
    unit = LIBBASE->dwm_Unit;
    ReleaseSemaphore(&LIBBASE->dwm_UnitSem);
    if (!unit)
        return FALSE;

    opener = AllocMem(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
    if (!opener)
        return FALSE;

    tags = req->ios2_BufferManagement;
    for (i = 0; i < sizeof(dwmac_rx_tags) / sizeof(IPTR); i++)
        opener->rx_function =
            (APTR)GetTagData(dwmac_rx_tags[i], (IPTR)opener->rx_function,
                             tags);
    for (i = 0; i < sizeof(dwmac_tx_tags) / sizeof(IPTR); i++)
        opener->tx_function =
            (APTR)GetTagData(dwmac_tx_tags[i], (IPTR)opener->tx_function,
                             tags);
    opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

    if (!opener->rx_function || !opener->tx_function)
    {
        D(bug("[dwmac] opener brought no copy functions\n");)
        FreeMem(opener, sizeof(struct Opener));
        return FALSE;
    }

    NEWLIST(&opener->read_port.mp_MsgList);
    opener->read_port.mp_Flags = PA_IGNORE;
    NEWLIST(&opener->initial_stats);

    if (flags & SANA2OPF_PROM)
        unit->dwu_Flags |= IFF_PROMISC;

    req->ios2_BufferManagement = opener;
    io->io_Unit = (struct Unit *)unit;
    io->io_Error = 0;

    Disable();
    AddTail((struct List *)&unit->dwu_Openers, (struct Node *)opener);
    Enable();
    unit->dwu_OpenCount++;

    return TRUE;
}

static int DWMAC_Close(LIBBASETYPEPTR LIBBASE, struct IORequest *io)
{
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct DWMACUnit *unit = (APTR)io->io_Unit;
    struct Opener *opener = req->ios2_BufferManagement;
    struct Node *node;

    if (unit && opener)
    {
        Disable();
        Remove((struct Node *)opener);
        Enable();

        while ((node = RemHead((struct List *)&opener->initial_stats)))
            FreeMem(node, sizeof(struct TypeStats));

        FreeMem(opener, sizeof(struct Opener));
        req->ios2_BufferManagement = NULL;
        unit->dwu_OpenCount--;
    }

    io->io_Unit = (struct Unit *)-1;
    io->io_Device = (struct Device *)-1;

    return TRUE;
}

ADD2INITLIB(DWMAC_Init, 0)
ADD2EXPUNGELIB(DWMAC_Expunge, 0)
ADD2OPENDEV(DWMAC_Open, 0)
ADD2CLOSEDEV(DWMAC_Close, 0)
