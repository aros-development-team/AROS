#ifndef WINHAND_H
#define WINHAND_H

extern BYTE *InfoStr;

STRPTR NameSansParens( STRPTR );
STRPTR NameForEntry( struct List *, LONG );
LONG openMainWindow( VOID );

#endif /* WINDHAND_H */
