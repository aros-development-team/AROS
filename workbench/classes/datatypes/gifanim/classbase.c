
/*
**
**  $VER: classbase.c 2.3 (24.5.98)
**  gifanim.datatype 2.3
**
**  Library routines for a DataTypes class
**
**  Written 1997/1998 by Roland 'Gizzy' Mainz
**  Original example source from David N. Junod
**
*/

#ifndef DEBUG
#   define DEBUG 0
#endif
#include <aros/debug.h>

struct GIFAnimInstData;
struct GIFEncoder;

/* main includes */
#include "classbase.h"


/****** gifanim.datatype/MAIN ************************************************
*
*    INTRODUCTION
*        Datatypes class for GIF animations.
*        Based on "giftopnm" by David Koblas, "ppmtogif" by Marcel Wijkstra
*        <wijkstra@fwi.uva.nl> and David Rowley <mgardi@watdscu.waterloo.edu>
*        and the CBM datatypes example source/ documents written by David
*        Junod.
*
*    LEGAL
*      * Note that this implementation uses LZW, which is PATENTED by UniSys
*        in the U.S.A.
*
*        Therefore, this application must not be used inside the United
*        States of America except for research purposes.
*
*      - Compuserves banner:
*
*      "The Graphics Interchange Format(c) is the Copyright property of
*      CompuServe Incorporated. GIF(sm) is a Service Mark property of
*      CompuServe Incorporated."
*
*      - "giftopnm" legal info:
*      +-------------------------------------------------------------------+
*      | Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)    |
*      |   Permission to use, copy, modify, and distribute this software   |
*      |   and its documentation for any purpose and without fee is hereby |
*      |   granted, provided that the above copyright notice appear in all |
*      |   copies and that both that copyright notice and this permission  |
*      |   notice appear in supporting documentation.  This software is    |
*      |   provided "as is" without express or implied warranty.           |
*      +-------------------------------------------------------------------+
*
*    REQUIREMENTS
*        - You need at least Kick/WB 3.0.
*          | Many people wrote me that they cannot find an
*          | "animation.datatype" class.
*          | Only the 3.1 release contains it. (Subclasses of)
*          | animation.datatype can run under 3.0.
*
*        - "datatypes/animation.datatype", >= V41
*          "animation.datatype V41" requires itself some
*          libraries/boopsi classes:
*        - "realtime.library", >= V39              - for timing
*        - "gadgets/tapedeck.gadget" (any version) - for the controls
*
*           If you want to attach samples, you need "sound.datatype" >= V39
*           and your prefereed subclass (8svx.datatype for IFF 8SVX samples
*           etc.).
*
*    USAGE
*        If the datatypes descriptor file was activated, any attempt to load
*        a GIF anim stream using GMultiView, MultiView, AmigaGuide or
*        SwitchWindow will load and play the animation. If the source was a
*        file, gifanim.datatype loads frames dynamically from disk.
*
*        If you want to save the current animation in gifanim.datatype's
*        local format, use MultiView's "Project/Save As..." menu (or
*        GMultiView's "Project/Save As Raw...").
*        gifanim.datatype saves the current animation, starting with the
*        current frame as GIF animation.
*
*        If you want to attach samples to the animation, you must edit the
*        prefs file (ENV:Classes/DataTypes/gifanim.prefs) and add the
*        following line:
*        VERBOSE SAMPLE="ram:have_a_nice_day.8svx"
*        Which loads and attaches the sample "ram:have_a_nice_day.8svx" to
*        the animation. See gifanim.datatype.doc/preferences for a complete
*        description of the prefs file.
*
*    INSTALLATION
*        After unpacking this archive:
*        Because this version does not include an Installer script, you have
*        to do the installation manually through the shell:
*
*          - Unpack this archive and copy the "gifanim.datatype" to
*            SYS:Classes/DataTypes/:
*
*        Copy CLONE FROM "gifanim.datatype" TO
*         "SYS:Classes/DataTypes/gifanim.datatype"
*
*          - Then copy the datatypes descriptor into the DEVS:DataTypes
*            directory.
*            If the descriptor already exists, you should not replace it,
*            otherwise you may loose "toolnodes" and other settings stored in
*            the existing descriptor.
*
*     Copy CLONE FROM "GIFANIM(%|.info)" TO DEVS:Datatypes/
*
*    SOURCE
*        Source is included as an example how to write an
*        animation.datatype subclass which deals with things chunky bitmaps
*        deltas and an encoder...
*
*    AUTHOR
*        If you want to blame me, report any bugs, or wants a new version
*        send your letter to:
*                        Roland Mainz
*                        Hohenstaufenstraße 8
*                        52388 Nörvenich
*                        GERMANY
*
*        Phone: (+49)(0)2426/901568
*        Fax:   (+49)(0)2426/901569
*
*        EMAIL is also available (if you want to send me attachments
*        larger than 1MB (up to 5MB, more with my permission):
*
*        GISBURN@w-specht.rhein-ruhr.de
*
*        Up to July 1998 I'm reachable using this email address, too:
*        Reinhold.A.Mainz@KBV.DE
*
*        | Please put your name and address in your mails !
*        | German mailers should add their phone numbers.
*        | See BUGS section above when submitting bug reports.
*
*        Sorry, but I can only look once a week for mails.
*        If you don't hear something from me within three weeks, please
*        send your mail again (but watch about new releases) (problems with
*        this email port are caused by reconfigurations, hackers, network
*        problems etc.).
*
*        The  entire  "gifanim.datatype"  package  may  be  noncommercially
*        redistributed, provided  that  the package  is always  distributed
*        in it's complete  form (including it's documentation). A small
*        copy fee  for media costs is okay but any kind of commercial
*        distribution is strictly forbidden without my permission !
*        Comments and suggestions how to improve this program are
*        generally appreciated!
*
*        Thanks to David Junod, who wrote the animation.datatype and lots of
*        the datatypes example code, David Koblas for his "giftopnm"
*        and other people for their compression formats, Peter McGavin for
*        his C2P function, Matt Dillon for his DICE, Olaf 'Olsen' Barthel
*        for his help, ideas and some text clips from his documentations.
*
******************************************************************************
*
*/



/****** gifanim.datatype/--datasheed-- ***************************************
*
*   NAME
*       gifanim.datatype -- data type for GIF Animations
*
*   SUPERCLASS
*       animation.datatype
*
*   DESCRIPTION
*       The anim datatype, a sub-class of the animation.datatype, is used to
*       load, play and encode (save) GIF animations.
*       It supports GIF 87a and GIF 89a compressed animations.
*       Using the prefs-file, any sound can be attached to the animation.
*
*   METHODS
*       OM_NEW -- Create a new animation object from a description file. The
*           source may only be a file. Or an empty object can be created for 
*           encoding (which must be filled by the application before 
*           encoding).
*
*       OM_DISPOSE -- Dispose instance and contents (frames, colormaps, sounds
*           etc.), then pass msg to superclass
*
*       OM_UPDATE -- Perform an ICM_CHECKLOOP check, and if succesfull, the
*           method will be executed like OM_SET downstairs.
*
*       OM_SET -- Pass msg to superclass and call GM_RENDER if retval from
*           superclass was != 0UL.
*
*       DTM_WRITE -- Save object's contents in local (GIF Animation) or
*           superclass (IFF ILBM) format.
*
*       ADTM_LOADFRAME -- Fill in struct adtFrame with requested information
*           from internal FrameNode list like bitmap, colormap and sample. If
*           the bitmap information is not loaded yet, it will be loaded from
*           disk.
*
*       ADTM_UNLOADFRAME -- Free resources obtained by ADTM_UNLOADFRAME.
*
*       All other methods are passed unchanged to superclass.
*
*   ATTRIBUTES
*       Following attributes are set by the object and are READ-ONLY for
*       applications:
*       DTA_ObjName             - file name
*       DTA_ObjAnnotation       - set by extension blocks (gif 89a comment
*                                 extention)
*       DTA_TotalHoriz          - same as ADTA_Width
*       DTA_TotalVert           - same as ADTA_Height
*       ADTA_Width              - set by GIF screen
*       ADTA_Height             - set by GIF screen
*       ADTA_Depth              - set by GIF screen
*       ADTA_Frames             - number of frames in animation
*       ADTA_FramesPerSecond    - fixed to 100 fps
*       ADTA_ModeID             - mode id flags
*       ADTA_KeyFrame           - Key frame of animation
*
*       The following attributes are only set if a sample is attached using 
*       the prefs-file:
*       ADTA_Sample             - only set to notify the anmation.datatype
*                                 instance that the object has sound data
*       ADTA_SampleLength       - size of sample data played within one 
*                                 timestamp
*       ADTA_Period             - sample period
*       ADTA_Volume             - volume as set by the prefs-file
*
*   BUGS
*       * The whole code looks like a big hack; sorry, but this datatype was
*         put together within 2 hours...
*         ...V1.2 only fixes some bugs and adds a few features.
*         ...V1.3 fixed some things, adds some legal stuff and CyberGFX
*            support
*
*       - Does not support transparency yet (the encoder supports
*         transparency througth a prefs-option).
*
*       - Very slow playback.
*         Reasons:
*         - Multiple memory allocations, Seek's and Read's in one
*           ADTM_LOADFRAME
*         - Unbufferd loading
*         - C2P conversion (fast, but not the fastest possible)
*         - crap design
*         - Slow gif decoder. Rewriting the beast in ASM maybe speed up
*           things, but I don't know much about m68k assembler.
*           Anybody out there who wants to do the job ?
*         ....
*
*         To get rid of this problem, the LOADALL switch is set for now.
*         On >= mc68060/50MHz, the LOADALL switch in unneccesary, use
*         NOLOADALL in this case...
*
*       - Some anims get a trashed background, maybe due a bug in the GIF89a
*         frame disposal support code.
*         Any hint/example animation ?
*
*       - This datatype was written for animation.datatype V41. Using this
*         datatype with animation.datatype V40.6 does not work !
*         Not a bug, but...
*
*       - In large videos, the frames at the end will be played slower than
*         those at the beginning of the file. This is the result of the
*         sequential search internally used (only serious with more than 25000
*         frames (mc6030/50mhz)).
*         May or may not be fixed.
*
*       - CyberGFX support is still under development. It seems that it works
*         good, but...
*
*       - CyberGFX C2P conversion (LUT8 -> RGB16/24) uses a very crap method
*         througth WriteRGBPixel...
*
*       - Some GIF89A_DISPOSE_RESTOREPREVIOUS and GIF89A_DISPOSE_NOP seems to be not
*         handled correctly. 
*         But I need more example files to analyse this...
*
*   TODO
*       - Fixing the bugs above.
*
*       - Write the "--input_format--"-Autodoc section.
*
*   HISTORY
*       V1.1
*         Released to the Waldspecht-BBS for testing.
*
*       V1.2
*         - Found and fixed the longstanding bug that animation.datatype
*           V40.7 didn't free some frames. Reason is that ADTM_LOADFRAME
*           may be used like "realloc". Now alf_UserData is checked;
*           any given frame will be freed (ADTM_UNLOADFRAME).
*           Fixed.
*
*         - Minor houskeeping changes; removed some unneccesary code.
*
*         - Now uses buffered reading (FRead instead of Read and SetVBuf).
*           Maybe this speeds up loading a little bit.
*
*         - Fixed the bug in NOLOADALL loading mode (dynamic loading of
*           frames from disk) that some "disposal" modes (previous image)
*           didn't work properly under some conditions.
*           Fixed.
*
*         - Fixed the bug that animations with a static background were
*           not correctly handled in LOADALL mode (I simply forgot
*           to write the code...).
*           Thanks to Francis Labrie (fb691875@er.uquam.ca) for reporting
*           the bug.
*           Fixed.
*
*         - Fixed the bug that ADTM_LOADFRAME returns evertimes
*           ERROR_NO_FREE_STORE on failure instead of returning the real
*           cause.
*           Also fixed the error handling code in ADTM_LOADFRAME.
*           Fixed.
*
*         - Implemented a delta mode for WPA8. If possible (e.g. if the
*           current frame uses the previous one as it's background), only
*           the changed areas are processed by the C2P code.
*
*         - Now supports the GIF comment extension. The character set is
*           converted automatically, chars > 128 are replaced by '_'
*           except the german 'ä', 'ö', 'ü', 'Ä', 'Ö', 'Ü' and 'ß'.
*           Multiple comment chunks are merged together.
*           The comment will be stored in DTA_ObjAnnotation.
*
*         - Cut some unneccesary VERBOSE output.
*
*       V1.3
*         | Internal testing release to search for the mysterious memory loss
*         | in conjunction with the 24BITCHUNKY option.
*
*         - Added the notice that UniSys holds the LZW patents in the USA.
*
*         - Removed BestModeIDA, because animation.datatype does the same
*           thing.
*
*         - Small code cleanup.
*           Removed some of the debugging code and removed some parts of
*           dead code.
*
*         - Fixed the "maximum timestamp" (ADTA_Frames) calculation.
*           the old way was very crap...
*           Fixed.
*
*         - Added some padding bytes in the decoder (instance data) part.
*           Now the data are aligned, which should speed up the decoding.
*
*         - Fixed possible problems when a GIF "Comment Extension" text is
*           not '\0'-terminated. Now the load buffer is zero'ed for each new
*           cycle.
*           Fixed.
*
*         - Added some VERBOSE about the "Plain Text Extension". The contents
*           are now shown in the verbose output.
*
*         - Fixed the bug that in the case that the first frame has no bitmap
*           a bitmap with 0,0,0 dimensions would be allocated.
*           (Did not occur, but...)
*           Fixed.
*
*         - Implemented a prefetch buffer for dynamic load mode (e.g.
*           NOLOADALL); now all data needed for the frame are loaded with one
*           Read statement, all following accesses are redirected to the
*           filled buffer.
*           Switched from FRead to Read again because we're now loading
*           larger blocks.
*
*         - Fixed the width padding problem, which caused an ugly border
*           filled with rubbish in some anims.
*           Fixed.
*
*         - Now supports different disposal modes in one animation (each
*           frame can have it's own disposal mode).
*
*         - Now supports different transarent colors in one animation (each
*           frame can have it's own transparent color).
*
*         - Removed all references to ADTA_BitMapHeader. It's not required
*           for this format.
*
*         - Now saves the transparent color values from the previous colormap.
*
*         - Fixed the bug that the datatypes creates everytimes a
*           palette-per-frame instead of creating them only if neccesary.
*           Fixed.
*
*         - Now sets the GIF Screen background color to 0 if there is no
*           global colormap.
*
*         - Added CyberGFX-Support. Upon request, the datatype tries to create
*           a 24-bit chunky bitmap if the 24BITCHUNKY prefs switch is set.
*           WARNING: Does only work with animation.datatype V41 or higher
*           (<= V41.2 had a small bug which has been fixed in V41.3).
*
*         - Added CyberGFX bug workaround when BestCModeIDA fails when the
*           dimensions given are too small (e.g. 32 * 50 returns INVALID_ID).
*           Then 640 * 480 are set.
*           Not very nice.
*
*         - Added GIF Picture descriptor to replace any version with a wrong
*           mask.
*
*       V1.4
*         - Very much thanks to Frank Mariak (fmariak@chaosengine.ping.de)
*           for finding the silly "too big palette"-bug in conjunction
*           with the new chunkypixel output.
*
*       V1.5
*         - Major code cleanup. BOOPSI class structures have been moved
*           to classdata.h, class independed functions have been moved
*           into misc.c, class preferences to prefs.c
*
*         - Replaced the custom stackswapping code with my "standard"
*           module "stackswap.c".
*
*         - OM_DISPOSE now preserves Result2 (IoErr()).
*
*         - Fixed the longstandig bug in ScanFrames that if an OS function
*           failed, and IoErr results 0, havoc broke out.
*           Fixed.
*
*         - ADTA_SampleLength calculations were wrong. The value was got
*           from the first frame. Now the value is set correctly to
*           alf_SampleLength / (alf_Duration + 1)
*           Fixed.
*
*         - If a ADTM_LOADFRAME method gets a message from a previous
*           ADTM_LOADFRAME call, the contents are freed now after
*           the requested data have been loaded. This avoids the
*           pathological case that if the same frame should be freed and
*           returned the frame IS freed and then re-loaded.
*           Now the free of the given frame is done after the loading
*           of the requested frame which avoids this inefficienty.
*           Fixed.
*
*         - Switched truecolor output down to 16 bit (565-bits-per-gun)
*           to save some mem...
*
*         - Added 16BITCHUNKY, NO16BITCHUNKY, TRUECOLOR and NOTRUECOLOR
*           options.
*
*         - Fixed ModeID handling. The previous behaviour was that
*           a 0 mode id causes the datatype to select it's own mode id.
*           But 0 means LORES. Now the default is -1 (which means
+           INVALID_ID), which causes the datatype to do it's own
*           calculations.
*           Fixed.
*
*       V1.6
*           internal version for testing
*
*       V2.1
*         - Implemeted the GIF animation encoder part.
*           The encoder writes GIF89a animation streams out.
*
*         - Fixed a mask bug in the suppied "GIF" descriptor;
*           a byte after the "GIF" signature was set to match,
*           Fixed.
*
*         - gifanim.datatype now requires at least animation.datatype V41
*           as base class.
*           (mainly to get rid of the V40 workround code which tried to
*           get class working with animation.datatype V40).
*
*         - Updated and cleaned-up the autodoc a little bit.
*
*       V2.2
*         - Minor code cleanup
*
*         - Removed REPEAT and NOREPEAT options/feature because this
*           hack-like "feature" seems to be incompatible to GMultiView's
*           repeat option.
*           Problem fixed.
*
*         - Added encoder prefs options to set GIF interlace mode, transparent
*           and background pens.
*           See "gifanim.datatype.doc" section "preferences" for details.
*           Currently untested...
*
*         - Major speedup in the encoder part (~ three times faster).
*           a) The encoder now writes any incoming bitmap in an chunkypixel
*              array and operates on it instead of doing ReadPixel for
*              each pixel.
*           b) Minor other changes in the encoder section...
*
*         - The decoder now treats 0x00-chars in the chunk id position as 
*           padding bytes and does not prompt any sytax error any more.
*           Now the "slidbar.gif" created by "GifBuilder 0.2" works...
*
*         - Introduced the option STRICTSYNTAX which prints additionally 
*           syntax errors (like the 0x00 padding bytes above...).
*
*         - Added Installation script "Install DataType".
*           It would be very nice if someone has the time to write a
*           1:1 CBM Installer version of it...
*
*       V2.3
*         - Fixed a couple of holes in the descriptor code which may caused
*           endless loops or - more worse - crashes in the past.
*           Thanks to Guillaume Ubbelohde (950947@mercure.umh.ac.be) for
*           reporting the bug.
*
*         - The descriptor now checks if the given gif stream is a
*           multi-picture gif stream or not, nothing else !
*           Syntax checking/version checking etc. is NOT done, this is the
*           job of the decoder (e.g. the class library).
*
*         - The error checking in the decoder has been improved a little
*           bit. Still not perfect, but...
*
*         - The decoder now explains most errors in a console window.
*
*         - Implemented a NOVERBOSE option (as requested by many people),
*           which turns all error messages OFF.
*           Be carefull with this option !!!
*
*         - Minor code changes and optimisations.
*
*   NOTES
*       This datatype first scans the whole animation to get index
*       information (if the LOADALL switch was set in the prefs-file,
*       the entire animation will be loaded), colormaps will be loaded
*       immediately.
*
*   SEE ALSO
*       animation.datatype,
*       anim.datatype,
*       mpegsystem.datatype, mpegvideo.datatype,
*       picmovie.datatype,
*       cdxl.datatype, avi.datatype, quicktime.datatype,
*       moviesetter.datatype,
*       film.datatype,
*       directory.datatype,
*       markabletextdtclass
*
*******************************************************************************
*
*/


/****** gifanim.datatype/--input_format-- ************************************
*
*    NAME
*        GIF ANIM -- GIF Animation format
*
*    DESCRIPTION
*        <Not written yet, sorry>
*
*    SEE ALSO
*        gif.doc, gif89a.doc, compress.gif
*
*******************************************************************************
*
*/



/*****************************************************************************/

DISPATCHERFLAGS
struct IClass *ObtainGIFAnimEngine( REGA6 struct ClassBase *cb )
{
    return( (cb -> cb_Lib . cl_Class) );
}

/*****************************************************************************/

#if !defined(__AROS__)
DISPATCHERFLAGS
struct Library *LibInit( REGD0 struct ClassBase *cb, REGA0 BPTR seglist, REGA6 struct ExecBase *sysbase )
{
    cb -> cb_SegList = seglist;

    cb -> cb_SysBase = sysbase;
#else
/* Open superclass */
ADD2LIBS("datatypes/animation.datatype", 0, struct Library *, AnimationBase);

static int LibInit(struct ClassBase *cb)
{
    D(bug("[gifanim.datatype] %s()\n", __func__);)
#endif

    InitSemaphore( (&(cb -> cb_Lock)) );

#if !defined(__AROS__)
    /* Kickstart V3.0 ? */
    if( (cb -> cb_SysBase -> LibNode . lib_Version) >= 39UL )
    {
      /* Obtain ROM libs */
      if( cb -> cb_UtilityBase = OpenLibrary( "utility.library", 39UL ) )
      {
        if( cb -> cb_DOSBase = OpenLibrary( "dos.library", 39UL ) )
        {
          if( cb -> cb_GfxBase = OpenLibrary( "graphics.library", 39UL ) )
          {
            cb -> cb_CyberGfxBase = OpenLibrary( CYBERGFXNAME, CYBERGFXVERSION );

            if( cb -> cb_IntuitionBase = OpenLibrary( "intuition.library", 39UL ) )
            {
              return( (&(cb -> cb_Lib . cl_Lib)) );

#ifdef COMMENTED_OUT
              CloseLibrary( (cb -> cb_IntuitionBase) );
#endif /* COMMENTED_OUT */
            }

            CloseLibrary( (cb -> cb_CyberGfxBase) );
            CloseLibrary( (cb -> cb_GfxBase) );
          }

          CloseLibrary( (cb -> cb_DOSBase) );
        }

        CloseLibrary( (cb -> cb_UtilityBase) );
      }
    }

    return( NULL );
#else
    return TRUE;
#endif
}

/*****************************************************************************/

#if !defined(__AROS__)
DISPATCHERFLAGS
LONG LibOpen( REGA6 struct ClassBase *cb )
{
    LONG retval = (LONG)cb;
    BOOL success = TRUE;

    ObtainSemaphore( (&(cb -> cb_Lock)) );

    /* Use an internal use counter */
    cb -> cb_Lib . cl_Lib . lib_OpenCnt++;
    cb -> cb_Lib . cl_Lib . lib_Flags &= ~LIBF_DELEXP;

    if( (cb -> cb_Lib . cl_Lib . lib_OpenCnt) == 1U )
    {
      if( (cb -> cb_Lib . cl_Class) == NULL )
      {
        success = FALSE;

        if( cb -> cb_DataTypesBase = OpenLibrary( "datatypes.library", 39UL ) )
        {
          if( cb -> cb_SuperClassBase = OpenLibrary( "datatypes/animation.datatype", 41UL ) )
          {
            if( cb -> cb_Lib . cl_Class = initClass( cb ) )
            {
              success = TRUE;
            }
          }
        }
      }
    }

    if( !success )
    {
      CloseLibrary( (cb -> cb_SuperClassBase) );
      CloseLibrary( (cb -> cb_DataTypesBase) );

      cb -> cb_DataTypesBase = cb -> cb_SuperClassBase = NULL;

      (cb -> cb_Lib . cl_Lib . lib_OpenCnt)--;

      retval = 0L;
    }

    ReleaseSemaphore( (&(cb -> cb_Lock)) );

    return( retval );
}

/*****************************************************************************/

DISPATCHERFLAGS
LONG LibClose( REGA6 struct ClassBase *cb )
{
    LONG retval = 0L;

    ObtainSemaphore( (&(cb -> cb_Lock)) );

    if( cb -> cb_Lib . cl_Lib . lib_OpenCnt )
    {
      (cb -> cb_Lib . cl_Lib . lib_OpenCnt)--;
    }

    if( ((cb -> cb_Lib . cl_Lib . lib_OpenCnt) == 0U) && (cb -> cb_Lib . cl_Class) )
    {
      if( FreeClass( (cb -> cb_Lib . cl_Class) ) )
      {
        cb -> cb_Lib . cl_Class = NULL;

        CloseLibrary( (cb -> cb_SuperClassBase) );
        CloseLibrary( (cb -> cb_DataTypesBase) );
      }
      else
      {
        cb -> cb_Lib . cl_Lib . lib_Flags |= LIBF_DELEXP;
      }
    }

    ReleaseSemaphore( (&(cb -> cb_Lock)) );

    if( (cb -> cb_Lib . cl_Lib . lib_Flags) & LIBF_DELEXP )
    {
      retval = LibExpunge( cb );
    }

    return( retval );
}

/*****************************************************************************/

DISPATCHERFLAGS
LONG LibExpunge( REGA6 struct ClassBase *cb )
{
    BPTR seg;

    if( cb -> cb_Lib . cl_Lib . lib_OpenCnt )
    {
      cb -> cb_Lib . cl_Lib . lib_Flags |= LIBF_DELEXP;

      seg = NULL;
    }
    else
    {
      Remove( (&(cb -> cb_Lib . cl_Lib . lib_Node)) );

      seg = cb -> cb_SegList;

      CloseLibrary( (cb -> cb_IntuitionBase) );
      CloseLibrary( (cb -> cb_CyberGfxBase) );
      CloseLibrary( (cb -> cb_GfxBase) );
      CloseLibrary( (cb -> cb_DOSBase) );
      CloseLibrary( (cb -> cb_UtilityBase) );

      FreeMem( (APTR)((ULONG)(cb) - (ULONG)(cb -> cb_Lib . cl_Lib . lib_NegSize)), (ULONG)((cb -> cb_Lib . cl_Lib . lib_NegSize) + (cb -> cb_Lib . cl_Lib . lib_PosSize)) );
    }

    return( (LONG)seg );
}
#else
ADD2INITLIB(LibInit, 0);
#endif
