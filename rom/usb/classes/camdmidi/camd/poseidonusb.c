
#include <proto/exec.h>
#include <proto/camdusbmidi.h>

#include <exec/types.h>
#include <midi/camddevices.h>
#include "camdusbmidi.h"

typedef ULONG (*camdTransmitFunc)(APTR driverdata);
typedef void (*camdReceiveFunc)(UWORD input, APTR driverdata);

extern AROS_UFP1(BOOL, AROS_SLIB_ENTRY(Init, PoseidonCAMDUSB, 0),
        AROS_UFPA(APTR, sysbase, A6));
extern AROS_UFP0(VOID, AROS_SLIB_ENTRY(Expunge, PoseidonCAMDUSB, 0));
extern AROS_UFP5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, PoseidonCAMDUSB, 0),
        AROS_UFPA(struct MidiDeviceData *, data, D1),
        AROS_UFPA(LONG, portnum, D1),
        AROS_UFPA(APTR, transmitfunc, D2),
        AROS_UFPA(APTR, receivefunc, D3),
        AROS_UFPA(APTR, userdata, A6));
extern AROS_UFP2(VOID, AROS_SLIB_ENTRY(ClosePort, PoseidonCAMDUSB, 0),
        AROS_UFPA(struct MidiDeviceData *   , data        , D1),
        AROS_UFPA(LONG, portnum    , D2));

VOID ActivateXmit(APTR userdata, LONG portnum);

char name[], vers[];

/*** Identification data must follow directly *********************************/

#define CAMDPORTCOUNT   16

static struct MidiDeviceData MidiDeviceData =
{
  MDD_Magic,
  name,
  vers,
  1, 2,
  (APTR)AROS_SLIB_ENTRY(Init, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(Expunge, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(OpenPort, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(ClosePort, PoseidonCAMDUSB, 0),
  CAMDPORTCOUNT,
  0
};

char name[] = "poseidonusb";
char vers[] = "$VER: Poseidon USB camdusbmidi.class driver 1.2 (14.02.2020)";

struct ExecBase *SysBase = NULL;
struct Library *nh = NULL;

AROS_UFH1(BOOL, AROS_SLIB_ENTRY(Init, PoseidonCAMDUSB, 0),
        AROS_UFHA(APTR, sysbase, A6))
{
    AROS_USERFUNC_INIT

#ifdef __AROS__
    SysBase = sysbase;
#else
    // sysbase is not valid in the original CAMD anyway
    SysBase = *(struct ExecBase**) 4;
#endif
    nh = OpenLibrary("camdusbmidi.class", 0);

    return (nh != NULL);

    AROS_USERFUNC_EXIT
}


AROS_UFH0(VOID, AROS_SLIB_ENTRY(Expunge, PoseidonCAMDUSB, 0))
{
    AROS_USERFUNC_INIT

    if (nh)
        CloseLibrary(nh);
    nh = NULL;

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, SendToCAMD,
    AROS_UFHA(struct Hook * , hook, A0),
    AROS_UFHA(void * , data, A2),
    AROS_UFHA(ULONG * , params, A1))
{
    AROS_USERFUNC_INIT
    
    struct CAMDAdapter *port = (struct CAMDAdapter *)hook->h_Data;
    ULONG len = *(ULONG *)data;
    UBYTE *msg = (UBYTE *)((IPTR)data + sizeof(ULONG));

    camdReceiveFunc portReceive = (camdReceiveFunc)port->ca_RXFunc;
    
    while (len > 0)
    {
        portReceive(*msg++, port->ca_UserData);

        len--;
    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3S(void, GetFromCAMD,
    AROS_UFHA(struct Hook * , hook, A0),
    AROS_UFHA(Object * , obj, A2),
    AROS_UFHA(struct CAMDAdapter *, port, A1))
{
    AROS_USERFUNC_INIT

    ULONG val, pos, sent = 0;

    camdTransmitFunc portTransmit = (camdTransmitFunc)port->ca_TXFunc;

    do
    {
        pos = port->ca_TXWritePos;
        val = portTransmit(port->ca_UserData);
        port->ca_TXBuffer[pos] = val;
        pos++;
        pos &= port->ca_TXBufSize;
        if (pos == port->ca_TXReadPos)
            break;
        port->ca_TXWritePos = pos;
        sent++;
    } while (sent < 100); // TODO: Not sure what to do here??

    Signal(port->ca_MsgPort->mp_SigTask, (1 << port->ca_MsgPort->mp_SigBit));

    AROS_USERFUNC_EXIT
}

struct CAMDAdapter *CAMDPortBases[CAMDPORTCOUNT] = { NULL };

AROS_UFH5(struct MidiPortData *, AROS_SLIB_ENTRY(OpenPort, PoseidonCAMDUSB, 0),
        AROS_UFHA(struct MidiDeviceData *, data, D1),
        AROS_UFHA(LONG, portnum, D1),
        AROS_UFHA(APTR, transmitfunc, D2),
        AROS_UFHA(APTR, receivefunc, D3),
        AROS_UFHA(APTR, userdata, A6))
{
    AROS_USERFUNC_INIT

    CAMDPortBases[portnum] = usbCAMDOpenPort(transmitfunc, receivefunc, userdata, name, portnum);
    if (CAMDPortBases[portnum])
    {
        CAMDPortBases[portnum]->ca_ActivateFunc = ActivateXmit;
        CAMDPortBases[portnum]->ca_CAMDRXFunc.h_Entry = (HOOKFUNC)SendToCAMD;
        CAMDPortBases[portnum]->ca_CAMDRXFunc.h_Data = CAMDPortBases[portnum];
        CAMDPortBases[portnum]->ca_CAMDTXFunc.is_Code = (APTR)GetFromCAMD;
        CAMDPortBases[portnum]->ca_CAMDTXFunc.is_Data = CAMDPortBases[portnum];
        CAMDPortBases[portnum]->ca_IsOpen = TRUE;
    }
    return (struct MidiPortData *)&CAMDPortBases[portnum]->ca_ActivateFunc;

    AROS_USERFUNC_EXIT
}

AROS_UFH2(VOID, AROS_SLIB_ENTRY(ClosePort, PoseidonCAMDUSB, 0),
        AROS_UFHA(struct MidiDeviceData *   , data        , D1),
        AROS_UFHA(LONG, portnum    , D2))
{
    AROS_USERFUNC_INIT

    if (CAMDPortBases[portnum])
    {
        CAMDPortBases[portnum]->ca_IsOpen = FALSE;
        usbCAMDClosePort(portnum, name);
    }
    CAMDPortBases[portnum] = NULL;

    AROS_USERFUNC_EXIT
}

VOID ActivateXmit(APTR userdata, LONG portnum)
{
    int i;
    for (i = 0; i < CAMDPORTCOUNT; i++)
    {
        if (CAMDPortBases[i])
            Cause(&CAMDPortBases[i]->ca_CAMDTXFunc);
    }
}
