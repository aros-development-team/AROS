/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <assert.h>

#include "aros_types.h"

int main (int argc, char ** argv)
{
    unsigned char *u;
    char   *c = "foo";
    void   *v = &c;
    APTR    a;
    be_ptr<char> s;

    u = c;
    s = c;
    a = v;

    long *l = (long *) c;
    be_ptr<long> bl = l; // calls constructor
    bl = l;              // calls assignment operator
    bl = c;

    assert (s == c);
    assert (strcmp (s, c) == 0);
}
