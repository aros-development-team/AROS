/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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
    LONG error;
    struct Locale *new;

    if(argc != 2)
    {
	PrintFault(ERROR_REQUIRED_ARG_MISSING, "SetLocale");
	return RETURN_FAIL;
    }

    new = OpenLocale(av[1]);
    error = IoErr();
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
	PrintFault(error, "SetLocale");
	return RETURN_FAIL;
    }

    return 0;
}
