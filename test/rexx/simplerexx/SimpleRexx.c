/*
 * Simple ARexx interface by Michael Sinz
 *
 * This is a very "Simple" interface to the world of ARexx...
 * For more complex interfaces into ARexx, it is best that you
 * understand the functions that are provided by ARexx.
 * In many cases they are more powerful than what is presented
 * here.
 *
 * This code is fully re-entrant and self-contained other than
 * the use of SysBase/AbsExecBase and the ARexx RVI support
 * library which is also self-contained...
 */

#include	<exec/types.h>
#include	<exec/nodes.h>
#include	<exec/lists.h>
#include	<exec/ports.h>
#include	<exec/memory.h>

#include	<proto/exec.h>
#include	<proto/alib.h>
#include	<proto/rexxsyslib.h>

#include	<rexx/storage.h>
#include	<rexx/rxslib.h>

#include	<string.h>
#include	<ctype.h>
#include	<stdio.h>

#if 0
/*
 * The prototypes for the few ARexx functions we will call...
 */
struct RexxMsg *CreateRexxMsg(struct MsgPort *,char *,char *);
void *CreateArgstring(char *,long);
void DeleteRexxMsg(struct RexxMsg *);
void DeleteArgstring(char *);
BOOL IsRexxMsg(struct Message *);

/*
 * Pragmas for the above functions...  (To make this all self-contained...)
 * If you use RexxGlue.o, this is not needed...
 *
 * These are for Lattice C 5.x  (Note the use of RexxContext->RexxSysBase)
 */
#pragma libcall RexxContext->RexxSysBase CreateRexxMsg 90 09803
#pragma libcall RexxContext->RexxSysBase CreateArgstring 7E 0802
#pragma libcall RexxContext->RexxSysBase DeleteRexxMsg 96 801
#pragma libcall RexxContext->RexxSysBase DeleteArgstring 84 801
#pragma libcall RexxContext->RexxSysBase IsRexxMsg A8 801

/*
 * Prototypes for the RVI ARexx calls...  (link with RexxVars.o)
 */
__stdargs long CheckRexxMsg(struct RexxMsg *);
__stdargs long GetRexxVar(struct RexxMsg *,char *,char **);
__stdargs long SetRexxVar(struct RexxMsg *,char *,char *,long);

/*
 * Now, we have made the pragmas needed, let's get to work...
 */
#endif

/*
 * A structure for the ARexx handler context
 * This is *VERY* *PRIVATE* and should not be touched...
 */
struct	ARexxContext
{
struct	MsgPort	*ARexxPort;	/* The port messages come in at... */
struct	Library	*RexxSysBase;	/* We will hide the library pointer here... */
	long	Outstanding;	/* The count of outstanding ARexx messages... */
	char	PortName[24];	/* The port name goes here... */
	char	ErrorName[28];	/* The name of the <base>.LASTERROR... */
	char	Extension[8];	/* Default file name extension... */
};

#define	AREXXCONTEXT	struct ARexxContext *

#include	"SimpleRexx.h"

/*
 * This function returns the port name of your ARexx port.
 * It will return NULL if there is no ARexx port...
 *
 * This string is *READ ONLY*  You *MUST NOT* modify it...
 */
char *ARexxName(AREXXCONTEXT RexxContext)
{
	char	*tmp=NULL;

	if (RexxContext) tmp=RexxContext->PortName;
	return(tmp);
}

/*
 * This function returns the signal mask that the Rexx port is
 * using.  It returns NULL if there is no signal...
 *
 * Use this signal bit in your Wait() loop...
 */
ULONG ARexxSignal(AREXXCONTEXT RexxContext)
{
	ULONG	tmp=0;

	if (RexxContext) tmp=1L << (RexxContext->ARexxPort->mp_SigBit);
	return(tmp);
}

/*
 * This function returns a structure that contains the commands sent from
 * ARexx...  You will need to parse it and return the structure back
 * so that the memory can be freed...
 *
 * This returns NULL if there was no message...
 */
struct RexxMsg *GetARexxMsg(AREXXCONTEXT RexxContext)
{
	struct	RexxMsg	*tmp=NULL;
	short	flag;

	if (RexxContext)
	    if (tmp=(struct RexxMsg *)GetMsg(RexxContext->ARexxPort))
	{
		if (tmp->rm_Node.mn_Node.ln_Type==NT_REPLYMSG)
		{
			/*
			 * If we had sent a command, it would come this way...
			 *
			 * Since we don't in this simple example, we just throw
			 * away anything that looks "strange"
			 */
			flag=FALSE;
			if (tmp->rm_Result1) flag=TRUE;

			/*
			 * Free the arguments and the message...
			 */
			DeleteArgstring(tmp->rm_Args[0]);
			DeleteRexxMsg(tmp);
			RexxContext->Outstanding-=1;

			/*
			 * Return the error if there was one...
			 */
			tmp=flag ? REXX_RETURN_ERROR : NULL;
		}
	}
	return(tmp);
}

/*
 * Use this to return a ARexx message...
 *
 * If you wish to return something, it must be in the RString.
 * If you wish to return an Error, it must be in the Error.
 * If there is an error, the RString is ignored.
 */
void ReplyARexxMsg(AREXXCONTEXT RexxContext,struct RexxMsg *rmsg,
			char *RString,LONG Error)
{
	if (RexxContext) if (rmsg) if (rmsg!=REXX_RETURN_ERROR)
	{
		rmsg->rm_Result2=0;
		if (!(rmsg->rm_Result1=Error))
		{
			/*
			 * if you did not have an error we return the string
			 */
			if (rmsg->rm_Action & (1L << RXFB_RESULT)) if (RString)
			{
				rmsg->rm_Result2=(LONG)CreateArgstring(RString,
							(LONG)strlen(RString));
			}
		}

		/*
		 * Reply the message to ARexx...
		 */
		ReplyMsg((struct Message *)rmsg);
	}
}

/*
 * This function will set an error string for the ARexx
 * application in the variable defined as <appname>.LASTERROR
 *
 * Note that this can only happen if there is an ARexx message...
 *
 * This returns TRUE if it worked, FALSE if it did not...
 */
short SetARexxLastError(AREXXCONTEXT RexxContext,struct RexxMsg *rmsg,
			char *ErrorString)
{
	short	OkFlag=FALSE;

	if (RexxContext) if (rmsg) if (CheckRexxMsg(rmsg))
	{
		/*
		 * Note that SetRexxVar() has more than just a TRUE/FALSE
		 * return code, but for this "basic" case, we just care if
		 * it works or not.
		 */
		if (!SetRexxVar(rmsg,RexxContext->ErrorName,ErrorString,
						(long)strlen(ErrorString)))
		{
			OkFlag=TRUE;
		}
	}
	return(OkFlag);
}

/*
 * This function will send a string to ARexx...
 *
 * The default host port will be that of your task...
 *
 * If you set StringFile to TRUE, it will set that bit for the message...
 *
 * Returns TRUE if it send the message, FALSE if it did not...
 */
short SendARexxMsg(AREXXCONTEXT RexxContext,char *RString,
			short StringFile)
{
	struct	MsgPort	*RexxPort;
	struct	RexxMsg	*rmsg;
	short	flag=FALSE;

	if (RexxContext) if (RString)
	{
		if (rmsg=CreateRexxMsg(RexxContext->ARexxPort,
					RexxContext->Extension,
					RexxContext->PortName))
		{
			rmsg->rm_Action=RXCOMM | (StringFile ?
							(1L << RXFB_STRING):0);
			if (rmsg->rm_Args[0]=(IPTR)CreateArgstring(RString,
							(LONG)strlen(RString)))
			{
				/*
				 * We need to find the RexxPort and this needs
				 * to be done in a Forbid()
				 */
				Forbid();
				if (RexxPort=FindPort(RXSDIR))
				{
					/*
					 * We found the port, so put the
					 * message to ARexx...
					 */
					PutMsg(RexxPort,(struct Message *)rmsg);
					RexxContext->Outstanding+=1;
					flag=TRUE;
				}
				else
				{
					/*
					 * No port, so clean up...
					 */
					DeleteArgstring(rmsg->rm_Args[0]);
					DeleteRexxMsg(rmsg);
				}
				Permit();
			}
			else DeleteRexxMsg(rmsg);
		}
	}
	return(flag);
}

/*
 * This function closes down the ARexx context that was opened
 * with InitARexx...
 */
void FreeARexx(AREXXCONTEXT RexxContext)
{
	struct	RexxMsg	*rmsg;

	if (RexxContext)
	{
		/*
		 * Clear port name so it can't be found...
		 */
		RexxContext->PortName[0]='\0';

		/*
		 * Clean out any outstanding messages we had sent out...
		 */
		while (RexxContext->Outstanding)
		{
			WaitPort(RexxContext->ARexxPort);
			while ((rmsg=GetARexxMsg(RexxContext)))
			{
				if (rmsg!=REXX_RETURN_ERROR)
				{
					/*
					 * Any messages that come now are blown
					 * away...
					 */
					SetARexxLastError(RexxContext,rmsg,
							    "99: Port Closed!");
					ReplyARexxMsg(RexxContext,rmsg,
							NULL,100);
				}
			}
		}

		/*
		 * Clean up the port and delete it...
		 */
		if (RexxContext->ARexxPort)
		{
			while ((rmsg=GetARexxMsg(RexxContext)))
			{
				/*
				 * Any messages that still are coming in are
				 * "dead"  We just set the LASTERROR and
				 * reply an error of 100...
				 */
				SetARexxLastError(RexxContext,rmsg,
							"99: Port Closed!");
				ReplyARexxMsg(RexxContext,rmsg,NULL,100);
			}
			DeletePort(RexxContext->ARexxPort);
		}

		/*
		 * Make sure we close the library...
		 */
		if (RexxContext->RexxSysBase)
		{
			CloseLibrary(RexxContext->RexxSysBase);
		}

		/*
		 * Free the memory of the RexxContext
		 */
		FreeMem(RexxContext,sizeof(struct ARexxContext));
	}
}

/*
 * This routine initializes an ARexx port for your process
 * This should only be done once per process.  You must call it
 * with a valid application name and you must use the handle it
 * returns in all other calls...
 *
 * NOTE:  The AppName should not have spaces in it...
 *        Example AppNames:  "MyWord" or "FastCalc" etc...
 *        The name *MUST* be less that 16 characters...
 *        If it is not, it will be trimmed...
 *        The name will also be UPPER-CASED...
 *
 * NOTE:  The Default file name extension, if NULL will be
 *        "rexx"  (the "." is automatic)
 */
AREXXCONTEXT InitARexx(char *AppName,char *Extension)
{
	AREXXCONTEXT	RexxContext=NULL;
	short		loop;
	short		count;
	char		*tmp;

	if (RexxContext=AllocMem(sizeof(struct ARexxContext),
					MEMF_PUBLIC|MEMF_CLEAR))
	{
		if (RexxContext->RexxSysBase=OpenLibrary("rexxsyslib.library",
								0))
		{
			/*
			 * Set up the extension...
			 */
			if (!Extension) Extension="rexx";
			tmp=RexxContext->Extension;
			for (loop=0;(loop<7)&&(Extension[loop]);loop++)
			{
				*tmp++=Extension[loop];
			}
			*tmp='\0';

			/*
			 * Set up a port name...
			 */
			tmp=RexxContext->PortName;
			for (loop=0;(loop<16)&&(AppName[loop]);loop++)
			{
				*tmp++=toupper(AppName[loop]);
			}
			*tmp='\0';

			/*
			 * Set up the last error RVI name...
			 *
			 * This is <appname>.LASTERROR
			 */
			strcpy(RexxContext->ErrorName,RexxContext->PortName);
			strcat(RexxContext->ErrorName,".LASTERROR");

			/* We need to make a unique port name... */
			Forbid();
			for (count=1,RexxContext->ARexxPort=(VOID *)1;
						RexxContext->ARexxPort;count++)
			{
#if 0
				stci_d(tmp,count);
#else
				sprintf(tmp,"_%d",count);
#endif
				RexxContext->ARexxPort=
						FindPort(RexxContext->PortName);
			}

			RexxContext->ARexxPort=CreatePort(
						RexxContext->PortName,0);
			Permit();
		}

		if (	(!(RexxContext->RexxSysBase))
		     ||	(!(RexxContext->ARexxPort))	)
		{
			FreeARexx(RexxContext);
			RexxContext=NULL;
		}
	}
	return(RexxContext);
}
