#include <pwd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include "error.h"
#include "varstr.h"
#include "util.h"

void * xmalloc (int size)
{
    void * mem = malloc (size);

    if (!mem)
	yyerror ("xmalloc: Out of memory");

    return mem;
}

void xfree (void * mem)
{
    if (!mem)
	yyerror ("free(NULL)");

    free (mem);
}

char * xstrdup (const char * str)
{
    char * dupe;

    if (!str)
	yyerror ("strdup(NULL)");

    dupe = strdup (str);

    if (!dupe)
	yyerror ("xstrdup: Out of memory");

    return dupe;
}

char * expandpath (const char * path)
{
    VarString * vs;
    char * str, * res;

    if (*path == '~')
    {
	const char * ptr = path+1;
	char user[32], *uptr = user;
	struct passwd * pw;

	while (isalpha (*ptr))
	    *uptr++ = *ptr++;

	*uptr = 0;

	pw = getpwnam (user);

	if (!pw)
	{
	    vs = createvarstring ("~");
	    appendtovarstring (vs, user);
	}
	else
	{
	    vs = createvarstring (pw->pw_dir);
	}

	appendtovarstring (vs, ptr);
    }
    else
	vs = createvarstring (path);

    str = tostring (vs);
    res = substvar (str);

    free (str);

    return res;
}

char * substvar (const char * str)
{
    char buffer[1024];
    char name[32];
    char * bptr, * nptr;

    bptr = buffer;

    while (*str)
    {
	if (*str == '$')
	{
	    char * val;

	    str ++;
	    nptr = name;

	    if (*str == '{')
	    {
		str ++;

		while (*str && *str != '}')
		    *nptr ++ = *str ++;

		if (*str)
		    str ++;
	    }
	    else if (*str == '(')
	    {
		str ++;

		while (*str && *str != ')')
		    *nptr ++ = *str ++;

		if (*str)
		    str ++;
	    }
	    else
	    {
		while (isalpha(*str))
		    *nptr ++ = *str ++;
	    }

	    *nptr = 0;

	    val = getenv (name);

	    if (!val)
	    {
		*bptr++ = '$';
		strcpy (bptr, name);
		bptr += strlen (bptr);
	    }
	    else
	    {
		strcpy (bptr, val);
		bptr += strlen (bptr);
	    }
	}
	else
	{
	    *bptr ++ = *str ++;
	}
    }

    *bptr = 0;

    return strdup (buffer);
}

char * xbasename (const char * path, const char * ext)
{
    const char * ptr = rindex (path, '/');
    char * ptr2;
    int len;

    if (!ptr)
	ptr = path;

    len = strlen (ptr);
    ptr2 = rindex (ptr, '.');

    if (ptr2 && !strcmp (ptr2, ext))
    {
	len -= strlen (ext);
    }

    ptr2 = xmalloc (len+1);
    memmove (ptr2, ptr, len);
    ptr2[len] = 0;

    return ptr2;
}

char * buildname (const char * fmt, ...)
{
    va_list args;
    char buffer[1024];

    va_start (args, fmt);
    vsnprintf (buffer, sizeof(buffer), fmt, args);
    va_end (args);

    return expandpath (buffer);
}

time_t getfiledate (const char * path)
{
    struct stat st;

    if (stat (path, &st))
	yyerror ("Error getting date of \"%s\": %s",
	    path,
	    strerror (errno));

    return st.st_mtime;
}
