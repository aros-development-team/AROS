/**********************************************************************************************/

/* modelclassdemo:

   connect one prop gadget with two other prop gadgets,
   one string gadget, and the IDCMP.
   
   This interconnection is done with modelclass and icclass objects

*/

/**********************************************************************************************/

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>

#include <stdio.h>
#include <string.h>

/**********************************************************************************************/

struct IntuitionBase 	*IntuitionBase;
static struct Window 	*win;
static struct Gadget 	*gad1, *gad2, *gad3, *gad4;
static Object 		*model, *ic1;
static BOOL 		gadgets_added_to_model = FALSE;

/**********************************************************************************************/

static struct TagItem prop_to_idcmp[] =
{
    {PGA_Top	, ICSPECIAL_CODE	},
    {TAG_DONE				}
};

static struct TagItem prop_to_string[] =
{
    {PGA_Top    , STRINGA_LongVal	},
    {TAG_DONE				}
};


/**********************************************************************************************/

static void cleanup(char *msg)
{
    if (msg) printf("modelclass: %s\n",msg);
    
    if (win) RemoveGList(win, gad1, -1);
    if (gad1) DisposeObject((Object *)gad1);
    if (gad3) DisposeObject((Object *)gad3);

    if (!gadgets_added_to_model)
    {
        if (gad2) DisposeObject((Object *)gad2);
	if (gad4) DisposeObject((Object *)gad4);
	if (ic1) DisposeObject((Object *)ic1);
    }
    
    if (model) DisposeObject(model);
    
    CloseWindow(win);

    if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    
}

/**********************************************************************************************/

static void openlibs(void)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39);
    if (!IntuitionBase) cleanup("can't open intuition.library V39!");
}

/**********************************************************************************************/

static void makegadgets(void)
{
    model = NewObject(0, MODELCLASS, ICA_TARGET, ICTARGET_IDCMP,
    				     ICA_MAP, (IPTR)prop_to_idcmp,
				     TAG_DONE);
    
    if (!model) cleanup("can't create modelclass object!");
    
    ic1 = NewObject(0, ICCLASS, TAG_DONE);

    if (!ic1) cleanup("can't create icclass object!");
    
    gad1 = (struct Gadget *)NewObject(0, PROPGCLASS, GA_Left, 10,
    						     GA_Top, 40,
						     GA_Width, 20,
						     GA_Height, 100,
						     PGA_Freedom, FREEVERT,
						     PGA_Top, 0,
						     PGA_Total, 1000,
						     PGA_Visible, 100,
						     ICA_TARGET, (IPTR)model,
						     TAG_DONE);
    if (!gad1) cleanup("can't create gadget 1!");
    
    gad2 = (struct Gadget *)NewObject(0, PROPGCLASS, GA_Left, 40,
    						     GA_Top, 40,
						     GA_Width, 20,
						     GA_Height, 200,
						     PGA_Freedom, FREEVERT,
						     PGA_Top, 0,
						     PGA_Total, 1000,
						     PGA_Visible, 100,
						     GA_Previous, (IPTR)gad1,
						     TAG_DONE);
    if (!gad2) cleanup("can't create gadget 2!");
    
    gad3 = (struct Gadget *)NewObject(0, STRGCLASS, GA_Left, 80,
    						    GA_Top, 40,
						    GA_Width, 100,
						    GA_Height, 20,
						    STRINGA_LongVal, 0,
						    GA_Previous, (IPTR)gad2, 
						    TAG_DONE);
						    
    if (!gad3) cleanup("can't create gadget 3!");

    gad4 = (struct Gadget *)NewObject(0, PROPGCLASS, GA_Left, 80,
    						     GA_Top, 80,
						     GA_Width, 150,
						     GA_Height, 20,
						     PGA_Freedom, FREEHORIZ,
						     PGA_Top, 0,
						     PGA_Total, 1000,
						     PGA_Visible, 100,
						     GA_Previous, (IPTR)gad3,
						     TAG_DONE);
    if (!gad4) cleanup("can't create gadget 4!");
						    
    SetAttrs(ic1, ICA_TARGET, (IPTR) gad3,
    		  ICA_MAP, (IPTR) prop_to_string,
		  TAG_DONE);
		  						    
    DoMethod(model, OM_ADDMEMBER, (IPTR) gad2);
    DoMethod(model, OM_ADDMEMBER, (IPTR) ic1);
    DoMethod(model, OM_ADDMEMBER, (IPTR) gad4);
    
    gadgets_added_to_model = TRUE;
    				    		     
}

/**********************************************************************************************/

static void makewin(void)
{
    win = OpenWindowTags(0, WA_Title		, (IPTR) "Modelclass demo: Use first prop gadget!",
    			    WA_Left		, 20000,
			    WA_Top		, 20,
			    WA_Width		, 400,
			    WA_Height		, 300,
			    WA_AutoAdjust	, TRUE,
			    WA_CloseGadget	, TRUE,
			    WA_DepthGadget	, TRUE,
			    WA_DragBar		, TRUE,
			    WA_IDCMP		, IDCMP_CLOSEWINDOW | IDCMP_IDCMPUPDATE,
			    WA_Activate		, TRUE,
			    WA_Gadgets		, (IPTR) gad1,
			    TAG_DONE
			    );
			    
    if (!win) cleanup("can't open window!");	
    		   
}

/**********************************************************************************************/

static void handleall(void)
{
    struct IntuiMessage *msg;
    BOOL quitme = FALSE;
    
    while(!quitme)
    {
        WaitPort(win->UserPort);
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
	{
	    switch(msg->Class)
	    {
	        case IDCMP_CLOSEWINDOW:
		    quitme = TRUE;
		    break;
		    
		case IDCMP_IDCMPUPDATE:
		    printf("IDCMP_IDCMPUPDATE: code = %d\n", msg->Code);
		    break;
		    
	    } /* switch msg->Class */
	    
	    ReplyMsg((struct Message *)msg);
	    
	} /* while msg = getmsg */
	
    } /* while (!quitme) */
}

/**********************************************************************************************/

int main(void)
{
    openlibs();
    makegadgets();
    makewin();
    handleall();
    cleanup(0);
    return 0;
    
}

/**********************************************************************************************/
