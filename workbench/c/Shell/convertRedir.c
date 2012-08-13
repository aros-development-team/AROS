/*
    Copyright (C) 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
 */

#include <proto/dos.h>

#include "buffer.h"
#include "Shell.h"

LONG convertRedir(ShellState *ss, Buffer *in, Buffer *out)
{
    STRPTR s = in->buf + in->cur;
    BOOL newIn = FALSE, newOut = FALSE;
    BOOL append = FALSE;
    TEXT file[FILE_MAX];

    if (*s == '<')
    {
	if (ss->newIn) /* multiple < not allowed */
	    return ERROR_TOO_MANY_LEVELS;

	newIn = TRUE; /* new input */
	++s;
    }

    if (*s == '>')
    {
	if (ss->newOut) /* multiple > not allowed */
	    return ERROR_TOO_MANY_LEVELS;

	if (*++s == '>')
	{
	    append = TRUE;
	    ++s;
	}

	newOut = TRUE; /* new output */
    }

    in->cur = s - in->buf;

    switch (bufferReadItem(file, FILE_MAX, in, DOSBase))
    {
    case ITEM_QUOTED:
    case ITEM_UNQUOTED:
	break;
    default:
	return ERROR_LINE_TOO_LONG;
    }

    if (newIn)
    {
	if (!(ss->newIn = Open(file, MODE_OLDFILE)))
	    return IoErr();

	ss->oldIn = SelectInput(ss->newIn);
    }

    if (newOut)
    {
	LONG mode = append ? MODE_READWRITE : MODE_NEWFILE;

	if (!(ss->newOut = Open(file, mode)))
	    return IoErr();

	if (append && Seek(ss->newOut, 0, OFFSET_END) == -1)
	    return IoErr();

	ss->oldOut = SelectOutput(ss->newOut);
    }

    return 0;
}
