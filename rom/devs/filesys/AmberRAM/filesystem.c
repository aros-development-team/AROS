/*

File: filesystem.c
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


static struct Block *AddDataBlock(struct Handler *handler,
   struct Object *file, UPINT length);
static VOID FreeDataBlock(struct Handler *handler, struct Object *file,
   struct Block *block);
static struct Block *GetLastBlock(struct Object *file);



/****i* ram.handler/DeleteHandler ******************************************
*
*   NAME
*	DeleteHandler --
*
*   SYNOPSIS
*	DeleteHandler(handler)
*
*	VOID DeleteHandler(struct Handler *);
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

VOID DeleteHandler(struct Handler *handler)
{
   struct DosList *volume;
   struct Object *root_dir;
   struct Lock *root_lock;
   APTR base;

   if(handler != NULL)
   {
      root_dir = handler->root_dir;

      if(root_dir != NULL)
      {
         root_lock = root_dir->lock;
         if(root_lock != NULL)
            CmdFreeLock(handler, root_lock);
         DeleteObject(handler, root_dir);
      }

      volume = handler->volume;
      if(volume != NULL)
      {
         RemDosEntry(volume);
         MyFreeDosEntry(handler, volume);
      }

      DeletePool(handler->public_pool);
      DeletePool(handler->muddy_pool);
      DeletePool(handler->clear_pool);

      DeleteMsgPort(handler->notify_port);

      /* Close libraries */

      base = LocaleBase;
      if(base != NULL) {
      	if (handler->locale)
             CloseLocale(handler->locale);
         CloseLibrary(base);
      }

      base = UtilityBase;
      if(base != NULL)
         CloseLibrary(base);

      FreeMem(handler, sizeof(struct Handler));
   }

   return;
}



/****i* ram.handler/CreateObject *******************************************
*
*   NAME
*	CreateObject --
*
*   SYNOPSIS
*	object = CreateObject(handler, name, type,
*	    parent)
*
*	struct Object *CreateObject(struct Handler *, TEXT *, BYTE,
*	    struct Object *);
*
*   FUNCTION
*	Creates a new filesystem object. The existence of a duplicate object
*	must be checked for beforehand.
*
*   INPUTS
*	name - The name of the new object, without a path prefixed.
*	type - The type of object to create.
*	parent - The new object's parent directory, or NULL for none.
*
*   RESULT
*	object - The newly created object, or NULL for failure.
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

struct Object *CreateObject(struct Handler *handler, const TEXT *name,
   BYTE type, struct Object *parent)
{
   struct Object *object = NULL;
   LONG error = 0;

   /* Get the real parent in case it's a hard link */

   if(parent != NULL)
      parent = GetRealObject(parent);

   /* Create a new object structure */

   object = AllocPooled(handler->clear_pool, sizeof(struct Object));
   if(object == NULL)
      error = ERROR_DISK_FULL;

   if(error == 0)
   {
      object->block_count = MEMBLOCKS(sizeof(struct Object));
      handler->block_count += MEMBLOCKS(sizeof(struct Object));

      NewList((struct List *)&object->elements);
      ((struct Node *)object)->ln_Pri = type;
      object->parent = parent;
      DateStamp(&object->date);
      NewList((struct List *)&object->notifications);

      if(type == ST_FILE)
         AddTail((APTR)&object->elements, (APTR)&object->start_block);

      if(name != NULL)
      {
         if(!SetName(handler, object, name))
            error = IoErr();
      }

      if(parent != NULL)
      {
         AddTail((struct List *)&parent->elements, (struct Node *)object);
         CopyMem(&object->date, &parent->date, sizeof(struct DateStamp));
      }
   }

   if(error == 0)
      MatchNotifyRequests(handler);

   if(error != 0)
   {
      DeleteObject(handler, object);
      object = NULL;
   }

   /* Return the new object */

   SetIoErr(error);
   return object;
}



/****i* ram.handler/AttemptDeleteObject ************************************
*
*   NAME
*	AttemptDeleteObject -- Attempt to delete an object.
*
*   SYNOPSIS
*	success = AttemptDeleteObject(handler, object)
*
*	BOOL AttemptDeleteObject(struct Handler *, struct Object *);
*
*   FUNCTION
*	Attempts to delete the specified object. Note that this function
*	does not check permissions.
*
*   INPUTS
*	object - the object to be deleted, or NULL for no action.
*
*   RESULT
*	success - success indicator.
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

BOOL AttemptDeleteObject(struct Handler *handler, struct Object *object,
   BOOL notify)
{
   LONG error = 0;
   BYTE object_type;

   if(object != NULL)
   {
      /* Check for a non-empty, unlinked directory */

      object_type = ((struct Node *)object)->ln_Pri;
      if(object_type == ST_USERDIR && object->hard_link.mln_Succ == NULL
         && !IsListEmpty((struct List *)&object->elements))
         error = ERROR_DIRECTORY_NOT_EMPTY;

      /* Ensure the object either isn't in use or is only a link */

      if(object->lock != NULL)
         error = ERROR_OBJECT_IN_USE;

      /* Delete object and notify anyone who's interested */

      if(error == 0)
      {
         if(notify)
            NotifyAll(handler, object, FALSE);
         UnmatchNotifyRequests(handler, object);
         AdjustExaminations(handler, object);
         DeleteObject(handler, object);
      }
   }

   /* Return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/DeleteObject *******************************************
*
*   NAME
*	DeleteObject --
*
*   SYNOPSIS
*	success = DeleteObject(handler, object)
*
*	BOOL DeleteObject(struct Handler *, struct Object *);
*
*   FUNCTION
*
*   INPUTS
*	object - the object to be deleted, or NULL for no action.
*
*   RESULT
*	None.
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

VOID DeleteObject(struct Handler *handler, struct Object *object)
{
   BYTE object_type;
   struct Block *block;
   struct MinNode *node;
   struct Object *real_object, *master_link, *heir;
   PINT block_diff;

   if(object != NULL)
   {
      object_type = ((struct Node *)object)->ln_Pri;
      real_object = GetRealObject(object);

      /* Remove the object from its directory */

      if(object->parent != NULL)
         Remove((struct Node *)object);

      /* Delete a hard link */

      if(object_type == ST_LINKFILE || object_type == ST_LINKDIR)
      {
         node = real_object->hard_link.mln_Succ;
         master_link = HARDLINK(node);
         node = node->mln_Succ;
         Remove((APTR)&object->hard_link);
         if(object == master_link)
         {
            if(node->mln_Succ != NULL)
            {
               master_link = HARDLINK(node);
               while((node = (APTR)RemHead((APTR)&object->elements))
                  != NULL)
                  AddTail((APTR)&master_link->elements, (APTR)node);
            }
            else
            {
               real_object->hard_link.mln_Succ = NULL;
               real_object->hard_link.mln_Pred = NULL;
            }
         }
      }

      /* Delete a linked object */

      else if((node = object->hard_link.mln_Succ) != NULL)
      {
         master_link = HARDLINK(node);
         heir = HARDLINK(RemTail((APTR)&master_link->elements));

         /* Swap names and comments */

         block_diff = SwapStrings((TEXT **)&((struct Node *)heir)->ln_Name,
            (TEXT **)&((struct Node *)object)->ln_Name);
         block_diff += SwapStrings(&heir->comment, &object->comment);
         object->block_count += block_diff;
         heir->block_count -= block_diff;

         /* Put object in its new directory */

         object->parent = heir->parent;
         AddTail((APTR)&object->parent->elements, (APTR)object);

         if(heir == master_link)
         {
            real_object->hard_link.mln_Succ = NULL;
            real_object->hard_link.mln_Pred = NULL;
         }

         /* Prepare to destroy "heir" link */

         Remove((APTR)heir);
         object = heir;
      }

      /* Delete an unlinked object */

      else
      {
         /* Remove all blocks if the object is a file */

         while((block = (APTR)RemTail((struct List *)&object->elements))
            != NULL)
         {
            if(block->length != 0)
               FreePooled(handler->muddy_pool, block,
                  sizeof(struct Block) + block->length);
         }
      }

      /* Free object's memory */

      SetString(handler, (TEXT **)&((struct Node *)object)->ln_Name, NULL);
      SetString(handler, &object->comment, NULL);
      handler->block_count -= object->block_count;
      FreePooled(handler->clear_pool, object, sizeof(struct Object));
   }

   return;
}



/****i* ram.handler/GetHardObject ******************************************
*
*   NAME
*	GetHardObject -- Find a non-soft-link object given a lock and path.
*
*   SYNOPSIS
*	object = GetHardObject(handler, lock, name,
*	    parent)
*
*	struct Object *GetHardObject(struct Handler *, struct Lock *, TEXT *,
*	    struct Object **);
*
*   FUNCTION
*	Looks for the object corresponding to the specified lock and path
*	combination. If any part of the path is a soft link, NULL is returned
*	and the appropriate error code is set.
*
*   INPUTS
*	lock - .
*	name - .
*	parent - parent directory will be returned here (will be NULL if
*	    path is invalid or contains a soft link).
*
*   RESULT
*	object - The located object, or NULL if not found.
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

struct Object *GetHardObject(struct Handler *handler, struct Lock *lock,
   const TEXT *name, struct Object **parent)
{
   struct Object *object;

   object = GetObject(handler, lock, name, parent, NULL);
   if(IoErr() == ERROR_IS_SOFT_LINK)
      object = NULL;

   return object;
}


/****i* ram.handler/GetObject **********************************************
*
*   NAME
*	GetObject -- Find an object given a lock and path.
*
*   SYNOPSIS
*	object = GetObject(handler, lock, name,
*	    parent, remainder_pos)
*
*	struct Object *GetObject(struct Handler *, struct Lock *, TEXT *,
*	    struct Object **, LONG *);
*
*   FUNCTION
*	Looks for the object corresponding to the specified lock and path
*	combination. If any part of the path is a soft link, the link object
*	is returned and ERROR_IS_SOFT_LINK is set. IoErr() will be zero if a
*	hard object is returned.
*
*   INPUTS
*	lock - .
*	name - .
*	parent - parent directory will be returned here (will be NULL if
*	    path is invalid or contains a soft link).
*
*   RESULT
*	object - The located object, or NULL if not found.
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

struct Object *GetObject(struct Handler *handler, struct Lock *lock,
   const TEXT *name, struct Object **parent, LONG *remainder_pos)
{
   const TEXT *p;
   TEXT ch, buffer[MAX_NAME_SIZE + 1];
   LONG error = 0, pos = 0;
   struct Object *object, *old_object;

   /* Skip device name */

   for(ch = *(p = name); ch != ':' && ch != '\0'; ch = *(++p));
   if(ch == ':')
      pos = p - name + 1;

   /* Get object referenced by lock */

   lock = FIXLOCK(handler, lock);
   object = (APTR)((struct FileLock *)lock)->fl_Key;
   old_object = object->parent;

   /* Traverse textual portion of path */

   while(pos != -1 && error == 0)
   {
      pos = SplitName(name, '/', buffer, pos, MAX_NAME_SIZE);
      if(*buffer != '\0')
      {
         if(((struct Node *)object)->ln_Pri > 0)
         {
            old_object = object;
            object = GetRealObject(object);
            object = (struct Object *)
               FindNameNoCase(handler, (struct List *)&object->elements, buffer);
            if(object != NULL)
            {
               /* Check for and handle a soft link */

               if(((struct Node *)object)->ln_Pri == ST_SOFTLINK)
               {
                  if(remainder_pos != NULL)
                     *remainder_pos = pos;
                  error = ERROR_IS_SOFT_LINK;
               }
            }
            else
               error = ERROR_OBJECT_NOT_FOUND;
         }
         else
         {
            error = ERROR_OBJECT_NOT_FOUND;
            object = NULL;
            old_object = NULL;
         }
      }
      else if(pos != -1)
      {
         object = object->parent;
         if(object == NULL)
            error = ERROR_OBJECT_NOT_FOUND;
      }
   }

   /* Record the parent directory of the supplied path, if it exists */

   if(parent != NULL)
   {
      if(pos == -1 && error != ERROR_IS_SOFT_LINK)
         *parent = old_object;
      else
         *parent = NULL;
   }

   /* Return the located object */

   SetIoErr(error);
   return object;
}



/****i* ram.handler/ChangeFileSize *****************************************
*
*   NAME
*	ChangeFileSize --
*
*   SYNOPSIS
*	new_size = ChangeFileSize(handler, opening, offset,
*	   mode)
*
*	PINT ChangeFileSize(struct Handler *, struct Opening *, PINT,
*	   LONG);
*
*   FUNCTION
*	If there is not enough space for the requested size, the file size
*	will remain at its initial value and -1 will be returned.
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

PINT ChangeFileSize(struct Handler *handler, struct Opening *opening,
   PINT offset, LONG mode)
{
   PINT length, new_length, remainder, end_length, full_length;
   UPINT diff, old_pos, block_count;
   struct Block *block, *end_block;
   struct Object *file;
   LONG error = 0;
   struct MinList *openings;
   struct Opening *tail;

   /* Get origin */

   file = opening->file;

   if(mode == OFFSET_BEGINNING)
      new_length = 0;
   else if(mode == OFFSET_CURRENT)
      new_length = opening->pos;
   else
      new_length = file->length;

   /* Check new size won't be negative */

   if(-offset > new_length)
   {
      SetIoErr(ERROR_SEEK_ERROR);
      return -1;
   }

   /* Get new length */

   new_length += offset;
   length = file->length;
   block_count = file->block_count;

   /* Get full length (including any unused area in last block) */

   old_pos = opening->pos;
   full_length = length;
   end_length = file->end_length;

   block = GetLastBlock(file);
   full_length += block->length - end_length;

   /* Change size */

   if(new_length > length)
   {
      /* Add the required number of data bytes */

      remainder = new_length - full_length;
      end_length -= remainder;

      while(remainder > 0)
      {
         block = AddDataBlock(handler, file, remainder);
         if(block != NULL)
         {
            end_length = remainder;
            remainder -= block->length;
         }
         else
            remainder = 0;
      }

      /* Remove new blocks upon failure */

      if(block == NULL)
      {
         new_length = -1;
         error = IoErr();
         ChangeFileSize(handler, opening, length, OFFSET_BEGINNING);
      }
      else
         file->end_length = end_length;
   }
   else if(length != new_length)
   {
      /* Remove surplus blocks from the file */

      block = NULL;
      while(full_length > new_length)
      {
         FreeDataBlock(handler, file, block);
         block = (APTR)RemTail((APTR)&file->elements);
         full_length -= block->length;
      }
      end_block = (APTR)file->elements.mlh_TailPred;
      file->end_length = end_length = end_block->length;

      /* Allocate a suitably sized end block and copy old data to it */

      diff = new_length - full_length;
      file->length = full_length;
      CmdSeek(handler, opening, 0, OFFSET_END);

      if(ChangeFileSize(handler, opening, diff, OFFSET_END) != -1)
      {
         WriteData(handler, opening,
            ((UBYTE *)block) + sizeof(struct Block), diff);
         FreeDataBlock(handler, file, block);
      }
      else
         AddTail((struct List *)&file->elements, (APTR)block);
   }

   /* Store new file size */

   if(new_length != -1)
      file->length = new_length;
   handler->block_count += file->block_count - block_count;

   /* Re-establish old seek position */

   opening->pos = old_pos;

   /* Adjust all openings for this file to ensure that their position isn't
      past the new EOF and that their block and block position are valid */

   openings = &file->lock->openings;
   opening = (APTR)openings->mlh_Head;
   tail = (APTR)&openings->mlh_Tail;

   while(opening != tail)
   {
      if(opening->pos > file->length)
      {
         opening->pos = file->length;
      }
      CmdSeek(handler, opening, opening->pos, OFFSET_BEGINNING);
      opening = (APTR)((struct MinNode *)opening)->mln_Succ;
   }

   SetIoErr(error);
   return new_length;
}



/****i* ram.handler/ReadData ***********************************************
*
*   NAME
*	ReadData --
*
*   SYNOPSIS
*	read_length = ReadData(opening, buffer, length)
*
*	UPINT ReadData(struct Opening *, UBYTE *, UPINT);
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

UPINT ReadData(struct Opening *opening, UBYTE *buffer, UPINT length)
{
   struct Block *block;
   UPINT block_pos, block_length, read_length = 0, remainder;
   struct Object *file;

   /* Fill buffer until request has been fulfilled or EOF is reached */

   block = opening->block;
   block_pos = opening->block_pos;
   file = opening->file;

   remainder = file->length - opening->pos;
   if(length > remainder)
      length = remainder;
   remainder = length;

   while(remainder > 0)
   {
      /* Get number of remaining valid bytes in this block */

      block_length = block->length - block_pos;

      /* Get next block if end of current one has been reached */

      if(block_length == 0)
      {
         block = (struct Block *)((struct MinNode *)block)->mln_Succ;
         block_pos = 0;
         /*read_length = 0;*/
         block_length = block->length;
      }

      /* Copy block contents into the caller's buffer */

      read_length = MIN(remainder, block_length);
      CopyMem(((UBYTE *)block) + sizeof(struct Block) + block_pos, buffer,
         read_length);
      remainder -= read_length;
      buffer += read_length;
      block_pos += read_length;
   }

   /* Record new position for next access */

   opening->block = block;
   opening->block_pos = block_pos;
   opening->pos += length;

   /* Return number of bytes read */

   return length;
}



/****i* ram.handler/WriteData **********************************************
*
*   NAME
*	WriteData --
*
*   SYNOPSIS
*	write_length = WriteData(handler, opening, buffer, length)
*
*	UPINT WriteData(struct Handler *, struct Opening *, UBYTE *, UPINT);
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

UPINT WriteData(struct Handler *handler, struct Opening *opening,
   UBYTE *buffer, UPINT length)
{
   struct Block *block, *new_block, *end_block;
   struct Object *file;
   UPINT block_pos, block_length, write_length, remainder = length,
      old_block_count, file_length, pos;
   LONG error = 0;

   file = opening->file;
   file_length = file->length;
   old_block_count = file->block_count;
   pos = opening->pos;
   block = opening->block;
   block_pos = opening->block_pos;

   while(remainder > 0 && error == 0)
   {
      block_length = block->length - block_pos;

      /* Move on to next block if end of current block reached */

      if(block_length == 0)
      {
         block = (struct Block *)((struct MinNode *)block)->mln_Succ;
         block_pos = 0;

         /* Add another block to the file if required */

         if(((struct MinNode *)block)->mln_Succ == NULL)
         {
            new_block = AddDataBlock(handler, opening->file, remainder);
            if(new_block != NULL)
            {
               block = new_block;
               file->end_length = 0;
            }
            else
               error = IoErr();
         }

         block_length = block->length;
      }

      /* Write as much as possible to the current block */

      if(error == 0)
      {
         write_length = MIN(remainder, block_length);
         CopyMem(buffer,
            ((UBYTE *)block) + sizeof(struct Block) + block_pos,
            write_length);
         remainder -= write_length;
         buffer += write_length;
         block_pos += write_length;
      }
   }

   /* Recalculate length of used portion of last block */

   end_block = GetLastBlock(file);
   if(block == end_block && block_pos >= file->end_length)
      file->end_length = block_pos;

   /* Update file size, volume size and current position */

   length -= remainder;
   pos += length;
   if(pos > file_length)
      file->length = pos;
   opening->pos = pos;
   opening->block = block;
   opening->block_pos = block_pos;
   handler->block_count += file->block_count - old_block_count;

   /* Return number of bytes written */

   SetIoErr(error);
   return length;
}



/****i* ram.handler/LockObject *********************************************
*
*   NAME
*	LockObject --
*
*   SYNOPSIS
*	lock = LockObject(handler, object,
*	    access)
*
*	struct Lock *LockObject(struct Handler *handler, struct Object *,
*	    LONG);
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

struct Lock *LockObject(struct Handler *handler, struct Object *object,
   LONG access)
{
   struct Lock *lock;
   LONG error = 0, lock_access;

   object = GetRealObject(object);
   lock = object->lock;

   if(lock == NULL)
   {
      lock = AllocPooled(handler->public_pool, sizeof(struct Lock));

      if(lock != NULL)
      {
         object->lock = lock;

         ((struct FileLock *)lock)->fl_Key = (PINT)object;
         ((struct FileLock *)lock)->fl_Access = access;
         ((struct FileLock *)lock)->fl_Task = handler->proc_port;
         ((struct FileLock *)lock)->fl_Volume = MKBADDR(handler->volume);

         NewList((struct List *)&lock->openings);
      }
      else
         error = IoErr();
   }
   else
   {
      lock_access = ((struct FileLock *)lock)->fl_Access;
      if(access == ACCESS_WRITE || lock_access == ACCESS_WRITE)
      {
         lock = NULL;
         error = ERROR_OBJECT_IN_USE;
      }
   }

   if(lock != NULL)
   {
      lock->lock_count++;
      handler->lock_count++;
   }

   SetIoErr(error);

   return lock;
}



/****i* ram.handler/ExamineObject ******************************************
*
*   NAME
*	ExamineObject --
*
*   SYNOPSIS
*	success = ExamineObject(handler, object,
*	    info)
*
*	BOOL ExamineObject(struct Handler *, struct Object *,
*	    struct FileInfoBlock *);
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

BOOL ExamineObject(struct Handler *handler, struct Object *object,
   struct FileInfoBlock *info)
{
   BOOL success;
   struct Object *next_object;
   TEXT *s;
   LONG entry_type;

   if(object == NULL)
   {
      object = (struct Object *)info->fib_DiskKey;
      next_object = (APTR)((struct Node *)object)->ln_Succ;
   }
   else
      next_object = (APTR)object->elements.mlh_Head;

   if(next_object != NULL)
   {
      /* Fill in information from object */

      entry_type = ((struct Node *)object)->ln_Pri;
      info->fib_DirEntryType = entry_type;
      info->fib_EntryType = entry_type;
      s = ((struct Node *)object)->ln_Name;
      info->fib_FileName[0] = StrSize(s) - 1;
      CopyMem(s, &info->fib_FileName[1], info->fib_FileName[0]);
      s = object->comment;
      if(s != NULL && ((struct Node *)object)->ln_Pri != ST_SOFTLINK) {
         info->fib_Comment[0] = StrSize(s) - 1;
         CopyMem(s, &info->fib_Comment[1], info->fib_Comment[0]);
      } else
         info->fib_Comment[0] = '\0';
      info->fib_NumBlocks = object->block_count;

      /* Fill in information from real object */

      object = GetRealObject(object);

      info->fib_Protection = object->protection;
      info->fib_Size = object->length;
      CopyMem(&object->date, &info->fib_Date, sizeof(struct DateStamp));

      /* Prepare for next examination */

      success = TRUE;
      info->fib_DiskKey = (PINT)next_object;
   }
   else
   {
      success = FALSE;
      SetIoErr(ERROR_NO_MORE_ENTRIES);
   }

   return success;
}



/****i* ram.handler/AdjustExaminations *************************************
*
*   NAME
*	AdjustExaminations -- Prevent ExAll() getting confused or crashing
*
*   SYNOPSIS
*	AdjustExaminations(handler, object)
*
*	VOID AdjustExaminations(struct Handler *, struct Object *);
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

VOID AdjustExaminations(struct Handler *handler, struct Object *object)
{
   struct Examination *examination, *tail;

   /* Adjust any ExAll()s that were due to examine the object */

   examination = (APTR)handler->examinations.mlh_Head;
   tail = (APTR)&handler->examinations.mlh_Tail;

   while(examination != tail)
   {
      if(examination->next_object == object)
         examination->next_object = (APTR)((struct Node *)object)->ln_Succ;
      examination = (APTR)((struct MinNode *)examination)->mln_Succ;
   }

   return;
}


/****i* ram.handler/SetName ************************************************
*
*   NAME
*	SetName --
*
*   SYNOPSIS
*	success = SetName(handler, object, name)
*
*	BOOL SetName(struct Handler *, struct Object *, TEXT *);
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

BOOL SetName(struct Handler *handler, struct Object *object,
   const TEXT *name)
{
   LONG error = 0;
   const TEXT *p;
   TEXT ch;
   struct Locale *locale;
   PINT block_diff;

   /* Check name isn't too long */

   if(StrSize(name) > MAX_NAME_SIZE)
      error = ERROR_INVALID_COMPONENT_NAME;

   /* Check name doesn't have any strange characters in it */

   if ((locale = handler->locale)) {
       for(p = name; (ch = *p) != '\0'; p++)
           if((ch & 0x80) == 0
               && (!IsPrint(locale, ch) || IsCntrl(locale, ch) || ch == ':'))
               error = ERROR_INVALID_COMPONENT_NAME;
   }

   /* Store new name */

   if(error == 0)
   {
      block_diff = SetString(handler,
         (TEXT **)&((struct Node *)object)->ln_Name, name);
      error = IoErr();
   }

   if(error == 0)
   {
      object->block_count += block_diff;
      handler->block_count += block_diff;
   }

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/AddDataBlock *******************************************
*
*   NAME
*	AddDataBlock --
*
*   SYNOPSIS
*	block = AddDataBlock(file, length)
*
*	struct Block *AddDataBlock(struct Object *, UPINT);
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
*	Attempts to allocate a smaller block if the requested size cannot be
*	obtained.
*
*   BUGS
*
*   SEE ALSO
*
****************************************************************************
*
*/

static struct Block *AddDataBlock(struct Handler *handler,
   struct Object *file, UPINT length)
{
   struct Block *block;
   UPINT alloc_size;
   ULONG limit;

   /* Ensure block size is within limits */

   alloc_size = sizeof(struct Block) + length;

   limit = handler->max_block_size;
   if(alloc_size > limit)
      alloc_size = limit;

   limit = handler->min_block_size;
   if(alloc_size < limit)
      alloc_size = limit;

   /* Allocate a multiple of the memory block size */

   block = NULL;
   while(block == NULL && alloc_size >= limit)
   {
      alloc_size = ((alloc_size - 1) & (~MEM_BLOCKMASK)) + MEM_BLOCKSIZE;
      block = AllocPooled(handler->muddy_pool, alloc_size);
      if(block == NULL)
         alloc_size >>= 1;
   }

   /* Add the block to the end of the file */

   if(block != NULL)
   {
      AddTail((struct List *)&file->elements, (struct Node *)block);
      block->length = alloc_size - sizeof(struct Block);
      file->block_count += alloc_size >> MEM_BLOCKSHIFT;
   }
   else
      SetIoErr(ERROR_DISK_FULL);

   /* Return the new block */

   return block;
}



/****i* ram.handler/FreeDataBlock ******************************************
*
*   NAME
*	FreeDataBlock --
*
*   SYNOPSIS
*	FreeDataBlock(file, block)
*
*	VOID FreeDataBlock(struct Object *, struct Block *);
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

static VOID FreeDataBlock(struct Handler *handler, struct Object *file,
   struct Block *block)
{
   UPINT alloc_size;

   if(block != NULL)
   {
      alloc_size = sizeof(struct Block) + block->length;
      file->block_count -= alloc_size >> MEM_BLOCKSHIFT;
      FreePooled(handler->muddy_pool, block, alloc_size);
   }

   return;
}



/****i* ram.handler/GetRealObject ******************************************
*
*   NAME
*	GetRealObject -- Dereference a hard link if necessary.
*
*   SYNOPSIS
*	real_object = GetRealObject(object)
*
*	struct Object *GetRealObject(struct Object *);
*
*   FUNCTION
*	If the specified object is a file, directory or soft link, the same
*	object is returned. If the object is a hard link, the link target is
*	returned instead.
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

struct Object *GetRealObject(struct Object *object)
{
   struct MinNode *node, *pred;

   /* Get first node in list */

   node = &object->hard_link;
   if(node->mln_Pred != NULL)
   {
      while((pred = node->mln_Pred) != NULL)
         node = pred;
      node = node->mln_Succ;
   }

   /* Get object from node address */

   object = HARDLINK(node);
   return object;
}



/****i* ram.handler/GetBlockLength *****************************************
*
*   NAME
*	GetBlockLength -- Get the number of utilised bytes in a block.
*
*   SYNOPSIS
*	length = GetBlockLength(file, block)
*
*	UPINT GetBlockLength(struct Object *, struct Block *);
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

UPINT GetBlockLength(struct Object *file, struct Block *block)
{
   UPINT length;

   if(block == (APTR)file->elements.mlh_TailPred)
      length = file->end_length;
   else
      length = block->length;

   return length;
}



/****i* ram.handler/GetLastBlock *******************************************
*
*   NAME
*	GetLastBlock --
*
*   SYNOPSIS
*	block = GetLastBlock(file)
*
*	struct Block *GetLastBlock(struct Object *);
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

static struct Block *GetLastBlock(struct Object *file)
{
   struct Block *block;

   block = (APTR)file->elements.mlh_TailPred;

   return block;
}



