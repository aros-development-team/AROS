#include "varstr.h"
#include <string.h>
#include <stdlib.h>
#include "util.h"

struct _VarString
{
    long len;
    long max;
    char * str;
};

#define BLOCKINGSIZE	    64
#define BLOCKING(x)         (((x) + BLOCKINGSIZE-1) & -BLOCKINGSIZE)

VarString * createvarstring (const char * initial)
{
    VarString * vs = xmalloc (sizeof (VarString));

    if (initial)
    {
	vs->len = strlen (initial);
	vs->max = BLOCKING(vs->len+1);

	vs->str = xmalloc (vs->max);
	strcpy (vs->str, initial);
    }
    else
    {
	vs->len = 0;
	vs->max = BLOCKINGSIZE;
	vs->str = xmalloc (vs->max);
	vs->str[0] = 0;
    }

    return vs;
}

int appendtovarstring (VarString * vs, const char * str)
{
    int len = strlen (str);

    if (vs->len+len+5 >= vs->max)
    {
	vs->max = BLOCKING (vs->len+len+5);

	vs->str = realloc (vs->str, vs->max);
    }

    strcpy (&vs->str[vs->len], str);
    vs->len += len;

    return 1;
}

char * tostring (VarString * vs)
{
    char * str = vs->str;

    xfree (vs);

    return str;
}

void deletevarstring (VarString * vs)
{
    xfree (vs->str);
    xfree (vs);
}

int getvschar (VarString * vs, int pos)
{
    if (pos <= 0)
	return vs->str[0];
    else if (pos < vs->len)
	return vs->str[pos];

    return vs->str[vs->len-1];
}
