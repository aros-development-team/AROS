#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "parse_html.h"
#include "parse.h"
#include <toollib/hash.h>
#include <toollib/error.h>

#define M_EOWS	     0
#define M_SKIPQUOTE  1

static Hash * HTMLAmpDB;

static int HTML_ScanAmp PARAMS ((String buffer, MyStream * stream, CBD data));

void HTML_InitParse (void)
{
    HTMLAmpDB = Hash_New ();

    Hash_Store (HTMLAmpDB, "lt", "<");
    Hash_Store (HTMLAmpDB, "gt", ">");
    Hash_Store (HTMLAmpDB, "amp", "&");
    Hash_Store (HTMLAmpDB, "quot", "\"");
    Hash_Store (HTMLAmpDB, "auml", "ä");
    Hash_Store (HTMLAmpDB, "ouml", "ö");
    Hash_Store (HTMLAmpDB, "uuml", "ü");
    Hash_Store (HTMLAmpDB, "Auml", "ä");
    Hash_Store (HTMLAmpDB, "Ouml", "ö");
    Hash_Store (HTMLAmpDB, "Uuml", "ü");
    Hash_Store (HTMLAmpDB, "sz", "ß");
}

static int
HTML_ScanAmp (String buffer, MyStream * stream, CBD data)
{
    int c;

    VS_Clear (buffer);

    c = Str_Get (stream, data);

    if (c == EOF)
    {
	PushError ("Unexpected EOF while parsing &");
	return T_ERROR;
    }
    else if (c == '#')
    {
	int val;

	c = Str_Get (stream, data);
	if (c == EOF)
	{
err_amphash:
	    PushError ("Unexpected EOF while parsing &#");
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
	    PushError ("Missing ; after &#");
	    return T_ERROR;
	}

	VS_AppendChar (buffer, val);
    }
    else
    {
	String str2 = VS_New (NULL);
	char * ptr;

	while ((c = Str_Get (stream, data)) != EOF)
	{
	    if (c == ';')
		break;
	    else if (str2->len > 10)
	    {
		PushError ("Unknown &-sequence: \"%s...\"", str2->buffer);
		return T_ERROR;
	    }

	    VS_AppendChar (str2, c);
	}

	ptr = Hash_Find (HTMLAmpDB, str2->buffer);

	VS_Delete (str2);

	if (!ptr)
	{
	    PushError ("Unknown &-sequence: \"%s\"", str2->buffer);
	    return T_ERROR;
	}

	VS_AppendString (buffer, ptr);
    }

    return T_OK;
}

int HTML_ScanText (String buffer, MyStream * stream, CBD data)
{
    int c, mode;

    VS_Clear (buffer);

    c = Str_Get (stream, data);

    if (c == EOF)
	return EOF;

    if (c == '<')
    {
	mode = M_EOWS;

	while ((c = Str_Get (stream, data)) != EOF)
	{
	    if (!isspace (c))
		goto beginOfTag;
	}

	if (c == EOF)
	{
	    PushError ("Unexpected EOF while parsing HTML tag");
	    return T_ERROR;
	}

	while ((c = Str_Get (stream, data)) != EOF)
	{
	    if (mode == M_EOWS && c == '>')
		break;

	    if (c == '"')
		mode = (mode == M_EOWS) ? M_SKIPQUOTE : M_EOWS;

beginOfTag:
	    VS_AppendChar (buffer, c);
	}

	if (c != '>')
	{
	    PushError ("Unexpected EOF while parsing HTML tag");
	    return T_ERROR;
	}

/* printf ("Tag=\"%s\"\n", buffer->buffer); */

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
    String	 str = VS_New (NULL);
    char       * ptr;
    int 	 c;
    int 	 mode;

    while ((c = Str_Get (stream, data)) != EOF)
    {
	if (!isspace (c))
	    break;
    }

    if (c != EOF)
    {
	VS_AppendChar (str, c);

	while ((c = Str_Get (stream, data)) != EOF)
	{
	    if (isspace (c))
		break;

	    VS_AppendChar (str, c);
	}
    }

    if (!str->len)
    {
	PushError ("Unexpected EOF while parsing HTML tag name");
	return NULL;
    }

    tag = new (HTMLTag);
    NewList (&tag->args);

    VS_ToUpper (str);

    tag->node.name = xstrdup (str->buffer);

/* printf ("Tag2=\"%s\"\n", tag->node.name); */

    while ((c = Str_Get (stream, data)) != EOF)
    {
	if (isspace (c))
	    continue;
	else
	{
	    VS_Clear (str);
	    VS_AppendChar (str, c);

	    mode = M_EOWS;

	    while ((c = Str_Get (stream, data)) != EOF)
	    {
		if (mode == M_EOWS && isspace (c))
		    break;

		if (c == '"')
		    mode = (mode == M_EOWS) ? M_SKIPQUOTE : M_EOWS;

		if (c == '&')
		{
		    String buf = VS_New (NULL);
		    int    rc;

		    rc = HTML_ScanAmp (buf, stream, data);

		    if (rc == T_ERROR)
		    {
			PushError ("HTML_ParseTag() failed");
			return NULL;
		    }

		    VS_AppendString (str, buf->buffer);

		    VS_Delete (buf);
		}
		else
		    VS_AppendChar (str, c);
	    }

	    arg = new (HTMLTagArg);

	    ptr = str->buffer;

	    while (*ptr && *ptr != '=') ptr ++;

	    if (*ptr)
		*ptr++ = 0;

	    arg->node.name = xstrdup (str->buffer);
	    strupper (arg->node.name);

	    if (*ptr)
		arg->value = stripquotes (ptr);
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

    mode = 0;

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
	    if (!strcasecmp (str->buffer+start, name))
	    {
		if (!allowNest)
		{
		    PushError ("HTML tag %s: Nested %s", name, name);
		    VS_Delete (str);
		    return NULL;
		}
		else
		{
		    level ++;
		}
	    }
	    else if (str->buffer[start] == '/' &&
		!strcasecmp (str->buffer+start+1, name))
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
		    PushError ("%s:%d:", Str_GetName (ss), Str_GetLine (ss));
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
