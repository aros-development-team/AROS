/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/* I'm too lazy to open the font and query it. Anyway this is hardcoded. */
#define TOPAZ_8_BASELINE 6

/* Perhaps this should be localized */
static const char title[] = "Program alert: ";

static struct IntuiText *displayalert_makebody(STRPTR string, struct TextAttr *font)
{
    struct IntuiText *res;
    char *s = string;
    unsigned int lines = 0;
    unsigned int i;

    /* First count number of lines */
    do
    {
        s += 3;		/* Skip coordinates			      */
	while (*s++);	/* Skip text bytes including NULL terminator  */
	lines++;
    } while (*s++);	/* This automatically skips continuation byte */

    res = AllocVec(sizeof(struct IntuiText) * lines, MEMF_ANY);
    if (!res)
	return NULL;

    s = string;
    for (i = 0; i < lines; i++)
    {
	res[i].FrontPen = 1;
	res[i].BackPen  = 0;
	res[i].DrawMode = JAM2;
	res[i].LeftEdge = AROS_BE2WORD(*((UWORD *)s));
	s += 2;
	res[i].TopEdge  = *s++ - TOPAZ_8_BASELINE;
	res[i].ITextFont = font;
	res[i].IText = s;
	while(*s++);
	res[i].NextText = *s++ ? &res[i+1] : NULL;
    }

    return res;
}

/*****************************************************************************

    NAME */
#include <proto/intuition.h>

        AROS_LH3(BOOL, DisplayAlert,

/*  SYNOPSIS */
        AROS_LHA(ULONG , alertnumber, D0),
        AROS_LHA(UBYTE*, string, A0),
        AROS_LHA(UWORD , height, D1),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 15, Intuition)

/*  FUNCTION
	Bring up an alert with the given message.

    INPUTS
        alertnumber - Value determining type of alert. For historical reasons,
                 this is the same value as passed to Alert(). However,
                 this functions takes into account only AT_DeadEnd bit.
        string - A pointer to text data. Text data have the following layout:
                 each string is preceded by 3 bytes. The first two of them are
                 the X coordinates of the string in the alert display. This is
                 given as a big-endian value. The third byte is the Y
                 coordinate of the text's baseline. Then a NUL-terminated
                 string follows by itself. After the NUL terminator there's
                 one more byte. If it's not zero, another string starts from
                 the next byte. Zero marks the end of the sequence. The text
                 is always rendered using the topaz/8 font.
        height - The height of alert display in pixels.

    RESULT
        Always FALSE if AT_DeadEnd bit is set in alertnumber. Otherwise the
        function returns TRUE or FALSE depending on what user chooses. In
        AROS, alerts are presented in a requester with two gadgets: Ok and
        Cancel. Ok returns TRUE; Cancel returns FALSE.

        If the alert could not be posted for whatever reason, FALSE is
        returned.

    NOTES
        This function is obsolete and exists only for backwards compatibility
        with AmigaOS(tm). On various modern systems this function has
        different effects. On classic Amiga(tm) this function may not work
        with RTG displays, so it is generally deprecated. Please don't use it
        in new software! Use legitimate intuition requesters if you want to
        present some message to the user.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TextAttr font = {
	"topaz.font",
	8,
	FS_NORMAL,
	0
    };
    struct IntuiText postext = {
	1, 0, JAM2,
	0, 0,
	NULL,
	"Ok"
    };
    struct IntuiText negtext = {
	1, 0, JAM2,
	0, 0,
	NULL,
	"Cancel"
    };
    struct IntuiText *body = displayalert_makebody(string, &font);
    struct Window *req;
    LONG ret = FALSE;

    if (body)
    {
	char *buf;
	unsigned int l1, l2;
	struct Task *t = FindTask(NULL);

	l1 = strlen(title);
	l2 = strlen(t->tc_Node.ln_Name) + 1;
	buf = AllocMem(l1 + l2, MEMF_ANY);
	if (buf)
	{
	    CopyMem(title, buf, l1);
	    CopyMem(t->tc_Node.ln_Name, &buf[l1], l2);

	    /*
	     * This is actually the same as AutoRequest(), without IDCMP processing.
	     * We use internal function instead of BuildSysRequest() because the latter
	     * does not allow to specify own title for the requester. In order to stay
	     * compatible with various patches which modify system requesters look and
	     * feel we call all three functions by their internal entry points.
	     */
	    if (alertnumber & AT_DeadEnd)
		req = buildsysreq_intern(NULL, buf, body, NULL, &postext, 0, 640, height, IntuitionBase);
	    else
		req = buildsysreq_intern(NULL, buf, body, &postext, &negtext, 0, 640, height, IntuitionBase);

		if (req)
	    {
		while ((ret = sysreqhandler_intern(req, NULL, TRUE, IntuitionBase)) == -2);
		freesysreq_intern(req, IntuitionBase);
	    }

	    FreeMem(buf, l1 + l2);
	}
	FreeVec(body);
    }

    return ret;

    AROS_LIBFUNC_EXIT
} /* DisplayAlert */
