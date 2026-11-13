
/*****************************************************************************
** This is the IconList custom class, a sub class of List.mui.
******************************************************************************/

#include "debug.h"

#define USE_INLINE_STDARG
#define __NOLIBBASE__
#include <proto/muimaster.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/poseidon.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/usbclass.h>
#include <proto/icon.h>
#include <proto/utility.h>

#include "Trident.h"
#include "ActionClass.h"
#include "IconListClass.h"
#include "DevWinClass.h"
#include "CfgListClass.h"

extern struct ExecBase *SysBase;
extern struct Library *ps;
/*extern struct Library *UtilityBase; */

#define NewList(list) NEWLIST(list)

/* /// "Icon data stuff" */
const ULONG Mason_colors[96] =
{
	0x96969696,0x96969696,0x96969696,
	0x2d2d2d2d,0x28282828,0x9e9e9e9e,
	0x00000000,0x65656565,0x9a9a9a9a,
	0x35353535,0x75757575,0xaaaaaaaa,
	0x65656565,0x8a8a8a8a,0xbabababa,
	0x0c0c0c0c,0x61616161,0xffffffff,
	0x24242424,0x5d5d5d5d,0x24242424,
	0x35353535,0x8a8a8a8a,0x35353535,
	0x86868686,0xb2b2b2b2,0x3d3d3d3d,
	0x0c0c0c0c,0xe3e3e3e3,0x00000000,
	0x4d4d4d4d,0x9e9e9e9e,0x8e8e8e8e,
	0x82828282,0x00000000,0x00000000,
	0xdfdfdfdf,0x35353535,0x35353535,
	0xdbdbdbdb,0x65656565,0x39393939,
	0xdbdbdbdb,0x8e8e8e8e,0x41414141,
	0xdfdfdfdf,0xbabababa,0x45454545,
	0xefefefef,0xe7e7e7e7,0x14141414,
	0x82828282,0x61616161,0x4d4d4d4d,
	0xa6a6a6a6,0x7e7e7e7e,0x61616161,
	0xcacacaca,0x9a9a9a9a,0x75757575,
	0x9a9a9a9a,0x55555555,0xaaaaaaaa,
	0xffffffff,0x00000000,0xffffffff,
	0xffffffff,0xffffffff,0xffffffff,
	0xdfdfdfdf,0xdfdfdfdf,0xdfdfdfdf,
	0xcacacaca,0xcacacaca,0xcacacaca,
	0xbabababa,0xbabababa,0xbabababa,
	0xaaaaaaaa,0xaaaaaaaa,0xaaaaaaaa,
	0x8a8a8a8a,0x8a8a8a8a,0x8a8a8a8a,
	0x65656565,0x65656565,0x65656565,
	0x4d4d4d4d,0x4d4d4d4d,0x4d4d4d4d,
	0x3c3c3c3c,0x3c3c3c3c,0x3b3b3b3b,
	0x00000000,0x00000000,0x00000000,
};

#define USE_CLASSES_BODY
#define USE_DEVICES_BODY
#define USE_GENERAL_BODY
#define USE_HARDWARE_BODY
#define USE_SETTINGS_BODY
#define USE_CLASS_NONE_BODY
#define USE_CLASS_AUDIO_BODY
#define USE_CLASS_CDCCONTROL_BODY
#define USE_CLASS_CDCDATA_BODY
#define USE_CLASS_CHIPSMARTCARD_BODY
#define USE_CLASS_COMMDEVICE_BODY
#define USE_CLASS_HID_BODY
#define USE_CLASS_HUB_BODY
#define USE_CLASS_MASSSTORAGE_BODY
#define USE_CLASS_PHYSICAL_BODY
#define USE_CLASS_PRINTER_BODY
#define USE_CLASS_SECURITY_BODY
#define USE_CLASS_VENDOR_BODY
#define USE_GREENLED_BODY
#define USE_ORANGELED_BODY
#define USE_ORANGELEDQ_BODY
#define USE_CLASS_BLUETOOTH_BODY
#define USE_CLASS_STILLIMAGE_BODY
#define USE_ONLINE_BODY
#define USE_POPUP_BODY

#include "MasonIcons/MI_Classes.c"
#include "MasonIcons/MI_Devices.c"
#include "MasonIcons/MI_General.c"
#include "MasonIcons/MI_Hardware.c"
#include "MasonIcons/MI_Settings.c"
#include "MasonIcons/MI_Class_None.c"
#include "MasonIcons/MI_Class_Audio.c"
#include "MasonIcons/MI_Class_CDCControl.c"
#include "MasonIcons/MI_Class_CDCData.c"
#include "MasonIcons/MI_Class_ChipSmartCard.c"
#include "MasonIcons/MI_Class_CommDevice.c"
#include "MasonIcons/MI_Class_HID.c"
#include "MasonIcons/MI_Class_Hub.c"
#include "MasonIcons/MI_Class_MassStorage.c"
#include "MasonIcons/MI_Class_Physical.c"
#include "MasonIcons/MI_Class_Printer.c"
#include "MasonIcons/MI_Class_Security.c"
#include "MasonIcons/MI_Class_Vendor.c"
#include "MasonIcons/MI_GreenLED.c"
#include "MasonIcons/MI_OrangeLED.c"
#include "MasonIcons/MI_OrangeLEDQ.c"
#include "MasonIcons/MI_Class_Bluetooth.c"
#include "MasonIcons/MI_Class_StillImage.c"
#include "MasonIcons/MI_Online.c"
#include "MasonIcons/MI_Popup.c"

static const UBYTE *mibodies[MAXMASONICONS] =
{
    General_body,
    Hardware_body,
    Devices_body,
    Classes_body,
    Settings_body,
    Class_None_body,
    Class_Audio_body,
    Class_CommDevice_body,
    Class_CDCControl_body,
    Class_HID_body,
    Class_Physical_body,
    Class_Printer_body,
    Class_MassStorage_body,
    Class_Hub_body,
    Class_CDCData_body,
    Class_ChipSmartCard_body,
    Class_Security_body,
    Class_Vendor_body,
    GreenLED_body,
    OrangeLED_body,
    OrangeLEDQ_body,
    Class_Bluetooth_body,
    Class_StillImage_body,
    Online_body,
    Popup_body
};
/* \\\ */

/* /// "IconListDispatcher()" */
AROS_UFH3(IPTR, IconListDispatcher,
          AROS_UFHA(struct IClass *, cl, A0),
          AROS_UFHA(Object *, obj, A2),
          AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    // There should never be an uninitialized pointer, but just in case, try to get an mungwall hit if so.
    struct IconListData *data = (struct IconListData *) 0xABADCAFE;

    // on OM_NEW the obj pointer will be void, so don't try to get the data base in this case.
    if(msg->MethodID != OM_NEW) data = INST_DATA(cl,obj);

    switch(msg->MethodID)
    {
        case OM_NEW:
            if(!(obj = (Object *) DoSuperMethodA(cl,obj,msg)))
                return(0);
            return((IPTR) obj);

        case MUIM_Setup:
            {
                ULONG cnt;
                for(cnt = 0; cnt < MAXMASONICONS; cnt++)
                {
                    data->mimainbody[cnt] = BodychunkObject,
                        MUIA_Bitmap_SourceColors, Mason_colors,
                        MUIA_FixWidth, 16,
                        MUIA_FixHeight, 16,
                        MUIA_Bitmap_Width, 16,
                        MUIA_Bitmap_Height, 16,
                        MUIA_Bodychunk_Depth, 5,
                        MUIA_Bodychunk_Body, mibodies[cnt],
                        MUIA_Bodychunk_Compression, 1,
                        MUIA_Bodychunk_Masking, 2,
                        MUIA_Bitmap_Transparent, 0,
                        MUIA_Bitmap_UseFriend, TRUE,
                        MUIA_Bitmap_Precision, PRECISION_ICON,
                        End;
                    data->mimainlist[cnt] = (Object *) DoMethod(obj, MUIM_List_CreateImage, data->mimainbody[cnt], 0);
                }
            }
            break;

        case MUIM_Cleanup:
            {
                ULONG cnt;
                for(cnt = 0; cnt < MAXMASONICONS; cnt++)
                {
                    DoMethod(obj, MUIM_List_DeleteImage, data->mimainlist[cnt]);
                    DoMethod(data->mimainbody[cnt], OM_DISPOSE);
                    data->mimainlist[cnt] = NULL;
                    data->mimainbody[cnt] = NULL;
                }
            }
            break;

    }
    return(DoSuperMethodA(cl,obj,msg));
    AROS_USERFUNC_EXIT
}
/* \\\ */
