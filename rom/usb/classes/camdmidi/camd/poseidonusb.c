
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

char vers[];

/* The class overwrites the name after the marker with the device's. */
static struct
{
    char marker[CAMDUSB_NAMEMARKERSIZE];
    char name[32];
} drivername __attribute__((used)) =
{
    CAMDUSB_NAMEMARKER,
    "poseidonusb"
};

/*** Identification data must follow directly *********************************/

#define CAMDPORTCOUNT   16

/* Nothing here references it -- camd.library finds it by scanning the loaded
 * seglist for MDD_Magic -- so the compiler has to be told to keep it. */
static struct MidiDeviceData MidiDeviceData __attribute__((used)) =
{
  MDD_Magic,
  drivername.name,
  vers,
  1, 2,
  (APTR)AROS_SLIB_ENTRY(Init, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(Expunge, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(OpenPort, PoseidonCAMDUSB, 0),
  (APTR)AROS_SLIB_ENTRY(ClosePort, PoseidonCAMDUSB, 0),
  CAMDPORTCOUNT,
  1     /* the new driver format: AROS's camd.library loads no other */
};

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

/* A software interrupt (ca_CAMDTXFunc), Cause()d by ActivateXmit(): it gets
   is_Data as an interrupt handler does, not the arguments of a hook. */
static AROS_INTH1(GetFromCAMD, struct CAMDAdapter *, port)
{
    AROS_INTFUNC_INIT

    ULONG val, pos, sent = 0;
    struct MsgPort *mp = port->ca_MsgPort;

    camdTransmitFunc portTransmit = (camdTransmitFunc)port->ca_TXFunc;

    if (mp == NULL)
    {
        /* The device is unplugged. Drop what CAMD has queued, or a client
           sending to the port would wait for room forever. */
        while (portTransmit(port->ca_UserData) <= 0xff)
            ;
        return FALSE;
    }

    do
    {
        pos = port->ca_TXWritePos;
        if (((pos + 1) & (port->ca_TXBufSize - 1)) == port->ca_TXReadPos)
            break;
        val = portTransmit(port->ca_UserData);
        if (val > 0xff)
            break;
        port->ca_TXBuffer[pos] = val;
        pos++;
        pos &= port->ca_TXBufSize - 1;
        port->ca_TXWritePos = pos;
        sent++;
    } while (sent < 100); // TODO: Not sure what to do here??

    Signal(mp->mp_SigTask, (1 << mp->mp_SigBit));

    return FALSE;

    AROS_INTFUNC_EXIT
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

    CAMDPortBases[portnum] = usbCAMDOpenPort(transmitfunc, receivefunc, userdata, drivername.name, portnum);
    if (CAMDPortBases[portnum])
    {
        CAMDPortBases[portnum]->ca_ActivateFunc = ActivateXmit;
        CAMDPortBases[portnum]->ca_CAMDRXFunc.h_Entry = (HOOKFUNC)SendToCAMD;
        CAMDPortBases[portnum]->ca_CAMDRXFunc.h_Data = CAMDPortBases[portnum];
        CAMDPortBases[portnum]->ca_CAMDTXFunc.is_Code = (APTR)GetFromCAMD;
        CAMDPortBases[portnum]->ca_CAMDTXFunc.is_Data = CAMDPortBases[portnum];
        CAMDPortBases[portnum]->ca_IsOpen = TRUE;
    }
    return CAMDPortBases[portnum] ? (struct MidiPortData *)&CAMDPortBases[portnum]->ca_ActivateFunc : NULL;

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
        usbCAMDClosePort(portnum, drivername.name);
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
