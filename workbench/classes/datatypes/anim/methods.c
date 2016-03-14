/*
**
** $Id$
**  anim.datatype 1.12
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

struct ClassBase;
struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"
#include "classdata.h"

/* ansi includes */
#include <limits.h>

extern LONG ifferr2doserr[];

/****** anim.datatype/OM_NEW *************************************************
*
*    NAME
*        OM_NEW -- Create a anim.datatype object.
*
*    FUNCTION
*        The OM_NEW method is used to create an instance of the anim.datatype
*        class.  This method is passed to the superclass first. After this,
*        anim.datatype parses the prefs file and makes a scan through
*        the data to get index information. Frame bitmaps are loaded if the
*        input stream isn't seekable (e.g. IFF handle/clipboard),
*        colormaps and the first frame are loaded immediately.
*        If a sample was set in the prefs, it will be loaded and attached
*        to the animation.
*
*    ATTRIBUTES
*        The following attributes can be specified at creation time.
*
*        DTA_SourceType (ULONG) -- Determinates the type of DTA_Handle
*            attribute. DTST_FILE, DTST_CLIPBOARD and DTST_RAM are supported.
*            If any other type was set in a given DTA_SourceType,
*            OM_NEW will be rejected.
*            A sourcetype of DTST_CLIPBOARD forces the LOADALL prefs
*            switch (can't seek on clipboard).
*            Defaults to DTST_FILE.
*
*        DTA_Handle -- For both DTST_FILE and DTST_CLIPBOARD, a
*            (struct IFFHandle *) is expected. This handle will be
*            created by datatypesclass depeding on the DTF_#? flag, which
*            is DTF_IFF here.  DTST_FILE, datatypesclass creates
*            a IFF handle from the given DTA_Name and DTA_Handle (a
*            BPTR returned by Lock), if DTST_CLIPBOARD, datatypesclass
*            passes the given (IFF) handle through.
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
IPTR DT_NewMethod(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct TagItem *ti;
    IPTR retval;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* We only support DTST_FILE, DTST_CLIPBOARD or DTST_RAM as source type */
    if (( ti = FindTagItem( DTA_SourceType, (((struct opSet *)msg) -> ops_AttrList) )) != NULL)
    {
        if( ((ti -> ti_Data) != DTST_FILE)      &&
            ((ti -> ti_Data) != DTST_CLIPBOARD) &&
            ((ti -> ti_Data) != DTST_RAM) )
        {
            SetIoErr( ERROR_OBJECT_WRONG_TYPE );

            return (IPTR)NULL;
        }
    }

    /* Create object */
    if ((retval = DoSuperMethodA( cl, o, msg )) != (IPTR)NULL)
    {
        LONG error;

        /* Load frames... */
        if  ((error = LoadFrames( cb, (Object *)retval )) != 0)
        {
            /* Something went fatally wrong, dispose object */
            CoerceMethod( cl, (Object *)retval, OM_DISPOSE );
            retval = 0UL;
        }

        SetIoErr( error );
    }
    return retval;
}


/****** anim.datatype/OM_DISPOSE *********************************************
*
*    NAME
*        OM_DISPOSE -- Delete a anim.datatype object.
*
*    FUNCTION
*        The OM_DISPOSE method is used to delete an instance of the
*        anim.datatype class. This method is passed to the superclass when
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
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    LONG saved_ioerr = IoErr();

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Get a pointer to our object data */
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );

    /* Wait for any outstanding blitter usage (which may use one of our bitmaps) */
    WaitBlit();

    /* Free colormaps etc. */
    FreeFrameNodeResources( cb, (&(aid -> aid_FrameList)) );

    /* Free our key bitmap */
    FreeBitMap( (aid -> aid_KeyBitMap) );

    /* Delete the pools */
    DeletePool( (aid -> aid_FramePool) );
    DeletePool( (aid -> aid_Pool) );

    /* Close input file */
    if( aid -> aid_FH )
    {
#ifdef DOASYNCIO
        CloseAsync( cb, (aid -> aid_FH) );
#else
        Close( (aid -> aid_FH) );
#endif /* DOASYNCIO */
    }

    /* Close verbose output file */
    if( aid -> aid_VerboseOutput )
    {
        Close( (aid -> aid_VerboseOutput) );
    }

    if (aid->aid_dt)
    {
        FreeMem(aid->aid_dt, sizeof(struct DataType) + sizeof(struct DataTypeHeader));
    }

    /* Dispose object */
    DoSuperMethodA( cl, o, msg );

    SetIoErr( saved_ioerr );

    return 1;
}

IPTR DT_GetMethod(struct IClass *cl, Object *o, struct opGet *msg)
{
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    IPTR retval = (IPTR)TRUE;

    D(bug("[anim.datatype] %s()\n", __func__));

    switch(msg->opg_AttrID)
    {
#if defined(__AROS__)
    case DTA_DataType:
        {
            struct DataType     *dt = NULL;
            struct opGet        superGet;

            D(bug("[anim.datatype] %s: DTA_DataType\n", __func__);)

            superGet.MethodID = OM_GET;
            superGet.opg_AttrID = msg->opg_AttrID;
            superGet.opg_Storage = (IPTR *)&dt;
            DoSuperMethodA (cl, o, (Msg) &superGet);

            D(bug("[anim.datatype] %s: DataType @ 0x%p\n", __func__, dt);)

            if ((dt) && !(aid->aid_dt))
            {
                if ((aid->aid_dt = AllocMem(sizeof(struct DataType) + sizeof(struct DataTypeHeader), MEMF_ANY)) != NULL)
                {
                    CopyMem(dt, aid->aid_dt, sizeof(struct DataType));
                    aid->aid_dt->dtn_Header = (struct DataTypeHeader *)((IPTR)aid->aid_dt + sizeof(struct DataType));
                    CopyMem(dt->dtn_Header, aid->aid_dt->dtn_Header, sizeof(struct DataTypeHeader));
                    switch (aid->aid_AnimMode)
                    {
                        case acmpILBM:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-0";
                            break;
                        case acmpXORILBM:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-1";
                            break;
                        case acmpLongDelta:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-2";
                            break;
                        case acmpShortDelta:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-3";
                            break;
                        case acmpDelta:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-4";
                            break;
                        case acmpByteDelta:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-5";
                            break;
                        case acmpStereoByteDelta:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-6";
                            break;
                        case acmpAnim7:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-7";
                            break;
                        case acmpAnim8:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-8";
                            break;
                        case acmpAnimJ:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-J";
                            break;
                        case acmpAnimI:
                            aid->aid_dt->dtn_Header->dth_Name = "ANIM-I";
                            break;
                        default:
                            aid->aid_dt->dtn_Header->dth_Name = "Unknown ANIM";
                            break;
                    }
                }
            }

            if (aid->aid_dt)
                *msg->opg_Storage = (IPTR) aid->aid_dt;
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

    D(bug("[anim.datatype] %s()\n", __func__));

#if (0)
    if( DoMethod( o, ICM_CHECKLOOP ) )
    {
        break;
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

/****** anim.datatype/DTM_WRITE **********************************************
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
*        If dtw_mode is DTWM_RAW, the object saved an IFF ANIM stream to
*        the filehandle given, starting with the current frame until
*        the end is reached.
*        The sequence saved can be controlled by the ADTA_Frame, ADTA_Frames
*        and ADTA_FrameIncrement attributes (see TAGS section below).
*
*    TAGS
*        When writing the local ("raw") format, IFF ANIM, the following
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
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    IPTR retval;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Local data format not supported yet... */
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
        retval = SaveIFFAnim( cb, cl, o, dtw );
    }
    else
    {
        /* Pass msg to superclass (which writes a single frame as an IFF ILBM picture)... */
        retval = DoSuperMethodA( cl, o, (Msg)dtw );
    }
    return retval;
}


/****** anim.datatype/ADTM_START *********************************************
*
*    NAME
*        ADTM_START -- Prepare for playback
*
*    FUNCTION
*        ADTM_START tells the subclass (us) to prepare for continous
*        playback. To avoid a long "search" for a full frame during the
*        playback clock is running, we load here the given timestamp
*        (asa_Frame) manually (and preseves it using a small trick, see
*        source).
*
*        After all, the method is passed to superclass, which starts the
*        playback (and the master clock).
*
*    RESULT
*        Returns result from superclass (animation.datatype)
*
*    NOTE
*
******************************************************************************
*
*/
IPTR DT_Start(struct IClass *cl, Object *o, struct adtStart *asa)
{
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;
    ULONG             timestamp;
    IPTR retval;

    D(bug("[anim.datatype] %s(%d)\n", __func__, asa -> asa_Frame));

    timestamp = asa -> asa_Frame;

    ObtainSemaphore( (&(aid -> aid_SigSem)) );

    /* Turn on async IO */
    aid -> aid_AsyncIO = TRUE;

    /* Find frame by timestamp */
    if ((fn = FindFrameNode( (&(aid -> aid_FrameList)), timestamp )) != NULL)
    {
        /* Load bitmaps only if we don't cache the whole anim and
         * if we have a filehandle to load from (an empty object created using DTST_RAM don't have this)...
         */
        if( ((aid -> aid_LoadAll) == FALSE) && (aid -> aid_FH) )
        {
            /* If no bitmap is loaded, load it... */
            if( (fn -> fn_BitMap) == NULL )
            {
                struct adtFrame alf;

                /* reset method msg */
                memset( (void *)(&alf), 0, sizeof( struct adtFrame ) );

                /* load frame */
                alf . MethodID      = ADTM_LOADFRAME;
                alf . alf_TimeStamp = timestamp;
                alf . alf_Frame     = timestamp;

                /* Load frame */
                if( DoMethodA( o, (Msg)(&alf) ) )
                {
                    /* Success ! */

                    /* The "trick" used here is to decrase the fn_UseCount
                     * WITHOUT unloading the bitmap. The first following
                     * ADTM_LOADFRAME triggered by animation.datatypes playback
                     * clock gets this frame without any problems
                     */
                    fn -> fn_UseCount--;
                }
                else
                {
                    /* Failure ! */
                    error_printf( cb, aid, "ADTM_START load error %ld", IoErr() );

                    /* Unload frame... */
                    alf . MethodID = ADTM_UNLOADFRAME;
                    DoMethodA( o, (Msg)(&alf) );
                }
            }
        }
    }

    ReleaseSemaphore( (&(aid -> aid_SigSem)) );

    retval = DoSuperMethodA( cl, o, (Msg)asa );

    return retval;
}


/****** anim.datatype/ADTM_PAUSE **********************************************
*
*    NAME
*        ADTM_PAUSE -- Pause playback
*
*    FUNCTION
*        ADTM_PAUSE tells the subclass (use) to pause playback.
*
*        The method is passed to animation.datatype first, which pauses the
*        playback clock.
*
*    RESULT
*        Returns result from superclass (animation.datatype)
*
*    NOTE
*
******************************************************************************
*
*/
IPTR DT_Pause(struct IClass *cl, Object *o, struct opSet *msg)
{
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    IPTR retval;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Pass msg to superclass first ! */
    retval = DoSuperMethodA( cl, o, msg );

    ObtainSemaphore( (&(aid -> aid_SigSem)) );

    /* Return to syncrounous IO */
    aid -> aid_AsyncIO = FALSE;

    ReleaseSemaphore( (&(aid -> aid_SigSem)) );

    return retval;
}


/****** anim.datatype/ADTM_STOP **********************************************
*
*    NAME
*        ADTM_STOP -- Stop playback
*
*    FUNCTION
*        ADTM_STOP tells the subclass (use) to stop playback.
*
*        The method is passed to animation.datatype first, which stops the
*        playback clock.
*
*        After clock has been stopped, we search for frames which have a
*        0 UseCount and have not been unloaded yet (a small cleaup to get
*        rid of frames which are loaded using the "trick" in ADTM_START
*        code).
*
*    RESULT
*        Returns result from superclass (animation.datatype)
*
*    NOTE
*
******************************************************************************
*
*/
IPTR DT_Stop(struct IClass *cl, Object *o, struct opSet *msg)
{
#if 0
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
#endif
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    IPTR retval;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Pass msg to superclass first ! */
    retval = DoSuperMethodA( cl, o, msg );

    ObtainSemaphore( (&(aid -> aid_SigSem)) );

#if 0
    if( ((aid -> aid_LoadAll) == FALSE) && (aid -> aid_FH) )
    {
        struct FrameNode *worknode,
             *nextnode;

        worknode = (struct FrameNode *)(aid -> aid_FrameList . mlh_Head);

        while( nextnode = (struct FrameNode *)(worknode -> fn_Node . mln_Succ) )
        {
            /* Free an existing bitmap if it isn't in use and if it is NOT the first bitmap */
            if( ((worknode -> fn_UseCount) == 0) && (worknode -> fn_BitMap) && (worknode != (struct FrameNode *)(aid -> aid_FrameList . mlh_Head)) )
            {
                /* Don't free the current nor the previous nor the next bitmap (to avoid problems with delta frames) */
                if( (worknode != (aid -> aid_CurrFN)) &&
                    (worknode != (struct FrameNode *)(aid -> aid_CurrFN -> fn_Node . mln_Succ)) &&
                    (worknode != (struct FrameNode *)(aid -> aid_CurrFN -> fn_Node . mln_Pred)) )
                {
                    FreePooledVec( cb, (aid -> aid_FramePool), (worknode -> fn_BitMap) );
                    worknode -> fn_BitMap = NULL;
                }
            }

            worknode = nextnode;
        }
    }
#endif

    /* Return to syncrounous IO */
    aid -> aid_AsyncIO = FALSE;

    ReleaseSemaphore( (&(aid -> aid_SigSem)) );
    
    return retval;
}


/****** anim.datatype/ADTM_LOADFRAME *****************************************
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
*        adtFrame. If the bitmap wasn't loaded at this time, this method 
*        attempts to load it from disk.
*
*    RESULT
*        Returns the bitmap ptr if a bitmap was found, 0UL otherwise;
*        in case of failure Result2 contains the cause:
*        ERROR_OBJECT_NOT_FOUND: Given timestamp does not exist
*        ERROR_NO_FREE_STORE:    No memory
*        and so on...
*
*    NOTE
*        It is expected that a 0 return code (error) causes an
*        ADTM_UNLOADFRAME that the invalid bitmap etc. will be freed.
*
******************************************************************************
*
*/
IPTR DT_LoadFrame(struct IClass *cl, Object *o, struct adtFrame *alf)
{
    struct ClassBase     *cb = (struct ClassBase *)(cl -> cl_UserData);
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;
    IPTR retval = 0;

    D(bug("[anim.datatype] %s(%d:%d)\n", __func__, alf -> alf_Frame, alf -> alf_TimeStamp));

    ObtainSemaphore( (&(aid -> aid_SigSem)) );

    D(bug("[anim.datatype] %s: sem obtained ..\n", __func__));

    /* Like "realloc": Free any given frame here */
    if( alf -> alf_UserData )
    {
        alf -> MethodID = ADTM_UNLOADFRAME;

        DoMethodA( o, (Msg)alf );

        alf -> MethodID = ADTM_LOADFRAME;
    }

    /* Find frame by timestamp */
    if ((fn = FindFrameNode( (&(aid -> aid_FrameList)), (ULONG)(alf -> alf_TimeStamp) )) != NULL)
    {
        LONG error = 0L;

        D(bug("[anim.datatype] %s: frame node @ 0x%p\n", __func__, fn));

        aid -> aid_CurrFN = fn;

        /* Load bitmaps only if we don't cache the whole anim and
         * if we have a filehandle to load from (an empty object created using DTST_RAM don't have this)...
         */
        if( ((aid -> aid_LoadAll) == FALSE) && (aid -> aid_FH) )
        {
             D(bug("[anim.datatype] %s: fh @ 0x%p\n", __func__, aid -> aid_FH));

            /* If no bitmap is loaded, load it... */
            if( (fn -> fn_BitMap) == NULL )
            {
                D(bug("[anim.datatype] %s: need to alloc/load .. \n", __func__));
                if ((fn -> fn_BitMap = AllocBitMapPooled( cb, (ULONG)(aid -> aid_BMH -> bmh_Width), (ULONG)(aid -> aid_BMH -> bmh_Height), (ULONG)(aid -> aid_BMH -> bmh_Depth), (aid -> aid_FramePool) )) != NULL)
                {
                    struct FrameNode *worknode = fn;
                    ULONG             rollback = 0UL;
                    UBYTE            *buff;
                    ULONG             buffsize;
                    BOOL                done = FALSE;

                    D(bug("[anim.datatype] %s: frame bitmap @ 0x%p\n", __func__, fn -> fn_BitMap));

                    /* Buffer to fill. Below we try to read some more bytes
                     * (the size value is stored in worknode -> fn_LoadSize)
                     * (ANHD chunk (~68 bytes), maybe a CMAP) to save
                     * the Seek in the next cycle.
                     * This makes only much sense when doing async io (during playback)...
                     */

                    /* Not the last frame !
                     * Note that this code is replicated in the loop below !!
                     */

                    worknode -> fn_LoadSize = worknode -> fn_BMSize;
                    D(bug("[anim.datatype] %s: loadsize %d\n", __func__, worknode -> fn_LoadSize));

                    if( (worknode -> fn_Node . mln_Succ -> mln_Succ) && (aid -> aid_AsyncIO) )
                    {
                        ULONG nextpos = (((struct FrameNode *)(worknode -> fn_Node . mln_Succ)) -> fn_BMOffset);

                        worknode -> fn_LoadSize = MAX( (worknode -> fn_LoadSize), (nextpos - worknode -> fn_BMOffset) );

                        /* Don't alloc a too large buffer... */
                        worknode -> fn_LoadSize = MIN( (worknode -> fn_LoadSize), ((worknode -> fn_BMSize) * 2UL) );
                    }

                    buffsize = worknode -> fn_LoadSize;

                    D(bug("[anim.datatype] %s: buffsize %d\n", __func__, worknode -> fn_LoadSize));

                    do
                    {
                        worknode = worknode -> fn_PrevFrame;
                        rollback++;

                        worknode -> fn_LoadSize = worknode -> fn_BMSize;

                        if( (worknode -> fn_Node . mln_Succ -> mln_Succ) && (aid -> aid_AsyncIO) )
                        {
                            ULONG nextpos = (((struct FrameNode *)(worknode -> fn_Node . mln_Succ)) -> fn_BMOffset);

                            worknode -> fn_LoadSize = MAX( (worknode -> fn_LoadSize), (nextpos - worknode -> fn_BMOffset) );

                            /* Don't alloc a too large buffer... */
                            worknode -> fn_LoadSize = MIN( (worknode -> fn_LoadSize), ((worknode -> fn_BMSize) * 2UL) );
                        }

                        buffsize = MAX( buffsize, (worknode -> fn_LoadSize) );
                    } while( ((worknode -> fn_BitMap) == NULL) && ((worknode -> fn_TimeStamp) != 0UL) );

                    if( ((worknode -> fn_BitMap) == NULL) && ((worknode -> fn_TimeStamp) == 0UL) )
                    {
                        verbose_printf( cb, aid, "first frame without bitmap ... !\n" );
                        ClearBitMap( (fn -> fn_BitMap) );
                    }

                    /* Alloc buffer for compressed frame (DLTA) data */
                    if (!(done) && ((buff = (UBYTE *)AllocPooledVec( cb, (aid -> aid_Pool), (buffsize + 32UL) )) != NULL))
                    {
                        D(bug("[anim.datatype] %s: buffer @ 0x%p (%d + 32 bytes)\n", __func__, buff, buffsize));
                        do
                        {
                            ULONG current = rollback;

                            worknode = fn;

                            D(bug("[anim.datatype] %s: worknode @ 0x%p\n", __func__, worknode));

                            while( current-- )
                            {
                                worknode = worknode -> fn_PrevFrame;
                            }

                            if( (worknode -> fn_BitMap) && (worknode != fn) )
                            {
                                D(bug("[anim.datatype] %s: copying bitmap @ 0x%p\n", __func__, worknode -> fn_BitMap));

                                CopyBitMap( cb, (worknode -> fn_BitMap), (fn -> fn_BitMap) );

                                D(bug("[anim.datatype] %s: worknode @ 0x%p\n", __func__, worknode));
                            }
                            else
                            {
                                LONG seekdist; /* seeking distance (later Seek result, if Seek'ed) */

                                D(bug("[anim.datatype] %s: loading data\n", __func__));

                                seekdist = ((worknode -> fn_BMOffset) - (aid -> aid_CurrFilePos));

                                D(
                                    bug("[anim.datatype] %s: fh off = %d\n", __func__, aid -> aid_CurrFilePos);
                                    bug("[anim.datatype] %s: bm off  = %d\n", __func__, worknode -> fn_BMOffset);
                                    bug("[anim.datatype] %s: seek = %d\n", __func__, seekdist);
                                )

                                /* Seek needed ? */
                                if( seekdist != 0L )
                                {
#ifdef DOASYNCIO
                                    seekdist = SeekAsync( cb, (aid -> aid_FH), seekdist, OFFSET_CURRENT );
#else
                                    seekdist = Seek( (aid -> aid_FH), seekdist, OFFSET_CURRENT );
#endif /* DOASYNCIO */
                                }

                                /* "Seek" success ? */
                                if( seekdist != (-1L) )
                                {
                                    LONG bytesread;

#ifdef DOASYNCIO
                                    bytesread = ReadAsync( cb, (aid -> aid_FH), buff, (worknode -> fn_LoadSize) );
#else
                                    bytesread = Read( (aid -> aid_FH), buff, (worknode -> fn_LoadSize) );
#endif /* DOASYNCIO */

                                    /* No error during reading ? */
                                    if ((bytesread >= (worknode -> fn_BMSize)) && (bytesread != -1L))
                                    {
                                        LONG ifferror;

                                        if ((ifferror = DrawDLTA( cb, aid, (fn -> fn_BitMap), (fn -> fn_BitMap), (&(worknode -> fn_AH)), buff, (worknode -> fn_BMSize) )) != 0)
                                        {
                                            error_printf( cb, aid, "dlta unpacking error %lu\n", ifferror );

                                            /* convert IFFParse error to DOS error */
                                            error = ifferr2doserr[ (-ifferror - 1) ];
                                        }

                                        /* Bump file pos */
                                        aid -> aid_CurrFilePos = (worknode -> fn_BMOffset + bytesread);
                                    }
                                    else
                                    {
                                        /* Read error */
                                        error = IoErr();

                                        /* Error, rewind stream */
#ifdef DOASYNCIO
                                        SeekAsync( cb, (aid -> aid_FH), 0L, OFFSET_BEGINNING );
#else
                                        Seek( (aid -> aid_FH), 0L, OFFSET_BEGINNING );
#endif /* DOASYNCIO */
                                        aid -> aid_CurrFilePos = 0L;
                                    }

                                    worknode -> fn_LoadSize = 0UL; /* destroy that this value won't affect anything else */
                                }
                                else
                                {
                                    /* Seek error */
                                    error = IoErr();
                                }
                            }
                        } while( rollback-- && (error == 0L) );

                        FreePooledVec( cb, (aid -> aid_Pool), buff );
                    }
                    else
                    {
                        /* No memory for compressed frame data */
                        error = ERROR_NO_FREE_STORE;
                    }
                }
                else
                {
                    /* No memory for frame bitmap */
                    error = ERROR_NO_FREE_STORE;
                }
            }
        }

        /* Store frame/context information */
        alf -> alf_Frame    = fn -> fn_Frame;
        alf -> alf_Duration = fn -> fn_Duration;
        alf -> alf_UserData = (APTR)fn;        /* Links back to this FrameNode (used by ADTM_UNLOADFRAME) */

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

        /* Frame "in use", even for a unsuccessful result; on error
         * animation.datatype send an ADTM_UNLOADFRAME which frees
         * allocated resources and decreases the "UseCount"...
         */
        fn -> fn_UseCount++;

        /* Is this node in the posted-free queue ? */
        if( fn -> fn_PostedFree )
        {
            Remove( (struct Node *)(&(fn -> fn_PostedFreeNode)) );
            fn -> fn_PostedFree = FALSE;
        }

        retval = ((error)?(0):(IPTR)(alf -> alf_BitMap)); /* Result  */
        SetIoErr( error );                                   /* Result2 */
    }
    else
    {
        /* no matching frame found */
        SetIoErr( ERROR_OBJECT_NOT_FOUND );
    }

    ReleaseSemaphore( (&(aid -> aid_SigSem)) );

    D(
        if (retval)
        {
            bug("[anim.datatype] %s: frame #%d:\n", __func__, alf -> alf_Frame);
            bug("[anim.datatype] %s:     bitmap @ 0x%p\n", __func__, alf -> alf_BitMap);
            bug("[anim.datatype] %s:     timestamp %d, duration %d\n", __func__, alf -> alf_TimeStamp, alf -> alf_Duration);
        }
        else
            bug("[anim.datatype] %s: failed (%08x)\n", __func__, retval);
    )
    return retval;
}

/****** anim.datatype/ADTM_UNLOADFRAME ***************************************
*
*    NAME
*        ADTM_UNLOADFRAME -- Load frame contents
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
    struct AnimInstData  *aid = (struct AnimInstData *)INST_DATA( cl, o );
    struct FrameNode *fn;
    IPTR retval = (IPTR)TRUE;

    D(bug("[anim.datatype] %s()\n", __func__));

    /* Free bitmaps only if we don't cache the whole anim */
    if( (aid -> aid_LoadAll) == FALSE )
    {
        struct MinNode *pfn;
        UWORD           i   = 10;

        ObtainSemaphore( (&(aid -> aid_SigSem)) );

        if ((fn = (struct FrameNode *)(alf -> alf_UserData) ) != NULL)
        {
            if( (fn -> fn_UseCount) > 0 )
            {
                fn -> fn_UseCount--;

                /* Free an existing bitmap if it isn't in use and if it is NOT the first bitmap */
                if( ((fn -> fn_UseCount) == 0) && (fn -> fn_BitMap) && (fn != (struct FrameNode *)(aid -> aid_FrameList . mlh_Head)) )
                {
                    if( FALSE /*FreeAbleFrame( aid, fn )*/ )
                    {
                        /* Is this node in the posted-free queue ? */
                        if( fn -> fn_PostedFree )
                        {
                            Remove( (struct Node *)(&(fn -> fn_PostedFreeNode)) );
                            fn -> fn_PostedFree = FALSE;

                            D( kprintf( "free posted 1 %lu\n", (fn -> fn_TimeStamp) ) );
                        }

                        FreePooledVec( cb, (aid -> aid_FramePool), (fn -> fn_BitMap) );
                        fn -> fn_BitMap = NULL;
                    }
                    else
                    {
                        if( (fn -> fn_PostedFree) == FALSE )
                        {
                            D( kprintf( "posted free %lu\n", (fn -> fn_TimeStamp) ) );

                            AddTail( (struct List *)(&(aid -> aid_PostedFreeList)), (struct Node *)(&(fn -> fn_PostedFreeNode)) );
                            fn -> fn_PostedFree = TRUE;
                        }
                    }
                }
            }
        }

        while ((pfn = (struct MinNode *)RemHead( (struct List *)(&(aid -> aid_PostedFreeList)) )) != NULL)
        {
            fn = POSTEDFREENODE2FN( pfn );
            fn -> fn_PostedFree = FALSE;

            if( (fn -> fn_UseCount) == 0 )
            {
                if( FreeAbleFrame( aid, fn ) )
                {
                    D( kprintf( "free posted 2 %lu at %lu\n", (fn -> fn_TimeStamp), (((struct FrameNode *)(alf -> alf_UserData)) -> fn_TimeStamp) ) );

                    FreePooledVec( cb, (aid -> aid_FramePool), (fn -> fn_BitMap) );
                    fn -> fn_BitMap = NULL;
                }
                else
                {
                    AddTail( (struct List *)(&(aid -> aid_PostedFreeList)), (struct Node *)(&(fn -> fn_PostedFreeNode)) );
                    fn -> fn_PostedFree = TRUE;
                }

                /* Don't process the list twice */
                if ((fn == ((struct FrameNode *)(alf -> alf_UserData)) ) != (IPTR)NULL)
                {
                    i = MIN( 1, i );

                    retval = 0;
                    break;
                }

                if( i-- == 0 )
                {
                    D( kprintf( "pl overflow at %lu\n", (((struct FrameNode *)(alf -> alf_UserData)) -> fn_TimeStamp) ) );

                    retval = 0;
                    break;
                }
            }
        }

        ReleaseSemaphore( (&(aid -> aid_SigSem)) );
    }

    /* Indicate that the frame has been free'ed. */
    alf -> alf_UserData = NULL;

    return retval;
}
