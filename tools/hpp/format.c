#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <toollib/error.h>
#include <toollib/vstring.h>
#include "db.h"
#include "var.h"

enum modes
{
    m_space = 0,
    m_prespace,
    m_punct,
    m_comment,
    m_cpp,
    m_identifier,
    m_string,
    m_header
};

static const char * ModeDelim[] =
{
    "",
    "",
    "cpunctuation",
    "ccomment",
    "cppkeyword",
    "cidentifier",
    "cstring",
    "cheader",
};

static void
emit_html_char (int c, FILE * out)
{
    switch (c)
    {
    case '&': fputs ("&amp;", out); break;
    case '<': fputs ("&lt;", out); break;
    case '>': fputs ("&gt;", out); break;
    default: fputc (c, out); break;
    }
}

static void
emit_html_string (const char * str, FILE * out)
{
    while (*str)
	emit_html_char (*str ++, out);
}

int
main (int argc, char ** argv)
{
    int    c;
    FILE * in;
    char * type;
    char * outformat;
    char * infilename;
    int    t;
    enum modes mode = m_prespace;
    String ident;
    char * data;

    Var_Init ();
    DB_Init ();
    DB_Add ("c-html", "c-html.db");

#define NEWMODE(nm)             \
    if ((nm) != mode)           \
    {				\
	if (ModeDelim[mode][0]) \
	    printf ("</%s>", ModeDelim[mode]);    \
	mode = (nm);            \
	if (ModeDelim[mode][0]) \
	    printf ("<%s>", ModeDelim[mode]);    \
    }

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

    if (strcasecmp (type, "c"))
    {
	Error ("Unknown type %s. Supported types: C", type);
	ErrorExit (10);
    }

    if (strcasecmp (outformat, "html"))
    {
	Error ("Unknown output format %s. Supported formats: HTML", outformat);
	ErrorExit (10);
    }

    ident = VS_New (NULL);

    while ((c = getc (in)) != EOF)
    {
	switch (c)
	{
	case '&':
	    NEWMODE(m_punct);
	    fputs ("&amp;", stdout);
	    break;
	case '<':
	    NEWMODE(m_punct);
	    fputs ("&lt;",  stdout);
	    break;
	case '>':
	    NEWMODE(m_punct);
	    fputs ("&gt;",  stdout);
	    break;
	case '.': case '!': case '^': case '%':
	case '(': case ')': case '=': case '?': case '{':
	case '[': case ']': case '}': case '+': case '*':
	case '~': case '-': case ':': case ',':
	case ';': case '|':
	    NEWMODE(m_punct);
	    putchar (c);
	    break;

	case ' ':
#if 0
	    if (mode != m_prespace)
	    {
		NEWMODE(m_space);
		putchar (c);
	    }
	    else
#endif
	    {
		fputs ("&nbsp;", stdout);
	    }
	    break;

	case '/': /* Comment or / */
	    c = getc (in);

	    if (c == '*')
	    {
		NEWMODE(m_comment);
		fputs ("/*",  stdout);

		while ((c = getc (in)) != EOF)
		{
rem_again:
		    if (c == '*')
		    {
			c = getc (in);

			if (c == '/')
			{
			    fputs ("*/",  stdout);
			    break;
			}
			else
			{
			    putchar ('/');
			    goto rem_again;
			}
		    }
		    else
			putchar (c);
		}
	    }
	    else
	    {
		NEWMODE(m_punct);
		putchar ('/');
		ungetc (c, in);
	    }
	    break;

	case '\n':
	    NEWMODE(m_space);
	    fputs ("<BR>\n", stdout);
	    NEWMODE(m_prespace);
	    break;

	case '"':
	    NEWMODE(m_string);
	    putchar (c);

	    while ((c = getc (in)) != EOF)
	    {
		putchar (c);

		if (c == '\\')
		{
		    c = getc (in);
		    if (c == '\n')
			fputs ("<BR>", stdout);
		    putchar (c);
		}
		else if (c == '"')
		{
		    break;
		}
	    }

	    break;

	case '#':

	    VS_Clear (ident);
	    VS_AppendChar (ident, c);

	    while ((c = getc (in)) != EOF)
	    {
		if (!isspace (c))
		    break;
	    }

	    VS_AppendChar (ident, c);

	    while ((c = getc (in)) != EOF)
	    {
		if (!isalpha (c))
		{
		    ungetc (c, in);
		    break;
		}

		VS_AppendChar (ident, c);
	    }

	    data = DB_FindData ("c-html", ident->buffer);

	    if (data)
	    {
		NEWMODE(m_space);
		fputs (data, stdout);
	    }
	    else
	    {
		NEWMODE(m_cpp);
		fputs (ident->buffer, stdout);
	    }

	    if (!strcmp (ident->buffer, "#include"))
	    {
		putchar (' ');
		VS_Clear (ident);

		while ((c = getc (in)) != EOF)
		{
		    if (!isspace (c))
			break;
		}

		VS_AppendChar (ident, c);

		while ((c = getc (in)) != EOF)
		{
		    if (isspace (c))
		    {
			ungetc (c, in);
			break;
		    }

		    VS_AppendChar (ident, c);
		}

		data = DB_FindData ("c-html", ident->buffer);

		if (data)
		{
		    NEWMODE(m_space);
		    fputs (data, stdout);
		}
		else
		{
		    NEWMODE(m_header);
		    emit_html_string (ident->buffer, stdout);
		}
	    }

	    break;

	default:
	    if (isalpha (c) || c == '_')
	    {
		VS_Clear (ident);
		VS_AppendChar (ident, c);

		while ((c = getc (in)) != EOF)
		{
		    if (!isalnum (c) && c != '_')
		    {
			ungetc (c, in);
			break;
		    }

		    VS_AppendChar (ident, c);
		}

		data = DB_FindData ("c-html", ident->buffer);

		if (data)
		{
		    NEWMODE(m_space);
		    fputs (data, stdout);
		}
		else
		{
		    NEWMODE(m_identifier);
		    fputs (ident->buffer, stdout);
		}
	    }
	    else
		putchar (c);

	    break;

	} /* switch (c) */
    } /* while !EOF */

    if (ModeDelim[mode][0])
	printf ("</%s>\n", ModeDelim[mode]);

    if (infilename)
	fclose (in);

    DB_Exit ();
    Var_Exit ();

    ErrorExit (0);
}

