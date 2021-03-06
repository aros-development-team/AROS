/*
    Copyright (C) 1995-2015, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <proto/dos.h>
#include <string.h>
#include <stdlib.h>

#define MODE_CON 0
#define MODE_RAW 1

static void SetConsoleMode( LONG mode );
static TEXT GetChar( void );
static void PutChar( TEXT buffer );
static void PutString( STRPTR buffer );

int main( void )
{
    TEXT ch;

    SetConsoleMode( MODE_RAW );

    while( TRUE )
    {
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

static void SetConsoleMode( LONG mode )
{
    SetMode( Input() , mode );
}

static TEXT GetChar( void )
{
    TEXT buffer;

    Read( Output() , &buffer , 1 );

    return buffer;
}

static void PutChar( TEXT buffer )
{
    Write( Output() , &buffer , 1 );
}

static void PutString( STRPTR buffer )
{
    Write( Output() , buffer , strlen( buffer ) );
}
