/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
          Dummy functions to be used in case no libraries have to be opened/closed
    Lang: english
*/

#include <aros/symbolsets.h>

DEFINESET(LIBS)

int set_open_libraries_list(const void *list[])   __attribute__ ((weak));
void set_close_libraries_list(const void *list[]) __attribute__ ((weak));

int set_open_libraries_list(const void *list[])
{
    return 1;
}

void set_close_libraries_list(const void *list[])
{
}
