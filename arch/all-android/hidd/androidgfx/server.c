#define DEBUG 2
#define CHECK_CONSISTENCY

#include <aros/debug.h>
#include <exec/alerts.h>
#include <exec/ports.h>
#include <hidd/unixio.h>
#include <hidd/unixio_inline.h>

#include <proto/exec.h>

#include "agfx.h"
#include "server.h"

#undef XSD
#define XSD(x) ((struct agfx_staticdata *)x)

static void ReadPipe(int pipe, void *data, int len, struct agfx_staticdata *xsd)
{
    int res;
    int err;
    
    res = Hidd_UnixIO_ReadFile(xsd->unixio, pipe, data, len, &err);
    if (res != len)
    {
	D(bug("[AGFX.server] Error reading pipe, result %d, error %d\n", res, err));

    	/* This likely means broken pipe. Our server is dead, we should die too. */
    	ShutdownA(SD_ACTION_POWEROFF);
    }
}

static const int ReplyLength[] = 
{
    0,
    8,	/* cmd_Query */
};

static inline void ReplyRequest(struct Request *request)
{
    DB2(bug("[AGFX.server] Replying request 0x%p\n", request));

    Signal((struct Task *)request->owner, request->signal);
}

void agfxInt(int pipe, int mode, void *data)
{
    DB2(bug("[AGFX.server] Event 0x%08X on pipe %d\n", mode, pipe));

    if (mode & vHidd_UnixIO_Error)
    {
    	/* This likely means broken pipe. Our server is dead, we should die too. */
    	D(bug("[AGFX.server] Error condition on input pipe\n"));

    	ShutdownA(SD_ACTION_POWEROFF);
    }

    if (mode & vHidd_UnixIO_Read)
    {
    	ULONG cmd;
    	ULONG status = STATUS_ACK;
    	int reply_len;

	ReadPipe(pipe, &cmd, sizeof(cmd), data);
    	DB2(bug("[AGFX.server] Command 0x%08X from server\n", cmd));

	if (cmd == cmd_Nak)
	{
	    ReadPipe(pipe, &cmd, sizeof(cmd), data);
	    D(bug("[AGFX.server] NAK %d received\n", cmd));

	    status = STATUS_NAK;
	}

	reply_len  = ReplyLength[cmd];
	if (reply_len)
	{
    	    struct Request *request = (struct Request *)RemHead((struct List *)&XSD(data)->waitQueue);

#ifdef CHECK_CONSISTENCY
	    if (!request)
	    {
		bug("[AGFX.server] Bogus response 0x%08X from server, no pending request!\n", cmd);
		Alert(AT_DeadEnd | AN_AsyncPkt);
	    }
	    if (cmd != request->cmd)
	    {
		bug("[AGFX.server] Got response 0x%08X for command 0x%08X from server!\n", cmd, request->cmd);
		Alert(AT_DeadEnd | AN_AsyncPkt);
	    }
#endif

	    if (status == STATUS_ACK)
	    {
	    	ULONG *cmd = &request->cmd;

    	        ReadPipe(pipe, &cmd[request->len + 2], reply_len, data);
    	    }

    	    request->status = status;
    	    ReplyRequest(request);
    	}
    }
}

void DoRequest(struct Request *req, struct agfx_staticdata *xsd)
{
    int len, res, err;
    int need_reply = ReplyLength[req->cmd];

    /* If ReplyLength array has nonzero value for this command, we need to wait for reply */
    if (need_reply)
    {
    	/*
     	 * Set request owner.
     	 * Limitation: one task can have only one request pending. But
     	 * it's faster than CreateMsgPort() every time.
     	 */
    	req->owner  = FindTask(NULL);
    	req->signal = SIGF_BLIT;

	/* Add the request to the wait queue */
    	Disable();
    	AddTail((struct List *)&xsd->waitQueue, (struct Node *)req);
    	Enable();

    	SetSignal(0, SIGF_BLIT);
    }

    len = (req->len + 2) * sizeof(ULONG);
    res = Hidd_UnixIO_WriteFile(xsd->unixio, xsd->DisplayPipe, &req->cmd, len, &err);
    if (res != len)
    {
        D(bug("[AGFX.server] Error writing pipe, result %d, error %d\n", res, err));

    	/* This likely means broken pipe. Our server is dead, we should die too. */
    	ShutdownA(SD_ACTION_POWEROFF);
    }

    if (need_reply)
    {
    	/* Wait for reply */
    	Wait(SIGF_BLIT);
    }
}
