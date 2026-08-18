/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Broadcom GENETv5 SANA-II driver, initialisation. This part is
          generic AROS device ceremony (Open/Close/BeginIO/AbortIO/Init/
          Expunge), not GENET-specific.
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/openfirmware.h>

#include <devices/sana2.h>

#include LC_LIBDEFS_FILE

#include "bcmgenet.h"

static int BCMGENET_Init(LIBBASETYPEPTR LIBBASE)
{
    struct bcmgenet_hw *hw = &LIBBASE->bgm_HW;

    LIBBASE->bgm_Found = FALSE;
    LIBBASE->bgm_Unit = NULL;
    InitSemaphore(&LIBBASE->bgm_UnitSem);

    LIBBASE->bgm_KernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->bgm_KernelBase)
    {
        D(bug("[bcmgenet] no kernel.resource\n");)
        return FALSE;
    }

    LIBBASE->bgm_OpenFirmwareBase = OpenResource("openfirmware.resource");
    if (!LIBBASE->bgm_OpenFirmwareBase)
    {
        D(bug("[bcmgenet] no openfirmware.resource\n");)
        return FALSE;
    }

    LIBBASE->bgm_UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);
    if (!LIBBASE->bgm_UtilityBase)
    {
        D(bug("[bcmgenet] no utility.library\n");)
        return FALSE;
    }

    if (!BCMGENET_Discover(LIBBASE, hw))
        return FALSE;

    /*
     * TODO: once BCMGENET_Read()/BCMGENET_HWReset() are written, sanity
     * check GENET_SYS_REV_CTRL here the way DWMAC_Init() checks
     * DWMAC_MAC_VERSION - a register block that reads all-ones or
     * all-zeros means the mapping did not really reach the hardware.
     */

    LIBBASE->bgm_Found = TRUE;

    return TRUE;
}

static int BCMGENET_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (LIBBASE->bgm_Unit)
    {
        BCMGENET_DeleteUnit(LIBBASE, LIBBASE->bgm_Unit);
        LIBBASE->bgm_Unit = NULL;
    }

    if (LIBBASE->bgm_UtilityBase)
    {
        CloseLibrary(LIBBASE->bgm_UtilityBase);
        LIBBASE->bgm_UtilityBase = NULL;
    }

    return TRUE;
}

AROS_LH1(void, BCMGENET_BeginIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, BCMGENET)
{
    AROS_LIBFUNC_INIT

    struct BCMGENETUnit *unit = (APTR)req->ios2_Req.io_Unit;

    req->ios2_Req.io_Error = 0;

    if (!unit || !unit->bgu_InputPort)
    {
        req->ios2_Req.io_Error = IOERR_OPENFAIL;
        if (!(req->ios2_Req.io_Flags & IOF_QUICK))
            ReplyMsg(&req->ios2_Req.io_Message);
        return;
    }

    if (AttemptSemaphore(&unit->bgu_Lock))
        BCMGENET_HandleRequest(LIBBASE, req);
    else
    {
        req->ios2_Req.io_Flags &= ~IOF_QUICK;
        PutMsg(unit->bgu_InputPort, &req->ios2_Req.io_Message);
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, BCMGENET_AbortIO,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, BCMGENET)
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

static const IPTR bcmgenet_rx_tags[] =
{
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const IPTR bcmgenet_tx_tags[] =
{
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

static int BCMGENET_Open(LIBBASETYPEPTR LIBBASE, struct IORequest *io,
                         ULONG unitnum, ULONG flags)
{
    struct Library *UtilityBase = LIBBASE->bgm_UtilityBase;
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct BCMGENETUnit *unit;
    struct Opener *opener;
    struct TagItem *tags;
    ULONG i;

    io->io_Error = IOERR_OPENFAIL;
    io->io_Unit = NULL;

    if (!LIBBASE->bgm_Found || unitnum != 0)
        return FALSE;

    if (io->io_Message.mn_Length < sizeof(struct IOSana2Req))
    {
        D(bug("[bcmgenet] open request is not a sana2 one\n");)
        return FALSE;
    }

    /* The hardware side comes up on first open and then stays up */
    ObtainSemaphore(&LIBBASE->bgm_UnitSem);
    if (!LIBBASE->bgm_Unit)
        LIBBASE->bgm_Unit = BCMGENET_CreateUnit(LIBBASE);
    unit = LIBBASE->bgm_Unit;
    ReleaseSemaphore(&LIBBASE->bgm_UnitSem);
    if (!unit)
        return FALSE;

    opener = AllocMem(sizeof(struct Opener), MEMF_PUBLIC | MEMF_CLEAR);
    if (!opener)
        return FALSE;

    tags = req->ios2_BufferManagement;
    for (i = 0; i < sizeof(bcmgenet_rx_tags) / sizeof(IPTR); i++)
        opener->rx_function =
            (APTR)GetTagData(bcmgenet_rx_tags[i], (IPTR)opener->rx_function,
                             tags);
    for (i = 0; i < sizeof(bcmgenet_tx_tags) / sizeof(IPTR); i++)
        opener->tx_function =
            (APTR)GetTagData(bcmgenet_tx_tags[i], (IPTR)opener->tx_function,
                             tags);
    opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

    if (!opener->rx_function || !opener->tx_function)
    {
        D(bug("[bcmgenet] opener brought no copy functions\n");)
        FreeMem(opener, sizeof(struct Opener));
        return FALSE;
    }

    NEWLIST(&opener->read_port.mp_MsgList);
    opener->read_port.mp_Flags = PA_IGNORE;
    NEWLIST(&opener->initial_stats);

    if (flags & SANA2OPF_PROM)
        unit->bgu_Flags |= IFF_PROMISC;

    req->ios2_BufferManagement = opener;
    io->io_Unit = (struct Unit *)unit;
    io->io_Error = 0;

    Disable();
    AddTail((struct List *)&unit->bgu_Openers, (struct Node *)opener);
    Enable();
    unit->bgu_OpenCount++;

    return TRUE;
}

static int BCMGENET_Close(LIBBASETYPEPTR LIBBASE, struct IORequest *io)
{
    struct IOSana2Req *req = (struct IOSana2Req *)io;
    struct BCMGENETUnit *unit = (APTR)io->io_Unit;
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
        unit->bgu_OpenCount--;
    }

    io->io_Unit = (struct Unit *)-1;
    io->io_Device = (struct Device *)-1;

    return TRUE;
}

ADD2INITLIB(BCMGENET_Init, 0)
ADD2EXPUNGELIB(BCMGENET_Expunge, 0)
ADD2OPENDEV(BCMGENET_Open, 0)
ADD2CLOSEDEV(BCMGENET_Close, 0)
