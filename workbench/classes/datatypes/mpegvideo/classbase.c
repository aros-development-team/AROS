
/*
**
**  $VER: classbase.c 1.11 (7.11.97)
**  mpegvideo.datatype 1.11
**
**  Library routines for a DataTypes class
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

#ifdef __SASC
/* SAS/C compatibly hack. This may be caused by a math library */
__stdargs
void _XCEXIT( long dummy )
{
}
#endif /* __SASC */


/****** mpegvideo.datatype/--datasheed-- *************************************
*
*   NAME
*       mpegvideo.datatype -- data type for mpeg video streams
*
*   SUPERCLASS
*       animation.datatype
*
*   DESCRIPTION
*       The mpegvideo data type, a sub-class of the animation.datatype, is
*       used to load and play mpeg 1 video streams (Layer I).
*
*   METHODS
*       OM_NEW -- Create a new animation object from a description file. The
*           source may only be a file.
*
*       OM_DISPOSE -- Dispose instance and contents (frames, colormaps etc.),
*           then pass msg to superclass
*
*       OM_UPDATE -- Perform an ICM_CHECKLOOP check, and if succesfull, the
*           method will be executed like OM_SET downstairs.
*
*       OM_SET -- Pass msg to superclass, and if the mpegvideo.datatype
*           instance is the "top instance", call GM_RENDER if retval from
*           superclass was != 0UL.
*
*       ADTM_LOADFRAME -- Fill in struct adtFrame with requested information
*           from internal FrameNode list like bitmap, colormap, samples, etc..
*
*       All other methods are passed unchanged to superclass.
*
*   ATTRIBUTES
*       Following attributes are set by the object and are READ-ONLY for
*       applications:
*       DTA_NominalHoriz        -- same as ADTA_Width
*       DTA_NominalVert         -- same as ADTA_Height
*       DTA_ObjName             -- same as DTA_Name
*       ADTA_Width              -- set by video stream
*       ADTA_Height             -- set by video stream
*       ADTA_Depth              -- set by video stream (or by prefs)
*       ADTA_NumColors          -- set by ADTA_Depth or to the num. of 
*                                  palette entries
*       ADTA_ColorRegisters     -- obj's palette with ADTA_NumColors entries
*       ADTA_CRegs              -- obj's palette with ADTA_NumColors entries
*       ADTA_Frames             -- total number of frames
*       ADTA_FramesPerSecond    -- set by video stream
*       ADTA_ModeID             -- calculated internally or set by prefs
*       ADTA_KeyFrame           -- key frame
*       ADTA_Sample             -- sample (optional, if a sample is attached)
*       ADTA_SampleLength       -- sample length (optional, see ADTA_Sample)
*       ADTA_Period             -- sample period (optional, see ADTA_Sample)
*       ADTA_Volume             -- sample volume (optional, see ADTA_Sample)
*       ADTA_Cycles             -- sample cycle  (optional, see ADTA_Sample)
*
*   BUGS
*       - If the mpeg video stream is not 100% standart conform,
*         the decoder part of this datatype will CRASH your machine.
*         Innocent memory will be overwritten !!
*
*       - If you don't have my "paranimdtcpatch" patch running,
*         animation.datatype (animation.datatype 40.7 (28.09.93)) subclasses
*         will suffer under timing problems when two or more
*         animation.datatype objetcs are playing. I've released a matching
*         patch, DON'T blame me for this bug. Any comments for this problem
*         should be related  to the "paranimdtcpatch" project.
*         "paranimdtcpatch" is available in the aminet
*         ("util/dtype/panimdtcptch012.LhA").
*         NOTE that animation.datatype V41 does NOT require the patch
*         anymore.
*
*       - In large videos, the frames at the end will be played slower than
*         those at the beginning of the file. This is the result of the
*         sequential search internally used (only serious with more than 25000
*         frames (mc68030/50mhz)).
*         Will be fixed.
*
*       - The LOADALL mode does not work perfectly. It sometimes "forgets" to
*         fill some frames during loading.
*
*       - The time code in the NOLODALL mode is slightly wrong (I assume up
*         to +/- 6 frames).
*
*       - The NOPFRAMES and NOBFRAMES options are incompatible with the
*         NOLOADALL mode.
*
*       - CyberGFX 24 bit output mode uses 32 bit ARGB bitmaps, but the A
*         (alpha) byte is not set to 0xFF (visible), instead it is set to 0.
*         The same for 15 bit output mode, where the MSB is a alpha mask bit.
*
*   TODO
*       - better error handling (and error messages)
*
*       - code cleanup
*
*       - bug fixing
*
*       - Writing an amigaguide document
*
*       - EHB
*
*   HISTORY
*       V1.1
*         First (official) public release.
*
*       V1.2
*         Major code cleanup. I decided to rebuild the whole project from
*         scratch instead of releasing my internal V1.8. The reason is that
*         many things coded in it wasn't done with the required care. Now
*         I try to fix this.
*
*         - Sorry, but in the V1.1 release, I've forgotten the COPYRIGHT
*           notice. Because I don't have the original ones (my code was a
*           mergin of mpeg_play 1.X, 2.0, I've taken them from the
*           mpeg_play 2.0 distribution).
*
*         - Implemented the NOPFRAMES and NOBFRAMES switch (for those people
*           who like it).
*
*         - The loader now returns usefull return codes instead of failing
*           silently.
*
*         - Decrased the default depth from 7 to 5 planes. This is a more
*           reliable default value for grayscale mpeg videos (screen or
*           window).
*
*         - Implemented "DICECOLOR" and "ORDERED" dithering.
*           The options COLORERROR and DICESKIP are for fine-tuning
*           speed <--> color output.
*
*         - Introduced MAXFRAME (maximum number of frame to load) and
*           SKIPFRAMES to allow loading of large videos.
*
*         - MODEID option added (for those which don't like the default
*           settings).
*
*         - The datatypes descriptor file was WRONG. Now it should be correct.
*           If not, send me a mail (and if possible, the sample file which
*           won't work (packed with LhA for compression and CHECKSUMMING).
*           (Later versions of the descriptor should use additional code
*           to check the data).
*
*       V1.3
*         - COLOR output implemented.
*
*         - DICECOLOR: Found a bug (?? very strange one ??) in my code which
*           avoids the usage of a frame palette. DICECOLOR was intended
*           to produce high-quality per frame color remapping instead using
*           a global anim palette. Broken (e.g. working, but after a frame is
*           finished, the color table isn't cleared for now...).
*
*         - I found out that the most time is consumed by the WritePixelArray
*           and copy functions. Later versions should contain a custom
*           WritePixelArray replacement which should be able to write directly
*           into FAST-RAM instead of writing into CHIP-RAM and them do a copy
*           into FAST-RAM. This would save __MUCH__ time.
*
*         - Silly mistake: A
*           SetIoErr( 0L ); len = FRead( ..., requested ); sequence returns
*           under various fs IoErr() == (-1L) or something else. The loader
*           then aborts with this error code. (The code worked with RAM: disk,
*           my VideoCD-fs ("white book" mode)) and AMICDROM, but wasn't tested
*           with FFS or something else).
*           The code now uses Read instead of FRead, checks for
*           (len != requested), and only if TRUE IoErr() will be checked for
*           any error.
*
*         - Introduced BUFFER preferences option.
*
*         - A CTRL-D signal send to the loading process now stops the load.
*           This feature was implemented in V1.2, but I forgot to write it
*           down here.
*
*         - The autodoc now has a TOC (table of contents).
*
*         - Support for Martin Apel's VMM (Virtual Memory Manager).
*           (USEVMM switch in prefs file). The bitmaps can now be in virtual
*           memory (the colormaps are traditional allocated by GetColorMap
*           and cannot be in virtual memory (not yet nor in the future !)).
*           The mpegvideo.datatype uses the vmm.library, this allows
*           virtual memory usage even if the vmm.prefs disables virtual memory
*           for all other tasks.
*           NOTE: VMM is Shareware !!
*
*       V1.4
*         - vmm.library is now opened on demand (e.g. the USEVMM switch was
*           set in the prefs file) inside the OM_NEW method.
*           This fixes two problems:
*           First: vmm.library was opened even it wasn't used.
*           Second: OpenLibrary( "vmm.library", ... ) inside the LibInit
*           function caused various problems.
*           vmm.library will be closed by LibExpuge function.
*
*         - Removed serial verbose output. Now, if the VERBOSE switch was set
*           in the prefs file, all verbose output will be printed to
*           "CON://///auto/wait/close/inactive".
*
*         - Set the SC NOOPTIMIZERPEEPHOLE switch for mc68060 support:
*           phase5 said that SAS/C 6.56 has a small bug in the peephole
*           optimizer which may cause trouble with mc68060. The peephole
*           optimizer will be turned on again if I know more details (and
*           a workaround).
*
*         - The product of LUM_RANGE * CR_RANGE * CB_RANGE was limited to
*           a maximum of 512.  LUM_RANGE, CR_RANGE, CB_RANGE can now have
*           any positive value up to 255. A value of 0 is treated as 1,
*           fixing the problem of a possible division by zero.
*
*         - Found a bug in COLOR dithering, which causes quality loss and
*           color disorientation. Fixing this bug will take MUCH time.
*           Not fixed yet.
*
*         - To match the "DataTypes proposal", changes have been made:
*           1. The location of the prefs file is now ENV:Classes/DataTypes/
*              mpegvideo.prefs instead of ENV:DataTypes/mpegvideo.prefs
*           2. Subclasses of mpegvideo.datatype are not supported. Any
*              attempt to create a subclass object of mpegvideo.datatype
*              will be rejected by mpegvideo.datatype.
*
*         - Partial code cleanup. I've implemented partial support for
*           output depths up to 16 bits. Currently, I'm limited to a maximum
*           depth of 8 bitplanes (e.g. 256 colors) for two reasons:
*           First, the system WritePixelArray8 function handles only byte
*           width pen indexes, second, animation.datatype handles only a
*           bitmaps up to 8 planes. Future releases of animation.datatype may
*           handle deeper bitplanes. (Custom players like my DBufDTAnim which
*           are using their own display code are able to display deeper
*           bitmaps yet.)
*
*         - Found out a little problem: When using VMM (swap file) with
*           Enforcer, enforcer hits may block your system (occured on a
*           A2000 Apollo 2030 mc68030 board).
*           Who knows an answer for this problem ?
*
*       V1.5
*         Minor changes to support special compiled versions for 68020+,
*         fpu etc.
*
*         - Implemented the IGNOREERRORS switch, which attempts to ignore any
*           error during loading.
*
*         - Removed some dead code.
*
*       V1.6
*         The datatypes supports now scaling and sound. Sound was implemented
*         for two reasons:
*         1. For those streams which are distributed with a matching sound
*            file.
*         2. I'm reworking mpegsystem.datatype (which can play system streams,
*            e.g. video with interleaved audio). The goal is to implement
*            mpegsystem.datatype as a subclass of mpegvideo.datatype. The
*            audio stream will be parsed using mpegaudio.datatype.
*         Note that mpegvideo.datatype is still a GID_ANIMATION type datatype.
*         The GID_#? idetifiers belongs to the source data, not the
*         modifications done by a datatype code (a mpeg 1 video stream does
*         not contain any audio information, the sound is attached later).
*
*         - Implemented the WIDTH and HEIGHT options in the prefs file to
*           support scaling.
*
*         - Implemented SAMPLE and VOLUME options to support sound.
*
*         - Now uses BestModeID for selecting the screen mode of the
*           animation. The MODEID preference option overides this.
*           The old behavior was to set the ADTA_ModeID attribute only if the
*           MODEID prefs option was set, otherwise the default from
*           animation.datatype was taken (which was everytimes 0).
*
*         - Fixed the FPS preference option. If it was set, the value was got
*           from the depth option instead using the given value.
*
*         - Implemented a processing gauge as requested by Allan Odgaard
*           (Duff@DK-Online.DK) and many other people. The matching
*           NOPROGRESSGAUGE switch disables it. If the input filehandle is a
*           pipe, the gauge may not work properly.
*
*         - Implemented a lowermem limit (MINTOTALMEM option) for those
*           people who wants to see at least the beginning of a big anim
*           (idea by Allan Odgaard (Duff@DK-Online.DK)).
*
*         - Implemented multi-line preferences, supports comments and
*           per-project settings (MATCHPROJECT option). The old preference
*           files/vars are compatible.
*
*         - The stack size for the OM_NEW method is now checked. If the
*           required size (curretly 12kb) isn't available, a requester will
*           notify the user and the method returns an error.
*
*       V1.7
*         - Recompiled with SAS/C 6.57. Switched the peephole optimizer on
*           (see V1.4 bugs).
*
*         - Rewrote the DICECOLOR dithering/remapping code. The DICECOLOR
*           color mode now creates a colormap per frame (animation.datatype
*           40.7 does not support these colormap changes per frame (it's
*           output looks like a color trash). Custom animation players which
*           uses animation.datatype subclasses for loading (like my
*           DBufDTAnim) don't have this problem. animation.datatype V41 will
*           support these colormaps-per-frame.
*
*           I changed the algorithm from a single-pass into a multi-pass
*           operation, which allows the code to run on other output modes
*           like ORDERED or FS (Floyd-Steinberg). A side-effect of this
*           change is that options like COLORERROR and DICESKIP are selected
*           automatically, which is more user-friendly.
*
*           If you want to get the old DICECOLOR output, use
*           DITHER=COLOR PALETTEPERFRAME options.
*
*         - The mpegvideo.datatype uses 24 bit-colors internally and
*           writes 32 bits per (r,b,g) gun. Now the high order bits of each
*           color gun are replicated through the whole INT32.
*
*         - Fixed a bug in the SAMPLE option, which caused possible crashes.
*
*         - Fixed a bug in LibExpunge (didn't check lib_OpenCnt), which is
*           also present in all my other external BOOSI classes (and
*           datatypes).
*           Thanks to Guenter Niki (gniki@informatik.uni-rostok.de) for
*           reporting this bug.
*
*         - Implemented the mpeg saving code (mpeg encoder). Currently,
*           only mpeg-1 streams are written out.
*           (Will be enabled in the public versions ONLY upon request;
*           code is currently under construction).
*
*         - Increased the stack requirements from 12kb up to 16kb, mainly
*           to allow more recursive operations.
*
*         - Implemented random access to frames (e.g you need not to load
*           the whole animation, decoding is done on the fly).
*           (Will be enabled in the public versions ONLY upon request;
*           code is currently under construction,
*           the matching LOADALL switch it set everytimes).
*
*       V1.8
*         - Added partial support for mpeg-2 (MPEG-2 does NOT work yet !),
*           both encoder+decoder.
*
*         - Added my own WritePixelArray8 replacement, which operates
*           directly on the fast-mem bitmaps. This saves some internal
*           copies, chipmem buffer for WPA8 etc.
*
*         - Added my own scaling routine, replacing
*           graphics.library/BitMapScale (which needs chipmem as temp.
*           buffer).
*           Does not work properly yet.
*
*         - Fixed the gauge, which didn't work correctly with large streams
*           (>= 8MB).
*           Fixed.
*
*         - The gauge ha now a text info, which shows the remaining time
*           to decode.
*
*         - Added experimental stack swapping code. The "Need more stack"-
*           requester has been removed for this reason.
*
*           This also fixes a possible deadlock of "input.device" because
*           the GM_LAYOUT method was running with low stack.
*           Fixed.
*
*         - Color table setup now retries color table build with an increased
*           COLORERROR if table (set by LUM_RANGE * CR_RANGE * CB_RANGE) does
*           not fit. This allows any #?_RANGE value.
*
*         - Fixed different problems within color setup. Now this should
*           work with a higher quality, and a little bit faster.
*
*         - Fixed the descriptor, which din't match all mpeg video streams.
*           Fixed.
*
*         - DTM_WRITE should return ERROR_NOT_IMPLEMENTED if DTWM_RAW mode is
*           requested (becuase the encoder has been disabled, e.g. commented
*           out). Now this is correctly done.
*           Fixed.
*
*         - The mc68060-Version now checks for the AFF_68060 (1L<<7) execbase
*           flag instead using the AFF_68040.
*           Fixed.
*
*         - Added QUALITY prefs option and matching float-dct code.
*
*       V1.9
*         - Recompiled with SAS/C 6.58. Should fix some mc68060 related
*           problems.
*
*         - Minor and major housekeeping changes.
*
*         - Added HAM code and HAM dither option as requested by many people.
*           Thanks to Olaf Barthel for his fast HAM conversion code.
*
*         - DTA_ObjName is now set (and equals to DTA_Name).
*
*         - GM_LAYOUT/DTM_PROCLAYOUT code has been removed because
*           animation.datatype class does the same.
*
*         - Removed NOREMAP switch (and matching DTM_FRAMEBOX code) because
*           this was an ugly hack. After all, it seems that MuliView 
*           sets ADTA_Remap to FALSE on it's custom screens, and that
*           using ADTA_Remap == FALSE on a screen which does not have the
*           matching size may cause trouble.
*
*         - ADTA_NumColors is now set to the number of used colors instead
*           of 1UL << anim_depth. This saves some pens when allocating
*           shared pens on shared screens.
*           When palette-per-frames are used, the number of colors equals to
*           1UL << anim_depth again.
*
*         - Moved scaling code before dithering code. It now scales the lum,
*           cr, cb data, which is more usefull when doing ordered or HAM
*           dithering.
*
*         - VMM support seems to be safe...
*
*         - Changed the whole prefs-system. Now the defaults options are more
*           usefull and are adapted correctly to some settings.
*
*         - DITHER option now defaults to HAM, for those people who don't
*           like to edit the prefs-file.
*
*         - Added NOPALETTEPERFRAME/S, PFRAMES/S, BFRAMES/S, NOLOADALL/S,
*           PROGRESSGAUGE/S, NOQUALITY/S switches to support multi-line
*           prefs-files which uses the MATCHPROJECT feature.
*
*         - Internal timing now uses animation.datatype V41
*           ADTA_TicksPerFrame instead of ADTA_FramesPerSecond. This
*           makes timing more precise.
*           (ADTA_TicksPerFrame means 1200 / fps, based on realtime.library's
*           timing model).
*
*         - Increased the decoding the speed a little bit. But this may affect
*           stability if someone feeds a corrupt mpeg file in the datatype.
*           I try to fix the problem...
*
*         - Now uses Peter McGavin's (p.mcgavin@irl.cri.nz) WritePixelArray8
*           code (the C version).
*
*         - Fixed the encoder stack problem.
*
*         - The encoder now has no problems with dynamic frame delays (e.g.
*           alf_Duration field in adtFrame).
*
*         - The SKIP option now uses struct adtFrame -> alf_Duration instead
*           of creating empty frames. This causes that the SKIPFRAMES option
*           works only with animation.datatype V41.
*
*         - Fixed the bug that the last sample might be too long, which
*           caused unallocated memory reads (but only a few bytes) and small
*           clicks at the end of the animation.
*
*         - The startup-code has been rewritten, the code now gurantees that
*           the order of OpenLibrary and CloseLibrary are correct and in a
*           more usefull order.
*
*         - Updated the autodoc a little bit.
*
*       V1.10
*         INTERNAL RELEASE
*
*         - Large code cleanup.
*
*         - If we reach the file end, we now add a end of sequence code
*           manually to avoid any problems if we does not see the end code
*           (this occurs in the NOLOADALL mode in the past...).
*
*         - Removed all remaining parts of "recmpeg". Now I'm allowed to
*           release the source :-)
*
*         - Moved the encoder to the mpegvideo.datatype V2.
*
*         - Moved the YUV -> 24-bit interleaved-planar to
*           mpegvideo.datatype V2.
*
*         - Removed the whole MPEG-2 support. mpegvideo.datatype V2 does this
*           better.
*
*         - Wrote a replacement for the 24 bit planar code (currently
*           non-interleaved).
*
*         - Rewrote the 24/15/16 bit dither code.
*           Should fix some visual problems.
*
*         - Fixed the bug that animation.datatype V41 wasn't detected
*           correctly (the version check didn't match, therefore
*           any V41 related features were not used).
*           Fixed.
*
*         - Added GAMMACORRECT and CHROMACORRECT options.
*
*         - Removed BestModeIDA code because animation.datatype does the
*           same job (better).
*
*         - Fixed the bug that MODEID=0 turns the internal mode selection
*           on instead of using LORES.
*           This also requires a change in the prefs behaviour, the
*           default value for the MODEID option is now "-1".
*           Fixed.
*
*         - Removed all internal accesses to ADTA_BitMapHeader because they
*           are unneccesary (dead code).
*
*         - The VERBOSE output is now turned on if a serious error occurs.
*
*         - If frames are skipped (using the NOPFRAMES or NOBFRAMES option)
*           now the duration of the last frame is increased to get sync
*           with the stream time.
*
*         - Fixed the bug that the total number of frames might be incorrect
*           in rare cases.
*           Fixed.
*
*         - Fixed a bug in the color setup of ORDERED dithering which
*           caused that the values runned from 0 upto 256 (256 == 0 in an
*           UBYTE). Now the color values are correctly running from 0-255.
*           Fixed.
*
*           Fixed the same bug in GRAY dither setup.
*           Fixed.
*
*       V1.11
*         - Enabled the NOLOADALL mode for the public.
*
*         - Small code cleanup to remove some unneccesary parts of code
*           (mainly the obsolete encoder support).
*
*         - Fixed the bug that the depth of a CyberGFX ARGB bitmap must be 32
*           instead of 24.
*           Fixed.
*
*         - Added some safety in CyerGFX LockBitMap handling (which has a bug
*           in tag parsing).
*
*         - Added workaround for NOLOADALL mode that ADTM_LOADFRAME returns
*           1UL instead of NULL if it forgets a frame (to avoid that the
*           playback stops...).
*           Not nice...
*
*         - Fixed a bug in 24bit -> HAM dithering introduced in V1.10.
*           Fixed.
*
*         - Replaced stack swapping code by a "standard module".
*
*         - Increased the "default" buffer size from 4096 up to 16384.
*           This makes the gauge less accurate for small streams, but
*           gives a small speedup when parsing large streams.
*
*   SEE ALSO
*       animation.datatype,
*       mpegsystem.datatype, mpegvideo.datatype
*       picmovie.datatype,
*       paranimdtcpatch
*
*******************************************************************************
*
*/



/*****************************************************************************/

DISPATCHERFLAGS
struct IClass *ObtainMPEGVideoEngine( REGA6 struct ClassBase *classbase )
{
    return( (classbase -> cb_Lib . cl_Class) );
}

/*****************************************************************************/

#if !defined(__AROS__)
DISPATCHERFLAGS
struct Library *LibInit( REGD0 struct ClassBase *classbase, REGA0 BPTR seglist, REGA6 struct ExecBase *sysbase )
{
#ifdef USEIEEELIB
    extern struct Library *MathIeeeDoubBasBase;
#endif /* USEIEEELIB */

    classbase -> cb_SegList = seglist;
    classbase -> cb_SysBase = sysbase;
#else
/* Open superclass */
ADD2LIBS("datatypes/animation.datatype", 0, struct Library *, AnimationBase);

static int LibInit(struct ClassBase *classbase)
{
    bug("[mpegvideo.datatype] %s()\n", __func__);
#endif
    
    InitSemaphore( (&(classbase -> cb_Lock)) );

#if !defined(__AROS__)
#ifdef REQUIREDAFF
    /* Check if the requested cpu/fpu is available*/
    if( ((classbase -> cb_SysBase -> AttnFlags) | REQUIREDAFF) != (classbase -> cb_SysBase -> AttnFlags) )
    {
      return( NULL );
    }
#endif /* REQUIREDAFF */

    /* Requires at least Kick 3.0 */
    if( (classbase -> cb_SysBase -> LibNode . lib_Version) >= 39UL )
    {
      /* Obtain ROM libs */
      if( classbase -> cb_UtilityBase = OpenLibrary( "utility.library", 39UL ) )
      {
#undef GfxBase
        if( classbase -> cb_GfxBase = (struct GfxBase *)OpenLibrary( "graphics.library", 39UL ) )
        {
          classbase -> cb_CyberGfxBase = OpenLibrary( CYBERGFXNAME, CYBERGFXVERSION );

          if( classbase -> cb_IntuitionBase = OpenLibrary( "intuition.library", 39UL ) )
          {
            if( classbase -> cb_DOSBase = OpenLibrary( "dos.library", 39UL ) )
            {
#ifdef USEIEEELIB
              if( MathIeeeDoubBasBase = OpenLibrary( "mathieeedoubbas.library", 38UL ) )
              {
#endif /* USEIEEELIB */
                /* vmm.library will be opened on demand (see dispatch.c) */
                classbase -> cb_VMMBase = NULL;

                return( (&(classbase -> cb_Lib . cl_Lib)) );

#ifdef COMMENTED_OUT
                CloseLibrary( MathIeeeDoubBasBase );
#endif /* COMMENTED_OUT */

#ifdef USEIEEELIB
              }

              CloseLibrary( (classbase -> cb_DOSBase) );
#endif /* USEIEEELIB */
            }

            CloseLibrary( (classbase -> cb_IntuitionBase) );
          }

          CloseLibrary( (classbase -> cb_CyberGfxBase) );
          CloseLibrary( (&(classbase -> cb_GfxBase -> LibNode)) );
        }

        CloseLibrary( (classbase -> cb_UtilityBase) );
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
LONG LibOpen( REGA6 struct ClassBase *classbase )
{
    LONG retval = (LONG)classbase;
    BOOL success = TRUE;

    ObtainSemaphore( (&(classbase -> cb_Lock)) );

    /* Use an internal use counter */
    classbase -> cb_Lib . cl_Lib . lib_OpenCnt++;
    classbase -> cb_Lib . cl_Lib . lib_Flags &= ~LIBF_DELEXP;

    if( (classbase -> cb_Lib . cl_Lib . lib_OpenCnt) == 1U )
    {
      if( (classbase -> cb_Lib . cl_Class) == NULL )
      {
        success = FALSE;

        if( classbase -> cb_DataTypesBase = OpenLibrary( "datatypes.library", 39UL ) )
        {
          if( classbase -> cb_SuperClassBase = OpenLibrary( "datatypes/animation.datatype", 39UL ) )
          {
            if( classbase -> cb_Lib . cl_Class = initClass( classbase ) )
            {
              success = TRUE;
            }
          }
        }
      }
    }

    if( !success )
    {
      CloseLibrary( (classbase -> cb_SuperClassBase) );
      CloseLibrary( (classbase -> cb_DataTypesBase) );

      classbase -> cb_DataTypesBase = classbase -> cb_SuperClassBase = NULL;

      (classbase -> cb_Lib . cl_Lib . lib_OpenCnt)--;

      retval = 0L;
    }

    ReleaseSemaphore( (&(classbase -> cb_Lock)) );

    return( retval );
}

/*****************************************************************************/

DISPATCHERFLAGS
LONG LibClose( REGA6 struct ClassBase *classbase )
{
    LONG retval = 0L;

    ObtainSemaphore( (&(classbase -> cb_Lock)) );

    if( classbase -> cb_Lib . cl_Lib . lib_OpenCnt )
    {
      (classbase -> cb_Lib . cl_Lib . lib_OpenCnt)--;
    }

    if( ((classbase -> cb_Lib . cl_Lib . lib_OpenCnt) == 0U) && (classbase -> cb_Lib . cl_Class) )
    {
      if( FreeClass( (classbase -> cb_Lib . cl_Class) ) )
      {
        classbase -> cb_Lib . cl_Class = NULL;

        CloseLibrary( (classbase -> cb_SuperClassBase) );
        CloseLibrary( (classbase -> cb_DataTypesBase) );
      }
      else
      {
        classbase -> cb_Lib . cl_Lib . lib_Flags |= LIBF_DELEXP;
      }
    }

    ReleaseSemaphore( (&(classbase -> cb_Lock)) );

    if( (classbase -> cb_Lib . cl_Lib . lib_Flags) & LIBF_DELEXP )
    {
      retval = LibExpunge( classbase );
    }

    return( retval );
}

/*****************************************************************************/

DISPATCHERFLAGS
LONG LibExpunge( REGA6 struct ClassBase *classbase )
{
    BPTR                   seg;
#ifdef USEIEEELIB
    extern struct Library *MathIeeeDoubBasBase;
#endif /* USEIEEELIB */

    if( classbase -> cb_Lib . cl_Lib . lib_OpenCnt )
    {
      classbase -> cb_Lib . cl_Lib . lib_Flags |= LIBF_DELEXP;

      seg = NULL;
    }
    else
    {
      Remove( (&(classbase -> cb_Lib . cl_Lib . lib_Node)) );

      seg = classbase -> cb_SegList;

      CloseLibrary( (classbase -> cb_VMMBase) );
#ifdef USEIEEELIB
      CloseLibrary( MathIeeeDoubBasBase );
#endif /* USEIEEELIB */
      CloseLibrary( (classbase -> cb_DOSBase) );
      CloseLibrary( (classbase -> cb_IntuitionBase) );
      CloseLibrary( (classbase -> cb_CyberGfxBase) );
      CloseLibrary( (&(classbase -> cb_GfxBase -> LibNode)) );
      CloseLibrary( (classbase -> cb_UtilityBase) );

      FreeMem( (APTR)((ULONG)(classbase) - (ULONG)(classbase -> cb_Lib . cl_Lib . lib_NegSize)), (ULONG)((classbase -> cb_Lib . cl_Lib . lib_NegSize) + (classbase -> cb_Lib . cl_Lib . lib_PosSize)) );
    }

    return( (LONG)seg );
}
#else
ADD2INITLIB(LibInit, 0);
#endif
