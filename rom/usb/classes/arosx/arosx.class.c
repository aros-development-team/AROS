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


struct Library * AROSXInit(void);

struct AROSXClassController *AROSXClass_CreateController(LIBBASETYPEPTR nh, UBYTE id);
struct AROSXClassController *AROSXClass_ConnectController(LIBBASETYPEPTR nh);
void AROSXClass_DisconnectController(LIBBASETYPEPTR nh, struct AROSXClassController *arosxclass_this_controller);
void AROSXClass_DestroyController(LIBBASETYPEPTR nh, struct AROSXClassController *arosxclass_this_controller);



/* /// "Lib Stuff" */
static int libInit(LIBBASETYPEPTR nh)
{

    mybug(0, ("libInit nh: 0x%08lx SysBase: 0x%08lx\n", nh, SysBase));

    AROSXClass_DestroyController(nh, NULL);

    nh->nh_arosx_controller_1 = AROSXClass_CreateController(nh, 1);
    nh->nh_arosx_controller_2 = AROSXClass_CreateController(nh, 2);
    nh->nh_arosx_controller_3 = AROSXClass_CreateController(nh, 3);
    nh->nh_arosx_controller_4 = AROSXClass_CreateController(nh, 4);

	InitSemaphore(&nh->nh_arosx_controller_lock);

	nh->nh_AROSXBase = AROSXInit();

#define AROSXBase   nh->nh_AROSXBase

    if(!AROSXBase)
    {
        mybug(-1, ("libInit: MakeLibrary(\"arosx.library\") failed!\n"));
        return(FALSE);
    }

    mybug(-1, ("AROSX: AROSXBase 0x%08lx\n", AROSXBase));
	//AROS_LC0(ULONG, Dummy1, LIBBASETYPEPTR, AROSXBase, 5, arosx);

    mybug(0, ("libInit: Ok\n"));

    return(TRUE);
}

static int libOpen(LIBBASETYPEPTR nh)
{
    mybug(0, ("libOpen nh: 0x%08lx\n", nh));
    return(TRUE);
}

static int libExpunge(LIBBASETYPEPTR nh)
{
    mybug(10, ("libExpunge nh: 0x%08lx\n", nh));
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
struct AROSXClassController * usbAttemptInterfaceBinding(struct AROSXClassBase *nh, struct PsdInterface *pif)
{
    struct Library *ps;

    struct AROSXClassController *nch;
    
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

        if((nch = AROSXClass_ConnectController(nh)))
        {
            nch->nch_ClsBase = nh;
            nch->nch_Device = pd;
            nch->nch_Interface = pif;

            nch->controller_type = AROSX_CONTROLLER_TYPE_GAMEPAD;

            psdSafeRawDoFmt(buf, 64, "arosx.class.gamepad.%01x", nch->id);
            nch->nch_ReadySignal = SIGB_SINGLE;
            nch->nch_ReadySigTask = FindTask(NULL);
            SetSignal(0, SIGF_SINGLE);
            if((tmptask = psdSpawnSubTask(buf, nHidTask, nch)))
            {
                psdBorrowLocksWait(tmptask, 1UL<<nch->nch_ReadySignal);
                if(nch->nch_Task)
                {
                    nch->nch_ReadySigTask = NULL;
                    //FreeSignal(nch->nch_ReadySignal);
                    psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &nch->nch_devname, TAG_END);

					psdSafeRawDoFmt(nch->name, 64, "%s (%01x)", nch->nch_devname, nch->id);

                    psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "Play it again, '%s'!", nch->name);

                    CloseLibrary(ps);
                    return(nch);
                }
            }
            nch->nch_ReadySigTask = NULL;
            //FreeSignal(nch->nch_ReadySignal);
            AROSXClass_DisconnectController(nh, nch);;
        }
        CloseLibrary(ps);
    }

    return(NULL);
}
/* \\\ */

/* /// "usbReleaseInterfaceBinding()" */
void usbReleaseInterfaceBinding(struct AROSXClassBase *nh, struct AROSXClassController *nch)
{
    struct Library *ps;
    struct PsdConfig *pc;
    struct PsdDevice *pd;
    STRPTR devname;

    mybug(0, ("nepHidReleaseInterfaceBinding(%08lx)\n", nch));

    /* Kill the nch_GUITask */
    if(nch->nch_GUITask)
    {
        Signal(nch->nch_GUITask, SIGBREAKF_CTRL_C);
    }

    if((ps = OpenLibrary("poseidon.library", 4)))
    {
        Forbid();
        nch->nch_ReadySignal = SIGB_SINGLE;
        nch->nch_ReadySigTask = FindTask(NULL);
        if(nch->nch_Task)
        {
            Signal(nch->nch_Task, SIGBREAKF_CTRL_C);
        }
        Permit();
        while(nch->nch_Task)
        {
            Wait(1L<<nch->nch_ReadySignal);
        }
        //FreeSignal(nch->nch_ReadySignal);
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface, IFA_Config, &pc, TAG_END);
        psdGetAttrs(PGA_CONFIG, pc, CA_Device, &pd, TAG_END);
        psdGetAttrs(PGA_DEVICE, pd, DA_ProductName, &devname, TAG_END);
        psdAddErrorMsg(RETURN_OK, (STRPTR) libname, "'%s' fell silent!", devname);

        AROSXClass_DisconnectController(nh, nch);

        CloseLibrary(ps);
    }
}
/* \\\ */

/* /// "usbGetAttrsA()" */
AROS_LH3(LONG, usbGetAttrsA,
         AROS_LHA(ULONG, type, D0),
         AROS_LHA(APTR, usbstruct, A0),
         AROS_LHA(struct TagItem *, tags, A1),
         LIBBASETYPEPTR, nh, 5, nep)
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
         LIBBASETYPEPTR, nh, 6, nep)
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
         LIBBASETYPEPTR, nh, 7, nep)
{
    AROS_LIBFUNC_INIT

    mybug(0, ("Do Method %ld\n", methodid));
    switch(methodid)
    {
        case UCM_AttemptInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ForceInterfaceBinding:
            return((IPTR) usbAttemptInterfaceBinding(nh, (struct PsdInterface *) methoddata[0]));

        case UCM_ReleaseInterfaceBinding:
            usbReleaseInterfaceBinding(nh, (struct AROSXClassController *) methoddata[0]);
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



struct AROSXClassController *AROSXClass_CreateController(LIBBASETYPEPTR nh, UBYTE id) {

    struct AROSXClassController *arosxclass_this_controller;

    arosxclass_this_controller = AllocVec(sizeof(struct AROSXClassController), MEMF_ANY|MEMF_CLEAR);

    if(arosxclass_this_controller == NULL) {
        mybug(-1, ("[AROSXClass] AROSXClass_CreateController: Failed to create new controller structure for controller %01x\n", id));
        return NULL;
    } else {
        arosxclass_this_controller->id = id;

        arosxclass_this_controller->status.connected = FALSE;
        arosxclass_this_controller->status.wireless = FALSE;
        arosxclass_this_controller->status.signallost = FALSE;

        mybug(-1, ("[AROSXClass] AROSXClass_CreateController: Created new controller structure %04lx for controller %01x\n", arosxclass_this_controller, arosxclass_this_controller->id));

        if (arosxclass_this_controller->nch_TimerMP = CreatePort(NULL, 0)) {
            if (arosxclass_this_controller->nch_TimerIO = (struct timerequest *)CreateExtIO(arosxclass_this_controller->nch_TimerMP, sizeof(struct timerequest))) {
                if (!(OpenDevice(TIMERNAME, UNIT_MICROHZ, (struct IORequest *)arosxclass_this_controller->nch_TimerIO, 0))) {
                    arosxclass_this_controller->nch_TimerBase = arosxclass_this_controller->nch_TimerIO->tr_node.io_Device;
                    FreeSignal(arosxclass_this_controller->nch_TimerMP->mp_SigBit);
                    return arosxclass_this_controller;
                }
                DeleteExtIO((struct IORequest *)arosxclass_this_controller->nch_TimerIO);
            }
            DeletePort(arosxclass_this_controller->nch_TimerMP);
        }
    }

    AROSXClass_DestroyController(nh, arosxclass_this_controller);    

    return NULL;

}

/*
    Just does a FreeVec, no checks to see if someone is using it...
     - Implemented some sanity
*/
void AROSXClass_DestroyController(LIBBASETYPEPTR nh, struct AROSXClassController *arosxclass_this_controller) {

    UBYTE id;

    if(arosxclass_this_controller != NULL) {
        id = arosxclass_this_controller->id;

        ObtainSemaphore(&nh->nh_arosx_controller_lock);
        if(id == 1) {
            FreeVec(nh->nh_arosx_controller_1);
            nh->nh_arosx_controller_1 = NULL;
        }else if(id == 2) {
            FreeVec(nh->nh_arosx_controller_2);
            nh->nh_arosx_controller_2 = NULL;
        }else if(id == 3) {
            FreeVec(nh->nh_arosx_controller_3);
            nh->nh_arosx_controller_3 = NULL;
        }else if(id == 4) {
            FreeVec(nh->nh_arosx_controller_4);
            nh->nh_arosx_controller_4 = NULL;
        }
        ReleaseSemaphore(&nh->nh_arosx_controller_lock);
    }else{
        mybug(-1, ("[AROSXClass] AROSXClass_DestroyController: Called on non existing controller...\n"));
    }
}

struct AROSXClassController *AROSXClass_ConnectController(LIBBASETYPEPTR nh) {

    struct AROSXClassController *arosxclass_this_controller;
    arosxclass_this_controller = NULL;

    ObtainSemaphore(&nh->nh_arosx_controller_lock);

    if(nh->nh_arosx_controller_1->status.connected == FALSE) {
        arosxclass_this_controller = nh->nh_arosx_controller_1;
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 1\n"));
    }else if(nh->nh_arosx_controller_2->status.connected == FALSE) {
        arosxclass_this_controller = nh->nh_arosx_controller_2;
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 2\n"));
    }else if(nh->nh_arosx_controller_3->status.connected == FALSE) {
        arosxclass_this_controller = nh->nh_arosx_controller_3;
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 3\n"));
    }else if(nh->nh_arosx_controller_4->status.connected == FALSE) {
        arosxclass_this_controller = nh->nh_arosx_controller_4;
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Assigned to controller number 4\n"));
    }else {
        ReleaseSemaphore(&nh->nh_arosx_controller_lock);
        mybug(-1, ("[AROSXClass] AROSXClass_ConnectController: Controller count exceeded, failing...\n"));
        return NULL;
    }

    arosxclass_this_controller->status.connected = TRUE;

    ReleaseSemaphore(&nh->nh_arosx_controller_lock);

    return arosxclass_this_controller;

}

void AROSXClass_DisconnectController(LIBBASETYPEPTR nh, struct AROSXClassController *arosxclass_this_controller) {

    if(arosxclass_this_controller != NULL) {
        ObtainSemaphore(&nh->nh_arosx_controller_lock);
        arosxclass_this_controller->status.connected = FALSE;
        arosxclass_this_controller->controller_type = AROSX_CONTROLLER_TYPE_UNKNOWN;
        ReleaseSemaphore(&nh->nh_arosx_controller_lock);
        mybug(-1, ("[AROSXClass] AROSXClass_DisconnectController: Disconnected controller number %01x\n", arosxclass_this_controller->id));
    }

}

/**************************************************************************/

#undef  ps
#define ps nch->nch_Base
#undef  TimerBase
#define TimerBase nch->nch_TimerBase

/* /// "nHidTask()" */
AROS_UFH0(void, nHidTask)
{
    AROS_USERFUNC_INIT

    struct AROSXClassController *nch;

    struct PsdPipe *pp;

    ULONG sigmask;
    ULONG sigs;

    UBYTE *epinbuf;
	UBYTE *ep0buf;

    LONG ioerr;
    ULONG len;

    /*
        This does not allocate nch, it is already present. It only fetches it from the task tc_UserData.
         - Currently code assumes controller to be a gamepad
    */
    if((nch = nAllocHid()))
    {

        nch->nch_TimerMP->mp_SigBit = AllocSignal(-1);
        nch->nch_TimerIO->tr_node.io_Message.mn_ReplyPort->mp_SigBit = nch->nch_TimerMP->mp_SigBit;
        nch->nch_TimerIO->tr_node.io_Message.mn_ReplyPort->mp_SigTask = FindTask(NULL);

    	epinbuf = nch->nch_EPInBuf;
		ep0buf  = nch->nch_EP0Buf;

        nch->status.signallost = TRUE;

        Forbid();
        if(nch->nch_ReadySigTask)
        {
            Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
        }
        Permit();
        sigmask = (1L<<nch->nch_TaskMsgPort->mp_SigBit)|SIGBREAKF_CTRL_C;

        psdDelayMS(2000);

        psdPipeSetup(nch->nch_EP0Pipe, URTF_IN|URTF_VENDOR|URTF_INTERFACE, 0x01, 0x0100, 0x00);
        do {
        	ioerr = psdDoPipe(nch->nch_EP0Pipe, ep0buf, 20);
    	} while(ioerr);

    	mybug(-1, ("EP0: %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx %02lx\n",
            	    ep0buf[0], ep0buf[1], ep0buf[2], ep0buf[3], ep0buf[4], ep0buf[5], ep0buf[6], ep0buf[7], ep0buf[8], ep0buf[9], ep0buf[10],
        	        ep0buf[11], ep0buf[12], ep0buf[13], ep0buf[14], ep0buf[15], ep0buf[16], ep0buf[17], ep0buf[18], ep0buf[19]));

        nch->status.wireless = (ep0buf[18]&(1<<0))? TRUE:FALSE;

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

            XInput descriptors for various gamepads, we have one for this interface in *nch_xinput_desc as a UBYTE array
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
    	bufout = nch->nch_EPOutBuf;

    	bufout[0] = 0x01;
    	bufout[1] = 0x03;
    	bufout[2] = nch->id + 1;
    	bufout[3] = 0x00;
    	bufout[4] = 0x00;
    	bufout[5] = 0x00;
    	bufout[6] = 0x00;
    	bufout[7] = 0x00;
		bufout[8] = 0x00;
    	bufout[9] = 0x00;
    	bufout[10] = 0x00;
		bufout[11] = 0x00;

    	psdDoPipe(nch->nch_EPOutPipe, bufout, 12);

		psdSendPipe(nch->nch_EPInPipe, epinbuf, 20);
        do
        {
            sigs = Wait(sigmask);
            while((pp = (struct PsdPipe *) GetMsg(nch->nch_TaskMsgPort)))
            {
                if(pp == nch->nch_EPInPipe)
                {
                    if(!(ioerr = psdGetPipeError(pp)))
                    {
                        len = psdGetPipeActual(pp);

                        if(Gamepad_ParseMsg(nch, epinbuf, len)) {
                            mybug(-1,("Timestamp %u #%x\n", nch->nch_arosx_gamepad.Timestamp, nch->id));
                        }

                        /* Wait */
                        /*
                        nch->nch_TimerIO->tr_node.io_Command = TR_ADDREQUEST;
                        nch->nch_TimerIO->tr_time.tv_secs = 0;
                        nch->nch_TimerIO->tr_time.tv_micro = 1000;
                        DoIO((struct IORequest *)nch->nch_TimerIO);
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
                    psdSendPipe(nch->nch_EPInPipe, epinbuf, 20);
                    break;
                }
            }
        } while(!(sigs & SIGBREAKF_CTRL_C));
        mybug(-1, ("Going down the river!\n"));
        psdAbortPipe(nch->nch_EPInPipe);
        psdWaitPipe(nch->nch_EPInPipe);
        nFreeHid(nch);
    }
    
    AROS_USERFUNC_EXIT
}
/* \\\ */

/* /// "Gamepad_ParseMsg()" */
BOOL Gamepad_ParseMsg(struct AROSXClassController *nch, UBYTE *buf, ULONG len) {

    struct AROSX_GAMEPAD  arosx_gamepad_new;

    struct AROSX_GAMEPAD *arosx_gamepad;
    arosx_gamepad = &nch->nch_arosx_gamepad;

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
	nch->status.signallost = (buf[14]&(1<<4))? FALSE:TRUE;

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
    bufout = nch->nch_EPOutBuf;

    bufout[0] = 0x00;
    bufout[1] = 0x08;
    bufout[2] = 0x00;
    bufout[3] = buf[6];
    bufout[4] = buf[7];
    bufout[5] = 0x00;
    bufout[6] = 0x00;
    bufout[7] = 0x00;
    psdDoPipe(nch->nch_EPOutPipe, bufout, 8);
    */

    if(ret) {
        GetSysTime(&current);
        arosx_gamepad->Timestamp = (ULONG)((current.tv_secs)<<14)|(UWORD)((current.tv_micro)>>6);

        if(nch->nch_GUITask)
        {
            Signal(nch->nch_GUITask, (ULONG) (1<<nch->nch_TrackingSignal));
        }
    }

    return ret;

}
/* \\\ */

/* /// "nAllocHid()" */
struct AROSXClassController * nAllocHid(void)
{
    struct Task *thistask;
    struct AROSXClassController *nch;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    do
    {
        if(!(nch->nch_Base = OpenLibrary("poseidon.library", 4)))
        {
            Alert(AG_OpenLib);
            break;
        }
        psdGetAttrs(PGA_INTERFACE, nch->nch_Interface,
                    IFA_Config, &nch->nch_Config,
                    IFA_InterfaceNum, &nch->nch_IfNum,
                    TAG_END);
        psdGetAttrs(PGA_CONFIG, nch->nch_Config,
                    CA_Device, &nch->nch_Device,
                    TAG_END);

        nch->nch_EPIn = psdFindEndpoint(nch->nch_Interface, NULL,
                                       EA_IsIn, TRUE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);

        nch->nch_EPOut = psdFindEndpoint(nch->nch_Interface, NULL,
                                       EA_IsIn, FALSE,
                                       EA_TransferType, USEAF_INTERRUPT,
                                       TAG_END);

        if((!nch->nch_EPIn)|(!nch->nch_EPOut))
        {
            mybug(1, ("Ooops!?! No Endpoints defined?\n"));
            psdAddErrorMsg(RETURN_FAIL, (STRPTR) libname,
                           "Failed to get endpoints!");
            break;
        }
        if((nch->nch_InpMsgPort = CreateMsgPort()))
        {
            if((nch->nch_InpIOReq = (struct IOStdReq *) CreateIORequest(nch->nch_InpMsgPort, sizeof(struct IOStdReq))))
            {
                if(!OpenDevice("input.device", 0, (struct IORequest *) nch->nch_InpIOReq, 0))
                {
                    nch->nch_InputBase = (struct Library *) nch->nch_InpIOReq->io_Device;
                    if((nch->nch_TaskMsgPort = CreateMsgPort()))
                    {
                        if((nch->nch_EP0Pipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, NULL)))
                        {
                            if((nch->nch_EPInPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPIn)))
                            {
                                psdSetAttrs(PGA_PIPE, nch->nch_EPInPipe,
                                            PPA_NakTimeout, FALSE,
                                            PPA_AllowRuntPackets, TRUE,
                                            TAG_END);

                                if((nch->nch_EP0Buf = psdAllocVec(100)))
                                {
                                	if((nch->nch_EPInBuf = psdAllocVec(100)))
                                	{
                                    	if((nch->nch_EPOutPipe = psdAllocPipe(nch->nch_Device, nch->nch_TaskMsgPort, nch->nch_EPOut)))
                                    	{
                                        	psdSetAttrs(PGA_PIPE, nch->nch_EPOutPipe,
                                            	PPA_NakTimeout, FALSE,
                                            	PPA_AllowRuntPackets, TRUE,
                                           		TAG_END);

                                        	if((nch->nch_EPOutBuf = psdAllocVec(100)))
                                        	{
                                            	nch->nch_Task = thistask;
                                            	return(nch);
                                        	}
                                        	psdFreePipe(nch->nch_EPOutPipe);
                                    	}
                                    	psdFreeVec(nch->nch_EPInBuf);
                                	}
                                	psdFreeVec(nch->nch_EP0Buf);
                            	}
                                psdFreePipe(nch->nch_EPInPipe);
                            }
                            psdFreePipe(nch->nch_EP0Pipe);
                        }
                        DeleteMsgPort(nch->nch_TaskMsgPort);
                    }
                    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
                }
                DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
            }
            DeleteMsgPort(nch->nch_InpMsgPort);
        }
    } while(FALSE);
    CloseLibrary(nch->nch_Base);
    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
    return(NULL);
}
/* \\\ */

/* /// "nFreeHid()" */
void nFreeHid(struct AROSXClassController *nch)
{
    psdFreeVec(nch->nch_EPOutBuf);
    psdFreeVec(nch->nch_EPInBuf);

    psdFreePipe(nch->nch_EPOutPipe);
    psdFreePipe(nch->nch_EPInPipe);
    psdFreePipe(nch->nch_EP0Pipe);

    DeleteMsgPort(nch->nch_TaskMsgPort);
    CloseDevice((struct IORequest *) nch->nch_InpIOReq);
    DeleteIORequest((struct IORequest *) nch->nch_InpIOReq);
    DeleteMsgPort(nch->nch_InpMsgPort);

    CloseLibrary(nch->nch_Base);

    Forbid();
    nch->nch_Task = NULL;
    if(nch->nch_ReadySigTask)
    {
        Signal(nch->nch_ReadySigTask, 1L<<nch->nch_ReadySignal);
    }
}
/* \\\ */

/**************************************************************************/

#undef ps

/* /// "nOpenCfgWindow()" */
LONG nOpenCfgWindow(struct AROSXClassController *nch)
{
    struct Library *ps;
    mybug(10, ("Opening GUI...\n"));
    if(!(ps = OpenLibrary("poseidon.library", 4)))
    {
        return(FALSE);
    }
    Forbid();
    if(!nch->nch_GUITask)
    {
        if((nch->nch_GUITask = psdSpawnSubTask(MOD_NAME_STRING " GUI", nGUITask, nch)))
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

#undef ps
#undef MUIMasterBase
#define ps PsdBase
#define MUIMasterBase MUIBase

/* /// "nGUITask()" */
AROS_UFH0(void, nGUITask)
{
    AROS_USERFUNC_INIT

    struct Task *thistask;
    struct AROSXClassBase *nh;
    struct AROSXClassController *nch;

    struct Library *MUIBase;
    struct Library *PsdBase;

    thistask = FindTask(NULL);
    nch = thistask->tc_UserData;
    nh = nch->nch_ClsBase;

    struct AROSX_GAMEPAD *arosx_gamepad;
    arosx_gamepad = &nch->nch_arosx_gamepad;

    ++nh->nh_Library.lib_OpenCnt;
    if((MUIMasterBase = OpenLibrary(MUIMASTER_NAME, MUIMASTER_VMIN)))
    {
        if((ps = OpenLibrary("poseidon.library", 4)))
        {

            nch->nch_App = ApplicationObject,
            MUIA_Application_Title      , (IPTR)libname,
            MUIA_Application_Version    , (IPTR)VERSION_STRING,
            MUIA_Application_Copyright  , (IPTR)"©2018 The AROS Development Team",
            MUIA_Application_Author     , (IPTR)"The AROS Development Team",
            MUIA_Application_Description, (IPTR)"Settings for the arosx.class",
            MUIA_Application_Base       , (IPTR)"AROSX",
            MUIA_Application_HelpFile   , (IPTR)"HELP:Poseidon.guide",
            MUIA_Application_Menustrip  , (IPTR)MenustripObject,
                Child, (IPTR)MenuObjectT((IPTR)"Project"),
                    Child, (IPTR)(nch->nch_AboutMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"About...",
                        MUIA_Menuitem_Shortcut, (IPTR)"?",
                        End),
                    End,
                Child, (IPTR)MenuObjectT((IPTR)"Settings"),
                    Child, (IPTR)(nch->nch_UseMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"Save",
                        MUIA_Menuitem_Shortcut, (IPTR)"S",
                        End),
                    Child, (IPTR)MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)NM_BARLABEL,
                        End,
                    Child, (IPTR)(nch->nch_MUIPrefsMI = MenuitemObject,
                        MUIA_Menuitem_Title, (IPTR)"MUI Settings",
                        MUIA_Menuitem_Shortcut, (IPTR)"M",
                        End),
                    End,
                End,

            SubWindow, (IPTR)(nch->nch_MainWindow = WindowObject,
                MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
                MUIA_Window_Title, (IPTR)nch->name,
                MUIA_HelpNode, (IPTR)libname,

                WindowContents, (IPTR)VGroup,
                    Child, (IPTR)(nch->nch_GamepadGroupObject = ColGroup(2),
                    	GroupFrameT("Gamepad"),
                    	MUIA_Disabled, TRUE,


                    	Child, (IPTR)HGroup,
            			Child, (IPTR)(nch->nch_GamepadObject_button_a = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_b = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_x = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_y = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_ls = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_rs = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_left_thumb = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_right_thumb = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_dpad_left = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_dpad_right = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_dpad_up = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_dpad_down = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_back = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),

            			Child, (IPTR)(nch->nch_GamepadObject_button_start = ImageObject,
                			MUIA_Image_FontMatch, TRUE,
                			MUIA_Selected, FALSE,
                			MUIA_ShowSelState, FALSE,
                			MUIA_Image_Spec, MUII_RadioButton,
                			MUIA_Frame, MUIV_Frame_None,
                   			End),
                        End,

                        Child, (IPTR)(nch->nch_GamepadObject_left_trigger = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(nch->nch_GamepadObject_right_trigger = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(nch->nch_GamepadObject_left_stick_x = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(nch->nch_GamepadObject_left_stick_y = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(nch->nch_GamepadObject_right_stick_x = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        Child, (IPTR)(nch->nch_GamepadObject_right_stick_y = GaugeObject,
                            GaugeFrame,
                            MUIA_Gauge_Max, 0xffff,
                            MUIA_Gauge_InfoText, (IPTR)"%lx",
                            MUIA_Gauge_Horiz, TRUE,
                            MUIA_Gauge_Current, 0,
                            End),

                        End),
                    Child, (IPTR)VSpace(0),
                    Child, (IPTR)HGroup,
                        MUIA_Group_SameWidth, TRUE,
                        Child, (IPTR)(nch->nch_UseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Save ",
                            End),
                        Child, (IPTR)(nch->nch_CloseObj = TextObject, ButtonFrame,
                            MUIA_Background, MUII_ButtonBack,
                            MUIA_CycleChain, 1,
                            MUIA_InputMode, MUIV_InputMode_RelVerify,
                            MUIA_Text_Contents, (IPTR)"\33c Use ",
                            End),
                        End,
                    End,
                End),
            End;

            if(nch->nch_App) 
            {
                DoMethod(nch->nch_MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
                DoMethod(nch->nch_UseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(nch->nch_CloseObj, MUIM_Notify, MUIA_Pressed, FALSE,
                         nch->nch_App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                DoMethod(nch->nch_AboutMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_ABOUT);
                DoMethod(nch->nch_UseMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_ReturnID, ID_STORE_CONFIG);
                DoMethod(nch->nch_MUIPrefsMI, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
                         nch->nch_App, 2, MUIM_Application_OpenConfigWindow, 0);


                IPTR  isopen = 0;
                IPTR  iconify = 0;
                ULONG sigs;
                ULONG sigmask;
                LONG retid;

                get(nch->nch_App, MUIA_Application_Iconified, &iconify);
                set(nch->nch_MainWindow, MUIA_Window_Open, TRUE);
                get(nch->nch_MainWindow, MUIA_Window_Open, &isopen);

                if((isopen || (!iconify)))
                {
                    nch->nch_TrackingSignal = AllocSignal(-1);
                    sigmask = (1<<nch->nch_TrackingSignal);
                    do
                    {
                        retid = DoMethod(nch->nch_App, MUIM_Application_NewInput, &sigs);
                        switch(retid)
                        {
                            case ID_ABOUT:
                                MUI_RequestA(nch->nch_App, nch->nch_MainWindow, 0, NULL, "Fabulous!", VERSION_STRING, NULL);
                                break;
                        }
                        if(retid == MUIV_Application_ReturnID_Quit)
                        {
                            break;
                        }
                        if(sigs)
                        {
                            sigs = Wait(sigs | sigmask | SIGBREAKF_CTRL_C);
                            if(sigs & SIGBREAKF_CTRL_C)
                            {
                                break;
                            }

							if((ULONG)(1<<nch->nch_TrackingSignal)) {

                                /* TODO: Check if the GUI goes to sleep when the controller says it's sleepy */
								if((nch->status.wireless)&&(nch->status.signallost)) {
                                    set(nch->nch_GamepadGroupObject, MUIA_Disabled, TRUE);
                                    //psdDelayMS(10);
                                } else {
                                    set(nch->nch_GamepadGroupObject, MUIA_Disabled, FALSE);

                                    set(nch->nch_GamepadObject_button_a, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_A));
                                    set(nch->nch_GamepadObject_button_b, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_B));
                                    set(nch->nch_GamepadObject_button_x, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_X));
                                    set(nch->nch_GamepadObject_button_y, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_Y));
                                    set(nch->nch_GamepadObject_button_ls, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_SHOULDER));
                                    set(nch->nch_GamepadObject_button_rs, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_SHOULDER));
                                    set(nch->nch_GamepadObject_left_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_LEFT_THUMB));
                                    set(nch->nch_GamepadObject_right_thumb, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_RIGHT_THUMB));
                                    set(nch->nch_GamepadObject_dpad_left, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_LEFT));
									set(nch->nch_GamepadObject_dpad_right, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_RIGHT));
									set(nch->nch_GamepadObject_dpad_up, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_UP));
									set(nch->nch_GamepadObject_dpad_down, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_DPAD_DOWN));
									set(nch->nch_GamepadObject_button_back, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_BACK));
									set(nch->nch_GamepadObject_button_start, MUIA_Selected, (arosx_gamepad->Buttons & AROSX_GAMEPAD_START));

                                    set(nch->nch_GamepadObject_left_trigger, MUIA_Gauge_Current, (arosx_gamepad->LeftTrigger));
                                    set(nch->nch_GamepadObject_right_trigger, MUIA_Gauge_Current, (arosx_gamepad->RightTrigger));

                            		if(arosx_gamepad->ThumbLX>=0x8000) {
                                		set(nch->nch_GamepadObject_left_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbLX-0x8000));
                            		} else {
                                		set(nch->nch_GamepadObject_left_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLX));
                            		}

                            		if(arosx_gamepad->ThumbLY>=0x8000) {
                                		set(nch->nch_GamepadObject_left_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbLY-0x8000));
                            		} else {
                                		set(nch->nch_GamepadObject_left_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbLY));
                            		}

                            		if(arosx_gamepad->ThumbRX>=0x8000) {
                                		set(nch->nch_GamepadObject_right_stick_x, MUIA_Gauge_Current, (arosx_gamepad->ThumbRX-0x8000));
                            		} else {
                                		set(nch->nch_GamepadObject_right_stick_x, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRX));
                            		}

                            		if(arosx_gamepad->ThumbRY>=0x8000) {
                                		set(nch->nch_GamepadObject_right_stick_y, MUIA_Gauge_Current, (arosx_gamepad->ThumbRY-0x8000));
                            		} else {
                                		set(nch->nch_GamepadObject_right_stick_y, MUIA_Gauge_Current, (0x8000+arosx_gamepad->ThumbRY));
                            		}

                            		/* 100Hz max. GUI update frequency should be enough for everyone... */
                            		//psdDelayMS(10);
                            	}
                            }
                        }
                    } while(TRUE);
                    set(nch->nch_MainWindow, MUIA_Window_Open, FALSE);
                }

                if(nch->nch_App)
                {
                    MUI_DisposeObject(nch->nch_App);
                    nch->nch_App = NULL;
                }

                if(MUIMasterBase)
                {
                    CloseLibrary(MUIMasterBase);
                    MUIMasterBase = NULL;
                }

                if(ps)
                {
                    CloseLibrary(ps);
                    ps = NULL;
                }
            }
        }
    }

    Forbid();
    FreeSignal(nch->nch_TrackingSignal);
    nch->nch_TrackingSignal = -1;
    nch->nch_GUITask = NULL;
    --nh->nh_Library.lib_OpenCnt;

    AROS_USERFUNC_EXIT
}
/* \\\ */

