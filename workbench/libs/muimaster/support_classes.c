/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <stdio.h>

#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>


#include "mui.h"
#include "support.h"
#include "support_classes.h"
#include "muimaster_intern.h"

#include "debug.h"

static const struct __MUIBuiltinClass * const builtins[] =
{
    &_MUI_Notify_desc,
    &_MUI_Family_desc,
    &_MUI_Application_desc,
    &_MUI_Window_desc,
    &_MUI_Area_desc,
    &_MUI_Rectangle_desc,
    &_MUI_Group_desc,
    &_MUI_Image_desc,
    &_MUI_Configdata_desc,
    &_MUI_Text_desc,
    &_MUI_Numeric_desc,
    &_MUI_Slider_desc,
    &_MUI_String_desc,
    ZUNE_BOOPSI_DESC
    &_MUI_Prop_desc,
    &_MUI_Scrollbar_desc,
    &_MUI_Register_desc,
    &_MUI_Menuitem_desc,
    &_MUI_Menu_desc,
    &_MUI_Menustrip_desc,
    ZUNE_VIRTGROUP_DESC
    ZUNE_SCROLLGROUP_DESC
    &_MUI_Scrollbutton_desc,
    &_MUI_Semaphore_desc,
    &_MUI_Dataspace_desc,
    &_MUI_Bitmap_desc,
    &_MUI_Bodychunk_desc,
    &_MUI_ChunkyImage_desc,
    &_MUI_Cycle_desc,
    &_MUI_Popstring_desc,
    &_MUI_Listview_desc,
    &_MUI_List_desc,
    ZUNE_POPASL_DESC
    &_MUI_Popobject_desc,
    ZUNE_GAUGE_DESC
    ZUNE_ABOUTMUI_DESC
    ZUNE_SETTINGSGROUP_DESC
    ZUNE_IMAGEADJUST_DESC
    ZUNE_POPIMAGE_DESC
    ZUNE_SCALE_DESC
    ZUNE_RADIO_DESC
    &_MUI_IconList_desc,
    &_MUI_IconDrawerList_desc,
    &_MUI_IconVolumeList_desc,
    ZUNE_ICONLISTVIEW_DESC
    ZUNE_BALANCE_DESC
    ZUNE_COLORFIELD_DESC
    ZUNE_COLORADJUST_DESC
    ZUNE_IMAGEDISPLAY_DESC
    ZUNE_PENDISPLAY_DESC
    ZUNE_PENADJUST_DESC
    ZUNE_POPPEN_DESC
    &_MUI_Mccprefs_desc,
    ZUNE_FRAMEDISPLAY_DESC
    ZUNE_POPFRAME_DESC
    ZUNE_FRAMEADJUST_DESC
    ZUNE_VOLUMELIST_DESC
    ZUNE_DIRLIST_DESC
    ZUNE_NUMERICBUTTON_DESC
    ZUNE_POPLIST_DESC
    ZUNE_CRAWLING_DESC
    ZUNE_POPSCREEN_DESC
    ZUNE_LEVELMETER_DESC
};

Class *ZUNE_GetExternalClass(ClassID classname, struct Library *MUIMasterBase)
{
    struct Library         *mcclib = NULL;
    struct MUI_CustomClass *mcc    = NULL ;
    CONST_STRPTR const     *pathptr;
    TEXT                    s[255];

    static CONST_STRPTR const searchpaths[] =
    {
        "Zune/%s",
        "Classes/Zune/%s",
        "MUI/%s",
        NULL,
    };

    for (pathptr = searchpaths; *pathptr; pathptr++)
    {
	snprintf(s, 255, *pathptr, classname);

	D(bug("Trying opening of %s\n",s));

        if ((mcclib = OpenLibrary(s, 0)))
	{
	    D(bug("Calling MCC Query. Librarybase at 0x%lx\n",mcclib));

	    mcc = MCC_Query(0);
	    if (mcc)
	    {
		if (mcc->mcc_Class)
		{
		    mcc->mcc_Module = mcclib;
		    D(bug("Successfully opened %s as external class\n",classname));

		    return mcc->mcc_Class;
		}
	    }

	    CloseLibrary(mcclib);
	}
    }

    D(bug("Failed to open external class %s\n",classname));
    return NULL;
}

/**************************************************************************/
Class *ZUNE_FindBuiltinClass(ClassID classid, struct Library *mb)
{
    Class *cl = NULL, *cl2;

    ForeachNode(&MUIMB(mb)->BuiltinClasses, cl2)
    {
        if (!strcmp(cl2->cl_ID, classid))
	{
            cl = cl2;
	    break;
	}
    }

    return cl;
}

Class *ZUNE_GetBuiltinClass(ClassID classid, struct Library *mb)
{
    Class *cl;

    ObtainSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    cl = ZUNE_FindBuiltinClass(classid, mb);

    if (!cl)
    {
        cl = ZUNE_MakeBuiltinClass(classid, mb);

	if (cl)
	{
	    ZUNE_AddBuiltinClass(cl, mb);

	    /* Increase the reference counter */
	    cl->cl_Dispatcher.h_Data++;
	}
    }

    ReleaseSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);

    return cl;
}

Class *ZUNE_MakeBuiltinClass(ClassID classid, struct Library *MUIMasterBase)
{
    int i;
    Class *cl          = NULL;
    struct Library *mb = NULL;


    for (i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++)
    {
	if (!strcmp(builtins[i]->name, classid))
	{
            Class   *supercl;
            ClassID  superclid;

	    /* This may seem strange, but opening muimaster.library here is done in order to
               increase muimaster.library's open count, so that it doesn't get expunged
	       while some of its internal classes are still in use.  */

	    mb = OpenLibrary("muimaster.library", 0);

            /* It can't possibly fail, but well... */
	    if (!mb)
	        break;

	    if (strcmp(builtins[i]->supername, ROOTCLASS) == 0)
            {
                superclid  = ROOTCLASS;
                supercl    = NULL;
            }
            else
            {
                superclid  = NULL;
                supercl    = MUI_GetClass(builtins[i]->supername);

                if (!supercl) break;
	    }

            cl = MakeClass(builtins[i]->name, superclid, supercl, builtins[i]->datasize, 0);
	    if (cl)
            {
#ifdef __MAXON__
                cl->cl_Dispatcher.h_Entry    = builtins[i]->dispatcher;
#else
                cl->cl_Dispatcher.h_Entry    = (HOOKFUNC)metaDispatcher;
                cl->cl_Dispatcher.h_SubEntry = builtins[i]->dispatcher;
#endif
                /* Use this as a reference counter */
                cl->cl_Dispatcher.h_Data     = 0;
	    }

	    break;
	}
    }

    if (!cl && mb)
        CloseLibrary(mb);

    return cl;
}

/*
 * metaDispatcher - puts h_Data in A6 and calls real dispatcher
 */

#ifdef __AROS__
AROS_UFH3(IPTR, metaDispatcher,
	AROS_UFHA(struct IClass  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
{
    return AROS_UFC4(IPTR, cl->cl_Dispatcher.h_SubEntry,
        AROS_UFPA(Class  *, cl,  A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1),
        AROS_UFPA(APTR    , cl->cl_Dispatcher.h_Data, A6)
    );
}

#else
#ifdef __MAXON__
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
{
	  /* We don't use a metaDispatcher */
    return 0;
}
#else
__asm ULONG metaDispatcher(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg)
{
    __asm ULONG (*entry)(register __a0 struct IClass *cl, register __a2 Object *obj, register __a1 Msg msg) =
	(__asm ULONG (*)(register __a0 struct IClass *, register __a2 Object *, register __a1 Msg))cl->cl_Dispatcher.h_SubEntry;

    putreg(REG_A6,(long)cl->cl_Dispatcher.h_Data);
    return entry(cl,obj,msg);
}
#endif
#endif



