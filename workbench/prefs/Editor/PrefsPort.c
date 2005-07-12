/***************************************************************
**** PrefsPort.c: Communication port with JanoEditor        ****
**** Free software under GNU license, started on 11/11/2000 ****
**** Written by T.Pierron                                   ****
***************************************************************/

#include <exec/types.h>
#include <exec/io.h>
#include <libraries/dos.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <clib/alib_protos.h>

#include "Gui.h"
#include "Prefs.h"
#include "IPC_Prefs.h"
#include "JanoPrefs.h"

/** Port of Pref and Jano **/
static struct MsgPort *port, *reply;
static struct JPacket *cmd;

UBYTE *PortName = JANOPREFS_PORT;

/** Look if prefs isn't alredy running **/
char find_prefs( void )
{
	ULONG sigwait;
	if((reply = (struct MsgPort *) FindPort(PortName)))
	{
		PortName = NULL;		/* Private port */
		if(( sigwait = create_port() ))
		{
			/* Send to the running preference tool that someone tries to launch it again */
			cmd->class = CMD_SHOW;
			PutMsg(reply, (struct Message *)cmd);
			/* cmd packet is associated with "port", thus reply will be done here */
			Wait( sigwait );
			/* Unqueue message (don't reply!) */
			GetMsg( port );
		}
		/* Cleanup will be done later */
		return 1;
	} else return 0;
}

/** Setup public port of the preference editor **/
ULONG create_port( void )
{
	/* Create a port and  */
	if(( port = (struct MsgPort *) CreatePort(PortName, 0L) ))
	{
		/* Create a message that can be sent to the editor */
		if(( cmd = (struct JPacket *) CreateExtIO(port, (long) sizeof (*cmd)) ))
			return (unsigned) (1L << port->mp_SigBit);

		DeletePort(port);
	}
	return 0;
}

/** Search for a running session of editor **/
char find_jano(PREFS *prefs)
{
	extern struct Screen *Scr;
	if((reply = (struct MsgPort *) FindPort(JANO_PORT)))
	{
		/* Get a copy of the preferences that uses the editor */
		cmd->class = CMD_PREF;
		PutMsg(reply, (struct Message *)cmd);
		Wait( 1 << port->mp_SigBit | SIGBREAKF_CTRL_C );
		GetMsg( port );
		/* Copy to our local buffer */
		CopyMem(&cmd->prefs, prefs, sizeof(*prefs));
		Scr = prefs->current;
		return 1;
	}
	return 0;
}

/** Send a preference struct to Jano's public port **/
char send_jano(PREFS *prefs, ULONG class)
{
	/* The port can be shutted down!! */
	if(( reply = (struct MsgPort *) FindPort(JANO_PORT)))
	{
		cmd->class = class;
		CopyMem(prefs, &cmd->prefs, sizeof(*prefs));

		PutMsg(reply, (struct Message *)cmd);
		Wait( 1 << port->mp_SigBit | SIGBREAKF_CTRL_C );
		GetMsg( port );
		return 1;
	} else return 0;
}

/** Shutdown port **/
void close_port( void )
{
	if( cmd )   DeleteExtIO((struct IORequest *)cmd);
	if( port ) {
		/* Be sure there are no message left */
		struct Message *msg;
		while((msg = GetMsg( port ))) ReplyMsg(msg);
		DeletePort( port );
	}
}

/** Handle messages posted to the public port **/
void handle_port( void )
{
	struct JPacket *msg;
	extern UBYTE    ConfigFile;
	while(( msg = (struct JPacket *) GetMsg(port) ))
	{
		switch( msg->class )
		{
			case CMD_SHOW: setup_guipref(); break;
			case CMD_KILL:
				/* Close preference tool only if it's associated to jano */
				if( !ConfigFile ) close_prefwnd(0); break;
		}
		ReplyMsg((struct Message *)msg);
	}
}
