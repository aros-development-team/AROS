#include "gram.h"
#include "emit.h"
#include "emit_intern.h"
#include "error.h"
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "scan.h"
#include "util.h"
#include "db.h"

static void emit_html_string (const unsigned char * str);
static void emit_html_code (const unsigned char * str);
static void emit_html_string_ws (const unsigned char * str);
static void emit_html_char (int c);
static void emit_html_char_always (int c);
static void emit_text (const unsigned char * str);
static void emit_code (const unsigned char * str);
static void forcebreak (void);
static void checkbreak (void);
static void readlinks (void);
static void emit_html_toc (void);
static void paste_html_file (const char * name);

extern void * yy_scan_string (const char *str);

static int pendingspace = 0;
static int labelcount = 0;

static FILE * tocfh;
static FILE * linkfh;
static char * newtocname;
static char * oldtocname;
static char * newlinkname;
static char * oldlinkname;
static char * baseoutname;
static int filecount;
static int lastemitcount = 0;
static char today[32];
static char * links[256];
static int    linkcnt;

static char * thischapter = NULL;

void setthischapter (const char * chapter)
{
    if (thischapter)
    {
	xfree (thischapter);
	thischapter = NULL;
    }

    thischapter = strdup (chapter);
}

#define MAXFILESIZE	15000

void emit_html_init (void)
{
    struct tm tm;
    time_t tt;

    baseoutname = xbasename (outname, ".html");

    newtocname = buildname ("%s.newtoc", baseoutname);
    oldtocname = buildname ("%s.toc", baseoutname);
    newlinkname = buildname ("%s.newlink", baseoutname);
    oldlinkname = buildname ("%s.link", baseoutname);

    tocfh = fopen (newtocname, "w");

    if (!tocfh)
	yyerror ("Can't open \"%s\" for writing: %s\n", newtocname,
	    strerror(errno));

    linkfh = fopen (newlinkname, "w");

    if (!linkfh)
	yyerror ("Can't open \"%s\" for writing: %s\n", newlinkname,
	    strerror(errno));

    filecount = 0;

    paste_html_file ("doc_header.html");
    emit_html_toc ();
    paste_html_file ("doc_footer.html");

    readlinks ();

    time (&tt);
    tm = *localtime (&tt);

    strftime (today, sizeof(today), "%d. %b %Y\n", &tm);
}

void readlinks (void)
{
    FILE * fh;
    char line[256];

    fh = fopen (oldlinkname, "r");

    if (!fh)
	return;

    linkcnt = 0;

    while (fgets (line, sizeof(line), fh))
    {
	line[strlen(line)-1] = 0;

	links[linkcnt++] = xstrdup (line);
    }

    fclose (fh);
}

void emit_html_exit (void)
{
    fclose (tocfh);
    fclose (linkfh);

    rename (newtocname, oldtocname);
    rename (newlinkname, oldlinkname);
}

static void forcebreak (void)
{
    if (filecount)
	paste_html_file ("page_footer.html");

    /* emit_special ("<CENTER><HR WIDTH=\"80%\"></CENTER><P>"); */
    lastemitcount = emitcount;
    filecount ++;

    if (out != stdout)
    {
	xfree (outname);
	fclose (out);

	outname = buildname ("%s-%d.html", baseoutname, filecount);
	out = fopen (outname, "w");

	if (!out)
	    yyerror ("Cannot open \"%s\" for writing: %s",
		outname, strerror (errno));

	fprintf (linkfh, "%s\n", outname);
    }

    paste_html_file ("page_header.html");
}

static void checkbreak (void)
{
    if (emitcount - lastemitcount > MAXFILESIZE)
    {
	forcebreak ();
    }
}

static void emit_html_toc (void)
{
    FILE * fh;
    char   line[256], title[256], filename[256], datestr[64];
    int    typ;
    int    label;
    char   num[32];
    time_t mod;
    struct tm tm;
    int    s = 0, ss = 0;

    fh = fopen (oldtocname, "r");

    if (!fh)
	return;

    emit_par ();
    emit_special ("<UL>\n\n");

    while (fgets (line, sizeof(line), fh))
    {
	if (!fgets (filename, sizeof(title), fh))
	    break;
	if (!fgets (title, sizeof(title), fh))
	    break;

	sscanf (line, "%d %s %lu %d", &typ, num, &mod, &label);
	filename[strlen (filename)-1] = 0;
	title[strlen (title)-1] = 0;

	tm = *localtime (&mod);
	strftime (datestr, sizeof(datestr), "%d. %b %Y", &tm);

	switch (typ)
	{
	case 1: /* Appendix */
	    if (ss)
	    {
		emit_special ("</UL>\n");
		ss = 0;
	    }
	    if (s)
	    {
		emit_special ("</UL>\n");
		s = 0;
	    }
	    emit_special ("<LI><FONT SIZE=\"+1\"><B><A HREF=\"%s#%d\">Appendix %s %s (%s)</A></B></FONT>\n",
		filename,
		label,
		num,
		title,
		datestr
	    );
	    break;

	case 2: /* Chapter */
	    if (ss)
	    {
		emit_special ("</UL>\n");
		ss = 0;
	    }
	    if (s)
	    {
		emit_special ("</UL>\n");
		s = 0;
	    }
	    emit_special ("<LI><FONT SIZE=\"+1\"><B><A HREF=\"%s#%d\">Chapter %s %s (%s)</A></B></FONT>\n",
		filename,
		label,
		num,
		title,
		datestr
	    );
	    break;

	case 3: /* Section */
	    if (!s)
	    {
		emit_special ("<UL>\n");
	    }
	    s++;
	    if (ss)
	    {
		emit_special ("</UL>\n");
		ss = 0;
	    }
	    emit_special ("<LI><B><A HREF=\"%s#%d\">%s %s (%s)</A></B>\n",
		filename,
		label,
		num,
		title,
		datestr
	    );
	    break;

	case 4: /* Subsection */
	    if (!ss)
	    {
		emit_special ("<UL>\n");
	    }
	    ss++;
	    emit_special ("<LI><FONT SIZE=\"-1\"><B><A HREF=\"%s#%d\">%s %s (%s)</A></B></FONT>\n\n",
		filename,
		label,
		num,
		title,
		datestr
	    );
	    break;
	}
    }

    if (ss)
	emit_special ("</UL>\n");

    if (s)
	emit_special ("</UL>\n");

    emit_special ("</UL>\n");

    fclose (fh);

    emit_par ();
}

static void paste_html_file (const char * name)
{
    char line[256];
    FILE * fh;

    fh = fopen (name, "r");

    if (!fh)
	yyerror ("Cannot open \"%s\" for reading: %s",
	    outname, strerror (errno));

    while (fgets (line, sizeof(line), fh))
    {
	if (!strcmp (line, "\\next\n"))
	{
	    if (filecount < linkcnt)
		snprintf (line, sizeof(line), "<A HREF=\"%s\">next</A>\n",
		    links[filecount]);
	    else
		strcpy (line, "next\n");
	}
	else if (!strcmp (line, "\\prev\n"))
	{
	    if (filecount > 1)
		snprintf (line, sizeof(line), "<A HREF=\"%s\">prev</A>\n",
		    links[filecount-2]);
	    else
		strcpy (line, "prev\n");
	}
	else if (!strcmp (line, "\\today\n"))
	{
	    strcpy (line, today);
	}
	else if (!strcmp (line, "\\thischapter\n"))
	{
	    strcpy (line, thischapter);
	}

	fputs (line, out);
    }

    fclose (fh);
}

void emit_html (int token, va_list args)
{
    switch (token)
    {
    case IF:
	{
	    char * scond, * sthen, * selse;
	    int    cond = 0;

	    scond = va_arg (args, char *);
	    sthen = va_arg (args, char *);
	    selse = va_arg (args, char *);

	    if (!strcmp (scond, "html"))
		cond = 1;
	    else
		yyerror ("Unknown condition {%s}", scond);

	    if (cond)
	    {
		scan_push ();
		filename = xstrdup (filename);
		yy_scan_string (sthen);
	    }
	    else if (selse)
	    {
		scan_push ();
		filename = xstrdup (filename);
		yy_scan_string (selse);
	    }
	}
	break;

    case CHAPTER:
	{
	    char * text = va_arg (args, char *);

	    setthischapter (text);

	    forcebreak ();

	    if (appendix)
	    {
		emit_special ("<H1><A NAME=\"%d\">Appendix %c ",
		    labelcount,
		    'A' + chapter - 1
		);
		printf ("Appendix %c ", 'A' + chapter - 1);
		fprintf (tocfh, "1 %c", 'A' + chapter - 1);
	    }
	    else
	    {
		emit_special ("<H1><A NAME=\"%d\">Chapter %d. ",
		    labelcount,
		    chapter
		);
		printf ("Chapter %d. ", chapter);
		fprintf (tocfh, "2 %d", chapter);
	    }
	    fprintf (tocfh, " %lu %d\n", getfiledate (filename), labelcount);
	    fprintf (tocfh, "%s\n%s%s\n", outname, text,
		isnewtext ? " *New*" : "");

	    emit_html_string_ws (text);
	    if (isnewtext) emit_html_string (" *New*");
	    printf ("%s\n", text);
	    emit_special ("</A></H1>");
	    emit_par ();
	    labelcount ++;
	}
	break;

    case SECTION:
	{
	    char * text = va_arg (args, char *);

	    checkbreak ();

	    if (appendix)
	    {
		emit_special ("<H2><A NAME=\"%d\">%c",
		    labelcount,
		    'A' + chapter - 1
		);
		fprintf (tocfh, "3 %c.%d", 'A' + chapter - 1, section);
	    }
	    else
	    {
		emit_special ("<H2><A NAME=\"%d\">%d",
		    labelcount,
		    chapter
		);
		fprintf (tocfh, "3 %d.%d", chapter, section);
	    }
	    fprintf (tocfh, " %lu %d\n", getfiledate (filename), labelcount);
	    fprintf (tocfh, "%s\n%s%s\n", outname, text,
		isnewtext ? " *New*" : "");

	    emit_special (".%d ", section);
	    emit_html_string_ws (text);
	    if (isnewtext) emit_html_string (" *New*");
	    emit_special ("</A></H2>\n\n");
	    emit_par ();
	    labelcount ++;
	}
	break;

    case SUBSECTION:
	{
	    char * text = va_arg (args, char *);

	    if (appendix)
	    {
		emit_special ("<H3><A NAME=\"%d\">%c",
		    labelcount,
		    'A' + chapter - 1
		);
		fprintf (tocfh, "4 %c.%d.%d", 'A' + chapter - 1, section, subsection);
	    }
	    else
	    {
		emit_special ("<H3><A NAME=\"%d\">%d",
		    labelcount,
		    chapter
		);
		fprintf (tocfh, "4 %d.%d.%d", chapter, section, subsection);
	    }
	    fprintf (tocfh, " %lu %d\n", getfiledate (filename), labelcount);
	    fprintf (tocfh, "%s\n%s%s\n", outname, text,
		isnewtext ? " *New*" : "");

	    emit_special (".%d.%d ", section, subsection);
	    emit_html_string_ws (text);
	    if (isnewtext) emit_html_string (" *New*");
	    emit_special ("</A></H3>\n\n");
	    emit_par ();
	    labelcount ++;
	}
	break;

    case TOC:
	break;

    case PAR:
	emit_special ("<P>");
	emit_par ();
	break;

    case NL:
	emit_special ("<BR>");
	emit_nl ();
	break;

    case SPACE:
	emit_special ("&nbsp;");
	break;

    case SMALLCODE:
	{
	    char * code = va_arg (args, char *);

	    emit_space ();
	    emit_special ("<TT>");
	    emit_code (code);
	    emit_special ("</TT>");
	}
	break;

    case BIGCODE:
	emit_space ();
	emit_special ("<UL><PRE>");
	emit_code (va_arg (args, char *));
	emit_special ("</PRE></UL>");
	break;

    case SHELL:
	emit_space ();
	emit_special ("<TT>");
	emit_html_string_ws (va_arg (args, char *));
	emit_special ("</TT>");
	break;

    case BOLD:
	emit_space ();
	emit_special ("<B>");
	emit_text (va_arg (args, char *));
	emit_special ("</B>");
	break;

    case ITALICS:
	emit_space ();
	emit_special ("<I>");
	emit_text (va_arg (args, char *));
	emit_special ("</I>");
	break;

    case SMALLPIC:
	emit_space ();
	emit_special ("<IMG SRC=\"%s\">", va_arg (args, char *));
	break;

    case LARGEPIC:
	{
	    char * text, * file;

	    text = va_arg (args, char *);
	    file = va_arg (args, char *);

	    emit_space ();
	    emit_special ("<A HREF=\"%s\">", file);
	    emit_html_string_ws (text);
	    emit_special ("</A>");
	}
	break;

    case FILENAME:
	{
	    char * filename = va_arg (args, char *);
	    const char * file;

	    file = getfile (filename);

	    emit_space ();
	    emit_special ("<B><I>");
	    if (file)
		emit_special ("<A HREF=\"%s\">", file);
	    emit_html_string_ws (filename);
	    if (file)
		emit_special ("</A>");
	    emit_special ("</I></B>");
	    /* lastemit = let_char; */
	}
	break;

    case TEXT:
	emit_text (va_arg (args, char *));
	break;

    case EXAMPLE:
	emit_par ();
	emit_special ("<UL><PRE>");
	emit_html_code (va_arg(args, char *));
	emit_special ("</PRE></UL>");
	emit_par ();
	break;

    case BEGIN_NEW:
	if (isnewtext)
	    emit_special ("<FONT COLOR=\"#108010\">");
	break;

    case BEGIN:
	emit_par ();

	switch (lmode)
	{
	case lm_none: break;
	case lm_new: /* doesn't happen */
	case lm_methods:
	case lm_tags:
	case lm_description: emit_special ("<DL>"); break;
	case lm_itemize:     emit_special ("<UL>"); break;
	case lm_enumeration: emit_special ("<OL TYPE=1>"); break;
	case lm_emph:	     emit_special ("<P>\n<UL>\n<EM>"); break;
	case lm_indent:      emit_special ("<UL>"); break;
	}
	break;

    case ITEM:
	{
	    char * text = va_arg (args, char *);

	    switch (lmode)
	    {
	    case lm_none:
		yyerror ("Unexpected \\item in normal text");
		break;

	    case lm_new:
		yyerror ("Unexpected \\item in \\begin{new}");
		break;

	    case lm_methods:
	    case lm_tags:
		if (!text) yyerror ("Missing argument for \\item in description");
		emit_special ("<P>\n\n<DT><TT>");
		emit_html_string_ws (text);
		emit_special ("</TT><DD>");
		emit_char ('\n');

		break;

	    case lm_description:
		if (!text) yyerror ("Missing argument for \\item in description");
		emit_special ("<P>\n\n<DT><I>");
		emit_html_string_ws (text);
		emit_special ("</I><DD>");
		emit_char ('\n');

		break;

	    case lm_itemize:
	    case lm_enumeration:
		if (text) yyerror ("Unexpected argument for \\item in itemize/enumeration");
		emit_char ('\n');
		emit_special ("<LI>");
		break;

	    case lm_emph:
		yyerror ("Unexpected \\item in emph/example");
		break;

	    case lm_indent:
		yyerror ("Unexpected \\item in indented block");
		break;
	    }
	}
	break;

    case END:
	switch (lmode)
	{
	case lm_none:
		break;
	case lm_new:
	    if (isnewtext)
		emit_special ("</FONT>");
	    break;

	case lm_methods: /* fall-thru */
	case lm_tags:    /* fall-thru */
	case lm_description:
	    emit_nl (); emit_special ("</DL>");
	    break;

	case lm_indent:  /* fall-thru */
	case lm_itemize:
	    emit_nl (); emit_special ("</UL>");
	    break;

	case lm_enumeration:
	    emit_nl (); emit_special ("</OL>");
	    break;

	case lm_emph:
	    emit_special ("</EM>\n</UL>");
	    break;
	}
	emit_par ();
	break;

    case LINK:
	{
	    char * text, * dest;

	    text = va_arg (args, char *);
	    dest = va_arg (args, char *);

	    emit_space ();
	    emit_special ("<A HREF=\"%s\">", dest);
	    emit_html_string (text);
	    emit_special ("</A>");
	}
	break;

    case EMAIL:
	{
	    char * text;

	    text = va_arg (args, char *);

	    emit_space ();
	    emit_special ("<A HREF=\"mailto:%s\">", text);
	    emit_html_string (text);
	    emit_special ("</A>");
	}
	break;

    case LREF:
	{
	    char * key, * text;
	    const char * str;

	    text = va_arg (args, char *);
	    key = va_arg (args, char *);

	    str = getlref (key);

	    emit_space ();

	    if (!str)
	    {
		yywarn ("Can't find reference \"%s\"", key);
		emit_html_string (text);
	    }
	    else
	    {
		emit_space ();
		emit_special ("<A HREF=\"%s\">", str);
		emit_html_string (text);
		emit_special ("</A>");
	    }
	}
	break;

    case LABEL:
	{
	    char * name, * str;
	    char buffer[64];

	    name = va_arg (args, char *);

	    if (*outname == '<')
		yyerror ("\\label is not possible if output is not a file");

	    snprintf (buffer, sizeof(buffer), "%s#%d", outname, labelcount);
	    str = xstrdup (buffer);

	    if (!str)
		yyerror ("Out of memory");

	    deflref (name, str);

	    emit_space ();
	    emit_special ("<A NAME=\"%s\"></A>", str);

	    labelcount ++;
	}
	break;

    default:
	yyerror ("Unknown token %d\n", token);
	/* doesn't return */
    }
}

void emit_text (const unsigned char * str)
{
    int haspar;

    if (isspace (*str))
    {
	haspar = 0;

	while (isspace (*str))
	{
	    if (*str == '\n' && str[1] == '\n')
	    {
		haspar = 1;
		str += 2;
	    }
	    else
		str ++;
	}

	if (lastemit == let_char && haspar)
	    emit_string ("<P>\n\n");
	else if (lastemit == let_special || pendingspace)
	    emit_char (' ');
    }

    pendingspace = 0;

    while (*str)
    {
	if (isspace (*str))
	{
	    haspar = 0;

	    while (isspace (*str))
	    {
		if (*str == '\n' && str[1] == '\n')
		{
		    haspar = 1;
		    str += 2;
		}
		else
		    str ++;
	    }

	    if (haspar)
	    {
		emit_string ("<P>\n\n");
	    }
	    else if (*str)
		emit_char (' ');
	    else
		pendingspace = 1;
	}
	else
	{
	    emit_html_char (*str);

	    str ++;
	}
    }
}

void emit_code (const unsigned char * str)
{
    char identifier[256], * ptr;
    const char * file;
    int comment=0;

    while (*str)
    {
	if (*str == '/' && str[1] == '*')
	    comment = 1;
	else if (*str == '*' && str[1] == '/')
	    comment = 0;

	if (!comment)
	{
	    if (isalpha (*str) || *str == '_')
	    {
		ptr = identifier;

		while (isalnum (*str) || *str == '_')
		    *ptr ++ = *str ++;

		*ptr = 0;

		file = getkeyword (identifier);

/* printf ("keyword=\"%s\"", identifier);
if (file) printf (" file=\"%s\"\n", file);
else printf ("no file\n"); */

		if (file)
		{
		    switch (*file)
		    {
		    case 'T': emit_special ("<B>"); break;
		    case 'K': emit_special ("<B><I>"); break;
		    case 'D': emit_special ("<B>"); break;
		    default: emit_special ("<I>"); break;
		    }

		    if (file[1])
			emit_special ("<A HREF=\"%s\">", file+1);
		}
		else
		    emit_special ("<I>");

		emit_html_string (identifier);
		if (file)
		{
		    if (file[1])
			emit_special ("</A>");

		    switch (*file)
		    {
		    case 'T': emit_special ("</B>"); break;
		    case 'K': emit_special ("</I></B>"); break;
		    case 'D': emit_special ("</B>"); break;
		    default: emit_special ("</I>"); break;
		    }
		}
		else
		    emit_special ("</I>");
	    }
	    else if (*str == '"')
	    {
		emit_special ("<B>");
		emit_html_char_always (*str ++);

		while (*str && *str != '"')
		{
		    if (*str == '\\')
		    {
			emit_html_char_always (*str ++);
			emit_html_char_always (*str ++);
		    }
		    else
			emit_html_char_always (*str ++);
		}

		emit_html_char_always (*str ++);
		emit_special ("</B>");
	    }
	    else
	    {
		emit_html_char_always (*str ++);
	    }
	}
	else
	{
	    emit_html_char_always (*str++);
	}
    }
}

void emit_html_string (const unsigned char * str)
{
    while (*str)
    {
	emit_html_char (*str);

	str ++;
    }
}

void emit_html_code (const unsigned char * str)
{
    while (*str)
    {
	emit_html_char_always (*str);

	str ++;
    }
}

void emit_html_string_ws (const unsigned char * str)
{
    if (isspace (*str))
    {
	while (isspace (*str))
	    str ++;

	if (lastemit == let_special || pendingspace)
	    emit_char (' ');
    }

    pendingspace = 0;

    while (*str)
    {
	if (isspace (*str))
	{
	    while (isspace (*str))
		str ++;

	    if (*str)
		emit_char (' ');
	    else
		pendingspace = 1;
	}
	else
	{
	    emit_html_char (*str);

	    str ++;
	}
    }
}

void emit_html_char (int c)
{
    switch (c)
    {
    case '&': emit_special ("&amp;"); break;
    case '<': emit_special ("&lt;"); break;
    case '>': emit_special ("&gt;"); break;
    case '"': emit_special ("&#34;"); break;
    default:  emit_char (c); break;
    }
}

void emit_html_char_always (int c)
{
    switch (c)
    {
    case '&': emit_special ("&amp;"); break;
    case '<': emit_special ("&lt;"); break;
    case '>': emit_special ("&gt;"); break;
    case '"': emit_special ("&#34;"); break;
    default:  emit_char_always (c); break;
    }
}


