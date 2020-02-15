
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGNAME "rtfile"

struct ReqToolsBase *ReqToolsBase;

static char s[300];

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
    struct rtFileRequester *req;
    
    struct TagItem tags[] =
    {
        { RTFI_Flags, FREQF_PATGAD },
    	{ TAG_DONE }
    };
    
    if ((req = rtAllocRequestA(RT_FILEREQ, tags)))
    {
        rtFileRequestA(req, s, "Title", tags);
	rtFreeRequest(req);
    }
}


int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0;
}
