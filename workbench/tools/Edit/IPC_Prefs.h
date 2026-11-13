/***************************************************************
**** IPC_Prefs.h: Datatypes used to communicate with Jano   ****
**** Free software under GNU license, started on 11/11/2000 ****
**** Written by T.Pierron                                   ****
***************************************************************/

#ifndef	IPC_PREFS_H
#define	IPC_PREFS_H

#define	JANOPREFS_PORT		"JANOPREF"
#define	JANO_PORT			"JANO"

/** Packet used to communicate between process **/
struct JPacket
{
	struct Message Msg;				/* Exec's message */
	ULONG  class;						/* Class of message (see below) */
#	ifdef	UTILITY_H
	union {
		PREFS        prefs;			/* Preference data */
		StartUpArgs *args;			/* List of file to edit */
	}	msg;
#	else
	PREFS           prefs;			/* args in needed only in Events.c */
#	endif
};

/** Possible values of class field (Jano & pref) **/
#define	CMD_SHOW					(CMD_NONSTD+1)		/* CMD_NONSTD defined in exec/io.h */
#define	CMD_KILL					(CMD_NONSTD+2)
#define	CMD_PREF					(CMD_NONSTD+3)		/* Copy local prefs into message */
#define	CMD_NEWPREF				(CMD_NONSTD+4)		/* Copy message prefs into local */
#define	CMD_SAVPREF				(CMD_NONSTD+5)		/* Copy and save message prefs into local */

/** JanoEditor only **/
#define	CMD_NEWEDIT				(CMD_NONSTD+6)

ULONG create_port( void );				/* Create control port */
void  handle_port( void );				/* Process commands */
char  find_prefs( void );				/* Search for preference port */
char  send_jano(PREFS *, ULONG);		/* Send new pref struct to Jano */
char  find_jano(PREFS *);				/* Try to find Jano and ask for its preference */
void  close_port( void );

#endif
