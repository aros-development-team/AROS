/*

File: commands.c
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

#include <aros/config.h>
#include <aros/debug.h>

#include "handler_protos.h"


static const TEXT default_vol_name[] = "RAM Disk";


static const UWORD examine_sizes[] =
{
   (UWORD)OFFSET(ExAllData, ed_Type),
   (UWORD)OFFSET(ExAllData, ed_Size),
   (UWORD)OFFSET(ExAllData, ed_Prot),
   (UWORD)OFFSET(ExAllData, ed_Days),
   (UWORD)OFFSET(ExAllData, ed_Comment),
   (UWORD)OFFSET(ExAllData, ed_OwnerUID)
};



/****i* ram.handler/CmdStartup *********************************************
*
*   NAME
*	CmdStartup --
*
*   SYNOPSIS
*	handler = CmdStartup(name, dev_node,
*	    proc_port)
*
*	struct Handler *CmdStartup(STRPTR, struct DeviceNode *,
*	    struct MsgPort *);
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

BOOL CmdStartup(struct Handler *handler, STRPTR name, struct DeviceNode *dev_node,
   struct MsgPort *proc_port)
{
   struct DosList *volume;
   struct Object *root_dir;
   struct MsgPort *port;
   LONG error;
   APTR base;

   /* Open extra libraries */

   error = 0;
   base = OpenLibrary(UTILITYNAME, UTILITY_VERSION);
   if(base != NULL)
      UtilityBase = base;
   else
      error = 1;

#if !(AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
   base = OpenLibrary("locale.library", LOCALE_VERSION);
   if(base != NULL)
      LocaleBase = base;
   else
      error = 1;
#endif

   /* Initialise private handler structure */

   if(error == 0)
   {
      handler->proc_port = proc_port;
      handler->block_count = MEMBLOCKS(StrSize(default_vol_name)) +
         MEMBLOCKS(sizeof(struct Handler));
      handler->min_block_size = MIN_BLOCK_SIZE;
      handler->max_block_size = MAX_BLOCK_SIZE;
      NewList((APTR)&handler->examinations);

      dev_node->dn_Task = proc_port;

      /* Create pools for zeroed, non-zeroed and public zeroed memory */

      handler->clear_pool = CreatePool(MEMF_CLEAR, CLEAR_PUDDLE_SIZE,
         CLEAR_PUDDLE_THRESH);
      if(handler->clear_pool == NULL)
         error = IoErr();
      handler->muddy_pool = CreatePool(MEMF_ANY, MUDDY_PUDDLE_SIZE,
         MUDDY_PUDDLE_THRESH);
      if(handler->muddy_pool == NULL)
         error = IoErr();
      handler->public_pool = CreatePool(MEMF_PUBLIC | MEMF_CLEAR,
         PUBLIC_PUDDLE_SIZE, PUBLIC_PUDDLE_THRESH);
      if(handler->public_pool == NULL)
         error = IoErr();

      /* Create a volume dos node */

      volume = MyMakeDosEntry(handler, default_vol_name, DLT_VOLUME);
      if(volume == NULL)
         error = IoErr();
   }

   if(error == 0)
   {
      volume->dol_Task = proc_port;
      DateStamp(&volume->dol_misc.dol_volume.dol_VolumeDate);
      volume->dol_misc.dol_volume.dol_DiskType = ID_DOS_DISK;

      /* Put volume node into dos list */

      AddDosEntry(volume);
      handler->volume = volume;

      /* Open default locale */
      if (LocaleBase)
          handler->locale = OpenLocale(NULL);

      /* Initialise notification handling */

      NewList((APTR)&handler->notifications);
      port = CreateMsgPort();
      handler->notify_port = port;
      if(port == NULL)
         error = IoErr();
   }

   /* Create the root directory and get a shared lock on it */

   if(error == 0)
   {
      root_dir = CreateObject(handler, default_vol_name, ST_ROOT, NULL);
      handler->root_dir = root_dir;
      if(root_dir != NULL)
      {
         if(LockObject(handler, root_dir, ACCESS_READ) == NULL)
            error = IoErr();
      }
      else
         error = IoErr();
   }

   /* Return result */

   SetIoErr(error);
   return error != 0;
}



/****i* ram.handler/CmdDie *************************************************
*
*   NAME
*	CmdDie --
*
*   SYNOPSIS
*	success = CmdDie(handler)
*
*	BOOL CmdDie(struct Handler *);
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

BOOL CmdDie(struct Handler *handler)
{
   struct Object *root_dir;
   LONG error = 0;

   root_dir = handler->root_dir;
   if(IsListEmpty((struct List *)&root_dir->elements)
      && IsListEmpty((struct List *)&handler->notifications)
      && IsListEmpty((struct List *)&root_dir->notifications)
      && root_dir->lock->lock_count == 1)
      DeleteHandler(handler);
   else
      error = ERROR_OBJECT_IN_USE;

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdIsFileSystem ****************************************
*
*   NAME
*	CmdIsFileSystem --
*
*   SYNOPSIS
*	result = CmdIsFileSystem()
*
*	BOOL CmdIsFileSystem();
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

BOOL CmdIsFileSystem()
{
   return TRUE;
}



/****i* ram.handler/CmdFind ************************************************
*
*   NAME
*	CmdFind --
*
*   SYNOPSIS
*	success = CmdFind(handler, handle, lock,
*	    name, mode)
*
*	BOOL CmdFind(struct Handler *, struct FileHandle *, struct Lock *,
*	    TEXT *, ULONG);
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

BOOL CmdFind(struct Handler *handler, struct FileHandle *handle,
   struct Lock *lock, const TEXT *name, ULONG type)
{
   LONG error = 0;
   ULONG mode;
   struct Object *file, *parent;

   /* Set access mode */

   if(type == MODE_NEWFILE)
      mode = ACCESS_WRITE;
   else
      mode = ACCESS_READ;

   /* Get file */

   if(type == MODE_NEWFILE && handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   if(error == 0)
   {
      file = GetHardObject(handler, lock, name, &parent);
      if(parent == NULL)
         error = IoErr();
   }

   /* Do any necessary file deletion or creation */

   if(error == 0)
   {
      name = FilePart(name);

      switch(type)
      {
      case MODE_NEWFILE:
         if(file != NULL)
         {
            if((GetRealObject(file)->protection & FIBF_DELETE) != 0)
               error = ERROR_DELETE_PROTECTED;

            if(((struct Node *)file)->ln_Pri >= 0)
               error = ERROR_OBJECT_EXISTS;
         }

         if(error == 0)
         {
            if(!AttemptDeleteObject(handler, file, FALSE))
               error = IoErr();
            file = NULL;
         }

      case MODE_READWRITE:
         if(error == 0 && file == NULL)
            file = CreateObject(handler, name, ST_FILE, parent);

      default:
         if(file == NULL)
            error = IoErr();
      }
   }

   /* Get a lock on the file */

   if(error == 0)
   {
      lock = LockObject(handler, file, mode);
      if(lock == NULL)
         error = IoErr();
      else if(type == MODE_NEWFILE)
         lock->changed = TRUE;
   }

   /* Open file */

   if(error == 0)
   {
      if(!CmdFHFromLock(handler, handle, lock))
      {
         error = IoErr();
         CmdFreeLock(handler, lock);
      }
   }

   /* Set error and return success code */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdFHFromLock ******************************************
*
*   NAME
*	CmdFHFromLock --
*
*   SYNOPSIS
*	success = CmdFHFromLock(handle, lock)
*
*	BOOL CmdFHFromLock(struct FileHandle *, struct Lock *);
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

BOOL CmdFHFromLock(struct Handler *handler, struct FileHandle *handle,
   struct Lock *lock)
{
   LONG error = 0;
   struct Opening *opening = NULL;
   struct Object *file;

   /* Check if access is allowed */

   file = (APTR)((struct FileLock *)lock)->fl_Key;
   if(((struct Node *)file)->ln_Pri != ST_FILE)
      error = ERROR_OBJECT_WRONG_TYPE;

   /* Create and initialise "opening" */

   if(error == 0)
   {
      opening = AllocPooled(handler->clear_pool, sizeof(struct Opening));
      if(opening != NULL)
      {
         opening->file = file;
         opening->block = (struct Block *)file->elements.mlh_Head;
         AddTail((struct List *)&lock->openings, (struct Node *)opening);
      }
      else
         error = IoErr();
   }

   /* Put address of opening in file handle */

   handle->fh_Arg1 = (PINT)opening;

   /* Set error and return success code */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdEnd *************************************************
*
*   NAME
*	CmdEnd --
*
*   SYNOPSIS
*	success = CmdEnd(handler, opening)
*
*	BOOL CmdEnd(struct Handler *, struct Opening *);
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

BOOL CmdEnd(struct Handler *handler, struct Opening *opening)
{
   struct Object *file, *parent;
   struct Lock *lock;
   LONG error = 0;

   /* Close file and update its date and flags */

   file = opening->file;
   lock = file->lock;
   if(lock->changed)
   {
      if(!handler->locked)
      {
         file->protection &= ~FIBF_ARCHIVE;
         DateStamp(&file->date);
         parent = file->parent;
         parent->protection &= ~FIBF_ARCHIVE;
         CopyMem(&file->date, &parent->date, sizeof(struct DateStamp));

         NotifyAll(handler, file, TRUE);
      }
      else
         error = ERROR_DISK_WRITE_PROTECTED;
   }

   if(error == 0)
   {
      Remove((APTR)opening);
      CmdFreeLock(handler, lock);
      FreePooled(handler->clear_pool, opening, sizeof(struct Opening));
   }

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdRead ************************************************
*
*   NAME
*	CmdRead --
*
*   SYNOPSIS
*	actual_length = CmdRead(opening, buffer, length)
*
*	UPINT CmdRead(struct Opening *, UBYTE *, UPINT);
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

UPINT CmdRead(struct Handler *handler, struct Opening *opening, UBYTE *buffer, UPINT length)
{
   LONG error = 0;
   struct Object *file;

   /* Read file if it isn't read protected */

   file = opening->file;

   if((file->protection & FIBF_READ) != 0)
      error = ERROR_READ_PROTECTED;

   if(error == 0)
   {
      length = ReadData(opening, buffer, length);
      error = IoErr();
   }
   else
   {
      length = -1;
   }

   /* Return number of bytes read */

   SetIoErr(error);
   return length;
}



/****i* ram.handler/CmdWrite ***********************************************
*
*   NAME
*	CmdWrite --
*
*   SYNOPSIS
*	actual_length = CmdWrite(handler, opening, buffer, length)
*
*	UPINT CmdWrite(struct Handler *, struct Opening *, UBYTE *, UPINT);
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

UPINT CmdWrite(struct Handler *handler, struct Opening *opening,
   UBYTE *buffer, UPINT length)
{
   LONG error = 0;
   struct Object *file;
   struct Lock *lock;

   /* Write to file if it isn't write protected */

   file = opening->file;
   lock = file->lock;

   if((file->protection & FIBF_WRITE) != 0)
      error = ERROR_WRITE_PROTECTED;

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   if(error == 0)
   {
      length = WriteData(handler, opening, buffer, length);
      error = IoErr();
      if(length > 0)
         lock->changed = TRUE;
   }
   else
   {
      length = -1;
   }

   /* Return number of bytes written */

   SetIoErr(error);
   return length;
}



/****i* ram.handler/CmdSeek ************************************************
*
*   NAME
*	CmdSeek --
*
*   SYNOPSIS
*	old_pos = CmdSeek(opening, offset, mode)
*
*	PINT CmdSeek(struct Opening *, PINT, LONG);
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

UPINT CmdSeek(struct Handler *handler, struct Opening *opening, PINT offset, LONG mode)
{
   struct Block *block;
   struct Object *file;
   UPINT block_pos, block_length, remainder, old_pos, new_pos;

   /* Get starting point */

   file = opening->file;
   old_pos = opening->pos;

   if(mode == OFFSET_BEGINNING)
   {
      block = (APTR)file->elements.mlh_Head;
      new_pos = block_pos = 0;
   }
   else if(mode == OFFSET_CURRENT)
   {
      block = opening->block;
      block_pos = opening->block_pos;
      new_pos = old_pos;
   }
   else
   {
      block = (APTR)file->elements.mlh_TailPred;
      block_pos = file->end_length;
      new_pos = file->length;
   }

   /* Check new position is within file */

   new_pos += offset;
   if(new_pos > file->length)
   {
      SetIoErr(ERROR_SEEK_ERROR);
      return -1;
   }

   if(offset >= 0)
   {
      /* Go forwards */

      block_length = GetBlockLength(file, block);
      remainder = offset + block_pos;
      while(remainder > block_length)
      {
         remainder -= block_length;
         block = (APTR)((struct MinNode *)block)->mln_Succ;
         block_length = GetBlockLength(file, block);
      }

      block_pos = remainder;
   }
   else
   {
      /* Go backwards */

      block_length = block_pos;
      remainder = -offset;
      while(remainder >= block_length && remainder > 0)
      {
         remainder -= block_length;
         block = (APTR)((struct MinNode *)block)->mln_Pred;
         block_length = GetBlockLength(file, block);
      }

      block_pos = block_length - remainder;
   }

   /* Record new position for next access */

   opening->block = block;
   opening->block_pos = block_pos;
   opening->pos = new_pos;

   /* Return old position */

   return old_pos;
}



/****i* ram.handler/CmdSetFileSize *****************************************
*
*   NAME
*	CmdSetFileSize --
*
*   SYNOPSIS
*	new_length = CmdSetFileSize(handler, opening, offset, mode)
*
*	PINT CmdSetFileSize(struct Handler *, struct Opening *, PINT, LONG);
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

PINT CmdSetFileSize(struct Handler *handler, struct Opening *opening,
   PINT offset, LONG mode)
{
   LONG error = 0, new_size;
   struct Object *file;
   struct Lock *lock;

   /* Change file size if it's open for writing */

   file = opening->file;
   lock = file->lock;

   if((file->protection & FIBF_WRITE) != 0)
      error = ERROR_WRITE_PROTECTED;
   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   if(error == 0)
   {
      new_size = ChangeFileSize(handler, opening, offset, mode);
      if(new_size == -1)
         error = IoErr();
      else
         lock->changed = TRUE;
   }
   else
      new_size = -1;

   /* Return new file size */

   SetIoErr(error);
   return new_size;
}



/****i* ram.handler/CmdLocateObject ****************************************
*
*   NAME
*	CmdLocateObject --
*
*   SYNOPSIS
*	lock = CmdLocateObject(handler, lock,
*	    name, mode)
*
*	struct Lock *CmdLocateObject(struct Handler *, struct Lock *,
*	    TEXT *, ULONG);
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

struct Lock *CmdLocateObject(struct Handler *handler, struct Lock *lock,
   const TEXT *name, ULONG mode)
{
   struct Object *object;

   /* Find the object and lock it */

   object = GetHardObject(handler, lock, name, NULL);

   if(object != NULL)
      lock = LockObject(handler, object, mode);
   else
      lock = NULL;

   /* Return result */

   return lock;
}



/****i* ram.handler/CmdFreeLock ********************************************
*
*   NAME
*	CmdFreeLock --
*
*   SYNOPSIS
*	success = CmdFreeLock(handler, lock)
*
*	BOOL CmdFreeLock(struct Handler *, struct Lock *);
*
*   FUNCTION
*
*   INPUTS
*	lock - May be NULL.
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

BOOL CmdFreeLock(struct Handler *handler, struct Lock *lock)
{
   struct Object *object;

   if(lock != NULL)
   {
      object = (APTR)((struct FileLock *)lock)->fl_Key;

      handler->lock_count--;
      if((--lock->lock_count) == 0)
      {
         FreePooled(handler->public_pool, lock, sizeof(struct Lock));
         object->lock = NULL;
      }
   }

   return TRUE;
}



/****i* ram.handler/CmdCopyDir *********************************************
*
*   NAME
*	CmdCopyDir --
*
*   SYNOPSIS
*	lock = CmdCopyDir(handler, lock)
*
*	struct Lock *CmdCopyDir(struct Handler *, struct Lock *);
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

struct Lock *CmdCopyDir(struct Handler *handler, struct Lock *lock)
{
   lock = FIXLOCK(handler, lock);
   lock = LockObject(handler, (APTR)((struct FileLock *)lock)->fl_Key,
      ACCESS_READ);

   return lock;
}



/****i* ram.handler/CmdCopyDirFH *******************************************
*
*   NAME
*	CmdCopyDirFH --
*
*   SYNOPSIS
*	lock = CmdCopyDirFH(handler, opening)
*
*	struct Lock *CmdCopyDirFH(struct Handler *, struct Opening *);
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

struct Lock *CmdCopyDirFH(struct Handler *handler,
   struct Opening *opening)
{
   return LockObject(handler, opening->file, ACCESS_READ);
}



/****i* ram.handler/CmdParent **********************************************
*
*   NAME
*	CmdParent --
*
*   SYNOPSIS
*	lock = CmdParent(handler, lock)
*
*	struct Lock *CmdParent(struct Handler *, struct Lock *);
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

struct Lock *CmdParent(struct Handler *handler, struct Lock *lock)
{
   struct Object *parent;
   LONG error = 0;

   lock = FIXLOCK(handler, lock);

   parent = ((struct Object *)((struct FileLock *)lock)->fl_Key)->parent;
   if(parent != NULL)
   {
      lock = LockObject(handler, parent, ACCESS_READ);
      if(lock == NULL)
         error = IoErr();
   }
   else
      lock = NULL;

   SetIoErr(error);
   return lock;
}



/****i* ram.handler/CmdParentFH ********************************************
*
*   NAME
*	CmdParentFH --
*
*   SYNOPSIS
*	lock = CmdParentFH(handler, opening)
*
*	struct Lock *CmdParentFH(struct Handler *, struct Opening *);
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

struct Lock *CmdParentFH(struct Handler *handler,
   struct Opening *opening)
{
   struct Object *parent;
   struct Lock *lock;

   parent = opening->file->parent;
   lock = LockObject(handler, parent, ACCESS_READ);

   return lock;
}



/****i* ram.handler/CmdSameLock ********************************************
*
*   NAME
*	CmdSameLock --
*
*   SYNOPSIS
*	result = CmdSameLock(handler, lock1, lock2)
*
*	BOOL CmdSameLock(struct Handler *, struct Lock *, struct Lock *);
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

BOOL CmdSameLock(struct Handler *handler, struct Lock *lock1, struct Lock *lock2)
{
   SetIoErr(0);
   return lock1 == lock2;
}



/****i* ram.handler/CmdCreateDir *******************************************
*
*   NAME
*	CmdCreateDir --
*
*   SYNOPSIS
*	lock = CmdCreateDir(handler, lock, name)
*
*	struct Lock *CmdCreateDir(struct Handler *, struct Lock *, TEXT *);
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

struct Lock *CmdCreateDir(struct Handler *handler,
   struct Lock *lock, const TEXT *name)
{
   struct Object *dir, *parent;
   LONG error = 0;

   /* Find parent directory and possible name clash */

   dir = GetHardObject(handler, lock, name, &parent);
   lock = NULL;

   /* Create a new directory */

   if(dir == NULL)
   {
      if(parent != NULL)
      {
         if(!handler->locked)
         {
            dir = CreateObject(handler, FilePart(name), ST_USERDIR, parent);
            if(dir != NULL)
            {
               lock = LockObject(handler, dir, ACCESS_WRITE);
               if(lock != NULL)
               {
                  MatchNotifyRequests(handler);
                  NotifyAll(handler, dir, FALSE);
               }
               else
               {
                  error = IoErr();
                  DeleteObject(handler, dir);
               }
            }
            else
               error = IoErr();
         }
         else
            error = ERROR_DISK_WRITE_PROTECTED;
      }
      else
         error = IoErr();
   }
   else
      error = ERROR_OBJECT_EXISTS;

   /* Set error code and return lock on new directory */

   SetIoErr(error);
   return lock;
}



/****i* ram.handler/CmdExamineObject ***************************************
*
*   NAME
*	CmdExamineObject --
*
*   SYNOPSIS
*	success = CmdExamineObject(handler, lock,
*	    info)
*
*	BOOL CmdExamineObject(struct Handler *, struct Lock *,
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

BOOL CmdExamineObject(struct Handler *handler, struct Lock *lock,
   struct FileInfoBlock *info)
{
   lock = FIXLOCK(handler, lock);
   return ExamineObject(handler, (APTR)((struct FileLock *)lock)->fl_Key,
      info);
}



/****i* ram.handler/CmdExamineFH *******************************************
*
*   NAME
*	CmdExamineFH --
*
*   SYNOPSIS
*	success = CmdExamineFH(handler, opening,
*	    info)
*
*	BOOL CmdExamineFH(struct Handler *, struct Opening *,
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

BOOL CmdExamineFH(struct Handler *handler, struct Opening *opening,
   struct FileInfoBlock *info)
{
   return ExamineObject(handler, opening->file, info);
}



/****i* ram.handler/CmdExamineNext *****************************************
*
*   NAME
*	CmdExamineNext --
*
*   SYNOPSIS
*	success = CmdExamineNext(handler, info)
*
*	BOOL CmdExamineNext(struct Handler *, struct FileInfoBlock *);
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

BOOL CmdExamineNext(struct Handler *handler, struct FileInfoBlock *info)
{
   return ExamineObject(handler, NULL, info);
}



/****i* ram.handler/CmdExamineAll ******************************************
*
*   NAME
*	CmdExamineAll --
*
*   SYNOPSIS
*	success = CmdExamineAll(handler, opening, buffer, size,
*	    type, control)
*
*	BOOL CmdExamineAll(struct Handler *, struct Lock *, UBYTE *, ULONG,
*	    ULONG, struct ExAllControl *);
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
* Note: we work under the assumption that if an error occurs, the caller
* will have to start from scratch if they want the rest of the entries.
*
*/

BOOL CmdExamineAll(struct Handler *handler, struct Lock *lock,
   UBYTE *buffer, ULONG size, ULONG type, struct ExAllControl *control)
{
   BOOL match;
   ULONG entry_count = 0;
   struct Object *object, *next_object, *real_object;
   TEXT *name, *comment, *pattern, *name_copy, *comment_copy;
   LONG error = 0;
   struct ExAllData *entry, *prev_entry = NULL;
   struct Hook *match_hook;
   UWORD struct_size, name_size, comment_size, entry_size;
   struct Examination *examination;

   /* Check we support the requested format */

   if(type > ED_COMMENT)
      error = ERROR_BAD_NUMBER;

   /* Get starting point */

   examination = (APTR)control->eac_LastKey;

   if(examination != NULL)
   {
      object = examination->next_object;
   }
   else
   {
      object = (APTR)((struct FileLock *)lock)->fl_Key;
      object = (APTR)object->elements.mlh_Head;
   }

   while(error == 0)
   {
      next_object = (struct Object *)((struct Node *)object)->ln_Succ;

      if(next_object != NULL)
      {
         struct_size = examine_sizes[type - 1];
         name = ((struct Node *)object)->ln_Name;
         name_size = StrSize(name);
         if(((struct Node *)object)->ln_Pri == ST_SOFTLINK)
            comment = &((struct Node *)object)->ln_Type;
         else
            comment = object->comment;
         comment_size = StrSize(comment);
         entry_size = struct_size + name_size;
         if(type >= ED_COMMENT)
            entry_size += comment_size;
         entry_size = (entry_size + 1) & (~0x1);

         pattern = control->eac_MatchString;
         if(pattern != NULL)
            match = MatchPatternNoCase(pattern, name);
         else
            match = TRUE;

         if(match && entry_size <= size)
         {
            /* Fill in entry */

            entry = (APTR)buffer;
            entry->ed_Next = NULL;

            real_object = GetRealObject(object);
            name_copy = buffer + struct_size;
            CopyMem(name, name_copy, name_size);
            entry->ed_Name = name_copy;

            if(type >= ED_TYPE)
               entry->ed_Type = ((struct Node *)object)->ln_Pri;

            if(type >= ED_SIZE)
               entry->ed_Size = real_object->length;

            if(type >= ED_PROTECTION)
               entry->ed_Prot = real_object->protection;

            if(type >= ED_DATE)
            {
               entry->ed_Days = real_object->date.ds_Days;
               entry->ed_Mins = real_object->date.ds_Minute;
               entry->ed_Ticks = real_object->date.ds_Tick;
            }

            if(type >= ED_COMMENT)
            {
               comment_copy = buffer + struct_size + name_size;
               CopyMem(comment, comment_copy, comment_size);
               entry->ed_Comment = comment_copy;
            }

            /* Call the filter hook if present */

            match_hook = control->eac_MatchFunc;
            if(match_hook != NULL)
               match = CallHookPkt(match_hook, &type, entry);
            else
               match = TRUE;

            /* Prepare for next entry if current one accepted */

            if(match)
            {
               buffer += entry_size;
               size -= entry_size;
               entry_count++;
               if(prev_entry != NULL)
                  prev_entry->ed_Next = entry;
               prev_entry = entry;
            }
         }
         else if(match)
            error = ERROR_NO_FREE_STORE;
      }
      else
         error = ERROR_NO_MORE_ENTRIES;

      object = next_object;
   }

   /* Running out of space isn't an error unless no entries could fit */

   if(error == ERROR_NO_FREE_STORE && entry_count != 0)
      error = 0;

   /* Prepare for subsequent call */

   if(error == 0 && examination == NULL)
   {
      examination = AllocPooled(handler->clear_pool,
         sizeof(struct Examination));
      if(examination != NULL)
         AddTail((APTR)&handler->examinations, (APTR)examination);
      else
         error = IoErr();
   }

   if(error == 0)
   {
      examination->next_object = object;
      control->eac_LastKey = (UPINT)examination;
   }
   else
   {
      CmdExamineAllEnd(handler, lock, buffer, size, type, control);
   }

   control->eac_Entries = entry_count;

   /* Return */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdExamineAllEnd ***************************************
*
*   NAME
*	CmdExamineAllEnd --
*
*   SYNOPSIS
*	CmdExamineAllEnd(handler, opening, buffer,
*	    size, type, control)
*
*	VOID CmdExamineAllEnd(struct Handler *, struct Lock *, UBYTE *,
*	    ULONG, ULONG, struct ExAllControl *);
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

VOID CmdExamineAllEnd(struct Handler *handler, struct Lock *lock,
   UBYTE *buffer, ULONG size, ULONG type, struct ExAllControl *control)
{
   struct Examination *examination;

   examination = (APTR)control->eac_LastKey;
   if(examination != NULL)
   {
      Remove((APTR)examination);
      FreePooled(handler->clear_pool, examination,
         sizeof(struct Examination));
   }

   return;
}



/****i* ram.handler/CmdInfo ************************************************
*
*   NAME
*	CmdInfo --
*
*   SYNOPSIS
*	success = CmdInfo(handler, info_data)
*
*	BOOL CmdExamineObject(struct Handler *, struct InfoData *);
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

BOOL CmdInfo(struct Handler *handler, struct InfoData *info_data)
{
   LONG disk_state;

   info_data->id_NumSoftErrors = 0;
   info_data->id_UnitNumber = 0;
   if(handler->locked)
      disk_state = ID_WRITE_PROTECTED;
   else
      disk_state = ID_VALIDATED;
   info_data->id_DiskState = disk_state;
   info_data->id_NumBlocks = handler->block_count
      + MEMBLOCKS(AvailMem(MEMF_ANY));
   info_data->id_NumBlocksUsed = handler->block_count;
   info_data->id_BytesPerBlock = MEM_BLOCKSIZE;
   info_data->id_DiskType = AROS_MAKE_ID('R','A','M',0);
   info_data->id_VolumeNode = MKBADDR(handler->volume);
   info_data->id_InUse = DOSBOOL(handler->lock_count > 1);

   return TRUE;
}



/****i* ram.handler/CmdSetProtect ******************************************
*
*   NAME
*	CmdSetProtect --
*
*   SYNOPSIS
*	success = CmdSetProtect(handler, lock, name, flags)
*
*	BOOL CmdSetProtect(struct Handler *, struct Lock *, TEXT *, ULONG);
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

BOOL CmdSetProtect(struct Handler *handler, struct Lock *lock,
   const TEXT *name, ULONG flags)
{
   LONG error = 0;
   struct Object *object;

   /* Set new protection flags if object isn't in use */

   object = GetHardObject(handler, lock, name, NULL);
   if(object != NULL)
   {
      if(handler->locked)
         error = ERROR_DISK_WRITE_PROTECTED;

      if(error == 0)
      {
         object = GetRealObject(object);
         object->protection = flags;
         NotifyAll(handler, object, TRUE);
      }
   }
   else
      error = IoErr();

   /* Return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdSetComment ******************************************
*
*   NAME
*	CmdSetComment --
*
*   SYNOPSIS
*	success = CmdSetComment(handler, lock, name, comment)
*
*	BOOL CmdSetComment(struct Handler *, struct Lock *, TEXT *, TEXT *);
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

BOOL CmdSetComment(struct Handler *handler, struct Lock *lock,
   const TEXT *name, const TEXT *comment)
{
   LONG error = 0;
   struct Object *object;
   const TEXT *p;
   TEXT ch;
   struct Locale *locale;
   PINT block_diff;

   /* Get object */

   object = GetHardObject(handler, lock, name, NULL);
   if(object == NULL)
      error = IoErr();

   /* Check comment isn't too long */

   if(StrSize(comment)
      > sizeof(((struct FileInfoBlock *)NULL)->fib_Comment))
      error = ERROR_COMMENT_TOO_BIG;

   /* Check comment doesn't have any strange characters in it */
   if ((locale = handler->locale)) {
       for(p = comment; (ch = *p) != '\0'; p++)
           if((ch & 0x80) == 0 && (!IsPrint(locale, ch) || IsCntrl(locale, ch)))
               error = ERROR_INVALID_COMPONENT_NAME;
   }

   /* Check volume isn't write-protected */

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   /* Store new comment */

   if(error == 0)
   {
      block_diff = SetString(handler, &object->comment, comment);
      if(block_diff == -1)
         error = IoErr();
      else
      {
         object->block_count += block_diff;
         handler->block_count += block_diff;
      }
   }

   /* Notify interested parties */

   if(error == 0)
   {
      NotifyAll(handler, object, FALSE);
   }

   /* Return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdRenameObject ****************************************
*
*   NAME
*	CmdRenameObject --
*
*   SYNOPSIS
*	success = CmdRenameObject(handler, old_lock, old_name,
*	    new_lock, new_name)
*
*	BOOL CmdRenameObject(struct Handler *, struct Lock *, STRPTR,
*	    struct Lock *, STRPTR);
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

BOOL CmdRenameObject(struct Handler *handler, struct Lock *old_lock,
   STRPTR old_name, struct Lock *new_lock, STRPTR new_name)
{
   struct Object *parent, *object, *duplicate, *p;
   LONG error = 0;

   /* Get object to be moved */

   object = GetHardObject(handler, old_lock, old_name, NULL);
   if(object == NULL)
      error = IoErr();

   /* Get destination directory and check if a different object already has
      the target name */

   duplicate = GetHardObject(handler, new_lock, new_name, &parent);
   if(duplicate != NULL && duplicate != object)
      error = ERROR_OBJECT_EXISTS;
   if(parent == NULL)
      error = IoErr();

   /* Check for a circular rename */

   for(p = parent; p != NULL; p = p->parent)
   {
      if(p == object)
         error = ERROR_OBJECT_NOT_FOUND;
   }

   /* Check volume isn't write-protected */

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   /* Give the object its new name */

   if(error == 0)
   {
      if(!SetName(handler, object, FilePart(new_name)))
         error = IoErr();
   }

   if(error == 0)
   {
      if(object->parent != parent)
      {
         /* Remove object from old parent and place it in new parent */

         Remove((struct Node *)object);
         AddTail((struct List *)&parent->elements, (struct Node *)object);
         object->parent = parent;
         AdjustExaminations(handler, object);
      }

      if(object != duplicate)
      {
         /* Update notifications */

         NotifyAll(handler, object, FALSE);
         UnmatchNotifyRequests(handler, object);
         MatchNotifyRequests(handler);
      }

      NotifyAll(handler, object, FALSE);
   }

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdRenameDisk ******************************************
*
*   NAME
*	CmdRenameDisk --
*
*   SYNOPSIS
*	success = CmdRenameDisk(handler, new_name)
*
*	BOOL CmdRenameDisk(struct Handler *, STRPTR);
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

BOOL CmdRenameDisk(struct Handler *handler, STRPTR new_name)
{
   LONG error = 0;

   /* Check volume isn't write-protected */

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   /* Rename volume's DOS entry and root directory */

   if(error == 0)
      if(!SetName(handler, handler->root_dir, new_name))
         error = IoErr();

   if(error == 0)
      if(!MyRenameDosEntry(handler, handler->volume, new_name))
         error = IoErr();

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdSetDate *********************************************
*
*   NAME
*	CmdSetDate --
*
*   SYNOPSIS
*	success = CmdSetDate(handler, lock, name,
*	    date)
*
*	BOOL CmdSetDate(struct Handler *, struct Lock *, STRPTR,
*	    struct DateStamp *);
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

BOOL CmdSetDate(struct Handler *handler, struct Lock *lock, STRPTR name,
   struct DateStamp *date)
{
   struct Object *object;
   LONG error = 0;

   /* Check volume isn't write-protected */

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   /* Get object and set its new date */

   if(error == 0)
   {
      object = GetHardObject(handler, lock, name, NULL);
      if(object != NULL)
      {
         object = GetRealObject(object);
         CopyMem(date, &object->date, sizeof(struct DateStamp));
      }
      else
         error = IoErr();
   }

   /* Notify interested parties */

   if(error == 0)
   {
      NotifyAll(handler, object, TRUE);
   }

   /* Return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdDeleteObject ****************************************
*
*   NAME
*	CmdDeleteObject --
*
*   SYNOPSIS
*	success = CmdDeleteObject(handler, lock, name)
*
*	BOOL CmdDeleteObject(struct Handler *, struct Lock *, STRPTR);
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

BOOL CmdDeleteObject(struct Handler *handler, struct Lock *lock,
   STRPTR name)
{
   LONG error = 0, pos = -1;
   struct Object *object;

   /* Find object and check it can be deleted */

   object = GetObject(handler, lock, name, NULL, &pos);
   if(object == NULL || pos != -1)
      error = IoErr();

   if(handler->locked)
      error = ERROR_DISK_WRITE_PROTECTED;

   if(error == 0)
   {
      if((GetRealObject(object)->protection & FIBF_DELETE) != 0)
         error = ERROR_DELETE_PROTECTED;
   }

   /* Attempt to delete object */

   if(error == 0)
   {
      if(!AttemptDeleteObject(handler, object, TRUE))
         error = IoErr();
   }

   /* Return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdCurrentVolume ***************************************
*
*   NAME
*	CmdCurrentVolume --
*
*   SYNOPSIS
*	volume = CmdCurrentVolume(handler)
*
*	struct DosList *CmdCurrentVolume(struct Handler *);
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

struct DosList *CmdCurrentVolume(struct Handler *handler)
{
   SetIoErr(0);
   return handler->volume;
}



/****i* ram.handler/CmdChangeMode ******************************************
*
*   NAME
*	CmdChangeMode --
*
*   SYNOPSIS
*	success = CmdChangeMode(handler, type, thing, new_mode)
*
*	BOOL CmdChangeMode(struct Handler *, ULONG, APTR, ULONG);
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

BOOL CmdChangeMode(struct Handler *handler, ULONG type, APTR thing, ULONG new_mode)
{
   struct Lock *lock;
   struct Opening *opening;
   LONG error = 0;

   /* Get the lock */

   if(type == CHANGE_FH)
   {
      opening = (APTR)((struct FileHandle *)thing)->fh_Arg1;
      lock = opening->file->lock;
   }
   else
      lock = thing;

   /* Change mode if possible */

   if(new_mode == ACCESS_WRITE && lock->lock_count > 1)
   {
      error = ERROR_OBJECT_IN_USE;
   }
   else
   {
      ((struct FileLock *)lock)->fl_Access = new_mode;
   }

   /* Set error code and return result */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdMakeLink ********************************************
*
*   NAME
*	CmdMakeLink --
*
*   SYNOPSIS
*	success = CmdMakeLink(handler, lock, name, reference,
*	    link_type)
*
*	BOOL CmdMakeLink(struct Handler *, struct Lock *, STRPTR, APTR,
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

BOOL CmdMakeLink(struct Handler *handler, struct Lock *lock, STRPTR name,
   APTR reference, LONG link_type)
{
   struct Object *link, *parent, *target = NULL, *master_link;
   LONG error = 0, object_type;
   PINT block_diff;
   struct MinNode *node;

   /* Find parent directory and possible name clash */

   link = GetHardObject(handler, lock, name, &parent);
   if(link != NULL)
      error = ERROR_OBJECT_EXISTS;
   else if(parent == NULL)
      error = IoErr();

   /* Determine link type */

   if(link_type == LINK_HARD)
   {
      target = (APTR)((struct FileLock *)reference)->fl_Key;
      if(((struct Node *)target)->ln_Pri == ST_FILE)
         object_type = ST_LINKFILE;
      else
         object_type = ST_LINKDIR;
   }
   else
      object_type = ST_SOFTLINK;

   /* Check volume isn't write-protected */

   if(error == 0)
   {
      if(handler->locked)
         error = ERROR_DISK_WRITE_PROTECTED;
   }

   /* Create a new link */

   if(error == 0)
   {
      link = CreateObject(handler, FilePart(name), object_type, parent);
      if(link == NULL)
         error = IoErr();
   }

   /* Store link target */

   if(error == 0)
   {
      if(link_type == LINK_HARD)
      {
         node = target->hard_link.mln_Succ;
         if(node == NULL)
         {
            master_link = link;
            AddTail((APTR)&master_link->elements, (APTR)&target->hard_link);
         }
         else
            master_link = HARDLINK(node);
         AddTail((APTR)&master_link->elements, (APTR)&link->hard_link);
      }
      else
      {
         block_diff =
            SetString(handler, (APTR)&link->soft_link_target, reference);
         if(block_diff == -1)
            error = IoErr();
         else
         {
            link->block_count += block_diff;
            handler->block_count += block_diff;
         }
      }
   }

   /* Notify */

   if(error == 0)
      NotifyAll(handler, link, FALSE);

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdReadLink ********************************************
*
*   NAME
*	CmdReadLink --
*
*   SYNOPSIS
*	success = CmdReadLink(handler, lock, name, buffer,
*	    buffer_size)
*
*	BOOL CmdReadLink(struct Handler *, struct Lock *, const TEXT *,
*	    TEXT *, LONG);
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

LONG CmdReadLink(struct Handler *handler, struct Lock *lock,
   const TEXT *name, TEXT *buffer, LONG buffer_size)
{
   struct Object *link;
   LONG error = 0, pos, length;
   const TEXT *p;

   /* Get link object */

   link = GetObject(handler, lock, name, NULL, &pos);
   if(link == NULL)
      error = IoErr();
   else if(((struct Node *)link)->ln_Pri != ST_SOFTLINK)
      error = ERROR_OBJECT_WRONG_TYPE;

   if(error == 0)
   {
      /* Copy part of path preceding link to buffer so that returned path
         will be relative to lock passed in */

      if(pos == -1)
         p = name + StrLen(name) - 1;
      else
         p = name + pos - 2;

      while(p != name && *p != '/' && *p != ':')
         p--;

      length = p - name;
      if(*p == '/' || *p == ':')
         length++;
      if(buffer_size >= length + 1)
      {
         CopyMem(name, buffer, length);
         buffer[length] = '\0';
      }
      else
         error = ERROR_BUFFER_OVERFLOW;
   }

   if(error == 0)
   {
      /* Add link target to link's parent path */

      if(AddPart(buffer, link->soft_link_target, buffer_size))
      {
         /* Add remainder of original path after link */

         if(pos != -1)
         {
            if(!AddPart(buffer, name + pos, buffer_size))
               error = ERROR_LINE_TOO_LONG;
         }
      }
      else
         error = ERROR_BUFFER_OVERFLOW;
   }

   /* Return path length or error indicator */

   if(error == ERROR_BUFFER_OVERFLOW)
      length = -2;
   else if(error != 0)
      length = -1;
   else
      length = StrLen(buffer);

   SetIoErr(error);
   return length;
}



/****i* ram.handler/CmdWriteProtect ****************************************
*
*   NAME
*	CmdWriteProtect --
*
*   SYNOPSIS
*	success = CmdWriteProtect(handler, on, key)
*
*	BOOL CmdWriteProtect(struct Handler *, BOOL, ULONG);
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

BOOL CmdWriteProtect(struct Handler *handler, BOOL on, ULONG key)
{
   LONG error = 0;
   struct Object *root_dir;

   root_dir = handler->root_dir;

   if(on)
   {
      if(!handler->locked)
         root_dir->length = key;
      else
         error = ERROR_DISK_WRITE_PROTECTED;
   }
   else
   {
      if(root_dir->length != 0 && root_dir->length != key)
         error = ERROR_INVALID_COMPONENT_NAME;
      else
         root_dir->length = 0;
   }

   if(error == 0)
      handler->locked = on;

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdFlush ***********************************************
*
*   NAME
*	CmdFlush --
*
*   SYNOPSIS
*	success = CmdFlush()
*
*	BOOL CmdFlush();
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

BOOL CmdFlush()
{
   return TRUE;
}



/****i* ram.handler/CmdAddNotify *******************************************
*
*   NAME
*	CmdAddNotify --
*
*   SYNOPSIS
*	success = CmdAddNotify(handler, request)
*
*	BOOL CmdAddNotify(struct Handler *, struct NotifyRequest *);
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

BOOL CmdAddNotify(struct Handler *handler, struct NotifyRequest *request)
{
   LONG error = 0;
   struct Notification *notification;
   struct Object *object;

   notification = AllocPooled(handler->clear_pool,
      sizeof(struct Notification));
   if(notification == NULL)
      error = IoErr();

   if(error == 0)
   {
      notification->request = request;
      request->nr_Flags &= ~NRF_MAGIC;

      object = GetHardObject(handler, NULL, request->nr_FullName, NULL);

      if(object != NULL)
      {
         AddTail((APTR)&object->notifications, (APTR)notification);
         if((request->nr_Flags & NRF_NOTIFY_INITIAL) != 0)
            Notify(handler, notification);
      }
      else
         AddTail((APTR)&handler->notifications, (APTR)notification);
   }

   /* Return success indicator */

   SetIoErr(error);
   return error == 0;
}



/****i* ram.handler/CmdRemoveNotify ****************************************
*
*   NAME
*	CmdRemoveNotify --
*
*   SYNOPSIS
*	success = CmdRemoveNotify(handler, request)
*
*	BOOL CmdRemoveNotify(struct Handler *, struct NotifyRequest *);
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

BOOL CmdRemoveNotify(struct Handler *handler, struct NotifyRequest *request)
{
   struct Notification *notification;

   notification = FindNotification(handler, request);
   Remove((APTR)notification);
   FreePooled(handler->clear_pool, notification,
      sizeof(struct Notification));

   return TRUE;
}



