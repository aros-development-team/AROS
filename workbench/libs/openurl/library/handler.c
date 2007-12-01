/*
**  OpenURL-Handler - Asynch ARexx handler for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
*/


#include "lib.h"

/***********************************************************************/

static ULONG
sendRexxMsg(struct MsgPort *reply,UBYTE *rxport,UBYTE *rxcmd)
{
    struct RexxMsg *rxmsg;

    if (rxmsg = CreateRexxMsg(reply,NULL,NULL))
    {
        rxmsg->rm_Action = RXCOMM|RXFF_STRING|RXFF_NOIO;

        if (rxmsg->rm_Args[0] = CreateArgstring(rxcmd,strlen(rxcmd)))
        {
            struct MsgPort *port;

            Forbid();

            if (port = FindPort(rxport))
            {
                PutMsg(port,(struct Message *)rxmsg);
                Permit();

                return TRUE;
            }

            Permit();

            DeleteArgstring(rxmsg->rm_Args[0]);
        }

        DeleteRexxMsg(rxmsg);
    }

    return FALSE;
}

/**************************************************************************/

#ifdef __MORPHOS__
void handler(void)
#else
void SAVEDS handler(void)
#endif
{
    struct MsgPort   port;
    struct Process   *me = (struct Process *)FindTask(NULL);
    struct startMsg  *smsg;
    ULONG            res;
    int              sig;

    WaitPort(&me->pr_MsgPort);
    smsg = (struct startMsg *)GetMsg(&me->pr_MsgPort);

    if ((sig = AllocSignal(-1))>=0)
    {
        INITPORT(&port,sig);
    res = sendRexxMsg(&port,smsg->port,smsg->cmd);
    }
    else res = FALSE;

    smsg->res = res;
    ReplyMsg((struct Message *)smsg);

    if (res)
    {
        struct RexxMsg *rxmsg;

        WaitPort(&port);
    rxmsg = (struct RexxMsg *)GetMsg(&port);

        DeleteArgstring(rxmsg->rm_Args[0]);
        DeleteRexxMsg(rxmsg);
    }

    if (sig>=0) FreeSignal(sig);

    Forbid();
    lib_use--;
}

/**************************************************************************/

