/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: SetLocale CLI command
    Lang: English              
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/locale.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/locale.h>

int main(int argc, char **av)
{
    struct Locale *new;

    if(argc != 2)
    {
	PrintFault(ERROR_REQUIRED_ARG_MISSING, "SetLocale");
	return 20;
    }

    new = OpenLocale(av[1]);
    FPuts(Output(), "Locale opened\n");
    if(new)
    {
	struct Locale *old = NULL;
	old = LocalePrefsUpdate(new);
	FPuts(Output(), "Locale set\n");
	CloseLocale(old);
	CloseLocale(new);
    }
    else
    {
	PrintFault(IoErr(), "SetLocale");
	return 20;
    }

    return 0;
}
