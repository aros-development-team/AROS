/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#define INTUITION_NO_INLINE_STDARG

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>

#include <libraries/mui.h>

#include <dos/dos.h>
#include <exec/types.h>

#include <clib/alib_protos.h>

#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <gadgets/colorwheel.h>

#include <libraries/asl.h>
#include <libraries/expansionbase.h>

#include <devices/trackdisk.h>
#include <devices/scsidisk.h>

#include <hidd/hidd.h>
#include <hidd/storage.h>

#include <mui/TextEditor_mcc.h>
#include <zune/iconimage.h>

#include "ia_locale.h"
#include "ia_install.h"
#include "ia_install_intern.h"
#include "ia_option.h"
#include "ia_volume.h"
#include "ia_volume_intern.h"
#include "ia_bootloader.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DVOL(x)

extern void SetOptObjNotificationFromObj(Object *objSrc, Object *optobjTgt, IPTR trigTag, IPTR trigVal, IPTR setTag, IPTR setVal);


AROS_UFH3
(
    void, volActivateHookFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(IPTR *, param, A1)
)
{
    AROS_USERFUNC_INIT

	struct Volume_Data *data = (struct Volume_Data *)hook->h_Data;
	Object *PopParentObj;

	bug("[InstallAROS:Volume] %s: Volume_Data @ 0x%p\n", __func__, data);

	WORD winx, winy;
	WORD winw, winh;

	if (data->vd_PopParentObj)
		PopParentObj = data->vd_PopParentObj;
	else PopParentObj = _parent(obj);

	winw = _mright(PopParentObj) - _mleft(PopParentObj), winh = 70;

    winx = _window(PopParentObj)->LeftEdge + _mleft(PopParentObj );
	winy = _window(obj)->TopEdge + _mbottom(obj);

	if (!data->vd_PopObj)
	{
		Object *PopPadObj = NULL;
		if (data->vd_PopPadTxt)
			PopPadObj = RectangleObject,
									MUIA_FixWidthTxt, data->vd_PopPadTxt,
								End;
		else
			PopPadObj = HVSpace;

		data->vd_PopObj = WindowObject,
				MUIA_Window_Height, winh,
				MUIA_Window_Width, winw,
				MUIA_Window_LeftEdge, winx,
				MUIA_Window_TopEdge, winy,
				MUIA_Window_CloseGadget, FALSE,
				MUIA_Window_DepthGadget, FALSE,
				MUIA_Window_SizeGadget, FALSE,
				MUIA_Window_Borderless, TRUE,
				MUIA_Window_DragBar, FALSE,
				WindowContents, (IPTR) (VGroup,
					MUIA_Frame, MUIV_Frame_ReadList,
					Child, (IPTR) HVSpace,
					Child, HGroup,
						Child, (IPTR)(RectangleObject,
							MUIA_Background, MUII_SHADOW,
							MUIA_FixWidth, 1,
						End),
						Child, (IPTR)PopPadObj,
						Child, (IPTR) ColGroup(5),
							Child, (IPTR) LLabel(_(MSG_FILESYSTEM)),
							Child, (IPTR) data->vd_FSObj,
							Child, (IPTR) LLabel(_(MSG_SIZE)),
							Child, (IPTR) data->vd_SizeObj,
							Child, HVSpace,
						End,
						Child, (IPTR)(RectangleObject,
							MUIA_Background, MUII_SHADOW,
							MUIA_FixWidth, 1,
						End),
					End,
					Child, (IPTR) HVSpace,
					Child, (IPTR)(RectangleObject,
						MUIA_Background, MUII_SHADOW,
						MUIA_FixHeight, 1,
					End),
				End),
			End;

		if (data->vd_PopObj)
		{
			bug("[InstallAROS:Volume] %s: new window created @ 0x%p\n", __func__, data->vd_PopObj);

			DoMethod(_app(obj), OM_ADDMEMBER, (IPTR) data->vd_PopObj);
			data->vd_EHNode.ehn_Object = obj;
			data->vd_EHNode.ehn_Events = IDCMP_INACTIVEWINDOW;
		}
	}
	else
	{
		struct TagItem windowTags[] = {
		        { MUIA_Window_LeftEdge, winx},
		        { MUIA_Window_TopEdge, winy},
				{ MUIA_Window_Width, winw},
		        { TAG_DONE }
			};
		SetAttrsA(data->vd_PopObj, windowTags);
	}
	if (data->vd_PopObj)
	{
		bug("[InstallAROS:Volume] %s: vd_PopObj @ 0x%p\n", __func__, data->vd_PopObj);
		SET(data->vd_PopObj, MUIA_Window_Open, TRUE);
		DoMethod(data->vd_PopObj, MUIM_Window_AddEventHandler, &data->vd_EHNode);
	}
    AROS_USERFUNC_EXIT
}

#   define BUFFERSIZE 512
static IPTR Volume__OM_NEW(Class * CLASS, Object * self, struct opSet *message)
{
	BOOL volInform = (BOOL)GetTagData(MUIA_Volume_Informative, 0, message->ops_AttrList);
    Object *installObj = (Object *)GetTagData(MUIA_Volume_InstallInstance, 0, message->ops_AttrList);

	Object *volCreateObj = (Object *)GetTagData(MUIA_Volume_CreateObj,  0, message->ops_AttrList);
	char *volHelpTxt = (char *)GetTagData(MUIA_Volume_Help,  0, message->ops_AttrList);
	char *volPadTxt = (char *)GetTagData(MUIA_Volume_PopPadTxt,  0, message->ops_AttrList);

	Object *volNameObj = (Object *)GetTagData(MUIA_Volume_NameObj,  0, message->ops_AttrList);
	Object *volFSObj = (Object *)GetTagData(MUIA_Volume_FSObj,  0, message->ops_AttrList);
	Object *volSizeObj = (Object *)GetTagData(MUIA_Volume_SizeObj,  0, message->ops_AttrList);
	Object *actionObj, *createGrpObj = NULL;
	char *actionObjImgPath;
	BPTR lock;
	TEXT imageSpec[BUFFERSIZE];

    D(bug("[InstallAROS:Volume] %s()\n", __func__));

	if (!installObj || !volNameObj || !volFSObj || !volSizeObj)
		return 0;

	if (volInform)
	{
		actionObjImgPath ="THEME:Images/Gadgets/Help";
	}
	else
	{
		actionObjImgPath ="THEME:Images/Gadgets/Below";
	}
	imageSpec[0] = '\0';
    lock = Lock(actionObjImgPath, ACCESS_READ);
    if (lock != BNULL)
    {
        strlcat(imageSpec, "3:", BUFFERSIZE);
        strlcat(imageSpec, actionObjImgPath, BUFFERSIZE);
        UnLock(lock);
	}

	actionObj = ImageObject,
			MUIA_InputMode, MUIV_InputMode_RelVerify,
			MUIA_Image_Spec, (IPTR)imageSpec,
			MUIA_Image_FreeVert,    FALSE,
			MUIA_Image_FreeHoriz,   FALSE,
			MUIA_Weight,            0,
		End;

	if (!actionObj)
		return 0;

	if (volCreateObj)
	{
		createGrpObj = HGroup,
						Child, (IPTR) volCreateObj,
					End;
		if (!createGrpObj)
		{
			MUI_DisposeObject(actionObj);
			return 0;
		}
	}
	
	self = (Object *) DoSuperNewTags
        (
            CLASS, self, NULL,

			(volHelpTxt) ? MUIA_ShortHelp : TAG_IGNORE,
				(IPTR)volHelpTxt,
			Child, (IPTR) HGroup,
				(volCreateObj) ? Child : TAG_IGNORE,
					(IPTR)createGrpObj,
				Child, (IPTR) HVSpace,
				Child, (IPTR) HGroup,
					Child, (IPTR) LLabel(_(MSG_NAME)),
					Child, (IPTR) volNameObj,
				End,
				Child, (IPTR)actionObj,
			End,

            TAG_MORE, (IPTR) message->ops_AttrList   
        );

	if (self)
	{
		struct Volume_Data *data = INST_DATA(CLASS, self);

		bug("[InstallAROS:Volume] %s: Volume_Data @ 0x%p\n", __func__, data);

		data->vd_PopPadTxt = volPadTxt;

		data->vd_EHNode.ehn_Class = CLASS;

		data->vd_CreateObj = volCreateObj;
		data->vd_NameObj = volNameObj;
		data->vd_FSObj = volFSObj;
		data->vd_SizeObj = volSizeObj;

		data->vd_ActivateHook.h_Entry = (APTR)volActivateHookFunc;
		data->vd_ActivateHook.h_Data = data;
		DoMethod(actionObj, MUIM_Notify, MUIA_Pressed, FALSE,
				 self, 2, MUIM_CallHook, &data->vd_ActivateHook);

		return (IPTR)self;
    }
	else
	{
		if (createGrpObj)
			MUI_DisposeObject(createGrpObj);
		MUI_DisposeObject(actionObj);
	}
    return (IPTR)NULL;
}

static IPTR Volume__OM_GET(Class * CLASS, Object * self, struct opGet *message)
{
    struct Volume_Data *data = INST_DATA(CLASS, self);

    DVOL(bug("[InstallAROS:Volume] %s()\n", __func__));

    switch(message->opg_AttrID)
    {
        default:
            break;
    }
    return DoSuperMethodA(CLASS, self, message);
}

static IPTR Volume__OM_SET(Class * CLASS, Object * self, struct opSet *message)
{
    struct Volume_Data *data = INST_DATA(CLASS, self);
    struct TagItem         *tag, *tags;

    for (tags = message->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
		case MUIA_Volume_ParentGrpObj:
			data->vd_PopParentObj = (Object *)tag->ti_Data;
			break;

        default:
            break;
        }
    }

    return DoSuperMethodA(CLASS, self, (Msg)message);
}

static IPTR Volume__MUIM_HandleEvent(Class * CLASS, Object * self, struct MUIP_HandleEvent *message)
{
    struct Volume_Data *data = INST_DATA(CLASS, self);

    DVOL(bug("[InstallAROS:Volume] %s()\n", __func__));

	if (data->vd_PopObj)
	{
		if ((BOOL)XGET(data->vd_PopObj, MUIA_Window_Open) == TRUE)
		{
			DoMethod(data->vd_PopObj, MUIM_Window_RemEventHandler, &data->vd_EHNode);
			SET(data->vd_PopObj, MUIA_Window_Open, FALSE);
		}
	}

	return MUI_EventHandlerRC_Eat;
}

BOOPSI_DISPATCHER(IPTR, Volume__Dispatcher, CLASS, self, message)
{
    DVOL(bug("[InstallAROS:Volume] %s(%08x)\n", __func__, message->MethodID));

    /* Handle our methods */
    switch (message->MethodID)
    {
    case OM_NEW:
        return Volume__OM_NEW(CLASS, self, (struct opSet *)message);

    case OM_GET:
        return Volume__OM_GET(CLASS, self, (struct opGet *)message);

    case OM_SET:
        return Volume__OM_SET(CLASS, self, (struct opSet *)message);

    case MUIM_HandleEvent:
        return Volume__MUIM_HandleEvent(CLASS, self, (struct MUIP_HandleEvent *)message);
    }
    return DoSuperMethodA(CLASS, self, message);
}
BOOPSI_DISPATCHER_END
