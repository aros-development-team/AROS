/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef BROWSERS_H
#define BROWSERS_H

#include <intuition/classusr.h>
#include <exec/lists.h>

struct URL_BrowserNode;

Object * make_edit_brow_win(void);

BOOL updateBrowserList( struct List * list, struct MinList PrefsBrowserList );

void updateBrowserWindow( struct URL_BrowserNode  * pBrowser );

void updateBrowserNode();
#endif // BROWSERS_H

