
#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/classes.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "desktop_intern.h"
#include "support.h"
#include "worker.h"

#include "desktop_intern_protos.h"
#include "worker_protos.h"

#define HS_STARTING 1
#define HS_RUNNING  2
#define HS_STOPPING 3

void startedMessage(void)
{
	struct Message *m;

	m=(struct Message*)AllocVec(sizeof(struct Message), MEMF_ANY);
	m->mn_Node.ln_Type=NT_MESSAGE;
	m->mn_ReplyPort=DesktopBase->db_HandlerPort;
	m->mn_Length=sizeof(struct Message);

	PutMsg((struct MsgPort*)((struct Process*)FindTask(NULL))->pr_Task.tc_UserData, m);
}

ULONG desktopHandler(void)
{
	struct DesktopInternMsg *msg, *finalMsg;
	ULONG handlerState=HS_STARTING;
	BOOL replyNow=TRUE, running=TRUE;
	ULONG userCount=0, appCount=0;
	ULONG idCount=0;
	struct MinList workingMessages;

	NewList((struct List*)&workingMessages);

	// The library's OPEN vector gets a mutex on the library
	// base.  Trouble will come our way if someone started this
	// handler elsewhere
	DesktopBase->db_HandlerPort=CreateMsgPort();

	kprintf("--- starting desktop handler\n");

	// let the creater know that it's now safe to send the handler
	// messages
	startedMessage();

	kprintf("--- desktop handler accepting messages\n");

	while(running)
	{
		WaitPort(DesktopBase->db_HandlerPort);
		while(msg=((struct Message*)GetMsg(DesktopBase->db_HandlerPort)))
		{
			if(msg->di_Message.mn_Node.ln_Type==NT_MESSAGE)
			{
				if(handlerState==HS_STARTING)
				{
					switch(msg->di_Command)
					{
						case DIMC_ADDUSER:
							userCount++;
							replyNow=TRUE;
							handlerState=HS_RUNNING;
							break;
						default:
							break;
					}
				}
				else if(handlerState==HS_STOPPING)
				{
					switch(msg->di_Command)
					{
						case DIMC_ADDUSER:
							userCount++;
							replyNow=TRUE;
							handlerState=HS_RUNNING;
							break;
						default:
							break;
					}
				}
				else if(handlerState==HS_RUNNING)
				{
					switch(msg->di_Command)
					{
						case DIMC_ADDUSER:
						{
							userCount++;
							replyNow=TRUE;
							break;
						}
						case DIMC_SUBUSER:
						{
							kprintf("--- DIMC_SUBUSER\n");
							userCount--;
							if(userCount==0 && appCount==0)
							{
							kprintf("--- time to exit...\n");
								handlerState=HS_STOPPING;
								replyNow=TRUE;
							kprintf("--- attempting semaphore\n");
								if(AttemptSemaphore(&DesktopBase->db_BaseMutex))
								{
									if(AttemptSemaphore(&DesktopBase->db_HandlerSafety))
									{
										running=FALSE;
										replyNow=FALSE;
										finalMsg=msg;
								kprintf("--- lets quit!\n");
									}
									else
										ReleaseSemaphore(&DesktopBase->db_BaseMutex);
								}
							}
							break;
						}
						case DIMC_SCANDIRECTORY:
						{
							struct HandlerScanRequest *scanMsg=(struct HandlerScanRequest*)msg;
							struct WorkingMessageNode *wmn;

							wmn=AllocVec(sizeof(struct WorkingMessageNode), MEMF_ANY);
							wmn->wm_Working=(struct DesktopInternMsg*)scanMsg;
							wmn->wm_ID=++idCount;
							AddTail((struct List*)&workingMessages, (struct Node*)wmn);

							wmn->wm_Port=startScannerWorker(idCount, scanMsg->hsr_DirLock, DesktopBase->db_HandlerPort);

							replyNow=TRUE;

							break;
						}
						default:
							break;
					}
				}

				if(replyNow)
					ReplyMsg((struct Message*)msg);
			}
			else if(msg->di_Message.mn_Node.ln_Type==NT_REPLYMSG)
			{
				struct WorkerMessage *wm=(struct WorkerMessage*)msg;

				switch(wm->w_Action)
				{
					case WA_SCANNER:
					{
						struct WorkerMessage *newMsg;
						struct WorkingMessageNode *wmn;
						struct WorkerScanRequest *wsr=(struct WorkerScanRequest*)wm;
						UWORD i;

						wmn=findWorkedMessage(&workingMessages, wsr->wsr_WMessage.w_ID);

						//DoMethod(wmsg->wm_Working->hsr_CallBack, ICM_AddIcon, msg->wsr_Results, msg->wsr_ResultsArray);

						if(wsr->wsr_More)
						{
							newMsg=createWorkerScanMessage(WM_RESUME, WA_SCANNER, wmn->wm_ID, DesktopBase->db_HandlerPort, wsr->wsr_DirLock);
							PutMsg(wmn->wm_Port, (struct Message*)newMsg);
						}
						else
						{
							Remove((struct Node*)wmn);
							ReplyMsg((struct Message*)wmn->wm_Working);
						}
						break;
					}
				}
				FreeVec(msg);
			}
		}
	}

	kprintf("deleting port\n");
	DeleteMsgPort(DesktopBase->db_HandlerPort);
	DesktopBase->db_HandlerPort=NULL;

	kprintf("releasing semaphores\n");
	ReleaseSemaphore(&DesktopBase->db_HandlerSafety);
	ReleaseSemaphore(&DesktopBase->db_BaseMutex);

	kprintf("replying to msg\n");
	ReplyMsg((struct Message*)finalMsg);

	kprintf("--- shutting down desktop handler\n");

	return 0;
}


