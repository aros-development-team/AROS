
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/reqtools.h>

#include <proto/security.h>

#include <clib/alib_protos.h>

#include "security_intern.h"

/*
 *      Post a warning
 */

void Warn(struct SecurityBase *secBase, STRPTR msg, ...)
{
    struct rtHandlerInfo *hinfo;
    struct MsgPort *timerport;
    struct timerequest *timerequest;
    struct LocaleInfo li;
    ULONG timersig, ret;
    ULONG sigs=0;
    D(
        bug( DEBUG_NAME_STR " %s('%s')\n", __func__, msg);
        bug( DEBUG_NAME_STR " %s: secBase @ %p\n", __func__, secBase);
    )

    if ((timerport = CreateMsgPort())!=NULL) {
        if ((timerequest = CreateIORequest(timerport,
                                             sizeof(struct timerequest)))!=NULL) {
                D(bug( DEBUG_NAME_STR " %s: timerequest @ %p\n", __func__, timerequest);)
                if (!OpenDevice(TIMERNAME, UNIT_VBLANK,
                             (struct IORequest *)timerequest, 0)) {
                     struct TagItem rtTags[] =
                     {
                        { RTEZ_ReqTitle,        0                       },
                        { RTEZ_Flags,           EZREQF_CENTERTEXT       },
                        { RT_ReqPos,            REQPOS_CENTERSCR        },
                        { RT_ReqHandler,        (IPTR)&hinfo            },
                        { TAG_DONE,             0                       }
                    };
                    D(bug( DEBUG_NAME_STR " %s: timer opened\n", __func__);)
                    OpenLoc(secBase, &li);
                    rtTags[0].ti_Data = (IPTR)GetLocS(secBase, &li, MSG_WARNING_GUI);
                    D(bug( DEBUG_NAME_STR " %s: request title '%s'\n", __func__, (char *)rtTags[0].ti_Data);)
                    D(bug( DEBUG_NAME_STR " %s: ReqToolsBase @ %p\n", __func__, ReqToolsBase);)
                    D(bug( DEBUG_NAME_STR " %s: showing EZRequest...\n", __func__);)
                    AROS_SLOWSTACKFORMAT_PRE(msg)
                    ret = rtEZRequestA(msg, GetLocS(secBase, &li, MSG_RESUME), NULL, AROS_SLOWSTACKFORMAT_ARG(msg), rtTags);
                    AROS_SLOWSTACKFORMAT_POST(msg)
                    if (ret == CALL_HANDLER) {
                        D(bug( DEBUG_NAME_STR " %s: adding timer request for 30secs...\n", __func__);)
                        timerequest->tr_node.io_Command = TR_ADDREQUEST;
                        timerequest->tr_time.tv_secs = 30;
                        timerequest->tr_time.tv_micro = 0;
                        SendIO((struct IORequest *)timerequest);
                        timersig = 1<<timerport->mp_SigBit;
                        do {
                            if (!hinfo->DoNotWait)
                            {
                                D(bug( DEBUG_NAME_STR " %s: calling Wait...\n", __func__);)
                                sigs = Wait(hinfo->WaitMask | timersig);
                            }
                            if (sigs & timersig)
                            {
                                 struct TagItem rtHandlerTags[] =
                                 {
                                    { RTRH_EndRequest,      TRUE        },
                                    { TAG_DONE,             0           }
                                };
                                D(bug( DEBUG_NAME_STR " %s: calling rtReqHandler (EndRequest)...\n", __func__);)
                                ret = rtReqHandlerA(hinfo, sigs, rtHandlerTags);
                            }
                            else
                            {
                                 struct TagItem rtHandlerTags[] =
                                 {
                                    { TAG_DONE,             0   }
                                };
                                D(bug( DEBUG_NAME_STR " %s: calling rtReqHandler...\n", __func__);)
                                ret = rtReqHandlerA(hinfo, sigs, rtHandlerTags);
                            }
                            D(bug( DEBUG_NAME_STR " %s: calling ret = %x\n", __func__, ret);)
                        } while (ret == CALL_HANDLER);
                        AbortIO((struct IORequest *)timerequest);
                        WaitIO((struct IORequest *)timerequest);
                    }
                    CloseLoc(&li);
                    CloseDevice((struct IORequest*)timerequest);
            }
            DeleteIORequest(timerequest);
        }
        DeleteMsgPort(timerport);
    }
}

/*
 *      Painless dead :-)
 */

void Die(STRPTR msg, ULONG code)
{
    D(bug( DEBUG_NAME_STR " %s()\n", __func__);)

    if (code)
        Alert(code);
    else {
        static UBYTE string[] = {
                0, 236, 19, 'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ',
                                                'F', 'a', 't', 'a', 'l', ' ',
                                                'E', 'r', 'r', 'o', 'r', 0, 1,
                0, 0, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
        int xpos;

        xpos = 320-4*strlen(msg);
        string[25] = xpos>>8;
        string[26] = xpos & 0xff;
        strncpy(&string[28], msg, 78);
        DisplayBeep(NULL);
        Delay(20);
        DisplayAlert(RECOVERY_ALERT, string, 51);
    }

    for (;;)
        Wait(0);
}
