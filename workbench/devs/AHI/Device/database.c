/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

#include <exec/memory.h>
#include <exec/semaphores.h>
#include <dos/dostags.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include <string.h>

#include "ahi_def.h"
#include "database.h"
#include "debug.h"
#include "header.h"
#include "misc.h"

static ULONG AddModeFile ( UBYTE *filename );

#ifdef __MORPHOS__
#define IS_MORPHOS 1
#else
#define IS_MORPHOS 0
#endif

/******************************************************************************
** Audio Database *************************************************************
******************************************************************************/

struct AHI_AudioMode
{
  struct MinNode          ahidbn_MinNode;
  struct TagItem          ahidbn_Tags[0];

  /* Taglist, mode name and driver name follows.
     Size variable. Use FreeVec() to free node. */
};

/*
** Lock the database for read access. Return NULL if database not present.
*/

struct AHI_AudioDatabase *
LockDatabase(void)
{
  struct AHI_AudioDatabase *audiodb;

  Forbid();
  
  audiodb = (struct AHI_AudioDatabase *) FindSemaphore(ADB_NAME);

  if(audiodb != NULL)
  {
    ObtainSemaphoreShared((struct SignalSemaphore *) audiodb);
  }

  Permit();

  return audiodb;
}

/*
** Lock the database for write access. Create it if not present.
*/

struct AHI_AudioDatabase *
LockDatabaseWrite(void)
{
  struct AHI_AudioDatabase *audiodb;

  Forbid();

  audiodb = (struct AHI_AudioDatabase *) FindSemaphore(ADB_NAME);

  if(audiodb != NULL)
  {
    ObtainSemaphore((struct SignalSemaphore *) audiodb);
  }
  else
  {
    audiodb = (struct AHI_AudioDatabase *)
        AllocVec(sizeof(struct AHI_AudioDatabase), MEMF_PUBLIC|MEMF_CLEAR);

    if(audiodb != NULL)
    {

      NewList( (struct List *) &audiodb->ahidb_AudioModes);

      audiodb->ahidb_Semaphore.ss_Link.ln_Name = audiodb->ahidb_Name;
      audiodb->ahidb_Semaphore.ss_Link.ln_Pri  = 20;
      strcpy(audiodb->ahidb_Semaphore.ss_Link.ln_Name, ADB_NAME);

      AddSemaphore((struct SignalSemaphore *) audiodb);
      ObtainSemaphore((struct SignalSemaphore *) audiodb);
    }
  }
  Permit();

  return audiodb;
}

void
UnlockDatabase ( struct AHI_AudioDatabase *audiodb )
{
  if(audiodb)
  {
    ReleaseSemaphore((struct SignalSemaphore *) audiodb);
  }
}

struct TagItem *
GetDBTagList ( struct AHI_AudioDatabase *audiodb,
               ULONG id )
{
  struct AHI_AudioMode *node;
  struct TagItem       *rc = NULL;

  if((audiodb != NULL) && (id != AHI_INVALID_ID))
  {
    for(node=(struct AHI_AudioMode *)audiodb->ahidb_AudioModes.mlh_Head;
        node->ahidbn_MinNode.mln_Succ;
        node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ)
    {
      if(id == GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags))
      {
        rc = node->ahidbn_Tags;
        break;
      }
    }
  }

  return rc;
}


/******************************************************************************
** AHI_NextAudioID ************************************************************
******************************************************************************/

/****** ahi.device/AHI_NextAudioID ******************************************
*
*   NAME
*       AHI_NextAudioID -- iterate current audio mode identifiers
*
*   SYNOPSIS
*       next_ID = AHI_NextAudioID( last_ID );
*       D0                         D0
*
*       ULONG AHI_NextAudioID( ULONG );
*
*   FUNCTION
*       This function is used to iterate through all current AudioIDs in
*       the audio database.
*
*   INPUTS
*       last_ID - previous AudioID or AHI_INVALID_ID if beginning iteration.
*
*   RESULT
*       next_ID - subsequent AudioID or AHI_INVALID_ID if no more IDs.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_GetAudioAttrsA(), AHI_BestAudioIDA()
*
****************************************************************************
*
*/

ULONG
_AHI_NextAudioID( ULONG           id,
		  struct AHIBase* AHIBase )
{
  struct AHI_AudioDatabase *audiodb;
  struct AHI_AudioMode *node;
  ULONG  nextid=AHI_INVALID_ID;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    Debug_NextAudioID(id);
  }

  audiodb = LockDatabase();

  if(audiodb != NULL)
  {
    node = (struct AHI_AudioMode *) audiodb->ahidb_AudioModes.mlh_Head;

    if(id != AHI_INVALID_ID)
    {
      while(node != NULL)
      {
        ULONG thisid;
        
        thisid = GetTagData(AHIDB_AudioID,AHI_INVALID_ID,node->ahidbn_Tags);
        node = (struct AHI_AudioMode *) node->ahidbn_MinNode.mln_Succ;

        if(thisid == id)
        {
          break;
        }
      }
    }

    while(node && node->ahidbn_MinNode.mln_Succ)
    {
      if( GetTagData( AHIDB_MultTable, FALSE, node->ahidbn_Tags ) )
      {
	// Pretend the "Fast" modes are not here
	node = (struct AHI_AudioMode*) node->ahidbn_MinNode.mln_Succ;
      }
      else
      {
	nextid = GetTagData(AHIDB_AudioID, AHI_INVALID_ID, node->ahidbn_Tags);
	break;
      }
    }

    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>0x%08lx\n",nextid);
  }

  return nextid;
}


/******************************************************************************
** AHI_AddAudioMode ***********************************************************
******************************************************************************/

/****i* ahi.device/AHI_AddAudioMode *****************************************
*
*   NAME
*       AHI_AddAudioMode -- add an audio mode to the database (V4)
*
*   SYNOPSIS
*       success = AHI_AddAudioMode( DBtags );
*       D0                          A0
*
*       ULONG AHI_AddAudioMode( struct TagItem *, UBYTE *, UBYTE *);
*
*   FUNCTION
*       Adds the audio mode described by a taglist to the audio mode
*       database. If the database does not exists, it will be created.
*
*   INPUTS
*       DBtags - Tags describing the properties of this mode.
*
*   RESULT
*       success - FALSE if the mode could not be added.
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

ULONG
_AHI_AddAudioMode( struct TagItem* DBtags,
		   struct AHIBase* AHIBase )
{
  struct AHI_AudioDatabase *audiodb;
  struct AHI_AudioMode *node;
  ULONG nodesize = sizeof(struct AHI_AudioMode), tagitems = 0;
  ULONG datalength = 0, namelength = 0, driverlength = 0, dvrbasenamelength = 0;
  struct TagItem *tstate = DBtags, *tp, *tag;
  ULONG rc = FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    Debug_AddAudioMode(DBtags);
  }

// Remove old mode if present in database
  AHI_RemoveAudioMode( GetTagData(AHIDB_AudioID, AHI_INVALID_ID, DBtags));

// Now add the new mode

  audiodb = LockDatabaseWrite();

  if(audiodb != NULL)
  {

// Find total size

    while( (tag = NextTagItem(&tstate)) != NULL )
    {

      if(tag->ti_Data) switch(tag->ti_Tag)
      {
        case AHIDB_Data:
          datalength    = ((ULONG *)tag->ti_Data)[0];
          nodesize     += datalength;
          break;

        case AHIDB_Name:
          namelength    = strlen((UBYTE *)tag->ti_Data)+1;
          nodesize     += namelength;
          break;

        case AHIDB_Driver:
          driverlength  = strlen((UBYTE *)tag->ti_Data)+1;
          nodesize     += driverlength;
          break;

        case AHIDB_DriverBaseName:
          dvrbasenamelength    = strlen((UBYTE *)tag->ti_Data)+1;
          nodesize     += dvrbasenamelength;
          break;
      }

      nodesize += sizeof(struct TagItem);
      tagitems++;
    }

    nodesize += sizeof(struct TagItem);  // The last TAG_END
    tagitems++;

    node = AllocVec(nodesize, MEMF_PUBLIC|MEMF_CLEAR);

    if(node != NULL)
    {
      tp      = node->ahidbn_Tags;
      tstate  = DBtags;
      while( (tag = NextTagItem(&tstate)) != NULL)
      {
        if(tag->ti_Data) switch(tag->ti_Tag)
        {
          case AHIDB_Data:
            tp->ti_Data = ((IPTR) &node->ahidbn_Tags[tagitems]);
            CopyMem((APTR)tag->ti_Data, (APTR)tp->ti_Data, datalength);
            break;

          case AHIDB_Name:
            tp->ti_Data = ((IPTR) &node->ahidbn_Tags[tagitems]) + datalength;
            strcpy((UBYTE *)tp->ti_Data, (UBYTE *)tag->ti_Data);
            break;

          case AHIDB_Driver:
            tp->ti_Data= ((IPTR) &node->ahidbn_Tags[tagitems]) + datalength + namelength;
            strcpy((UBYTE *)tp->ti_Data, (UBYTE *)tag->ti_Data);
            break;

          case AHIDB_DriverBaseName:
            tp->ti_Data = ((IPTR) &node->ahidbn_Tags[tagitems]) + datalength + namelength + driverlength;
            strcpy((UBYTE *)tp->ti_Data, (UBYTE *)tag->ti_Data);
            break;

          default:
            tp->ti_Data = tag->ti_Data;
            break;
        }
        tp->ti_Tag = tag->ti_Tag;
        tp++;
      }
      tp->ti_Tag = TAG_DONE;

      AddHead((struct List *) &audiodb->ahidb_AudioModes, (struct Node *) node);
      rc = TRUE;
    }

unlock:
    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** AHI_RemoveAudioMode ********************************************************
******************************************************************************/

/****i* ahi.device/AHI_RemoveAudioMode **************************************
*
*   NAME
*       AHI_RemoveAudioMode -- remove a audio mode to the database (V4)
*
*   SYNOPSIS
*       success = AHI_RemoveAudioMode( ID );
*       D0                             D0
*
*       ULONG AHI_RemoveAudioMode( ULONG );
*
*   FUNCTION
*       Removes the audio mode from the audio mode database.
*
*   INPUTS
*       ID - The audio ID of the mode to be removed, or AHI_INVALID_ID.
*
*   RESULT
*       success - FALSE if the mode could not be removed.
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

ULONG
_AHI_RemoveAudioMode( ULONG           id,
		      struct AHIBase* AHIBase )
{
  struct AHI_AudioMode *node;
  struct AHI_AudioDatabase *audiodb;
  ULONG rc=FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    Debug_RemoveAudioMode(id);
  }


  /* Why ?? */

  audiodb = LockDatabaseWrite();

  if(audiodb != NULL)
  {
    UnlockDatabase(audiodb);
  }

  audiodb = LockDatabaseWrite();

  if(audiodb != NULL)
  {
    if(id != AHI_INVALID_ID)
    {
      for(node=(struct AHI_AudioMode *)audiodb->ahidb_AudioModes.mlh_Head;
          node->ahidbn_MinNode.mln_Succ;
          node=(struct AHI_AudioMode *)node->ahidbn_MinNode.mln_Succ)
      {
        if(id == GetTagData(AHIDB_AudioID, AHI_INVALID_ID, node->ahidbn_Tags))
        {
          Remove((struct Node *) node);
          FreeVec(node);
          rc = TRUE;
          break;
        }
      }

      // Remove the entire database if it's empty

      Forbid();

      if(audiodb->ahidb_AudioModes.mlh_Head->mln_Succ == NULL)
      {
        UnlockDatabase(audiodb);

        audiodb = (struct AHI_AudioDatabase *) FindSemaphore(ADB_NAME);

        if(audiodb != NULL)
        {
          RemSemaphore((struct SignalSemaphore *) audiodb);
          FreeVec(audiodb);
        }

        audiodb = NULL;

      }

      Permit();

    }
    UnlockDatabase(audiodb);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** AHI_LoadModeFile ***********************************************************
******************************************************************************/

/****i* ahi.device/AHI_LoadModeFile *****************************************
*
*   NAME
*       AHI_LoadModeFile -- Add all modes in a mode file to the database (V4)
*
*   SYNOPSIS
*       success = AHI_LoadModeFile( name );
*       D0                          A0
*
*       ULONG AHI_LoadModeFile( STRPTR );
*
*   FUNCTION
*       This function takes the name of a file or a directory and either
*       adds all modes in the file or the modes of all files in the
*       directory to the audio mode database. Directories inside the
*       given directory will not be recursed. The file format is IFF-AHIM.
*
*   INPUTS
*       name - A pointer to the name of a file or directory.
*
*   RESULT
*       success - FALSE on error. Check dos.library/IOErr() for more
*           information.
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

ULONG
_AHI_LoadModeFile( UBYTE*          name,
		   struct AHIBase* AHIBase )
{
  ULONG rc=FALSE;
  struct FileInfoBlock  *fib;
  BPTR  lock,thisdir;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    Debug_LoadModeFile(name);
  }

  SetIoErr(0);

  fib = AllocDosObject(DOS_FIB, TAG_DONE);

  if(fib != NULL)
  {
    lock = Lock(name, ACCESS_READ);

    if(lock != 0)
    {
      if(Examine(lock,fib))
      {
        if(fib->fib_DirEntryType>0) // Directory?
        {
          thisdir = CurrentDir(lock);

          while(ExNext(lock, fib))
          {
            if(fib->fib_DirEntryType>0)
            {
              continue;     // AHI_LoadModeFile(fib->fib_FileName); for recursion
            }
            else
            {
#if 0
              rc = AddModeFile(fib->fib_FileName);

              if(!rc)
              {
                break;
              }
#else
              // Try to load. Just continue if failing.
              AddModeFile(fib->fib_FileName);
	      rc = TRUE;
#endif
            }
          }
          if(IoErr() == ERROR_NO_MORE_ENTRIES)
          {
            SetIoErr(0);
          }

          CurrentDir(thisdir);
        }
        else  // Plain file
        {
          rc = AddModeFile(name);
        }
      }

      UnLock(lock);
    }

    FreeDosObject(DOS_FIB,fib);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}

/* AddModeFile **********************************************************/

static ULONG
AddModeFile ( UBYTE *filename )
{
  struct IFFHandle *iff;
  struct StoredProperty *name,*data;
  struct CollectionItem *ci;
  struct TagItem *tag,*tstate;
  struct TagItem extratags[]=
  {
    { AHIDB_Driver,         0 },
    { AHIDB_Data,           0 },
    { AHIDB_DriverBaseName, (IS_MORPHOS ? "MOSSYS:DEVS/AHI" : "DEVS:AHI") },
    { TAG_MORE,             0 }
  };
  ULONG rc=FALSE;

  iff = AllocIFF();

  if(iff != NULL)
  {

    iff->iff_Stream = (IPTR)Open(filename, MODE_OLDFILE);

    if(iff->iff_Stream != 0)
    {
      InitIFFasDOS(iff);

      if(!OpenIFF(iff, IFFF_READ))
      {

        if(!(PropChunk(iff,       ID_AHIM, ID_AUDN)
          || PropChunk(iff,       ID_AHIM, ID_AUDD)
          || CollectionChunk(iff, ID_AHIM, ID_AUDM)
          || StopOnExit(iff,      ID_AHIM, ID_FORM)))
        {
          if(ParseIFF(iff, IFFPARSE_SCAN) == IFFERR_EOC)
          {
            name = FindProp(iff,       ID_AHIM, ID_AUDN);
            data = FindProp(iff,       ID_AHIM, ID_AUDD);
            ci   = FindCollection(iff, ID_AHIM, ID_AUDM);

            rc = TRUE;

            if(name != NULL)
            {
              char            driver_name[ 128 ];
              struct Library* driver_base;

              rc = FALSE;

              if( name->sp_Size <= 0 )
              {
                Req( "%s:\nAUDN chunk has illegal size: %ld.", 
                     filename, name->sp_Size );
              }
              else
              {
                LONG   i;
                STRPTR s;

                // Make sure string is NUL-terminated
                
                for( s = (STRPTR) name->sp_Data;
                     (APTR) s < name->sp_Data + name->sp_Size;
                     ++s )
                {
                  if( *s == 0 )
                  {
                    rc = TRUE;
                    break;
                  }
                }
                
                if( !rc )
                {
                  Req( "%s:\nAUDN chunk is not NUL-terminated.", 
                       filename );
                }
              }

              extratags[0].ti_Data = (IPTR) name->sp_Data;

              // Now verify that the driver can really be opened

              strcpy( driver_name, IS_MORPHOS ? "MOSSYS:DEVS/AHI/" : "DEVS:AHI/" );
              strncat( driver_name, name->sp_Data, 100 );
              strcat( driver_name, ".audio" );

              driver_base = OpenLibrary( driver_name, DriverVersion );
              if( driver_base == NULL )
              {
                if (IS_MORPHOS == 0)
                {
                  rc = FALSE;
                }
                else
                {
                  // Make it MOSSYS:DEVS:AHI/...
                  //                    ^
                  driver_name[7 + 4] = ':';

				// Try "DEVS:AHI/...."
				//
                  driver_base = OpenLibrary( driver_name + 7, DriverVersion );
                  if( driver_base == NULL )
                  {
                    rc = FALSE;
                  }
                  else
                  {
                    // It is a DEVS:AHI driver!
                    extratags[2].ti_Data = (IPTR) "DEVS:AHI";

                    CloseLibrary( driver_base );
                  }
                }
              }
              else
              {
                CloseLibrary( driver_base );
              }
            }

            if(data != NULL)
            {
              if( name->sp_Size <= 0 )
              {
                Req( "%s:\nAUDD chunk has illegal size: %ld.", 
                     filename, name->sp_Size );

                rc = FALSE;
              }

              extratags[1].ti_Data = (IPTR) data->sp_Data;
            }

            while(rc && ci != NULL)
            {
              // Relocate loaded taglist

              tstate = (struct TagItem *) ci->ci_Data;

              while( rc && ( tag = NextTagItem( &tstate ) ) != NULL )
              {
		EndianSwap( sizeof tag->ti_Tag, &tag->ti_Tag );
		EndianSwap( sizeof tag->ti_Data, &tag->ti_Data );
		
                if(tag->ti_Tag & (AHI_TagBaseR ^ AHI_TagBase))
                {
                  tag->ti_Data += ci->ci_Data;
                }
               
                rc = FALSE;
 
                switch( tag->ti_Tag )
                {
                  case AHIDB_Name:
                  {
                    // Make sure the string is within the chunk and NUL-term.
                    
                    if( tag->ti_Data <  (IPTR) ci->ci_Data || 
                        tag->ti_Data >= (IPTR) ci->ci_Data + ci->ci_Size )
                    {
                      Req( "%s:\nAUDM chunk contains an invalid string.", 
                           filename );
                    }
                    else
                    {
                      LONG   i;
                      STRPTR s;
                      
                      // Make sure string is NUL-terminated
                
                      for( s = (STRPTR) tag->ti_Data;
                           (APTR) s < ci->ci_Data + ci->ci_Size;
                           ++s )
                      {
                        if( *s == 0 )
                        {
                          rc = TRUE;
                          break;
                        }
                      }
                    }
                    
                    break;
                  }

                  default:
                    rc = TRUE;
                    break;
                }

                if( !rc )
                {
                  Req( "%s:\nAUDM chunk contains a string that is not "
                       "NUL-terminated.", filename  );
                }
              }

              if( rc )
              {
                // Link taglists

                extratags[3].ti_Data = (IPTR) ci->ci_Data;

                rc = AHI_AddAudioMode(extratags);

                ci = ci->ci_Next;
              }
            }
          }
        }
        CloseIFF(iff);
      }
      Close((BPTR) iff->iff_Stream);
    }
    FreeIFF(iff);
  }

  return rc;
}
