/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>

#include <stdio.h>

#include "buffer.h"
#include "Shell.h"

#include <aros/debug.h>

// TODO:  remove these comments when fixed and validated
// TODO:  C++ style comments should be avoided
// FIXME: why use a $ in a filename ? just to try the resistance of bad FS
// TODO:  use pipes instead of plain file ?

#define SHELL_EMBED "T:Shell$embed"

/* BackTicks handling... like V45, we allow several embedded
 * commands, while V40 only allows one per line.
 */
LONG convertBackTicks(ShellState *ss, Buffer *in, Buffer *out, BOOL *quoted)
{
    Buffer embedIn = {0}, embedOut = {0}; /* TODO pre-alloc */
    LONG c = 0, error = 0, n = in->len, p = 0;
    TEXT buf[512] = SHELL_EMBED;
    ShellState ess = {0};

    ess.ss_DOSBase = DOSBase;
    ess.ss_SysBase = SysBase;

    for (++in->cur; in->cur < n; p = c)
    {
	c = in->buf[in->cur];

	if (p == '*')
	{
	    c = 0;
	    bufferCopy(in, &embedIn, 1, SysBase);
	}
	else if (c == '`')
	    break;
	else
	    bufferCopy(in, &embedIn, 1, SysBase);
    }

    if (c != '`')
    {
	bufferCopy(&embedIn, out, embedIn.len, SysBase);
	goto freebufs;
    }

    ++in->cur; /* last backtick */

    if (embedIn.len <= 0) /* `` empty command, no output */
	goto freebufs;

    initDefaultInterpreterState(&ess);

    /* The Amiga shell has severe problems when using
     * redirections in embedded commands so here, the
     * semantics differ somewhat. Unix shells seems to be
     * a little bit sloppy with this, too.
     * If you really wanted to, you could track down
     * uses of > and >> and make them work inside ` `, too,
     * but this seems to be rather much work for little gain.
     */
    if ((error = Redirection_init(&ess)))
	goto cleanup;

    /* Construct temporary output filename */
    l2a(ss->cliNumber, buf + sizeof(SHELL_EMBED) - 1);

    if (!(ess.newOut = Open(buf, MODE_NEWFILE)))
    {
	error = IoErr();
	goto cleanup;
    }

    ess.oldOut = SelectOutput(ess.newOut);
    
    /* Embedded command isn't echo'ed, but its result will be integrated
       in final command line, which will be echo'ed if the var is set. */
    D(bug("[Shell] embedded command: %s\n", embedIn.buf));
    if ((error = checkLine(&ess, &embedIn, &embedOut, FALSE)) == 0)
    {
	LONG i, len = -1, size;

	/* copy result to output */
	if ((size = Seek(ess.newOut, 0, OFFSET_BEGINNING)) >= 0)
	{
	    while ((len = Read(ess.newOut, buf, 512)) > 0)
	    {
		for (i = 0; i < len - 1; ++i) /* replace all \n but last */
		    if (buf[i] == '\n')
			buf[i] = ' ';

		size -= len;

		if (size <= 0 && buf[i] == '\n')
		    --len;

		bufferAppend(buf, len, out, SysBase);
	    }
	}

	if (len == -1)
	    error = IoErr();
    }

cleanup:
    Redirection_release(&ess);
    /* TODO: delete generated file */
freebufs:
    bufferFree(&embedIn, SysBase);
    bufferFree(&embedOut, SysBase);

    D(bug("[Shell] embedded command done, error = %d\n", error));
    return error;
}
