#include "exec.h"
#include "error.h"
#include "util.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern FILE * out;

static char * split (char * _str)
{
    static char * str;
    char * arg;

    if (_str)
	str = _str;

    while (isspace (*str))
	str ++;

    if (!*str)
	return NULL;

    arg = str;

    if (*str == '"')
    {
	while (*str && *str != '"')
	{
	    if (*str == '\\')
	    {
		str ++;
		if (*str)
		    str ++;
	    }
	    else
		str ++;
	}

	if (*str)
	    *str ++ = 0;
    }
    else
    {
	while (!isspace (*str) && *str)
	    str ++;

	if (*str)
	    *str ++ = 0;
    }

    return arg;
}

void runcommand (const char * command)
{
    char * buffer = xstrdup (command);
    char * argv[64];
    int argc=0;
    int fdout;

    for (argv[argc++]=split(command); (argv[argc++]=split(NULL)); );

    fflush (stdin);
    fflush (stdout);
    fflush (stderr);
    fflush (out);

    fdout = fileno (out);

    switch (fork ())
    {
    case 0: /* Child */
	out = fdopen (dup (fdout), "w");
	dup2 (fdout, 1);

	execvp (argv[0], argv);
	yyerror ("Can't exec \"%s\": %s\n", command, strerror(errno));
	exit (127);

    case -1: /* error */
	yyerror ("Can't fork: %s\n", strerror(errno));
	return;

    default: /* parent */
	wait (&argc);
	break;
    }

    free (buffer);
}
