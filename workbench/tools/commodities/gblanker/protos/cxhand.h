#ifndef CXHAND_H
#define CXHAND_H

#define SIG_CX ( 1L << CxPort->mp_SigBit )

extern BlankMsg *InterruptMsg;
extern struct MsgPort *CxPort;
extern CxObj *ServerBroker;
extern LONG timeCount;

LONG HandleCxMess( VOID );
VOID ShutdownCX( VOID );
LONG SetupCX( VOID );
LONG CheckCX( VOID );

#endif /* CXHAND_H */
