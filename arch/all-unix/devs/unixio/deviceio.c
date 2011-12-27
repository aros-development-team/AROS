#include <hidd/unixio.h>
#include <hidd/unixio_inline.h>
#include <proto/exec.h>

#include "unixio_device.h"

#include <signal.h>

static inline void SetupActive(struct Queue *q)
{
    q->Data   = q->Active->io_Data;
    q->Length = (q->Active->io_Length == -1) ? strlen(q->Data) : q->Active->io_Length;
}

/* Proceed to the next pending request, if any */
static inline void Next(struct Queue *q)
{
    q->Active = REMHEAD(&q->Pending);
    if (q->Active)
        SetupActive(q);
}

static void HandleIO(struct Queue *q, int len, int err)
{
    BOOL done = FALSE;

    if (len == -1)
    {
        done = data->ErrorCallback(q->Active, err);
    }
    else
    {
        q->Active->io_Actual += len;
        q->Data              += len;
        q->Length            -= len;

        done == (q->Length == 0);
    }

    if (done)
    {
        /* Active request is done, reply it */
        q->Active->io_Error = 0;
        ReplyMsg(&q->Active->io_Message);

        /* Proceed to the next pending request, if any */
        Next(q);
    }
}

static void Push(struct Queue *q, struct IoStdReq *req)
{
    /*
     * As I am returning immediately I will tell that this
     * could not be done QUICK   
     */
    req->io_Flags &= ~IOF_QUICK;
    req->io_Actual = 0;

    Disable();

    if (q->Active)
    {
        /* There's already an active request. Add to the list of pending ones. */
        ADDTAIL(&q->Pending, req);
    }
    else
    {
        q->Active = req;
        SetupActive(q);

        UnixDevice->iface->raise(SIGIO);
        AROS_HOST_BARRIER
    }

    Enable();
}

static void Flush(struct IoStdReq *req)
{
    struct IoStdReq *next;

    for (; req->io_Message.mn_Node.ln_Succ; req = next)
    {
	next = (struct IoStdReq *)req->io_Message.mn_Node.ln_Succ

        ireq->io_Error = IOERR_ABORTED;
        ReplyMsg(&req->io_Message);    
    }
}   

void unit_io(int fd, int mode, struct UnitData *data)
{
    struct IOStdReq *req;
    int err;

    if (data->stopped)
    {
        /* The unit is stopped. Ignore the interrupt. */
        return;
    }

    if (mode & vHidd_UnixIO_Read)
    {
        /* Read as much as we can */
        while (data->readQueue.Active)
        {
            len = Hidd_UnixIO_ReadFile(data->unixio, fd, data->readQueue.Data, data->readQueue.Length, &err);

            if ((len == -1) && (err == EWOULDBLOCK))
            {
                /* There are no more data to read, but we need to read more. Wait for the next interrupt. */
                break;
            }

            HandleIO(&data->readQueue, len, err);
        }
    }

    if (mode & vHidd_UnixIO_Write)
    {
        /* Write as much as we can */
        while (data->writeQueue.Active)
        {
            len = Hidd_UnixIO_WriteFile(data->unixio, fd, data->writeQueue.Data, data->writeQueue.Length, &err);

            if ((len == -1) && (err == EWOULDBLOCK))
            {
                /* Output buffer filled up. Wait for the next interrupt. */
                break;
            }

            HandleIO(&data->writeQueue, len, err);
        }
    }
}

AROS_LH1(void, beginio,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         struct unixioDev *, UnixDevice, 5, Unixio)
{
    AROS_LIBFUNC_INIT

    struct UnitData *data = (struct UnitData *)ioreq->io_Unit;
    struct IoStdReq *read, *write, *activeread, *activewrite;

    D(bug("unixio.device: beginio(0x%p)\n", ioreq));

    /* WaitIO will look into this */
    ioreq->io_Message.mn_Node.ln_Type=NT_MESSAGE;

    switch (ioreq->io_Command)
    {
    case CMD_READ:
        D(bug("Queuing the read request.\n"));
        Push(&data->writeQueue, ioreq);

        return;

    case CMD_WRITE:
        D(bug("Queuing the write request.\n"));
        Push(&data->writeQueue, ioreq);

        return;

    case CMD_RESET:
        /*
         * All IORequests, including the active ones, are aborted.
         * The same as CMD_FLUSH (see below) plus handles Active requests.
         */
        Disable();

        read  = (struct IOStdReq *)data->readQueue.Pending.mlh_Head;
        write = (struct IOStdReq *)data->writeQueue.Pending.mlh_Head;
        NEWLIST(&data->readQueue.Pending);
        NEWLIST(&data->writeQueue.Pending);        

        activeread  = data->readQueue.Active;
        activewrite = data->writeQueue.Active;
        data->readQueue.Active  = NULL;
        data->writeQueue.Active = NULL;
        
        Enable();

        if (activeread)
        {
            activeread->io_Error = IOERR_ABORTED;
            ReplyMsg(&activeread->io_Message);
        }
        if (activewrite)
        {
            activewrite->io_Error = IOERR_ABORTED;
            ReplyMsg(&activewrite->io_Message);
        }

        Flush(read);
        Flush(write);

        ioreq->io_Error = 0;
        break;

    case CMD_FLUSH:
        /* 
         * Clear all queued IO request for the given parallel unit except
         * for the active ones.
         * An optimization trick: in order not to hold Disable()d state for a while,
         * we just detach chains of pending requests from queue's lists and reinitialize
         * lists. After this we can Enable(), then reply all requests in our chains.
         */
        Disable();

        read  = (struct IOStdReq *)data->readQueue.Pending.mlh_Head;
        write = (struct IOStdReq *)data->writeQueue.Pending.mlh_Head;

        NEWLIST(&data->readQueue.Pending);
        NEWLIST(&data->writeQueue.Pending);

        Enable();

        Flush(read);
        Flush(write);

        ioreq->io_Error = 0;
        break;

    case CMD_START:
        data->stopped = FALSE;

        if (data->readQueue.Active || data->writeQueue.Active)
        {
            UnixDevice->iface->raise(SIGIO);
            AROS_HOST_BARRIER
        }

        ioreq->io_Error = 0;
        break;

    case CMD_STOP:
        data->stopped = TRUE;
        ioreq->io_Error = 0;
        break;

    default:
        /* unknown command */
        ioreq->io_Error = IOERR_NOCMD;
    }

    /*
     * The request could be completed immediately.
     * Check if I have to reply the message
     */
    if (0 == (ioreq->IOPar.io_Flags & IOF_QUICK))
        ReplyMsg(&ioreq->IOPar.io_Message);

    D(bug("id: Return from BeginIO()\n"));

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(LONG, abortio,
         AROS_LHA(struct IOStdReq *, ioreq, A1),
         struct unixioDev *, UnixDevice, 6, Unixio)
{
    AROS_LIBFUNC_INIT

    struct UnitData *data = (struct UnitData *)ioreq->io_Unit;

    Disable();
    
    if (data->readQueue.Active == ioreq)
    	Next(&data->readQueue);
    else if (data->writeQueue.Active == ioreq)
    	Next(&data->writeQueue);
    else
    	REMOVE(ioreq);

    Enable();

    ioreq->io_Error = IOERR_ABORTED;
    ReplyMsg(&ioreq->io_Message);

    return 0;

    AROS_LIBFUNC_EXIT
}
