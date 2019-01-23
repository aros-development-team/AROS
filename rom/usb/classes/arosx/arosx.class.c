/*
    Copyright © 2018-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Gamepad (XInput) USB class driver
    Lang: English
*/


#include <aros/libcall.h>
#include <devices/timer.h>

#include <proto/timer.h>

#include "debug.h"

#include "arosx.class.h"


struct AROSXBase * AROSXInit(void);

struct AROSXClassController *AROSXClass_CreateController(LIBBASETYPEPTR arosxb, UBYTE id);
struct AROSXClassController *AROSXClass_ConnectController(LIBBASETYPEPTR arosxb, UBYTE type);
void AROSXClass_DisconnectController(LIBBASETYPEPTR arosxb, struct AROSXClassController *arosxc);
void AROSXClass_DestroyController(LIBBASETYPEPTR arosxb, struct AROSXClassController *arosxc);
BOOL AROSXClass_SendEvent(LIBBASETYPEPTR arosxb, ULONG ehmt, APTR param1, APTR param2);



/* /// "Lib Stuff" */
static int libInit(LIBBASETYPEPTR arosxb)
{

    mybug(0, ("libInit arosxb: 0x%08lx SysBase: 0x%08lx\n", arosxb, SysBase));

    arosxb->tv_secs = 0;
    arosxb->tv_micro = 0;

    arosxb->arosxc_count = 0;

    arosxb->arosxc_0 = AROSXClass_CreateController(arosxb, 0);
    arosxb->arosxc_1 = AROSXClass_CreateController(arosxb, 1);
    arosxb->arosxc_2 = AROSXClass_CreateController(arosxb, 2);
    arosxb->arosxc_3 = AROSXClass_CreateController(arosxb, 3);

	InitSemaphore(&arosxb->arosxc_lock);
    InitSemaphore(&arosxb->event_lock);

    memset(&arosxb->event_reply_port, 0, sizeof(arosxb->event_reply_port));
    arosxb->event_reply_port.mp_Flags = PA_IGNORE;
    NewList(&arosxb->event_reply_port.mp_MsgList);

    NewList(&arosxb->event_port_list);

	arosxb->AROSXBase = AROSXInit();

#define AROSXBase   arosxb->AROSXBase

    if(!AROSXBase)
    {
        mybug(-1, ("libInit: MakeLibrary(\"arosx.library\") failed!\n"));
        return(FALSE);
    }

    AROSXBase->arosxb = arosxb;

    mybug(-1, ("AROSX: AROSXBase 0x%08lx\n", AROSXBase));
	//AROS_LC0(ULONG, Dummy1, LIBBASETYPEPTR, AROSXBase, 5, arosx);

    mybug(0, ("libInit: Ok\n"));

    return(TRUE);
}

static int libOpen(LIBBASETYPEPTR arosxb)
{
    mybug(0, ("libOpen arosxb: 0x%08lx\n", arosxb));
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR arosxb)
{
    mybug(10, ("libExpunge arosxb: 0x%08lx\n", arosxb));
    //CloseLibrary((struct Library *) UtilityBase);
    return(TRUE);
}

ADD2INITLIB(libInit, 0)
ADD2OPENLIB(libOpen, 0)
ADD2EXPUNGELIB(libExpunge, 0)
/* \\\ */

/*
 * ***********************************************************************
 * * Library functions                                                   *
 * ***********************************************************************
 */

#define ps ps
/* /// "usbAttemptInterfaceBinding()" */
struct AROSXClassController * usbAttemptInterfaceBinding(struct AROSXClassBase *arosxb, struct PsdInterface *pif)
{
    struct Library *ps;

    struct AROSXClassController *arosxc;
    
    IPTR ifclass;
    IPTR subclass;
    IPTR proto;

    struct PsdConfig *pc;
    struct PsdDevice *pd;

    struct PsdDescriptor *pdd;
    UBYTE                *xinput_desc;

    UBYTE buf[64];
    struct Task *tmptask;

    mybug(0, ("nepHidAttemptInterfaceBinding(%08lx)\n", pif));

    if((ps = OpenLibrary("poseidon.library", 4)))
    {

        psdGetAttrs(PGA_INTERFACE, pif,
                    IFA_Class, &ifclass,
                    IFA_SubClass, &subclass,
                    IFA_Protocol, &proto,
                    IFA_Config, &pc,
                    TAG_DONE);

        psdGetAttrs(PGA_CONFIG, pc,
                    CA_Device, &pd,
                    TAG_END);

        /*
            Check to see if it is believed to be an XInput Gamepad interface and if so store the XInput descriptor pdd and the data
             - We could extend this class to also house code for other XInput devices,
               but for now it's only XInput Gamepad
        */
        pdd = psdFindDescriptor(pd, NULL, DDA_DescriptorType, 33, DDA_Interface, pif, TAG_END);
        if(((ifclass != 255) || (subclass != 93) || (proto != 1) || (pdd == NULL)))
        {
            mybug(0, ("nepHidAttemptInterfaceBinding(%08lx) %d %d %d Nope!\n", pif, ifclass, subclass, proto));
            CloseLibrary(ps);
            return(NULL);
        }

        /*
            Make sure the XInput descriptor takes the form we expect
                [16]   33   16    1    1  [36] [129]  20    3    0    3   19    2    0    3    0
                [17]   33   16    1    1  [37] [129]  20    3    3    3    4   19    2    8    3    3
            XInput descriptor length has to match with the "nibble count"
                - XInput "USAGE" seems to take the form of nibbles on the bitmask
                - Nibble byte count seems to relate to the size of the descriptor (adjusted)
            TODO: Make the class bailout earlier if the interface isn't what we want and clean this mess
        */

        psdGetAttrs(PGA_DESCRIPTOR, pdd, DDA_DescriptorData, &xinput_desc, TAG_END);

        UBYTE nibble_check;
        nibble_check = ( (( (xinput_desc[5]>>1) + (xinput_desc[5] & 1) ) - 2) );

        mybug(0, ("nepHidAttemptInterfaceBinding(%08lx) Nibble check %d\n", pif, nibble_check));
        nDebugMem(ps, xinput_desc, xinput_desc[0]);

        if( (xinput_desc[6] != 129) | (nibble_check != xinput_desc[0]) ) 
        {
            mybug(-1, ("nepHidAttemptInterfaceBinding(%08lx) Not a gamepad! (that we know of...)\n", pif));
            CloseLibrary(ps);
            return(NULL);
        }

        if((arosxc = AROSXClass_ConnectController(arosxb, AROSX_CONTROLLER_TYPE_GAMEPAD)))
        {

            arosxc->Device = pd;
            arosxc->Interface = pif;

            psdSafeRawDoFmt(buf, 64, "arosx.class.gamepad.%01x", arosxc->id);
            arosxc->ReadySignal = SIGB_SINGLE;
            arosxc->ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, nHidTask, arosxc)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<arosxc->ReadySignal);
                if(arosxc->Task)
                {
                    arosxc->ReadySigTask = NULL;
                    //FreeSignal(arosxc->ReadySignal);
                    psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &arosxc->devname, TAG_END);

					psdSafeRawDoFmt(arosxc->name, 64, "%s (%01x)", arosxc->devname, arosxc->id);

                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Play it again, '%s'!", arosxc->name);

                    CloseLibrary(ps);
                    return(arosxc);
                }
            }
            arosxc->ReadySigTask = NULL;
            //FreeSignal(arosxc->ReadySignal);
            AROSXClass_DisconnectController(arosxb, arosxc);;
        }
        CloseLibrary(ps);
    }

    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct AROSXClassBase *arosxb, struct AROSXClassController *arosxc)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    mybug(0, ("nepHidReleaseInterfaceBinding(%08lx)\n", arosxc));

    /* Kill the GUITask */
    if(arosxc->GUITask)
    {
        Signal(arosxc->GUITask, SIGBREAKF_CTRL_C);
    }

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        arosxc->ReadySignal = SIGB_SINGLE;
        arosxc->ReadySigTask = FindTask(NULL);
        if(arosxc->Task)
        {
            Signal(arosxc->Task, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(arosxc->Task)
        {
            Wait(1L<<arosxc->ReadySignal);
        }
        //FreeSignal(arosxc->ReadySignal);
        psdGetAttrs(PGA_INTERFACE, arosxc->Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "'%s' fell silent!", devname);

        AROSXClass_DisconnectController(arosxb, arosxc);

        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, arosxb, 5, nep)
{
    AROS_LIBFUNC_INIT

    struct TagItem *ti;
    LONG count = 0;

    mybug(0, ("nepHidGetAttrsA(%ld, %08lx, %08lx)\n", type, usbstruct, tags));
    switch(type)
    {
        case UGA_CLASS:
             if((ti = FindTagItem(UCCA_Priority, tags)))
             {
                 *((SIPTR *) ti->ti_Data) = -100;
                 count++;
             }
             if((ti = FindTagItem(UCCA_Description, tags)))
             {
                 *((STRPTR *) ti->ti_Data) = "Gamepad (XInput)";
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasClassCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_HasBindingCfgGUI, tags)))
             {
                 *((IPTR *) ti->ti_Data) = TRUE;
                 count++;
             }
             if((ti = FindTagItem(UCCA_AfterDOSRestart, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             break;
         case UGA_BINDING:
             if((ti = FindTagItem(UCCA_UsingDefaultCfg, tags)))
             {
                 *((IPTR *) ti->ti_Data) = FALSE;
                 count++;
             }
             break;
    }
    return(count);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbSetAttrsA()" */
AROS_LH3(LONG, usbSetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, arosxb, 6, nep)
{
    AROS_LIBFUNC_INIT
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */

/* /// "usbDoMethodA()" */
AROS_LH2(IPTR, usbDoMethodA,
         AROS_LHA(ULONG, methodid, D0),
         AROS_LHA(IPTR *, methoddata, A1),
         LIBBASETYPEPTR, arosxb, 7, nep)
{
    AROS_LIBFUNC_INIT

    mybug(0, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(arosxb, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(arosxb, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(arosxb, (struct AROSXClassController *) methoddata[0]);
            return(TRUE);

        case UCM_OpenBindingCfgWindow:
            return(nOpenCfgWindow((struct AROSXClassController *) methoddata[0]));

        default:
            break;
    }
    return(0);
    AROS_LIBFUNC_EXIT
}
/* \\\ */



struct AROSXClassController *AROSXClass_CreateController(LIBBASETYPEPTR arosxb, UBYTE id) {

    struct AROSXClassController *arosxc;

    arosxc = AllocVec(sizeof(struct AROSXClassController), MEMF_ANY|MEMF_CLEAR);

    if(arosxc == NULL) {
        mybug(-1, ("[AROSXClass] AROSXClass_CreateController: Failed to create new controller structure for controller %01x\n", id));
        return NULL;
    } else {
        arosxc->id = id;

        arosxc->status.connected = FALSE;
        arosxc->status.wireless = FALSE;
        arosxc->status.signallost = FALSE;

        arosxc->arosxb = arosxb;

        mybug(-1, ("[AROSXClass] AROSXClass_CreateController: Created new controller structure %04lx for controller %01x\n", arosxc, arosxc->id));

        if (arosxc->TimerMP = CreatePort(NULL, 0)) {
            if (arosxc->TimerIO = (struct timerequest *)CreateExtIO(arosxc->TimerMP, sizeof(struct timerequest))) {
                if (!(OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)arosxc->TimerIO, 0))) {
                    arosxc->TimerBase = arosxc->TimerIO->tr_node.io_Device;

                    /*
                        Timestamp starts from zero once the first controller structure gets build
                    */
                    if((arosxb->tv_secs == 0) && (arosxb->tv_micro == 0)) {
                        #undef  TimerBase
                        #define TimerBase arosxc->TimerBase
                        struct timeval current;
                        GetSysTime(&current);
                        arosxb->tv_secs = current.tv_secs;
                        arosxb->tv_micro = current.tv_micro;

                        mybug(-1,("Initial timestamp %u %u\n", arosxb->tv_secs, arosxb->tv_micro));
                    }

                    arosxc->initial_tv_secs = arosxb->tv_secs;
                    arosxc->initial_tv_micro = arosxb->tv_micro;

                    FreeSignal(arosxc->TimerMP->mp_SigBit);
                    return arosxc;
                }
                DeleteExtIO((struct IORequest *)arosxc->TimerIO);
            }
            DeletePort(arosxc->TimerMP);
        }
    }

    AROSXClass_DestroyController(arosxb, arosxc);    

    return NULL;

}

/*
    Just does a FreeVec, no checks to see if someone is using it...
     - Implemented some sanity
*/
void AROSXClass_DestroyController(LIBBASETYPEPTR arosxb, struct AROSXClassController *arosxc) {

    UBYTE id;

    if(arosxc != NULL) {
        id = arosxc->id;

        ObtainSemaphore(&arosxb->arosxc_lock);
        if(id == 0) {
            FreeVec(arosxb->arosxc_0);
            arosxb->arosxc_0 = NULL;
        }else if(id == 1) {
            FreeVec(arosxb->arosxc_1);
            arosxb->arosxc_1 = NULL;
        }else if(id == 2) {
            FreeVec(arosxb->arosxc_2);
            arosxb->arosxc_2 = NULL;
        }else if(id == 3) {
            FreeVec(arosxb->arosxc_3);
            arosxb->arosxc_3 = NULL;
        }
        ReleaseSemaphore(&arosxb->arosxc_lock);
    }else{
        mybug(-1, ("[AROSXClass] AROSXClass_DestroyController: Called on non existing controller...\n"));
    }
}

struct AROSXClassController *AROSXClass_ConnectController(LIBBASETYPEPTR arosxb, UBYTE type) {

    struct AROSXClassController *arosxc;
    arosxc = NULL;

    ObtainSemaphore(&arosxb->arosxc_lock);

    if(arosxb->arosxc_count != 4) {
        if(arosxb->arosxc_0->status.connected == FALSE) {
            arosxc = arosxb->arosxc_0;
            mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 0\n"));
        }else if(arosxb->arosxc_1->status.connected == FALSE) {
            arosxc = arosxb->arosxc_1;
            mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 1\n"));
        }else if(arosxb->arosxc_2->status.connected == FALSE) {
            arosxc = arosxb->arosxc_2;
            mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 2\n"));
        }else if(arosxb->arosxc_3->status.connected == FALSE) {
            arosxc = arosxb->arosxc_3;
            mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 3\n"));
        }else {
            ReleaseSemaphore(&arosxb->arosxc_lock);
            mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: How did you get here? Failing...\n"));
            return NULL;
        }
    }else {
        ReleaseSemaphore(&arosxb->arosxc_lock);
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Controller count exceeded, failing...\n"));
        return NULL;
    }

    arosxb->arosxc_count++;

    arosxc->controller_type = type;

    arosxc->status.connected = TRUE;

    /*
        Send connect event from this controller.
         - If no event handler has been installed then the msg goes to nowhere and is lost
         - New connect event is sent when event handler is created including this controller
    */

    if(AROSXClass_SendEvent(arosxb, ((((1L<<arosxc->id))<<28) | ((arosxc->controller_type)<<20) | AROSX_EHMF_CONNECT), (APTR)1, (APTR)2)) {
        mybug(-1,("Attach event sent\n"));
    } else {
        mybug(-1,("Attach event not sent\n"));
    }

    ReleaseSemaphore(&arosxb->arosxc_lock);

    return arosxc;

}

void AROSXClass_DisconnectController(LIBBASETYPEPTR arosxb, struct AROSXClassController *arosxc) {

    if(arosxc != NULL) {
        ObtainSemaphore(&arosxb->arosxc_lock);

        arosxb->arosxc_count--;

        arosxc->status.connected = FALSE;

        if(AROSXClass_SendEvent(arosxb, ((((1L<<arosxc->id))<<28) | AROSX_EHMF_DISCONNECT), (APTR)1, (APTR)2)) {
            mybug(-1,("Detach event sent\n"));
        } else {
            mybug(-1,("Detach event not sent\n"));
        }

        arosxc->controller_type = AROSX_CONTROLLER_TYPE_UNKNOWN;
        ReleaseSemaphore(&arosxb->arosxc_lock);

        mybug(-1, ("[AROSXClass] AROSXClass_DisconnectController: Disconnected controller number %01x\n", arosxc->id));
    }

}

BOOL AROSXClass_SendEvent(LIBBASETYPEPTR arosxb, ULONG ehmt, APTR param1, APTR param2) {

    struct AROSX_EventNote *en;
    struct AROSX_EventHook *eh;
    ULONG msgmask = ehmt;

    BOOL ret = FALSE;

    while((en = (struct AROSX_EventNote *) GetMsg(&arosxb->event_reply_port))) {
        mybug(0, ("    Free SendEvent (%p)\n", en));
        FreeVec(en);
    }

    ObtainSemaphore(&arosxb->event_lock);
    eh = (struct AROSX_EventHook *) arosxb->event_port_list.lh_Head;
    while(eh->eh_Node.ln_Succ) {
        /*
            TODO: Make message event mask differentiate controller type also
        */
        if((eh->eh_MsgMask>>28) & (msgmask)>>28) {
            if((en = AllocVec(sizeof(struct AROSX_EventNote), MEMF_CLEAR|MEMF_ANY))) {
                en->en_Msg.mn_ReplyPort = &arosxb->event_reply_port;
                en->en_Msg.mn_Length = sizeof(struct AROSX_EventNote);
                en->en_Event = ehmt;
                en->en_Param1 = param1;
                en->en_Param2 = param2;
                mybug(0, ("[AROSXClass] SendEvent(%p, %p, %p)\n", ehmt, param1, param2));
                PutMsg(eh->eh_MsgPort, &en->en_Msg);
                ret = TRUE;
            }
        }
        eh = (struct AROSX_EventHook *) eh->eh_Node.ln_Succ;
    }
    ReleaseSemaphore(&arosxb->event_lock);

    if(ret) {
            mybug(0,("Event sent\n"));
    } else {
            mybug(0,("Event not sent\n"));
    }

    return ret;

}



/**************************************************************************/

#undef  ps
#define ps arosxc->Base
#undef  TimerBase
#define TimerBase arosxc->TimerBase

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct AROSXClassController *arosxc;
    struct AROSXClassBase *arosxb;

    struct PsdPipe *pp;

    ULONG sigmask;
    ULONG sigs;

    UBYTE *epinbuf;
	UBYTE *ep0buf;

    LONG ioerr;
    ULONG len;

    /*
        This does not allocate arosxc, it is already present. It only fetches it from the task tc_UserData.
         - Currently code assumes controller to be a gamepad
    */
    if((arosxc = nAllocHid()))
    {

        arosxb = arosxc->arosxb;

        arosxc->TimerMP->mp_SigBit = AllocSignal(-1);
        arosxc->TimerIO->tr_node.io_Message.mn_ReplyPort->mp_SigBit = arosxc->TimerMP->mp_SigBit;
        arosxc->TimerIO->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

    	epinbuf = arosxc->EPInBuf;
		ep0buf  = arosxc->EP0Buf;

        arosxc->status.signallost = TRUE;

        Forbid();
        if(arosxc->ReadySigTask)
        {
            Signal(arosxc->ReadySigTask, 1L<<arosxc->ReadySignal);
        }
        Permit();
        sigmask = (1L<<arosxc->TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;

        psdDelayMS(2000);

        psdPipeSetup(arosxc->EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE, 0x01, 0x0100, 0x00);
        do {
        	ioerr = psdDoPipe(arosxc->EP0Pipe, ep0buf, 20);
    	} while(ioerr);

    	mybug(0, ("EP0: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
            	    ep0buf[0], ep0buf[1], ep0buf[2], ep0buf[3], ep0buf[4], ep0buf[5], ep0buf[6], ep0buf[7], ep0buf[8], ep0buf[9], ep0buf[10],
        	        ep0buf[11], ep0buf[12], ep0buf[13], ep0buf[14], ep0buf[15], ep0buf[16], ep0buf[17], ep0buf[18], ep0buf[19]));

        arosxc->status.wireless = (ep0buf[18]&(1<<0))? TRUE:FALSE;

		/*
			:) First

            Wireless Logitech F710
			EP0: 00 14 ff f7 ff ff c0 ff c0 ff c0 ff c0 ff 00 00 00 00 01 00
			 - What we have here is a bitmask for all(?) the inputs
			 - We're the first, I think...
			 - If this holds true then the analog thumb stick values aren't exactly 16-bit wide :)
            EPIn: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

            Wired Logitech F310
            EP0: 00 14 ff f7 ff ff c0 ff c0 ff c0 ff c0 ff 00 00 00 00 00 00

            XInput descriptors for various gamepads, we have one for this interface in *xinput_desc as a UBYTE array
             - Check if it's an index to the bitmask

            [Gamepad F710 descriptors, wireless]
              0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
             16  33  16   1   1  36 129  20   3   0   3  19   2   0   3   0

            [Gamepad F510 descriptors]
              0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
             16  33  16   1   1  36 129  20   3   0   3  19   2   0   3   0

            [Gamepad F310 descriptors]
              0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
             16  33  16   1   1  36 129  20   3   0   3  19   2   0   3   0

            [Xbox360 Controller, other]
              0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
             16  33  16   1   1  36 129  20   3   0   3  19   2   0   3   0

            [Xbox360 Controller, v1.60]
              0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16
             17  33  16   1   1  37 129  20   3   3   3   4  19   2   8   3   3

		*/

        /*
        	Set led ring to gamepad number. Should flash for a while and then lid on constantly.
        */

    	UBYTE *bufout;
    	bufout = arosxc->EPOutBuf;

    	bufout[0] = 0x01;
    	bufout[1] = 0x03;
    	bufout[2] = arosxc->id + 2;
    	bufout[3] = 0x00;
    	bufout[4] = 0x00;
    	bufout[5] = 0x00;
    	bufout[6] = 0x00;
    	bufout[7] = 0x00;
		bufout[8] = 0x00;
    	bufout[9] = 0x00;
    	bufout[10] = 0x00;
		bufout[11] = 0x00;

    	psdDoPipe(arosxc->EPOutPipe, bufout, 12);

        psdSendPipe(arosxc->EPInPipe, epinbuf, 20);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(arosxc->TaskMsgPort)))
            {
                if(pp == arosxc->EPInPipe)
                {
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        len = psdGetPipeActual(pp);

                        if(Gamepad_ParseMsg(arosxc, epinbuf, len)) {
                            AROSXClass_SendEvent(arosxb, (((1L<<(arosxc->id)))<<28), (APTR)1, (APTR)2);
                            mybug(0,("Timestamp %u #%x\n", arosxc->arosx_gamepad.Timestamp, arosxc->id));
                        }

                        /* Wait */
                        /*
                        arosxc->TimerIO->tr_node.io_Command = TR_ADDREQUEST;
                        arosxc->TimerIO->tr_time.tv_secs = 0;
                        arosxc->TimerIO->tr_time.tv_micro = 1000;
                        DoIO((struct IORequest *)arosxc->TimerIO);
                        */

                    } else {
                        mybug(1, ("Int Pipe failed %ld\n", ioerr));
                        psdDelayMS(200);
                    }
                    /*
                    	TODO: One Chinese gamepad doesn't wait for new input but sends data back at once (8mS apart...)
                    		   - Check if response is the same and not much time has elapsed between and set some babble flag and force wait between calls
                    */
                    //psdDelayMS(1);
                    psdSendPipe(arosxc->EPInPipe, epinbuf, 20);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));

        mybug(-1, ("(%d) Going down the river!\n", arosxc->id));
        psdAbortPipe(arosxc->EPInPipe);
        psdWaitPipe(arosxc->EPInPipe);
        nFreeHid(arosxc);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "Gamepad_ParseMsg()" */
BOOL Gamepad_ParseMsg(struct AROSXClassController *arosxc, UBYTE *buf, ULONG len) {

    struct AROSX_GAMEPAD  arosx_gamepad_new;

    struct AROSX_GAMEPAD *arosx_gamepad;
    arosx_gamepad = &arosxc->arosx_gamepad;

    struct timeval current;

    BOOL ret = FALSE;

/* TODO: Check the input message type... */

/*
    Ta-daa!!
    When Logitech Wireless Gamepad F710 goes to sleep we get this in our endpoint
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 84 00 00 00 0 0 00
        bit 4 on byte 14

    And this is the first message after it wakes on "A" button press
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00
        followed by this, the "A" button msg
        Msg: 00 14 00 10 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Mode LED on
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 9c 00 55 00 00 00

    Mode LED off
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00

    Long vibration (Enables rumble effect on controller)
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Short vibration (Disables rumble effect on controller)
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 94 00 55 00 00 00

    Taking the controller out of range and we get this (same as removing the battery)
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 a4 00 00 00 00 00

    Taking the battery out and the dongle soon sends this
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 a4 00 00 00 00 00

    Re-inserting the battery and we get this
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 b4 00 55 00 00 00

    Pressing "A" on Logitech (wired) Gamepad F310
        Msg: 00 14 00 10 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00

    Toggling mode button on Logitech (wired) Gamepad F310 has no effect on the msg but we get one
        some bit from byte 14 and on could tell if it's a wireless or wired (bit 7 of byte 14)
        Byte 16 might be the battery level on Wireless F710?
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00
        Msg: 00 14 00 00 00 00 80 00 80 00 80 00 80 00 00 00 00 00 00 00

    mybug(0, ("EPIn: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
                    buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], buf[9], buf[10],
                    buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17], buf[18], buf[19]));
    */

    /*
        Works at least with Logitech F710
    */
	arosxc->status.signallost = (buf[14]&(1<<4))? FALSE:TRUE;

 	/*
    	This will map everything according to Microsoft game controller API
        Check if our gamepad needs a timestamp (change on inputs)
    */
    arosx_gamepad_new.Buttons = (UWORD)(buf[2]<<0) | (buf[3]<<8);
    if(arosx_gamepad_new.Buttons != arosx_gamepad->Buttons) {
        arosx_gamepad->Buttons = arosx_gamepad_new.Buttons;
        ret = TRUE;
    }

    arosx_gamepad_new.LeftTrigger = (UBYTE)(buf[4]);
    if(arosx_gamepad_new.LeftTrigger != arosx_gamepad->LeftTrigger) {
        arosx_gamepad->LeftTrigger = arosx_gamepad_new.LeftTrigger;
        ret = TRUE;
    }

    arosx_gamepad_new.RightTrigger = (UBYTE)(buf[5]);
    if(arosx_gamepad_new.RightTrigger != arosx_gamepad->RightTrigger) {
        arosx_gamepad->RightTrigger = arosx_gamepad_new.RightTrigger;
        ret = TRUE;
    }

    arosx_gamepad_new.ThumbLX = (WORD)((buf[6])  | (buf[7]<<8));
    if(arosx_gamepad_new.ThumbLX != arosx_gamepad->ThumbLX) {
        arosx_gamepad->ThumbLX = arosx_gamepad_new.ThumbLX;
        ret = TRUE;
    }

    arosx_gamepad_new.ThumbLY = (WORD)((buf[8])  | (buf[9]<<8));
    if(arosx_gamepad_new.ThumbLY != arosx_gamepad->ThumbLY) {
        arosx_gamepad->ThumbLY = arosx_gamepad_new.ThumbLY;
        ret = TRUE;
    }

    arosx_gamepad_new.ThumbRX = (WORD)((buf[10]) | (buf[11]<<8));
    if(arosx_gamepad_new.ThumbRX != arosx_gamepad->ThumbRX) {
        arosx_gamepad->ThumbRX = arosx_gamepad_new.ThumbRX;
        ret = TRUE;
    }

    arosx_gamepad_new.ThumbRY = (WORD)((buf[12]) | (buf[13]<<8));
    if(arosx_gamepad_new.ThumbRY != arosx_gamepad->ThumbRY) {
        arosx_gamepad->ThumbRY = arosx_gamepad_new.ThumbRY;
        ret = TRUE;
    }

    /* Rumble effect
    UBYTE *bufout;
    bufout = arosxc->EPOutBuf;

    bufout[0] = 0x00;
    bufout[1] = 0x08;
    bufout[2] = 0x00;
    bufout[3] = buf[6];
    bufout[4] = buf[7];
    bufout[5] = 0x00;
    bufout[6] = 0x00;
    bufout[7] = 0x00;
    psdDoPipe(arosxc->EPOutPipe, bufout, 8);
    */

    if(ret) {
        GetSysTime(&current);
        arosx_gamepad->Timestamp = (ULONG)((((current.tv_secs-arosxc->initial_tv_secs) * 1000000) + (current.tv_micro-arosxc->initial_tv_micro))/1000);
    }

    return ret;

}
/* \\\ */

/* /// "nAllocHid()" */
struct AROSXClassController * nAllocHid(void)
{
    struct Task *thistask;
    struct AROSXClassController *arosxc;

    thistask = FindTask(NULL);
    arosxc = thistask->tc_UserData;
    do
    {
        if(!(arosxc->Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        psdGetAttrs(PGA_INTERFACE, arosxc->Interface,
                    IFA_Config, &arosxc->Config,
                    IFA_InterfaceNum, &arosxc->IfNum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, arosxc->Config,
                    CA_Device, &arosxc->Device,
                    TAG_END);

        arosxc->EPIn = psdFindEndpoint(arosxc->Interface, NULL,
                                       EA_IsIn, TRUE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);

        arosxc->EPOut = psdFindEndpoint(arosxc->Interface, NULL,
                                       EA_IsIn, FALSE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);

        if((!arosxc->EPIn)|(!arosxc->EPOut))
        {
            mybug(1, ("Ooops!?! No Endpoints defined?\n"));
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "Failed to get endpoints!");
            break;
        }
        if((arosxc->InpMsgPort = CreateMsgPort()))
        {
            if((arosxc->InpIOReq = (struct IOStdReq *) CreateIORequest(arosxc->InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) arosxc->InpIOReq, 0))
                {
                    arosxc->InputBase = (struct Library *) arosxc->InpIOReq->io_Device;
                    if((arosxc->TaskMsgPort = CreateMsgPort()))
                    {
                        if((arosxc->EP0Pipe = psdAllocPipe(arosxc->Device, arosxc->TaskMsgPort, NULL)))
                        {
                            if((arosxc->EPInPipe = psdAllocPipe(arosxc->Device, arosxc->TaskMsgPort, arosxc->EPIn)))
                            {
                                psdSetAttrs(PGA_PIPE, arosxc->EPInPipe,
                                            PPA_NakTimeout, FALSE,
                                            PPA_AllowRuntPackets, TRUE,
                                            TAG_END);

                                if((arosxc->EP0Buf = psdAllocVec(100)))
                                {
                                	if((arosxc->EPInBuf = psdAllocVec(100)))
                                	{
                                    	if((arosxc->EPOutPipe = psdAllocPipe(arosxc->Device, arosxc->TaskMsgPort, arosxc->EPOut)))
                                    	{
                                        	psdSetAttrs(PGA_PIPE, arosxc->EPOutPipe,
                                            	PPA_NakTimeout, FALSE,
                                            	PPA_AllowRuntPackets, TRUE,
                                           		TAG_END);

                                        	if((arosxc->EPOutBuf = psdAllocVec(100)))
                                        	{
                                            	arosxc->Task = thistask;
                                            	return(arosxc);
                                        	}
                                        	psdFreePipe(arosxc->EPOutPipe);
                                    	}
                                    	psdFreeVec(arosxc->EPInBuf);
                                	}
                                	psdFreeVec(arosxc->EP0Buf);
                            	}
                                psdFreePipe(arosxc->EPInPipe);
                            }
                            psdFreePipe(arosxc->EP0Pipe);
                        }
                        DeleteMsgPort(arosxc->TaskMsgPort);
                    }
                    CloseDevice((struct IORequest *) arosxc->InpIOReq);
                }
                DeleteIORequest((struct IORequest *) arosxc->InpIOReq);
            }
            DeleteMsgPort(arosxc->InpMsgPort);
        }
    } while(FALSE);
    CloseLibrary(arosxc->Base);
    Forbid();
    arosxc->Task = NULL;
    if(arosxc->ReadySigTask)
    {
        Signal(arosxc->ReadySigTask, 1L<<arosxc->ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeHid()" */
void nFreeHid(struct AROSXClassController *arosxc)
{
    psdFreeVec(arosxc->EPOutBuf);
    psdFreeVec(arosxc->EPInBuf);

    psdFreePipe(arosxc->EPOutPipe);
    psdFreePipe(arosxc->EPInPipe);
    psdFreePipe(arosxc->EP0Pipe);

    DeleteMsgPort(arosxc->TaskMsgPort);
    CloseDevice((struct IORequest *) arosxc->InpIOReq);
    DeleteIORequest((struct IORequest *) arosxc->InpIOReq);
    DeleteMsgPort(arosxc->InpMsgPort);

    CloseLibrary(arosxc->Base);

    Forbid();
    arosxc->Task = NULL;
    if(arosxc->ReadySigTask)
    {
        Signal(arosxc->ReadySigTask, 1L<<arosxc->ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

#undef ps

/* /// "nOpenCfgWindow()" */
LONG nOpenCfgWindow(struct AROSXClassController *arosxc)
{
    struct Library *ps;
    mybug(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!arosxc->GUITask)
    {
        if((arosxc->GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, arosxc)))
        {
            Permit();
            CloseLibrary(ps);
            return(TRUE);
        }
    }
    Permit();
    CloseLibrary(ps);
    return(FALSE);
}
/* \\\ */
