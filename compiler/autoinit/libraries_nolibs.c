/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: autoinit library - automatic library opening/closing handling
          Dummy functions to be used in case no libraries have to be opened/closed
    Lang: english
*/


int set_open_libraries(void)   __attribute__ ((weak));
void set_close_libraries(void) __attribute__ ((weak));

int set_open_libraries(void)
{
    return 0;
}

void set_close_libraries(void)
{
}
