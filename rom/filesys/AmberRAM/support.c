/*

File: support.c
Author: Neil Cafferkey
Copyright (C) 2001-2008 Neil Cafferkey

This file is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This file is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this file; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/


#include "handler_protos.h"



/****i* ram.handler/SetString **********************************************
*
*   NAME
*	SetString --
*
*   SYNOPSIS
*	block_diff = SetString(field, new_str)
*
*	PINT SetString(TEXT **, TEXT *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

PINT SetString(struct Handler *handler, TEXT **field, const TEXT *new_str)
{
   LONG error = 0;
   UPINT size;
   TEXT *str_copy = NULL, *old_str;
   PINT block_diff = 0;

   /* Allocate new string */

   size = StrSize(new_str);
   if(size > 1)
   {
      str_copy = AllocPooled(handler->muddy_pool, size);
      if(str_copy != NULL)
      {
         CopyMem(new_str, str_copy, size);
      }
      else
         error = ERROR_DISK_FULL;
   }
   else
      size = 0;

   if(error == 0)
   {
      /* Deallocate old string */

      if(size > 0)
         block_diff = MEMBLOCKS(size);
      old_str = *field;
      if(old_str != NULL)
      {
         size = StrSize(old_str);
         FreePooled(handler->muddy_pool, old_str, size);
         block_diff -= MEMBLOCKS(size);
      }

      /* Store new string */

      *field = str_copy;
   }

   /* Store error and return difference in block utilisation */

   SetIoErr(error);
   return block_diff;
}



/****i* ram.handler/SwapStrings ********************************************
*
*   NAME
*	SwapStrings --
*
*   SYNOPSIS
*	block_diff = SwapStrings(field1, field2)
*
*	PINT SwapStrings(TEXT **, TEXT **);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

PINT SwapStrings(TEXT **field1, TEXT **field2)
{
   TEXT *str1, *str2;
   PINT block_diff = 0;

   str1 = *field1;
   if(str1 != NULL)
      block_diff = MEMBLOCKS(StrSize(str1));

   str2 = *field2;
   if(str2 != NULL)
      block_diff -= MEMBLOCKS(StrSize(str2));

   *field1 = str2;
   *field2 = str1;

   /* Return difference in block utilisation */

   return block_diff;
}



/****i* ram.handler/StrLen *************************************************
*
*   NAME
*	StrLen --
*
*   SYNOPSIS
*	length = StrLen(s)
*
*	UPINT StrLen(TEXT *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

UPINT StrLen(const TEXT *s)
{
   const TEXT *p;

   for(p = s; *p != '\0'; p++);
   return p - s;
}



/****i* ram.handler/StrSize ************************************************
*
*   NAME
*	StrLen --
*
*   SYNOPSIS
*	size = StrSize(s)
*
*	UPINT StrSize(TEXT *);
*
*   FUNCTION
*
*   INPUTS
*	s - The string (may be NULL).
*
*   RESULT
*	size - Size of the string in bytes including terminating NUL.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

UPINT StrSize(const TEXT *s)
{
   const TEXT *p;
   UPINT length = 0;

   p = s;

   if(s != NULL)
   {
      while(*p != '\0')
         p++;
      length = p - s + 1;
   }

   return length * sizeof(TEXT);
}



/****i* ram.handler/FindNameNoCase *****************************************
*
*   NAME
*	FindNameNoCase --
*
*   SYNOPSIS
*	node = FindNameNoCase(handler, start, name)
*
*	struct Node *FindNameNoCase(struct Handler *handler, struct List *, TEXT *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

struct Node *FindNameNoCase(struct Handler *handler, struct List *start, const TEXT *name)
{
   struct Node *node, *next_node, *matching_node;
   TEXT *node_name;

   matching_node = NULL;
   node = start->lh_Head;

   while(node != NULL)
   {
      next_node = node->ln_Succ;
      if(next_node != NULL)
      {
         node_name = node->ln_Name;
         if(node_name != NULL)
            if(Stricmp(name, node_name) == 0)
            {
               matching_node = node;
               next_node = NULL;
            }
      }
      node = next_node;
   }

   return matching_node;
}



/****i* ram.handler/MyMakeDosEntry *****************************************
*
*   NAME
*	MyMakeDosEntry --
*
*   SYNOPSIS
*	dos_entry = MyMakeDosEntry(handler, name, type)
*
*	struct DosList *MyMakeDosEntry(struct Handler *handler, TEXT *, LONG);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

struct DosList *MyMakeDosEntry(struct Handler *handler, const TEXT *name, LONG type)
{
   struct DosList *entry;
   LONG error = 0;

   entry = AllocMem(sizeof(struct DosList), MEMF_CLEAR | MEMF_PUBLIC);

   if(entry != NULL)
   {
      if(!MyRenameDosEntry(handler, entry, name))
         error = IoErr();
      entry->dol_Type = type;
   }
   else
      error = IoErr();

   if(error != 0)
   {
      MyFreeDosEntry(handler, entry);
      entry = NULL;
   }

   SetIoErr(error);
   return entry;
}



/****i* ram.handler/MyFreeDosEntry *****************************************
*
*   NAME
*	MyFreeDosEntry --
*
*   SYNOPSIS
*	MyFreeDosEntry(handler, entry)
*
*	VOID MyFreeDosEntry(struct Handler *handler, struct DosList *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

VOID MyFreeDosEntry(struct Handler *handler, struct DosList *entry)
{
   if(entry != NULL)
   {
      MyRenameDosEntry(handler, entry, NULL);
      FreeMem(entry, sizeof(struct DosList));
   }

   return;
}



/****i* ram.handler/MyRenameDosEntry ***************************************
*
*   NAME
*	MyRenameDosEntry --
*
*   SYNOPSIS
*	success = MyRenameDosEntry(entry, name)
*
*	BOOL MyRenameDosEntry(struct DosList *, TEXT *);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

#ifdef __AROS__
BOOL MyRenameDosEntry(struct Handler *handler, struct DosList *entry, const TEXT *name)
{
   LONG error = 0;
   UPINT length;
   TEXT *name_copy, *old_name;

   /* Allocate new string */

   name_copy = NULL;
   if(name != NULL)
   {
      length = StrLen(name);
#ifdef AROS_FAST_BPTR
      name_copy = AllocVec(length + 1, MEMF_PUBLIC);
      if(name_copy != NULL)
      {
         CopyMem(name, name_copy, length + 1);
      }
#else
      name_copy = AllocVec(length + 2, MEMF_PUBLIC);
      if(name_copy != NULL)
      {
         name_copy[0] = (UBYTE)(length > 255 ? 255 : length); 
         CopyMem(name, name_copy + 1, length + 1);
      }
#endif
      else
         error = IoErr();
   }

   /* Deallocate old string and set new one */

   if(error == 0)
   {
      old_name = BADDR(entry->dol_Name);
      if(old_name != NULL)
         FreeVec(old_name);
      entry->dol_Name = MKBADDR(name_copy);
   }

   /* Store error code and return success flag */

   SetIoErr(error);
   return error == 0;
}



#else
BOOL MyRenameDosEntry(struct DosList *entry, const TEXT *name)
{
   LONG error = 0;
   UPINT length;
   TEXT *name_copy, *old_name;

   /* Allocate new string */

   name_copy = NULL;
   if(name != NULL)
   {
      length = StrLen(name);
      name_copy = AllocVec(length + 2, MEMF_PUBLIC);
      if(name_copy != NULL)
      {
         CopyMem(name, name_copy + 1, length + 1);
         *name_copy = length;
      }
      else
         error = IoErr();
   }

   /* Deallocate old string and set new one */

   if(error == 0)
   {
      old_name = BADDR(entry->dol_Name);
      if(old_name != NULL)
         FreeVec(old_name);
      entry->dol_Name = MKBADDR(name_copy);
   }

   /* Store error code and return success flag */

   SetIoErr(error);
   return error == 0;
}



#endif
