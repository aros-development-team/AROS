/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1

#include <aros/debug.h>

#include <exec/types.h>
#include <stdlib.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/poseidon.h>
#include <proto/cybergraphics.h>

#include <libraries/mui.h>
#include <libraries/poseidon.h>

#include <clib/alib_protos.h>

#include <cybergraphx/cybergraphics.h>
#include <graphics/gfx.h>

//#include "ov534.h"
//#include "ov772x.h"

#define OV534_REG_CTRL              0xF5
#define OV534_REG_DI                0xF4
#define OV534_REG_DO                0xF3
#define OV534_REG_MS_ADDRESS        0xF2
#define OV534_REG_MS_ID             0xF1
#define OV534_REG_SYS_CTRL          0xE7
#define OV534_OP_WRITE_3            0x37
#define OV534_OP_WRITE_2            0x33
#define OV534_OP_READ_2             0xf9
#define OV534_REG_GPIO_V0           0x23
#define OV534_REG_GPIO_C0           0x21

#define OV772X_SLAVE_ADDRESS_WRITE  0x42
#define OV772X_REG_VER              0x0B
#define OV772X_REG_PID              0x0A

#define MYBUG_LEVEL 1
#define mybug(l, x) D(if ((l>=MYBUG_LEVEL)||(l==-1)) { do { { bug x; } } while (0); } )

#define MUIA_Resolution             (TAG_USER | 0x80420000) + 0
//#define MUIM_Action_HandlePsdEvents (TAG_USER | 0x80420000) + 1
#define MUIM_Action_HandleTmrEvents (TAG_USER | 0x80420000) + 2


extern char kitty640_pure[];
//extern char kitty640_video[];
extern char kitty320_pure[];
//extern char kitty320_video[];

unsigned char kitty[640*480*4];

struct Library *ps;

struct InstData {
    BOOL guisleep;
    BOOL resolutionvga;

    UWORD pos;

    Object *self;

    struct MsgPort *ps3eye_epmsgport;
//    struct PsdEventHook *psdeventhandler;

    struct MUI_InputHandlerNode psdeventihn;
    struct MUI_InputHandlerNode tmreventihn;

    struct PsdDevice *pd;
    struct PsdAppBinding *pab;
    struct Hook releasehook;

    struct PsdInterface *ps3eye_interface;

    struct PsdEndpoint *ps3eye_ep1in;

    struct PsdPipe *ps3eye_ep0pipe;
    struct PsdPipe *ps3eye_ep1pipe;

};

ULONG ov534_reg_write(struct InstData *data, UBYTE reg, UBYTE *val) {
    mybug(-1, ("ov534_reg_write register %01x with value %01x\n", reg, *val));

    psdPipeSetup(data->ps3eye_ep0pipe, URTF_OUT|URTF_VENDOR|URTF_DEVICE, 0x01, 0x00, reg);

    return(psdDoPipe(data->ps3eye_ep0pipe, val, 1));
}

ULONG ov534_reg_read(struct InstData *data, UBYTE reg, UBYTE *val) {

    ULONG ioerr;

    psdPipeSetup(data->ps3eye_ep0pipe, URTF_IN|URTF_VENDOR|URTF_DEVICE, 0x01, 0x00, reg);
    ioerr =psdDoPipe(data->ps3eye_ep0pipe, val, 1);

    mybug(-1, ("ov534_reg_read register %01x returns value %01x (ioerr %x)\n", reg, *val, ioerr));

    return(ioerr);
}

ULONG ov534_reg_bitset(struct InstData *data, UBYTE reg, UBYTE *bitset) {

    UBYTE tmp;
    ULONG ioerr;

    ioerr = ov534_reg_read(data, reg, &tmp);
    if(ioerr) {
        return(ioerr);
    }

    tmp |= *bitset;
    return(ov534_reg_write(data, reg, &tmp));
}

ULONG ov534_reg_bitclr(struct InstData *data, UBYTE reg, UBYTE *bitset) {

    UBYTE tmp;
    ULONG ioerr;

    if(ioerr = ov534_reg_read(data, reg, &tmp)) {
        return(ioerr);
    }

    tmp &= ~(*bitset);
    return(ov534_reg_write(data, reg, &tmp));
}

void ov772x_reg_write(struct InstData *data, UBYTE reg, UBYTE *val) {

    UBYTE tmp = reg;

	ov534_reg_write(data, OV534_REG_MS_ADDRESS, &tmp);

    tmp = *val;
	ov534_reg_write(data, OV534_REG_DO, &tmp);

    tmp = OV534_OP_WRITE_3;
	ov534_reg_write(data, OV534_REG_CTRL, &tmp);

}

UBYTE ov772x_reg_read(struct InstData *data, UBYTE reg) {

    UBYTE tmp = reg;

	ov534_reg_write(data, OV534_REG_MS_ADDRESS, &tmp);

    tmp = OV534_OP_WRITE_2;
	ov534_reg_write(data, OV534_REG_CTRL, &tmp);

    tmp = OV534_OP_READ_2;
	ov534_reg_write(data, OV534_REG_CTRL, &tmp);

	ov534_reg_read(data, OV534_REG_DI, &tmp);

    return(tmp);
}

void freedevice(struct InstData *data) {

    if(data->ps3eye_ep0pipe) {
        psdFreePipe(data->ps3eye_ep0pipe); // Allowed to be NULL
        data->ps3eye_ep0pipe = NULL;
        mybug(-1, ("releasehook freed endpoint 0 pipe\n"));
    }

    if(data->ps3eye_ep1pipe) {
        psdFreePipe(data->ps3eye_ep1pipe); // Allowed to be NULL
        data->ps3eye_ep1pipe = NULL;
        mybug(-1, ("releasehook freed endpoint 1 pipe\n"));
    }

    if(data->pab) {
        /* CHECKME: Calls releasehook? */
        psdReleaseAppBinding(data->pab); // Allowed to be NULL
        data->pab = NULL;
        data->pd = NULL;
        mybug(-1, ("freedevice released PS3Eye camera binding\n"));
    }
}

AROS_UFH3(void, releasehook, AROS_UFHA(struct Hook *, hook, A0), AROS_UFHA(APTR, pab, A2), AROS_UFHA(struct InstData *, data, A1)) {
    AROS_USERFUNC_INIT

    mybug(-1,("PSD Dispatcher!\n"));

    freedevice(data);

    AROS_USERFUNC_EXIT
}

void allocdevice(struct InstData *data) {

    UBYTE regval, tmp;

    /*
        Try to find FREE PS3Eye camera (DA_Binding = FALSE)
    */
    data->pd = psdFindDevice(NULL, DA_VendorID, 0x1415, DA_ProductID, 0x2000, DA_Binding, FALSE, TAG_END);
    if(data->pd) {
        mybug(-1, ("allocdevice found PS3Eye camera\n"));

        data->releasehook.h_Entry = (APTR) releasehook;

        data->pab = psdClaimAppBinding(ABA_Device, data->pd,
                                       ABA_ReleaseHook, &data->releasehook,
                                       ABA_UserData, data,
                                       ABA_ForceRelease, FALSE,
                                       TAG_END);

        if(data->pab) {
            mybug(-1, ("allocdevice claimed PS3Eye camera\n"));

            if((data->ps3eye_interface = psdFindInterface(data->pd, NULL, IFA_InterfaceNum, 0, TAG_END))){
                mybug(-1, ("allocdevice found interface 0\n"));

                if((data->ps3eye_ep0pipe = psdAllocPipe(data->pd, data->ps3eye_epmsgport, NULL))) {
                    mybug(-1, ("allocdevice allocated endpoint 0 pipe (CONTROL)\n"));

                    if((data->ps3eye_ep1in = psdFindEndpoint(data->ps3eye_interface, NULL, EA_IsIn, TRUE, EA_EndpointNum, 1, EA_TransferType, USEAF_BULK, TAG_END))) {
                        mybug(-1, ("allocdevice found endpoint 1 (BULK) from interface 0\n"));

                        if((data->ps3eye_ep1pipe = psdAllocPipe(data->pd, data->ps3eye_epmsgport, data->ps3eye_ep1in))) {
                            mybug(-1, ("allocdevice allocated endpoint 1 pipe (BULK)\n"));

                            /*
                                We need to call our set method in order for the led to luminate or not depending on the tick box (may have been pressed before usb connection)
                                Well, it seems to work...
                            */
                            SetAttrs(data->self, MUIA_Resolution, data->resolutionvga, TAG_DONE);

                            /* Turn the camera on */
                            regval = 0x3a;
                            ov534_reg_write(data, OV534_REG_SYS_CTRL, &regval);

                            regval = OV772X_SLAVE_ADDRESS_WRITE;
                            ov534_reg_write(data, OV534_REG_MS_ID, &regval);

                            /* probe the sensor */
                        	mybug(-1, ("Sensor ID: %02x%02x\n", ov772x_reg_read(data, OV772X_REG_PID), ov772x_reg_read(data, OV772X_REG_VER)));

                            return;

                        } else {
                            mybug(-1, ("allocdevice failed to allocate endpoint 1 pipe (BULK)\n"));
                        }
                    } else {
                        mybug(-1, ("allocdevice could not find endpoint 1 (BULK) from interface 0\n"));
                    }

                } else {
                    mybug(-1, ("allocdevice failed to allocate endpoint 0 pipe (CONTROL)\n"));   
                }

            }

            psdReleaseAppBinding(data->pab);
            mybug(-1, ("allocdevice released PS3Eye camera binding\n"));

        } else {
            mybug(-1, ("allocdevice unable to claim PS3Eye camera\n"));
        }
    }
}

IPTR mNew(Class *cl, Object *obj, struct opSet *msg) {
	mybug(-1, ("mNew gets called\n"));

    if((obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg))) {
        struct InstData *data = INST_DATA(cl, obj);
        //mybug(-1, ("resolutionvga %d\n", data->resolutionvga));
        data->resolutionvga = FALSE;

        data->pd = NULL;
        data->pab = NULL;

        /* Force a failure commenting this out */
#if 1
        if((data->ps3eye_epmsgport = CreateMsgPort())) {
//            if((data->psdeventhandler = psdAddEventHandler(data->psdeventmsgport, ~0))) {

//                data->psdeventihn.ihn_Object = obj;
//                data->psdeventihn.ihn_Signals = 1UL<<data->psdeventmsgport->mp_SigBit;
//                data->psdeventihn.ihn_Flags = 0;
//                data->psdeventihn.ihn_Method = MUIM_Action_HandlePsdEvents;

                data->tmreventihn.ihn_Object = obj;
                data->tmreventihn.ihn_Millis = 40;
                data->tmreventihn.ihn_Flags  = MUIIHNF_TIMER;
                data->tmreventihn.ihn_Method = MUIM_Action_HandleTmrEvents;

                data->self = obj;

                return (IPTR)obj;
//            }
        }
#endif
        CoerceMethod(cl,obj,OM_DISPOSE);
    }

    return (IPTR)NULL;
}

IPTR mDispose(Class *cl, Object *obj, struct opGet *msg) {
	mybug(-1, ("mDispose gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);
    //mybug(-1, ("resolutionvga %d\n", data->resolutionvga));

//    if(data->psdeventhandler){
//        psdRemEventHandler(data->psdeventhandler);
//        data->psdeventhandler = NULL;
//    }

    freedevice(data);

    if(data->ps3eye_epmsgport) {
        DeleteMsgPort(data->ps3eye_epmsgport);
        data->ps3eye_epmsgport = NULL;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

IPTR mSetup(Class *cl, Object *obj, struct MUIP_Setup *msg) {
	mybug(-1, ("mSetup gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);
    
    if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

//    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->psdeventihn);
    DoMethod(_app(obj), MUIM_Application_AddInputHandler, (IPTR)&data->tmreventihn);

    return TRUE;
}


IPTR mCleanup(Class *cl, Object *obj, struct MUIP_Cleanup *msg) {
	mybug(-1, ("mCleanup gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

    DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->tmreventihn); 
//	DoMethod(_app(obj), MUIM_Application_RemInputHandler, (IPTR)&data->psdeventihn);
    
    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR mSet(Class *cl, Object *obj, struct opSet *msg) {
    mybug(-1, ("mSet gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

    struct TagItem *tags  = msg->ops_AttrList;
    struct TagItem *tag;

    UBYTE regval;

    while ((tag = NextTagItem(&tags)) != NULL) {
        switch(tag->ti_Tag) {
            case MUIA_Resolution:
                data->resolutionvga = tag->ti_Data;

                if(data->ps3eye_ep1pipe) {
                    if(data->resolutionvga) {
                        /* Turn red led on */
                        regval = 0x80;
                        ov534_reg_bitset(data, OV534_REG_GPIO_C0, &regval);
                        ov534_reg_bitset(data, OV534_REG_GPIO_V0, &regval);
                    } else {
                        /* Turn red led off */
                        regval = 0x80;
                        ov534_reg_bitset(data, OV534_REG_GPIO_C0, &regval);
                        ov534_reg_bitclr(data, OV534_REG_GPIO_V0, &regval);
                    }
                }

                SetAttrs(_win(obj), MUIA_Window_Open, FALSE, MUIA_Window_Open, TRUE, TAG_DONE);
            break;

        }
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}

IPTR mGet(Class *cl, Object *obj, struct opGet *msg) {
    mybug(0, ("mGet gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);
    IPTR retval = TRUE;
    
    switch(msg->opg_AttrID) {
    	case MUIA_Resolution:
	        *msg->opg_Storage = data->resolutionvga;
            mybug(-1, ("mGet MUIA_Resolution = %d\n", data->resolutionvga));
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, obj, (Msg)msg);
	    break;
    }
    
    return retval;
}

IPTR mAskMinMax(Class *cl, Object *obj, struct MUIP_AskMinMax *msg) {
	mybug(-1, ("mAskMinMax gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

	DoSuperMethodA(cl, obj, (Msg)msg);

    if(data->resolutionvga) {    
	    msg->MinMaxInfo->MinWidth  += 640;
	    msg->MinMaxInfo->MinHeight += 480;
	    msg->MinMaxInfo->DefWidth  += 640;
	    msg->MinMaxInfo->DefHeight += 480;
    } else {
	    msg->MinMaxInfo->MinWidth  += 320;
	    msg->MinMaxInfo->MinHeight += 240;
	    msg->MinMaxInfo->DefWidth  += 320;
	    msg->MinMaxInfo->DefHeight += 240;
    }

    return TRUE;
}

IPTR mShow(Class *cl, Object *obj, struct MUIP_Show *msg) {
    mybug(-1, ("mShow gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);
    IPTR    	      retval;

    data->guisleep = FALSE;

    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    return retval;
}

IPTR mHide(Class *cl, Object *obj, struct MUIP_Hide *msg) {
    mybug(-1, ("mHide gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

    data->guisleep = TRUE;

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


void DoEffect(UBYTE *src, UBYTE *dest, ULONG w, ULONG h) {
    //CopyMemQuick(src, dest, w*h*4);

    ULONG l, i, ir, ig, ib, iw;

    static LONG z = 0;

    UBYTE R, G, B;

    ULONG *destulong;
    destulong = (ULONG *)dest;

    if(w == 640) {
        ir = ((rand() % 4)*w)+(rand() % 4);
        ig = ((rand() % 2)*w)+(rand() % 2);
        ib = ((rand() % 2)*w)+(rand() % 2);
    } else {
        ir = ((rand() % 2)*w)+(rand() % 2);
        ig = ((rand() % 2)*w)+(rand() % 2);
        ib = ((rand() % 2)*w)+(rand() % 2);
    }

    l = w*h;

    z += (w/320)*2;
    if(z>(LONG)h) {
        z = -(h/10);
    }

    for(i=0; i<l; i++) {

        iw = (i/w);

        if((ir>=0) && (ir<l)) {
            R = src[(ir*4)+0];
        } else {
            R = 0;
        }
        if((iw+0) % 3) {
            R = (R|0x1f);
        }

        if((ig>=0) && (ig<l)) {
            G = src[(ig*4)+1];
        } else {
            G = 0;
        }
        if((iw+1) % 3) {
            G = (G|0x1f);
        }

        if((ib>=0) && (ib<l)) {
            B = src[(ib*4)+2];
        } else {
            B = 0;
        }
        if((iw+2) % 3) {
            B = (B|0x1f);
        }

        ir++;
        ig++;
        ib++;

        if( ((LONG)(iw) > z) && ((LONG)(iw) < (z+(h/10))) ) {
            R /= 2;
            G /= 2;
            B /= 2;
        }

        destulong[i] = (ULONG)(R<<0|G<<8|B<<16);
    }

}

IPTR mDraw(Class *cl, Object *obj, struct MUIP_Draw *msg) {
    mybug(0, ("mDraw gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

    WORD    	      y;
    IPTR    	      retval;

    static ULONG sec=0, mic=0, lastTick=0, currTick=0;
    
    retval = DoSuperMethodA(cl, obj, (Msg)msg);
    
    if (!(msg->flags & (MADF_DRAWOBJECT | MADF_DRAWUPDATE))) return 0;

    if(data->ps3eye_ep1pipe) {        
        for(y= 0; y < _mheight(obj); y++) {
            WORD col;
            col = ((y + data->pos) / 8) % 2;
            SetAPen(_rp(obj), _pens(obj)[col ? MPEN_SHADOW : MPEN_SHINE]);
            RectFill(_rp(obj), _mleft(obj), _mtop(obj) + y, _mright(obj), _mtop(obj) + y);
        }
        data->pos++;
    } else {
        /* Add static distortion or old crt-monitor effect */
        if(data->resolutionvga) {
            DoEffect(kitty640_pure, kitty, 640, 480);
            WritePixelArray(kitty, 0, 0, 640*4, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), RECTFMT_RGB032);
//          WritePixelArray(kitty640_pure, 0, 0, 640*4, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), RECTFMT_RGB032); //+(rand() % (10))
        } else {

            DoEffect(kitty320_pure, kitty, 320, 240);
            WritePixelArray(kitty, 0, 0, 320*4, _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), RECTFMT_RGB032);
        }
    }


    //lastTick = currTick;
    //CurrentTime(&sec, &mic);
    //currTick = ((sec * 1000) + (mic / 1000));
    //mybug(-1, ("Delta %d mS\n", currTick-lastTick));

    return retval;
}

/* Just use the timer event handler, no need for this one */
//IPTR mPsdEventHandler(Class *cl, Object *obj, Msg msg) {
//    mybug(-1, ("mPsdEventHandler gets called\n"));

//    struct InstData *data = INST_DATA(cl, obj);

//    APTR pen;

//    while((pen = GetMsg(data->psdeventmsgport))) {
//        ReplyMsg(pen);
//    }

//    return TRUE;
//}

IPTR mTmrEventHandler(Class *cl, Object *obj, Msg msg) {
    mybug(0, ("mTmrEventHandler gets called\n"));

    struct InstData *data = INST_DATA(cl, obj);

    if(!(data->pab)) {
        allocdevice(data);
    }

    MUI_Redraw(obj, MADF_DRAWUPDATE);

    return TRUE;
}

/*
#define BOOPSI_DISPATCHER(rettype,name,cl,obj,msg) \
    AROS_UFH3(SAVEDS rettype, name,\
        AROS_UFHA(Class  *, cl,  A0),\
        AROS_UFHA(Object *, obj, A2),\
        AROS_UFHA(Msg     , msg, A1)) {AROS_USERFUNC_INIT
*/

BOOPSI_DISPATCHER(IPTR, classdispatcher, cl, obj, msg) {
    mybug(0,("Class Dispatcher!\n"));

    switch (msg->MethodID) {

        case OM_NEW:
            return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:
            return mDispose(cl,obj,(APTR)msg);

        case MUIM_Setup:
            return mSetup(cl,obj,(APTR)msg);
        case MUIM_Cleanup:
            return mCleanup(cl,obj,(APTR)msg);

        case OM_SET:
            return mSet(cl,obj,(APTR)msg);
        case OM_GET:
            return mGet(cl,obj,(APTR)msg);

        case MUIM_AskMinMax:
            return mAskMinMax(cl,obj,(APTR)msg);
        case MUIM_Hide:
            return mHide(cl,obj,(APTR)msg);
        case MUIM_Show:
            return mShow(cl,obj,(APTR)msg);

        case MUIM_Draw:
            return mDraw(cl,obj,(APTR)msg);

//        case MUIM_Action_HandlePsdEvents:
//            return mPsdEventHandler(cl,obj,(APTR)msg);

        case MUIM_Action_HandleTmrEvents:
            return mTmrEventHandler(cl,obj,(APTR)msg);

        default:
            return DoSuperMethodA(cl,obj,msg);
    }

    return 0;
}
BOOPSI_DISPATCHER_END

int main(void) {

    SetTaskPri(FindTask(NULL), 15);

    struct MUI_CustomClass *mcc;

    if((ps = OpenLibrary("poseidon.library", 4))) {

        if((mcc = MUI_CreateCustomClass(NULL, "Area.mui", NULL, sizeof(struct InstData), classdispatcher))) {
            mybug(-1,("mui custon class at %p\n", mcc));

            Object *app, *window, *tick1, *custom;

            app = ApplicationObject,
            SubWindow, window = WindowObject,
                MUIA_Window_Title, "PS3Eye",
                    MUIA_Window_Activate, TRUE,
                        WindowContents, HGroup,
		    		        Child, HGroup,
		    			        MUIA_Weight, 100,
		    			        Child, custom = NewObject(mcc->mcc_Class, NULL, TAG_DONE),
		    		        End,

                            Child, VGroup,
                                MUIA_Weight, 1,
                                MUIA_Group_SameWidth, TRUE,
                                GroupFrameT("Configuration"),
                                    Child, HGroup,
                                        Child, tick1 = MUI_MakeObject(MUIO_Checkmark, NULL),
                                        Child, MUI_MakeObject(MUIO_Label,"640x480", 0),
                                    End,
                                Child, (IPTR) VSpace(0),
                            End,
                        End,
                    End,
                End;

            if(app) {
                ULONG sigs = 0;

                DoMethod(tick1, MUIM_Notify, MUIA_Selected, TRUE,  (IPTR)custom, 3, MUIM_Set, MUIA_Resolution, TRUE);
                DoMethod(tick1, MUIM_Notify, MUIA_Selected, FALSE, (IPTR)custom, 3, MUIM_Set, MUIA_Resolution, FALSE);

                DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

                set(window, MUIA_Window_Open, TRUE);

                while((LONG)DoMethod(app, MUIM_Application_NewInput, &sigs) != (LONG)MUIV_Application_ReturnID_Quit) {
                    if(sigs) {
                        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                        if(sigs & SIGBREAKF_CTRL_C)
                            break;
                    }
                }

                MUI_DisposeObject(app);
            }

            MUI_DeleteCustomClass(mcc);
        }
        CloseLibrary(ps);
    }

    mybug(-1,("Exiting\n"));
    return 0;
}
