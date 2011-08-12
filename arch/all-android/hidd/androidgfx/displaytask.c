#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/ports.h>
#include <hidd/unixio.h>
#include <hidd/unixio_inline.h>

#include <proto/exec.h>

#include "agfx.h"
#include "server.h"

/*
 * In order to keep data stream consistent, we need to writa data to a pipe
 * in a single process. Otherwise it will come to the server broken into pieces.
 */

#undef XSD
#define XSD(x) ((struct agfx_staticdata *)x)

static void agfxInt(int pipe, int mode, void *data)
{
    Signal(XSD(data)->clientTask, XSD(data)->clientRead);
}

static void ReadPipe(int pipe, void *data, int len, struct agfx_staticdata *xsd)
{
    int res;
    
    res = Hidd_UnixIO_ReadFile(xsd->unixio, pipe, data, len, NULL);
    if (res != len)
    {
	D(bug("[AGFX.task] Error reading pipe, result %d\n", res));

    	/* This likely means broken pipe. Our server is dead, we should die too. */
    	ShutdownA(SD_ACTION_POWEROFF);
    }
}   

/* Request lengths in bytes */
static const int RequestLength[] =
{
    0,
    8,	/* cmd_Query */
};

static const int ReplyLength[] = 
{
    0,
    8,	/* cmd_Query */
};

void agfxTask(int pipe, struct agfx_staticdata *xsd)
{
    int pipeready = 0;
    LONG readsig;
    int res = -1;
    struct Task *me = FindTask(NULL);

    D(bug("[AGFX.task] Task started\n"));

    readsig = AllocSignal(-1);
    if (readsig != -1)
    {
    	xsd->clientRead = 1L << readsig;

    	xsd->clientInt.fd      	   = pipe;
    	xsd->clientInt.mode   	   = vHidd_UnixIO_Read;
    	xsd->clientInt.handler 	   = agfxInt;
    	xsd->clientInt.handlerData = xsd;

    	res = Hidd_UnixIO_AddInterrupt(xsd->unixio, &xsd->clientInt);
    }

    if (res != 0)
    {
    	D(bug("[AGFX.task] Initialization failed\n"));
    	xsd->clientRead = 0;	/* Signal failure to parent task */
    }

    /* In AROS all tasks are ETasks */
    Signal(me->tc_UnionETask.tc_ETask->et_Parent, SIGF_BLIT);

    if (res != 0)
    	return;

    do
    {
    	struct Request *request;

	/* Send out all pending requests */
	while ((request = (struct Request *)GetMsg(xsd->clientPort)))
	{
	    ULONG cmd = request->cmd;
	    int len = RequestLength[cmd & ~CMD_FLAGS_MASK];

    	    DB2(bug("[AGFX.task] Command 0x%08X to server\n", cmd));

	    if (cmd == cmd_Shutdown)
	    {
		D(bug("[AGFX.task] Got shutdown command\n"));

		/*
		 * Close pipe and exit immediately.
		 * Closing a pipe will cause "Broken pipe" error on other side,
		 * so the server will know we disconnected.
		 */
		Hidd_UnixIO_RemInterrupt(xsd->unixio, &xsd->clientInt);
		Hidd_UnixIO_CloseFile(xsd->unixio, pipe, NULL);

		Signal(request->owner, request->signal);
		return;
	    }

    	    res = Hidd_UnixIO_WriteFile(xsd->unixio, pipe, &request->cmd, len, NULL);

    	    if (res != len)
    	    {
    	        D(bug("[AGFX.task] Error writing pipe, result %d\n", res));

    	        /* This likely means broken pipe. Our server is dead, we should die too. */
    	        ShutdownA(SD_ACTION_POWEROFF);
    	    }

	    if (cmd & CMDF_Quick)
	    {
	    	/* No reply needed from the server. Signal completion. */
	    	Signal(request->owner, request->signal);
	    }
	    else
	    {
	    	/* This commands expects a reply, add to queue. */
    	    	AddTail((struct List *)&xsd->sentQueue, &request->msg.mn_Node);
    	    }
    	}

	/*
	 * Now handle incoming data.
	 *
	 * SIGIO can actually be sent only when the server actually writes something.
	 * If we read only a part of the available data (for example if we have more than one
	 * packet pending), we won't get a second SIGIO until the server writes one more
	 * packet to us.
	 * In order to prevent ourselves from being stuck because of this, we wait
	 * for SIGIO on our pipe only after it gets empty.
	 * When the signal arrives, we already know that the pipe is readable, so we
	 * can skip polling.
	 */
	if (!pipeready)
	{
	    pipeready = Hidd_UnixIO_Poll(xsd->unixio, pipe, vHidd_UnixIO_Read, NULL);
	    if (pipeready == -1)
	    {
    	    	D(bug("[AGFX.task] Error polling pipe\n"));

    	    	/* This likely means broken pipe. Our server is dead, we should die too. */
    	    	ShutdownA(SD_ACTION_POWEROFF);
    	    }
    	}

    	if (pipeready & vHidd_UnixIO_Read)
    	{
    	    ULONG cmd;

	    ReadPipe(pipe, &cmd, sizeof(cmd), xsd);
    	    DB2(bug("[AGFX.task] Command 0x%08X from server\n", cmd));

	    if (!(cmd & CMDF_Quick))
	    {
	    	struct Request *request = (struct Request *)RemHead((struct List *)&xsd->sentQueue);

    	    	if (request)
    	    	{
    	    	    int request_len = RequestLength[cmd & ~CMD_FLAGS_MASK];
    	            int reply_len   = ReplyLength[cmd & ~CMD_FLAGS_MASK];

#ifdef CHECK_CONSISTENCY
		    if (cmd != request->cmd)
		    {
		    	bug("[AGFX.task] Got response 0x%08X for command 0x%08X from server!\n", cmd, request->cmd);
		    	Alert(AT_DeadEnd | AN_AsyncPkt);
		    }
#endif
    	            ReadPipe(pipe, (char *)&request->cmd + request_len, reply_len, xsd);

    	            DB2(bug("[AGFX.task] Request 0x%p replied\n", request));
    	            Signal(request->owner, request->signal);
    	        }
#ifdef CHECK_CONSISTENCY
    	        else
    	            bug("[AGFX.task] Bogus response 0x%08X from server, no pending request!\n", cmd));
#endif
    	    }

    	    pipeready = 0;
    	}
    	else
    	{
    	    /* Pipe was not ready, wait for something. */
    	    ULONG reqmask  = 1L << xsd->clientPort->mp_SigBit;
    	    ULONG sigmask  = Wait(reqmask | xsd->clientRead);

	    if (sigmask & xsd->clientRead)
	    	pipeready = vHidd_UnixIO_Read;
    	}

    } while(1);
}

void DoRequest(struct Request *req, struct agfx_staticdata *xsd)
{
    /*
     * Set request owner.
     * Limitation: one task can have only one request pending. But
     * it's faster than CreateMsgPort() every time.
     */
    req->owner  = FindTask(NULL);
    req->signal = SIGF_BLIT;

    /* Send the request */
    SetSignal(0, SIGF_BLIT);
    PutMsg(xsd->clientPort, &req->msg);

    /* Wait for reply */
    Wait(SIGF_BLIT);
}
