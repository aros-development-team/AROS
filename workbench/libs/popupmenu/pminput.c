//
// PopupMenu
// ©1996-2002 Henrik Isaksson
//
// User Input & timer
//

#include "pmpriv.h"
#include "pminput.h"

//
// Input Handler
//

/// Input handler
struct InputEvent * __saveds ASM myhandler(register __a0 struct InputEvent *ev GNUCREG(a0),
    register __a1 struct MsgPort *port GNUCREG(a1))
{
    struct InputEvent *evnt=ev;

    do {
        if(evnt->ie_Class == IECLASS_RAWKEY) {
            struct PM_InpMsg *m;

            m=PM_Mem_Alloc(sizeof(struct PM_InpMsg));
            if(m) {
                m->msg.mn_Length=sizeof(struct PM_InpMsg);
                m->msg.mn_ReplyPort=NULL;
                m->Kind=0;
                switch(evnt->ie_Code) {
                    case 208:
                    case 0x45:
                            m->Kind=PM_MSG_TERMINATE;
                                  break;
                    case 0x4c:
                            m->Kind=PM_MSG_UP;
                            break;
                    case 0x4d:
                            m->Kind=PM_MSG_DOWN;
                            break;
                    case 0x4e:
                            m->Kind=PM_MSG_OPENSUB;
                            break;
                    case 0x4f:
                            m->Kind=PM_MSG_CLOSESUB;
                            break;
                    case 0x44:
                    case 0x43:
                            m->Kind=PM_MSG_SELECT;
                            break;
                    case 0x40:
                            m->Kind=PM_MSG_MULTISELECT;
                            break;
		    case 0x50:
			    m->Kind=PM_MSG_DEBUGINFO;
			    break;
                }
                if(m->Kind) PutMsg(port, (struct Message *)m);
                else PM_Mem_Free(m);
            }
            evnt->ie_Class = 0;
        }
        if(evnt->ie_Class == IECLASS_TIMER) {
            struct PM_InpMsg *m;

            m=PM_Mem_Alloc(sizeof(struct PM_InpMsg));
            if(m) {
                m->msg.mn_Length=sizeof(struct PM_InpMsg);
                m->msg.mn_ReplyPort=NULL;
                m->Kind=PM_MSG_TIMER;
                PutMsg(port, (struct Message *)m);
            }
        }
        if(evnt->ie_Class == IECLASS_RAWMOUSE) {
            struct PM_InpMsg *m;

            m=PM_Mem_Alloc(sizeof(struct PM_InpMsg));
            if(m) {
                m->msg.mn_Length=sizeof(struct PM_InpMsg);
                m->msg.mn_ReplyPort=NULL;
                m->Kind=PM_MSG_RAWMOUSE;
                m->Code=evnt->ie_Code;
                m->Qual=evnt->ie_Qualifier;
                PutMsg(port, (struct Message *)m);
            }

            evnt->ie_Code=IECODE_NOBUTTON;
        }
        evnt=evnt->ie_NextEvent;
    } while(evnt);

    return ev;
}
///

/// handlername
static char handlername[] = "PM Input Handler";
///

#ifdef __AROS__
AROS_UFH2(struct InputEvent *, myhandler_aros,
 AROS_UFHA(struct InputEvent *, ev, A0),
 AROS_UFHA(struct MsgPort *, port, A1))
{
    AROS_USERFUNC_INIT
    
    return myhandler(ev, port);
    
    AROS_USERFUNC_EXIT
}
#define HANDLER_CODE (APTR)AROS_ASMSYMNAME(myhandler_aros)
#else
#define HANDLER_CODE (void *)myhandler
#endif


/// Install handler
struct PM_InputHandler *PM_InstallHandler(int pri)
{
    struct PM_InputHandler *pmh;

    pmh=PM_Mem_Alloc(sizeof(struct PM_InputHandler));
    if(pmh) {
        pmh->mp=CreatePort(0,0);
        if(pmh->mp) {
            pmh->port=CreatePort(0,0);
            if(pmh->port) {
                pmh->ior=CreateStdIO(pmh->mp);
                if(pmh->ior) {
                    pmh->error=OpenDevice("input.device",0,(struct IORequest *)pmh->ior,0);
                    if(!pmh->error) {
                        pmh->intr.is_Data=(APTR)pmh->port;
                        pmh->intr.is_Code=HANDLER_CODE;
                        pmh->intr.is_Node.ln_Pri=pri;
                        pmh->intr.is_Node.ln_Name=handlername;
                        pmh->ior->io_Command=IND_ADDHANDLER;
                        pmh->ior->io_Data=(APTR)&pmh->intr;
                        pmh->ior->io_Message.mn_ReplyPort=pmh->mp;

                        DoIO((struct IORequest *)pmh->ior);

                    }
                }
            }
        }

    }

    return pmh;
}
///

/// Remove handler
void PM_RemoveHandler(struct PM_InputHandler *pmh)
{
    struct PM_InpMsg *msg;

    if(pmh->mp) {
        if(pmh->port) {
            if(pmh->ior) {
                if(!pmh->error) {
                    pmh->ior->io_Command=IND_REMHANDLER;
                    pmh->ior->io_Data=(APTR)&pmh->intr;
                    DoIO((struct IORequest *)pmh->ior);

                    CloseDevice((struct IORequest *)pmh->ior);
                }
                DeleteStdIO(pmh->ior);
            }
	    while((msg=(struct PM_InpMsg *)GetMsg(pmh->port))) {
		PM_Mem_Free(msg);
	    }
            DeletePort(pmh->port);
        }
        DeletePort(pmh->mp);
    }
    PM_Mem_Free(pmh);
}
///

//
// Timer
//

/// Timer functions
//
// timer funcs
//

void EZDeleteTimer(struct timerequest *TimeRequest)
{
    struct MsgPort *TimePort;

    if(TimeRequest)
    {
        if(TimeRequest->tr_node.io_Device)
            CloseDevice((struct IORequest *)TimeRequest);

        if((TimePort=TimeRequest->tr_node.io_Message.mn_ReplyPort))
            DeletePort(TimePort);

        DeleteExtIO((struct IORequest *)TimeRequest);
    }
}

struct timerequest *EZCreateTimer(LONG Unit)
{
    struct MsgPort      *TimePort;
    struct timerequest  *TimeRequest;

    if(!(TimePort = (struct MsgPort *)CreatePort(NULL,0)))
        return(NULL);

    if(!(TimeRequest = (struct timerequest *)CreateExtIO(TimePort,sizeof(struct timerequest))))
    {
        DeletePort(TimePort);

        return(NULL);
    }

    if(OpenDevice(TIMERNAME, Unit, (struct IORequest *)TimeRequest, 0))
    {
        DeleteExtIO((struct IORequest *)TimeRequest);
        DeletePort(TimePort);

        return(NULL);
    }

    return(TimeRequest);
}
///
