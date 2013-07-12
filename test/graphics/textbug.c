#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/keymap.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <exec/execbase.h>

static void dotest(void)
{
    struct Window *win;
    struct IntuiMessage *msg;
    BOOL quitme = FALSE;
    
    win = OpenWindowTags(NULL, WA_Left, 20,
    	    	    	       WA_Top, 20,
			       WA_Width, 300,
			       WA_Height, 100,
			       WA_Activate, TRUE,
			       WA_CloseGadget, TRUE,
			       WA_DepthGadget, TRUE,
			       WA_Title, (IPTR)"TextBug",
			       WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_RAWKEY,
			       TAG_DONE);
    if (!win)
    {
    	return;
    }
    
    SetSoftStyle(win->RPort, FSF_BOLD, AskSoftStyle(win->RPort));
    SetAPen(win->RPort, 2);
    SetBPen(win->RPort, 1);
#if 1
    SetDrMd(win->RPort, JAM2);
    Move(win->RPort, -20, 40);
    Text(win->RPort, "1234567890", 10);
    Move(win->RPort, 120, 40);
    Text(win->RPort, "1234567890", 10);

    SetDrMd(win->RPort, JAM1);
    Move(win->RPort, -20, 60);
    Text(win->RPort, "1234567890", 10);
    Move(win->RPort, 120, 60);
    Text(win->RPort, "1234567890", 10);

#endif
    RefreshWindowFrame(win);
    
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
		    
	    }
	    ReplyMsg((struct Message *)msg);
	}
    }
    
    CloseWindow(win);
}

int main(int argc, char **argv)
{
    dotest();
    return 0;

}

