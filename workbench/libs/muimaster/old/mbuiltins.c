/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef __AROS__
#include <proto/intuition.h>
#include <proto/muimaster.h>
#endif

#include <zunepriv.h>
#include <builtin.h>
#include <mcc.h>
#include <string.h>

static const struct __MUIBuiltinClass *builtins[] =
{
    &_MUI_Notify_desc,
    &_MUI_Family_desc,
    &_MUI_Application_desc,
    &_MUI_Window_desc,
    &_MUI_Area_desc,
    &_MUI_Rectangle_desc,
    &_MUI_Group_desc,
//    &_MUI_Image_desc,
//    &_MUI_Dataspace_desc,
//    &_MUI_Configdata_desc,
    &_MUI_Text_desc,
    &_MUI_Numeric_desc,
    &_MUI_Slider_desc,
};

static int num_builtins = sizeof(builtins) / sizeof(struct __MUIBuiltinClass *);

#ifdef AMIGA
AROS_UFP3(IPTR, metaDispatcher,
	AROS_UFPA(Class  *, cl,  A0),
	AROS_UFPA(Object *, obj, A2),
	AROS_UFPA(Msg     , msg, A1));
#endif

/*
 * Given the builtin class, construct the
 * class and make it public (because of the fake lib base).
 */
static Class *
builtin_to_public_class(const struct __MUIBuiltinClass *desc)
{
    Class *cl;
    Class *superClassPtr;
    CONST_STRPTR superClassID = NULL;

//kprintf("*** builtin_to_public_class()\n");
    if (strcmp(desc->supername, ROOTCLASS) == 0)
    {
        superClassID  = desc->supername;
        superClassPtr = NULL;
    }
    else
    {
        superClassID  = NULL;
        superClassPtr = MUI_GetClass(desc->supername);
        if (!superClassPtr)
            return NULL;
    }

    cl = MakeClass(desc->name, superClassID, superClassPtr, desc->datasize, 0);
    if (!cl)
    {
	//kprintf("*** MakeClass(\"%s\", \"%s\", ...) failed\n", desc->name, desc->supername);
	return NULL;
    }

#ifdef AMIGA
    cl->cl_Dispatcher.h_Entry = metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = desc->dispatcher;
    cl->cl_Dispatcher.h_Data = MUIMasterBase;
#else
    cl->cl_Dispatcher.h_Entry = desc->dispatcher;
#endif

    return cl;
}


/*
 * Create a builtin class and all its superclasses.
 */
Class *
_zune_builtin_class_create (CONST_STRPTR className)
{
    int i;

    for (i = 0 ; i < num_builtins ; i++)
    {
	const struct __MUIBuiltinClass *builtin = builtins[i];

	/* found the class to create */
	if (!strcmp(builtin->name, className))
	{
#ifndef __AROS__
	    /* rootclass is our parent */
	    if (!strcmp(builtin->supername, ROOTCLASS))
		_zune_add_root_class();
#endif

	    return builtin_to_public_class(builtin);
	}
    }

    return NULL;
}
