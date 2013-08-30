/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#ifndef UTILITY_H
#define UTILITY_H

#include <intuition/classusr.h>

int VARARGS68K RA_Request( Object * pWin, const char * strTitle, const char * strGadgets, const char * strFormat, ... );
void freeList( struct List * list );

#define iget(obj, attr)         ({uint32 b=0; IIntuition->GetAttr(attr, (Object *)(obj), &b); b;})
#define gadset(obj, win, ...)   IIntuition->SetGadgetAttrs((obj), (win), NULL, __VA_ARGS__, TAG_DONE)
#define iset(obj, win, ...)     IIntuition->SetAttrs((obj), __VA_ARGS__, TAG_DONE)

#endif // UTILITY_H

