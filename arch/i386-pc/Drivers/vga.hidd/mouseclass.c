/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>
#include <hidd/serial.h>

#include <devices/inputevent.h>

#include "vga.h"

#define DEBUG 0
#include <aros/debug.h>

#define MOUSE_PROTOCOL_MS_LT 1      /* if not defined MouseSystems Protocol is used */

ULONG mouse_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata);

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL, NULL }
};

struct mouse_data
{
    VOID (*mouse_callback)(APTR, struct pHidd_Mouse_Event *);
    APTR callbackdata;
    
    OOP_Object * Ser;
    OOP_Object * Unit;
    
    UWORD buttonstate;
};

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/***** Mouse::New()  ***************************************/
static OOP_Object * mouse_new(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
   
    EnterFunc(bug("Mouse::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->mousehidd)
        has_mouse_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_mouse_hidd) /* Cannot open twice */
        ReturnPtr("Mouse::New", Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct mouse_data *data = OOP_INST_DATA(cl, o);
        struct TagItem *tag, *tstate;

        tstate = msg->attrList;
        while ((tag = NextTagItem((const struct TagItem **)&tstate)))
        {
            ULONG idx;
    
            if (IS_HIDDMOUSE_ATTR(tag->ti_Tag, idx))
            {
                switch (idx)
                {
                    case aoHidd_Mouse_IrqHandler:
                        data->mouse_callback = (APTR)tag->ti_Data;
                        break;

                    case aoHidd_Mouse_IrqHandlerData:
                        data->callbackdata = (APTR)tag->ti_Data;
                        break;
                }
            }
    
        } /* while (tags to process) */

        /* Install the mouse hidd */

        if (OpenLibrary("serial.hidd",0))
        {
            if ((data->Ser = OOP_NewObject(NULL, CLID_Hidd_Serial, NULL)))
            {
                D(bug("Got serial object = %p", data->Ser));
                if ((data->Unit = HIDD_Serial_NewUnit(data->Ser, 0)))
                {
                    int i;
                    struct TagItem stags[] = {
#ifdef MOUSE_PROTOCOL_MS_LT
                                            { TAG_DATALENGTH,   7 },
#else
                                            { TAG_DATALENGTH,   8 },
#endif
                                            { TAG_STOP_BITS,    1 },
                                            { TAG_PARITY_OFF,   1 },
                                            { TAG_DONE,         0 }};
                    struct TagItem t2[] = {
                                            { TAG_SET_MCR,      0 },
                                            { TAG_DONE,         0 }};   // DTR + RTS
    
                    D(bug("Got Unit object = %p", data->Unit));

                    HIDD_SerialUnit_SetBaudrate(data->Unit, 1200);
                    HIDD_SerialUnit_SetParameters(data->Unit, stags);

                    t2[0].ti_Data = 1;

                    HIDD_SerialUnit_SetParameters(data->Unit, t2);

                    i = 3000000;
                    while (i) {i--;};
                    t2[0].ti_Data = 3;

                    HIDD_SerialUnit_SetParameters(data->Unit, t2);

                    i = 3000000;
                    while (i) {i--;};

                    HIDD_SerialUnit_Init(data->Unit, mouse_InterruptHandler, data, NULL, NULL);
                }
            }
        }
        ObtainSemaphore( &XSD(cl)->sema);
        XSD(cl)->mousehidd = o;
        ReleaseSemaphore( &XSD(cl)->sema);
    }
    return o;
}

/***** Mouse::HandleEvent()  ***************************************/

static VOID mouse_handleevent(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_HandleEvent *msg)
{
    struct mouse_data * data;

    EnterFunc(bug("mouse_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);

/* Nothing done yet */

    ReturnVoid("Mouse::HandleEvent");
}

#undef XSD
#define XSD(cl) xsd

static struct vga_staticdata *vsd;

/********************  init_kbdclass()  *********************************/

#define NUM_ROOT_METHODS 1
#define NUM_MOUSE_METHODS 1

OOP_Class *init_mouseclass (struct vga_staticdata *xsd)
{
    OOP_Class *cl = NULL;

    struct OOP_MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
        {OOP_METHODDEF(mouse_new),  moRoot_New},
        {NULL, 0UL}
    };
    
    struct OOP_MethodDescr mousehidd_descr[NUM_MOUSE_METHODS + 1] = 
    {
        {OOP_METHODDEF(mouse_handleevent),  moHidd_Mouse_HandleEvent},
        {NULL, 0UL}
    };
    
    struct OOP_InterfaceDescr ifdescr[] =
    {
        {root_descr, IID_Root,  NUM_ROOT_METHODS},
        {mousehidd_descr,       IID_Hidd_HwMouse, NUM_MOUSE_METHODS},
        {NULL, NULL, 0}
    };
    
    OOP_AttrBase MetaAttrBase = OOP_ObtainAttrBase(IID_Meta);

    struct TagItem tags[] =
    {
        {aMeta_SuperID,         (IPTR)CLID_Hidd },
        {aMeta_InterfaceDescr,  (IPTR)ifdescr},
        {aMeta_InstSize,        (IPTR)sizeof (struct mouse_data) },
        {aMeta_ID,              (IPTR)CLID_Hidd_HwMouse },
        {TAG_DONE, 0UL}
    };

    EnterFunc(bug("MouseHiddClass init\n"));
    
    if (MetaAttrBase)
    {
        cl = OOP_NewObject(NULL, CLID_HiddMeta, tags);
        if(cl)
        {
            cl->UserData = (APTR)xsd;
            xsd->mouseclass = cl;
    
            vsd = xsd;
    
            if (OOP_ObtainAttrBases(attrbases))
            {
                D(bug("MouseHiddClass ok\n"));

                OOP_AddClass(cl);
            }
            else
            {
                free_mouseclass(xsd);
                cl = NULL;
            }
        }
        /* Don't need this anymore */
        OOP_ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_mouseclass", Class *, cl);
}

/*************** free_kbdclass()  **********************************/
VOID free_mouseclass(struct vga_staticdata *xsd)
{
    EnterFunc(bug("free_mouseclass(xsd=%p)\n", xsd));

    if(xsd)
    {
        OOP_RemoveClass(xsd->mouseclass);

        if(xsd->mouseclass) OOP_DisposeObject((OOP_Object *) xsd->mouseclass);
        xsd->mouseclass = NULL;

        OOP_ReleaseAttrBases(attrbases);
    }
    ReturnVoid("free_mouseclass");
}

#undef OOPBase
#define OOPBase (vsd->oopbase)

#define OLD_GFXMOUSE_HACK 0

#ifdef MOUSE_PROTOCOL_MS_LT
ULONG  mouse_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata)
{
    /* Don't initialize static variables with "=0", otherwise they go into DATA segment */
    
    static UBYTE inbuf[3];
    static UBYTE cnt;

#if OLD_GFXMOUSE_HACK
    static OOP_MethodID mid;
    static struct pHidd_Gfx_SetMouseXY p;
#else
    static struct mouse_data *mousedata;
    static struct pHidd_Mouse_Event e;
    UWORD buttonstate;
#endif

#if OLD_GFXMOUSE_HACK 
    if (!mid)
    {
        mid = OOP_GetMethodID(IID_Hidd_Gfx, moHidd_Gfx_SetMouseXY);
        p.mID = mid;
    }
#endif
    
    /* Get bytes untill there is anything to get */
    while (length)
    {
        while ((cnt < 3) && length)
        {
            inbuf[cnt] = *data++;
            length--;
            cnt++;
        }
        if (cnt == 3)
        {
            cnt = 0;
            while (!(inbuf[0] & 0x40))
            {
                inbuf[0] = inbuf[1];
                inbuf[1] = inbuf[2];

                if (length)
                {
                    inbuf[2] = *data++;
                    length--;
                }
                else return 0;
            }

#if OLD_GFXMOUSE_HACK
            p.dx = (char)(((inbuf[0] & 0x03) << 6) | (inbuf[1] & 0x3f));
            p.dy = (char)(((inbuf[0] & 0x0c) << 4) | (inbuf[2] & 0x3f));
            OOP_DoMethod(vsd->vgahidd, (Msg) &p);
#else

/*
    microsoft serial mouse protocol:

        D7      D6      D5      D4      D3      D2      D1      D0

1.      X       1       LB      RB      Y7      Y6      X7      X6
2.      X       0       X5      X4      X3      X2      X1      X0      
3.      X       0       Y5      Y4      Y3      Y2      Y1      Y0

*/

            mousedata = (struct mouse_data *)userdata;

            e.x = (char)(((inbuf[0] & 0x03) << 6) | (inbuf[1] & 0x3f));
            e.y = (char)(((inbuf[0] & 0x0c) << 4) | (inbuf[2] & 0x3f));
            if (e.x || e.y)
            {
                e.button = vHidd_Mouse_NoButton;
                e.type = vHidd_Mouse_Motion;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }
    
            buttonstate  = ((inbuf[0] & 0x20) >> 5); /* left  button bit goes to bit 0 in button state */
            buttonstate |= ((inbuf[0] & 0x10) >> 3); /* right button bit goes to bit 1 in button state */

            if((buttonstate & LEFT_BUTTON) != (mousedata->buttonstate & LEFT_BUTTON))
            {
                e.button = vHidd_Mouse_Button1;
                e.type = (buttonstate & LEFT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }

            if((buttonstate & RIGHT_BUTTON) != (mousedata->buttonstate & RIGHT_BUTTON))
            {
                e.button = vHidd_Mouse_Button2;
                e.type = (buttonstate & RIGHT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }

            mousedata->buttonstate = buttonstate;
#endif
        }
    }
    
    return 0;
}
#else
/* MouseSystems Test */
ULONG  mouse_InterruptHandler(UBYTE * data, ULONG length, ULONG unitnum, APTR userdata)
{
    /* Don't initialize static variables with "=0", otherwise they go into DATA segment */

    static UBYTE inbuf[5];
    static UBYTE cn;

    static struct mouse_data *mousedata;
    static struct pHidd_Mouse_Event e;
    UWORD buttonstate;
    
    /* Get bytes untill there is anything to get */
    while (length)
    {
        while ((cnt < 5) && length)
        {
            inbuf[cnt] = *data++;
            length--;
            cnt++;
        }
        if (cnt == 5)
        {
            cnt = 0;
            while ((inbuf[0] & 0xf8) != 0x80)
            {
                inbuf[0] = inbuf[1];
                inbuf[1] = inbuf[2];
                inbuf[2] = inbuf[3];
                inbuf[3] = inbuf[4];

                if (length)
                {
                    inbuf[4] = *data++;
                    length--;
                }
                else return 0;
            }

/*
    MouseSystems serial mouse protocol:

        D7      D6      D5      D4      D3      D2      D1      D0

1.      1       0       0       0       0       LB      CB      RB
2.      X7      X6      X5      X4      X3      X2      X1      X0      
3.      Y7      Y6      Y5      Y4      Y3      Y2      Y1      Y0
4.      X7'     X6'     X5'     X4'     X3'     X2'     X1'     X0'      
5.      Y7'     Y6'     Y5'     Y4'     Y3'     Y2'     Y1'     Y0'

*/

            mousedata = (struct mouse_data *)userdata;

            e.x = (char)(inbuf[1]+inbuf[3]);
            e.y = (char)(inbuf[2]+inbuf[4]);
            if (e.x || e.y)
            {
                e.button = vHidd_Mouse_NoButton;
                e.type = vHidd_Mouse_Motion;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }
    
            buttonstate  = ((inbuf[0] & 0x04) >> 2); /* left   button bit goes to bit 0 in button state */
            buttonstate |= (inbuf[0] & 0x02);        /* right  button bit goes to bit 1 in button state */
            buttonstate |= ((inbuf[0] & 0x01) << 1); /* middle button bit goes to bit 2 in button state */

            if((buttonstate & LEFT_BUTTON) != (mousedata->buttonstate & LEFT_BUTTON))
            {
                e.button = vHidd_Mouse_Button1;
                e.type = (buttonstate & LEFT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }

            if((buttonstate & RIGHT_BUTTON) != (mousedata->buttonstate & RIGHT_BUTTON))
            {
                e.button = vHidd_Mouse_Button2;
                e.type = (buttonstate & RIGHT_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }

            if((buttonstate & MIDDLE_BUTTON) != (mousedata->buttonstate & MIDDLE_BUTTON))
            {
                e.button = vHidd_Mouse_Button3;
                e.type = (buttonstate & MIDDLE_BUTTON) ? vHidd_Mouse_Press : vHidd_Mouse_Release;

                mousedata->mouse_callback(mousedata->callbackdata, &e);
            }

            mousedata->buttonstate = buttonstate;
        }
    }
    
    return 0;
}
#endif
