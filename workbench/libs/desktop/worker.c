#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "worker.h"

#include "desktop_intern.h"

#include "desktop_intern_protos.h"

#include <stdarg.h>

/*
FUTURE WORK:
1.  The worker should work inbetween messages from a requester.
    This will give faster results.
2.  ExAll() seems to to return eac->eac_Entries as 1 more than
    the actual amount in the buffer
3.  The WorkerMessage structure is a bit bloated, perhaps have
    a seperate message for starts?
*/

void scan(struct ScannerWorkerContext *swc)
{
	struct ExAllData *ead;
	struct SingleResult *sr=NULL;
	int i=0;

	swc->swc_More=ExAll(swc->swc_DirLock, (struct ExAllData*)swc->swc_Buffer, SCAN_BUFFER, ED_OWNER, swc->swc_EAC);

	if(swc->swc_EAC->eac_Entries)
	{
		ead=swc->swc_Buffer;
		sr=(struct SingleResult*)AllocVec(swc->swc_EAC->eac_Entries*sizeof(struct SingleResult), MEMF_ANY);
//		for(i=0; i<swc->swc_EAC->eac_Entries; i++)
		while(ead)
		{
			sr[i].sr_Name=ead->ed_Name;
			ead=ead->ed_Next;
			i++;
		}
	}

//	((struct WorkerScanRequest*)swc->swc_CurrentRequest)->wsr_Results=swc->swc_EAC->eac_Entries-1;
	((struct WorkerScanRequest*)swc->swc_CurrentRequest)->wsr_Results=i;
	((struct WorkerScanRequest*)swc->swc_CurrentRequest)->wsr_ResultsArray=sr;
	((struct WorkerScanRequest*)swc->swc_CurrentRequest)->wsr_More=swc->swc_More;

}

void startScan(struct ScannerWorkerContext *swc)
{
	swc->swc_Buffer=(STRPTR)AllocVec(SCAN_BUFFER, MEMF_ANY);
	swc->swc_DirLock=((struct WorkerScanRequest*)swc->swc_CurrentRequest)->wsr_DirLock;
	swc->swc_EAC=(struct ExAllControl*)AllocDosObject(DOS_EXALLCONTROL, NULL);
	swc->swc_EAC->eac_LastKey=0;

	scan(swc);
}

void resumeScan(struct ScannerWorkerContext *swc)
{
	scan(swc);
}

void stopScan(struct ScannerWorkerContext *swc)
{
	ExAllEnd(swc->swc_DirLock, (struct ExAllData*)swc->swc_Buffer, SCAN_BUFFER, ED_OWNER, swc->swc_EAC);
}

ULONG workerEntry(void)
{
	BOOL running=TRUE;
	struct MsgPort *port;
	struct WorkerMessage *msg;
	struct ScannerWorkerContext *swc=NULL;

	port=&((struct Process*)FindTask(NULL))->pr_MsgPort;

	while(running)
	{
		WaitPort(port);

		while(msg=(struct WorkerMessage*)(GetMsg(port)))
		{
			switch(msg->w_Command)
			{
				case WM_START:
				{
					swc=(struct ScannerWorkerContext*)AllocVec(sizeof(struct ScannerWorkerContext), MEMF_ANY);
					swc->swc_Context.workerAction=msg->w_Command;
					swc->swc_Context.start=(APTR)startScan;
					swc->swc_Context.resume=(APTR)resumeScan;
					swc->swc_Context.stop=(APTR)stopScan;
					swc->swc_EAC=NULL;
					swc->swc_DirLock=NULL;
					swc->swc_More=FALSE;
					swc->swc_Buffer=NULL;
					swc->swc_CurrentRequest=(struct WorkerMessage*)msg;
					swc->swc_Context.start(swc);
					if(!swc->swc_More)
						running=FALSE;

					break;
				}
				case WM_RESUME:
					swc->swc_CurrentRequest=msg;
					swc->swc_Context.resume(swc);
					if(!swc->swc_More)
						running=FALSE;
					break;

				case WM_STOP:
					if(msg)
						swc->swc_Context.stop(swc);
					running=FALSE;
					break;

				default:
					break;
			}

			ReplyMsg((struct Message*)msg);
			msg=NULL;
		}
	}

	return 0;
}

struct WorkerMessage* createWorkerScanMessage(ULONG workerCommand, ULONG workerAction, ULONG messageID, struct MsgPort *replyPort, BPTR dirLock)
{
	struct WorkerMessage *wm=NULL;
	struct WorkerScanRequest *sr;

	sr=AllocVec(sizeof(struct WorkerScanRequest), MEMF_ANY);
	sr->wsr_WMessage.w_Message.mn_Node.ln_Type=NT_MESSAGE;
	sr->wsr_WMessage.w_Message.mn_ReplyPort=replyPort;
	sr->wsr_WMessage.w_Message.mn_Length=sizeof(struct WorkerScanRequest);
	sr->wsr_WMessage.w_Command=workerCommand;

	sr->wsr_WMessage.w_Action=workerAction;
	sr->wsr_WMessage.w_ID=messageID;
	sr->wsr_DirLock=dirLock;
	wm=(struct WorkerMessage*)sr;

	return wm;
}

struct MsgPort* startScannerWorker(ULONG id, BPTR dirLock, struct MsgPort *replyPort)
{
	struct Process *process;
	struct ScannerWorkerMessage *msg;
	struct TagItem  procTags[]=
	{
        {NP_Entry, workerEntry},
        {NP_StackSize, 8192},
        {NP_Name, (IPTR)"Worker_Scanner"},
        {TAG_DONE, 0}
    };

	process=CreateNewProc(procTags);
	if(!process)
		return NULL;

	msg=createWorkerScanMessage(WM_START, WA_SCANNER, id, replyPort, dirLock);

	PutMsg(&process->pr_MsgPort, (struct Message*)msg);


	return &process->pr_MsgPort;
}

