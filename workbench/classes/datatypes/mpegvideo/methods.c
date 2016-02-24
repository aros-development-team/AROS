
/*
**
**  $VER: dispatch.c 1.11 (7.11.97)
**  mpegvideo.datatype 1.11
**
**  DataTypes class method implementations
**
**  Written 1996/1997 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

/* main includes */
#include "classbase.h"

/* ansi includes */
#include <limits.h>

/* project includes */
#include "mpegutil.h"

/****** mpegvideo.datatype/OM_NEW *********************************************
*
*    NAME
*        OM_NEW -- Create a mpegvideo.datatype object.
*
*    FUNCTION
*        The OM_NEW method is used to create an instance of the
*        mpegvideo.datatype class.  This method is passed to the superclass
*        first. After this, mpegvideo.datatype loads it's preference file
*        (and the sound file, if one was specified), parses the whole video
*        stream, putting pictures into memory.
*        Any fatal error aborts the load, non-serious errors are reported.
*
*        Subclasses of mpegvideo.datatype are not supported. Any attempt to
*        create a subclass object of mpegvideo.datatype will be rejected
*        by this method.
*
*    ATTRIBUTES
*        The following attributes can be specified at creation time.
*
*        DTA_SourceType (ULONG) -- Determinates the type of DTA_Handle
*            attribute. Currently, only a source type of DTST_FILE is
*            supported. If any other type was set in a given DTA_SourceType,
*            OM_NEW will be rejected with result2 == ERROR_OBJECT_WRONG_TYPE.
*            Defaults to DTST_FILE.
*
*        DTA_Handle (BPTR) -- If DTA_SourceType is DTST_FILE, the given file
*            handle will be used as an mpeg video stream to read in.
*
*    RESULT
*        If the object was created a pointer to the object is returned,
*        otherwise NULL is returned.
*
******************************************************************************
*
*/
IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct ClassBase     *classbase = (struct ClassBase *)(cl -> cl_UserData);
    struct TagItem *ti;
    IPTR retval = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));
    D(bug("[mpegvideo.datatype] %s: o @ 0x%p, cl @ 0x%p\n", __func__, o, cl));
    D(bug("[mpegvideo.datatype] %s:OCLASS @ 0x%p\n", __func__, OCLASS(o)));
          /* We only support DTST_FILE as DTA_SourceType type */
          if ((ti = FindTagItem( DTA_SourceType, (((struct opSet *)msg) -> ops_AttrList) )) != NULL)
          {
            if( (ti -> ti_Data) != DTST_FILE )
            {
              SetIoErr( ERROR_OBJECT_WRONG_TYPE );

              return (IPTR)NULL;
            }
          }

          /* This must not be a subclass of mpegvideo.datatype
           * (not implemented yet)
           */
          if( o == (Object *)cl )
          {
            if( ( retval = DoSuperMethodA( cl, o, msg ) ) )
            {
              LONG error;

              /* Load frames... */
              if ((error = LoadFrames( classbase, (Object *)retval ) ) != 0)
              {
                /* Something went fatally wrong, dispose object */
                CoerceMethod( cl, (Object *)retval, OM_DISPOSE );
                retval = 0UL;
              }

              SetIoErr( error );
            }
          }
          else
          {
            /* Subclasses of mpegvideo.datatype are not implemented */
            SetIoErr( ERROR_NOT_IMPLEMENTED );
          }

    return retval;
}


/****** mpegvideo.datatype/OM_DISPOSE *****************************************
*
*    NAME
*        OM_DISPOSE -- Delete a mpegvideo.datatype object.
*
*    FUNCTION
*        The OM_DISPOSE method is used to delete an instance of the
*        mpegvideo.datatype class.  This method is passed to the superclass
*        when it has completed.
*        This method frees all frame nodes and their contents (pictures, their
*        colormaps, sounds etc.).
*
*    RESULT
*        The object is deleted. 0UL is returned.
*
******************************************************************************
*
*/
IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
    struct MPEGVideoInstData *mvid = (struct MPEGVideoInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

          if( (mvid -> mvid_LoadAll) == FALSE )
          {
            mpeg_closedown( mvid );
          }

          /* Wait for any outstanding blitter usage (which may use one of our bitmaps) */
          WaitBlit();

          /* Delete the frame list */
          while ((fn = (struct FrameNode *)RemHead( (struct List *)(&(mvid -> mvid_FrameList)) )) != NULL)
          {
            FreeFrameNode( mvid, fn );
          }

          /* Free our key bitmap */
          FreeBitMap( (mvid -> mvid_KeyBitMap) );

          /* Delete the frame pool */
          DeletePool( (mvid -> mvid_Pool) );

          /* Close VERBOSE output file */
          if( mvid -> mvid_VerboseOutput )
          {
            Close( (mvid -> mvid_VerboseOutput) );
          }

          /* Dispose object */
          DoSuperMethodA( cl, o, msg );

    return 1;
}


IPTR DT_SetMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

#if (0)
    if( DoMethod( o, ICM_CHECKLOOP ) )
    {
        break;
    }
#endif

          /* Pass the attributes to the animation class and force a refresh if we need it */
          if( ( retval = DoSuperMethodA( cl, o, msg ) ) )
          {
/* The following statement is commented out because mpegvideo.datatype does not allow
 * subclasses. Thus, the following statement is NOP unless subclasses are supported...
 */
#ifdef COMMENTED_OUT
            /* Top instance ? */
            if( OCLASS( o ) == cl )
#endif /* COMMENTED_OUT */
            {
              struct RastPort *rp;

              /* Get a pointer to the rastport */
              if ((rp = ObtainGIRPort( (((struct opSet *)msg) -> ops_GInfo) )) != NULL)
              {
                struct gpRender gpr;

                /* Force a redraw */
                gpr . MethodID   = GM_RENDER;
                gpr . gpr_GInfo  = ((struct opSet *)msg) -> ops_GInfo;
                gpr . gpr_RPort  = rp;
                gpr . gpr_Redraw = GREDRAW_UPDATE;

                DoMethodA( o, (Msg)(&gpr) );

                /* Release the temporary rastport */
                ReleaseGIRPort( rp );

                retval = 0UL;
              }
            }
          }
    return retval;
}


/****** mpevideo.datatype/DTM_WRITE ******************************************
*
*    NAME
*        DTM_WRITE -- Save data
*
*    FUNCTION
*        This method saves the object's contents to disk.
*
*        If dtw_Mode is DTWM_IFF, the method is passed unchanged to the
*        superclass, animation.datatype, which writes a single IFF ILBM
*        picture.
*        The local data format (e.g. dtw_Mode == DTWM_RAW) is currently
*        not supported, this method returns 0UL
*        (and result2 == ERROR_NOT_IMPLEMENTED) in that case.
*
*    RESULT
*        Returns 0 for failure (IoErr() returns result2), non-zero
*        for success.
*
******************************************************************************
*
*/
IPTR DT_Write(struct IClass *cl, Object *o, struct dtWrite *dtw)
{
    IPTR retval = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

          /* Local data format not supported yet... */
          if( (dtw -> dtw_Mode) == DTWM_RAW )
          {
            /* Return result = 0UL and result2 = ERROR_NOT_IMPLEMENTED */
            SetIoErr( ERROR_NOT_IMPLEMENTED );
          }
          else
          {
            /* Pass msg to superclass (which writes a single frame as an IFF ILBM picture)... */
            retval = DoSuperMethodA( cl, o, (Msg)dtw );
          }
    return retval;
}


/****** mpegvideo.datatype/ADTM_LOADFRAME ************************************
*
*    NAME
*        ADTM_LOADFRAME -- Load frame
*
*    FUNCTION
*        The ADTM_LOADFRAME method is used to obtain the bitmap and timing
*        data of the animation.
*        The given timestamp will be used to find the closest timestamp
*        in the internal FrameNode list. If it was found, the corresponding
*        timing, bitmap, colormap and sound data are stored into the struct
*        adtFrame.
*
*        The mpegvideo.datatype always loads a whole IPPB...I sequence
*        into memory; the sequence stays in memory until all frames of
*        the sequence are unloaded.
*
*    RESULT
*        Returns non-zero if the frame was loaded, 0UL otherwise
*        (Result2 (IoErr()) contains then the cause).
*
******************************************************************************
*
*/
IPTR DT_LoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf)
{
    struct MPEGVideoInstData *mvid = (struct MPEGVideoInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;
    LONG              error = 0L;
    IPTR retval = 0;

    D(bug("[mpegvideo.datatype] %s()\n", __func__));

          /* Like "realloc": Free any given frame here */
          if( alf -> alf_UserData )
          {
            alf -> MethodID = ADTM_UNLOADFRAME;

              DoMethodA( o, (Msg)alf );

            alf -> MethodID = ADTM_LOADFRAME;
          }

          /* Find frame by timestamp */
          if ((fn = FindFrameNode( (&(mvid -> mvid_FrameList)), (alf -> alf_TimeStamp) )) != NULL)
          {
            struct FrameNode *i_frame = ((fn -> fn_IFrame)?(fn -> fn_IFrame):(fn));

            /* Load bitmaps only if we don't cache the whole anim and
             * if we have a filehandle to load from (an empty object created using DTST_RAM)...
             */
            if( ((mvid -> mvid_LoadAll) == FALSE) && (mvid -> input) )
            {
              ObtainSemaphore( (&(mvid -> mvid_SigSem)) );

              /* No BitMap loaded ? */
              if( (fn -> fn_BitMap) == NULL )
              {
                struct FrameNode *ni_frame = FindNextIFrame( i_frame );

                totNumFrames          = i_frame -> fn_Frame - 1UL;                            /* set start of load sequence */
                mvid -> mvid_MaxFrame = (ni_frame)?(ni_frame -> fn_Frame):(totNumFrames + 1); /* set stop  of load sequence */

                debug_printf( mvid, "ADTM_LOADFRAME: frame %lu %lx %lx I sequence %lu [%lx] - %lu [%lx]\n",
                                (alf -> alf_TimeStamp), i_frame, fn, totNumFrames, (i_frame -> fn_BMOffset), (mvid -> mvid_MaxFrame),
                                ((ni_frame)?(ni_frame -> fn_BMOffset):(~0UL)) );

                /* Set "point of return" on decoder exit/error */
                if( setjmp( exit_buf ) == 0 )
                {
                  ULONG      data;
                  VidStream *vid_stream = (mvid -> mvid_VidStream);

                  mvid -> mvid_retval  = RETURN_OK;
                  mvid -> mvid_retval2 = 0L;

                  mvid -> mvid_IndexScan = FALSE;

                  /* Initialize bitstream i/o fields. */
                  curBits   = 0UL;
                  bitOffset = 0L;
                  bufLength = 0L;
                  bitBuffer = NULL;

                  EOF_flag = FALSE;

                  ResetVidStream( mvid, (mvid -> mvid_VidStream) );

                  /* Move to the beginning of the I frame start. Use next_start_code to find the
                   * correct start of the start_code; therefore we move a little bit BEFORE the
                   * start that next_start_code can do it's job properly
                   */
                  (void)Seek( (mvid -> input), (i_frame -> fn_BMOffset), OFFSET_BEGINNING );

                  {
                    /* Set global curVidStream to vid_stream. Necessary because bit i/o use
                     * curVidStream and are not passed vid_stream. Also set global bitstream
                     * parameters.
                     */
                    curVidStream  =  vid_stream;
                    bitOffset     =  curVidStream -> bit_offset;
                    curBits       = *curVidStream -> buffer << bitOffset;
                    bufLength     =  curVidStream -> buf_length;
                    bitBuffer     =  (ULONG *)curVidStream -> buffer;
                  }

                  correct_underflow( mvid ); /* fill buffer */

                  do
                  {
                    {
                      /* Copy global bit i/o variables back into vid_stream. */
                      vid_stream -> buffer      = (unsigned int *)bitBuffer;
                      vid_stream -> buf_length  = bufLength;
                      vid_stream -> bit_offset  = bitOffset;
                    }

                    mpegVidRsrc( mvid, 0, (mvid -> mvid_VidStream) );

                    {
                      /* Set global curVidStream to vid_stream. Necessary because bit i/o use
                       * curVidStream and are not passed vid_stream. Also set global bitstream
                       * parameters.
                       */
                      curVidStream  =  vid_stream;
                      bitOffset     =  curVidStream -> bit_offset;
                      curBits       = *curVidStream -> buffer << bitOffset;
                      bufLength     =  curVidStream -> buf_length;
                      bitBuffer     =  (ULONG *)curVidStream -> buffer;
                    }

                    show_bits32( data );
                    debug_printf( mvid, "^^^ next code is %lx\n", data );
                  } while( totNumFrames <= (mvid -> mvid_MaxFrame) );
                }

                /* Check for errors during loading (if there was not IGNOREERRORS switch set) */
                if( ((mvid -> mvid_retval) >= RETURN_WARN) && ((mvid -> mvid_IgnoreErrors) == FALSE) )
                {
                  error = mvid -> mvid_retval2;
                }
              }

              ReleaseSemaphore( (&(mvid -> mvid_SigSem)) );
            }

            /* Store frame/context information */
            alf -> alf_Frame    = fn -> fn_Frame;
            alf -> alf_Duration = fn -> fn_Duration;
            alf -> alf_UserData = (APTR)fn;

            /* Store bitmap information */
            alf -> alf_BitMap = fn -> fn_BitMap;
            alf -> alf_CMap   = fn -> fn_CMap;

            /* Is there a sample to play ? */
            if( fn -> fn_Sample )
            {
              /* Store sound information */
              alf -> alf_Sample       = fn -> fn_Sample;
              alf -> alf_SampleLength = fn -> fn_SampleLength;
              alf -> alf_Period       = fn -> fn_Period;
            }
            else
            {
              /* No sound */
              alf -> alf_Sample       = NULL;
              alf -> alf_SampleLength = 0UL;
              alf -> alf_Period       = 0UL;
            }

            /* Frame sequence "in use", even for a unsuccessful result; on error
             * animation.datatype send an ADTM_UNLOADFRAME which frees
             * allocated resources and decreases the "UseCount"...
             */
            i_frame -> fn_UseCount++;

            retval = ((error)?(0UL):(IPTR)(alf -> alf_BitMap)); /* Result  */
            SetIoErr( error );                                   /* Result2 */

            /* Workaround for a BUG in NOLOADALL mode when the encoder "forgets" to fill a frame.
             * This statement avoids that playback will be stopped in such a case...
             */
            if( (retval == 0UL) && ((mvid -> mvid_LoadAll) == FALSE) && (error == 0L) )
            {
              retval = 1UL;
            }
          }
          else
          {
            SetIoErr( ERROR_OBJECT_NOT_FOUND );
          }
          
    return retval;
}


/****** mpegvideo.datatype/ADTM_UNLOADFRAME **********************************
*
*    NAME
*        ADTM_UNLOADFRAME -- Unload frame contents
*
*    FUNCTION
*        The ADTM_UNLOADFRAME method is used to release the contents of a
*        animation frame.
*
*        This method frees the bitmap data found in adtFrame.
*
*    RESULT
*        Returns always 0UL.
*
******************************************************************************
*
*/
IPTR DT_UnLoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf)
{
    struct MPEGVideoInstData *mvid = (struct MPEGVideoInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;


          /* Free bitmaps only if we don't cache the whole anim */
          if( (mvid -> mvid_LoadAll) == FALSE )
          {
            ObtainSemaphore( (&(mvid -> mvid_SigSem)) );

            if ((fn = (struct FrameNode *)(alf -> alf_UserData) ) != NULL)
            {
              struct FrameNode *i_frame = ((fn -> fn_IFrame)?(fn -> fn_IFrame):(fn));

              if( (i_frame -> fn_UseCount) > 0 )
              {
                i_frame -> fn_UseCount--;

                /* Free an existing bitmap if it isn't in use and if it is NOT the first bitmap */
                if( ((i_frame -> fn_UseCount) == 0) &&
                    (fn -> fn_BitMap) && ((i_frame -> fn_IsKeyFrame) == FALSE) )
                {
                  struct FrameNode *worknode = i_frame,
                                   *nextnode;

                  /* Free the whole IBBP...I sequence... */
                  while ((nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) ) != NULL)
                  {
                    /* We stop if we reach the next I frame */
                    if( (((worknode -> fn_IFrame) == NULL) && (worknode != i_frame)) )
                    {
                      break;
                    }

                    FreeFrameBitMap( mvid, (worknode -> fn_BitMap) );
                    worknode -> fn_BitMap = NULL;

                    worknode = nextnode;
                  }
                }
              }
            }

            ReleaseSemaphore( (&(mvid -> mvid_SigSem)) );
          }

          /* The frame has been freed ! */
          alf -> alf_UserData = NULL;

    return 1;
}
