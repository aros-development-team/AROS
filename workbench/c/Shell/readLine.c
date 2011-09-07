/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

/* TODO + line continuations */

#include <dos/stdio.h>

#include <proto/dos.h>

#include "Shell.h"

#include <aros/debug.h>

LONG readLine(struct CommandLineInterface *cli, Buffer *out, BOOL *moreLeft, APTR DOSBase)
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

	if (c == ';') /* comment line */
	{
	    comment = TRUE;
	    continue;
	}
	else if (c == '\n') /* end of line */
	{
	    comment = FALSE;
	}
	else if (comment)
	    continue;

	buf[j++] = c;

	if (c == '\n') /* end of line */
	    break;
    }

    if (i >= LINE_MAX) {
        D(bug("[Shell] ERROR_LINE_TOO_LONG\n"));
	return ERROR_LINE_TOO_LONG;
    }

    buf[j] = '\0';
    bufferAppend(buf, j, out);
    *moreLeft = (c != ENDSTREAMCH);

    D(bug("[Shell] readLine %d%s: %s", j, *moreLeft ?  "" : "'", buf));
    return 0;
}

