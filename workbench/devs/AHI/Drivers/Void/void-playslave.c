
#include <config.h>

#include <devices/ahi.h>
#include <devices/timer.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>
#include <proto/timer.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct VoidData*) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

void Slave(struct ExecBase *SysBase);

#if defined( __AROS__ )

#include <aros/asmcall.h>

AROS_UFH3(void, SlaveEntry,
          AROS_UFHA(STRPTR, argPtr, A0),
          AROS_UFHA(ULONG, argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    Slave(SysBase);
    AROS_USERFUNC_EXIT
}

#else

void SlaveEntry(void)
{
    struct ExecBase *SysBase = *((struct ExecBase **) 4);

    Slave(SysBase);
}
#endif

void
Slave(struct ExecBase *SysBase)
{
    struct AHIAudioCtrlDrv *AudioCtrl;
    struct DriverBase      *AHIsubBase;
    struct MsgPort         *timerport;
    struct timerequest     *timereq = NULL;
    struct Device          *TimerBase = NULL;
    struct EClockVal        ev;
    UQUAD                   deadline, now, rem = 0, delay;
    ULONG                   efreq;
    BOOL                    running;
    ULONG                   signals;

    /* Note that in OS4, we cannot call FindTask(NULL) here, since IExec
     * is inside AHIsubBase! */
    AudioCtrl  = (struct AHIAudioCtrlDrv *) FindTask(NULL)->tc_UserData;
    AHIsubBase = (struct DriverBase *) dd->ahisubbase;

    dd->slavesignal = AllocSignal(-1);

    // There is no hardware to set the pace, so the timer stands in for it
    timerport = CreateMsgPort();
    if(timerport != NULL) {
        timereq = (struct timerequest *) CreateIORequest(timerport, sizeof(struct timerequest));
    }
    if(timereq != NULL &&
       OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *) timereq, 0) == 0) {
        TimerBase = timereq->tr_node.io_Device;
    }

    if(dd->slavesignal != -1 && TimerBase != NULL) {
        // Everything set up. Tell Master we're alive and healthy.

        Signal((struct Task *) dd->mastertask,
               1L << dd->mastersignal);

        efreq    = ReadEClock(&ev);
        deadline = ((UQUAD) ev.ev_hi << 32) | ev.ev_lo;

        running = TRUE;

        while(running) {
            signals = SetSignal(0L, 0L);

            if(signals & (SIGBREAKF_CTRL_C | (1L << dd->slavesignal))) {
                running = FALSE;
            } else {
                CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
                CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer);

                // The mixing buffer is now filled with AudioCtrl->ahiac_BuffSamples
                // of sample frames (type AudioCtrl->ahiac_BuffType). Discard
                // them, but take as long as playing them would have.

                // The remainder keeps rates that don't divide evenly from drifting
                rem      += (UQUAD) AudioCtrl->ahiac_BuffSamples * efreq;
                deadline += rem / AudioCtrl->ahiac_MixFreq;
                rem      %= AudioCtrl->ahiac_MixFreq;

                ReadEClock(&ev);
                now = ((UQUAD) ev.ev_hi << 32) | ev.ev_lo;

                if(deadline > now) {
                    delay = (deadline - now) * 1000000 / efreq;
                    timereq->tr_node.io_Command = TR_ADDREQUEST;
                    timereq->tr_time.tv_secs    = delay / 1000000;
                    timereq->tr_time.tv_micro   = delay % 1000000;
                    DoIO((struct IORequest *) timereq);
                } else if(now - deadline > efreq / 4) {
                    // Far behind, e.g. starved of CPU: resync, don't burst
                    deadline = now;
                }
            }
        }
    }

    if(TimerBase != NULL) {
        CloseDevice((struct IORequest *) timereq);
    }
    DeleteIORequest((struct IORequest *) timereq);
    DeleteMsgPort(timerport);

    FreeSignal(dd->slavesignal);
    dd->slavesignal = -1;

    Forbid();

    // Tell the Master we're dying

    Signal((struct Task *) dd->mastertask,
           1L << dd->mastersignal);

    dd->slavetask = NULL;

    // Multitaking will resume when we are dead.
}
