/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

extern struct MUI_CustomClass *MCC;

/**
    what:
        0 - [struct MUI_CustomClass *] pointer to the class
        1 - [struct MUI_CustomClass *] pointer to the prefs class
        2 - [Object *]                 pointer to prefs image object
        3 - [BOOL]                     ONLYGLOBAL ?!?
*/

#define MCC_CLASS                (0)
#define MCC_PREFS_CLASS          (1)
#define MCC_PREFS_IMAGE          (2)
#define MCC_IS_ONLY_GLOBAL       (3)

IPTR MCC_Query( LONG what )
{
    switch( what )
    {
        case MCC_CLASS:          return MCC;
        case MCC_PREFS_CLASS:    return NULL;
        case MCC_PREFS_IMAGE:    return NULL;
        case MCC_IS_ONLY_GLOBAL: return NULL;
    }
    
    return NULL;
}
