/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
 
    Initialize a BOOPSI class.
*/

#include <exec/lists.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include "intuition_intern.h"

/*****************************************************************************
 
    NAME */
#include <intuition/classes.h>
#include <proto/intuition.h>

AROS_LH5(struct IClass *, MakeClass,

         /*  SYNOPSIS */
         AROS_LHA(UBYTE         *, classID, A0),
         AROS_LHA(UBYTE         *, superClassID, A1),
         AROS_LHA(struct IClass *, superClassPtr, A2),
         AROS_LHA(ULONG          , instanceSize, D0),
         AROS_LHA(ULONG          , flags, D1),

         /*  LOCATION */
         struct IntuitionBase *, IntuitionBase, 113, Intuition)

/*  FUNCTION
    Only for class implementators.
 
    This function creates a new public BOOPSI class. The SuperClass
    should be another BOOPSI class; all BOOPSI classes are subclasses
    of the ROOTCLASS.
 
    SuperClasses can by private or public. You can specify a name/ID
    for the class if you want it to become a public class. For public
    classes, you must call AddClass() afterwards to make it public
    accessible.
 
    The return value contains a pointer to the IClass structure of your
    class. You must specify your dispatcher in cl_Dispatcher. You can
    also store shared data in cl_UserData.
 
    To get rid of the class, you must call FreeClass().
 
    INPUTS
    classID - NULL for private classes otherwise the name/ID of the
        public class.
    superClassID - Name/ID of a public SuperClass. NULL is you don't
        want to use a public SuperClass or if you have the pointer
        your SuperClass.
    superClassPtr - Pointer to the SuperClass. If this is non-NULL,
        then superClassID is ignored.
    instanceSize - The amount of memory which your objects need (in
        addition to the memory which is needed by the SuperClass(es))
    flags - For future extensions. To maintain comaptibility, use 0
        for now.
 
    RESULT
    Pointer to the new class or NULL if
    - There wasn't enough memory
    - The superclass couldn't be found
    - There already is a class with the same name/ID.
 
    NOTES
    No copy is made of classID. So make sure the lifetime of the contents
    of classID is at least the same as the lifetime of the class itself.
 
    EXAMPLE
 
    BUGS
 
    SEE ALSO
 
    INTERNALS
 
*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    Class *iclass = NULL;
    
    EXTENDUWORD(instanceSize);

    DEBUG_MAKECLASS(dprintf("MakeClass: ID <%s> SuperID <%s> Super 0x%lx Size 0x%lx Flags 0x%lx\n",
                            classID ? classID : (UBYTE*)"NULL",
                            superClassID ? superClassID : (UBYTE*)"NULL",
                            superClassPtr,
                            instanceSize,
                            flags));

    /* trust the user ;-) */
    if (!superClassID && !superClassPtr)
    {
#ifdef __MORPHOS__
        /* Workaround for buggy callers: set z flag - Piru */
        REG_SR |= 4;
#endif
        return NULL;
    }

    ObtainSemaphoreShared(&GetPrivIBase(IntuitionBase)->ClassListLock);

    /* Does this class already exist? */
    if (!FindClass(classID))
    {
        /* Has the user specified a classPtr? */
        if (!superClassPtr)
        {
            /* Search for the class... */
            superClassPtr = FindClass(superClassID);
        }

        if (superClassPtr)
        {
            /* Allocate memory */
            iclass = (Class *) AllocMem
            (
                sizeof(Class), MEMF_PUBLIC | MEMF_CLEAR
            );
            
            if (iclass != NULL)
	    {
                /* Initialize fields */
		iclass->cl_Super      = superClassPtr;
		iclass->cl_ID	      = classID;
		iclass->cl_InstOffset = superClassPtr->cl_InstOffset +
					superClassPtr->cl_InstSize;
		iclass->cl_InstSize   = instanceSize;
		iclass->cl_Flags      = flags;
                iclass->cl_ObjectSize = iclass->cl_InstOffset 
                                      + iclass->cl_InstSize
                                      + sizeof(struct _Object);
                
                /* Initialize memory subsystem */
                iclass->cl_MemoryPool = CreatePool
                (
                    MEMF_ANY | MEMF_CLEAR | MEMF_SEM_PROTECTED, 
                    32 * iclass->cl_ObjectSize, iclass->cl_ObjectSize
                );
                   
                if (iclass->cl_MemoryPool != NULL)
                {
                    /* SuperClass is used one more time now */
                    AROS_ATOMIC_INC(superClassPtr->cl_SubclassCount);
                }
                else
                {
                    FreeMem(iclass, sizeof(Class));
                    iclass = NULL;
                }
            }
        }
        else
        {
            DEBUG_MAKECLASS(dprintf("MakeClass: superclass not found\n"));
        }
    }
    else
    {
        DEBUG_MAKECLASS(dprintf("MakeClass: already there\n"));
    }

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ClassListLock);

    DEBUG_MAKECLASS(dprintf("MakeClass: return 0x%lx\n", iclass));

#ifdef __MORPHOS__
    /* Workaround for buggy callers: clear/set z flag - Piru */
    if (iclass) REG_SR &= (ULONG) ~4;
    else        REG_SR |= 4;
#endif

    return iclass;

    AROS_LIBFUNC_EXIT
} /* MakeClass() */
