#include <error.h>
#include <stringcb.h>

int
StringGetCharCB (void * obj, int cmd, CBD data)
{
    StringStream * ss = (StringStream *)obj;
    int c;

    switch (cmd & STRCB_CMDMASK)
    {
    case STRCB_GETCHAR:
	if (ss->pos == ss->max)
	    c = -1;
	else
	    c = ss->string[ss->pos++];

	break;

    case _STRCB_UNGETC:
	if (!ss->pos)
	    c = -2;
	else
	{
	    c = (cmd & 0xFFFF);

	    ss->pos --;

	    if (c != ss->string[ss->pos])
		c = -2;
	}
	break;

    default:
	c = -2;
    }

    return c;
}

StringStream *
StrStr_New (const char * string)
{
    StringStream * ss = new (StringStream);

    ss->string = string;
    ss->pos    = 0;
    ss->max    = strlen (string);
    ss->line   = 1;

    return ss;
}

void
StrStr_Delete (StringStream * ss)
{
    xfree (ss);
}

int
StrStr_GetLine (StringStream * ss)
{
    return ss->line;
}
