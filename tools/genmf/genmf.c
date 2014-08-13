/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#define PROTOTYPES
#define HAVE_STDARG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>

#if defined(HAVE_STDARG_H) && defined(__STDC__) && __STDC__
#   include <stdarg.h>
#   define VA_START(args, lastarg) va_start(args, lastarg)
#else
#   include <varargs.h>
#   define VA_START(args, lastarg) va_start(args)
#endif

#ifdef PROTOTYPES
#   define PARAMS(x) x
#else
#   define PARAMS(x) ()
#endif /* PROTOTYPES */


/* Types */
typedef struct _Node Node;

struct _Node
{
    Node * next,
	 * prev;
    char * name;
};

typedef struct
{
    Node * first,
	 * last,
	 * prelast;
}
List;

typedef struct
{
    int    len;
    int    max;
    char * value;
}
String;

typedef struct
{
    Node     node;
    int      argcount;
    List     args;
    String * body;
}
Template;

typedef struct
{
    Node   node;
    char * defval;
    /* char * value; */
    int    flags;
    int    pos;
#define ARGF_MULTI  1
#define ARGF_ALWAYS 2
#define ARGF_GOTIT  4
}
Arg;

/* Globals */
List templates;
char * curdir = NULL;

/* Macros */
#   define NewList(l)       (((List *)l)->prelast = (Node *)(l), \
			    ((List *)l)->last = 0, \
			    ((List *)l)->first = (Node *)&(((List *)l)->last))

#   define AddHead(l,n)     ((void)(\
	((Node *)n)->next        = ((List *)l)->first, \
	((Node *)n)->prev        = (Node *)&((List *)l)->first, \
	((List *)l)->first->prev = ((Node *)n), \
	((List *)l)->first       = ((Node *)n)))

#   define AddTail(l,n)     ((void)(\
	((Node *)n)->next          = (Node *)&((List *)l)->last, \
	((Node *)n)->prev          = ((List *)l)->prelast, \
	((List *)l)->prelast->next = ((Node *)n), \
	((List *)l)->prelast       = ((Node *)n) ))

#   define Remove(n)        ((void)(\
	((Node *)n)->prev->next = ((Node *)n)->next,\
	((Node *)n)->next->prev = ((Node *)n)->prev ))

#   define GetHead(l)       (void *)(((List *)l)->first->next \
				? ((List *)l)->first \
				: (Node *)0)
#   define GetTail(l)       (void *)(((List *)l)->prelast->prev \
				? ((List *)l)->prelast \
				: (Node *)0)
#   define GetNext(n)       (void *)(((Node *)n)->next->next \
				? ((Node *)n)->next \
				: (Node *)0)
#   define GetPrev(n)       (void *)(((Node *)n)->prev->prev \
				? ((Node *)n)->prev \
				: (Node *)0)
#   define ForeachNode(l,n) \
	for (n=(void *)(((List *)(l))->first); \
	    ((Node *)(n))->next; \
	    n=(void *)(((Node *)(n))->next))
#   define ForeachNodeSafe(l,node,nextnode) \
	for (node=(void *)(((List *)(l))->first); \
	    ((nextnode)=(void*)((Node *)(node))->next); \
	    (node)=(void *)(nextnode))

#define cfree(x)        if (x) free (x)
#define SETSTR(str,val) \
    cfree (str); \
    str = val ? xstrdup (val) : NULL

#define xstrdup(str)        _xstrdup(str,__FILE__,__LINE__)
#define xmalloc(size)       _xmalloc(size,__FILE__,__LINE__)
#define xfree(ptr)          _xfree(ptr,__FILE__,__LINE__)

#define new(type)           xmalloc(sizeof(type))

/* Prototypes */
extern void cleanup PARAMS ((int exitcode));
extern void error PARAMS ((const char * fmt, ...));

/* Functions */
char *
_xstrdup (const char * str, const char * file, int line)
{
    char * nstr;

    if (!str)
    {
	fprintf (stderr, "NULL string passed to strdup from %s:%d\n", file, line);
	cleanup (20);
    }

    nstr = strdup (str);

    if (!nstr)
    {
	fprintf (stderr, "Out of memory in %s:%d\n", file, line);
	cleanup (20);
    }

    return nstr;
}

void *
_xmalloc (size_t size, const char * file, int line)
{
    void * ptr;

    ptr = malloc (size);

    if (size && !ptr)
    {
	fprintf (stderr, "Out of memory in %s:%d", file, line);
	cleanup (20);
    }

    return ptr;
}

void
_xfree (void * ptr, const char * file, int line)
{
    if (ptr)
    {
#if 0
        ((int *)ptr)[0] = 0xDEADBEEF;
#endif
	free (ptr);
    }
    else
	fprintf (stderr, "Illegal free(NULL) in %s:%d", file, line);
}

Node * FindNode (const List * l, const char * name)
{
    Node * n;

    ForeachNode (l, n)
    {
	if (!strcmp (n->name, name))
	    return n;
    }

    return NULL;
}

void error (const char * fmt, ...)
{
    va_list args;
    VA_START (args, fmt);
    fprintf (stderr, "Error: ");
    vfprintf (stderr, fmt, args);
    fprintf (stderr, ": %s\n", strerror (errno));
    va_end (args);
    exit (1);
}

void cleanup (int exitcode)
{
#if 0
    if (startdir)
    {
	chdir (startdir);
	free (startdir);
    }
#endif

    exit (exitcode);
}

String * new_string (const char * init)
{
    String * s = new (String);

    if (init && *init)
    {
	s->len = strlen (init);
	s->max = (s->len+31)&-32;
	s->value = xmalloc (s->max+1);
	strcpy (s->value, init);
    }
    else
    {
	s->len = 0;
	s->max = 32;
	s->value = xmalloc (s->max+1);
	s->value[0] = 0;
    }

    return s;
}

void free_string (String * s)
{
    cfree (s->value);
    xfree (s);
}

void append_string (String * s, const char * app)
{
    int len;

    if (!app || !*app)
	return;

    len = strlen (app);

    if (len < s->max - s->len - 1)
    {
	strcpy (s->value+s->len, app);
	s->len += len;
    }
    else
    {
	char * ns;

	s->max = (s->len+len+31) & -32;

	ns = xmalloc (s->max+1);

	strcpy (ns, s->value);
	strcpy (ns+s->len, app);

	s->len += len;

	xfree (s->value);
	s->value = ns;
    }
}

String * getline (FILE * fh)
{
    String * s = NULL;
    char line[256], * ptr;
    int len;

    for (;;)
    {
	ptr = fgets (line, sizeof (line), fh);

	if (!ptr)
	    break;

	len = strlen (line);

	if (line[len - 1] == '\n')
	{
	    line[len - 1] = 0;

	    if (s)
		append_string (s, line);
	    else
		s = new_string (line);

	    break;
	}

	if (s)
	    append_string (s, line);
	else
	    s = new_string (line);
    }

    return s;
}

void add_continuation(String *s, FILE *fh)
{
    String *s2;
    
    while (s->value[s->len-1]=='\\')
    {
	s2 = getline(fh);
	s->value[s->len-1]='\0';
	s->len--;
	append_string(s, s2->value);
	free_string(s2);
    }
}

	    
void output (const char * line, FILE * outfile)
{
    const char * ptr = line;

    while (*ptr)
    {
	if (*ptr == '$' && ptr[1] == '(' && curdir &&
	    !strncmp ("CURDIR)", ptr+2, 7)
	)
	{
	    ptr += 9;
	    fputs (curdir, outfile);
	}
	else
	{
	    fputc (*ptr, outfile);
	    ptr ++;
	}
    }
}

char ** getargs (const char * line, int * argc)
{
    static char * argv[64];
    static char * buffer = NULL;
    char * src;
    int arg;

    cfree (buffer);

    assert (line);

    buffer = xstrdup (line);

    assert (buffer);

    src = buffer;
    arg = 0;

    while (*src)
    {
	while (isspace (*src))
	    src ++;

	if (!*src)
	    break;

	assert (arg < 63);
	argv[arg++] = src;

	while (*src && !isspace (*src))
	{
	    if (*src == '"')
	    {
		src ++;

		while (*src && *src != '"')
		    src ++;

		src ++;
	    }
	    else
		src ++;
	}

	if (*src)
	    *src++ = 0;
    }

    argv[arg] = NULL;

    if (argc)
	*argc = arg;

    return argv;
}

void print_template (Template * tmpl)
{
    Arg * arg;

    printf ("Template %s:\n", tmpl->node.name);
    ForeachNode (&tmpl->args, arg)
    {
	printf ("   %s (%s) %d\n", arg->node.name,
	    arg->defval ? arg->defval : "--none--",
	    arg->flags
	);
    }
    printf ("   body=%s---\n", tmpl->body->value);
}

void read_templates (const char * fn)
{
    FILE * fh;
    String * line;
    Template * tmpl = NULL;
    char * ptr;
    char ** argv;
    int argc;

    if (!(fh = fopen (fn, "r")) )
    {
	error ("Can't open %s for reading", fn);
	cleanup (10);
    }

    while ((line = getline (fh)))
    {
	ptr = line->value;
	while (isspace (*ptr)) ptr++;

	if (!strncmp (ptr, "%define", 7))
	{
	    int t;
	    char * defval, * flags;
	    Arg * arg;

	    add_continuation(line, fh);
	    
	    tmpl = new (Template);

	    NewList (&tmpl->args);
	    tmpl->body = NULL;

	    argv = getargs (line->value, &argc);

	    tmpl->node.name = xstrdup (argv[1]);
	    tmpl->argcount = argc-2;

	    for (t=2; t<argc; t++)
	    {
		defval = argv[t];

		while (*defval && *defval != '=') defval ++;

		if (*defval)
		    *defval ++ = 0;

		flags = defval;

		if (*flags)
		{
		    flags += strlen (flags);

		    while (flags-2 >= defval && flags[-2] == '/' && isalpha (flags[-1]))
			flags -= 2;

		    if (*flags)
			*flags++ = 0;
		}

#if 0
    printf ("arg=\"%s\" defval=\"%s\" flags=\"%s\"\n",
	argv[t], defval, flags);
#endif

		arg = new (Arg);

		arg->node.name = xstrdup (argv[t]);
		arg->pos = t-2;

		if (*defval)
		{
		    if (*defval == '"')
		    {
			defval ++;
			defval[strlen (defval) -1] = 0;
		    }

		    arg->defval = xstrdup (defval);
		}
		else
		    arg->defval = NULL;

		arg->flags = 0;

		while (*flags)
		{
		    if (*flags == 'M')
			arg->flags |= ARGF_MULTI;
		    else if(*flags == 'A')
			arg->flags |= ARGF_ALWAYS;
		    else
			error ("Unknown flag %s in argument %s of template %s",
			    flags, arg->node.name, tmpl->node.name);

		    flags ++;
		    if (*flags)
			flags ++;
		}

		AddTail (&tmpl->args, arg);
	    }

	    free_string (line);

	    while ((line = getline (fh)))
	    {
		if (!strcmp (line->value, "%end"))
		    break;

		ptr = line->value;
		while (isspace(*ptr)) ptr++;
		if (*ptr=='%' && ptr[1]!='(' && ptr[1]!='\0')
		    add_continuation(line, fh);
		
		if (tmpl->body)
		    append_string (tmpl->body, line->value);
		else
		    tmpl->body = new_string (line->value);

		append_string (tmpl->body, "\n");
	    }

#if 0
	    print_template (tmpl);
#endif
	    AddTail (&templates, tmpl);
	}

	free_string (line);
    }

    fclose (fh);
}

Arg * findArg (Template * tmpl, char ** argstr)
{
    char argname[64];
    char * ptr = *argstr;
    int len;
    Arg * arg;

    ptr += 2;
    len = 0;

    while (*ptr && *ptr != ')')
	argname[len++] = *ptr++;

    argname[len] = 0;
    if (*ptr) ptr++;

    arg = (Arg *)FindNode (&tmpl->args, argname);

    if (!arg)
	error ("Unknown argument %s for template %s",
	    argname, tmpl->node.name);

    *argstr = ptr;

    return arg;
}

void replace_template (const char * string, FILE * outfile)
{
    char ** argv;
    int argc, t;
    Template * tmpl;
    Arg * arg;
    char * argnptr, * value, * ptr;
    char ** values, * freeflags;

    argv = getargs (string, &argc);

    tmpl = (Template *) FindNode (&templates, argv[0]+1);

    if (!tmpl)
    {
	/* Print no error for GNU Make patterns */
	/* error ("Can't find template %s\n", argv[0]); */
	fputs (string, outfile);
    }
    else
    {
	values = xmalloc (sizeof (values[0]) * tmpl->argcount);
	freeflags = xmalloc (sizeof (freeflags[0]) * tmpl->argcount);

	t = 0;
	ForeachNode (&tmpl->args, arg)
	{
	    freeflags[t] = 0;
	    values[t] = arg->defval ? arg->defval : "";
	    t ++;
	}
	assert (t == tmpl->argcount);

	arg = GetHead (&tmpl->args);

	for (t=1; t<argc; t++)
	{
	    value = argv[t];
	    while (*value && *value != '=') value++;

	    if (*value)
	    {
		argnptr = argv[t];
		*value ++ = 0;
	    }
	    else
	    {
		argnptr = "";
		value = argv[t];
	    }

	    if (*value == '"')
	    {
		value ++;
		value[strlen (value) - 1] = 0;
	    }

	    if (*argnptr)
	    {
		arg = (Arg *) FindNode (&tmpl->args, argnptr);
		if (!arg)
		    error ("Unknown argument %s to template %s", argnptr, tmpl->node.name);
	    }

	    if (arg->flags & ARGF_MULTI)
	    {
		String * vals = new_string (value);

		for (t++; t<argc; t++)
		{
		    append_string (vals, " ");
		    append_string (vals, argv[t]);
		}

		values[arg->pos] = vals->value;
		freeflags[arg->pos] = 1;
		vals->value = NULL;
		free_string (vals);
		arg->flags |= ARGF_GOTIT;
	    }
	    else
	    {
		arg->flags |= ARGF_GOTIT;
		values[arg->pos] = xstrdup (value);
		freeflags[arg->pos] = 1;
		arg = GetNext (arg);
	    }
	}

	ForeachNode (&tmpl->args, arg)
	{
	    if(	    arg->flags & ARGF_ALWAYS
		&&  (arg->flags & ARGF_GOTIT) == 0 )
	    {
		error("No value supplied for argument %s for template %s",
		    arg->node.name, tmpl->node.name
		);
	    }
	}

	for (ptr=tmpl->body->value; *ptr; )
	{
	    if (*ptr == '%' && ptr[1] == '(' /*)*/)
	    {
		arg = findArg (tmpl, &ptr);
		if (arg)
		    output (values[arg->pos], outfile);
	    }
	    else if (*ptr == '%')
	    {
		/* nested template */
		String * str = new_string ("");
		char app[2];
		app[1] = 0;

		while (*ptr)
		{
		    if (*ptr == '%' && ptr[1] == '(' /*)*/)
		    {
			arg = findArg (tmpl, &ptr);
			if (arg)
			    append_string (str, values[arg->pos]);
		    }
		    else
		    {
			app[0] = *ptr++;
			append_string (str, app);
		    }

		    if (*ptr == '\n')
			break;
		}

		replace_template (str->value, outfile);
		free_string (str);
	    }
	    else if (*ptr == '$' && ptr[1] == '(' && curdir &&
		    !strncmp ("CURDIR)", ptr+2, 7)
		)
	    {
		ptr += 9;
		fputs (curdir, outfile);
	    }
	    else
	    {
		fputc (*ptr, outfile);
		ptr ++;
	    }
	}

	for (t=0; t<tmpl->argcount; t++)
	{
	    if (freeflags[t])
		xfree (values[t]);
	}

	xfree (values);
	xfree (freeflags);
    }
}

void process_file(FILE * infile, FILE *outfile)
{
    char * ptr;
    String * line;
    int common = 0;

    fputs (
	"####################################################################\n"
	"####################################################################\n"
	"############# THIS IS A GENERATED FILE ! DO NOT EDIT ###############\n"
	"####################################################################\n"
	"####################################################################\n"
	, outfile
    );

    while ((line = getline (infile)))
    {
	ptr = line->value;
	while (isspace (*ptr)) ptr++;

	if (*ptr == '%')
	{
	    int pos = ptr - line->value;
	    
	    if (!strncmp (ptr+1, "common", 6))
		common = 1;
	    
	    add_continuation (line, infile);
	    ptr = line->value + pos;
	    
	    replace_template (ptr, outfile);
	}
	else
	{
	    output (line->value, outfile);
	    fputc ('\n', outfile);
	}

	free_string (line);
    }

    if (!common)
    {
	replace_template ("%common", outfile);
    }

}

int main (int argc, char ** argv)
{
    int t;
    FILE * infile = NULL, * outfile = NULL;
    FILE * listfile = NULL;
    char * infilename, *outfilename;
    String * line;

    NewList (&templates);

    for (t=1; t<argc; t++)
    {
        if (!strncmp ("--curdir=", argv[t], 9))
            curdir = argv[t] + 9;
        else if(!strncmp ("--listfile", argv[t], 10)) 
        {
            // next parameter is the list file name
            if (t+1 <= argc)
            {
                if (!(listfile = fopen (argv[t+1], "r" )))
                {
                    error ( "Can't open list file %s\n", argv[t+1] );
                    t++;
                }
            }
            else
                error ( "No list file name given\n" );
        }
        else
	        read_templates (argv[t]);
    }
    if (listfile)
    {
        for (t = 0; (line = getline (listfile)); t++)
        {
            infilename = strtok (line->value, " \t"); 
            if (!infilename)
                error("Syntax error in list file at line %d\n", t);
            outfilename = strtok (NULL, " \t");
            if (!outfilename)
                error("Syntax error in list file at line %d\n", t);
            if (!(infile = fopen (infilename, "r")))
                error("Can't open input file %s\n", infilename);
            if (!(outfile = fopen (outfilename, "w")))
                error("Can't open output file %s\n", outfilename);
            process_file (infile, outfile);
            fclose (infile);
            fclose (outfile);
            free_string (line);
        }
        fclose (listfile);
    }
    else process_file (stdin, stdout);

    return 0;
}
