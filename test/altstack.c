/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$

    Test for altstack from arossupport
*/

#include <aros/altstack.h>
#include <proto/exec.h>
#include <assert.h>

int main(void)
{
    struct Task *t = FindTask(NULL);
    IPTR v;

    aros_set_altstack(t, 1);

    assert(aros_get_altstack(t) == (IPTR)1);

    aros_push_altstack(t, 2);

    assert(aros_get_altstack(t) == (IPTR)2);

    v = aros_pop_altstack(t);

    assert(v == (IPTR)2);

    assert(aros_get_altstack(t) == (IPTR)1);

    return 0;
}
