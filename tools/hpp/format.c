#include <stdio.h>
#include <toollib/error.h>

int
main (int argc, char ** argv)
{
    int    c;
    FILE * in;
    char * type;
    char * outformat;
    char * infilename;
    int    t;

    type = outformat = infilename = NULL;

    for (t=1; t<argc; t++)
    {
	if (!strcmp (argv[t], "-type"))
	    type = argv[++t];
	else if (!strcmp (argv[t], "-outformat"))
	    outformat = argv[++t];
	else
	    infilename = argv[t];
    }

    if (!type)
    {
	Error ("Missing argument -type");
	ErrorExit (10);
    }

    if (!outformat)
    {
	Error ("Missing argument -outformat");
	ErrorExit (10);
    }

    if (infilename)
    {
	in = fopen (infilename, "r");

	if (!in)
	{
	    StdError ("Opening %s for reading", infilename);
	    ErrorExit (10);
	}
    }
    else
	in = stdin;

    while ((c = getc (in)) != EOF)
    {
	switch (c)
	{
	case '&': fputs ("&amp;", stdout); break;
	case '<': fputs ("&lt;",  stdout); break;
	case '>': fputs ("&gt;",  stdout); break;
	default: putchar (c);
	}
    }

    if (infilename)
	fclose (in);

    ErrorExit (0);
}

