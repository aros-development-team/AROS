/*
    Copyright 2001 AROS - The Amiga Research OS
    $Id$

    Desc: file descriptors handling internals
    Lang: english
*/

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <aros/symbolsets.h>
#include "__open.h"

void *__stdfiles[3];

int __numslots = 0;
fdesc **__fd_array = NULL;

fdesc *__getfdesc(register int fd)
{
    return ((__numslots>=fd) && (fd>=0))?__fd_array[fd]:NULL;
}

void __setfdesc(register int fd, fdesc *fdesc)
{
    __fd_array[fd] = fdesc;
}

int __getfirstfd(register int startfd)
{
    for (
	;
	startfd < __numslots && __fd_array[startfd];
	startfd++
    );

    return startfd;
}

int __getfdslot(int wanted_fd)
{
    if (wanted_fd>=__numslots)
    {
        void *tmp;

        tmp = malloc((wanted_fd+1)*sizeof(fdesc *));

	if (!tmp) return -1;

	if (__fd_array)
	{
	    CopyMem(__fd_array, tmp, __numslots*sizeof(fdesc *));
	    free(__fd_array);
     	}

	__fd_array = tmp;

	bzero(__fd_array + __numslots, (wanted_fd - __numslots + 1) * sizeof(fdesc *));
	__numslots = wanted_fd+1;
    }
    else if (wanted_fd<0)
        return -1;
    else if (__fd_array[wanted_fd])
    	close(wanted_fd);

    return wanted_fd;
}

#warning perhaps this has to be handled in a different way...
void __init_stdfiles(void)
{
    struct Process *me;
    fdesc *indesc=NULL, *outdesc=NULL, *errdesc=NULL;
    int res = __getfdslot(2);

    if
    (
        res == -1                          ||
	!(indesc  = malloc(sizeof(fdesc))) ||
	!(outdesc = malloc(sizeof(fdesc))) ||
	!(errdesc = malloc(sizeof(fdesc)))
    )
    {
    	SetIoErr(ERROR_NO_FREE_STORE);
    	exit(20);
    }


    me = (struct Process *)FindTask (NULL);
    indesc->fh  = __stdfiles[STDIN_FILENO]  = Input();
    outdesc->fh = __stdfiles[STDOUT_FILENO] = Output();
    errdesc->fh = __stdfiles[STDERR_FILENO] = me->pr_CES ? me->pr_CES : me->pr_COS;

    indesc->flags  = O_RDONLY;
    outdesc->flags = O_WRONLY | O_APPEND;
    errdesc->flags = O_WRONLY | O_APPEND;

    indesc->opencount = outdesc->opencount = errdesc->opencount = 1;

    __fd_array[STDIN_FILENO]  = indesc;
    __fd_array[STDOUT_FILENO] = outdesc;
    __fd_array[STDERR_FILENO] = errdesc;
}


void __exit_stdfiles(void)
{
    int i = __numslots;
    while (i)
    {
	close(--i);
    }
}

ADD2INIT(__init_stdfiles, 2);
ADD2EXIT(__exit_stdfiles, 2);
