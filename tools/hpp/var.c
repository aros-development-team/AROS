#include <ctype.h>
#include <toollib/error.h>
#include "var.h"

static List globals;
static List functions;

void
Var_Init (void)
{
    NewList (&globals);
    NewList (&functions);
}

void
Var_Exit (void)
{
    VarLevel * level, * next;

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
    cfree (var->value);
    xfree (var->node.name);
    xfree (var);
}

void
Var_Set (const char * name, const char * value)
{
    Var * var;

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
	ForeachNode (&level->vars, var)
	{
	    if (!strcmp (var->node.name, name))
		return var;
	}
    }

    return NULL;
}

void
Var_PushLevel (void)
{
    VarLevel * level = new (VarLevel);

    NewList (&level->vars);
    AddHead (&globals, level);
}

VarLevel *
Var_PopLevel (void)
{
    return (VarLevel *) RemHead (&globals);
}

String
Var_Subst (const char * in)
{
    String result;
    String varname;
    String args = NULL;
    String ret;

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

	    if (*in == '{')
	    {
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
	    }
	    else
	    {
		while (!isspace (*in) && *in)
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
	    }
	    else
		in = end; /* Don't skip spaces if no function */

	    if (args)
	    {
		Function * f = Func_Find (varname->buffer);

		if (!f)
		{
		    PushError ("Unknown function %s()", varname->buffer);
		    return NULL;
		}

		ret = (String) CallCB (f->cb, args->buffer, 0, f->cbd);

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
    return (Function *) FindNode (&functions, name);
}
