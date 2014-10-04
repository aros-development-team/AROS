/*
 * This is an example of how to use the SimpleRexx code.
 */

#include	<exec/types.h>
#include	<libraries/dos.h>
#include	<libraries/dosextens.h>
#include	<intuition/intuition.h>
#include	<intuition/iobsolete.h>

#include	<proto/exec.h>
#include	<proto/intuition.h>

#include	<rexx/storage.h>
#include	<rexx/rxslib.h>

#include	<stdio.h>
#include	<string.h>

#include	"SimpleRexx.h"
#include	"stptok.h"

/*
 * Lattice control-c stop...
 */
int CXBRK(void) { return(0); }  /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */

/*
 * The strings in this program
 */
char *strings[]=
{
	"50: Window already open",		/* STR_ID_WINDOW_OPEN */
	"101: Window did not open",		/* STR_ID_WINDOW_ERROR */
	"50: No Window",			/* STR_ID_WINDOW_NONE */
	"80: Argument error to WINDOW command",	/* STR_ID_WINDOW_ARG */
	"100: Unknown command",			/* STR_ID_COMMAND_ERROR */
	"ARexx port name: %s\n",		/* STR_ID_PORT_NAME */
	"No ARexx on this system.\n",		/* STR_ID_NO_AREXX */
	"SimpleRexxExample Window"		/* STR_ID_WINDOW_TITLE */
};

#define	STR_ID_WINDOW_OPEN	0
#define	STR_ID_WINDOW_ERROR	1
#define	STR_ID_WINDOW_NONE	2
#define	STR_ID_WINDOW_ARG	3
#define	STR_ID_COMMAND_ERROR	4
#define	STR_ID_PORT_NAME	5
#define	STR_ID_NO_AREXX		6
#define	STR_ID_WINDOW_TITLE	7

/*
 * NewWindow structure...
 */
static struct NewWindow nw=
{
	97,47,299,44,-1,-1,
	CLOSEWINDOW,
	WINDOWSIZING|WINDOWDRAG|WINDOWDEPTH|WINDOWCLOSE|SIMPLE_REFRESH|NOCAREREFRESH,
	NULL,NULL,
	NULL,
	NULL,NULL,290,40,-1,-1,WBENCHSCREEN
};

/*
 * A *VERY* simple and simple-minded example of using the SimpleRexx.c code.
 *
 * This program, when run, will print out the name of the ARexx port it
 * opens.  Use that port to tell it to SHOW the window.  You can also
 * use the ARexx port to HIDE the window, to READTITLE the window's
 * titlebar, and QUIT the program.  You can also quit the program by
 * pressing the close gadget in the window while the window is up.
 *
 * Note: You will want to RUN this program or have another shell available such
 *       that you can still have access to ARexx...
 */
int main(int argc,char *argv[])
{
short	loopflag=TRUE;
AREXXCONTEXT	RexxStuff;
struct	Window	*win=NULL;
ULONG	signals;

	if (IntuitionBase=(struct IntuitionBase *)
					OpenLibrary("intuition.library",0))
	{
		/*
		 * Note that SimpleRexx is set up such that you do not
		 * need to check for an error to initialize your REXX port
		 * This is so your application could run without REXX...
		 */
		RexxStuff=InitARexx("Example","test");

		if (argc)
		{
			if (RexxStuff) printf(strings[STR_ID_PORT_NAME],
							ARexxName(RexxStuff));
			else printf(strings[STR_ID_NO_AREXX]);
		}

		while (loopflag)
		{
			signals=ARexxSignal(RexxStuff);
			if (win) signals|=(1L << (win->UserPort->mp_SigBit));

			if (signals)
			{
			struct	RexxMsg		*rmsg;
			struct	IntuiMessage	*msg;

				signals=Wait(signals);

				/*
				 * Process the ARexx messages...
				 */
				while ((rmsg=GetARexxMsg(RexxStuff)))
				{
				char	cBuf[24];
				char	*nextchar;
				char	*error=NULL;
				char	*result=NULL;
				long	errlevel=0;

					nextchar=stptok(ARG0(rmsg),
								cBuf,24," ,");
					if (nextchar && *nextchar) nextchar++;

					if (!stricmp("WINDOW",cBuf))
					{
						if (!stricmp("OPEN",nextchar))
						{
							if (win)
							{
								error=strings[STR_ID_WINDOW_OPEN];
								errlevel=5;
							}
							else
							{
								nw.Title=strings[STR_ID_WINDOW_TITLE];
								if (!(win=OpenWindow(&nw)))
								{
									error=strings[STR_ID_WINDOW_ERROR];
									errlevel=30;
								}
							}
						}
						else if (!stricmp("CLOSE",nextchar))
						{
							if (win)
							{
								CloseWindow(win);
								win=NULL;
							}
							else
							{
								error=strings[STR_ID_WINDOW_NONE];
								errlevel=5;
							}
						}
						else
						{
							error=strings[STR_ID_WINDOW_ARG];
							errlevel=20;
						}
					}
					else if (!stricmp("READTITLE",cBuf))
					{
						if (win)
						{
							result=win->Title;
						}
						else
						{
							error=strings[STR_ID_WINDOW_NONE];
							errlevel=5;
						}
					}
					else if (!stricmp("QUIT",cBuf))
					{
						loopflag=FALSE;
					}
					else
					{
						error=strings[STR_ID_COMMAND_ERROR];
						errlevel=20;
					}

					if (error)
					{
						SetARexxLastError(RexxStuff,rmsg,error);
					}
					ReplyARexxMsg(RexxStuff,rmsg,result,errlevel);
				}

				/*
				 * If we have a window, process those messages
				 */
				if (win) while (msg=(struct IntuiMessage *)
							GetMsg(win->UserPort))
				{
					if (msg->Class==CLOSEWINDOW)
					{
						/*
						 * Quit if the close gadget...
						 */
						loopflag=FALSE;
					}
					ReplyMsg((struct Message *)msg);
				}
			}
			else loopflag=FALSE;
		}
		if (win) CloseWindow(win);

		FreeARexx(RexxStuff);
		CloseLibrary((struct Library *)IntuitionBase);
	}
	return 0;
}
