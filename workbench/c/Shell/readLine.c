/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

/* TODO + line continuations */

#include <dos/stdio.h>

#include <proto/dos.h>

#include "Shell.h"

#include <aros/debug.h>

BOOL readLine(struct CommandLineInterface *cli, Buffer *out, BOOL *moreLeft, APTR DOSBase)
{
    BPTR fh = cli->cli_CurrentInput;
    STRPTR buf = out->buf; /* pre-allocated by caller */
    BOOL comment = FALSE;
    LONG c, i, j;

    for (i = 0, j = 0; i < LINE_MAX; ++i)
    {
	c = FGetC(fh);

	if (c == ENDSTREAMCH)
	    break;

	if (j == 0)
	{
	    if (c == ';') /* comment line */
	    {
		comment = TRUE;
		continue;
	    }
	    else if (c == '\n') /* empty line */
	    {
		comment = FALSE;
		if (isInteractive(cli) == FALSE)
		    continue;
	    }
	    else if (comment || c == ' ' || c == '\t') /* leading spaces */
		continue;
	}

	if (c == '\n') /* end of line */
	    break;

	buf[j++] = c;
    }

    if (i >= LINE_MAX)
	return ERROR_LINE_TOO_LONG;

    buf[j] = '\0';
    bufferAppend(buf, j, out);
    *moreLeft = (c != ENDSTREAMCH);

    D(bug("[Shell] readLine %d%s: %s\n", j, *moreLeft ?  "" : "'", buf));
    return 0;
}

