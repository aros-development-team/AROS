#ifndef _POINTER_H_
#define _POINTER_H_
/***************************************************************************

 NBalance.mcc - New Balance MUI Custom Class
 Copyright (C) 2008-2013 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

// forward declarations
struct InstData;

// enums
enum PointerType
{
  PT_HORIZ=0,  // horizontal size pointer
  PT_VERT,     // vertical size pointer
  PT_NONE
};

// prototypes
void SetupCustomPointers(struct InstData *data);
void CleanupCustomPointers(struct InstData *data);
void ShowCustomPointer(Object *obj, struct InstData *data);
void HideCustomPointer(Object *obj, struct InstData *data);

#endif // _POINTER_H_
