/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <intuition/classusr.h>
#include <libraries/gadtools.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "muimaster_intern.h"
#include "mui.h"

STATIC Object *CreateMenuString( struct NewMenu *newmenu, ULONG flags, struct Library *MUIMasterBase)
{
    int i = 0;
    Object *menustrip = MenuitemObject,End;
    Object *menu = NULL;
    Object *menuitem = NULL;

    if (!menustrip) return NULL;

    for (;newmenu[i].nm_Type != NM_END;i++)
    {
    	if (newmenu[i].nm_Type == NM_TITLE)
    	{
	    menu = MenuitemObject, MUIA_Menuitem_Title, newmenu[i].nm_Label, End;
	    if (menu) DoMethod(menustrip, MUIM_Family_AddTail, menu);
	    menuitem = NULL;
	} else
	{
	    char *label = newmenu[i].nm_Label;
	    char *commkey;

	    if (flags & MUIO_MenustripNM_CommandKeyCheck)
	    {
	    	if ((label != (char*)NM_BARLABEL) && !label[1])
	    	{
	    	    label+=2;
	    	    commkey = label;
	    	}
	    	else commkey = newmenu[i].nm_CommKey;
	    } else commkey = newmenu[i].nm_CommKey;

	    if (newmenu[i].nm_Type == NM_ITEM)
    	    {
	        menuitem = MenuitemObject,
	            MUIA_Menuitem_Title, label,
	            MUIA_Menuitem_Shortcut, commkey,
	            MUIA_Menuitem_Checkit, !!(newmenu[i].nm_Flags & CHECKIT),
		    MUIA_Menuitem_Checked, !!(newmenu[i].nm_Flags & CHECKED),
	            MUIA_Menuitem_Exclude, newmenu[i].nm_MutualExclude,
		    MUIA_Menuitem_CommandString, !!(newmenu[i].nm_Flags & NM_COMMANDSTRING),
		    MUIA_Menuitem_Toggle, !!(newmenu[i].nm_Flags & MENUTOGGLE),
		    MUIA_UserData, newmenu[i].nm_UserData,
	            End;
	        if (menuitem) DoMethod(menu, MUIM_Family_AddTail, menuitem);
	    } else
	    if (newmenu[i].nm_Type == NM_SUB)
	    {
		Object *subitem = MenuitemObject,
	            MUIA_Menuitem_Title, label,
	            MUIA_Menuitem_Shortcut, commkey,
	            MUIA_Menuitem_Checkit, !!(newmenu[i].nm_Flags & CHECKIT),
	            MUIA_Menuitem_Checked, !!(newmenu[i].nm_Flags & CHECKED),
	            MUIA_Menuitem_Exclude, newmenu[i].nm_MutualExclude,
		    MUIA_Menuitem_CommandString, !!(newmenu[i].nm_Flags & NM_COMMANDSTRING),
		    MUIA_Menuitem_Toggle, !!(newmenu[i].nm_Flags & MENUTOGGLE),
		    MUIA_UserData, newmenu[i].nm_UserData,
	            End;
		if (subitem) DoMethod(menuitem, MUIM_Family_AddTail, subitem);
	    }
	}
    }
    return menustrip;
}

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm Object *MUI_MakeObjectA(register __d0 LONG type, register __a0 IPTR *params)
#else
	AROS_LH2(Object *, MUI_MakeObjectA,

/*  SYNOPSIS */
	AROS_LHA(LONG, type, D0),
	AROS_LHA(ULONG *, params, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 20, MUIMaster)
#endif

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,MUIMasterBase)

    switch (type)
    {
	case MUIO_Label: /* STRPTR label, ULONG flags */
	{
	    struct TagItem tags[5];
	    int ntags = 0;

	    if (params[1] & MUIO_Label_SingleFrame)
	    {
		tags[ntags].ti_Tag = MUIA_Frame;
		tags[ntags].ti_Data = (IPTR)MUIV_Frame_ImageButton;
		ntags++;
	    }
	    else if (params[1] & MUIO_Label_DoubleFrame)
	    {
		tags[ntags].ti_Tag = MUIA_Frame;
		tags[ntags].ti_Data = (IPTR)MUIV_Frame_String;
		ntags++;
	    }

	    if (params[1] & MUIO_Label_LeftAligned)
	    {
		tags[ntags].ti_Tag = MUIA_Text_PreParse;
		tags[ntags].ti_Data = (IPTR)"\33l";
		ntags++;
	    }
	    else if (params[1] & MUIO_Label_Centered)
	    {
		tags[ntags].ti_Tag = MUIA_Text_PreParse;
		tags[ntags].ti_Data = (IPTR)"\33c";
		ntags++;
	    }
	    else
	    {
		tags[ntags].ti_Tag = MUIA_Text_PreParse;
		tags[ntags].ti_Data = (IPTR)"\33r";
		ntags++;
	    }

	    if (params[1] & MUIO_Label_FreeVert)
	    {
		tags[ntags].ti_Tag = MUIA_Text_SetVMax;
		tags[ntags].ti_Data = (IPTR)FALSE;
		ntags++;
	    }

	    tags[ntags].ti_Tag = TAG_DONE;
	    tags[ntags].ti_Data = 0;

	    return MUI_NewObject(MUIC_Text,
		MUIA_FramePhantomHoriz, TRUE,
		MUIA_Text_SetMax, TRUE,
		(params[1] & 0xff)?MUIA_Text_HiChar:TAG_IGNORE, params[1] & 0xff,
		(params[1] & 0xff)?TAG_IGNORE:MUIA_Text_HiCharIdx, '_',
		MUIA_Text_Contents, params[0],
		TAG_MORE, tags,
		TAG_DONE);
	    break;
	}

	case MUIO_Button: /* STRPTR label */
	{
	    return MUI_NewObject(MUIC_Text,
		ButtonFrame,
		MUIA_Font, MUIV_Font_Button,
		MUIA_Text_HiCharIdx, '_',
		MUIA_Text_Contents, params[0],
		MUIA_Text_PreParse, "\33c",
		MUIA_InputMode    , MUIV_InputMode_RelVerify,
		MUIA_Background   , MUII_ButtonBack,
		TAG_DONE);
	}

	case MUIO_Checkmark: /* STRPTR label */
	{
	    return MUI_NewObject(MUIC_Text, //Image
		    ButtonFrame,
		    MUIA_Weight,0,
//	            MUIA_Image_Spec, MUII_CheckMark,
		    MUIA_Text_Contents, "C",
	            MUIA_InputMode, MUIV_InputMode_Toggle,
//	            MUIA_Frame, MUIV_Frame_None,
//	            MUIA_ShowSelState, FALSE,
		    TAG_DONE);

#if 0
	    return MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz,TRUE,
		Child, Label(params[0]),
		Child, MUI_NewObject(MUIC_Image,
	            MUIA_Image_Spec, MUII_CheckMark,
	            MUIA_InputMode, MUIV_InputMode_Toggle,
	            MUIA_Frame, MUIV_Frame_None,
	            MUIA_ShowSelState, FALSE,
		    TAG_DONE),
		TAG_DONE);
	    break;
#endif
	}

	case MUIO_Cycle: /* STRPTR label, STRPTR *entries */
		return MUI_NewObject(MUIC_Cycle,
		    MUIA_Cycle_Entries, params[1],
		    TAG_DONE);
	    break;

	case MUIO_Radio: /* STRPTR label, STRPTR *entries */
	    break;



	case MUIO_Slider: /* STRPTR label, LONG min, LONG max */
	    return SliderObject, /* control char is missing */
	        MUIA_Numeric_Min, params[1],
	        MUIA_Numeric_Max, params[2],
	        End;
	    break;

	case MUIO_String: /* STRPTR label, LONG maxlen */
	    return MUI_NewObject(MUIC_String,
	        StringFrame,
	    	MUIA_String_MaxLen,params[1],
	    	TAG_DONE);
	    break;


	case MUIO_PopButton: /* STRPTR imagespec */
	    return MUI_NewObject(MUIC_Text,//MUIC_Image,
	    	ButtonFrame,
	    	MUIA_Weight,0,
	    	MUIA_Text_SetVMax,FALSE,
//		MUIA_Image_Spec, params[0],
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_Text_Contents, "Pop",
//		MUIA_ShowSelState, FALSE,
		TAG_DONE);
	    break;

	case MUIO_HSpace: /* LONG space   */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_FixWidth, params[0],
		TAG_DONE);
	    break;

	case MUIO_VSpace: /* LONG space   */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_FixHeight, params[0],
		TAG_DONE);
	    break;

	case MUIO_HBar: /* LONG space   */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_Rectangle_HBar, TRUE,
		MUIA_FixHeight, params[0],
		TAG_DONE);
	    break;

	case MUIO_VBar: /* LONG space   */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_Rectangle_VBar, TRUE,
		MUIA_FixWidth, params[0],
		TAG_DONE);
	    break;

	case MUIO_MenustripNM: /* struct NewMenu *nm, ULONG flags */
	    return CreateMenuString((struct NewMenu *)params[0],params[1],MUIMasterBase);

	case MUIO_Menuitem: /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
	    return MUI_NewObject( MUIC_Menuitem,
		MUIA_Menuitem_Title, params[0],
		MUIA_Menuitem_Shortcut, params[1],
		/* flags NYI */
		MUIA_UserData, params[3],
	    	TAG_DONE);

	case MUIO_BarTitle: /* STRPTR label */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_Rectangle_HBar, TRUE,
		MUIA_Rectangle_BarTitle, params[0],
		TAG_DONE);
	    break;

	case MUIO_NumericButton: /* STRPTR label, LONG min, LONG max, STRPTR format */
	    break;
    }


    return NULL;

    AROS_LIBFUNC_EXIT

} /* MUIA_MakeObjectA */
