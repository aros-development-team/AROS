#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>

#define PROGNAME "rtfile"

struct Library *RTBase;

static char s[300];

static void cleanup(char *msg)
{
    if (msg) printf(PROGNAME ": %s\n", msg);
    
    if (RTBase) CloseLibrary(RTBase);
    
    exit(0);
}

static void openlibs(void)
{
    RTBase = OpenLibrary("reqtools.library", 0);
    if (!RTBase) cleanup("Can't open reqtools.library");
}

static void action(void)
{
    struct rtFileRequester *req;
    
    struct TagItem tags[] =
    {
    	{TAG_DONE									    }
    };
    
    if ((req = rtAllocRequestA(RT_FILEREQ, tags)))
    {
    kprintf("--\n");
        rtFileRequestA(req, s, "Title", tags);
	rtFreeRequest(req);
    }
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);
}
