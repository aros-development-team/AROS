/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: stdio internals
    Lang: english
*/
#include <stdio.h>
#include <stdlib.h>
#include <exec/lists.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/symbolsets.h>
#include "__stdio.h"

FILE * stdin;
FILE * stdout;
FILE * stderr;

struct MinList __stdio_files =
{
    (struct MinNode *)&__stdio_files.mlh_Tail,
    NULL,
    (struct MinNode *)&__stdio_files
};

int __stdio_fd = 3;

static FILENODE *new_file_node(BPTR fh, long flags, int fd)
{
	FILENODE *fn;

	if (!(fn = malloc (sizeof (FILENODE))) )
    {
		SetIoErr(ERROR_NO_FREE_STORE);
		exit(RETURN_FAIL);
    }

    fn->File.fh = (void *)fh;
    fn->File.flags = flags;
    fn->fd = fd;

	return fn;
}

void __init_stdio(void)
{
	struct Process *me;
	FILENODE *fn;

	fn = new_file_node(Input(), 0, 0);
	AddTail ((struct List *)&__stdio_files, (struct Node *)fn);
	stdin = FILENODE2FILE(fn);

	fn = new_file_node(Output(), 0, 1);
    AddTail ((struct List *)&__stdio_files, (struct Node *)fn);
	stdout = FILENODE2FILE(fn);

	me = (struct Process *)FindTask (NULL);
	fn = new_file_node(me->pr_CES ? me->pr_CES : me->pr_COS, 0, 2);
    AddTail ((struct List *)&__stdio_files, (struct Node *)fn);
	stderr = FILENODE2FILE(fn);
}

ADD2INIT(__init_stdio, 5);

