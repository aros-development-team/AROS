#include <string.h>
#include <toollib/mystream.h>

int
Str_Init (MyStream * ms, const char * name)
{
    memset (ms, 0, sizeof (MyStream));

    ms->line = 1;
    ms->name = xstrdup (name);

    return 1;
}

void
Str_Delete (MyStream * ms)
{
    xfree (ms->name);
}

int
Str_Puts (MyStream * ms, const char * str, CBD data)
{
    int c;

    if (ms->puts)
	c = CallCB (ms->puts, ms, (int)str, data);
    else
    {
	c = 0;

	while (*str && (c = Str_Put(ms,*str,data)) > 0)
	    str ++;
    }

    return c;
}

