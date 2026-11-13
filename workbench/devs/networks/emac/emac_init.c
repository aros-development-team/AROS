#define DEBUG 1

#include <asm/amcc440.h>

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

#include <utility/utility.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <hidd/pci.h>

#include <proto/oop.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/kernel.h>

#include "emac.h"
#include LC_LIBDEFS_FILE


static int GM_UNIQUENAME(Init)(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[EMAC ] Init()\n"));

    if (!FindResident("pci-amcc440.hidd"))
    {
        D(bug("[EMAC ] Emac driver may run only on AMCC440 CPU's\n"));
        return FALSE;
    }

    if (FindTask(EMAC_TASK1_NAME) != NULL)
    {
        D(bug("[EMAC ] Device already up and running.\n"));
        return FALSE;
    }

    LIBBASE->emb_Sana2Info.HardwareType = S2WireType_Ethernet;
    LIBBASE->emb_Sana2Info.MTU = ETH_MTU;
    LIBBASE->emb_Sana2Info.AddrFieldSize = 8 * ETH_ADDRESSSIZE;

    LIBBASE->emb_Pool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

    if (LIBBASE->emb_Pool != NULL)
    {
        EMAC_MAL_Init(LIBBASE);

        LIBBASE->emb_Units[0] = CreateUnit(LIBBASE, 0);
        LIBBASE->emb_Units[1] = CreateUnit(LIBBASE, 1);

        return TRUE;
    }

    return FALSE;
}

static const ULONG rx_tags[] = {
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const ULONG tx_tags[] = {
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

/*
 * Open device handles currently only one pcnet32 unit.
 */
static int GM_UNIQUENAME(Open)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req,
    ULONG unitnum,
    ULONG flags
)
{
    struct TagItem *tags;
    struct EMACUnit *unit = NULL;
    struct Opener *opener;
    BYTE error=0;
    int i;

    D(bug("[EMAC ] OpenDevice(%d)\n", unitnum));

    req->ios2_Req.io_Unit = NULL;
    tags = req->ios2_BufferManagement;

    req->ios2_BufferManagement = NULL;

    /* Check request size */

    if(req->ios2_Req.io_Message.mn_Length < sizeof(struct IOSana2Req))
        error = IOERR_OPENFAIL;

    if ((error != 0) || (unitnum > 1))
    {
        error = IOERR_OPENFAIL;
    }
    else
    {
        unit = (struct EMACUnit *)(req->ios2_Req.io_Unit = (struct Unit *)LIBBASE->emb_Units[unitnum]);
    }

    /* Handle device sharing */
    if(error == 0)
    {
        if(unit->eu_OpenCount != 0 && ((unit->eu_Flags & IFF_SHARED) == 0 ||
          (flags & SANA2OPF_MINE) != 0))
            error = IOERR_UNITBUSY;
        else
            unit->eu_OpenCount++;
    }

    if(error == 0)
    {
        if((flags & SANA2OPF_MINE) == 0)
            unit->eu_Flags |= IFF_SHARED;
        else if((flags & SANA2OPF_PROM) != 0)
            unit->eu_Flags |= IFF_PROMISC;

        /* Set up buffer-management structure and get hooks */
        opener = AllocVecPooled(LIBBASE->emb_Pool, sizeof(struct Opener));
        req->ios2_BufferManagement = (APTR)opener;

        if(opener == NULL)
            error = IOERR_OPENFAIL;
    }

    if(error == 0)
    {
        NEWLIST(&opener->read_port.mp_MsgList);
        opener->read_port.mp_Flags = PA_IGNORE;
        NEWLIST((APTR)&opener->initial_stats);

        for(i = 0; i < 2; i++)
            opener->rx_function = (APTR)GetTagData(rx_tags[i], (IPTR)opener->rx_function, tags);
        for(i = 0; i < 3; i++)
            opener->tx_function = (APTR)GetTagData(tx_tags[i], (IPTR)opener->tx_function, tags);

        opener->filter_hook = (APTR)GetTagData(S2_PacketFilter, 0, tags);

        Disable();
        AddTail((APTR)&unit->eu_Openers, (APTR)opener);
        Enable();
    }

    if (error != 0)
        CloseDevice((struct IORequest *)req);
    else
        unit->start(unit);

    req->ios2_Req.io_Error = error;
    return (error !=0) ? FALSE : TRUE;
}

static int GM_UNIQUENAME(Close)
(
    LIBBASETYPEPTR LIBBASE,
    struct IOSana2Req* req
)
{
    struct EMACUnit *unit = (struct EMACUnit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

    if (unit)
    {
        D(bug("[EMAC%d] CloseDevice\n", unit->eu_UnitNum));

        unit->stop(unit);

        opener = (APTR)req->ios2_BufferManagement;
        if (opener != NULL)
        {
            Disable();
            Remove((struct Node *)opener);
            Enable();
            FreeVecPooled(LIBBASE->emb_Pool, opener);
        }
    }

    return TRUE;
}

ADD2INITLIB(GM_UNIQUENAME(Init),0)
ADD2OPENDEV(GM_UNIQUENAME(Open),0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close),0)

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, EMAC)
{
    AROS_LIBFUNC_INIT

    struct EMACUnit *unit;

    req->ios2_Req.io_Error = 0;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (unit)
    {
        D(bug("[EMAC%d] BeginIO\n", unit->eu_UnitNum));

        if (AttemptSemaphore(&unit->eu_Lock))
        {
            handle_request(LIBBASE, req);
        }
        else
        {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->eu_InputPort, (struct Message *)req);
        }
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 6, EMAC)
{
    AROS_LIBFUNC_INIT

    struct EMACUnit *unit;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (unit)
    {
        D(bug("[EMAC%d] AbortIO\n", unit->eu_UnitNum));

        Disable();
        if ((req->ios2_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE) &&
                (req->ios2_Req.io_Flags & IOF_QUICK) == 0)
        {
            Remove((struct Node *)req);
            req->ios2_Req.io_Error = IOERR_ABORTED;
            req->ios2_WireError = S2WERR_GENERIC_ERROR;
            ReplyMsg((struct Message *)req);
        }
        Enable();
    }

    return 0;

    AROS_LIBFUNC_EXIT
}
