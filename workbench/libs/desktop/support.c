
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <intuition/classes.h>
#include <utility/tagitem.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include "desktop_intern.h"
#include "support.h"

#include "desktop_intern_protos.h"

BOOL startDesktopHandler(void)
{
	// NOTE: the OPEN vector (the only caller of this function)
	// already has a mutex on the library base at this point
	struct Process *process;
	struct MsgPort *port;
	struct Message *msg;
	struct TagItem  procTags[]=
	{
        {NP_Entry, (ULONG)desktopHandler},
        {NP_StackSize, 8192},
        {NP_Name, (IPTR)"Desktop Handler"},
		{NP_UserData, NULL},
        {TAG_DONE, 0}
    };

	port=(struct MsgPort*)CreateMsgPort();
	procTags[3].ti_Data=(ULONG)port;

	DesktopBase->db_Library.lib_OpenCnt++;

	kprintf("*** Starting desktop handler\n");
    process=CreateNewProc(procTags);
	if(!process)
		return FALSE;

	WaitPort(port);
	msg=GetMsg(port);
	ReplyMsg(msg);

	kprintf("*** Desktop Handler started OK\n");

	return TRUE;
}

BOOL handlerAddUser(void)
{
	struct MsgPort *port;
	struct DesktopInternMsg msg;

	kprintf("/// Attempting to obtain semaphore\n");
	if(!AttemptSemaphoreShared(&DesktopBase->db_HandlerSafety))
		return FALSE;

	port=CreateMsgPort();
	if(port)
	{
		msg.di_Message.mn_Node.ln_Type=NT_MESSAGE;
		msg.di_Message.mn_ReplyPort=port;
		msg.di_Message.mn_Length=sizeof(struct DesktopInternMsg);
		msg.di_Command=DIMC_ADDUSER;

		PutMsg(DesktopBase->db_HandlerPort, (struct Message*)&msg);

		kprintf("/// addmsg: awaitng reply from handler\n");
		WaitPort(port);
		GetMsg(port);
	}
	kprintf("/// addmsg: got reply, releasing semaphore\n");

	ReleaseSemaphore(&DesktopBase->db_HandlerSafety);

	return TRUE;
}

BOOL handlerSubUser(void)
{
	struct MsgPort *port;
	struct DesktopInternMsg msg;

	port=CreateMsgPort();
	if(port)
	{
		msg.di_Message.mn_Node.ln_Type=NT_MESSAGE;
		msg.di_Message.mn_ReplyPort=port;
		msg.di_Message.mn_Length=sizeof(struct DesktopInternMsg);
		msg.di_Command=DIMC_SUBUSER;

		PutMsg(DesktopBase->db_HandlerPort, (struct Message*)&msg);

		WaitPort(port);
		GetMsg(port);
	}

	return TRUE;
}

struct HandlerScanRequest* createScanMessage(ULONG command, struct MsgPort *replyPort, BPTR dirLock, Object *callback)
{
	struct HandlerScanRequest *hsr;

	hsr=(struct HandlerScanRequest*)AllocVec(sizeof(struct HandlerScanRequest), MEMF_ANY);
	hsr->hsr_Message.di_Message.mn_Length=sizeof(struct HandlerScanRequest);
	hsr->hsr_Message.di_Message.mn_Node.ln_Type=NT_MESSAGE;
	hsr->hsr_Message.di_Message.mn_ReplyPort=replyPort;
	hsr->hsr_Message.di_Command=command;
	hsr->hsr_CallBack=callback;
	hsr->hsr_DirLock=dirLock;

	return hsr;
}

struct WorkingMessageNode* findWorkedMessage(struct MinList *list, ULONG id)
{
	struct WorkingMessageNode *wmn;
	BOOL found=FALSE;

	wmn=list->mlh_Head;
	while(!found && wmn->wm_Node.mln_Succ)
	{
		if(wmn->wm_ID==id)
			found=TRUE;
		else
			wmn=wmn->wm_Node.mln_Succ;
	}

	return wmn;
}


