/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>
#include <ctype.h>
#include <toollib/vstring.h>

String
VS_New (const char * ini)
{
    String str = new (struct _String);

    if (ini && *ini)
    {
	int len = strlen (ini) + 1;

	str->maxlen = ALIGN(len,8);
	str->buffer = xmalloc (str->maxlen);
	str->len    = len;

	strcpy (str->buffer, ini);
    }
    else
    {
	str->maxlen = 32;
	str->buffer = xmalloc (str->maxlen);
	str->len    = 0;

	str->buffer[0] = 0;
    }

    return str;
}

void
VS_Delete (String str)
{
    cfree (str->buffer);
    xfree (str);
}

void
VS_Clear (String str)
{
    str->len = 0;
}

void
VS_AppendChar (String str, int c)
{
    if (str->len + 2 >= str->maxlen)
    {
	str->maxlen += 32;

	str->buffer = xrealloc (str->buffer, str->maxlen);
    }

    str->buffer[str->len ++] = c;
    str->buffer[str->len]    = 0;
}

void
VS_AppendString (String str, const char * app)
{
    int len = strlen (app);

    if (str->len + len + 1 >= str->maxlen)
    {
	str->maxlen = ALIGN(str->len+len+1,8);

	str->buffer = xrealloc (str->buffer, str->maxlen);
    }

    strcpy (str->buffer + str->len, app);
    str->len += len;
}

void
VS_ToUpper (String str)
{
    int t;

    for (t=0; t<str->len; t++)
	str->buffer[t] = toupper (str->buffer[t]);
}

void
VS_ToLower (String str)
{
    int t;

    for (t=0; t<str->len; t++)
	str->buffer[t] = tolower (str->buffer[t]);
}

String
VS_SubString (const char * str, int begin, int len)
{
    int    n;
    String nstr;

    n = strlen (str);

    if (begin >= n)
	return VS_New (NULL);

    if (len == -1)
	n -= begin;
    else
    {
	if (begin+len > n)
	    n -= begin;
	else
	    n = len;
    }

    nstr = new (struct _String);

    nstr->maxlen = ALIGN(n+1,8);
    nstr->buffer = xmalloc (nstr->maxlen);
    nstr->len	 = n;

    strncpy (nstr->buffer, str + begin, n);
    nstr->buffer[n] = 0;

    return nstr;
}

char *
strdupsub (const char * str, int begin, int len)
{
    char * nstr;
    int    n;

    if (!str)
	return NULL;

    n = strlen (str);

    if (begin >= n || len <= 0)
	return strdup ("");

    if (begin+len > n)
	n -= begin;
    else
	n = len;

    nstr = xmalloc (n+1);

    strncpy (nstr, str + begin, n);
    nstr[n] = 0;

    return nstr;
}

char *
stripquotes (const char * str)
{
    int begin;
    int len;

    if (*str == '"')
    {
	begin = 1;

	len = strlen (str + begin);

	if (str[len] == '"')
	    len --;

	if (len <= 0)
	    len = -1;

	return strdupsub (str, begin, len);
    }

    return xstrdup (str);
}

char *
stripws (const char * str)
{
    int begin;
    int len;

    for (begin=0; isspace (str[begin]); begin++);

    len = strlen (str + begin);

    for (len--; len > 0 && isspace (str[begin + len]); len --);

    len ++;

    return strdupsub (str, begin, len);
}

char *
stripwsq (const char * str)
{
    int begin;
    int len;

    for (begin=0; isspace (str[begin]); begin++);

    if (str[begin] == '"')
	begin ++;

    len = strlen (str + begin);

    for (len--; len > 0 && isspace (str[begin + len]); len --);

    if (str[begin + len] == '"')
	len --;

    len ++;

    return strdupsub (str, begin, len);
}

char *
strupper (char * str)
{
    char * ptr;

    for (ptr=str; *ptr; ptr++)
	*ptr = toupper (*ptr);

    return str;
}

char *
strlower (char * str)
{
    char *ptr;

    for (ptr=str; *ptr; ptr++)
	*ptr = tolower (*ptr);

    return str;
}
