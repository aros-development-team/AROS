/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: The main mouse class.
    Lang: English.
*/

/*
    This is the native-i386 hidd maintaining all available mouse types. It
    maintains all COM/PS2 mouses available. USB is in a way (we need pci.hidd
    working, then usb.hidd).
    
    Please keep code clean from all .bss and .data sections. .rodata may exist
    as it will be connected together with .text section during linking. In near
    future this driver will be compiled as elf executable (instead of object)
    with -fPIC flag.
*/

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>

#include <hidd/hidd.h>
#include <hidd/mouse.h>

#include <devices/inputevent.h>
#include <string.h>

#include <aros/symbolsets.h>

#include "mouse.h"

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

/* !!!!!!!!!! Remove all .data from file
*/
#define HiddMouseAB	(MSD(cl)->hiddMouseAB)
/*
static OOP_AttrBase HiddMouseAB;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_Mouse, &HiddMouseAB },
    { NULL, NULL }
};

*/

/* Prototypes */

int test_mouse_usb(OOP_Class *, OOP_Object *);
int test_mouse_ps2(OOP_Class *, OOP_Object *);
int test_mouse_com(OOP_Class *, OOP_Object *);
void dispose_mouse_usb(OOP_Class *, OOP_Object *);
void dispose_mouse_ps2(OOP_Class *, OOP_Object *);
void dispose_mouse_seriell(OOP_Class *, OOP_Object *);
void getps2State(OOP_Class *, OOP_Object *, struct pHidd_Mouse_Event *);

/* defines for buttonstate */

#define LEFT_BUTTON 	1
#define RIGHT_BUTTON 	2
#define MIDDLE_BUTTON	4

/***** Mouse::New()  ***************************************/
OOP_Object * PCMouse__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    BOOL has_mouse_hidd = FALSE;
   
    EnterFunc(bug("_Mouse::New()\n"));
 
    ObtainSemaphoreShared( &MSD(cl)->sema);
 
    if (MSD(cl)->mousehidd)
        has_mouse_hidd = TRUE;

    ReleaseSemaphore( &MSD(cl)->sema);
 
    if (has_mouse_hidd) /* Cannot open twice */
        ReturnPtr("_Mouse::New", Object *, NULL); /* Should have some error code here */

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        struct mouse_data   *data = OOP_INST_DATA(cl, o);
        struct TagItem      *tag, *tstate;

        tstate = msg->attrList;

        /* Search for all mouse attrs */

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

        /* Search for mouse installed. As USB is the fastest to test, do it
        first, if not found search for PS/2 mouse. If failure then check every
        COM port in the system - the las chance to see... */
	
	data->type = MDT_USB;
        if (!test_mouse_usb(cl, o))
        {
	    memset(&data->u.com, 0, sizeof(data->u.com));
	    data->type = MDT_SERIELL;
	    
	    if (!test_mouse_com(cl, o))
	    {
	    	memset(&data->u.ps2, 0, sizeof(data->u.ps2));
		data->type = MDT_PS2;
		
        	if (!test_mouse_ps2(cl, o))
                {
                    /* No mouse found. What we can do now is just Dispose() :( */
                    OOP_MethodID disp_mid;
                    data->type = MDT_UNKNOWN;
                    disp_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
                    OOP_CoerceMethod(cl, o, (OOP_Msg) &disp_mid);

                    o = NULL;
                }
            }
        }

        ObtainSemaphore( &MSD(cl)->sema);
        MSD(cl)->mousehidd = o;
        ReleaseSemaphore( &MSD(cl)->sema);
    }
    
    return o;
}

VOID PCMouse__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);

    ObtainSemaphore( &MSD(cl)->sema);
    MSD(cl)->mousehidd = NULL;
    ReleaseSemaphore( &MSD(cl)->sema);

    switch (data->type)
    {
	case MDT_USB:
	   dispose_mouse_usb(cl, o);
	   break;

	case MDT_SERIELL:
	   dispose_mouse_seriell(cl, o);	  
	   break;

	case MDT_PS2:
	   dispose_mouse_ps2(cl, o);
	   break;
    }

    OOP_DoSuperMethod(cl, o, msg);
}

/***** Mouse::Get()  ***************************************/
VOID PCMouse__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct mouse_data *data = OOP_INST_DATA(cl, o);
    ULONG   	       idx;

    if (IS_HIDDMOUSE_ATTR(msg->attrID, idx))
    {
	switch (idx)
	{
	    case aoHidd_Mouse_IrqHandler:
		*msg->storage = (IPTR)data->mouse_callback;
		return;
		
	    case aoHidd_Mouse_IrqHandlerData:
		*msg->storage = (IPTR)data->callbackdata;
		return;
		
	    case aoHidd_Mouse_State:
		if (data->type == MDT_PS2)
		    getps2State(cl, o, (struct pHidd_Mouse_Event *)msg->storage);
		return;

    	    case aoHidd_Mouse_RelativeCoords:
	    	*msg->storage = TRUE;
	    	return;
	}
	
    }
    
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/***** Mouse::HandleEvent()  ***************************************/

VOID PCMouse__Hidd_Mouse__HandleEvent(OOP_Class *cl, OOP_Object *o, struct pHidd_Mouse_HandleEvent *msg)
{
    struct mouse_data * data;

    EnterFunc(bug("_mouse_handleevent()\n"));

    data = OOP_INST_DATA(cl, o);

    /* Nothing done yet */

    ReturnVoid("_Mouse::HandleEvent");
}

/********************  init_kbdclass()  *********************************/

static int PCMouse_InitAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB },
        { NULL	    	, NULL      	    }
    };
	
    EnterFunc(bug("PCMouse_InitAttrs\n"));

    ReturnInt("PCMouse_InitAttr", ULONG, OOP_ObtainAttrBases(attrbases));
}

/*************** free_kbdclass()  **********************************/
static int PCMouse_ExpungeAttrs(LIBBASETYPEPTR LIBBASE)
{
    struct OOP_ABDescr attrbases[] =
    {
        { IID_Hidd_Mouse, &LIBBASE->msd.hiddMouseAB },
        { NULL	    	, NULL      	    }
    };
    
    EnterFunc(bug("PCMouse_InitClass\n"));

    OOP_ReleaseAttrBases(attrbases);

    return TRUE;
}

ADD2INITLIB(PCMouse_InitAttrs, 0)
ADD2EXPUNGELIB(PCMouse_ExpungeAttrs, 0)
