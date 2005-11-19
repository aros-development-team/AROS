/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*
 * notify2.c : Test the MUI_Notify class, and show how to connect actions
 * to attributes changes.
 * Copied from a MUI2C example. Greetings to Jason Birch, MUI2C author.
 */

#include <exec/types.h>

//#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#ifdef __AROS__
#include <libraries/mui.h>
#endif
#include <proto/utility.h>
#include <stdio.h>

struct Library       *MUIMasterBase;

#ifndef __AROS__

#include <mui.h>
#undef SysBase

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int openmuimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    __zune_prefs_init(&__zprefs);

    return 1;
}

void closemuimaster(void)
{
}

#else

int openmuimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void closemuimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif

#ifndef TAGBASE
#define TAGBASE (TAG_USER | (9853 << 16))
#endif

/*
 * This example uses 2 custom classes, Test and ExtendedTest.
 * Hierarchy : MUIC_Notify <-- Test <-- ExtendedTest.
 * Setting public attributes of these classes will trigger notifications.
 */


/* Test : This is a simple class with 2 numeric attributes A and B. */

/*
 * Private data structure for instances of class Test.
 */
struct TestData {
    int a;
    int b;
};

/*
 * Public attributes of class Test
 * i : init
 * s : set
 * g : get
 */
enum {
    MUIA_Test_A = TAGBASE,  /* isg */
    MUIA_Test_B,            /* isg */
};

/*
 * Public methods of class Test
 */
enum {
    MUIM_Test_Print = TAGBASE,
    MUIM_Test_GetBoth,
};

/*
 * Special parameter structures for Test methods.
 */
struct MUIP_Test_Print   { ULONG MsgID; };
struct MUIP_Test_GetBoth { ULONG MsgID; int *x; int *y;};



/*
 * Constructor of Test object.
 */
static ULONG
Test_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TestData *data;
    struct TagItem  *tag;
    /*
     * Call constructor of superclass.
     */
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return 0;
    /*
     * Set default values to attributes.
     */
    data = INST_DATA(cl, obj);
    data->a = 0;
    data->b = 0;
    /*
     * Init attributes.
     */ 
    if ((tag = FindTagItem(MUIA_Test_A, msg->ops_AttrList)))
	data->a = tag->ti_Data;
    if ((tag = FindTagItem(MUIA_Test_B, msg->ops_AttrList)))
	data->b = tag->ti_Data;
    /*
     * Return newly constructed object.
     */
    return (ULONG)obj;
}


/*
 * Setting public attributes. The tags in the message may not be ours,
 * so do't forget to pass them to the super class.
 */
static ULONG 
Test_Set(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct TestData       *data = INST_DATA(cl, obj);
    const struct TagItem  *tags = msg->ops_AttrList;
    struct TagItem        *tag;

    /* There are many ways to find out what tag items provided by set()
    ** we do know. The best way should be using NextTagItem() and simply
    ** browsing through the list.
    */
    while ((tag = NextTagItem(&tags)) != NULL)
    {
	switch (tag->ti_Tag)
	{
	case MUIA_Test_A:
	    data->a = tag->ti_Data;
	    break;
	case MUIA_Test_B:
	    data->b = tag->ti_Data;
	    break;
	}
    }
    /*
     * To handle unkown attributes and notifications.
     */
    return(DoSuperMethodA(cl, obj, (Msg) msg));
}


/*
 * Getting public attributes.
 */
static ULONG
Test_Get(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct TestData *data = INST_DATA(cl, obj);
#define STORE *(msg->opg_Storage)

    switch(msg->opg_AttrID)
    {
    case MUIA_Test_A:
	STORE = (ULONG) data->a;
	return(TRUE);
    case MUIA_Test_B:
	STORE = (ULONG) data->b;
	return(TRUE);
    }

    /* Our handler didn't understand the attribute, we simply pass
    ** it to our superclass now.
    */
    return(DoSuperMethodA(cl, obj, (Msg) msg));
#undef STORE
}


/*
 * Special get method to get both attributes.
 */
static ULONG 
Test_GetBoth(struct IClass *cl, Object *obj, struct MUIP_Test_GetBoth *msg)
{
    struct TestData *data = INST_DATA(cl, obj);

    *(msg->x) = data->a;
    *(msg->y) = data->b;
    return TRUE;
}


/*
 * Print attributes value.
 */
static ULONG 
Test_Print(struct IClass *cl, Object *obj, struct MUIP_Test_Print *msg)
{
    struct TestData *data = INST_DATA(cl, obj);
    printf("A value: %d. B value: %d.\n", data->a, data->b);
    return TRUE;
}


/*
 * Test class method dispatcher.
 */
#ifndef __AROS__
__asm IPTR Test_Dispatcher(register __a0 Class *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, Test_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    AROS_USERFUNC_INIT

    /*
     * Watch out for methods we do understand.
     */
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
	case OM_NEW:
	    return(Test_New(cl, obj, (struct opSet *) msg));
	case OM_SET:
	    return(Test_Set(cl, obj, (struct opSet *)msg));
	case OM_GET:
	    return(Test_Get(cl, obj, (struct opGet *)msg));
	case MUIM_Test_Print:
	    return(Test_Print(cl, obj, (APTR)msg));
	case MUIM_Test_GetBoth:
	    return(Test_GetBoth(cl, obj, (APTR)msg));
    }
    /*
     * We didn't understand the last method, so call our superclass.
     */
    return(DoSuperMethodA(cl, obj, msg));

    AROS_USERFUNC_EXIT
}


/******************************************************************************/
/* Extended Test : holds statistics about Test attributes, and update them    */
/* automatically when they change.                                            */
/******************************************************************************/

/*
 * Public methods of class ExtendedTest
 */
enum {
    MUIM_ExtendedTest_Update = TAGBASE+11,
    MUIM_ExtendedTest_Print,
};

struct MUIP_ExtendedTest_Update   { ULONG MsgID; };
struct MUIP_ExtendedTest_Print    { ULONG MsgID; };

/*
 * Internal attributes of class ExtendedTest
 */
struct ExtendedTestData {
    int sum;
    int average;
    int usecount;
};

/*
 * Constructor of ExtendedTest object.
 */
static ULONG
ExtendedTest_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct ExtendedTestData *data;
    /*
     * Call constructor of superclass.
     */
    obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg);
    if (!obj)
	return 0;
    /*
     * Set default values to attributes.
     */
    data = INST_DATA(cl, obj);
    DoMethod(obj, MUIM_ExtendedTest_Update);
    data->usecount = 0;
    /*
     * Setup notifications on our attributes.
     */
    DoMethod(obj, MUIM_Notify,
	     MUIA_Test_A,                /* attribute to watch */
	     MUIV_EveryTime,             /* notify when setting to everything */
	     (IPTR)obj,                  /* object to call on notification */
             1,                          /* number of parameters following */
             MUIM_ExtendedTest_Update);  /* method to invoke */

    DoMethod(obj, MUIM_Notify,
	     MUIA_Test_B,
	     MUIV_EveryTime,
	     (IPTR)obj,
	     1,
	     MUIM_ExtendedTest_Update);
    /*
     * Return newly constructed object.
     */
    return (ULONG)obj;
}


/*
 * Recalculate sum and average.
 */
static ULONG
ExtendedTest_Update(struct IClass *cl, Object *obj,
		    struct MUIP_ExtendedTest_Update *noMsg)
{
    struct ExtendedTestData *data = INST_DATA(cl, obj);
    int a;
    int b;

    DoMethod(obj, MUIM_Test_GetBoth, (IPTR)&a, (IPTR)&b);
    data->sum = a + b;
    data->average = (a + b) / 2;
    data->usecount++;
    return TRUE;
}


/*
 * Print values.
 */
static ULONG
ExtendedTest_Print(struct IClass *cl, Object *obj,
		   struct MUIP_ExtendedTest_Print *noMsg)
{
    struct ExtendedTestData *data = INST_DATA(cl, obj);

    DoMethod(obj, MUIM_Test_Print);
    printf("Sum: %d. Average: %d. Usecount: %d.\n",
	   data->sum,
	   data->average,
	   data->usecount);

    return FALSE;
}


/*
 * ExtendedTest class method dispatcher.
 */
#ifndef __AROS__
__asm IPTR ExtendedTest_Dispatcher(register __a0 Class *cl, register __a2 Object *obj, register __a1 Msg msg)
#else
AROS_UFH3S(IPTR, ExtendedTest_Dispatcher,
	AROS_UFHA(Class  *, cl,  A0),
	AROS_UFHA(Object *, obj, A2),
	AROS_UFHA(Msg     , msg, A1))
#endif
{
    AROS_USERFUNC_INIT

    /*
     * Watch out for methods we do understand.
     */
    switch (msg->MethodID)
    {
	/* Whenever an object shall be created using NewObject(), it will be
	** sent a OM_NEW method.
	*/
    case OM_NEW:
	return(ExtendedTest_New(cl, obj, (struct opSet *) msg));
    case MUIM_ExtendedTest_Print:
	return(ExtendedTest_Print(cl, obj, (APTR)msg));
    case MUIM_ExtendedTest_Update:
	return(ExtendedTest_Update(cl, obj, (APTR)msg));
    }
    /*
     * We didn't understand the last method, so call our superclass.
     */
    return(DoSuperMethodA(cl, obj, msg));

    AROS_USERFUNC_EXIT
}


/*
 * Create both classes, create an ExtendedTest object,
 * changes attributes and print values.
 */
int main (void)
{
    Object                 *obj;
    struct MUI_CustomClass *testClass;
    struct MUI_CustomClass *extendedTestClass;
    int result = 0;

    if (!openmuimaster()) return 20;

    testClass = MUI_CreateCustomClass(NULL, MUIC_Notify, NULL,
				      sizeof(struct TestData),
				      Test_Dispatcher);
    if (!testClass)
    {
	printf("cannot create Test class\n");
	result = 5;
	goto error;
    }
    extendedTestClass = MUI_CreateCustomClass(NULL, NULL, testClass,
					      sizeof(struct ExtendedTestData),
					      ExtendedTest_Dispatcher);
    if (!extendedTestClass)
    {
	MUI_DeleteCustomClass(testClass);
	printf("cannot create ExtendedTest class\n");
	result = 5;
	goto error;
    }

    obj = NewObject(extendedTestClass->mcc_Class, NULL,
		    MUIA_Test_A, 10,
		    MUIA_Test_B, 20,
		    TAG_DONE);
    if (obj)
    {
	int x;
	int y;

	printf("\nSum and Average will be automatically calculated"
	    " after each change of A and B values.\n\n");
	printf("- Show the contents of the object:\n");
	DoMethod(obj, MUIM_ExtendedTest_Print);

	printf("\n- Set the A attribute of the object to 5 and check its new contents:\n");
	set(obj, MUIA_Test_A, 5);
	DoMethod(obj, MUIM_ExtendedTest_Print);

	printf("\n- Set the B attribute of the object to 10 and check its new contents:\n");
	set(obj, MUIA_Test_B, 10);
	DoMethod(obj, MUIM_ExtendedTest_Print);

	printf("\n- Get the A and B attributes using MUIP structure:\n");
	DoMethod(obj, MUIM_Test_GetBoth, (IPTR)&x, (IPTR)&y);
	printf("Values returned: %d %d.\n", x, y);
	DisposeObject(obj);
    }
    MUI_DeleteCustomClass(extendedTestClass);
    MUI_DeleteCustomClass(testClass);

error:

    closemuimaster();

    return result;
}
