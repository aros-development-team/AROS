#include <exec/types.h>

VOID	InitLocale( VOID );
VOID	FreeLocale( VOID );
STRPTR	GetString( STRPTR );
VOID	LocalizeMenus( struct NewMenu * );
VOID	LocalizeLabels( STRPTR * );

ULONG	EasyReq( STRPTR, STRPTR, APTR );
ULONG	LocEZReq( STRPTR, STRPTR, ... );
ULONG	EZReq( STRPTR, STRPTR, ... );

#include "rtstrings.h"
