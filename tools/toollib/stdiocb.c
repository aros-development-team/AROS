#include <error.h>
#include <stdiocb.h>

int
StdioGetCharCB (void * obj, int cmd, CBD data)
{
    StdioStream * ss = (StdioStream *)obj;
    int c;

    switch (cmd & STRCB_CMDMASK)
    {
    case STRCB_GETCHAR:
	c = getc (ss->fh);

	if (c == '\n')
	    ss->line ++;

	break;

    case _STRCB_UNGETC:
	c = (cmd & 0xFFFF);

	if (c == 0xFFFF)
	    c = -1;

	c = ungetc (c, ss->fh);

	break;

    default:
	c = -2;
    }

    return c;
}

StdioStream *
StdStr_New (const char * path, const char * mode)
{
    StdioStream * ss = new (StdioStream);

    ss->name = xstrdup (path);
    ss->line = 1;
    ss->fh   = fopen (path, mode);

    if (!ss->fh)
    {
	xfree (ss->name);
	xfree (ss);
	ss = NULL;
    }

    return ss;
}

void
StdStr_Delete (StdioStream * ss)
{
    fclose (ss->fh);
    xfree (ss->name);
    xfree (ss);
}

const char  *
StdStr_GetName (StdioStream * ss)
{
    return ss->name;
}

int
StdStr_GetLine (StdioStream * ss)
{
    return ss->line;
}
