/*
    Copyright © 1998-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Test program for Commodities
    Lang: English
*/

#include <proto/exec.h>
#include <proto/commodities.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <exec/memory.h>
#include <libraries/commodities.h>
#include <dos/dos.h>

#include <stdio.h>
#include <stdlib.h>

struct IntuitionBase *IntuitionBase;

int main(int argc, char* argv[])
{
    struct Library *CxBase;
    CxObj          *myBroker;
    CxObj          *filter;
    CxObj          *trans;
    struct InputEvent myie;
    struct Window  *window;

    struct NewBroker brok =
    {
	NB_VERSION,
	"AROS TestBroker",
	"SDuvan 20.04.98",
	"Broker for testing the commodities library",
	0,
	0,
	0,
	NULL,			/* nb_Port - will be initialized below */
	0
    };


    myie.ie_NextEvent = NULL;
    myie.ie_Class = IECLASS_RAWKEY;
    myie.ie_SubClass = IESUBCLASS_COMPATIBLE;
    myie.ie_Code= 0x20;
    myie.ie_Qualifier = 0;
    
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39);

    window = OpenWindowTags(NULL,
			    WA_IDCMP,	IDCMP_RAWKEY,
			    WA_Height, 50,
			    WA_Width, 100,
			    TAG_END);
    
    if(!window)
    {
	fprintf(stderr, "Window failed to open.\n");
	exit(1);
    }    


    fprintf(stderr, "Testing commodities.library...\n");
    fflush(stderr);
    CxBase = OpenLibrary("commodities.library", 0);
        
    brok.nb_Port = CreateMsgPort();
    
    if(CxBase == NULL)
    {
	fprintf(stderr, "Couldn't open commodities.library.\n");
	return -1;
    }
    
    fprintf(stderr, "Calling CxBroker().\n");
    fflush(stderr);
 
    myBroker = CxBroker(&brok, NULL);

    fprintf(stderr, "Broker installed.\n");
    fflush(stderr);
    
    if(myBroker == NULL)
    {
        fprintf(stderr, "Error in creating object.\n");
	CloseLibrary(CxBase);
	return -1;
    }
    
    fprintf(stderr, "Creating filter object.\n");
    filter = CxFilter("rawkey return");

    if(!filter)
    {
        fprintf(stderr, "Error in creating filter.\n");
	CloseLibrary(CxBase);
	return -1;
    }

    fprintf(stderr, "Filter created.\n");

    AttachCxObj(myBroker, filter);

    fprintf(stderr, "Filter attached.\n");
    fflush(stderr);

    {
	/*	int sig = AllocSignal(-1); */
	
	/* trans = CxSignal(FindTask(NULL), sig); */
	/* trans = CxDebug(1); */
	trans = CxTranslate(&myie); 
	if(!trans)
	{
	    fprintf(stderr, "Error in creating translator.\n");
	    return -1;
	}
	
	AttachCxObj(filter, trans);
	
	ActivateCxObj(myBroker, TRUE);
	
	fprintf(stderr, "Broker activated.\n");
	fflush(stderr);
	
	Wait(SIGBREAKF_CTRL_C);
	/*	Wait(SIGBREAKF_CTRL_C | 1 << sig); */
    }

    fprintf(stderr, "Deleting all objects.\n");
    DeleteCxObjAll(myBroker);

    CloseWindow(window);

    fprintf(stderr, "Closing commodities.library...\n");
    CloseLibrary(CxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    fprintf(stderr, "All done.\n");

    return 0;
}

