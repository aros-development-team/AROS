#ifndef PKG_GUI_H
#define PKG_GUI_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

/****************************************************************************/
/** Prototypes **************************************************************/

BOOL GUI_Open();
void GUI_Close();
void GUI_Update( LONG position, LONG max );

#endif /* PKG_GUI_H */
