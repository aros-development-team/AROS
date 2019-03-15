/***************************************************************************/
/*                                                                         */
/*  ftsystem.c                                                             */
/*                                                                         */
/*    ANSI-specific FreeType low-level system interface (body).            */
/*                                                                         */
/*  Copyright 1996-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* This file contains the default interface used by FreeType to access   */
  /* low-level, i.e. memory management, i/o access as well as thread       */
  /* synchronisation.  It can be replaced by user-specific routines if     */
  /* necessary.                                                            */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_INTERNAL_DEBUG_H
#include FT_SYSTEM_H
#include FT_ERRORS_H
#include FT_TYPES_H

#include <stdio.h>
#include <stdlib.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

//#define DEBUG 1
#include <aros/debug.h>


  /*************************************************************************/
  /*                                                                       */
  /*                       MEMORY MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/

  /*************************************************************************/
  /*                                                                       */
  /* It is not necessary to do any error checking for the                  */
  /* allocation-related functions.  This will be done by the higher level  */
  /* routines like FT_Alloc() or FT_Realloc().                             */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_alloc                                                           */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory allocation function.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory :: A pointer to the memory object.                          */
  /*                                                                       */
  /*    size   :: The requested size in bytes.                             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of newly allocated block.                              */
  /*                                                                       */
  FT_CALLBACK_DEF( void* )
  ft_alloc( FT_Memory  memory,
            long       size )
  {
    ULONG memsize = size + sizeof(ULONG);
    void* retval;
    APTR MemPool = (APTR)memory->user;

    D(bug("Entering ft_alloc(memory=%x,size=%ld)\n", memory, size));
    
    retval = AllocPooled(MemPool, memsize);

    D(bug("Allocated retval=%x\n", (void*)retval));

    *((ULONG *)retval) = memsize;
    
    D(bug("Leaving ft_alloc retval=%x\n", (void*)((ULONG*)retval + 1)));

    return (void*)((ULONG*)retval + 1);
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_free                                                            */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory release function.                                       */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory  :: A pointer to the memory object.                         */
  /*                                                                       */
  /*    block   :: The address of block in memory to be freed.             */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_free( FT_Memory  memory,
           void*      block )
  {
    APTR mem = (APTR)((char*)block - sizeof(ULONG));
    APTR MemPool = (APTR)memory->user;

    D(bug("Entering ft_free(memory=%x, block=%x)\n", memory, block));
      
    FreePooled( MemPool, mem, *((ULONG*)mem) );

    D(bug("Leaving ft_free\n"));
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_realloc                                                         */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The memory reallocation function.                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A pointer to the memory object.                        */
  /*                                                                       */
  /*    cur_size :: The current size of the allocated memory block.        */
  /*                                                                       */
  /*    new_size :: The newly requested size in bytes.                     */
  /*                                                                       */
  /*    block    :: The current address of the block in memory.            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The address of the reallocated memory block.                       */
  /*                                                                       */
  FT_CALLBACK_DEF( void* )
  ft_realloc( FT_Memory  memory,
              long       cur_size,
              long       new_size,
              void*      block )
  {
    void* retval;

    D(bug("Entering ft_free(memory=%x, cur_size=%ld, new_size=%ld, block=%x)\n",
	  memory, cur_size, new_size, block));

    if ((retval = ft_alloc( memory, new_size )) != NULL)
    {
        CopyMem( block, retval, cur_size );
        ft_free( memory, block );
    }

    D(bug("Leaving ft_free retval=%x\n", retval));

    return retval;
  }


  /*************************************************************************/
  /*                                                                       */
  /*                     RESOURCE MANAGEMENT INTERFACE                     */
  /*                                                                       */
  /*************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_io

  /* We use the macro STREAM_FILE for convenience to extract the       */
  /* system-specific stream handle from a given FreeType stream object */
#define STREAM_FILE( stream )  ( (BPTR)stream->descriptor.pointer )


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_ansi_stream_close                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to close a stream.                                    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  FT_CALLBACK_DEF( void )
  ft_ansi_stream_close( FT_Stream  stream )
  {
    D(bug("Entering ft_ansi_stream_close(stream=%x)\n", stream));
    
    Close( STREAM_FILE( stream ) );

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = 0;

    D(bug("Entering ft_ansi_stream_close\n"));
  }


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    ft_ansi_stream_io                                                  */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The function to open a stream.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    stream :: A pointer to the stream object.                          */
  /*                                                                       */
  /*    offset :: The position in the data stream to start reading.        */
  /*                                                                       */
  /*    buffer :: The address of buffer to store the read data.            */
  /*                                                                       */
  /*    count  :: The number of bytes to read from the stream.             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    The number of bytes actually read.                                 */
  /*                                                                       */
  FT_CALLBACK_DEF( unsigned long )
  ft_ansi_stream_io( FT_Stream       stream,
                     unsigned long   offset,
                     unsigned char*  buffer,
                     unsigned long   count )
  {
    BPTR file;
    unsigned long actcount;
    
    D(bug("Entering ft_ansi_stream_io(stream=%x,offset=%ld,buffer=%x,count=%ld)\n",
	  stream, offset, buffer, count));
    file = STREAM_FILE( stream );

    Seek( file, offset, OFFSET_BEGINNING );
    if (count>0)
      actcount = (unsigned long)Read( file, buffer, count );
    else
      actcount = 0;
    
    D(bug("Leaving ft_ansi_stream_io actcount=%ld",actcount));
    
    return actcount;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname )
  {
    BPTR file;

    if ( !stream )
      return FT_Err_Invalid_Stream_Handle;

    file = Open( filepathname, MODE_OLDFILE );
    if ( !file )
    {
      FT_ERROR(( "FT_Stream_Open:" ));
      FT_ERROR(( " could not open `%s'\n", filepathname ));

      return FT_Err_Cannot_Open_Resource;
    }

    Seek( file, 0, OFFSET_END );
    stream->size = Seek( file, 0, OFFSET_BEGINNING );

    stream->descriptor.pointer = (APTR)file;
    stream->pathname.pointer   = (char*)filepathname;
    stream->pos                = 0;

    stream->read  = ft_ansi_stream_io;
    stream->close = ft_ansi_stream_close;

    FT_TRACE1(( "FT_Stream_Open:" ));
    FT_TRACE1(( " opened `%s' (%d bytes) successfully\n",
                filepathname, stream->size ));

    return FT_Err_Ok;
  }


#ifdef FT_DEBUG_MEMORY

  extern FT_Int
  ft_mem_debug_init( FT_Memory  memory );

  extern void
  ft_mem_debug_done( FT_Memory  memory );

#endif


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( FT_Memory )
  FT_New_Memory( void )
  {
    FT_Memory  memory;
    APTR MemPool;

    D(bug("Entering FT_New_Memory\n"));
      
    MemPool = CreatePool(MEMF_ANY, 2048, 256);
    if (MemPool)
    {
      memory = (FT_Memory)AllocPooled( MemPool, sizeof ( *memory ) );
      if ( memory )
      {
        memory->user    = (void*)MemPool;
        memory->alloc   = ft_alloc;
        memory->realloc = ft_realloc;
        memory->free    = ft_free;
#ifdef FT_DEBUG_MEMORY
        ft_mem_debug_init( memory );
#endif
      }
    }
    else
      memory=NULL;

    D(bug("Leaving FT_New_Memory memory=%x\n", memory));

    return memory;
  }


  /* documentation is in ftobjs.h */

  FT_EXPORT_DEF( void )
  FT_Done_Memory( FT_Memory  memory )
  {
#ifdef FT_DEBUG_MEMORY
    ft_mem_debug_done( memory );
#endif
    DeletePool((APTR)memory->user);
  }


/* END */
