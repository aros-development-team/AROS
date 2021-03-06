/*
    Copyright (C) 1995-2014, The AROS Development Team. All rights reserved.
*/

#include <stdio.h>
#include <errno.h>
#include <ctype.h>

int main (int argc, char ** argv)
{
    int c, f;

    c = 'a';

    f = isalpha (c);
    f = tolower (c);
    f = toupper (c);

    c = errno;

    return 0;
}
