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
#include <sys/stat.h>
#include <unistd.h>

#include "ldscript.h"

#define TMPTEMPLATE "aros-collect.XXXXXX"

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
    char *ret = strdup(TMPTEMPLATE);
    int rc = mkstemp(ret); //tempnam("/tmp", NULL);
    fatalerror(!rc);

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
    extern char **environ;

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

char *tempoutname  = NULL;
char *ldscriptname = NULL;

void exitfunc(void)
{
    remove(ldscriptname);
    remove(tempoutname);
}

int main(int argc, char *argv[])
{
    int cnt, ret = 0;
    char *output;
    char *command;
    char **ldargs;
    FILE *pipe;
    FILE *ldscriptfile = NULL;
    char buf[200];
    int thereare  = 0, incremental            = 0,
        strip_all = 0, ignore_missing_symbols = 0;

    char *LINKERPATH  = getenv("AROS_TARGET_LINKERPATH");
    char *OBJDUMPPATH = getenv("AROS_TARGET_OBJDUMPPATH");
    char *NMPATH      = getenv("AROS_TARGET_NMPATH");
    char *STRIPPATH   = getenv("AROS_TARGET_STRIPPATH");


    if (!LINKERPATH)
    {
        fprintf(stderr, "%s: AROS_TARGET_LINKERPATH variable not set, using default value /usr/bin/ld\n", argv[0]);
        LINKERPATH = "/usr/bin/ld";
    }

    if (!OBJDUMPPATH)
    {
        fprintf(stderr, "%s: AROS_TARGET_OBJDUMPPATH variable not set, using default value /usr/bin/objdump\n", argv[0]);
        OBJDUMPPATH = "/usr/bin/objdump";
    }

    if (!NMPATH)
    {
        fprintf(stderr, "%s: AROS_TARGET_NMPATH variable not set, using default value /usr/bin/nm\n", argv[0]);
        NMPATH = "/usr/bin/nm";
    }

    if (!STRIPPATH)
    {
        fprintf(stderr, "%s: AROS_TARGET_STRIPPATH variable not set, using default value /usr/bin/strip\n", argv[0]);
        NMPATH = "/usr/bin/strip";
    }

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
	    /* Incremental linking is requested */
            if (argv[cnt][1]=='r' && argv[cnt][2]=='\0')
	        incremental = 1;
	    else
	    /* Ignoring of missing symbols is requested */
	    if (argv[cnt][1]=='i')
	        ignore_missing_symbols = 1;
	    else
	    /* Complete stripping is requested, but we do it our own way */
	    if (argv[cnt][1]=='s' && argv[cnt][2]=='\0')
	    {
                strip_all = 1;
		argv[cnt][1] = 'r'; /* Just some non-harming option... */
	    }
	    else
	    /* The user just requested help info, don't do anything else */
	    if (strncmp(&argv[cnt][1], "-help", 6) == 0)
	    {
	        /* I know, it's not incremental linking we're after, but the end result
		   is the same */
	        incremental = 1;
	        break;
	    }

	}
    }

    ldargs = xmalloc(sizeof(char *) * (argc+2));

    ldargs[0] = basename(LINKERPATH);
    ldargs[1] = "-r";

    for (cnt = 1; cnt < argc; cnt++)
    	ldargs[cnt+1] = argv[cnt];

    ldargs[cnt+1] = NULL;

    docommand(LINKERPATH, ldargs);

    if (incremental)
        return 0;

    tempoutname  = xtempnam();
    ldscriptname = joinstrings(tempoutname, "-ldscript.x", NULL);
    command      = joinstrings(OBJDUMPPATH, " -h ", output, NULL);
    pipe         = xpopen(command);
    ldscriptfile = xfopen(ldscriptname, "w");

    fprintf(ldscriptfile, LDSCRIPT_PART1);
    ret = gensets(pipe, ldscriptfile);
    fprintf(ldscriptfile, LDSCRIPT_PART2);

    fclose(ldscriptfile);
    pclose(pipe);

    if (ret)
    {
        free(command);
        command = joinstrings(LINKERPATH, " -r -o ", tempoutname, " ", output, " -T ", ldscriptname, NULL);
	xsystem(command);
	free(command);
	command = joinstrings(MVPATH, " -f ", tempoutname, " ", output, NULL);
	xsystem(command);
    }

    free(command);
    free(tempoutname);

    if (ignore_missing_symbols)
    {
        thereare = 0;
	goto end;
    }

    command = joinstrings(NMPATH, " -C -ul ", output, NULL);

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

    pclose(pipe);

end:
    if (thereare)
        remove(output);
    else
	chmod(output, 0766);

    if (strip_all)
    {
        free(command);
        command = joinstrings(STRIPPATH, " --strip-unneeded ", output, NULL);
	xsystem(command);
    }

    return thereare;
}


