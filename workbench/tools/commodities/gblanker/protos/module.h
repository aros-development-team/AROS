#ifndef MODULE_H
#define MODULE_H

extern struct Screen *ServerScr;
extern struct Window *Wnd;
extern LONG NumBlankEntries;

VOID PingFunc( VOID );
VOID InternalBlank( VOID );
VOID MessageModule( STRPTR, LONG );
VOID ToggleModuleDisabled( BlankerPrefs * );
VOID ExecSubProc( STRPTR, STRPTR );
VOID LoadModule( STRPTR, STRPTR );
VOID FreeBlankerEntries( struct List * );
struct List *LoadBlankerEntries( STRPTR );

#endif /* MODULE_H */
