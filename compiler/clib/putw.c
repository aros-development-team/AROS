/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    SVID function putw().
*/

#include <stdio.h>

int putw(int word, FILE *stream)
{
    if (fwrite(&word, sizeof(word), 1, stream) > 0) return 0;
    else                                            return EOF;
}

