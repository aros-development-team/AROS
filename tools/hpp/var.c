#include "var.h"

static List globals;

void
Var_Init (void)
{
    NewList (&globals);
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

