/*
 * Copyright © 2011, The AROS Development Team. All rights reserved.
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 * $Id$
*/

#define __AROS_ALTSTACK_NODEFINE__
#include <aros/altstack.h>

#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************

    NAME
#include <aros/altstack.h>  */

        void aros_init_altstack(

/*  SYNOPSIS */
        struct Task * t)

/*  FUNCTION
        Initialize alternative stack

    INPUTS
        t - Task to initialize

    RESULT
        -

    NOTES
        This function is normally executed by the OS for task started with
        NewAddTask or when using NewStackSwap to run code with a new stack.
        So normally used programs should not need to call this function.

    EXAMPLE

    BUGS

    SEE ALSO
        aros_set_altstack(), aros_get_altstack(),
        aros_push_altstack(), aros_pop_altstack()

    INTERNALS

******************************************************************************/
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;

    stackarray[0] = (IPTR)&stackarray[2];
    stackarray[1] = AROS_ALTSTACK_ID;
}


/*****************************************************************************

    NAME
#include <aros/altstack.h>  */

        IPTR aros_set_altstack(

/*  SYNOPSIS */
        struct Task * t, IPTR value)

/*  FUNCTION
        Replace current top of the stack with a new value.

    INPUTS
        t - Task with alternative stack
        value - Value to replace old value on top of the stack

    RESULT
        The old value that was on top of the stack.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aros_init_altstack(), aros_get_altstack(),
        aros_push_altstack(), aros_pop_altstack()

    INTERNALS

******************************************************************************/
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;
    IPTR *stacktop = (IPTR *)stackarray[0];

    D(
        if (stackarray[1] != AROS_ALTSTACK_ID)
            bug("[aros_set_altstack]: Error! Altstack ID overwritten!\n");

        if (AROS_GET_SP <= (unsigned char *)stacktop)
            bug("[aros_set_altstack]: Error! Stack pointer below top of alternative stack!\n");
    )

    IPTR ret = *stacktop;

    *stacktop = value;

    return ret;
}


/*****************************************************************************

    NAME
#include <aros/altstack.h>  */

        IPTR aros_get_altstack(

/*  SYNOPSIS */
        struct Task * t)

/*  FUNCTION
        Get top value from alternative stack

    INPUTS
        t - Task with alternative stack

    RESULT
        Top value of alternative stack

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aros_init_altstack(), aros_set_altstack(),
        aros_push_altstack(), aros_pop_altstack()

    INTERNALS

******************************************************************************/
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;
    IPTR *stacktop = (IPTR *)stackarray[0];

    D(
        if (stackarray[1] != AROS_ALTSTACK_ID)
            bug("[aros_get_altstack]: Error! Altstack ID overwritten!\n");

        if (AROS_GET_SP <= (unsigned char *)stacktop)
            bug("[aros_get_altstack]: Error! Stack pointer below top of alternative stack!\n");
    )

    return *stacktop;
}


/*****************************************************************************

    NAME
#include <aros/altstack.h>  */

        void aros_push_altstack(

/*  SYNOPSIS */
        struct Task * t, IPTR value)

/*  FUNCTION
        Push a value on alternative stack.

    INPUTS
        t - Task with alternative stack
        value - Value to push on the alternative stack

    RESULT
        -

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aros_init_altstack(), aros_set_altstack(), aros_get_altstack(),
        aros_pop_altstack()

    INTERNALS

******************************************************************************/
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;
    IPTR **stacktopptr = (IPTR **)stackarray;

    D(
        if (stackarray[1] != AROS_ALTSTACK_ID)
            bug("[aros_push_altstack]: Error! Altstack ID overwritten!\n");
    )

    *(++(*stacktopptr)) = value;

    D(
      if (AROS_GET_SP <= (unsigned char *)(*stacktopptr))
            bug("[aros_push_altstack]: Error! Stack pointer below top of alternative stack!\n");
    )
}


/*****************************************************************************

    NAME
#include <aros/altstack.h>  */

        IPTR aros_pop_altstack(

/*  SYNOPSIS */
        struct Task * t)

/*  FUNCTION
        Pop value from alternative stack.

    INPUTS
        t - Task with alternative stack

    RESULT
        Old top value that has been popped from the stack.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        aros_init_altstack(), aros_set_altstack(), aros_get_altstack(),
        aros_push_altstack()

    INTERNALS

******************************************************************************/
{
    IPTR *stackarray = (IPTR *)t->tc_SPLower;
    IPTR **stacktopptr = (IPTR **)stackarray;

    D(
        if (stackarray[1] != AROS_ALTSTACK_ID)
            bug("[aros_pop_altstack]: Error! Altstack ID overwritten!\n");

        if (AROS_GET_SP <= (unsigned char *)(*stacktopptr))
            bug("[aros_pop_altstack]: Error! Stack pointer below top of alternative stack!\n");
    )

    return *((*stacktopptr)--);
}
