#include <ctype.h>
#include <stdio.h>
#include <toollib/error.h>
#include "var.h"
#include "parse.h"

#define DEBUG_SET	0
#define DEBUG_LEVEL	0

static List globals;
static List functions;

static String DefinedCB (const char ** args, int dummy, CBD data);
static String UpVarCB (const char ** args, int dummy, CBD data);

static String
DefinedCB (const char ** args, int dummy, CBD data)
{
    if (!args[0])
    {
	PushError ("$defined(): Expecting one arg");
	return NULL;
    }

    return VS_New ((Var_Find (args[0])) ? "1" : "0");
}

static String
UpVarCB (const char ** args, int dummy, CBD data)
{
    VarLevel * level;
    Var      * var;

    if (!args[0])
    {
	PushError ("$upvar(): Expecting one arg");
	return NULL;
    }

    level = GetHead (&globals);

    if (!level)
	return NULL;

    for (level=GetNext(level); level; level=GetNext(level))
    {
	var = (Var *) FindNodeNC (&level->vars, args[0]);

	if (var)
	    return VS_New (var->value);
    }

    PushError ("$upvar(): Can't find %s", args[0]);
    return NULL;
}

static int init = 0;

void
Var_Init (void)
{
    if (init)
	return;

    init = 1;

    NewList (&globals);
    NewList (&functions);

    Func_Add ("defined", (CB) DefinedCB, NULL);
    Func_Add ("upvar", (CB) UpVarCB, NULL);
}

void
Var_Exit (void)
{
    VarLevel * level, * next;

    if (!init)
	return;

    init = 0;

    ForeachNodeSafe (&globals, level, next)
    {
	Remove (level);
	Var_FreeLevel (level);
    }
}

void
Var_FreeLevel (VarLevel * level)
{
    Var * var, * next;

    ForeachNodeSafe (&level->vars, var, next)
    {
	Remove (var);
	Var_Free (var);
    }
}

void
Var_Free (Var * var)
{
    if (var->freevalue)
	cfree (var->value);

    xfree (var->node.name);
    xfree (var);
}

void
Var_Set (const char * name, const char * value)
{
    Var * var;

#if DEBUG_SET
    printf ("Set %s=%s\n", name, value?value:"");
#endif

    if ((var = Var_Find (name)))
    {
	setstr (var->value, value);
    }
    else
    {
	VarLevel * level = GetHead (&globals);

	if (!level)
	{
	    Var_PushLevel ();

	    level = GetHead (&globals);
	}

	var = new (Var);

	var->node.name = xstrdup (name);
	var->value     = cstrdup (value);
	var->freevalue = 0;

	AddTail (&level->vars, var);
    }
}

void
Var_SetLocal (const char * name, const char * value)
{
    VarLevel * level = GetHead (&globals);
    Var      * var;

    if (!level)
	Var_Set (name, value);
    else
    {
#if DEBUG_SET
    printf ("Set %s=%s\n", name, value?value:"");
#endif
	if ((var = (Var *) FindNodeNC (&level->vars, name)))
	{
	    setstr (var->value, value);
	}
	else
	{
	    var = new (Var);

	    var->node.name = xstrdup (name);
	    var->value	   = cstrdup (value);
	    var->freevalue = 0;

	    AddTail (&level->vars, var);
	}
    }
}

void
Var_SetConst (const char * name, const char * value)
{
    Var * var;

#if DEBUG_SET
    printf ("Set %s=%s\n", name, value?value:"");
#endif

    if ((var = Var_Find (name)))
    {
	setstr (var->value, value);
    }
    else
    {
	VarLevel * level = GetHead (&globals);

	if (!level)
	{
	    Var_PushLevel ();

	    level = GetHead (&globals);
	}

	var = new (Var);

	var->node.name = xstrdup (name);
	var->value     = (char *)value;
	var->freevalue = 0;

	AddTail (&level->vars, var);
    }
}

char *
Var_Get (const char * name)
{
    Var * var;

    if ((var = Var_Find (name)))
	return var->value;

    return NULL;
}

void
Var_UnSet (const char * name)
{
    Var * var;

    if ((var = Var_Find (name)))
    {
	Remove (var);
	Var_Free (var);
    }
}

Var *
Var_Find (const char * name)
{
    VarLevel * level;
    Var      * var;

    ForeachNode (&globals, level)
    {
	var = (Var *) FindNodeNC (&level->vars, name);

	if (var)
	    return var;
    }

    return NULL;
}

void
Var_PushLevel (void)
{
    VarLevel * level = new (VarLevel);

#if DEBUG_LEVEL
    printf ("Push\n");
#endif

    NewList (&level->vars);
    AddHead (&globals, level);
}

VarLevel *
Var_PopLevel (void)
{
#if DEBUG_LEVEL
    printf ("Pop\n");
#endif

    return (VarLevel *) RemHead (&globals);
}

String
Var_Subst (const char * in)
{
    String result;
    String varname;
    String args = NULL;
    String ret;
    int skipbrace;

    if (!in)
	return NULL;

    result = VS_New (NULL);

    while (*in)
    {
	if (*in == '$')
	{
	    const char * end;
	    in ++;

	    varname = VS_New (NULL);
	    skipbrace = 0;

	    if (*in == '{')
	    {
		in ++;

		while (*in && *in != '}')
		{
		    VS_AppendChar (varname, *in);
		    in ++;
		}

		if (*in != '}')
		{
		    PushError ("Missing '}' in varname");
		    return NULL;
		}

		skipbrace = 1;
	    }
	    else
	    {
		while (isalnum (*in) || *in == '_')
		{
		    VS_AppendChar (varname, *in);
		    in ++;
		}
	    }

	    end = in;

	    while (isspace (*in)) in ++;

	    if (*in == '(')
	    {
		int level = 0;

		in ++;
		args = VS_New (NULL);

		while (*in)
		{
		    if (*in == ')')
		    {
			if (!level)
			    break;
			else
			    level --;
		    }
		    else if (*in == '(')
		    {
			level ++;
		    }

		    VS_AppendChar (args, *in);
		    in ++;
		}

		if (*in != ')')
		{
		    PushError ("Missing ')' in parameters to function");
		    return NULL;
		}
		else
		    in ++; /* Skip ) */
	    }
	    else
		in = end; /* Don't skip spaces if no function */

	    if (skipbrace)
		in ++;
#if 0
    printf ("var=%s\nargs=%s\n", varname->buffer, args?args->buffer: "");
#endif

	    if (args)
	    {
		Function * f = Func_Find (varname->buffer);
		char ** argv;

		if (!f)
		{
		    PushError ("Unknown function %s()", varname->buffer);
		    return NULL;
		}

		argv = Func_SplitArgs (args->buffer);

		if (!argv)
		{
		    PushError ("Error reading arguments for function %s()", varname->buffer);
		    return NULL;
		}

		ret = (String) CallCB (f->cb, argv, 0, f->cbd);

		Func_FreeArgs (argv);

		if (!ret)
		{
		    PushError ("Error in function %s (%s)", varname->buffer, args->buffer);
		    return NULL;
		}

		VS_AppendString (result, ret->buffer);
		VS_Delete (ret);
		VS_Delete (args);
		args = NULL;
	    }
	    else
	    {
		Var * var = Var_Find (varname->buffer);

		if (!var)
		{
		    PushError ("Unknown variable \"%s\"", varname->buffer);
		    return NULL;
		}

		if (var->value)
		    VS_AppendString (result, var->value);
	    }

	    VS_Delete (varname);
	    varname = NULL;
	}
	else
	{
	    VS_AppendChar (result, *in);
	    in ++;
	}
    }

    return result;
}

void
Func_Add (const char * name, CB cb, CBD cbd)
{
    Function * f = new (Function);

    f->node.name = xstrdup (name);
    f->cb = cb;
    f->cbd = cbd;

    AddTail (&functions, f);
}

Function *
Func_Find (const char * name)
{
    return (Function *) FindNodeNC (&functions, name);
}

char **
Func_SplitArgs (const char * args)
{
    int t, n;
    char ** argv;
    String  arg, newarg;

    n = 0;

    if (*args)
	n++;

    for (t=0; args[t]; t++)
    {
	if (args[t]==',')
	    n ++;
    }

    argv = xmalloc (sizeof (char *) * (n+1));

    n = 0;

    if (*args)
    {
	arg = VS_New (NULL);

	for (t=0; args[t]; t++)
	{
	    if (args[t]==',')
	    {
		newarg = Var_Subst (arg->buffer);

		if (!newarg)
		{
		    return NULL;
		}

		argv[n++] = xstrdup (newarg->buffer);

		VS_Clear (newarg);
		VS_Clear (arg);
	    }
	    else
	    {
		VS_AppendChar (arg, args[t]);
	    }
	}

	newarg = Var_Subst (arg->buffer);

	if (!newarg)
	{
	    return NULL;
	}

	argv[n++] = newarg->buffer;

	newarg->buffer = NULL;
	VS_Delete (newarg);
	VS_Delete (arg);
    }

    argv[n] = NULL;

    return argv;
}

void
Func_FreeArgs (char ** argv)
{
    int t;

    for (t=0; argv[t]; t++)
	xfree (argv[t]);

    xfree (argv);
}

void
Var_PrintAll (void)
{
    VarLevel * level;
    Var      * var;

    ForeachNode (&globals, level)
    {
	printf ("----------------\n");

	ForeachNode (&level->vars, var)
	{
	    printf ("%s={%s}\n", var->node.name, var->value);
	}
    }
}
