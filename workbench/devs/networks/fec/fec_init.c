/*
 * fec_init.c
 *
 *  Created on: May 14, 2009
 *      Author: misc
 */

#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>

#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/devices.h>
#include <exec/ports.h>

#include <utility/tagitem.h>

#include <devices/sana2.h>
#include <devices/sana2specialstats.h>

#include <proto/openfirmware.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <inttypes.h>

#include "fec.h"

#include LC_LIBDEFS_FILE

typedef struct {
	uint8_t b[6];
} mac_addr_t;

typedef struct {
	intptr_t 	addr;
	intptr_t	size;
} reg_t;

static int FEC_Init(struct FECBase *FECBase)
{
	int retval = FALSE;
	void *OpenFirmwareBase = NULL;
	fec_t *fec = NULL;

	D(bug("[FEC] Fast Ethernet Controller Init.\n"));

	OpenFirmwareBase = OpenResource("openfirmware.resource");

	if (OpenFirmwareBase)
	{
		void *key, *prop;

		key = OF_OpenKey("/builtin/ethernet");

		if (key)
		{
			prop = OF_FindProperty(key, "reg");

			if (prop)
			{
				reg_t *reg = OF_GetPropValue(prop);
				fec = (fec_t *)reg->addr;

				D(bug("[FEC] FEC registers at %08x\n", fec));

				prop = OF_FindProperty(key, "mac-address");

				if (prop)
				{
					FECBase->feb_Pool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED, 8192, 4096);

					if (FECBase->feb_Pool)
					{
						if (FEC_CreateUnit(FECBase, fec))
						{
							mac_addr_t *mac = OF_GetPropValue(prop);
							D(bug("[FEC] MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
									mac->b[0], mac->b[1], mac->b[2], mac->b[3], mac->b[4], mac->b[5]));

							FECBase->feb_Unit->feu_OrgAddr[0] = mac->b[0];
							FECBase->feb_Unit->feu_OrgAddr[1] = mac->b[1];
							FECBase->feb_Unit->feu_OrgAddr[2] = mac->b[2];
							FECBase->feb_Unit->feu_OrgAddr[3] = mac->b[3];
							FECBase->feb_Unit->feu_OrgAddr[4] = mac->b[4];
							FECBase->feb_Unit->feu_OrgAddr[5] = mac->b[5];

							retval = TRUE;
						}
						else
						{
							D(bug("[FEC] Failed to create the unit\n"));

							DeletePool(FECBase->feb_Pool);
						}
					}
				}
			}
		}
	}
	else
	{
		bug("[FEC] OpenFirmware not found. Aborting.\n");
	}

	return retval;
}


static const uintptr_t rx_tags[] = {
    S2_CopyToBuff,
    S2_CopyToBuff16
};

static const uintptr_t tx_tags[] = {
    S2_CopyFromBuff,
    S2_CopyFromBuff16,
    S2_CopyFromBuff32
};

/*
 * Open device handles currently only one pcnet32 unit.
 */
static int FEC_Open(struct FECBase *FECBase, struct IOSana2Req* req, ULONG unitnum, ULONG flags)
{
    struct TagItem *tags;
    struct FECUnit *unit = NULL;
    struct Opener *opener;
    BYTE error=0;
    int i;

    D(bug("[FEC] OpenDevice(%d)\n", unitnum));

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
        unit = FECBase->feb_Unit;
        req->ios2_Req.io_Unit = &unit->feu_Unit;
    }

    /* Handle device sharing */
    if(error == 0)
    {
        if(unit->feu_OpenCount != 0 && ((unit->feu_Flags & IFF_SHARED) == 0 ||
          (flags & SANA2OPF_MINE) != 0))
            error = IOERR_UNITBUSY;
        else
            unit->feu_OpenCount++;
    }

    if(error == 0)
    {
        if((flags & SANA2OPF_MINE) == 0)
            unit->feu_Flags |= IFF_SHARED;
        else if((flags & SANA2OPF_PROM) != 0)
            unit->feu_Flags |= IFF_PROMISC;

        /* Set up buffer-management structure and get hooks */
        opener = AllocVecPooled(FECBase->feb_Pool, sizeof(struct Opener));
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
        AddTail((APTR)&unit->feu_Openers, (APTR)opener);
        Enable();
    }

    if (error != 0)
        CloseDevice((struct IORequest *)req);
    else
        unit->start(unit);

    req->ios2_Req.io_Error = error;
    return (error !=0) ? FALSE : TRUE;
}

static int FEC_Close(struct FECBase *FECBase, struct IOSana2Req* req)
{
    struct FECUnit *unit = (struct FECUnit *)req->ios2_Req.io_Unit;
    struct Opener *opener;

    if (unit)
    {
        D(bug("[FEC] CloseDevice\n"));

        unit->stop(unit);

        opener = (APTR)req->ios2_BufferManagement;
        if (opener != NULL)
        {
            Disable();
            Remove((struct Node *)opener);
            Enable();
            FreeVecPooled(FECBase->feb_Pool, opener);
        }
    }

    return TRUE;
}

ADD2INITLIB(FEC_Init,0)
ADD2OPENDEV(FEC_Open,0)
ADD2CLOSEDEV(FEC_Close,0)

AROS_LH1(void, beginio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, FEC)
{
    AROS_LIBFUNC_INIT

    struct FECUnit *unit;

    req->ios2_Req.io_Error = 0;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (unit)
    {
        D(bug("[FEC] BeginIO\n"));

        if (AttemptSemaphore(&unit->feu_Lock))
        {
            handle_request(LIBBASE, req);
        }
        else
        {
            req->ios2_Req.io_Flags &= ~IOF_QUICK;
            PutMsg(unit->feu_InputPort, (struct Message *)req);
        }
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, abortio,
    AROS_LHA(struct IOSana2Req *, req, A1),
    LIBBASETYPEPTR, LIBBASE, 5, FEC)
{
    AROS_LIBFUNC_INIT

    struct FECUnit *unit;
    unit = (APTR)req->ios2_Req.io_Unit;

    if (unit)
    {
        D(bug("[FEC] AbortIO\n"));

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
