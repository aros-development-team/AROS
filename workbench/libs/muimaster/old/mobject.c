/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <utility/tagitem.h>
#endif

#include <zunepriv.h>
#include <muio.h>
#include <macros.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <renderinfo.h>
#include <Text.h>
#include <Area.h>
#include <Rectangle.h>
#include <Image.h>
#include <Group.h>
#include <Cycle.h>

#define MAX_TAGS 50

/*
 * Create an object from a class.
 */
/*****************************************************************************

    NAME */
	AROS_LH2(Object *, MUI_NewObjectA,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR    , className, A0),
	AROS_LHA(struct TagItem *, tags,      A1),

/*  LOCATION */
	struct Library *, MUIMasterBase, 5, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    Class  *cl;

/*      g_print("MUI_NewObjectA %s\n", className); */

    cl = MUI_GetClass(className);
    if (cl)
    {
#warning FIXME: I should increase the open count of library (use cl->hook->data)
	return NewObjectA(cl, NULL, tags);
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

/*
 * Delete a MUI object.
 */
/*****************************************************************************

    NAME */
	AROS_LH1(VOID, MUI_DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 6, MUIMaster)

/*  IMPLEMENTATION
 *	MUI will call DisposeObject(), then call CloseLibrary() on
 *	OCLASS(obj)->h_Data if cl_ID!=NULL && h_Data!=NULL.
 */
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#warning FIXME: I should decrease the open count of library (use cl->hook->data)
    DisposeObject(obj);

    AROS_LIBFUNC_EXIT
}


#ifndef MUI_MakeObject
#define MUI_MakeObject(a0, tags...) \
	({ULONG _tags[] = { tags }; MUI_MakeObjectA((a0), (ULONG *)_tags);})
#endif /* !MUI_MakeObject */

/*
 * create an object from the builtin object collection.
 */
/*****************************************************************************

    NAME */
	AROS_LH2(Object *, MUI_MakeObjectA,

/*  SYNOPSIS */
	AROS_LHA(LONG   , type,   D0),
	AROS_LHA(ULONG *, params, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 20, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

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
		MUIA_Text_HiChar, params[1] & 0xff,
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
	}

	case MUIO_Cycle: /* STRPTR label, STRPTR *entries */
	    return MUI_NewObject(MUIC_Group,
		MUIA_Group_Horiz,TRUE,
		Child, LLabel1(params[0]),
		Child, MUI_NewObject(MUIC_Cycle,
		    MUIA_Cycle_Entries, params[1],
		    TAG_DONE),
		TAG_DONE);
	    break;

	case MUIO_Radio: /* STRPTR label, STRPTR *entries */
	    break;

	case MUIO_Slider: /* STRPTR label, LONG min, LONG max */
	    break;

	case MUIO_String: /* STRPTR label, LONG maxlen */
	    break;

	case MUIO_PopButton: /* STRPTR imagespec */
	    return MUI_NewObject(MUIC_Image,
		MUIA_Image_Spec, params[0],
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_ShowSelState, FALSE,
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
	    break;

	case MUIO_Menuitem: /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
	    break;

	case MUIO_BarTitle: /* STRPTR label */
	    return MUI_NewObject(MUIC_Rectangle,
		MUIA_Rectangle_HBar, TRUE,
		MUIA_Rectangle_BarTitle, params[0],
		TAG_DONE);
	    break;

	case MUIO_NumericButton: /* STRPTR label, LONG min, LONG max, STRPTR format */
	    break;

	default:
#warning FIXME: complain loud for missing/wrong MUIO_something
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}


/*
 * Redraw yourself.
 * Instead of drawing directly during OM_SET, you should simply call
 * MUI_Redraw(). MUI calculates all necessary coordinates
 * and clip regions (in case of virtual groups) and then sends
 * a MUIM_Draw method to your object.
 */
/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_Redraw,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj,   A0),
	AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 17, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    muiRenderInfo(obj)->mri_ClipRect.x      = _left(obj);
    muiRenderInfo(obj)->mri_ClipRect.y      = _top(obj);
    muiRenderInfo(obj)->mri_ClipRect.width  = _width(obj);
    muiRenderInfo(obj)->mri_ClipRect.height = _height(obj);

    DoMethod(obj, MUIM_Draw, flags);

    AROS_LIBFUNC_EXIT
}


/*
 * Pop up a MUI requester.
 */
/*****************************************************************************

    NAME */
	AROS_LH7(LONG, MUI_RequestA,

/*  SYNOPSIS */
	AROS_LHA(APTR        , app,     D0),
	AROS_LHA(APTR        , win,     D1),
	AROS_LHA(LONG        , flags,   D2),
	AROS_LHA(CONST_STRPTR, title,   A0),
	AROS_LHA(CONST_STRPTR, gadgets, A1),
	AROS_LHA(CONST_STRPTR, format,  A2),
	AROS_LHA(APTR        , params,  A3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 7, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#warning FIXME: implement MUI_RequestA
    return 0;

    AROS_LIBFUNC_EXIT
}


/*
 * Request input events for your custom class.
 * Whenever an input event you requested arrives at your parent
 * windows message port, your object will receive a
 * MUIM_HandleInput method.
 */
/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RequestIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj,   A0),
	AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 15, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

/* Get window object
 * Get list of objects that requested idcmp
 * put object and idcmp in list
 * window will check that list for matching idcmp
 */
    Object *winobj = _win(obj);

    if (winobj != NULL)
    {

    }

    AROS_LIBFUNC_EXIT
}



/*
 * Reject previously requested input events.
 */
/*****************************************************************************

    NAME */
	AROS_LH2(VOID, MUI_RejectIDCMP,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj,   A0),
	AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 16, MUIMaster)

{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)


    AROS_LIBFUNC_EXIT
}
