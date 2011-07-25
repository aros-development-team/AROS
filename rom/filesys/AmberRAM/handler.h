/*

File: handler.h
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

#ifndef HANDLER_H
#define HANDLER_H


#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/notify.h>
#include <dos/exall.h>
#include <exec/memory.h>
#include <exec/initializers.h>
#include <utility/utility.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/locale.h>
#include <proto/utility.h>


#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif                         
#endif

#ifndef MEM_BLOCKSHIFT
#define MEM_BLOCKSHIFT ((sizeof(APTR) == 8) ? 4 : 3)
#endif

#define MIN(A, B) (((A) < (B))? (A): (B))
#define MEMBLOCKS(A) ((((A) - 1) >> (MEM_BLOCKSHIFT)) + 1)
#define HARDLINK(A)\
   ((struct Object *)\
   (((BYTE *)(A)) - (UPINT)&((struct Object *)NULL)->hard_link))
#define DOSBOOL(A) ((A)? DOSTRUE: DOSFALSE)
#define FIXLOCK(handler, lock)\
   ((lock != NULL)? (lock): (handler)->root_dir->lock)

#define MAX_NAME_SIZE (sizeof(((struct FileInfoBlock *)NULL)->fib_FileName))
#define MIN_BLOCK_SIZE 64
#define MAX_BLOCK_SIZE 0x8000
#define CLEAR_PUDDLE_SIZE (8 * 1024)
#define CLEAR_PUDDLE_THRESH (4 * 1024)
#define MUDDY_PUDDLE_SIZE (16 * 1024)
#define MUDDY_PUDDLE_THRESH (8 * 1024)
#define PUBLIC_PUDDLE_SIZE (4 * 1024)
#define PUBLIC_PUDDLE_THRESH (2 * 1024)
#define DOS_VERSION 39
#define UTILITY_VERSION 37
#define LOCALE_VERSION 38


struct Block
{
   struct MinNode node;
   UWORD length;            /* number of data bytes */
};


struct Lock
{
   struct FileLock lock;
   UPINT lock_count;
   BOOL changed;
   struct MinList openings;
};


struct Object
{
   struct Node node;
   UWORD end_length;        /* number of valid bytes in last data block */
   struct MinList elements;
   UPINT length;            /* logical length in bytes for a file */
   UPINT block_count;       /* number of memory blocks */
   struct Lock *lock;
   struct Object *parent;
   struct DateStamp date;
   ULONG protection;
   TEXT *comment;
   struct MinNode hard_link;
   struct MinList notifications;
   struct Block start_block;   /* a zero-length block */
};
#define soft_link_target comment


struct Opening
{
   struct MinNode node;
   struct Object *file;
   UPINT pos;
   struct Block *block;
   UPINT block_pos;
};


struct Notification
{
   struct MinNode node;
   struct NotifyRequest *request;
};


struct Examination
{
   struct MinNode node;
   struct Object *next_object;
};


struct Handler
{
   struct Object *root_dir;
   struct DosList *volume;
   struct MsgPort *proc_port;
   struct MsgPort *notify_port;
   UPINT block_count;
   UPINT lock_count;
   struct Locale *locale;
   ULONG min_block_size;
   ULONG max_block_size;
   BOOL locked;
   struct MinList notifications;
   struct MinList examinations;
   APTR clear_pool;
   APTR muddy_pool;
   APTR public_pool;

   TEXT b_buffer[256];
   TEXT b_buffer2[256];
   struct DosLibrary *DOSBase;
   struct UtilityBase *UtilityBase;
   struct LocaleBase *LocaleBase;
};

/* FIXME: Remove these #define xxxBase hacks
   Do not use this in new code !
*/
#define LocaleBase (handler->LocaleBase)
#define UtilityBase (handler->UtilityBase)
#define DOSBase (handler->DOSBase)

#endif


