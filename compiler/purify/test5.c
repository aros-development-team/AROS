/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>

int c;

int main (int argc, char ** argv)
{
    int a;

    a = c;
    c = a;
}
