/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: AROS linker wrapper that handles symbol sets
    Lang: english
    Original Author: falemagn
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <unistd.h>
#include "collect-aros.h"

extern int gensets(FILE *in, FILE *out);

void fatalerror(int status)
{
    if (status)
    {
    	if (errno) perror("collect-aros");
	exit(status);
    }
}

void *xmalloc(size_t size)
{
    void *ret = malloc(size);

    fatalerror(!ret);

    return ret;
}

FILE *xpopen(char *command)
{
    FILE *ret = popen(command, "r");

    fatalerror(!ret);

    return ret;
}

char *xtempnam(void)
{
    char *ret = tempnam("/tmp", NULL);

    fatalerror(!ret);

    return ret;
}

void xsystem(char *command)
{
    /*	For some bizarre reason, WEXITSTATUS() requires an lvalue
	when used on FreeBSD.  */
    int stat = system(command);
    fatalerror(WEXITSTATUS(stat));
}

FILE *xfopen(char *name, char *mode)
{
    FILE *ret = fopen(name, mode);

    fatalerror(!ret);

    return ret;
}

void docommand(char *path, char *argv[])
{
    extern char **environ; /*this is specially needed by collect2,
                            so that it can find 'ld' in the PATH */
    pid_t pid=vfork();
    int status;

    fatalerror(pid==-1);

    if (!pid)
    {
    	if (execve(path, argv, environ))
	{
	   fprintf(stderr, "collect-aros - Error while executing %s\n", path);
	   perror(NULL);
	}

	errno = 0; /* the parent process is going to exit too
	            and we don't want it to complain again about the error. */

	_exit(1); /* we can't use exit because it would close the I/O channels  of the parent process */
    }

    waitpid(pid, &status, 0);

    fatalerror(WEXITSTATUS(status));
}

char *joinstrings(char *first, ...)
{
    va_list strings;
    char *str, *s;
    int size = 0;

    if (!first)
       	return NULL;

    size += strlen(first);

    va_start (strings, first);

    while ((s=va_arg(strings, char *)))
    	size += strlen(s);

    va_end(strings);

    str = xmalloc(size+1);

    str[0]='\0';

    strcat(str, first);

    va_start(strings, first);

    while ((s=va_arg(strings, char *)))
    	strcat(str, s);

    va_end (strings);

    return str;
}

char *basename(char *path)
{
    char *base = path;

    for (; *path; path++)
        if (*path=='/') base = path+1;

    return base;
}

char *tempoutname = NULL;
char *setsfilename = NULL;

void exitfunc(void)
{
    remove(setsfilename);
    remove(tempoutname);
}

int main(int argc, char *argv[])
{
    int cnt, ret = 0;
    char *output;
    char *command;
    char **ldargs;
    FILE *pipe;
    FILE *setsfile = NULL;
    char buf[200];
    int thereare = 0, incremental = 0;

    atexit(exitfunc);

    /* Do some stuff with the arguments */
    output = "a.out";
    for (cnt = 1; argv[cnt]; cnt++)
    {
    	/* We've encountered an option */
	if (argv[cnt][0]=='-')
	{
            /* Get the output file name */
	    if (argv[cnt][1]=='o')
     	        output = argv[cnt][2]?&argv[cnt][2]:argv[++cnt];
            else
	    /* Incremental linking is required */
            if (argv[cnt][0]=='r')
	        incremental = 1;
	}
    }

    tempoutname  = xtempnam();
    setsfilename = joinstrings(tempoutname, "-set.c", NULL);

    /* disabled: for some strange reasons this doesn't always work...
    arguments = joinargs(argv);
    command = joinstrings("ld -r ", arguments, NULL);
    printf(">>>1<<< %s\n", command);


    if ((ret=WEXITSTATUS(system(command))))
    	ERROR(ret, );
    */

    ldargs = xmalloc(sizeof(char *) * (argc+2));

    ldargs[0] = basename(LINKERPATH);
    ldargs[1] = "-r";

    for (cnt = 1; cnt < argc; cnt++)
    	ldargs[cnt+1] = argv[cnt];

    ldargs[cnt+1] = NULL;

    docommand(LINKERPATH, ldargs);

    if (incremental)
        return 0;

    command = joinstrings(NMPATH " ", output, NULL);

    pipe     = xpopen(command);
    setsfile = xfopen(setsfilename, "w");

    ret = gensets(pipe, setsfile);
    fclose(setsfile);
    pclose(pipe);

    if (ret)
    {
	free(command);
	command = joinstrings(GCCPATH " -nostartfiles -nostdlib -Wl,-r -o ", tempoutname, " ", output, " ", setsfilename, NULL);
	xsystem(command);
	free(command);
	command = joinstrings(MVPATH " -f ", tempoutname, " ", output, NULL);
	xsystem(command);
    }

    free(command);
    command = joinstrings(NMPATH " -ul ", output, NULL);

    pipe = xpopen(command);

    while ((cnt = fread(buf, 1, sizeof(buf), pipe))!=0)
    {
	if (!thereare)
	{
	    thereare = 1;
	    fprintf(stderr, "There are undefined symbols in %s:\n", output);
        }

	fwrite(buf, cnt, 1, stderr);
    }
    fatalerror(ferror(pipe));

    if (thereare)
    	remove(output);

    return thereare;
}


