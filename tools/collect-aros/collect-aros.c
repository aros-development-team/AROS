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

static char *old_path;
static char *compiler_path;

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

pid_t xvfork(void)
{
    pid_t pid = vfork();

    fatalerror(pid == -1);

    return pid;
}

void xwaitpid(pid_t pid)
{
    int status;

    waitpid(pid, &status, 0);

    fatalerror(WEXITSTATUS(status));
}
#define docommand(func, cmd, ...)                                             \
do                                                                            \
{                                                                             \
    pid_t pid = xvfork();                                                     \
    if (pid == 0)                                                             \
    {                                                                         \
	func(cmd, __VA_ARGS__);                                               \
	                                                                      \
        fprintf(stderr, "collect-aros - Error while executing %s\n", cmd);    \
                                                                              \
	/* we can't use exit because it would close the I/O channels  of the  \
	   parent process.  */                                                \
	_exit(1);                                                             \
    }                                                                         \
                                                                              \
    xwaitpid(pid);                                                            \
} while (0)


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

    extern char **environ;

    char *compiler_path = getenv("COMPILER_PATH");
    char *path          = getenv("PATH");
    char *new_path;

    atexit(exitfunc);

    if (!compiler_path) compiler_path = "";
    if (!path) path = "";

    new_path = malloc(5 + strlen(compiler_path) + 1 + strlen(path) + 1);
    if (new_path)
    {
        strcat(new_path, "PATH=");
        strcat(new_path, compiler_path);
        strcat(new_path, ":");
        strcat(new_path, path);

	putenv(new_path);
    }

    for (cnt = 0; environ[cnt] != NULL; cnt++)
        printf("[%03d] %s\n", cnt, environ[cnt]);

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
            if ((argv[cnt][1]=='r' || argv[cnt][1]=='i') && argv[cnt][2]=='\0')
	        incremental = 1;
	    else
	    /* Ignoring of missing symbols is requested */
	    if (strncmp(&argv[cnt][1], "ius", 4) == 0)
	    {
	        ignore_missing_symbols = 1;
		argv[cnt][1] = 'r';  /* Just some non-harming option... */
		argv[cnt][2] = '\0';
	    }
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

    ldargs[0] = "ld";
    ldargs[1] = "-r";

    for (cnt = 1; cnt < argc; cnt++)
    	ldargs[cnt+1] = argv[cnt];

    ldargs[cnt+1] = NULL;

    docommand(execvp, "ld", ldargs);

    if (incremental)
        return 0;

    tempoutname  = xtempnam();
    ldscriptname = joinstrings(tempoutname, "-ldscript.x", NULL);
    command      = joinstrings("objdump -h ", output, NULL);
    pipe         = xpopen(command);
    ldscriptfile = xfopen(ldscriptname, "w");

    fprintf(ldscriptfile, LDSCRIPT_PART1);
    ret = gensets(pipe, ldscriptfile);
    fprintf(ldscriptfile, LDSCRIPT_PART2);

    fclose(ldscriptfile);
    pclose(pipe);

    if (ret)
    {
	docommand(execlp, "ld", "ld", "-r", "-o", tempoutname, output, "-T", ldscriptname, NULL);
	docommand(execlp, "mv", "mv", "-f", tempoutname, output, NULL);
    }

    free(tempoutname);

    if (ignore_missing_symbols)
    {
        thereare = 0;
	goto end;
    }

    command = joinstrings("nm -C -ul ", output, NULL);

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

    if (!thereare && strip_all)
    {
        docommand(execlp, "strip", "strip", "--strip-unneeded", output, NULL);
    }

    return thereare;
}


