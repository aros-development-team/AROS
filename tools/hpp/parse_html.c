#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "parse_html.h"
#include "parse.h"
#include <toollib/hash.h>
#include <toollib/error.h>

static Hash * HTMLAmpDB;

#if 0
static int HTML_ScanAmp PARAMS ((String buffer, MyStream * stream, CBD data));

static int
HTML_ScanAmp (String buffer, MyStream * stream, CBD data)
{
    int c;

    VS_Clear (buffer);

    c = Str_Get (stream, data);

    if (c == EOF)
    {
	Str_PushError (stream, "Unexpected EOF while parsing &");
	return T_ERROR;
    }
    else if (c == '#')
    {
	int val;

	c = Str_Get (stream, data);
	if (c == EOF)
	{
err_amphash:
	    Str_PushError (stream, "Unexpected EOF while parsing &#");
	    return T_ERROR;
	}
	val = hexval (c);
	c = Str_Get (stream, data);
	if (c == EOF)
	    goto err_amphash;
	val = val * 16 + hexval (c);
	c = Str_Get (stream, data);
	if (c != ';')
	{
	    Str_PushError (stream, "Missing ; after &#");
	    return T_ERROR;
	}

	VS_AppendChar (buffer, val);
    }
    else
    {
	String str2 = VS_New (NULL);
	char * ptr;

	VS_AppendChar (str2, c);

	while ((c = Str_Get (stream, data)) != EOF)
	{
	    if (c == ';')
		break;
	    else if (str2->len > 10)
	    {
		Str_PushError (stream, "Unknown &-sequence: \"%s...\"", str2->buffer);
		return T_ERROR;
	    }

	    VS_AppendChar (str2, c);
	}

	ptr = Hash_FindNC (HTMLAmpDB, str2->buffer);

	if (!ptr)
	{
	    Str_PushError (stream, "Unknown &-sequence: \"%s\"", str2->buffer);
	    VS_Delete (str2);
	    return T_ERROR;
	}

	VS_Delete (str2);
	VS_AppendString (buffer, ptr);
    }

    return T_OK;
}
#endif

void HTML_InitParse (void)
{
    HTMLAmpDB = Hash_New ();

    Hash_StoreNC (HTMLAmpDB, "lt", "<");
    Hash_StoreNC (HTMLAmpDB, "gt", ">");
    Hash_StoreNC (HTMLAmpDB, "amp", "&");
    Hash_StoreNC (HTMLAmpDB, "quot", "\"");
    Hash_StoreNC (HTMLAmpDB, "auml", "ä");
    Hash_StoreNC (HTMLAmpDB, "ouml", "ö");
    Hash_StoreNC (HTMLAmpDB, "uuml", "ü");
    Hash_StoreNC (HTMLAmpDB, "Auml", "ä");
    Hash_StoreNC (HTMLAmpDB, "Ouml", "ö");
    Hash_StoreNC (HTMLAmpDB, "Uuml", "ü");
    Hash_StoreNC (HTMLAmpDB, "sz", "ß");
}

int
HTML_Get (MyStream * stream, CBD data)
{
    int c;

    c = Str_Get (stream, data);

    if (isspace (c))
    {
	while (isspace (c))
	{
	    c = Str_Get (stream, data);
	}

	Str_Unget (stream, c, data);
	c = ' ';
    }

    return c;
}

int
HTML_ScanText (String buffer, MyStream * stream, CBD data)
{
    int    c;
    int    line;
    char * ptr;

again:
    line = Str_GetLine (stream);

    VS_Clear (buffer);

    c = Str_Get (stream, data);

    if (c == EOF)
	return EOF;

    if (c == '<')
    {
	c = HTML_Get (stream, data);

	if (isspace (c))
	    c = HTML_Get (stream, data);
	else if (c == EOF)
	{
	    Str_SetLine (stream, line);
	    Str_PushError (stream, "Unexpected EOF while parsing HTML tag");
	    return T_ERROR;
	}

	do
	{
	    if (c == '>')
		break;

	    if (c == '"')
	    {
		VS_AppendChar (buffer, c);

		while ((c = Str_Get (stream, data)) != EOF && c != '"')
		    VS_AppendChar (buffer, c);
	    }

	    VS_AppendChar (buffer, c);
	}
	while ((c = HTML_Get (stream, data)) != EOF);

	if (c != '>')
	{
	    Str_SetLine (stream, line);
	    Str_PushError (stream, "Unexpected EOF while parsing HTML tag");
	    return T_ERROR;
	}

#if 0
    printf ("Tag=\"%s\"\n", buffer->buffer);
#endif

	ptr=buffer->buffer;

	if (!strncasecmp (ptr, "REM", 3))
	{
	    ptr += 3;

	    while (isspace (*ptr)) ptr ++;

	    if (*ptr)
		goto again;
	    else
	    {
		String rem = HTML_ReadBody (stream, data, "REM", 1);

		if (rem)
		{
		    VS_Delete (rem);
		    goto again;
		}
		else
		{
		    Str_SetLine (stream, line);
		    Str_PushError (stream, "Unexpected EOF while reading comment");
		}
	    }
	}

	return T_HTML_TAG;
    }

    VS_AppendChar (buffer, c);

    while ((c = Str_Get (stream, data)) != EOF)
    {
	if (c == '<')
	{
	    Str_Unget (stream, c, data);
	    break;
	}
#if 0
	else if (c == '&')
	{
	    String str = VS_New (NULL);
	    int    rc;

	    rc = HTML_ScanAmp (str, stream, data);

	    if (rc == T_ERROR)
		return T_ERROR;

	    VS_AppendString (buffer, str->buffer);

	    VS_Delete (str);
	}
#endif
	else
	    VS_AppendChar (buffer, c);
    }

    return T_TEXT;
}

HTMLTag *
HTML_ParseTag (MyStream * stream, CBD data)
{
    HTMLTag    * tag;
    HTMLTagArg * arg;
    String	 name  = VS_New (NULL);
    String	 value = VS_New (NULL);
    int 	 c;

    c = HTML_Get (stream, data);

    if (isspace (c))
	c = HTML_Get (stream, data);

    if (c != EOF)
    {
	do
	{
	    if (isspace (c))
		break;

	    VS_AppendChar (name, c);
	}
	while ((c = HTML_Get (stream, data)) != EOF);
    }
    else
    {
	Str_PushError (stream, "Unexpected EOF while parsing HTML tag name");
	return NULL;
    }

    tag = new (HTMLTag);
    NewList (&tag->args);

    VS_ToUpper (name);

    tag->node.name = xstrdup (name->buffer);

#if 0
    printf ("Tag2=\"%s\"\n", tag->node.name);
#endif

    while ((c = HTML_Get (stream, data)) != EOF)
    {
	if (isspace (c))
	    continue;
	else
	{
	    VS_Clear (name);
	    VS_Clear (value);
	    VS_AppendChar (name, c);

	    while ((c = HTML_Get (stream, data)) != EOF)
	    {
		if (isspace (c) || c == '=')
		    break;

		VS_AppendChar (name, c);
	    }

	    if (isspace (c))
		c = HTML_Get (stream, data);

	    if (c == '=')
	    {
		c = HTML_Get (stream, data);
		if (c == EOF)
		{
		    Str_PushError (stream, "Unexpected EOF while parsing argument for HTML tag %s", tag->node.name);
		    return NULL;
		}

		do
		{
		    if (isspace (c))
			break;
		    else if (c == '"')
		    {
			do
			{
			    VS_AppendChar (value, c);
			}
			while ((c = HTML_Get (stream, data)) != EOF && c != '"');
		    }

		    VS_AppendChar (value, c);
		}
		while ((c = HTML_Get (stream, data)) != EOF);
	    }
	    else
		Str_Unget (stream, c, data);

#if 0
    if (value->len)
	printf ("{%s}={%s}\n", name->buffer, value->buffer);
    else
	printf ("{%s}\n", name->buffer);
#endif

	    arg = new (HTMLTagArg);

	    arg->node.name = xstrdup (name->buffer);
	    strupper (arg->node.name);

	    if (value->len)
		arg->value = stripquotes (value->buffer);
	    else
		arg->value = NULL;

	    AddTail (&tag->args, arg);
	}
    }

    return tag;
}

void
HTML_FreeTag (HTMLTag * tag)
{
    HTMLTagArg * arg, * next;

    assert (tag);

    ForeachNodeSafe (&tag->args, arg, next)
    {
	Remove (arg);

	cfree (arg->value);
	xfree (arg->node.name);
	xfree (arg);
    }

    xfree (tag->node.name);
    xfree (tag);
}

void
HTML_PrintTag (HTMLTag * tag)
{
    HTMLTagArg * arg;

    assert (tag);

    printf ("HTMLTag %s\n", tag->node.name);

    ForeachNode (&tag->args, arg)
    {
	if (arg->value)
	    printf ("    ARG %s=%s\n", arg->node.name, arg->value);
	else
	    printf ("    ARG %s\n", arg->node.name);
    }
}

String
HTML_ReadBody (MyStream * stream, CBD data, const char * name, int allowNest)
{
    String str	 = VS_New (NULL);
    int    start = 0;
    int    level = 0;
    int    paren = 0;
    int    c;
    int    mode;
    int    line = Str_GetLine (stream);
    int    namelen;

    mode  = 0;

    c = Str_Get (stream, data);

    if (c == EOF)
	return str;

    if (c != '\n')
	VS_AppendChar (str, c);

    namelen = strlen (name);

    while ((c = Str_Get (stream, data)) != EOF)
    {
	if (c == '<')
	{
	    mode = 1;
	    paren = str->len;
	}
	else if (mode == 1 && !isspace (c))
	{
	    mode = 0;
	    start = str->len;
	}

	if (c == '>')
	{
	    if (!strncasecmp (str->buffer+start, name, namelen)
		&& (isspace (str->buffer[start+namelen]) || str->buffer[start+namelen] == 0)
	    )
	    {
		if (!allowNest)
		{
		    Str_PushError (stream, "HTML tag %s: Nested %s", name, name);
		    VS_Delete (str);
		    return NULL;
		}
		else
		{
		    level ++;
		}
	    }
	    else if (str->buffer[start] == '/'
		&& !strncasecmp (str->buffer+start+1, name, namelen)
		&& (isspace (str->buffer[start+namelen+1]) || str->buffer[start+namelen+1] == 0)
	    )
	    {
		if (level == 0)
		{
		    if (start)
			start --;

		    str->len = paren;
		    str->buffer[paren] = 0;
		    break;
		}
		else
		    level --;
	    }
	}

	VS_AppendChar (str, c);
    }

    if (level)
    {
	Str_SetLine (stream, line);
	Str_PushError (stream, "Unexpected EOF while reading body of %s\n", name);
	VS_Delete (str);
	return NULL;
    }

#if 0
    printf ("Body={%s}\n", str->buffer);
#endif

    return str;
}

#ifdef TEST

#include <stdio.h>
#include <stringcb.h>

void main (int argc, char ** argv)
{
    int 	  t;
    int 	  token;
    String	  str;
    StdioStream * ss;

    str = VS_New (NULL);

    for (t=1; t<argc; t++)
    {
	ss = StdStr_New (argv[t], "r");

	if (!ss)
	    PrintErrorStack ();
	else
	{
	    while ((token = HTML_ScanText (str, ss, NULL)) != EOF)
	    {
		printf ("%d: %s\n", token, str->buffer);

		switch (token)
		{
		case T_HTML_TAG:
		    {
			StringStream * strs = StrStr_New (str->buffer);
			HTMLTag * tag;

			tag = HTML_ParseTag (strs, NULL);

			HTML_PrintTag (tag);
			HTML_FreeTag (tag);
			StrStr_Delete (strs);
		    }
		    break;

		case T_ERROR:
		    Str_PushError (ss, "Error in HTML_ScanText()");
		    PrintErrorStack ();
		    break;
		}
	    } /* while */

	    StdStr_Delete (ss);
	} /* if (fh) */
    } /* for all args */

    ErrorExit (0);
}

#endif /* TEST */
