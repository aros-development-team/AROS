#ifndef LOCALE_H
#define LOCALE_H 1

/*************************************************************************/

/*
** Prototypes
*/

BOOL   Locale_Open( STRPTR catname, ULONG version, ULONG revision );
void   Locale_Close( void );
STRPTR Locale_GetString( long ID );

/*************************************************************************/

#endif /* LOCALE_H */
