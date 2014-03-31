/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>

#define MODE_CON 0
#define MODE_RAW 1

void SetConsoleMode( LONG mode );
TEXT GetChar( void );
void PutChar( TEXT buffer );
void PutString( STRPTR buffer );
void Wait4Char( void );

int main( void )
{
    TEXT ch;

    SetConsoleMode( MODE_RAW );

    while( TRUE )
    {
        Wait4Char();
        ch = GetChar();
        PutString( " >" );
        PutChar( ch );
        PutString( "< " );

        if( ch == 'x' || ch == 3 )
            break;
    }

    SetConsoleMode( MODE_CON );
    PutChar( '\n' );

    return 0;
}

void SetConsoleMode( LONG mode )
{
    SetMode( Input() , mode );
}

TEXT GetChar( void )
{
    TEXT buffer;

    Read( Output() , &buffer , 1 );

    return buffer;
}

void PutChar( TEXT buffer )
{
    Write( Output() , &buffer , 1 );
}

void PutString( STRPTR buffer )
{
    Write( Output() , buffer , strlen( buffer ) );
}

void Wait4Char( void )
{
    WaitForChar( Input() , 0 );
}
