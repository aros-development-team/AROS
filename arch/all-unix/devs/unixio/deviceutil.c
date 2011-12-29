#define DEBUG 0
#define DOPEN(x)

#include <errno.h>

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/errors.h>
#include <proto/exec.h>

#include "unixio_device.h"

static int unixdevice_Open(struct UnixDevice *unixioDev, struct IOStdReq *ioreq, STRPTR unitname, ULONG flags)
{
    struct UnitData *unit;

    D(bug("unixio.device: open UnitData %s\n", unitname));

    ObtainSemaphore(&unixioDev->sigsem);

    unit = (struct UnitData *)FindName(&unixioDev->units, unitname);

    if (UnitData)
    {
        /* TODO: Check sharing permission here */
        UnitData->usecount++;
        ReleaseSemaphore(&unixioDev->sigsem);

        ioreq->io_Unit                    = (struct Unit *)unit;
        ioreq->io_Error                   = 0;
        ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

        DOPEN(bug("unixio.device: in libopen func. Unit is already open\n"));    
        return TRUE;
    }

    DOPEN(bug("unixio.device: in libopen func. Creating new UnitData ...\n"));

    unit = (struct UnitData *)AllocVec(sizeof(struct UnitData) + strlen(unitname), MEMF_PUBLIC);
    if (unit)
    {
        unit->fd = Hidd_UnixIO_Open(unixioDev->unixio, unitname, O_RWDR|O_NONBLOCK, 0755, NULL);
        if (unit->fd != -1)
        {
            unit->unitNode.ln_Name = unit->unitName;
            unit->unixio           = unixioDev->unixio;
            unit->usecount         = 1;
            unit->writeLength      = 0;
            unit->stopped          = FALSE;
            unit->eofmode          = FALSE;
            NEWLIST(&unit->readQueue);
            NEWLIST(&unit->writeQueue);
            strcpy(unit->unitName, unitname);

            AddTail((struct List *)&unixioDev->units, &UnitData->n);
            ReleaseSemaphore(&unixioDev->sigsem);

            ioreq->io_Unit                    = (struct Unit *)unit;
            ioreq->io_Error                   = 0;
            ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;

            return TRUE;
        }
    }

    ReleaseSemaphore(&unixioDev->sigsem);
    ioreq->io_Error = IOERR_OPENFAIL;

    return FALSE;
}

ADD2OPENDEV(unixdevice_Open, 0);

/****************************************************************************************/

static int unixdevice_Close((struct UnixDevice *unixioDev, struct IOStdReq *ioreq)
{
    struct UnitData *unit = (struct UnitData *)ioreq->io_Unit;
    ULONG usecnt;

    D(bug("unixio.device: close unit %s\n", unit->n.ln_Name));

    ObtainSemaphore(&unixioDev->sigsem);

    usecnt = --unit->usecount;
    if (!usecnt)
    {
        D(bug("unixio.device: Unit is no more in use, disposing...\n"));

        Remove(&unit->unitNode);
    }

    ReleaseSemaphore(&unixioDev->sigsem);

    if (!usecnt)
    {
        Hidd_UnixIO_CloseFile(unixioDev->unixio, unit->fd, NULL);
        FreeVec(unit);
    }

    return TRUE;
}

ADD2CLOSEDEV(unixdevice_Close, 0);

/****************************************************************************************/
