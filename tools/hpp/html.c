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
    int    expanding;
}
HTMLMacro;

typedef struct
{
    Node   node;
    List   args; /* HTMLOptArg */
    String body;
    int    expanding;
}
HTMLBlock;

#define MAX_ENVSP   32
static const char * ENVEnd[MAX_ENVSP];
static int	    ENVEndSP = MAX_ENVSP;

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
static int HTML_ENV	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_DEF	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_BDEF	 PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));
static int HTML_EDEF	 PARAMS ((HTMLTag * tag));
static int HTML_OtherTag PARAMS ((HTMLTag * tag, MyStream * in, MyStream * out, CBD data));

static int
HTML_IF (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    String	   body;
    HTMLTagArg	 * condarg;
    String	   condstr;
    int 	   condvalue;
    char	 * elseptr;
    char	 * str;
    int 	   line = Str_GetLine (in);

    body = HTML_ReadBody (in, data, "IF", 1);

    if (!body)
    {
	Str_SetLine (in, line);
	Str_PushError (in, "Can't find body to IF");
	return 0;
    }

    condarg = (HTMLTagArg *) FindNode (&tag->args, "COND");

    if (!condarg)
    {
	Str_SetLine (in, line);
	Str_PushError (in, "Can't find condition for IF");
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
	    Str_SetLine (in, line);
	    Str_PushError (in, "Can't expand condition for IF");
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
	if (*elseptr == '<')
	{
	    elseptr ++;

	    while (isspace (*elseptr)) elseptr ++;

	    if (!strncasecmp (elseptr, "ELSE", 4))
	    {
		elseptr += 4;

		while (isspace (*elseptr)) elseptr ++;

		if (*elseptr == '>')
		    break;
	    }
	}

	elseptr ++;
    }

    if (!*elseptr)
	elseptr = NULL;

    if (condvalue)
    {
	if (elseptr)
	{
	    while (*elseptr != '<') elseptr --;

	    *elseptr = 0;
	}

	str = body->buffer;
    }
    else if (elseptr)
    {
	while (*elseptr && *elseptr != '>') elseptr ++;

	if (*elseptr)
	    elseptr ++;

	str = elseptr;
    }
    else
	str = NULL;

#if 0
    printf ("condvalue=%d elseptr=\"%s\" out=\"%s\"\n", condvalue,
	elseptr ? elseptr : "(nil)",
	str ? str : "(nil)");
#endif

    if (str)
    {
	String	       vstr;
	StringStream * ss;
	int	       rc;

	vstr = Var_Subst (str);

	if (!vstr)
	{
	    Str_SetLine (in, line);
	    Str_PushError (in, "Error expanding IF body");
	    return T_ERROR;
	}

#if 0
    printf ("Parse \"%s\"\n", vstr->buffer);
#endif

	ss = StrStr_New (vstr->buffer);

	rc = HTML_Parse ((MyStream *)ss, out, data);

	StrStr_Delete (ss);
	VS_Delete (vstr);

	return rc;
    }

    return T_OK;
}

static int
HTML_ENV (HTMLTag * tag, MyStream * in, MyStream * out, CBD data)
{
    HTMLEnv    * env;
    HTMLTagArg * name;
    const char * str = NULL;
    int 	 endtag;

    endtag = (tag->node.name[0] == '/');

    if (!endtag)
    {
	name = (HTMLTagArg *) FindNodeNC (&tag->args, "NAME");

	if (!name)
	{
	    Str_PushError (in, "Missing argument NAME in ENV");
	    return T_ERROR;
	}

	if (!name->value)
	{
	    Str_PushError (in, "Missing value for argument NAME in ENV");
	    return T_ERROR;
	}

	env = (HTMLEnv *) FindNodeNC (&EDefs, name->value);

	if (!env)
	{
	    Str_PushError (in, "Unknown environment %s", name->value);
	    return T_ERROR;
	}

	if (!ENVEndSP)
	{
	    Str_PushError (in, "ENVs nested too deep");
	    return T_ERROR;
	}

	ENVEnd[--ENVEndSP] = env->end;

	str = env->begin;
    }
    else
    {
	if (ENVEndSP == MAX_ENVSP)
	{
	    Str_PushError (in, "</ENV> without <ENV>");
	    return T_ERROR;
	}

	str = ENVEnd[ENVEndSP++];
    }

    if (str)
    {
	int rc;

	StringStream * ss = StrStr_New (str);

	rc = HTML_Parse ((MyStream *)ss, out, data);

	StrStr_Delete (ss);

	return rc;
    }

    return T_OK;
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
	Str_PushError (in, "There is already a block with the same name %s", tag->node.name);
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
	Str_PushError (in, "HTML tag DEF: Missing argument NAME");
	return 0;
    }

    if (!name->value)
    {
	Str_PushError (in, "HTML tag DEF: Missing value for argument NAME");
	return 0;
    }

    macro = new (HTMLMacro);

    macro->node.name = xstrdup (name->value);
    strupper (macro->node.name);
    NewList (&macro->args);
    macro->expanding = 0;

    option = NULL;

    ForeachNode (&tag->args, arg)
    {
	if (!strcmp (arg->node.name, "OPTION"))
	{
	    if (!arg->value)
	    {
		Str_PushError (in, "HTML tag DEF: Missing value for argument OPTION");
		return 0;
	    }

	    option = new (HTMLOptArg);

	    option->node.name = xstrdup (arg->value);
	    option->defvalue = NULL;
	    strupper (option->node.name);
	    AddTail (&macro->args, option);
	}
	else if (!strcmp (arg->node.name, "DEFAULT"))
	{
	    if (!arg->value)
	    {
		Str_PushError (in, "HTML tag DEF: Missing value for argument DEFAULT");
		return 0;
	    }

	    if (!option)
	    {
		Str_PushError (in, "HTML tag DEF: Missing OPTION for DEFAULT");
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
	Str_PushError (in, "There is already a macro with the same name %s", tag->node.name);
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
	Str_PushError (in, "HTML tag DEF: Missing argument NAME");
	return 0;
    }

    if (!name->value)
    {
	Str_PushError (in, "HTML tag DEF: Missing value for argument NAME");
	return 0;
    }

    block = new (HTMLBlock);

    block->node.name = xstrdup (name->value);
    strupper (block->node.name);
    NewList (&block->args);
    block->expanding = 0;

    option = NULL;

    ForeachNode (&tag->args, arg)
    {
	if (!strcmp (arg->node.name, "OPTION"))
	{
	    if (!arg->value)
	    {
		Str_PushError (in, "HTML tag DEF: Missing value for argument OPTION");
		return 0;
	    }

	    option = new (HTMLOptArg);

	    option->node.name = xstrdup (arg->value);
	    option->defvalue = NULL;
	    strupper (option->node.name);
	    AddTail (&block->args, option);
	}
	else if (!strcmp (arg->node.name, "DEFAULT"))
	{
	    if (!arg->value)
	    {
		Str_PushError (in, "HTML tag DEF: Missing value for argument DEFAULT");
		return 0;
	    }

	    if (!option)
	    {
		Str_PushError (in, "HTML tag DEF: Missing OPTION for DEFAULT");
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

    env->node.name = xstrdup (name->value);
    env->begin	   = begin->value ? xstrdup (begin->value) : NULL;
    env->end	   = end->value   ? xstrdup (end->value) : NULL;

    strupper (env->node.name);

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
    int 	 rc = T_OK;

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
	char * str = NULL;

	if (!endtag)
	    str = env->begin;
	else
	    str = env->end;

	if (str)
	{
	    StringStream * ss = StrStr_New (str);

	    rc = HTML_Parse ((MyStream *)ss, out, data);

	    StrStr_Delete (ss);
	}
    }
    else if (macro)
    {
	if (macro->expanding)
	{
	    Str_PushError (in, "Loop in expansion of macro %s found", macro->node.name);
	    return T_ERROR;
	}

	if (macro->body->buffer)
	{
	    StringStream * ss;

	    if (!IsListEmpty (&macro->args))
	    {
		HTMLOptArg * oarg;

		Var_PushLevel ();

		ForeachNode (&macro->args, oarg)
		{
		    arg = (HTMLTagArg *) FindNodeNC (&tag->args, oarg->node.name);

		    if (!arg)
		    {
			if (!oarg->defvalue)
			    Warn ("Missing value for DEF argument %s\n", oarg->node.name);
			else
			    Var_Set (oarg->node.name, oarg->defvalue);
		    }
		    else
			Var_Set (oarg->node.name, arg->value);
		}
	    }

	    ss = StrStr_New (macro->body->buffer);

	    macro->expanding = 1;
	    rc = HTML_Parse ((MyStream *)ss, out, data);
	    macro->expanding = 0;

	    if (rc != T_OK)
	    {
		Str_PushError (in, "Error in expanding macro %s", macro->node.name);
	    }

	    StrStr_Delete (ss);

	    if (!IsListEmpty (&macro->args))
	    {
		Var_FreeLevel (Var_PopLevel ());
	    }
	}
    }
    else if (block)
    {
	if (block->expanding)
	{
	    Str_PushError (in, "Loop in expansion of block %s found", block->node.name);
	    return T_ERROR;
	}

	if (block->body->buffer)
	{
	    StringStream * ss;
	    String body;
	    HTMLOptArg * oarg;

	    Var_PushLevel ();

	    body = HTML_ReadBody (in, data, block->node.name, 1);

	    Var_SetConst ("@body", body->buffer);

	    ForeachNode (&block->args, oarg)
	    {
		arg = (HTMLTagArg *) FindNodeNC (&tag->args, oarg->node.name);

		if (!arg)
		{
		    if (!oarg->defvalue)
			Warn ("Missing value for BDEF argument %s\n", oarg->node.name);
		    else
		    {
			Var_Set (oarg->node.name, oarg->defvalue);
		    }
		}
		else
		{
		    Var_Set (oarg->node.name, arg->value);
		}
	    }

#if 0
printf ("Found as BDEF.Def=%s\nBody=%s\n",
	block->body->buffer,
	body->buffer
);
#endif

	    ss = StrStr_New (block->body->buffer);

	    block->expanding = 1;
	    rc = HTML_Parse ((MyStream *)ss, out, data);
	    block->expanding = 0;

	    if (rc != T_OK)
	    {
		Str_PushError (in, "Error in expanding block %s", block->node.name);
	    }

	    StrStr_Delete (ss);

	    Var_FreeLevel (Var_PopLevel ());
	    VS_Delete (body);
	}
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

    return rc;
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
    if (ENVEndSP != MAX_ENVSP)
    {
	Error ("Some <ENV>'s were not closed");
    }
}

int
HTML_Parse (MyStream * in, MyStream * out, CBD data)
{
    int    line;
    int    token;
    String str;

    str = VS_New (NULL);

    while ((token = HTML_ScanText (str, in, NULL)) != EOF)
    {
	/* printf ("%d: %s\n", token, str->buffer); */

	line = Str_GetLine (in);

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

#if 0
    HTML_PrintTag (tag);
#endif

		if (!strcmp (tag->node.name, "DEF"))
		{
		    if (!HTML_DEF (tag, in, out, data))
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in DEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "BDEF"))
		{
		    if (!HTML_BDEF (tag, in, out, data))
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in BDEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "IF"))
		{
		    if (HTML_IF (tag, in, out, data) != T_OK)
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in IF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "EDEF"))
		{
		    if (!HTML_EDEF (tag))
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in EDEF");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "FILTER"))
		{
		    if (HTML_Filter (tag, in, out, data) != T_OK)
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in FILTER");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "ENV")
		    || !strcmp (tag->node.name, "/ENV"))
		{
		    if (HTML_ENV (tag, in, out, data) != T_OK)
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in ENV");
			return T_ERROR;
		    }
		}
		else if (!strcmp (tag->node.name, "LINK"))
		{
		    HTMLTagArg * name, * ref;

		    name = (HTMLTagArg *) FindNodeNC (&tag->args, "NAME");
		    ref  = (HTMLTagArg *) FindNodeNC (&tag->args, "REF");

		    if (!name)
		    {
			Str_PushError (in, "Missing argument NAME in LINK");
			return T_ERROR;
		    }

		    if (!ref)
		    {
			Str_PushError (in, "Missing argument REF in LINK");
			return T_ERROR;
		    }

		    Str_Puts (out, "<A HREF=\"", data);
		    Str_Puts (out, ref->value, data);
		    Str_Puts (out, "\">", data);
		    Str_Puts (out, name->value, data);
		    Str_Puts (out, "</A>", data);
		}
		else
		{
		    if (HTML_OtherTag (tag, in, out, data) != T_OK)
		    {
			Str_SetLine (in, line);
			Str_PushError (in, "HTML_Parse() failed in %s", tag->node.name);
			return T_ERROR;
		    }
		}

		HTML_FreeTag (tag);
		StrStr_Delete (strs);
	    }
	    break;

	case T_ERROR:
	    Str_SetLine (in, line);
	    Str_PushError (in, "HTML_Parse() failed");
	    return T_ERROR;

	default:
	    Str_SetLine (in, line);
	    Str_PushError (in, "HTML_Parse(): Unknown token %d\n", token);
	    return T_ERROR;
	}
    } /* while */

    return T_OK;
}

