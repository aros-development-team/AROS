#include <toollib.h>
#include <stringcb.h>
#include <stdiocb.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include "parse.h"
#include "parse_html.h"
#include "html.h"

typedef struct
{
    Node   node;
    char * begin;
    char * end;
}
HTMLEnv;

typedef struct
{
    Node   node;
    List   args; /* HTMLTagArg */
    String body;
}
HTMLMacro;

static List Defs;   /* Macros */
static List EDefs;  /* Environments */

static int HTML_DEF	 PARAMS ((HTMLTag * tag, CB getc, void * stream, CBD data));
static int HTML_EDEF	 PARAMS ((HTMLTag * tag));
static int HTML_OtherTag PARAMS ((HTMLTag * tag));

static void HTML_FreeMacro (HTMLMacro * macro)
{
    Node * node, * next;

    VS_Delete (macro->body);

    ForeachNodeSafe (&macro->args, node, next)
    {
	Remove (node);

	xfree (node->name);
	xfree (node);
    }

    xfree (macro->node.name);
    xfree (macro);
}

static int HTML_DEF (HTMLTag * tag, CB getc, void * stream, CBD data)
{
    HTMLMacro  * old,
	       * macro;
    HTMLTagArg * name,
	       * arg;
    Node       * option;
    int 	 start;
    int 	 c;

    old = (HTMLMacro *) FindNode (&Defs, tag->node.name);

    if (old)
    {
	Warn ("Macro %s redefined\n", tag->node.name);

	Remove (old);
	HTML_FreeMacro (old);
    }

    name = (HTMLTagArg *) FindNode (&tag->args, "NAME");

    if (!name)
    {
	PushError ("HTML tag DEF: Missing argument NAME");
	return 0;
    }

    if (!name->value)
    {
	PushError ("HTML tag DEF: Missing value for argument NAME");
	return 0;
    }

    macro = new (HTMLMacro);

    macro->node.name = stripquotes (name->value);
    NewList (&macro->args);

    ForeachNode (&tag->args, arg)
    {
	if (!strcmp (arg->node.name, "OPTION"))
	{
	    if (!arg->value)
	    {
		PushError ("HTML tag DEF: Missing value for argument OPTION");
		return 0;
	    }

	    option = new (Node);

	    option->name = stripquotes (arg->value);
	    strupper (option->name);
	    AddTail (&macro->args, option);
	}
    }

    macro->body = VS_New (NULL);
    start = 0;

    while ((c = CallCB (getc, stream, STRCB_GETCHAR, data)) != EOF)
    {
	if (c == '<')
	    start = macro->body->len;

	if (c == '>')
	{
	    if (!strcasecmp (macro->body->buffer+start+1, "DEF"))
	    {
		PushError ("HTML tag DEF: Nested DEF");
		return 0;
	    }
	    else if (!strcasecmp (macro->body->buffer+start+1, "/DEF"))
	    {
		if (start)
		    start --;
		macro->body->len = start;
		macro->body->buffer[start] = 0;
		break;
	    }
	}

	VS_AppendChar (macro->body, c);
    }

printf ("Value=%s\n", macro->body->buffer);

    AddTail (&Defs, macro);

    return 1;
}

static void HTML_FreeEnv (HTMLEnv * env)
{
    cfree (env->begin);
    cfree (env->end);
    xfree (env->node.name);
    xfree (env);
}

static int HTML_EDEF (HTMLTag * tag)
{
    HTMLEnv    * old;
    HTMLTagArg * name,
	       * begin,
	       * end;
    HTMLEnv    * env;

    old = (HTMLEnv *) FindNode (&EDefs, tag->node.name);

    if (old)
    {
	Warn ("Environment %s redefined\n", tag->node.name);

	Remove (old);
	HTML_FreeEnv (old);
    }

    name  = (HTMLTagArg *) FindNode (&tag->args, "NAME");
    begin = (HTMLTagArg *) FindNode (&tag->args, "BEGIN");
    end   = (HTMLTagArg *) FindNode (&tag->args, "END");

    if (!name)
    {
	PushError ("HTML tag EDEF: Missing argument NAME");
	return 0;
    }

    if (!name->value)
    {
	PushError ("HTML tag EDEF: Missing value for argument NAME");
	return 0;
    }

    if (!begin)
    {
	PushError ("HTML tag EDEF: Missing argument BEGIN");
	return 0;
    }

    if (!end)
    {
	PushError ("HTML tag EDEF: Missing argument END");
	return 0;
    }

    env = new (HTMLEnv);

    env->node.name = stripquotes (name->value);
    env->begin	   = begin->value ? stripquotes (begin->value) : NULL;
    env->end	   = end->value   ? stripquotes (end->value) : NULL;

    AddTail (&EDefs, env);

    return 1;
}

static int HTML_OtherTag (HTMLTag * tag)
{
    void       * aptr;
    HTMLTagArg * arg;
    char       * name;
    int 	 endtag;

    name = tag->node.name;

    if (*name == '/')
    {
	endtag = 1;
	name ++;
    }
    else
	endtag = 0;

/* printf ("OtherTag name=%s end=%d\n", name, endtag); */

    if ((aptr = FindNode (&Defs, name)))
    {
	HTMLMacro * macro = (HTMLMacro *)aptr;

printf ("Found as DEF\n");

	fputs (macro->body->buffer, stdout);
    }
    else if ((aptr = FindNode (&EDefs, name)))
    {
	HTMLEnv * env = (HTMLEnv *)aptr;

/* printf ("Found as EDEF\n"); */

	if (!endtag)
	{
	    if (env->begin)
		fputs (env->begin, stdout);
	}
	else
	{
	    if (env->end)
		fputs (env->end, stdout);
	}
    }
    else
    {
/* printf ("Found as normal tag\n"); */
	printf ("<%s", tag->node.name);

	ForeachNode (&tag->args, arg)
	{
	    if (arg->value)
		printf (" %s=%s", arg->node.name, arg->value);
	    else
		printf (" %s", arg->node.name);
	}

	printf (">");

    }

    return 1;
}

void
HTML_Init (void)
{
    HTML_InitParse ();

    NewList (&Defs);
    NewList (&EDefs);
}

void
HTML_Exit (void)
{
}

int
HTML_Parse (CB getc, void * stream, CBD data)
{
    int    token;
    String str;

    str = VS_New (NULL);

    while ((token = HTML_ScanText (str, StdioGetCharCB, stream, NULL)) != EOF)
    {
	/* printf ("%d: %s\n", token, str->buffer); */

	switch (token)
	{
	case T_TEXT:
	    fputs (str->buffer, stdout);
	    break;

	case T_HTML_TAG:
	    {
		StringStream * strs = StrStr_New (str->buffer);
		HTMLTag * tag;

		tag = HTML_ParseTag (StringGetCharCB, strs, NULL);

		/* HTML_PrintTag (tag); */

		if (!strcmp (tag->node.name, "DEF"))
		{
		    if (!HTML_DEF (tag, getc, stream, data))
		    {
			PushError ("HTML_Parse() failed in DEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "EDEF"))
		{
		    if (!HTML_EDEF (tag))
		    {
			PushError ("HTML_Parse() failed in EDEF");
			return T_ERROR;
		    }
		}
		else
		{
		    if (!HTML_OtherTag (tag))
		    {
			PushError ("HTML_Parse() failed in %s", tag->node.name);
			return T_ERROR;
		    }
		}

		HTML_FreeTag (tag);
		StrStr_Delete (strs);
	    }
	    break;

	case T_ERROR:
	    PushError ("HTML_Parse() failed");
	    return T_ERROR;

	default:
	    PushError ("HTML_Parse(): Unknown token %d\n", token);
	    return T_ERROR;
	}
    } /* while */

    return T_OK;
}

