#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "emit.h"
#include "util.h"

char * filename = NULL;
char * progname;
enum emitmode emode = em_html;

extern int yyparse (void * fh);
extern int yydebug;

void PrintUsage (void)
{
    fprintf (stderr, "Usage: %s [-dh] [--debug] [--html] [--help] \n"
		"\t<filename>\n", progname);
}

void close_out (void)
{
    fclose (out);
}

int main (int argc, char ** argv)
{
    FILE * fh;
    emode = em_html;

    progname = argv[0];

    argc --;
    argv ++;

    while (argc)
    {
	if (**argv == '-')
	{
	    if (!strcmp (*argv, "-d") || !strcmp (*argv, "--debug"))
		yydebug = 1;
	    else if (!strcmp (*argv, "-o") || !strncmp (*argv, "--output", 8))
	    {
		char * ptr;

		if (argv[0][1] == 'o')
		{
		    if (argv[0][2])
			ptr = &argv[0][2];
		    else
		    {
			argv ++;
			argc --;

			if (!argc)
			{
			    fprintf (stderr, "%s: Missing argument to -o\n", progname);
			    exit (10);
			}

			ptr = *argv;
		    }
		}
		else
		{
		    if (argv[0][8])
		    {
			ptr = &argv[0][8];
			if (*ptr == '=')
			    ptr ++;
		    }
		    else
		    {
			argv ++;
			argc --;

			if (!argc)
			{
			    fprintf (stderr, "%s: Missing argument to --output\n", progname);
			    exit (10);
			}

			ptr = *argv;
		    }
		}

		if (strcmp (ptr, "-"))
		{
		    outname = xstrdup (ptr);

		    out = fopen (outname, "w");

		    if (!out)
		    {
			fprintf (stderr, "%s: Can't open \"%s\" for output\n", progname, outname);
			exit (10);
		    }
		}
	    }
	    else if (!strcmp (*argv, "-h") || !strcmp (*argv, "--html"))
		emode = em_html;
	    else if (!strcmp (*argv, "--help"))
	    {
		PrintUsage ();
		return 0;
	    }
	    else
	    {
		fprintf (stderr, "%s: Unknown option: %s (ignored)\n",
		    progname,
		    *argv
		);
	    }
	}
	else if (!filename)
	{
	    filename = *argv;

	    if (*filename == '-' && !filename[1])
	    {
		filename = "<stdin>";
		fh = stdin;
	    }
	    else
	    {
		fh = fopen (filename, "r");

		if (!fh)
		{
		    fprintf (stderr, "%s: Can't open %s: %s\n",
			progname, filename, strerror(errno));
		    return 10;
		}
	    }
	}
	else
	{
	    fprintf (stderr, "%s: Too man parameters: %s (ignored)\n",
		progname,
		*argv
	    );
	}

	argc --;
	argv ++;
    }

    if (!filename)
    {
	PrintUsage ();
	return 10;
    }

    if (!out)
    {
	outname = "<stdout>";
	out = stdout;
    }
    else
	atexit (close_out);

    emit_init ();

    yyparse (fh);

    emit_exit ();

    fclose (fh);

    return 0;
}
