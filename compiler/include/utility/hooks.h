#ifndef UTILITY_HOOKS_H
#define UTILITY_HOOKS_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif
#ifndef AROS_ASMCALL_H
#   include <aros/asmcall.h>
#endif

/* A callback Hook */
struct Hook
{
    struct MinNode  h_MinNode;
    APTR	    h_Entry;        /* Main entry point */
    APTR	    h_SubEntry;     /* Secondary entry point */
    APTR	    h_Data;	    /* Whatever you want */
};

/* You can use this if you want for casting function pointers. */
typedef IPTR (*HOOKFUNC)();

/*
    Calling conventions for Amiga Hooks.

    The callback hook is invoked with the following parameters, which
    on an Amiga/m68k are passed in the following registers:

    A0	-   pointer to the Hook data structure
    A2	-   pointer to Hook specific data ("object")
    A1	-   pointer to Hook parameters ("message")

    When the Hook is invoked, control is passed to the h_Entry function.
    This function MUST be defined in the following manner for correct
    operation on all systems.

    AROS_UFH3(  return type ,  function name ,
	AROS_UFHA(struct Hook *,    hook, A0),
	AROS_UFHA(APTR,             object, A2),
	AROS_UFHA(APTR,             message, A1)
    )

    Note the order of the arguments, hook, object, message.

    ----------------------------------------------------------------------
     Note for people using Amiga compilers without registerized arguments
    ----------------------------------------------------------------------

    If your compiler cannot accept these parameters in these registers
    (where applicable), you will have to define a small assembly stub
    to push the arguments on the stack, and then call the h_SubEntry
    function.

    However this is unlikely to be required unless you are trying to
    compile your program on an Amiga with a very old compiler.

    A sample stub (in new Motorola m68k syntax):

    _HookEntry:
	move.l	a1,-(sp)
	move.l	a2,-(sp)
	move.l	a0,-(sp)
	move.l	(h_SubEntry,a0),a0
	jsr	(a0)
	lea	(12,sp),sp
	rts

    There is a suitable function defined in amiga.lib called HookEntry
    that can be used for this purpose. See the documentation of HookEntry
    for more information.

*/

#define CALLHOOKPKT(hook, object, message)     \
    AROS_UFC3                                  \
    (                                          \
        IPTR, ((struct Hook *) hook)->h_Entry, \
	AROS_UFCA(struct Hook *, hook,    A0), \
	AROS_UFCA(APTR,          object,  A2), \
	AROS_UFCA(APTR,          message, A1)  \
    )

#endif /* UTILITY_HOOKS_H */
