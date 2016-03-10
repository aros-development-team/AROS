/*
**
**  $VER: methods.c 2.4 (01.01.2016)
**  gifanim.datatype 2.4
**
**  DataTypes class method implementations
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct MyStackSwapStruct;
struct GIFAnimInstData;
struct GIFEncoder;

/* main includes */
#include "classbase.h"
#include "classdata.h"

#include "dispatch.h"

/* ansi includes */
#include <limits.h>

/****** gifanim.datatype/OM_NEW **********************************************
*
*    NAME
*        OM_NEW -- Create a gifanim.datatype object.
*
*    FUNCTION
*        The OM_NEW method is used to create an instance of the
*        gifanim.datatype class.  This method is passed to the superclass
*        first. After this, gifanim.datatype parses the prefs file and makes
*        a scan through the data to get index information. Frame bitmaps are
*        loaded if the input stream isn't seekable, colormaps and the first
*        frame are loaded immediately.
*        If a sample was set in the prefs, it will be loaded and attached
*        to the animation.
*
*    ATTRIBUTES
*        The following attributes can be specified at creation time.
*
*        DTA_SourceType (ULONG) -- Determinates the type of DTA_Handle
*            attribute. Only DTST_FILE and DTST_RAM are supported.
*            If any other type was set in a given DTA_SourceType,
*            OM_NEW will be rejected.
*            Defaults to DTST_FILE.
*
*        DTA_Handle -- For DTST_FILE, a BPTR filehandle is expected. This
*            handle will be created by datatypesclass depeding on the DTF_#?
*            flag, which is DTF_BINARY here.  DTST_FILE, datatypesclass
*            creates a file handle from the given DTA_Name and DTA_Handle
*            (a BPTR returned by Lock).
*            A DTST_RAM (create empty object) source type requires a NULL
*            handle.
*
*    RESULT
*        If the object was created a pointer to the object is returned,
*        otherwise NULL is returned.
*
******************************************************************************
*
*/
struct Gadget *DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct TagItem *ti;
    IPTR retval;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* We only support DTST_FILE or DTST_RAM as source type */
    if ((ti = FindTagItem( DTA_SourceType, (((struct opSet *)msg) -> ops_AttrList) )) != NULL)
    {
        if( ((ti -> ti_Data) != DTST_FILE) && ((ti -> ti_Data) != DTST_RAM) )
        {
            SetIoErr( ERROR_OBJECT_WRONG_TYPE );

            return NULL;
        }
    }

    if( ( retval = DoSuperMethodA( cl, o, msg ) ) )
    {
        D(bug("[gifanim.datatype] %s: dtobject @ 0x%p\n", __func__, retval));
        /* Load frames... */
        if( !ScanFrames( cb, (Object *)retval ) )
        {
            D(bug("[gifanim.datatype] %s: failed to scan file\n", __func__));

            /* Something went fatally wrong, dispose object */
            CoerceMethod( cl, (Object *)retval, OM_DISPOSE );
            retval = 0UL;
        }
    }

    D(bug("[gifanim.datatype] %s: returning 0x%p\n", __func__, retval));

    return (struct Gadget *)retval;
}

/****** gifanim.datatype/OM_DISPOSE ******************************************
*
*    NAME
*        OM_DISPOSE -- Delete a gifanim.datatype object.
*
*    FUNCTION
*        The OM_DISPOSE method is used to delete an instance of the
*        gifanim.datatype class. This method is passed to the superclass when
*        it has completed.
*        This method frees all frame nodes and their contents (bitmaps,
*        colormaps, samples etc.)
*
*    RESULT
*        The object is deleted. 0UL is returned.
*
******************************************************************************
*
*/
IPTR DT_DisposeMethod(struct IClass *cl, Object *o, Msg msg)
{
  /* Get a pointer to our object data */
    struct GIFAnimInstData *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    LONG saved_ioerr = IoErr();

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Wait for any outstanding blitter usage (which may use one of our bitmaps) */
    WaitBlit();

    /* Free colormaps etc. (e.g. all resources which are NOT free'ed on DeletePool below) */
    FreeFrameNodeResources( cb, gaid );

    /* Free our key bitmap */
    FreeBitMap( (gaid -> gaid_KeyBitMap) );

    /* Delete the frame pool */
    DeletePool( (gaid -> gaid_Pool) );

    /* Close input file */
    if( gaid -> gaid_FH )
    {
        Close( (gaid -> gaid_FH) );
    }

    /* Close verbose output file */
    if ((gaid->gaid_VerboseOutput) && (gaid->gaid_VerboseOutput != (BPTR)-1L))
    {
        Close(gaid->gaid_VerboseOutput);
    }


    if (gaid->gaid_dt)
    {
        FreeMem(gaid->gaid_dt, sizeof(struct DataType) + sizeof(struct DataTypeHeader));
    }
    FreeVec(gaid->gaid_VerStr);

    /* Dispose object */
    DoSuperMethodA( cl, o, msg );

    /* Restore Result2 */
    SetIoErr( saved_ioerr );

    return (IPTR)NULL;
}

IPTR DT_FrameBox(struct IClass *cl, Object *o, struct dtFrameBox *msg)
{
    struct GIFAnimInstData *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );
    struct dtFrameBox *dtf = (struct dtFrameBox *)msg;
    IPTR retval;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* pass to superclas first */
    retval = DoSuperMethodA( cl, o, (Msg) msg );

    /* Does someone tell me in what for an environment (screen) I'll be attached to ? */
    if( (dtf -> dtf_FrameFlags) & FRAMEF_SPECIFY )
    {
        if( dtf -> dtf_ContentsInfo )
        {
            if( dtf -> dtf_ContentsInfo -> fri_Screen )
            {
                struct BitMap *bm = dtf -> dtf_ContentsInfo -> fri_Screen -> RastPort . BitMap;

                /* Does we have a non-planar bitmap ? */
                if( !(GetBitMapAttr( bm, BMA_FLAGS ) & BMF_STANDARD) )
                {
                    /* I assume here that the system is able to map a 24 bit bitmap into the screen
                             * if it is deeper than 8 bit.
                             */
                    if( ((bm -> Depth) > 8UL) && ((dtf -> dtf_ContentsInfo -> fri_Dimensions . Depth) > 8UL) )
                    {
                        verbose_printf( cb, gaid, "using chunky bitmap\n" );
                    }
                }
            }
        }
    }
    return retval;
}


IPTR DT_GetMethod(struct IClass *cl, Object *o, struct opGet *msg)
{
    struct GIFAnimInstData  *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );
    IPTR retval = (IPTR)TRUE;

    D(bug("[gifanim.datatype] %s()\n", __func__));

    switch(msg->opg_AttrID)
    {
#if defined(__AROS__)
    case DTA_DataType:
        {
            struct DataType     *dt = NULL;
            struct opGet        superGet;

            D(bug("[gifanim.datatype] %s: DTA_DataType\n", __func__);)

            superGet.MethodID = OM_GET;
            superGet.opg_AttrID = msg->opg_AttrID;
            superGet.opg_Storage = (IPTR *)&dt;
            DoSuperMethodA (cl, o, (Msg) &superGet);

            D(bug("[gifanim.datatype] %s: DataType @ 0x%p\n", __func__, dt);)

            if ((dt) && !(gaid->gaid_dt))
            {
                if ((gaid->gaid_dt = AllocMem(sizeof(struct DataType) + sizeof(struct DataTypeHeader), MEMF_ANY)) != NULL)
                {
                    CopyMem(dt, gaid->gaid_dt, sizeof(struct DataType));
                    gaid->gaid_dt->dtn_Header = (struct DataTypeHeader *)((IPTR)gaid->gaid_dt + sizeof(struct DataType));
                    CopyMem(dt->dtn_Header, gaid->gaid_dt->dtn_Header, sizeof(struct DataTypeHeader));
                    gaid->gaid_dt->dtn_Header->dth_Name = gaid->gaid_VerStr;
                }
            }

            if (gaid->gaid_dt)
                *msg->opg_Storage = (IPTR) gaid->gaid_dt;
            else if (dt)
                *msg->opg_Storage = (IPTR) dt;
            else
                *msg->opg_Storage = (IPTR) NULL;
        }
        break;
#endif
    default:
        return DoSuperMethodA (cl, o, (Msg) msg);
    }

    return retval;
}


IPTR DT_SetMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

#if (0)
    if( DoMethod( o, ICM_CHECKLOOP ) )
    {
        return NULL;
    }
#endif

    /* Pass the attributes to the animation class and force a refresh if we need it */
    if( ( retval = DoSuperMethodA( cl, o, msg ) ) )
    {
        /* Top instance ? */
        if( OCLASS( o ) == cl )
        {
            struct RastPort *rp;

            /* Get a pointer to the rastport */
            if ((rp = ObtainGIRPort( (((struct opSet *)msg) -> ops_GInfo) ) ) != NULL)
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

                /* We did a refresh... */
                retval = 0UL;
            }
        }
    }

    return retval;
}

/****** gifanim.datatype/DTM_WRITE *******************************************
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
*
*        If dtw_mode is DTWM_RAW, the object saved an GIF Animation stream 
*        to the filehandle given, starting with the current frame until
*        the end is reached.
*        The sequence saved can be controlled by the ADTA_Frame, ADTA_Frames
*        and ADTA_FrameIncrement attributes (see TAGS section below).
*
*    TAGS
*        When writing the local ("raw") format, GIF Animation, the following
*        attributes are recognized:
*
*        ADTA_Frame (ULONG) - start frame, saving starts here.
*            Defaults to the current frame displayed.
*
*        ADTA_Frames (ULONG) - the number of frames to be saved,
*            Defaults to (max_num_of_frames - curr_frame).
*
*        ADTA_FrameIncrement (ULONG) - frame increment when saving.
*            Defaults to 1, which means: "jump to next frame".
*
*    NOTE
*        - Any sound attached to the animation will NOT be saved.
*
*        - A CTRL-D signal to the writing process aborts the save.
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
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    IPTR retval;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Local data format requested ?... */
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
        retval = SaveGIFAnim( cb, cl, o, dtw );
    }
    else
    {
        /* Pass msg to superclass (which writes a single frame as an IFF ILBM picture)... */
        retval = DoSuperMethodA( cl, o, (Msg) dtw );
    }
    return retval;
}

/****** gifanim.datatype/ADTM_LOADFRAME *****************************************
*
*    NAME
*        ADTM_LOADFRAME -- Load frame
*
*    FUNCTION
*        The ADTM_LOADFRAME method is used to obtain the bitmap and timing
*        data of the animation.
*        The given timestamp will be used to find a matching timestamp
*        in the internal FrameNode list. If it was found, the corresponding
*        timing, bitmap and colormap data are stored into the struct
*        adtFrame. If the bitmap wasn't loaded, this method attempts to
*        load it from disk.
*
*    RESULT
*        the bitmap ptr if a bitmap was found,
*        0 (and result2 with the reason).
*
******************************************************************************
*
*/
IPTR DT_LoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf)
{
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct GIFAnimInstData *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );
    struct adtFrame   freeframe;
    struct FrameNode *fn;
    LONG              error       = 0L;
    IPTR retval = 0;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    ObtainSemaphore( (&(gaid -> gaid_SigSem)) );

    /* Like "realloc": Free any given frame (the free is done AFTER the load to
    * avoid that a frame which is loaded will be freed and then loaded again...
    */
    if( alf -> alf_UserData )
    {
        /* Copy message contents that we can call ADTM_UNLOADFRAME below */
        CopyMem( alf, &freeframe, sizeof(struct adtFrame) );
        alf -> alf_UserData = NULL; /* "freeframe" now owns the frame data to free ! */
    }
    else
    {
        /* No data to free... */
        freeframe . alf_UserData = NULL;
    }

    /* Find frame by timestamp */
    if ((fn = FindFrameNode( (&(gaid -> gaid_FrameList)), (alf -> alf_TimeStamp) )) != NULL)
    {
        /* Load bitmaps only if we don't cache the whole anim and
         * if we have a filehandle to load from (an empty object created using DTST_RAM won't have this)...
         */
        if( (!(gaid -> gaid_LoadAll)) && (gaid -> gaid_FH) )
        {
          /* If no bitmap is loaded, load it... */
          if( !(fn -> fn_BitMap) )
          {
            ULONG animwidth  = (ULONG)(gaid -> gaid_PaddedWidth),
                  animheight = (ULONG)(gaid -> gaid_Height);

            /* Allocate array for chunkypixel data */
            if ((fn->fn_ChunkyMap = (UBYTE *)AllocPooledVec( cb, (gaid -> gaid_Pool), ((animwidth * animheight) + 256) )) != NULL)
            {
              /* Get a clean background to prevent garbage showing througth transparent parts */
              memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );

              if ((fn->fn_BitMap = AllocFrameBitMap( cb, gaid )) != NULL)
              {
                struct FrameNode *worknode = fn;
                struct FrameNode *prevnode = NULL;
                ULONG             rollback = 0UL;
                UBYTE            *deltamap = NULL;

                struct GIFDecoder *gifdec = (&(gaid -> gaid_GIFDec)); /* shortcut */

                /* See if we need a rollback (if TRUE, we copy (below) the previous chunkymap into our
                 * current chunkymap as background. If Left/Top != 0 or transparent colors are present,
                 * parts of this previous image will occur).
                 */
                switch( fn -> fn_GIF89aDisposal )
                {
                  case GIF89A_DISPOSE_NODISPOSE:
                  case GIF89A_DISPOSE_RESTOREPREVIOUS:
                  {
                      do
                      {
                        worknode = GetPrevFrameNode( worknode, 1UL );

                        rollback++;
                      } while( ((worknode -> fn_ChunkyMap) == NULL) && ((worknode -> fn_TimeStamp) != 0UL) );
                  }
                      break;
                }

                if( ((worknode -> fn_ChunkyMap) == NULL) && ((worknode -> fn_TimeStamp) == 0UL) )
                {
                  error_printf( cb, gaid, "frame %d frame without bitmap ... !\n", 0);
                }

                do
                {
                  ULONG current = rollback;

                  worknode = fn;

                  while( current-- )
                  {
                    worknode = GetPrevFrameNode( worknode, 1UL );
                  }

                  if( (worknode -> fn_ChunkyMap) && (worknode != fn) )
                  {
                    prevnode = worknode;
                  }
                  else
                  {
                    if( Seek( (gaid -> gaid_FH), ((worknode -> fn_BMOffset) - (gaid -> gaid_CurrFilePos)), OFFSET_CURRENT ) != (-1L) )
                    {
                      if ((gifdec->file_buffer = AllocVec( ((worknode -> fn_BMSize) + 16UL), MEMF_PUBLIC )) !=NULL)
                      {
                        BOOL   useGlobalColormap;
                        UWORD  bitPixel;
                        UBYTE  buf[ 16 ];

                        /* Init buffer */
                        gifdec -> buffer     = gifdec -> file_buffer;
                        gifdec -> buffersize = worknode -> fn_BMSize;
                        gifdec -> which_fh   = WHICHFH_BUFFER;

                        /* Fill buffer */
                        if( Read( (gifdec -> file), (gifdec -> buffer), (gifdec -> buffersize) ) == (gifdec -> buffersize) )
                        {
                          /* This "Read" can't fail because it comes from our memory buffer */
                          (void)ReadOK( cb, gifdec, buf, 9 );

                          useGlobalColormap = !BitSet( buf[ 8 ], LOCALCOLORMAP );

                          bitPixel = 1 << ((buf[ 8 ] & 0x07) + 1);

                          /* disposal method */
                          switch( worknode -> fn_GIF89aDisposal )
                          {
                            case GIF89A_DISPOSE_NOP:
                            {
                              /* Background not transparent ? */
                              if( ((worknode -> fn_GIF89aTransparent) == ~0U) ||
                                  ((worknode -> fn_GIF89aTransparent) != 0U) )
                              {
                                /* Restore to color 0 */
                                memset( (fn -> fn_ChunkyMap), 0, (size_t)(animwidth * animheight) );
                              }
                            }
                                break;

                            case GIF89A_DISPOSE_NODISPOSE:
                            {
                                /* do not dispose prev image */

                                /* Check if we have a prevnode link to the previous image.
                                 * If this is NULL, we assume that our chunkymap already contain
                                 * the previous image
                                 */
                                if( prevnode )
                                {
                                  CopyMem( (prevnode -> fn_ChunkyMap), (fn -> fn_ChunkyMap), (animwidth * animheight) );
#ifdef DELTAWPA8
                                  CopyBitMap( cb, (fn -> fn_BitMap), (prevnode -> fn_BitMap), animwidth, animheight );
                                  deltamap = prevnode -> fn_ChunkyMap;
#endif /* DELTAWPA8 */
                                }
                                else
                                {
#ifdef DELTAWPA8
                                  deltamap = NULL;
#endif /* DELTAWPA8 */
                                }
                            }
                                break;

                            case GIF89A_DISPOSE_RESTOREBACKGROUND:
                            {
                                /* Background not transparent ? */
                                if( ((worknode -> fn_GIF89aTransparent) == ~0U) ||
                                    ((worknode -> fn_GIF89aTransparent) != (gaid -> gaid_GIFDec . GifScreen . Background)) )
                                {
                                  /* Restore to background color */
                                  memset( (fn -> fn_ChunkyMap), (gaid -> gaid_GIFDec . GifScreen . Background), (size_t)(animwidth * animheight) );
                                }
                            }
                                break;

                            case GIF89A_DISPOSE_RESTOREPREVIOUS:
                            {
                                /* restore image of previous frame */

                                /* Check if we have a prevnode link to the previous image.
                                 * If this is NULL, we assume that our chunkymap already contain
                                 * the previous image
                                 */
                                if( prevnode )
                                {
                                  CopyMem( (prevnode -> fn_ChunkyMap), (fn -> fn_ChunkyMap), (animwidth * animheight) );
#ifdef DELTAWPA8
                                  CopyBitMap( cb, (fn -> fn_BitMap), (prevnode -> fn_BitMap), animwidth, animheight );
                                  deltamap = prevnode -> fn_ChunkyMap;
#endif /* DELTAWPA8 */
                                }
                                else
                                {
#ifdef DELTAWPA8
                                  deltamap = NULL;
#endif /* DELTAWPA8 */
                                }
                            }
                                break;

                            default: /* GIF89A_DISPOSE_RESERVED4 - GIF89A_DISPOSE_RESERVED7 */
                            {
                                error_printf( cb, gaid, "unsupported disposal method %lu\n", (ULONG)(gaid -> gaid_GIFDec . Gif89 . disposal) );
                            }
                                break;
                          }

                          if( !useGlobalColormap )
                          {
                            /* Skip colormap (in buffer) */
                            gifdec -> buffer += (GIFCMAPENTRYSIZE * bitPixel);
                          }

                          (void)ReadImage( cb, gaid,
                                           (fn -> fn_ChunkyMap),
                                           (UWORD)animwidth,
                                           LOHI2UINT16( buf[ 0 ], buf[ 1 ] ),
                                           LOHI2UINT16( buf[ 2 ], buf[ 3 ] ),
                                           LOHI2UINT16( buf[ 4 ], buf[ 5 ] ),
                                           LOHI2UINT16( buf[ 6 ], buf[ 7 ] ),
                                           BitSet( buf[ 8 ], INTERLACE ),
                                           FALSE,
                                           (worknode -> fn_GIF89aTransparent) );
                        }

                        FreeVec( (gifdec -> file_buffer) );
                        gifdec -> file_buffer = gifdec -> buffer = NULL;
                      }

                      /* Bump file pos */
                      gaid -> gaid_CurrFilePos = Seek( (gaid -> gaid_FH), 0L, OFFSET_CURRENT ); /* BUG: does not check for failure */
                    }
                    else
                    {
                      /* seek failed */
                      error = IoErr();
                      break;
                    }

                    prevnode = NULL; /* a previous image is now in our chunkymap,
                                      * we don't need the link anymore
                                      */
                  }
                } while( rollback-- );

                if( error == 0L )
                {
                  if( fn -> fn_ChunkyMap )
                  {
                    if( gaid -> gaid_UseChunkyMap )
                    {
                      WriteRGBPixelArray8( cb, (fn -> fn_BitMap), animwidth, animheight, (fn -> fn_ColorMap), (fn -> fn_ChunkyMap) );
                    }
                    else
                    {
                      WriteDeltaPixelArray8Fast( (fn -> fn_BitMap), (fn -> fn_ChunkyMap), deltamap );
                    }
                  }
                }
              }
              else
              {
                /* can't alloc bitmap */
                error = ERROR_NO_FREE_STORE;
              }
            }
            else
            {
              /* can't alloc chunkymap */
              error = ERROR_NO_FREE_STORE;
            }
          }
        }

        /* Store timing/context information */
        alf -> alf_Duration = fn -> fn_Duration;
        alf -> alf_Frame    = fn -> fn_Frame;
        alf -> alf_UserData = (APTR)fn;        /* Links back to this FrameNode (used by ADTM_UNLOADFRAME) */

        /* Store bitmap information */
        alf -> alf_BitMap = fn -> fn_BitMap;
        alf -> alf_CMap   = ((gaid -> gaid_UseChunkyMap)?(NULL):(fn -> fn_CMap)); /* we does not use a colormap with a direct RGB-coded bitmap */

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

        /* Frame "in use", even for a unsuccessful result; on error
         * animation.datatype send an ADTM_UNLOADFRAME which frees
         * allocated resources and decreases the "UseCount"...
         */
        fn -> fn_UseCount++;

        /* Return bitmap ptr of possible, 0UL and error cause otherwise */
        retval = ((error)?(0UL):(IPTR)(alf -> alf_BitMap)); /* Result  */
    }
    else
    {
        /* no matching frame found */
        error = ERROR_OBJECT_NOT_FOUND;
    }

    /* Like "realloc": Free any given frame here */
    if( freeframe . alf_UserData )
    {
        freeframe . MethodID = ADTM_UNLOADFRAME;
        DoMethodA( o, (Msg)(&freeframe) );
    }

    SetIoErr( error ); /* Result2 */

    ReleaseSemaphore( (&(gaid -> gaid_SigSem)) );

    return retval;
}

/****** gifanim.datatype/ADTM_UNLOADFRAME ************************************
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
    struct ClassBase        *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct GIFAnimInstData *gaid = (struct GIFAnimInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;

    D(bug("[gifanim.datatype]: %s()\n", __func__));

    /* Free bitmaps only if we don't cache the whole anim */
    if( (gaid -> gaid_LoadAll) == FALSE )
    {
        ObtainSemaphore( (&(gaid -> gaid_SigSem)) );

        if ((fn = (struct FrameNode *)(alf -> alf_UserData)) != NULL)
        {
            if ((fn -> fn_UseCount) > 0)
            {
                fn -> fn_UseCount--;

                /* Free an existing bitmap if it isn't in use and if it is NOT the first bitmap */
                if( ((fn -> fn_UseCount) == 0) && (fn -> fn_BitMap) && (fn != (struct FrameNode *)(gaid -> gaid_FrameList . mlh_Head)) )
                {
                    FreeFrameBitMap( cb, gaid, (fn -> fn_BitMap) );
                    FreePooledVec( cb, (gaid -> gaid_Pool), (fn -> fn_ChunkyMap) );
                    fn -> fn_BitMap    = NULL;
                    fn -> fn_ChunkyMap = NULL;
                }
            }
        }

        ReleaseSemaphore( (&(gaid -> gaid_SigSem)) );
    }

    /* The frame has been freed ! */
    alf -> alf_UserData = NULL;
    return (IPTR)NULL;
}
