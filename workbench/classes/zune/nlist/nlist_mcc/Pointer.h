#if !defined(_POINTER_H_)
#define _POINTER_H_
/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
 Copyright (C) 2001-2014 NList Open Source Team

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
struct NLData;

// enums
enum PointerType
{
  PT_NONE=0,  // no custom pointer active
  PT_SIZE,    // sizePointer active
  PT_MOVE,    // movePointer active
  PT_SELECT,  // selectPointer active
};

// prototypes
void SetupCustomPointers(struct NLData *data);
void CleanupCustomPointers(struct NLData *data);
void ShowCustomPointer(struct NLData *data, enum PointerType type);
void HideCustomPointer(struct NLData *data);

#endif // _POINTER_H_
