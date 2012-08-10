#ifndef USBAUDIO_CLASS_H
#define USBAUDIO_CLASS_H

/*
 *----------------------------------------------------------------------------
 *                       Includes for usbaudio class
 *----------------------------------------------------------------------------
 *                   By Chris Hodges <chrisly@platon42.de>
 */

#include "common.h"

#include <devices/usb_audio.h>
#include <libraries/usbclass.h>

#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include <string.h>
#include <stddef.h>
#include <stdio.h>

#include "usbaudio.h"

#define SUBLIBBASETYPEPTR struct NepAudioSubLibBase *

/* Protos */

struct NepClassAudio * usbAttemptInterfaceBinding(struct NepAudioBase *nh, struct PsdInterface *pif);
struct NepClassAudio * usbForceInterfaceBinding(struct NepAudioBase *nh, struct PsdInterface *pif);
void usbReleaseInterfaceBinding(struct NepAudioBase *nh, struct NepClassAudio *nch);

AROS_UFP3(SUBLIBBASETYPEPTR, subLibInit,
          AROS_UFPA(SUBLIBBASETYPEPTR, nas, D0),
          AROS_UFPA(BPTR, seglist, A0),
          AROS_UFPA(struct ExecBase *, SysBase, A6));

AROS_LD1(SUBLIBBASETYPEPTR, subLibOpen,
         AROS_LDA(ULONG, version, D0),
         SUBLIBBASETYPEPTR, nas, 1, nep);
         
AROS_LD0(BPTR, subLibClose,
         SUBLIBBASETYPEPTR, nas, 2, nep);
         
AROS_LD1(BPTR, subLibExpunge,
         AROS_LDA(SUBLIBBASETYPEPTR, extralh, D0),
         SUBLIBBASETYPEPTR, nas, 3, nep);
         
AROS_LD0(SUBLIBBASETYPEPTR, subLibReserved,
         SUBLIBBASETYPEPTR, nas, 4, nep);

AROS_LD2(ULONG, subLibAllocAudio,
         AROS_LDA(struct TagItem *, tags, A1),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 5, nep);    

AROS_LD1(void, subLibFreeAudio,
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 6, nep);

AROS_LD1(void, subLibDisable,
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 7, nep);

AROS_LD1(void, subLibEnable,
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 8, nep);

AROS_LD2(ULONG, subLibStart,
         AROS_LDA(ULONG, flags, D0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 9, nep);

AROS_LD2(ULONG, subLibUpdate,
         AROS_LDA(ULONG, flags, D0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 10, nep);

AROS_LD2(ULONG, subLibStop,
         AROS_LDA(ULONG, flags, D0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 11, nep);
         
AROS_LD5(ULONG, subLibSetVol,
         AROS_LDA(UWORD, channel, D0),
         AROS_LDA(Fixed, volume, D1),
         AROS_LDA(sposition, pan, D2),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LDA(ULONG, flags, D3),
         SUBLIBBASETYPEPTR, nas, 12, nep);

AROS_LD4(ULONG, subLibSetFreq,
         AROS_LDA(UWORD, channel, D0),
         AROS_LDA(ULONG, freq, D1),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LDA(ULONG, flags, D2),
         SUBLIBBASETYPEPTR, nas, 13, nep);

AROS_LD6(ULONG, subLibSetSound,
         AROS_LDA(UWORD, channel, D0),
         AROS_LDA(UWORD, sound, D1),
         AROS_LDA(ULONG, offset, D2),
         AROS_LDA(LONG, length, D3),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         AROS_LDA(ULONG, flags, D4),
         SUBLIBBASETYPEPTR, nas, 14, nep);

AROS_LD2(ULONG, subLibSetEffect,
         AROS_LDA(ULONG *, effect, A0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 15, nep);

AROS_LD4(ULONG, subLibLoadSound,
         AROS_LDA(UWORD, sound, D0),
         AROS_LDA(ULONG, type, D1),
         AROS_LDA(APTR, info, A0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 16, nep);

AROS_LD2(ULONG, subLibUnloadSound,
         AROS_LDA(UWORD, sound, D0),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 17, nep);

AROS_LD5(IPTR, subLibGetAttr,
         AROS_LDA(ULONG, attr, D0),
         AROS_LDA(LONG, arg, D1),
         AROS_LDA(LONG, defvalue, D2),
         AROS_LDA(struct TagItem *, tags, A1),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 18, nep);

AROS_LD3(IPTR, subLibHardwareControl,
         AROS_LDA(ULONG, attr, D0),
         AROS_LDA(LONG, arg, D1),
         AROS_LDA(struct AHIAudioCtrlDrv *, audioctrl, A2),
         SUBLIBBASETYPEPTR, nas, 19, nep);

AROS_UFIP(subLibPlayerIntV4);
          
AROS_UFIP(subLibPlayerIntV6);

AROS_UFIP(subLibPlayerIntDummy);

struct NepClassAudio * nAllocAudio(void);
void nFreeAudio(struct NepClassAudio *nch);

BOOL nLoadClassConfig(struct NepAudioBase *nh);
LONG nOpenCfgWindow(struct NepAudioBase *nh);

void nGUITaskCleanup(struct NepAudioBase *nh);

AROS_UFP0(void, nAudioTask);
AROS_UFP0(void, nGUITask);

AROS_UFP3(void, nOutReqHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFPA(struct IOUsbHWBufferReq *, ubr, A1));

AROS_UFP3(void, nInReqHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFPA(struct IOUsbHWBufferReq *, ubr, A1));

AROS_UFP3(void, nInDoneHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(struct IOUsbHWRTIso *, urti, A2),
          AROS_UFPA(struct IOUsbHWBufferReq *, ubr, A1));

AROS_UFP3(void, nReleaseHook,
          AROS_UFPA(struct Hook *, hook, A0),
          AROS_UFPA(APTR, prt, A2),
          AROS_UFPA(APTR, unused, A1));

#endif /* USBAUDIO_CLASS_H */
