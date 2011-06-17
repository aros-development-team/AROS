#include <exec/types.h>

VOID	InitLocale( VOID );
VOID	FreeLocale( VOID );
CONST_STRPTR	GetString( CONST_STRPTR );
VOID	LocalizeMenus( struct NewMenu * );
VOID	LocalizeLabels( CONST_STRPTR * );

ULONG	EasyReq( STRPTR, STRPTR, APTR );
ULONG	LocEZReq( STRPTR, STRPTR, ... );
ULONG	EZReq( STRPTR, STRPTR, ... );

#include "rtstrings.h"
