#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/alib.h>

#include <stdio.h>

struct IntuitionBase *IntuitionBase;

static struct EasyStruct es;

int main(void)
{
    struct Window *req;

    LONG result, counter = 0;

    if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
    {

	es.es_StructSize = sizeof(es);
	es.es_Flags = 0;

	es.es_Title = "First Requester";
	es.es_TextFormat = "I'm a very nice requester :)";
	es.es_GadgetFormat = "Yep!";

	EasyRequestArgs(0, &es, 0, 0);

	es.es_Title = "Second Requester";
	es.es_TextFormat = "This requester should have\ntwo text lines!";
	es.es_GadgetFormat = "Ok|Cancel";

	result = EasyRequestArgs(0, &es, 0, 0);
	puts((result == 1) ? "You clicked on \"OK\"." : "You clicked on \"Cancel\".");

	es.es_Title = "Third Requester";
	es.es_TextFormat = "One\nTwo\nThree\nFour\n\nTest Test";
	es.es_GadgetFormat = "10|20|30|40|50|60|70|80|90|100";

	result = EasyRequestArgs(0, &es, 0, 0);
	printf("Result %ld\n",result);

	es.es_Title = "Fourth Requester";
	es.es_TextFormat = "Requester Text with args:\n\nArg 1: %ld  Arg 2: %ld  Arg 3: %ld\nStringarg: %s";
	es.es_GadgetFormat = "Coooool";

	EasyRequest(0, &es, 0, 10, 20, 30, (IPTR)"I'm the string");

	es.es_Title = "Fifth Requester";
	es.es_TextFormat = "Requester Text with text and gadget args:\n\nArg 1: %ld  Arg 2: %ld  Arg 3: %ld\nStringarg: %s";
	es.es_GadgetFormat = "Coooool %ld|Holy %s";

	EasyRequest(0, &es, 0, 10, 20, 30, (IPTR)"I'm the string", 7777, "crap");

	es.es_Title = "Sixth Requester";
	es.es_TextFormat = "I'm an asynchronous Requester.\nWatch shell output while\nrequester is open!";
	es.es_GadgetFormat = "Incredible|Great|Not bad";

	req = BuildEasyRequestArgs(0, &es, 0, 0);
	if (req && (req != (struct Window *)1))
	{
	    do
	    {
		Delay(20);
		result = SysReqHandler(req, 0, FALSE);
		printf("*** Async Counter: %ld ***\n",++counter);

	    } while (result == -2);

	    printf("You clicked on \"%s\"\n",(result == 1) ? "Incredible" : (result == 2) ? "Great" : "Not bad"); 
	}
	FreeSysRequest(req);

	CloseLibrary((struct Library *)IntuitionBase);
    }
    else
    {
	fprintf(stderr, "Could not open intuition.library V39 or greater.\n");
	return 10;
    }

return 0;
}
