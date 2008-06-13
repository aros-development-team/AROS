#ifndef PREFS_SERIAL_H
#define PREFS_SERIAL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial prefs definitions
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif


#define ID_SERL MAKE_ID('S','E','R','L')

struct SerialPrefs
{
    LONG  sp_Reserved[3];
    ULONG sp_Unit0Map;
    ULONG sp_BaudRate;

    ULONG sp_InputBuffer;
    ULONG sp_OutputBuffer;

    UBYTE sp_InputHandshake;
    UBYTE sp_OutputHandshake;

    UBYTE sp_Parity;
    UBYTE sp_BitsPerChar;
    UBYTE sp_StopBits;
};

#define PARITY_NONE	0
#define PARITY_EVEN	1
#define PARITY_ODD	2
#define PARITY_MARK	3
#define PARITY_SPACE	4

#define HSHAKE_XON	0
#define HSHAKE_RTS	1
#define HSHAKE_NONE	2

#endif /* PREFS_SERIAL_H */
