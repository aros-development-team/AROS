/*
    Copyright  2011, The AROS Development Team.
    $Id$
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <exec/types.h>

/* TODO: make private */
LONG GetInt(STRPTR v);
void GetIntegers(STRPTR v, LONG *v1, LONG *v2);
void GetTripleIntegers(STRPTR v, LONG *v1, LONG *v2, LONG *v3);
void GetColors(STRPTR v, LONG *v1, LONG *v2);
BOOL GetBool(STRPTR v, STRPTR id);
/* TODO: make private */

struct DecorConfig
{
    /* Menu Section */
    BOOL    MenuIsTiled;
    LONG    MenuTileLeft;
    LONG    MenuTileTop;
    LONG    MenuTileRight;
    LONG    MenuTileBottom;
    LONG    MenuInnerLeft;
    LONG    MenuInnerTop;
    LONG    MenuInnerRight;
    LONG    MenuInnerBottom;
};

struct DecorConfig * LoadConfig(STRPTR path);

#endif
