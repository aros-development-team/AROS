#ifndef MAIN_H
#define MAIN_H

#include <workbench/startup.h>
#include <sys/commwben.h>

extern LONG Blanking, BlankAfterInit;

extern struct MsgPort *ServerPort, *TimerPort;
extern struct timerequest *TimeOutIO;
extern struct List *BlankerEntries;
extern struct Task *ServerTask, *PingTask;
extern struct WBStartup *WbMsg;
extern BlankerPrefs *Prefs;
extern BYTE ProgName[];

/* Interface prototypes */
ULONG ISigs( VOID );
LONG OpenInterface( VOID );
VOID CloseInterface( VOID );
LONG HandleInterface( VOID );

VOID FreeResources( LONG );
LONG AllocResources( VOID );

#endif /* MAIN_H */
