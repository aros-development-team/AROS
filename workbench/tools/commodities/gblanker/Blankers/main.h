#ifndef MAIN_H
#define MAIN_H

#include <proto/dos.h>

extern struct Library *GarshnelibBase;
extern struct ExecBase *SysBase;
extern struct DosLibrary *DOSBase;
extern struct GfxBase *GfxBase;
extern struct IntuitionBase *IntuitionBase;

LONG MessageServer( LONG );
LONG HandleSignal( LONG );
LONG ContinueBlanking( VOID );
VOID Complain( STRPTR );

VOID Defaults( PrefObject * );
LONG Blank( PrefObject * );

#define Inc( Value, Amt, Max ) (( Value + Amt > Max )?( Value = Max ):( Value = Value + Amt ))
#define Dec( Value, Amt, Min ) (( Value - Amt < Min )?( Value = Min ):( Value = Value - Amt ))
#define CompX( x ) ((( FontX * x ) + 3 ) / 6 )
#define CompY( x ) ((( FontY * x ) + 4 ) / 9 )
#define ComputeX( x ) ((( FontX * x ) + 3 ) / 6 )
#define ComputeY( x ) ((( FontY * x ) + 4 ) / 9 )

#endif /* MAIN_H */
