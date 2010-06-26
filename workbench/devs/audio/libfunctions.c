/*
     Copyright 2010, The AROS Development Team. All rights reserved.
     $Id$
 */

/*
 * audio.device
 *
 * by Nexus Development 2003
 * coded by Emanuele Cesaroni
 *
 * $Id: libfunctions.c,v 1.5 2003/12/17 22:39:02 cesaroni Exp $
 */

#include "audio_intern.h"
#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>

#include LC_LIBDEFS_FILE

/***********************************************************************/

AROS_LH1(void, beginio,
        AROS_LHA(struct IOAudio *, ioreq, A1),
        struct audiobase *, AudioDevice, 5, Audio)
{
    AROS_LIBFUNC_INIT

    ioreq->ioa_Request.io_Error = 0;
    ioreq->ioa_Request.io_Message.mn_Node.ln_Type = NT_MESSAGE;

    D(bug("BeginIO: Task 0x%lx\n", FindTask(NULL)));

    switch(ioreq->ioa_Request.io_Command)
    {
        case CMD_CLEAR:
        D(bug("CMD_CLEAR\n"));
        audio_CLEAR(ioreq);
        break;

        case CMD_FLUSH:
        D(bug("CMD_FLUSH\n"));
        audio_FLUSH(ioreq);
        break;

        case CMD_READ:
        D(bug("CMD_READ\n"));
        audio_READ(ioreq);
        break;

        case CMD_RESET:
        D(bug("CMD_RESET\n"));
        audio_RESET(ioreq);
        break;

        case CMD_START:
        D(bug("CMD_START\n"));
        audio_START(ioreq);
        break;

        case CMD_STOP:
        D(bug("CMD_STOP\n"));
        audio_STOP(ioreq);
        break;

        case CMD_UPDATE:
        D(bug("CMD_UPDATE\n"));
        audio_UPDATE(ioreq);
        break;

        case CMD_WRITE:
        D(bug("CMD_WRITE\n"));
        audio_WRITE(ioreq);
        break;

        case ADCMD_ALLOCATE:
        audio_ALLOCATE(ioreq);
        break;

        case ADCMD_FINISH:
        D(bug("CMD_FINISH\n"));
        audio_FINISH(ioreq);
        break;

        case ADCMD_FREE:
        audio_FREE(ioreq);
        break;

        case ADCMD_LOCK:
        audio_LOCK(ioreq);
        break;

        case ADCMD_PERVOL:
        D(bug("CMD_PERVOL\n"));
        audio_PERVOL(ioreq);
        break;

        case ADCMD_SETPREC:
        D(bug("CMD_SETPREC\n"));
        audio_SETPREC(ioreq);
        break;

        case ADCMD_WAITCYCLE:
        D(bug("CMD_WAITCYCLE\n"));
        audio_WAITCYCLE(ioreq);
        break;

        default:
        ((struct IOAudio*) ioreq)->ioa_Request.io_Error = IOERR_NOCMD;

        if((ioreq->ioa_Request.io_Flags & IOF_QUICK) == 0)
        {
            ReplyMsg(&ioreq->ioa_Request.io_Message);
        }

        break;
    }

    AROS_LIBFUNC_EXIT
}

/***********************************************************************/

AROS_LH1(LONG, abortio,
        AROS_LHA(struct IOAudio *, ioreq, A1),
        struct audiobase *, AudioDevice, 6, Audio)
{
    AROS_LIBFUNC_INIT

    audio_ABORTIO(ioreq,global_eta);

    return 0;

    AROS_LIBFUNC_EXIT
}

/***********************************************************************/
static int GM_UNIQUENAME( Init)
(LIBBASETYPEPTR AudiolDevice)
{
    memset(&emasys0.es_devsem,0,sizeof(struct SignalSemaphore));
    InitSemaphore(&emasys0.es_devsem);

    return TRUE;
}

/***********************************************************************/
static int DevClose(struct IOAudio *MyIORequest, LIBBASETYPEPTR MyDevBase)
{
    UWORD Command;
    UBYTE Flags;
    BYTE Error;
    struct MsgPort *Msgport;
    struct Unit *Unita;

    ObtainSemaphore(&emasys0.es_devsem);

    D(bug("NEWD: CLOSEDEVICE called\n"));

    Command = MyIORequest->ioa_Request.io_Command;
    Flags = MyIORequest->ioa_Request.io_Flags;
    Error = MyIORequest->ioa_Request.io_Error;
    Msgport = MyIORequest->ioa_Request.io_Message.mn_ReplyPort;
    Unita = MyIORequest->ioa_Request.io_Unit;

    MyIORequest->ioa_Request.io_Unit = (struct Unit*) 15;
    MyIORequest->ioa_Request.io_Command = ADCMD_FREE;
    //MyIORequest->ioa_Request.io_Message.mn_ReplyPort = NULL;
    MyIORequest->ioa_Request.io_Flags = IOF_QUICK;
    //MyIORequest->ioa_Request.io_Flags                = 0;
    BeginIO(&MyIORequest->ioa_Request);
    WaitIO(&MyIORequest->ioa_Request);

    //WaitPort(MyIOreuest->ioa_Request.io_Message->mn_ReplyPort);

    //MyIORequest->ioa_Request.io_Flags   = 0;								// I try the waiting method.
    //DoIO(&MyIORequest->ioa_Request);
    //BeginIO(&MyIORequest->ioa_Request);
    //WaitIO(&MyIORequest->ioa_Request);

    MyIORequest->ioa_Request.io_Device = (struct Device*) -1;
    MyIORequest->ioa_Request.io_Command = Command;
    MyIORequest->ioa_Request.io_Flags = Flags;
    MyIORequest->ioa_Request.io_Error = Error;
    MyIORequest->ioa_Request.io_Message.mn_ReplyPort = Msgport;
    MyIORequest->ioa_Request.io_Unit = Unita;

    // If this is the last client closing, free allocated resources
    if (MyDevBase->Dev.dd_Library.lib_OpenCnt == 1)
    {
        FreeSLAVE(MyIORequest); /* break Forbid() */// 24.9 Changed the order so actually never modify this order!!!!!!
        FreeESYS(); // ok
    }

    ReleaseSemaphore(&emasys0.es_devsem);
    return TRUE;
}

/***********************************************************************/
static int GM_UNIQUENAME( Open)
(
        LIBBASETYPEPTR AudioDevice,
        struct IORequest *ioreq,
        ULONG unitnum,
        ULONG flags
)
{
    UBYTE tflags;
    UWORD oldcmd;
    struct IOAudio * audioio = (struct IOAudio *)ioreq;

    ObtainSemaphore(&emasys0.es_devsem);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 24.9 Ok each time OpenDevice() is called do these: set no error.

    audioio->ioa_Request.io_Error = 0; // Ok, called for now no errors.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 24.9 First thing.
    //      It the device is opened for the first time it needs to have the ESYS ready to accepts new task arriving.
    //      So if this is the first time please inits the ESYS system before all the other.

    if(AudioDevice->Dev.dd_Library.lib_OpenCnt == 0)
    {
        audioio->ioa_Request.io_Error = IOERR_OPENFAIL;

        if(InitESYS() == OKEY)
        {
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // 24.9 Well if no error i have the ESYS ready, and a task into the tasklist system.
            //      So please if the slave process isn't ready let
            //      it ready when the device is for the first time opened.
            if(InitSLAVE(audioio) == OKEY)
            {
                audioio->ioa_Request.io_Error = 0;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 24.9	This a well know piece of code still valid.
    //
    audioio->ioa_Request.io_Device = (struct Device*) AudioDevice;

    if(audioio->ioa_Request.io_Error == 0)
    {
        if(audioio->ioa_Length)
        {
            D(bug("NEWD: ADCMD_ALLOCATE during opendevice\n"));

            tflags = audioio->ioa_Request.io_Flags;
            oldcmd = audioio->ioa_Request.io_Command;
            audioio->ioa_Request.io_Command = ADCMD_ALLOCATE;
            audioio->ioa_Request.io_Flags = (IOF_QUICK | ADIOF_NOWAIT);
            //MyIORequest->ioa_Request.io_Flags   = 0;							// add.
            BeginIO(&audioio->ioa_Request); // add
            WaitIO(&audioio->ioa_Request); // add

            D(bug("NEWD: ADCMD_ALLOCATE after the well know situation\n"));

            //DoIO(&MyIORequest->ioa_Request);											// prev
            audioio->ioa_Request.io_Flags = tflags;
            audioio->ioa_Request.io_Command = oldcmd;
        }
    }

    if(audioio->ioa_Request.io_Error == 0)
    {
        AudioDevice->DevState &= ~DEVF_DELEXP;
    }
    else
    {
        DevClose(audioio, AudioDevice);
    }

    ReleaseSemaphore(&emasys0.es_devsem);

    return TRUE;
}

/***********************************************************************/

static int GM_UNIQUENAME( Close)
(
        LIBBASETYPEPTR AudioDevice,
        struct IORequest *ioreq
)
{
    return DevClose((struct IOAudio *)ioreq, AudioDevice);
}

ADD2INITLIB(GM_UNIQUENAME(Init), 0)
ADD2OPENDEV(GM_UNIQUENAME(Open), 0)
ADD2CLOSEDEV(GM_UNIQUENAME(Close), 0)
