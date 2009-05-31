#ifndef PREFS_H
#define PREFS_H

extern struct IBox WinBox;
extern STRPTR CornerStrs[];

VOID BlankerToEnv( BlankerPrefs * );
VOID WinBoxToEnv( struct IBox * );
BlankerPrefs *LoadDefaultPrefs( VOID );
STRPTR RandomModule( VOID );

#endif /* PREFS_H */
