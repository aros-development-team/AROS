#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <stdio.h>

struct IntuitionBase *IntuitionBase;

static struct IntuiText body, body2, pos, neg;

int main(void)
{
    if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {
	pos.IText = "Positive text";
	neg.IText = "Negative text";

	body.IText = "Text Line 1";
	body2.IText = "Text Line 2";
	body2.TopEdge = 20;

	AutoRequest(0, &body, &pos, &neg, 0, 0, 0, 0);

	body.NextText = &body2;

	AutoRequest(0, &body, &pos, &neg, 0, 0, 0, 0);

	AutoRequest(0, &body, 0, &neg, 0, 0, 0, 0);

	body2.LeftEdge = 50;
	body2.IText = "This is a long text line";

	AutoRequest(0, &body, 0, &neg, 0, 0, 0, 0);


	CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
	fprintf(stderr, "Could not open intuition.library V39 or greater.\n");
	return 10;
    }

return 0;
}
