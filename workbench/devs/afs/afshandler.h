#ifndef AFS_HANDLER_H
#define AFS_HANDLER_H

#include <exec/devices.h>
#include <dos/filesystem.h>

struct afsbase
{
	struct Device device;
	BPTR seglist;
	struct MsgPort port;			/* MsgPort of the handler */
	struct MsgPort rport;		/* replyport of the handler */
	struct IOFileSys *iofs;		/* to be aborted iofs or NULL */
};

#define expunge() AROS_LC0(BPTR, expunge, struct afsbase *, afsbase, 3, afs)

#endif
