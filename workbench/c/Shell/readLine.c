/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <dos/stdio.h>

#include <proto/dos.h>

#include "Shell.h"

#include <aros/debug.h>

#define PIPE_NAME "PIPE "

static BOOL checkPipe(STRPTR pchar, STRPTR mchar, STRPTR in, LONG inlen)
{
   LONG c, n;
   BOOL quoted = FALSE;
   int mcharn = strlen(mchar), pcharn = strlen(pchar);
   
   for (n = 0; n < inlen; n++)
   {
        c = in[n];
        if (c == '"')
        {
            quoted = !quoted;
        }

        if (quoted)
            continue;

        if (mcharn > 0 && c == mchar[0] && memcmp(&in[n], mchar, mcharn) == 0)
            return TRUE;

        if (pcharn > 0 && c == pchar[0] && memcmp(&in[n], pchar, pcharn) == 0)
            return TRUE;
    }
    return FALSE;
}

LONG readLine(ShellState *ss, struct CommandLineInterface *cli, Buffer *out, WORD *moreLeft)
{
    BPTR fh = cli->cli_CurrentInput;
    STRPTR buf = out->buf; /* pre-allocated by caller */
    BOOL comment = FALSE;
    BOOL quoted = FALSE;
    LONG c, i, j, len;
    TEXT pchar[3], mchar[3];

    len = GetVar("_pchar", pchar, sizeof pchar, GVF_LOCAL_ONLY | LV_VAR);
    if (len <= 0)
        pchar[0] = 0;
    pchar[2] = 0;

    len = GetVar("_mchar", mchar, sizeof mchar, GVF_LOCAL_ONLY | LV_VAR);
    if (len <= 0)
        mchar[0] = 0;
    mchar[2] = 0;
    ss->pchar0 = pchar[0];
    ss->mchar0 = mchar[0];

    for (i = 0, j = 0; i < LINE_MAX; ++i)
    {
	c = FGetC(fh);

	if (c == ENDSTREAMCH)
	    break;

        if (c == '"')
             quoted = !quoted;
 
        if (!quoted && c == ';' && c != mchar[0] ) /* comment line */
	{
	    comment = TRUE;
	    continue;
	}
	else if (c == '\n') /* end of line */
	{
	    comment = FALSE;

	    /* '+' continuation */
	    if (j > 0 && buf[j-1]=='+') {
	        buf[j-1]=c;
	        continue;
            }
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
    bufferAppend(buf, j, out, SysBase);

    if (checkPipe(pchar, mchar, buf, j))
        bufferInsert(PIPE_NAME, strlen(PIPE_NAME), out, SysBase);

    *moreLeft = (c != ENDSTREAMCH);

    D(bug("[Shell] readLine %d%s: %s", j, *moreLeft ?  "" : "'", buf));
    return 0;
}

