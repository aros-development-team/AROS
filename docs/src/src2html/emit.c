#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include "gram.h"
#include "emit.h"
#include "emit_intern.h"
#include "hash.h"
#include "error.h"
#include "exec.h"
#include "util.h"

#define LISTSTACKDEPTH	32
enum listmode liststack[LISTSTACKDEPTH];
enum listmode lmode = lm_none;
int listsp = 0;
int lasttoken = -1;
int pendingspace = 0;
int emitcount = 0;
int appendix = 0;
FILE * out;
char * outname;

int chapter = 0, section = 0, subsection = 0;
int isnewtext;
static int actualidate;

enum listmode convlistmode (const char * name);

enum lastemittype lastemit = let_par;

static const char * listmodename[] =
{
    "none",
    "description",
    "itemize",
    "enumeration",
    "emph",
    "new",
    "methods",
    "taglist",
    "indent",
    NULL
};

Hash * lrefs;

void emit_init (void)
{
    FILE * fh;
    char key[64], data[256];
    struct tm tm;
    time_t tt;

    lrefs = createhash ();

    fh = fopen (".lrefs", "r");

    if (fh)
    {
	for (;;)
	{
	    if (!fgets (key, sizeof(key), fh))
		break;

	    if (!fgets (data, sizeof(data), fh))
	    {
		fprintf (stderr, "%s: Missing data for lref \"%s\" in .lrefs\n",
		    progname, key);
		exit (10);
	    }

	    key[strlen(key)-1] = 0;
	    data[strlen(data)-1] = 0;

	    deflref (key, data);
	}

	fclose (fh);
    }

    switch (emode)
    {
    case em_html:
	emit_html_init ();
	break;

    default:
	fprintf (stderr, "emit_init: Illegal emit-mode %d\n", emode);
	exit (10);
    }

    time (&tt);
    tm = *localtime (&tt);

    actualidate = tm.tm_mday + tm.tm_mon*31 + (tm.tm_year+1900)*31*12;
}

void writelrefs (char * key, char * str, FILE * fh)
{
#if 0
    printf ("key=\"%s\", str=\"%s\"\n", key, str);
#endif
    fputs (key, fh);
    putc ('\n', fh);
    fputs (str, fh);
    putc ('\n', fh);
}

void emit_exit (void)
{
    FILE * fh;

    switch (emode)
    {
    case em_html:
	emit_html_exit ();
	break;

    default:
	fprintf (stderr, "emit_exit: Illegal emit-mode %d\n", emode);
	exit (10);
    }

    fh = fopen (".lrefs", "w");

    if (!fh)
    {
	fprintf (stderr, "%s: Cannot open .lrefs for writing: %s\n",
	    progname, strerror (errno));
	exit (10);
    }

    traversehash (lrefs, (TraverseProc)writelrefs, fh);

    fclose (fh);
}

int getidate (const char * str)
{
    int day, month, year, n;

    n = sscanf (str, "%d.%d.%d", &day, &month, &year);
    if (n != 3)
    {
	printf ("illegal date '%s'\n", str);
	exit (10);
    }

    if (year < 100)
    {
	if (year < 90)
	    year += 2000;
	else
	    year += 1900;
    }

    return day + month*31 + year*31*12;
}

void emit (int token, ...)
{
    va_list args;

    va_start (args, token);

    switch (token)
    {
    case FILEINFO:
	{
	    char * filename = expandpath (va_arg (args, char *));
	    int rc;
	    struct stat st;

	    rc = stat (filename, &st);

	    if (rc < 0)
	    {
		fprintf (stderr, "Error examining file %s: %s\n",
		    filename, strerror (errno));
		fprintf (out, "***, ***");
	    }
	    else
	    {
		long size = st.st_size;
		struct tm tm;
		char tmbuf[32];

		if (size < 2000)
		{
		    fprintf (out, "%ld Bytes", size);
		}
		else if (st.st_size < 10000)
		{
		    size = (size + 51)*10/1024;
		    fprintf (out, "%ld.%ld KB", size/10, size%10);
		}
		else if (st.st_size < 100000)
		{
		    size = (size + 512)/1024;
		    fprintf (out, "%ld KB", size);
		}
		else if (st.st_size < 10000000)
		{
		    size = (size + 51200)*10/(1024*1024);
		    fprintf (out, "%ld.%ld MB", size/10, size%10);
		}
		else
		{
		    size = (size + 512000)/(1024*1024);
		    fprintf (out, "%ld MB", size/10);
		}

		fputs (", ", out);

		tm = *localtime (&st.st_mtime);

		strftime (tmbuf, sizeof(tmbuf), "%d. %b %Y", &tm);
		fputs (tmbuf, out);
	    }

	    free (filename);
	}
	break;

    case APPENDIX:
	appendix = 1;
	chapter = 0;
	break;

    case TODAY:
	{
	    struct tm tm;
	    char tmbuf[32];
	    time_t tt;

	    time (&tt);
	    tm = *localtime (&tt);

	    strftime (tmbuf, sizeof(tmbuf), "%d. %b %Y", &tm);
	    fputs (tmbuf, out);
	}
	break;

    default:
	switch (token)
	{
	case BEGIN_NEW:
	    {
		char * date = va_arg (args, char *);
		int idate = getidate (date);

		if (idate + 30 > actualidate)
		    isnewtext = 1;
		else
		    isnewtext = 0;

		/* printf ("NEW %d %d %d\n", idate, actualidate, isnewtext); */

		liststack[listsp++] = lmode;
		lmode = lm_new;
	    }
	    break;

	case BEGIN:
	    if (listsp == LISTSTACKDEPTH)
		yyerror ("Too many \\begin{}s");
	    liststack[listsp] = lmode;
	    listsp ++;
	    lmode = convlistmode (va_arg (args, char *));

	    va_end (args);
	    va_start (args, token);
	    break;

	case CHAPTER:
	    chapter ++;
	    section = 0;
	    subsection = 0;
	    break;

	case SECTION:
	    section ++;
	    subsection = 0;
	    break;

	case SUBSECTION:
	    subsection ++;
	    break;

	}

	switch (emode)
	{
	case em_html:
	    emit_html (token, args);
	    break;

	default:
	    fprintf (stderr, "Illegal emit-mode %d\n", emode);
	    exit (10);
	}

	if (token == END)
	{
	    enum listmode lm2;

	    if (!listsp)
		yyerror ("Too many \\end{}s");

	    lm2 = convlistmode (va_arg (args, char *));

	    if (lm2 != lmode)
		yyerror ("\\end{%s} doesn't match \\begin{%s}",
			listmodename[lm2], listmodename[lmode]);

	    va_end (args);
	    va_start (args, token);

	    if (lmode == lm_new)
		isnewtext = 0;

	    lmode = liststack[--listsp];
	}
    }

    lasttoken = token;

    va_end (args);
}

enum listmode convlistmode (const char * name)
{
    enum listmode lm;

    for (lm=0; listmodename[lm]; lm++)
    {
	if (!strcmp (name, listmodename[lm]))
	    return lm;
    }

    yyerror ("Unknown mode \"%s\" in \\begin{}/\\end{}\n", name);
    return lm_none;
}

void deflref (const char * name, const char * str)
{
#if 0
    printf ("Adding key=\"%s\" data=\"%s\"\n", name, str);
#endif

    storedata (lrefs, xstrdup(name), xstrdup(str));
}

const char * getlref (const char * name)
{
    return retrievedata (lrefs, name);
}

void emit_char (int c)
{
    if (!isspace (c))
	lastemit = let_char;
    else if (c == '\n')
    {
	if (lastemit == let_nl)
	    lastemit = let_par;
	else if (lastemit == let_par)
	{
	    return;
	}
	else
	    lastemit = let_nl;
    }
    else
    {
	if (lastemit == let_nl || lastemit == let_par || lastemit == let_space)
	{
	    return;
	}

	lastemit = let_space;
    }

    emit_char_always (c);
}

void emit_char_always (int c)
{
    emitcount ++;
    putc (c, out);
}

void emit_string (const char * str)
{
    while (*str)
	emit_char (*str ++);
}

void emit_par (void)
{
    emit_char ('\n');
    emit_char ('\n');
}

void emit_nl (void)
{
    if (lastemit != let_par && lastemit != let_nl)
	emit_char ('\n');
}

void emit_space (void)
{
    if (lastemit == let_char || lastemit == let_special)
	emit_char (' ');
}

void emit_special (const char * fmt, ...)
{
    va_list args;
    va_start (args, fmt);

    if (lasttoken == TEXT && pendingspace)
    {
	emit_char (' ');
	pendingspace = 0;
    }

    lastemit = let_special;
    vfprintf (out, fmt, args);

    va_end (args);
}

