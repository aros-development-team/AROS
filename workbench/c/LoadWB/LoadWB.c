/*
    Copyright © 2002, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Load the Workbench
    Lang: English
*/

/******************************************************************************

    NAME

	LoadWB

    SYNOPSIS

    LOCATION

	Workbench:C

    FUNCTION

	Loads a *very* simple "Workbench".
 
    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define   DEBUG  0
#include  <aros/debug.h>

#include <dos/dos.h>
#include <dos/notify.h>
#include <dos/dostags.h>
#include <dos/filesystem.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <libraries/gadtools.h>

#include <proto/intuition.h>
#include <proto/workbench.h>
#include <proto/gadtools.h>
#include <proto/dos.h>
#include <proto/alib.h>

#include <stdlib.h>

#include "Desktop.h"

static const char version[] = "$VER: LoadWB 0.2 (14.04.2002)\n";

extern struct Window  * wbwindow;
extern struct Menu *    menus;

ULONG     notifysig;
struct MsgPort * notifyport;
struct Screen  * wbscreen;
APTR vi;


/*********************************************************************************************/
int QuitRequest(void);
void Cleanup(STRPTR msg);
void ExecuteCommand(void);
/*********************************************************************************************/

LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Workbench";

void DoDetach(void)
{
    kprintf("LoadWB.DoDetach\n");

    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
}

/*********************************************************************************************/

int InitWB()
{
    kprintf("LoadWB.InitWB\n");

    notifyport = CreateMsgPort();

    if (!notifyport)
	Cleanup("Can't create notification msg port!\n");

    notifysig = 1L << notifyport->mp_SigBit;

    wbscreen = LockPubScreen(NULL);
    vi = GetVisualInfoA(wbscreen, NULL);

    RegisterWorkbench(notifyport);

    OpenDesktop(DESKTOP_All);

    return TRUE;
}

/*********************************************************************************************/

int QuitRequest()
{
int retval;
struct EasyStruct es;

    es.es_StructSize   = sizeof(es);
    es.es_Flags        = 0;
    es.es_Title        = "AROS Workbench...";
    es.es_TextFormat   = "Do you really want to quit\nAROS Workbench?";
    es.es_GadgetFormat = "Ok|Cancel";
    retval = EasyRequest ( wbwindow, &es, NULL, NULL, NULL );

    return retval;
}

/*********************************************************************************************/

void Cleanup(STRPTR msg)
{
    kprintf("LoadWB.Cleanup\n");

    if (msg)
    {
        FPuts(Error(), msg);
    }

    CloseDesktop(DESKTOP_All);

    UnregisterWorkbench(notifyport);

    DoDetach();

    exit(msg ? RETURN_FAIL : RETURN_OK);
}

/*********************************************************************************************/

void HandleNotify()
{
struct IntuiMessage *msg;
struct MenuItem *item;
ULONG class;
UWORD code;

kprintf("LoadWB.HandleNotify\n");

    msg = (struct IntuiMessage *) GetMsg( wbwindow->UserPort );
    class = msg->Class;
    code = msg->Code;

    switch(class)
    {
	case IDCMP_CLOSEWINDOW :
	    if(QuitRequest())
	    {
		Cleanup(NULL);
	    }
	    break;

	case IDCMP_MENUPICK :
	    while(code != MENUNULL)
	    {
//		kprintf("Menu: %d %d %d\n", MENUNUM(code), ITEMNUM(code), SUBNUM(code));
		switch(MENUNUM(code))
		{
		    case 0: /* Workbench */
			switch(ITEMNUM(code))
			{
			    case 0: /* Backdrop */
				CloseDesktop(DESKTOP_Main);
				Desktop.Backdrop = !Desktop.Backdrop;
				OpenDesktop(DESKTOP_Main);
      				break;

			    case 1: /* Execute Command... */
				ExecuteCommand();
				break;

			    case 2: /* Redraw All */
				break;

			    case 3: /* Update All */
				break;

			    case 4: /* Last Message */
				break;

			    case 5: /* Shell */
			    {
				BPTR win = Open("CON:10/10/640/480/AROS-Shell/CLOSE", FMF_READ);
				SystemTags
                                (
                                    "",
				    SYS_Asynch,     TRUE,
				    SYS_Background, FALSE,
				    SYS_Input,	    (IPTR)win,
				    SYS_Output,	    (IPTR)NULL,
				    SYS_Error,	    (IPTR)NULL,
				    SYS_UserShell,  TRUE,
				    TAG_DONE
				);

                                break;
 			    }
			    case 6: /* About... */
			    {
				struct EasyStruct es;

				es.es_StructSize   = sizeof(es);
				es.es_Flags        = 0;
				es.es_Title        = "About AROS Workbench...";
				es.es_TextFormat   = "Written by Henning Kiel <hkiel@aros.org>\nCopyright © 2002, The AROS Development Team.\nAll rights reserved.\n\nAROS 0.7x ROM (Alpha)\nWe made it...\nThe AROS Development Team:\nAaron Digulla, Georg Steger, Nils Henrik Lorentzen,\nHenning Kiel, Staf Verhaegen, Henrik Berglund,\nMichal Schulz, Iain Templeton, Fabio Alemagna,\nSebastian Heutling, Johan Grip, Tobias Seiler,\nJohan Alfredsson, Adam Chodorowski, Matt Parsons...\nTo be continued...";
				es.es_GadgetFormat = "Better than ever!";
				EasyRequest ( wbwindow, &es, NULL, NULL, NULL );

				break;
			    }
			    case 7: /* Quit... */
				if(QuitRequest())
				{
				    Cleanup(NULL);
				}
				break;

			    default:
				break;
			}
			break;

		    case 1: /* Window */
			break;
		    case 2: /* Icon */
			break;
		    case 3: /* Tools */
			break;
		    default:
			break;
		}

                if ( (item = ItemAddress ( menus, code ) ) != NULL )
		{
		    code = item->NextSelect;
		}
		{
		    code = MENUNULL;
		}

	    }
	    break;

	default:
	    break;

    }
    ReplyMsg ( (struct Message *)msg );

}

/*********************************************************************************************/

void HandleAll()
{
    ULONG sigs;

kprintf("LoadWB.HandleAll\n");

    for(;;)
    {
	sigs = Wait(notifysig | SIGBREAKF_CTRL_C);

	if (sigs & SIGBREAKF_CTRL_C) break;

	if (sigs & notifysig) HandleNotify();
    }
}

/********************************************************************************************/

int main(void)
{
    if( InitWB() == 0 )
	return RETURN_FAIL;

    DoDetach();
    HandleAll();

    Cleanup(NULL);

    return RETURN_OK;
}

