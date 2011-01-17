
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"

#include <mui/Lamp_mcc.h>

/***********************************************************************/

static UBYTE *ons, *offs, *btFixWidthTxt;

/***********************************************************************/

struct MiamiPanelLButtonClass_DATA
{
    Object *lamp;
    Object *text;
};

/***********************************************************************/

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

static ULONG
MUIPC_LButton__OM_NEW(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelLButtonClass_DATA    temp;
    struct TagItem *attrs = message->ops_AttrList;

    temp.lamp = LampObject, End;

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Frame,              MUIV_Frame_Button,
            MUIA_Background,         MUII_ButtonBack,
            MUIA_Font,               MUIV_Font_Button,
            MUIA_InputMode,          MUIV_InputMode_RelVerify,
            MUIA_CycleChain,         TRUE,
            MUIA_Group_Horiz,        TRUE,
            MUIA_Group_HorizSpacing, 0,

            temp.lamp ? Child : TAG_IGNORE, temp.lamp,
            temp.lamp ? Child : TAG_IGNORE, temp.lamp ? (RectangleObject, MUIA_FixWidthTxt, ".", End) : NULL,

            Child, temp.text = TextObject,
                MUIA_FixWidthTxt,   btFixWidthTxt,
                MUIA_Text_PreParse, MUIX_C,
                MUIA_Text_Contents, ons,
            End,

            TAG_MORE,attrs))
    {
        struct MiamiPanelLButtonClass_DATA *data = INST_DATA(CLASS,self);

        CopyMem(&temp, data, sizeof(struct MiamiPanelLButtonClass_DATA));
    }

    return (ULONG)self;
}

/***********************************************************************/

static ULONG
MUIPC_LButton__OM_SET(struct IClass *CLASS,Object *self,struct opSet *message)
{
    struct MiamiPanelLButtonClass_DATA    *data = INST_DATA(CLASS,self);
    struct TagItem *tag;

    if (tag = FindTagItem(MPA_LButton_State,message->ops_AttrList))
    {
        UBYTE  *text;
        ULONG  color, dis;

        switch (tag->ti_Data)
        {
            case MIAMIPANELV_AddInterface_State_GoingOnline:
                color = MUIV_Lamp_Color_Error;
                text  = offs;
                dis   = FALSE;
                break;

            case MIAMIPANELV_AddInterface_State_GoingOffline:
                color = MUIV_Lamp_Color_Warning;
                text  = ons;
                dis   = TRUE;
                break;

            case MIAMIPANELV_AddInterface_State_Suspending:
                color = MUIV_Lamp_Color_FatalError;
                text  = NULL;
                dis   = TRUE;
                break;

            case MIAMIPANELV_AddInterface_State_Online:
                color = MUIV_Lamp_Color_Ok;
                text  = offs;
                dis   = FALSE;
                break;

            case MIAMIPANELV_AddInterface_State_Suspended:
                color = MUIV_Lamp_Color_Processing;
                text  = NULL;
                dis   = TRUE;
                break;

            default:
                color = MUIV_Lamp_Color_Off;
                text  = ons;
                dis   = FALSE;
                break;
        }

        if (data->lamp) set(data->lamp,MUIA_Lamp_Color,color);
        if (text) set(data->text,MUIA_Text_Contents,text);
        SetSuperAttrs(CLASS,self,MUIA_Disabled,dis,TAG_DONE);
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_LButton_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_SET: return MUIPC_LButton__OM_SET(CLASS,self,(APTR)message);
        case OM_NEW: return MUIPC_LButton__OM_NEW(CLASS,self,(APTR)message);
        default:     return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_LButton_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if (MiamiPanelBaseIntern->mpb_lbuttonClass = MUI_CreateCustomClass(NULL, MUIC_Group, NULL, sizeof(struct MiamiPanelLButtonClass_DATA), MUIPC_LButton_Dispatcher))
    {
        ons  = __(MSG_IF_Button_Online);
        offs = __(MSG_IF_Button_Offline);

        if (strlen(ons)>strlen(offs)) btFixWidthTxt = ons;
        else btFixWidthTxt = offs;

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
