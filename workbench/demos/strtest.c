/*
    (C) 1995-2006 AROS - The Amiga Research OS
    $Id$

    Desc: Demo/test for AROS stringgadgets.
    Lang: english
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>

#define DEBUG 1
#include <aros/debug.h>

VOID HandleEvents(struct Window *);

/***************
**  Gadgetry  **
***************/
#define STRBUFSIZE 100
#define STRGADWIDTH	100
#define STRGADHEIGHT	20


UBYTE strbuf[STRBUFSIZE];
UBYTE undobuf[STRBUFSIZE];

UWORD strborderdata[] = { 0,0, STRGADWIDTH + 3,0, STRGADWIDTH + 3,STRGADHEIGHT +3,
		0,STRGADHEIGHT + 3, 0,0};

struct Border strborder = { -2, -2, 1, 0, JAM1, 5, strborderdata, };

struct StringInfo strinfo = {strbuf, undobuf, 0, STRBUFSIZE, };
struct Gadget strgad = {NULL, 20, 20, STRGADWIDTH, STRGADHEIGHT, 
	GFLG_GADGHCOMP, GACT_RELVERIFY|GACT_STRINGLEFT, GTYP_STRGADGET,
	&strborder, NULL, NULL, 0, &strinfo, 0, NULL, };

struct IntuitionBase *IntuitionBase;

/*************
**  main()  **
*************/
int main(int argc, char **argv)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (IntuitionBase)
    {
    	struct Window *window = OpenWindowTags(NULL,
		WA_Width,	200,
    		WA_Height,	100,
    		WA_Title,	(IPTR)"Stringgadget Demo",
    		WA_Gadgets,	(IPTR)&strgad,
    		WA_IDCMP,	IDCMP_GADGETUP|IDCMP_RAWKEY|IDCMP_CLOSEWINDOW,
		WA_DragBar,     TRUE,
		WA_CloseGadget, TRUE,
    		TAG_END);
    	if (window)
    	{
	    HandleEvents(window);
	    CloseWindow(window);
    	}
    	
    	CloseLibrary((struct Library *)IntuitionBase);
    }

    return (0);
}

/*******************
**  HandleEvents  **
*******************/
VOID HandleEvents(struct Window *win)
{
    struct IntuiMessage *imsg;
    struct MsgPort *port = win->UserPort;
    BOOL terminated = FALSE;
	
    while (!terminated)
    {
	if ((imsg = (struct IntuiMessage *)GetMsg(port)) != NULL)
	{
	    
	    switch (imsg->Class)
	    {
		
	    case IDCMP_GADGETUP:
		break;

	    case IDCMP_RAWKEY:
	        if(imsg->Code == 0x10)
		    terminated = TRUE;

		break;

	    case IDCMP_CLOSEWINDOW:
		terminated = TRUE;
		break;
		    					
	    } /* switch (imsg->Class) */
	    ReplyMsg((struct Message *)imsg);
	    
	    			
	} /* if ((imsg = GetMsg(port)) != NULL) */
	else
	{
	    Wait(1L << port->mp_SigBit);
	}
    } /* while (!terminated) */
	
    return;
} /* HandleEvents() */
