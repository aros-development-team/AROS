/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>

#ifdef _AROS
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>
#endif

#include <zunepriv.h>
#include <muimaster_intern.h>
#include <extclass.h>
#include <builtin.h>
#include <mcc.h>
#include <string.h>

static GMemChunk *mccMemChunk = NULL;

#ifdef _AROS
/* Classes... need a hash table for this */
static Class **Classes = NULL;
static int     ClassCount = 0;
static int     ClassSpace = 0;
#endif

#define CHECK_INIT_MCC \
if (!mccMemChunk) \
{ \
    init_mcc(); \
}

static void destroy_mcc ()
{
    g_mem_chunk_destroy(mccMemChunk);
}

static void init_mcc ()
{
   mccMemChunk = g_mem_chunk_create(struct MUI_CustomClass, 10, G_ALLOC_AND_FREE);    
   g_atexit(destroy_mcc);
}


#ifdef _AROS
static Class *GetPublicClass(CONST_STRPTR className)
{
    Class *cl;
    int i;

kprintf("*** GetPublicClass(\"%s\"): ", className);
    for (i = 0; i < ClassCount; i++)
    {
      cl = Classes[i];
      if (cl && !strcmp(cl->cl_ID, className))
      {
kprintf("FOUND\n");
        return cl;
      }
    }

kprintf("not found\n");
    return NULL;
}

/* FIXME: this should go into muimaster_init.c with all its data */
BOOL destroy_classes(void)
{
    int i;

kprintf("*** destroy_classes: count=%ld\n", ClassCount);

    /* NOTE: not the other way round, otherwise you will
     * try to free superclasses before subclasses... */
    /* TODO: when we'll have a hash table, we'll need to loop thru it
     * until either we don't have any more classes or we can't free any
     * (think of it like the last pass of a bubble sort). */
    for (i = ClassCount-1; i >= 0; i--)
    {
      if (Classes[i])
      {
        if (FreeClass(Classes[i]))
          Classes[i] = NULL;
        else
        {
          kprintf("*** destroy_classes: FreeClass() failed for %s:\n"
                  "    SubclassCount=%ld ObjectCount=%ld\n",
                  Classes[i]->cl_ID,
                  Classes[i]->cl_SubclassCount,
                  Classes[i]->cl_ObjectCount);
          return FALSE;
        }
      }
    }

kprintf("*** destroy_classes: freeing array\n");
    FreeVec(Classes);
    Classes = NULL;
    ClassCount = 0;
    ClassSpace = 0;
    return TRUE;
}

#endif

#ifdef AMIGA

/*
 * metaDispatcher - puts h_Data in A6 and calls real dispatcher
 */
AROS_UFH3(IPTR, metaDispatcher,
	AROS_UFHA(Class  *, cl,  A0),
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

#endif

/* An obsolete function in the API, but very helpful for internals !
 */
/*****************************************************************************

    NAME */
AROS_LH1(struct IClass *, MUI_GetClass,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, className, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 13, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    Class *cl = NULL;

    if (className == NULL)
	return NULL;

kprintf("*** MUI_GetClass(\"%s\")...\n", className);
    cl = GetPublicClass(className);
    if (!cl)
    {
#ifdef _AROS
	/* do we have space for another class pointer? */
	if (ClassCount == ClassSpace)
	{
	  Class **t;
	  int newSpace = ClassSpace ? ClassSpace+16 : 32;

	  t = (Class **)AllocVec(sizeof(Class *) * newSpace, MEMF_ANY);
	  if (t == NULL)
	  {
kprintf("*** MUI_GetClass: no space for class pointers\n");
	    return NULL;
	  }

	  if (Classes != NULL)
	  {
	    CopyMem(Classes, t, sizeof(Class *) * ClassSpace);
kprintf("*** MUI_GetClass: freeing old array\n");
	    FreeVec(Classes);
	  }

	  Classes    = t;
	  ClassSpace = newSpace;
	}
#endif

	cl = _zune_builtin_class_create(className);
	if (!cl)
	{
	    cl = _zune_class_load(className);
	}

#ifdef _AROS
	if (cl)
	{
#warning FIXME: I should increase the open count of library (use cl->hook->data)
	    ASSERT(cl->cl_ID != NULL);
	    ASSERT(strcmp(className, cl->cl_ID) == 0);
kprintf("*** MUI_GetClass: adding class %s\n", cl->cl_ID);
	    Classes[ClassCount++] = cl;
	}
#endif
    }
    return cl;

    AROS_LIBFUNC_EXIT
}

/***************************************************************************

    NAME */
	AROS_LH1(VOID, MUI_FreeClass,

/*  SYNOPSIS */
	AROS_LHA(struct IClass *, classPtr, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 14, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

#warning FIXME: I should decrease the open count of library (use cl->hook->data)
    return;

    AROS_LIBFUNC_EXIT
}

/* To create a class, it may be needed to create the superclass,
 * thus possible indirect recursive calls via MUI_GetClass(supername).
 * So avoid using static storage in all public, builtin and dynamic
 * class creation (yes I was trapped :)
 */
/*****************************************************************************

    NAME */
AROS_LH5(struct MUI_CustomClass *, MUI_CreateCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct Library         *, base,       A0),
	AROS_LHA(CONST_STRPTR            , supername,  A1),
	AROS_LHA(struct MUI_CustomClass *, supermcc,   A2),
	AROS_LHA(LONG                    , datasize,   D0),
	AROS_LHA(APTR                    , dispatcher, A3),

/*  LOCATION */
	struct Library *, MUIMasterBase, 18, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    struct MUI_CustomClass *mcc;
    Class                  *cl, *super;
    ClassID                 id = NULL;

    if ((supername == NULL) && (supermcc == NULL))
	return NULL;

    if (!supermcc)
    {
	super = MUI_GetClass(supername);
	if (!super)
	    return NULL;
    }
    else
	super = supermcc->mcc_Class;

    CHECK_INIT_MCC;
    mcc = g_chunk_new0(struct MUI_CustomClass, mccMemChunk);
    if (!mcc)
	return NULL;

    if (base)
	id = FilePart(((struct Node *)base)->ln_Name);

    cl = MakeClass(id, NULL, super, datasize, 0);
    if (!cl)
    {
	g_chunk_free(mcc, mccMemChunk);
	return NULL;
    }

    mcc->mcc_UtilityBase   = (struct Library *)UtilityBase;
    mcc->mcc_DOSBase       = (struct Library *)DOSBase;
    mcc->mcc_GfxBase       = (struct Library *)GfxBase;
    mcc->mcc_IntuitionBase = (struct Library *)IntuitionBase;

    mcc->mcc_Class  = cl;
    mcc->mcc_Super  = super;
    mcc->mcc_Module = NULL; /* _zune_class_load() will set this */

#ifdef AMIGA
    cl->cl_Dispatcher.h_Entry    = metaDispatcher;
    cl->cl_Dispatcher.h_SubEntry = dispatcher;
    cl->cl_Dispatcher.h_Data     = base;
#else
    cl->cl_Dispatcher.h_Entry = dispatcher;
#endif

    return mcc;

    AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
AROS_LH1(BOOL, MUI_DeleteCustomClass,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_CustomClass *, mcc, A0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 19, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    if (mcc && FreeClass(mcc->mcc_Class))
    {
        CloseLibrary(mcc->mcc_Module);
        g_chunk_free(mcc, mccMemChunk);
        return TRUE;
    }

    return FALSE;

    AROS_LIBFUNC_EXIT
}
