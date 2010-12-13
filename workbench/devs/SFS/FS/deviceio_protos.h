#ifndef _DEVICEIO_PROTOS_H
#define _DEVICEIO_PROTOS_H

#include <exec/types.h>
#include <exec/tasks.h>
#include <dos/filehandler.h>

void update(void);
void motoroff(void);

UBYTE isdiskpresent(void);
LONG writeprotection(void);
ULONG deviceapiused(void);

LONG transfer(UWORD action, UBYTE *buffer, ULONG blockoffset, ULONG blocklength);

LONG initdeviceio(UBYTE *devicename, ULONG unit, ULONG flags, struct DosEnvec *de);
void cleanupdeviceio(void);

LONG addchangeint(struct Task *task, ULONG signal);
void removechangeint(void);
ULONG getchange(void);

void changegeometry(struct DosEnvec *de);

#endif // _DEVICEIO_PROTOS_H
