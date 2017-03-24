/*
 * Simple ARexx interface by Michael Sinz
 *
 * This is a very "Simple" interface...
 */

#ifndef	SIMPLE_REXX_H
#define	SIMPLE_REXX_H

#include	<exec/types.h>
#include	<exec/nodes.h>
#include	<exec/lists.h>
#include	<exec/ports.h>

#include	<rexx/storage.h>
#include	<rexx/rxslib.h>

/*
 * This is the handle that SimpleRexx will give you
 * when you initialize an ARexx port...
 *
 * The conditional below is used to skip this if we have
 * defined it earlier...
 */
#ifndef	AREXXCONTEXT

typedef void *AREXXCONTEXT;

#endif	/* AREXXCONTEXT */

/*
 * The value of RexxMsg (from GetARexxMsg) if there was an error returned
 */
#define	REXX_RETURN_ERROR	((struct RexxMsg *)-1L)

/*
 * This function closes down the ARexx context that was opened
 * with InitARexx...
 */
void FreeARexx(AREXXCONTEXT);

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
AREXXCONTEXT InitARexx(char *,char *);

/*
 * This function returns the port name of your ARexx port.
 * It will return NULL if there is no ARexx port...
 *
 * This string is *READ ONLY*  You *MUST NOT* modify it...
 */
char *ARexxName(AREXXCONTEXT);

/*
 * This function returns the signal mask that the Rexx port is
 * using.  It returns NULL if there is no signal...
 *
 * Use this signal bit in your Wait() loop...
 */
ULONG ARexxSignal(AREXXCONTEXT);

/*
 * This function returns a structure that contains the commands sent from
 * ARexx...  You will need to parse it and return the structure back
 * so that the memory can be freed...
 *
 * This returns NULL if there was no message...
 */
struct RexxMsg *GetARexxMsg(AREXXCONTEXT);

/*
 * Use this to return a ARexx message...
 *
 * If you wish to return something, it must be in the RString.
 * If you wish to return an Error, it must be in the Error.
 */
void ReplyARexxMsg(AREXXCONTEXT,struct RexxMsg *,char *,LONG);

/*
 * This function will send a string to ARexx...
 *
 * The default host port will be that of your task...
 *
 * If you set StringFile to TRUE, it will set that bit for the message...
 *
 * Returns TRUE if it send the message, FALSE if it did not...
 */
short SendARexxMsg(AREXXCONTEXT,char *,short);

/*
 * This function will set an error string for the ARexx
 * application in the variable defined as <appname>.LASTERROR
 *
 * Note that this can only happen if there is an ARexx message...
 *
 * This returns TRUE if it worked, FALSE if it did not...
 */
short SetARexxLastError(AREXXCONTEXT,struct RexxMsg *,char *);

#endif	/* SIMPLE_REXX_H */
