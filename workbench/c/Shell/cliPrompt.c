/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>

#include <string.h>

#include "Shell.h"

void cliPrompt(ShellState *ss)
{
    struct CommandLineInterface *cli = Cli();
    BSTR prompt = cli->cli_Prompt;
    LONG length = AROS_BSTR_strlen(prompt);
    BPTR output = Output();
    ULONG i;

    if (cli->cli_Background)
	return;

    for (i = 0; i < length; i++)
    {
	if (AROS_BSTR_getchar(prompt, i) == '%')
	{
	    if (++i == length)
		break;

	    switch (AROS_BSTR_getchar(prompt, i))
	    {
	    case 'N': case 'n':
		Printf("%ld", ss->cliNumber);
		break;
	    case 'R': case 'r':
		Printf("%ld", cli->cli_ReturnCode);
		break;
	    case 'S': case 's':
		FPuts(output, AROS_BSTR_ADDR(cli->cli_SetName));
		break;
	    default:
		FPutC(output, '%');
		FPutC(output, AROS_BSTR_getchar(prompt, i));
		break;
	    }
	}
	else
	    FPutC(output, AROS_BSTR_getchar(prompt, i));
    }

    Flush(output);
}

