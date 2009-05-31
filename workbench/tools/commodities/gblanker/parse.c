#include <proto/exec.h>
#include <proto/dos.h>
#include <ctype.h>
#include "protos/parse.h"

extern VOID *Memory;

LONG ScanDigit( BPTR File )
{
    LONG Char, Digit;

    while( isspace( Char = FGetC( File )));
    Digit = Char - '0';
    do
    {
        Char = FGetC( File );
        if( isdigit( Char ))
        {
            Digit *= 10;
            Digit += ( Char - '0' );
        }
        else
            break;
    }
    while( 1 );

    return Digit;
}

STRPTR ScanToken( BPTR File )
{
    STRPTR Anchor, Ptr = AllocPooled( Memory, sizeof( BYTE ) * 64 );

    if( Anchor = Ptr )
    {
        while( isspace( *Ptr = FGetC( File )));
        
        if( *Ptr == '"' )
        {
            *Ptr = ( BYTE )FGetC( File );
            while( *Ptr != '"' )
                *(++Ptr) = ( BYTE )FGetC( File );
        }
        else
        {
            do
                *(++Ptr) = ( BYTE )FGetC( File );
            while( !isspace( *Ptr ));
        }
        *Ptr = '\0';
    }
    
    return Anchor;
}

STRPTR *ScanTokenArray( BPTR File )
{
    LONG NumTokens = ScanDigit( File ), i;
    STRPTR *Tokens;

    if( Tokens = AllocPooled( Memory, sizeof( STRPTR ) * ( NumTokens + 1 )))
        for( i = 0; i < NumTokens; i++ )
            Tokens[i] = ScanToken( File );
    
    return Tokens;
}

LONG ScanType( BPTR File )
{
	STRPTR IDStr = ScanToken( File );

	switch( tolower( IDStr[0] ))
	{
	case 'c':
		return GAD_CYCLE;
	case 'd':
		return ( tolower( IDStr[1] ) == 'i' )? GAD_DISPLAY : GAD_DELIM;
	case 'f':
		return GAD_FONT;
	case 's':
		return ( tolower( IDStr[1] ) == 'l' )? GAD_SLIDER : GAD_STRING;
	}

	return 0L;
}
