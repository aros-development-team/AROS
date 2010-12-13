
#include <config.h>

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <libraries/iffparse.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/iffparse.h>
#include <proto/utility.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "version.h"

static const char version[] = "$VER: CloneAudioModes " VERS "\n\r";

long __oslibversion = 39;

#define TEMPLATE "FROM/A,TO/A,BOARD/N,VERBOSE/S"

BOOL
ProcessModeChunk( APTR* buffer, ULONG* size, ULONG* new_board );


int
main( void )
{
  struct
  {
      STRPTR from;
      STRPTR to;
      ULONG* board;
      ULONG  verbose;
  } args = { NULL, NULL, NULL, FALSE };

  struct RDArgs *rdargs;
  int            rc = RETURN_ERROR;

  rdargs = ReadArgs( TEMPLATE , (LONG *) &args, NULL );

  if( rdargs != NULL )
  {
    struct IFFHandle* from;
    struct IFFHandle* to;
    LONG              error;

    from = AllocIFF();
    to   = AllocIFF();
    
    if( from != NULL && to != NULL )
    {
      from->iff_Stream = (ULONG) Open( args.from, MODE_OLDFILE );
      
      if( from->iff_Stream != 0 )
      {
	InitIFFasDOS( from );
	
	error = OpenIFF( from, IFFF_READ );

/* 	if( error == 0 ) */
/* 	{ */
/* 	  error = PropChunk( from, ID_AHIM, ID_AUDM ); */
/* 	} */

/* 	if( error == 0 ) */
/* 	{ */
/* 	  error = PropChunk( from, ID_AHIM, ID_AUDN ); */
/* 	} */
	
	if( error == 0 )
	{
	  to->iff_Stream = (ULONG) Open( args.to, MODE_NEWFILE );

	  if( to->iff_Stream != 0 )
	  {
	    InitIFFasDOS( to );

	    error = OpenIFF( to, IFFF_WRITE );

	    if( error == 0 )
	    {
	      while( TRUE )
	      {
		struct ContextNode* cn = NULL;
		
		error = ParseIFF( from, IFFPARSE_STEP );

		if( error == 0 || error == IFFERR_EOC )
		{
		  cn = CurrentChunk( from );

		  if( cn == NULL )
		  {
		    PrintFault( ERROR_OBJECT_NOT_FOUND, "CurrentChunk()" );
		    break;
		  }

		  if( cn->cn_Type != ID_AHIM )
		  {
		    Printf( "%s: Not an AHIM IFF file.\n", (ULONG) args.from );
		    break;
		  }
		}

		
		if( error == 0 )
		{
		  char idbuf[ 5 ];
		  APTR  buffer;
		  ULONG size;

		  error = 0;
		  
		  if( from->iff_Depth > 1 )
		  {
		    if( args.verbose )
		    {
		      IDtoStr( cn->cn_ID, idbuf );

		      Printf( "Processing %s (%ld bytes) ... ",
			      (ULONG) idbuf, cn->cn_Size );
		    }

		    size   = cn->cn_Size;
		    buffer = AllocVec( size, MEMF_ANY );

		    if( buffer == NULL )
		    {
		      PrintFault( ERROR_NO_FREE_STORE, idbuf );
		      break;
		    }

		    error = ReadChunkBytes( from, buffer, size );

		    if( cn->cn_ID == ID_AUDM )
		    {
		      if( ! ProcessModeChunk( &buffer, &size, args.board ) )
		      {
			PrintFault( ERROR_NO_FREE_STORE, idbuf );
			break;
		      }
		    }
		  }
		  
		  if( error >= 0 )
		  {
		    error = PushChunk( to,
				       cn->cn_Type, cn->cn_ID, IFFSIZE_UNKNOWN );
		  }

		  if( from->iff_Depth > 1 )
		  {
		    if( error >= 0 )
		    {
		      error = WriteChunkBytes( to, buffer, size );
		    }

		    FreeVec( buffer );
		  
		    if( error < 0 )
		    {
		      Printf( "%s: IFF error code %ld\n", (ULONG)
			      (ULONG) args.to, error );
		      break;
		    }
		  }
		}
		else if( error == IFFERR_EOC )
		{
		  error = PopChunk( to );

		  if( error != 0 )
		  {
		    Printf( "%s: IFF error code %ld\n", (ULONG) args.to, error );
		    break;
		  }

		  if( from->iff_Depth > 1 && args.verbose )
		  {
		    Printf( "OK.\n" );
		  }
		}
		else if( error == IFFERR_EOF )
		{
		  if( args.verbose )
		  {
		    Printf( "Done!\n" );
		  }

		  rc = RETURN_OK;
		  break;
		}
		else
		{
		  Printf( "%ld\n", error );
		  break;
		}
	      } // while
	    }
	    else
	    {
	      Printf( "%s: IFF error code %ld\n", (ULONG) args.to, error );
	    }
	  
	    Close( (BPTR) to->iff_Stream );

	    if( rc != RETURN_OK )
	    {
	      Printf( "Deleting destination file %s.\n", (ULONG) args.to );
	      DeleteFile( args.to );
	    }
	  }
	  else
	  {
	    PrintFault( IoErr(), args.to );
	  }
	}
	else
	{
	  Printf( "%s: IFF error code %ld\n", (ULONG) (ULONG) args.from, error );
	}
	
	Close( (BPTR) from->iff_Stream );
      }
      else
      {
	PrintFault( IoErr(), args.from );
      }
    }

    FreeIFF( from );
    FreeIFF( to );
    
    FreeArgs( rdargs );
  }
  else
  {
    PrintFault( IoErr(), "CloneAudioModes" );
  }

  return rc;
}

BOOL
ProcessModeChunk( APTR* buffer, ULONG* size, ULONG* new_board )
{
  struct TagItem* tstate;
  struct TagItem* tag;

  ULONG           board     = new_board == NULL ? 1 : *new_board - 1;
  
  char*           name      = NULL;
  ULONG           name_size = 0;
  ULONG           tag_size  = 4;

  APTR            new_buffer;
  ULONG*          ptr;

  tstate = *buffer;

  while( ( tag = NextTagItem( &tstate ) ) )
  {
    tag_size += sizeof( struct TagItem );

    if( tag->ti_Tag == AHIDB_AudioID )
    {
      if( new_board == NULL )
      {
	board = ( ( tag->ti_Data >> 12 ) & 15 ) + 1;
      }
    }
    else if( tag->ti_Tag == AHIDB_Name )
    {
      name = tag->ti_Data + *buffer;

      name_size = strlen( name ) + 1 + 2;
    }
  }

  new_buffer = AllocVec( tag_size + name_size, MEMF_ANY );

  if( new_buffer != NULL )
  {
    char* card;
    char* mode;

    ptr = new_buffer;

    tstate = *buffer;

    while( ( tag = NextTagItem( &tstate ) ) )
    {
      *ptr++ = tag->ti_Tag;

      if( tag->ti_Tag == AHIDB_AudioID )
      {
	*ptr++ = ( tag->ti_Data & 0xffff0fff ) | ( board << 12 );
      }
      else if( tag->ti_Tag == AHIDB_Name )
      {
	*ptr++ = tag_size;
      }
      else
      {
	*ptr++ = tag->ti_Data;
      }
    }

    *ptr++ = TAG_DONE;

    card = name;
      
    while( *name != ':' && *name != 0 )
    {
      name++;
    }

    if( name[ -2 ] == '-' && isdigit( name[ -1 ] ) )
    {
      name[ -2 ] = 0;
    }

    *name++ = 0;

    mode = name;

    name_size = sprintf( (char*) ptr, "%s-%d:%s", card, (int) board + 1, mode );
    name_size += 1;
  }
  
  FreeVec( *buffer );
  
  *buffer = new_buffer;
  *size   = tag_size + name_size;

  return new_buffer != NULL;
}
