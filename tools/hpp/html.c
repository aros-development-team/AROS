#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <toollib/toollib.h>
#include <toollib/mystream.h>
#include <toollib/stringcb.h>
#include <toollib/error.h>
#include "parse.h"
#include "parse_html.h"
#include "var.h"
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

static int HTML_IF	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_DEF	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_BDEF	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_EDEF	 PARAMS ((HTMLTag * tag));
static int HTML_OtherTag PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));

static int
HTML_IF (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    String	 body;
    HTMLTagArg * condarg;
    String	 condstr;
    int 	 condvalue;
    char       * elseptr;

    body = HTML_ReadBody (in, data, "IF", 1);

    if (!body)
    {
	PushError ("Can't find body to IF");
	return 0;
    }

    condarg = (HTMLTagArg *) FindNode (&tag->args, "COND");

    if (!condarg)
    {
	PushError ("Can't find condition for IF");
	return 0;
    }

    if (!condarg->value)
	condvalue = 0;
    else
    {
	char * ptr;

	condstr = Var_Subst (condarg->value);

	if (!condstr)
	{
	    PushError ("Can't expand condition for IF");
	    return 0;
	}

	ptr = condstr->buffer;

	while (isspace (*ptr)) ptr ++;

	if (isdigit (*ptr))
	    condvalue = (atoi (ptr) != 0);
	else
	    condvalue = (*ptr != 0);
    }

    elseptr = body->buffer;

    while (*elseptr)
    {
	if (*elseptr == '<' && !strcasecmp (elseptr+1, "ELSE"))
	    break;
    }

    if (!*elseptr)
	elseptr = NULL;

    if (condvalue)
    {
	if (elseptr)
	    *elseptr = 0;

	Str_Puts (out, body->buffer, data);
    }
    else if (elseptr)
    {
	while (*elseptr && *elseptr != '>') elseptr ++;

	Str_Puts (out, elseptr, data);
    }

    return 1;
}

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
HTML_DEF (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    HTMLMacro  * old,
	       * macro;
    HTMLTagArg * name,
	       * arg;
    HTMLOptArg * option;

    if (FindNode (&BDefs, tag->node.name))
    {
	PushError ("There is already a block with the same name %s", tag->node.name);
	return 0;
    }

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

    macro->body = HTML_ReadBody (in, data, "DEF", 0);

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
HTML_BDEF (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    HTMLBlock  * old,
	       * block;
    HTMLTagArg * name,
	       * arg;
    HTMLOptArg * option;

    if (FindNode (&Defs, tag->node.name))
    {
	PushError ("There is already a macro with the same name %s", tag->node.name);
	return 0;
    }

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

    block->body = HTML_ReadBody (in, data, "BDEF", 0);

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
HTML_OtherTag (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    HTMLMacro  * macro;
    HTMLBlock  * block;
    HTMLEnv    * env;
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

    if (endtag && !IsListEmpty (&tag->args))
    {
	Warn ("Unexpected arguments in %s\n", tag->node.name);
    }

    env = (HTMLEnv *) FindNode (&EDefs, name);
    macro = (HTMLMacro *) FindNode (&Defs, name);
    block = (HTMLBlock *) FindNode (&BDefs, name);

    if (env && !IsListEmpty (&tag->args) && !macro && !block)
    {
	Warn ("Unexpected arguments to ENV %s\n", name);
    }

    if (env && !IsListEmpty (&tag->args) && (macro || block))
	env = NULL;

    if (env)
    {
	if (!endtag)
	{
	    if (env->begin)
		Str_Puts (out, env->begin, data);
	}
	else
	{
	    if (env->end)
		Str_Puts (out, env->end, data);
	}
    }
    else if (macro)
    {
	printf ("Found as DEF:");
	Str_Puts (out, macro->body->buffer, data);
    }
    else if (block)
    {
	String body;

	body = HTML_ReadBody (in, data, block->node.name, 1);

printf ("Found as BDEF.Def=%s\nBody=%s\n",
	block->body->buffer,
	body->buffer
);

	VS_Delete (body);
    }
    else
    {
/* printf ("Found as normal tag\n"); */
	Str_Put (out, '<', data);
	Str_Puts (out, tag->node.name, data);

	ForeachNode (&tag->args, arg)
	{
	    Str_Put (out, ' ', data);
	    Str_Puts (out, arg->node.name, data);

	    if (arg->value)
	    {
		Str_Put (out, '=', data);
		Str_Puts (out, arg->value, data);
	    }
	}

	Str_Put (out, '>', data);
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
HTML_Parse (MyStream * in, MyStream * out, CBD data)
{
    int    token;
    String str;

    str = VS_New (NULL);

    while ((token = HTML_ScanText (str, in, NULL)) != EOF)
    {
	/* printf ("%d: %s\n", token, str->buffer); */

	switch (token)
	{
	case T_TEXT:
	    Str_Puts (out, str->buffer, data);
	    break;

	case T_HTML_TAG:
	    {
		StringStream * strs = StrStr_New (str->buffer);
		HTMLTag * tag;

		tag = HTML_ParseTag ((MyStream *)strs, NULL);

		/* HTML_PrintTag (tag); */

		if (!strcmp (tag->node.name, "DEF"))
		{
		    if (!HTML_DEF (tag, in, out, data))
		    {
			PushError ("HTML_Parse() failed in DEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "BDEF"))
		{
		    if (!HTML_BDEF (tag, in, out, data))
		    {
			PushError ("HTML_Parse() failed in BDEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "IF"))
		{
		    if (!HTML_IF (tag, in, out, data))
		    {
			PushError ("HTML_Parse() failed in IF");
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
		    if (!HTML_OtherTag (tag, in, out, data))
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

