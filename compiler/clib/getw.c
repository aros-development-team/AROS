/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
    
    SVID function getw().
*/

#include <stdio.h>

int getw(FILE *stream)
{
    int word;
    
    if (fread(&word, sizeof(word), 1, stream) > 0) return word;
    else                                           return EOF;
}

