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
    char * defvalue;
}
HTMLOptArg;

typedef struct
{
    Node   node;
    List   args; /* HTMLOptArg */
    String body;
}
HTMLMacro;

typedef struct
{
    Node   node;
    List   args; /* HTMLOptArg */
    String body;
}
HTMLBlock;

/* Definitions:

    Macros are texts which are replaced by other texts. The code between
    the <DEF></DEF> is parsed and inserted.

    Environments insert text before and after other text. Their contents
    is parsed before it is inserted.

    Blocks convert the text between the two tags. The code between the
    <BDEF></BDEF> is parsed and inserted.
*/
static List Defs;   /* Macros */
static List BDefs;  /* Blocks */
static List EDefs;  /* Environments */

static int HTML_DEF	 PARAMS ((HTMLTag * tag, CB getc, void * stream, CBD data));
static int HTML_BDEF	 PARAMS ((HTMLTag * tag, CB getc, void * stream, CBD data));
static int HTML_EDEF	 PARAMS ((HTMLTag * tag));
static int HTML_OtherTag PARAMS ((HTMLTag * tag, CB getc, void * stream, CBD data));

static void
HTML_FreeMacro (HTMLMacro * macro)
{
    HTMLOptArg * node, * next;

    VS_Delete (macro->body);

    ForeachNodeSafe (&macro->args, node, next)
    {
	Remove (node);

	cfree (node->defvalue);
	xfree (node->node.name);
	xfree (node);
    }

    xfree (macro->node.name);
    xfree (macro);
}

static int
HTML_DEF (HTMLTag * tag, CB getc, void * stream, CBD data)
{
    HTMLMacro  * old,
	       * macro;
    HTMLTagArg * name,
	       * arg;
    HTMLOptArg * option;

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

    option = NULL;

    ForeachNode (&tag->args, arg)
    {
	if (!strcmp (arg->node.name, "OPTION"))
	{
	    if (!arg->value)
	    {
		PushError ("HTML tag DEF: Missing value for argument OPTION");
		return 0;
	    }

	    option = new (HTMLOptArg);

	    option->node.name = stripquotes (arg->value);
	    option->defvalue = NULL;
	    strupper (option->node.name);
	    AddTail (&macro->args, option);
	}
	else if (!strcmp (arg->node.name, "DEFAULT"))
	{
	    if (!arg->value)
	    {
		PushError ("HTML tag DEF: Missing value for argument DEFAULT");
		return 0;
	    }

	    if (!option)
	    {
		PushError ("HTML tag DEF: Missing OPTION for DEFAULT");
		return 0;
	    }

	    if (option->defvalue)
	    {
		Warn ("HTML tag DEF: Overriding DEFAULT for OPTION \"%s\"", option->node.name);

		xfree (option->defvalue);
	    }

	    option->defvalue = xstrdup (arg->value);
	}
    }

    macro->body = HTML_ReadBody (getc, stream, data, "DEF", 0);

/* printf ("Value=%s\n", macro->body->buffer); */

    AddTail (&Defs, macro);

    return 1;
}

static void
HTML_FreeBlock (HTMLBlock * block)
{
    HTMLOptArg * node, * next;

    VS_Delete (block->body);

    ForeachNodeSafe (&block->args, node, next)
    {
	Remove (node);

	cfree (node->defvalue);
	xfree (node->node.name);
	xfree (node);
    }

    xfree (block->node.name);
    xfree (block);
}

static int
HTML_BDEF (HTMLTag * tag, CB getc, void * stream, CBD data)
{
    HTMLBlock  * old,
	       * block;
    HTMLTagArg * name,
	       * arg;
    HTMLOptArg * option;

    old = (HTMLBlock *) FindNode (&BDefs, tag->node.name);

    if (old)
    {
	Warn ("Block %s redefined\n", tag->node.name);

	Remove (old);
	HTML_FreeBlock (old);
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

    block = new (HTMLBlock);

    block->node.name = stripquotes (name->value);
    NewList (&block->args);

    option = NULL;

    ForeachNode (&tag->args, arg)
    {
	if (!strcmp (arg->node.name, "OPTION"))
	{
	    if (!arg->value)
	    {
		PushError ("HTML tag DEF: Missing value for argument OPTION");
		return 0;
	    }

	    option = new (HTMLOptArg);

	    option->node.name = stripquotes (arg->value);
	    option->defvalue = NULL;
	    strupper (option->node.name);
	    AddTail (&block->args, option);
	}
	else if (!strcmp (arg->node.name, "DEFAULT"))
	{
	    if (!arg->value)
	    {
		PushError ("HTML tag DEF: Missing value for argument DEFAULT");
		return 0;
	    }

	    if (!option)
	    {
		PushError ("HTML tag DEF: Missing OPTION for DEFAULT");
		return 0;
	    }

	    if (option->defvalue)
	    {
		Warn ("HTML tag DEF: Overriding DEFAULT for OPTION \"%s\"", option->node.name);

		xfree (option->defvalue);
	    }

	    option->defvalue = xstrdup (arg->value);
	}
    }

    block->body = HTML_ReadBody (getc, stream, data, "BDEF", 0);

/* printf ("Value=%s\n", block->body->buffer); */

    AddTail (&BDefs, block);

    return 1;
}

static void
HTML_FreeEnv (HTMLEnv * env)
{
    cfree (env->begin);
    cfree (env->end);
    xfree (env->node.name);
    xfree (env);
}

static int
HTML_EDEF (HTMLTag * tag)
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

static int
HTML_OtherTag (HTMLTag * tag, CB getc, void * stream, CBD data)
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
    else if ((aptr = FindNode (&BDefs, name)))
    {
	HTMLBlock * block = (HTMLBlock *)aptr;
	String body;

	body = HTML_ReadBody (getc, stream, data, block->node.name, 1);

printf ("Found as BDEF.Def=%s\nBody=%s\n",
	block->body->buffer,
	body->buffer
);

	VS_Delete (body);
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
    NewList (&BDefs);
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
		else if (!strcmp (tag->node.name, "BDEF"))
		{
		    if (!HTML_BDEF (tag, getc, stream, data))
		    {
			PushError ("HTML_Parse() failed in BDEF");
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
		    if (!HTML_OtherTag (tag, getc, stream, data))
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

