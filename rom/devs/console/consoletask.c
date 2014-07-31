/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code executed by the console.device task.
    Lang: english
*/

#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/console.h>
#include <exec/io.h>
#include <exec/alerts.h>
#include <dos/dos.h>
#include <devices/input.h>
#include <devices/rawkeycodes.h>

#include "console_gcc.h"
#include "consoleif.h"

#define DEBUG 0
#include <aros/debug.h>

VOID con_inject(struct ConsoleBase *ConsoleDevice, struct ConUnit *cu,
    const UBYTE *data, LONG size)
{
    struct intConUnit *icu = ICU(cu);
    ObtainSemaphore(&ConsoleDevice->consoleTaskLock);
    if (data && size)
    {
        struct intPasteData *p =
            AllocMem(sizeof(struct intPasteData), MEMF_ANY);
        if (p)
        {
            if (size < 0)
                size = strlen(data);
            p->pasteBuffer = AllocMem(size, MEMF_ANY);
            if (p->pasteBuffer)
            {
                CopyMem((APTR) data, p->pasteBuffer, size);
                p->pasteBufferSize = size;
                p->unit = icu;
                AddTail((struct List *)&icu->pasteData, (struct Node *)p);
                D(bug("con_inject(%x %x %x %d)\n", icu, p, p->pasteBuffer,
                        p->pasteBufferSize));
            }
            else
            {
                FreeMem(p, sizeof(struct intPasteData));
            }
        }
    }
    ReleaseSemaphore(&ConsoleDevice->consoleTaskLock);
    /* Wake up possible waiting CMD_READs */
    Signal(ConsoleDevice->consoleTask, SIGBREAKF_CTRL_D);
}

/* Protos */
static BOOL checkconunit(Object *unit, struct ConsoleBase *ConsoleDevice);
static void answer_read_request(struct IOStdReq *req,
    struct ConsoleBase *ConsoleDevice);

static VOID answer_requests(APTR unit, struct ConsoleBase *ConsoleDevice)
{
    struct IOStdReq *req, *nextreq;
    /* See if there are any queued io read requests that wants to be replied */
    ForeachNodeSafe(&ConsoleDevice->readRequests, req, nextreq)
    {
        if ((APTR) req->io_Unit == (APTR) unit)
        {
            /* Paranoia */
            if (0 != ICU(req->io_Unit)->numStoredChars)
            {
                Remove((struct Node *)req);
                answer_read_request(req, ConsoleDevice);
            }
        }
    }
}

static ULONG pasteData(struct intConUnit *icu,
    struct ConsoleBase *ConsoleDevice)
{
    /* Check if we have data to paste to the input buffer */
    struct intPasteData *pd =
        (struct intPasteData *)GetHead(&icu->pasteData);
    if (pd)
    {
        ULONG tocopy =
            MIN(pd->pasteBufferSize - icu->pasteBufferPos,
            CON_INPUTBUF_SIZE - icu->numStoredChars);

        D(bug("Pasting %ld bytes\n", tocopy));
        CopyMem(pd->pasteBuffer + icu->pasteBufferPos,
            icu->inputBuf + icu->numStoredChars, tocopy);
        icu->numStoredChars += tocopy;
        icu->pasteBufferPos += tocopy;

        if (icu->pasteBufferPos >= pd->pasteBufferSize)
        {
            FreeMem(pd->pasteBuffer, pd->pasteBufferSize);
            Remove((struct Node *)pd);
            FreeMem(pd, sizeof(struct intPasteData));
            icu->pasteBufferPos = 0;
        }

        answer_requests((APTR) icu, ConsoleDevice);
        return tocopy;
    }
    return 0;
}

VOID consoleTaskEntry(struct ConsoleBase *ConsoleDevice)
{
    BOOL success = FALSE;
    LONG waitsigs = 0L, wakeupsig, commandsig, inputsig;


    /* CD input handler puts data into this port */
    struct MsgPort *inputport;


    /* Used for input.device */
    struct MsgPort *inputmp;

    /* Add the CDInputHandler to input.device's list of input handlers */
    inputmp = CreateMsgPort();
    if (inputmp)
    {
        struct IOStdReq *inputio;
        inputio =
            (struct IOStdReq *)CreateIORequest(inputmp,
            sizeof(struct IOStdReq));
        if (inputio)
        {
            /* Open the input.device */
            if (!OpenDevice("input.device", -1, (struct IORequest *)inputio,
                    0UL))
            {
                /* Initialize the inputhandler itself */
                ConsoleDevice->inputHandler = initCDIH(ConsoleDevice);
                if (ConsoleDevice->inputHandler)
                {
                    inputio->io_Data = ConsoleDevice->inputHandler;
                    inputio->io_Command = IND_ADDHANDLER;

                    DoIO((struct IORequest *)inputio);
                    success = TRUE;
                }
                CloseDevice((struct IORequest *)inputio);

            }

            DeleteIORequest((struct IORequest *)inputio);
        }

        DeleteMsgPort(inputmp);
    }

    NEWLIST(&ConsoleDevice->readRequests);

    /* if not successful, throw an alert */
    if (!success)
    {
        Alert(AT_DeadEnd | AN_ConsoleDev | AG_OpenDev | AO_Unknown);
    }

    D(bug("Console task initialized\n"));


    /* Get console.device input handler's port */
    inputport =
        ((struct cdihData *)ConsoleDevice->inputHandler->is_Data)->
        inputPort;

    inputsig = 1 << inputport->mp_SigBit;
    commandsig = 1 << ConsoleDevice->commandPort->mp_SigBit;

    waitsigs = inputsig | commandsig | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D;

    for (;;)
    {
        wakeupsig = Wait(waitsigs);

        /* Anyone wanting to kill us ? */
        if (wakeupsig & SIGBREAKF_CTRL_C)
        {
            break;
        }

        ObtainSemaphore(&ConsoleDevice->consoleTaskLock);

        if (wakeupsig & SIGBREAKF_CTRL_D)
        {
            struct IOStdReq *req;
            BOOL active = TRUE;
            D(bug("SIGBREAKF_CTRL_D\n"));
            /* console.device pasted data? */
            while (active)
            {
                active = FALSE;
                ForeachNode(&ConsoleDevice->readRequests, req)
                {
                    if (pasteData(ICU(req->io_Unit), ConsoleDevice))
                    {
                        answer_requests(NULL, ConsoleDevice);
                        active = TRUE;
                        break;
                    }
                }
            }
        }

        if (wakeupsig & inputsig)
        {
            /* A message from the console device input handler */
            struct cdihMessage *cdihmsg;

            while ((cdihmsg = (struct cdihMessage *)GetMsg(inputport)))
            {
                /* Check that the ConUnit has not been disposed,
                   while the message was passed
                 */
                if (checkconunit(cdihmsg->unit, ConsoleDevice))
                {
                    switch (cdihmsg->ie.ie_Class)
                    {
                    case IECLASS_CLOSEWINDOW:
                        /* this is a hack. It would actually be up to the
                         * console.device user (CON: handler for example) to
                         * activate CLOSEWINDOW raw events (SET RAW EVENTS cmd)
                         * and then look out for this in the input stream
                         * (CMD_READ) */
                        /* fall through */

                    case IECLASS_RAWKEY:
                        {
#define MAPRAWKEY_BUFSIZE 80

                            UBYTE inputBuf[MAPRAWKEY_BUFSIZE + 1];
                            LONG actual;
                            ULONG tocopy;
                            struct InputEvent e;

                            /* Mouse wheel support */
                            if (cdihmsg->ie.ie_Code == RAWKEY_NM_WHEEL_UP ||
                                cdihmsg->ie.ie_Code == RAWKEY_NM_WHEEL_DOWN)
                            {
                                e.ie_EventAddress =
                                    (cdihmsg->ie.ie_Code ==
                                    RAWKEY_NM_WHEEL_UP) ? (APTR) 1 : (APTR)
                                    2;
                                e.ie_Class = IECLASS_GADGETDOWN;
                                Console_HandleGadgets(cdihmsg->unit, &e);
                                e.ie_Class = IECLASS_GADGETUP;
                                Console_HandleGadgets(cdihmsg->unit, &e);
                            }
                            else
                            {
                                /* Convert it to ANSI chars */

                                if (cdihmsg->ie.ie_Class ==
                                    IECLASS_CLOSEWINDOW)
                                {
                                    /* HACK */
                                    inputBuf[0] = 28;   /* CTRL-\ */
                                    actual = 1;
                                    /* HACK */
                                }
                                else
                                {
                                    actual =
                                        RawKeyConvert(&cdihmsg->ie,
                                        inputBuf, MAPRAWKEY_BUFSIZE, NULL);

                                    if (cdihmsg->ie.ie_Qualifier ==
                                        IEQUALIFIER_RCOMMAND && actual == 1)
                                    {
                                        switch (inputBuf[0])
                                        {
                                        case 'c':
                                            D(bug("Console_Copy\n"));
                                            Console_Copy(cdihmsg->unit);
                                            actual = 0;
                                            break;
                                        case 'v':
                                            D(bug("Console_Paste\n"));
                                            Console_Paste(cdihmsg->unit);
                                            /* We have likely put something
                                             * into the input buffer */
                                            if (ICU(cdihmsg->unit)->
                                                numStoredChars)
                                                answer_requests(cdihmsg->
                                                    unit, ConsoleDevice);
                                            actual = 0;
                                            break;
                                        }
                                    }
                                }

                                D(bug("RawKeyConvert returned %ld\n",
                                        actual));


                                if (actual > 0)
                                {
                                    /* Copy received characters to the console
                                     * unit input buffer. If the buffer is
                                     * full, then console input will be lost
                                     */
                                    tocopy =
                                        MIN(actual,
                                        CON_INPUTBUF_SIZE -
                                        ICU(cdihmsg->unit)->numStoredChars);

                                    /* Copy the input over to the unit's
                                     * buffer */
                                    CopyMem(inputBuf,
                                        ICU(cdihmsg->unit)->inputBuf +
                                        ICU(cdihmsg->unit)->numStoredChars,
                                        tocopy);

                                    ICU(cdihmsg->unit)->numStoredChars +=
                                        tocopy;

                                    answer_requests(cdihmsg->unit,
                                        ConsoleDevice);
                                } /* if (actual > 0) */
                            }
                        }
                        break;

                    case IECLASS_GADGETDOWN:
                    case IECLASS_GADGETUP:
                    case IECLASS_TIMER:
                    case IECLASS_RAWMOUSE:
                        Console_HandleGadgets(cdihmsg->unit,
                            &(cdihmsg->ie));
                        //.ie_Class, (APTR)cdihmsg->ie.ie_EventAddress);
                        break;

                    case IECLASS_REFRESHWINDOW:
                    case IECLASS_SIZEWINDOW:
                        {
                            /* Intentionally empty (Begin|End)Refresh() pair,
                             * as we are required to call them to keep
                             * Intuition informed, but we don't want to be
                             * restricted to rendering just in the damaged
                             * area (WFLG_NOCAREREFRESH is another possibility,
                             * but it's not our window). */
                            BeginRefresh(WINDOW(cdihmsg->unit));
                            EndRefresh(WINDOW(cdihmsg->unit), TRUE);

                            /* Refresh window contents */
                            Console_NewWindowSize(cdihmsg->unit);
                        }
                        break;
                    default:
                        D(bug("cdihmsg->ie.ie_Class = %d\n",
                                cdihmsg->ie.ie_Class));
                    } /* switch(cdihmsg->ie.ie_Class) */
                } /* if (checkconunit(cdihmsg->unit, ConsoleDevice)) */

                /* Tell the input handler we're done */
                ReplyMsg((struct Message *)cdihmsg);
            } /* while ((cdihmsg = (struct cdihMessage *)GetMsg(inputport))) */
        } /* if (wakeupsig & inputsig) */

        if (wakeupsig & commandsig)
        {
            /* We got a command from the outside. Investigate it */
            struct IOStdReq *req;

            while ((req =
                    (struct IOStdReq *)GetMsg(ConsoleDevice->commandPort)))
            {
                pasteData(ICU(req->io_Unit), ConsoleDevice);

                switch (req->io_Command)
                {
                case CMD_READ:
                    if (0 != ICU(req->io_Unit)->numStoredChars)
                    {
                        answer_read_request(req, ConsoleDevice);
                    }
                    else
                    {
                        /* Not enough bytes in the buffer to fill the request,
                         * put it on hold. ioReq already removed from the
                         * queue with GetMsg() */
                        AddTail((struct List *)&ConsoleDevice->readRequests,
                            (struct Node *)req);
                    }
                    break;

                default:
                    kprintf
                        ("!!! UNKNOWN COMMAND RECEIVED BY CONSOLE TASK !!!\n");
                    kprintf("!!! THIS SHOULD NEVER HAPPEN !!!\n");
                    break;
                }

            } /* while ((req = (struct IOStdReq *)GetMsg(ConsoleDevice->commandPort))) */

        } /* if (wakeupsig & commandsig) */

        ReleaseSemaphore(&ConsoleDevice->consoleTaskLock);
    } /* forever */

/* FIXME: Do cleanup here */
}

/********** checkconunit()  *******************************/

/* Checks that the supplied unit has not been disposed */
static BOOL checkconunit(Object *unit, struct ConsoleBase *ConsoleDevice)
{
    Object *o, *ostate;
    BOOL found = FALSE;

    ObtainSemaphoreShared(&ConsoleDevice->unitListLock);

    ostate = (Object *) ConsoleDevice->unitList.mlh_Head;
    while ((o = NextObject(&ostate)) && (!found))
    {
        if (o == unit)
        {
            found = TRUE;
        }
    }

    ReleaseSemaphore(&ConsoleDevice->unitListLock);
    return found;
}

/******** answer_read_request() ***************************/

static void answer_read_request(struct IOStdReq *req,
    struct ConsoleBase *ConsoleDevice)
{
    Object *unit;

    D(bug("answer_read_request\n"));
    /* This function assumes that there are at least one character
       available in the unitsinput buffer 
     */

    unit = (Object *) req->io_Unit;

    req->io_Actual = MIN(ICU(unit)->numStoredChars, req->io_Length);

    /* Copy characters from the buffer into the request */
    CopyMem((APTR) ICU(unit)->inputBuf, req->io_Data, req->io_Actual);

    if (ICU(unit)->numStoredChars > req->io_Length)
    {
        ULONG i;

        ICU(unit)->numStoredChars -= req->io_Actual;

        /* We have to move the rest of the bytes to the start
           of the buffer.

           NOTE: we could alternatively use a circular buffer
         */

        for (i = 0; i < ICU(unit)->numStoredChars; i++)
        {
            ICU(unit)->inputBuf[i] =
                ICU(unit)->inputBuf[i + req->io_Actual];
        }
    }
    else
    {
        /* No more unread characters in the buffer */
        ICU(unit)->numStoredChars = 0;

    }

    req->io_Error = 0;
    /* All done. Just reply the request */

    /* stegerg: caller of answer_read_request is responsible for the Remove,
       because the Remove must be used only if the req was in
       the readrequests list
       Remove((struct Node *)req);

     */

/* kprintf("receiving task=%s, sigbit=%d\n, mode=%d"
	, ((struct Task *)req->io_Message.mn_ReplyPort->mp_SigTask)->tc_Node.ln_Name
	, req->io_Message.mn_ReplyPort->mp_SigBit
	, req->io_Message.mn_ReplyPort->mp_Flags
);
*/
    ReplyMsg((struct Message *)req);

    return;
}
