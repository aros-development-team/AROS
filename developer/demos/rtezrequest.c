
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>

#define PROGNAME "rtezrequest"

struct ReqToolsBase *ReqToolsBase;

static void cleanup(char *msg)
{
    if (msg) printf(PROGNAME ": %s\n", msg);
    
    if (ReqToolsBase) CloseLibrary((struct Library *)ReqToolsBase);
    
    exit(0);
}

static void openlibs(void)
{
    ReqToolsBase = (struct ReqToolsBase *)OpenLibrary("reqtools.library", 0);
    if (!ReqToolsBase) cleanup("Can't open reqtools.library");
}

static void myEZRequest(struct TagItem *tags, struct rtReqInfo *reqinfo, char *bodyfmt, char *gadfmt, ...)
{
    AROS_SLOWSTACKFORMAT_PRE_USING(gadfmt, bodyfmt)
    rtEZRequestA(bodyfmt, gadfmt, reqinfo, AROS_SLOWSTACKFORMAT_ARG(gadfmt), tags);
    AROS_SLOWSTACKFORMAT_POST(gadfmt)
}

static void action(void)
{
    struct TagItem tags[] =
    {
        { RT_Underscore	, (IPTR)'_' },
	{ RTEZ_ReqTitle	, (IPTR)"I'm a ReqTools Requester" },
    	{ TAG_DONE }
    };

    myEZRequest(tags, NULL,
                "This is a requester\n"
    		 "which was created\n"
		 "with rtEZRequestA",
    		 "_Ok");

    myEZRequest(tags, NULL,
                "And another rtEZRequestA\n"
                "requester\n"
		 "\n"
		 "String arg: \"%s\"  Integer arg: %ld",
    		 "Coo_l|So _what",
                "ABCDEF12345678", 12345678);
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0;
}
