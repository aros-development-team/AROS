/*
     AHI - The AHI preferences program
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _SUPPORT_H_
#define _SUPPORT_H_

#include <exec/types.h>
#include <exec/lists.h>
#include <devices/ahi.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

extern struct AHIGlobalPrefs globalprefs;

BOOL Initialize(void);
void CleanUp(void);

struct List * GetUnits(char * );
struct List * GetModes(struct AHIUnitPrefs * );
char ** List2Array(struct List * );
char ** GetInputs(ULONG );
char ** GetOutputs(ULONG );
BOOL SaveSettings(char * , struct List * );
BOOL WriteIcon(char * );
void FreeList(struct List * );
struct Node * GetNode(int , struct List * );

BOOL PlaySound(struct AHIUnitPrefs *);

#endif /* _SUPPORT_H_ */
