#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    	if (errno) perror("Internal Error");
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
    fatalerror(WEXITSTATUS(system(command)));
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
	   perror("Internal error");

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

    atexit(exitfunc);

    /* Get the output file name */
    output = "a.out";
    for (cnt = 1; argv[cnt]; cnt++)
    {
    	if (argv[cnt][0]=='-' && argv[cnt][1]=='o')
     	    output = argv[cnt][2]?&argv[cnt][2]:argv[++cnt];
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

    command = joinstrings(NMPATH " ", output, NULL);

    pipe     = xpopen(command);
    setsfile = xfopen(setsfilename, "w");

    ret = gensets(pipe, setsfile);
    fclose(setsfile);

    if (ret)
    {
	free(command);
	command = joinstrings(GCCPATH " -nostartfiles -nostdlib -Wl,-r -o ", tempoutname, " ", output, " ", setsfilename, NULL);
	xsystem(command);
	xsystem(joinstrings(MVPATH " -f ", tempoutname, " ", output, NULL));
    }

    return 0;
}


