/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Init and exit set entries of a link library member that only the final
    link adds to the program.

    libsetmember comes before libsetuser on the link line, so the first,
    relocatable link stage does not take its member: nothing refers to it
    yet. collect-aros gives the libraries to the linker again and the member
    gets in then. Its set entries have to be called like any others.

    Expected output:

        init set function called: yes
        exit set function called
*/

#include <proto/dos.h>
#include <dos/dos.h>

int setuser_value(void);

int main(void)
{
    int called = (setuser_value() == 1);

    Printf("init set function called: %s\n", (IPTR)(called ? "yes" : "NO"));

    return called ? RETURN_OK : RETURN_FAIL;
}
