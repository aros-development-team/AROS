
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

static void action(void)
{
    struct TagItem tags[] =
    {
        { RT_Underscore	, (IPTR)'_' },
	{ RTEZ_ReqTitle	, (IPTR)"I'm a ReqTools Requester" },
    	{ TAG_DONE }
    };

    IPTR args[] = {(IPTR)"ABCDEF12345678", 12345678};
       
    rtEZRequestA("This is a requester\n"
    		 "which was created\n"
		 "with rtEZRequestA\n"
		 "\n"
		 "String arg: \"%s\"  Integer arg: %ld",
    		 "_Ok|Coo_l|So _what",
		 NULL,
		 args,
		 tags);
}


int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0;
}
