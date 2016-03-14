
/*
**
** $Id$
**  anim.datatype 1.12
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

struct ClassBase;
struct AnimInstData;
struct FrameNode;

/* main includes */
#include "classbase.h"


/****** anim.datatype/MAIN ***************************************************
*
*    INTRODUCTION
*        Datatypes class for IFF ANIM animations. Based on the CBM datatypes
*        example source/ documents written by David Junod and the public
*        domain Anim5 code by Jim Kent.
*
*    SHAREWARE
*        Starting with V1.8, anim.datatype is Shareware.
*
*        "anim.datatype" is this version and it's contributed files
*        are Copyright 1995/96/97 by Roland Mainz except otherwise noted.
*        All rights reserved.
*
*        You are allowed to use anim.datatype for 30 days. If you
*        want to use it after this time, please register.
*        The registration fee is 15 DM, 10 US-$ or equivalent.
*        There is no different registered version, this version of
*        anim.datatype is fully functional.
*
*        Payment my be done using bank transfer (prefereed; see AUTHOR
*        section below for my bank account), euro-scheques (on my name)
*        or cash money in an envelope (ON YOUR OWN RISK !).
*
*        New versions are released through the Aminet.
*
*        As a service for registered users I will notify you about
*        new versions if I get your email address.
*        To be able to do this, you should fill in the following form
*        and send this to my email/snail-mail address:
*
*        -- snip --
*
*        Product/Version: anim.datatype 1.10 (4.8.97)_______________________
*
*        Name:     _________________________________________________________
*
*        Street:   _________________________________________________________
*
*        City/ID:  _________________________________________________________
*
*        Country:  _________________________________________________________
*
*        Phone:    _________________________________________________________
*
*        EMail:    _________________________________________________________
*
*        Shareware fee: _ 10 US-$
*                       _ 15 DM
*                       _ other (in a amount which equals to the US-$ fee)
*
*        Payed by  _ cash money in envelope (on your own risk !)
*                  _ euro-scheque (on my name)
*                  _ bank transfer
*
*        Comments: _________________________________________________________
*
*                  _________________________________________________________
*
*                  _________________________________________________________
*
*        -- snip --
*
*    REQUIREMENTS
*        - You need at least Kick/WB 3.0.
*          | Many people wrote me that they cannot find an
*          | "animation.datatype" class.
*          | Only the 3.1 release contains it. (Subclasses of)
*          | animation.datatype can run under 3.0.
*
*        - "datatypes/animation.datatype", >= V39.
*          "animation.datatype 40.7 (28.09.93)" requires itself some
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
*        an IFF anim stream using GMultiView, MultiView, AmigaGuide or
*        SwitchWindow will load and play the animation. If the source was a
*        file, anim.datatype loads frames dynamically from disk, if the
*        source was the clipboard, anim.datatype caches the whole anim.
*
*        anim.datatype supports also the clipboard as input, e.g.
*        "MultiView CLIPBOARD", ClipView or SwitchWindow are able to show
*        IFF ANIMs from clipboard.
*
*        If you want to save the current animation in anim.datatype's local
*        format, use MultiView's "Project/Save As..." menu (or GMultiView's
*        "Project/Save As Raw...").
*        anim.datatype saves the current animation, stating with the current
*        frame as IFF ANIM-3.
*        Other compression formats such as ANIM-0/1/2/4/5/7/8/J will
*        be implemeted later.
*
*        If you want to attach samples to the animation, you must edit the
*        prefs file (ENV:Classes/DataTypes/anim.prefs) and add the following
*        line:
*        VERBOSE SAMPLE="ram:have_a_nice_day.8svx"
*        Which loads and attaches the sample "ram:have_a_nice_day.8svx" to the
*        animation. See anim.datatype.doc/preferences for a complete
*        description of the prefs file.
*
*    INSTALLATION
*        After unpacking this archive:
*        Because this version does not include an Installer script, you have
*        to do the installation manually through the shell:
*
*          - Unpack this archive and copy the "anim.datatype" to
*            SYS:Classes/DataTypes:
*
*     Copy CLONE FROM "anim.datatype" TO "SYS:Classes/DataTypes/anim.datatype"
*
*          - Then copy the datatypes descriptor into the DEVS:DataTypes
*            directory.
*            If the descriptor already exists, you should not replace it,
*            otherwise you may loose "toolnodes" and other settings stored in
*            the existing descriptor.
*
*     Copy CLONE FROM "ANIM(%|.info)" TO DEVS:Datatypes/
*
*    SOURCE
*        Partial source is included as an example how to write an
*        animation.datatype subclass which deals with things like
*        "delta-compression" techniques.
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
*        Bank account: Kto. 4866 02-508
*        At:           Postbank Köln,
*                      BLZ 37010050
*
*        EMAIL is also available (if you want to send me attachments
*        larger than 1MB (up to 5MB, more with my permission):
*
*        GISBURN@w-specht.rhein-ruhr.de
*
*        Up to December 1997 I'm reachable using this email address, too:
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
*        The  entire  "anim.datatype"  package  may  be  noncommercially
*        redistributed, provided  that  the package  is always  distributed
*        in it's complete  form (including it's documentation). A small
*        copy fee  for media costs is okay but any kind of commercial
*        distribution is strictly forbidden without my permission !
*        Comments and suggestions how to improve this program are
*        generally appreciated!
*
*        Thanks to David Junod, who wrote the animation.datatype and lots of
*        the datatypes example code, Jim Kent, Eric Graham and other people
*        for their compression formats, Matt Dillon for his DICE, Olaf
*        'Olsen' Barthel for his help, ideas and some text clips from his
*        documentations.
*
******************************************************************************
*
*/



/****** anim.datatype/--datasheed-- ******************************************
*
*   NAME
*       anim.datatype -- data type for IFF ANIM animations
*
*   SUPERCLASS
*       animation.datatype
*
*   DESCRIPTION
*       The anim datatype, a sub-class of the animation.datatype, is used to
*       load and play IFF anim animations.
*       It supports all currently defined IFF anim compressions
*       (0/1/2/3/4/5/6/7/8/J) and any "interleave count".
*       ILBM BODY Interleaved bitmaps can be uncompressed or compressed using
*       cmpByteRun1.
*       Using the prefs-file, any sound can be attached to the animation.
*
*   METHODS
*       OM_NEW -- Create a new animation object from a description file. The
*           source may be a file or a clipboard unit, both given as an
*           IFFHandle, or you may create an empty object (DTST_RAM).
*
*       OM_DISPOSE -- Dispose instance and contents (frames, colormaps, sounds
*           etc.), then pass msg to superclass.
*
*       OM_UPDATE -- Perform an ICM_CHECKLOOP check, and if succesfull, the
*           method will be executed like OM_SET downstairs.
*
*       OM_SET -- Pass msg to superclass and call GM_RENDER if retval from
*           superclass was != 0UL.
*
*       DTM_WRITE -- Save object's contents in local (IFF ANIM-3) or
*           superclass (IFF ILBM) format.
*
*       ADTM_START -- Start playback.
*
*       ADTM_PAUSE -- Pause playback.
*
*       ADTM_STOP -- Stop playback.
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
*       DTA_ObjName             -- set by anim file (IFF generic NAME chunk)
*       DTA_ObjAuthor           -- set by anim file (IFF generic AUTH chunk)
*       DTA_ObjAnnotation       -- set by anim file (IFF generic ANNO chunk)
*       DTA_ObjCopyright        -- set by anim file (IFF generic (C)  chunk)
*       DTA_ObjVersion          -- set by anim file (IFF generic FVER chunk)
*       DTA_TotalVert           -- set by BitMapHeader (ILBM BMHD chunk)
*       DTA_TotalHoriz          -- set by BitMapHeader (ILBM BMHD chunk)
*       ADTA_Width              -- set by BitMapHeader (ILBM BMHD chunk)
*       ADTA_Height             -- set by BitMapHeader (ILBM BMHD chunk)
*       ADTA_Depth              -- set by BitMapHeader (ILBM BMHD chunk)
*       ADTA_Frames             -- number of frames in animation
*       ADTA_FramesPerSecond    -- FPS rate (possibly set by DPAN chunk)
*       ADTA_KeyFrame           -- Key frame of animation
*
*   BUGS
*       - In large videos, the frames at the end will be played slower than
*         those at the beginning of the file. This is the result of the
*         sequential search internally used (only serious with more than 25000
*         frames (mc6030/50mhz)).
*         May or may not be fixed.
*
*       - Animations with more than 8 planes will crash the system
*         (anim.datatype handels 12 bit animations, but animation.datatype 
*         V40 handles only anims with a depth of 8).
*
*       - ANIM-1-Code has been disabled because it draws the blocks to X and H
*         dimensions which are divisible by 8 (can't be fixed: no test files).
*
*       - The ANHD (AnimHeader) ah_Mask-field will be ignored for ANIM-0,
*         2,3,4,5,6,7,8,J. ANIM-1 (acmpXORILBM) respects this.
*
*       - The BMHD (BitMapHeader) bmh_Masking-field will be ignored.
*
*       - ANIM-4 isn' tested yet (no test files).
*
*       - ANIM-4 has been disabled for the reason above.
*
*       - ANIM-J is twice as slow as it should be because ANIM-J operates on
*         interleaved bitmaps, but (currently) anim.datatype uses
*         non-interleaved bitmaps internally. Therefore the current
*         implementation interleaves the bitmap, then unpacks, then
*         de-interleaves the bitmap. Not very efficient.
*
*       - Any flags in the ANHD (AnimHeader) ah_Flags field except ahfLongData
*         (bit 0; used by Anim-7/8) and ahfXOR (bit 1; used for all
*         compression types except ANIM-J (not required, ANIM-J has an
*         internal XOR mode) and anim-0) are ignored. ANIM-4 suffers under
*         these problems (cannot be fixed without example files).
*
*       - ANIM-6 left/right channels are viewed interleaved. Upon request,
*         I can add options to view the left or right channel only.
*
*       - The internal "rollback" machanism used for creating full images from
*         given DLTAs is slow. This is caused by implementation details,
*         not the idea itself.
*
*       - If an animations contains colormaps-per-frame and the given CMAP
*         contains less colors than (1UL << anim_depth), the remaining colors
*         are filled with black. I don't know where to take the remaining
*         colors (from last frame/previous (e.g. interleaved) frame)...
*         Comments __VERY__ welcome.
*
*       - The ANIM-3 encoder has a slight problem in run-length compression;
*         therefore this part has been disabled.
*
*       - The encoder does not support dynamic timing properly. Instead,
*         all frames are saved.
*
*       - The encoded initial ANIM-3 IFF ILBM BODY is uncompressed.
*
*       - The encoder rejects any attempt to work with non-planar or
*         interleaved planar bitmaps.
*
*   TODO
*       - Fixing the bugs above.
*
*       - Internal cache for packed DLTA chunks. This may speed up the
*         internal rollback mechanism used for random frame access.
*
*       - Other compression formats when saving, including the popular
*         ANIM-5.
*
*       - PERIOD options to modify the given sound.
*
*       - Add support for animation.datatype V41 ADTA_TicksPerFrame timing.
*
*       - Write the "--input_format--"-Autodoc section.
*
*   HISTORY
*       V1.1
*         Released to the Waldspecht-BBS for testing.
*
*       V1.2
*         Released to the Waldspecht-BBS for testing.
*
*       V1.3
*         Released to the Waldspecht-BBS for testing.
*
*       V1.4
*         First Aminet release.
*
*         - Partial code cleanup
*
*         - Recompiled with SAS/C 6.57.
*
*         - Redirected serial "verbose" output to a CON:-window, including
*           the matching VERBOSE prefs option.
*
*         - Changed the VERBOSE output a little bit.
*
*       V1.5
*         - Implemented palette per frame (ILBM CMAPs per frame).
*           (animation.datatype 40.7 does not support these colormap changes
*           per frame (it's output may look like a color trash). Custom
*           animation players which uses animation.datatype subclasses for
*           loading (like my DBufDTAnim) don't have this problem.
*           animation.datatype will support these colormaps-per-frame in one
*           of the next updates.)
*
*         - Internal changes to be able to support up to 12 planes in an anim.
*           The animation.datatype 40.7 supports only anims up to 8 planes,
*           otherwise the machine will crash. My animation.datatype V41 will
*           support this.
*
*         - The generic IFF ANNO, AUTH, FVER, NAME and "(C) " chunks are now
*           moved to the corresponsing DataTypes attributes if they appear in
*           a FORM ILBM.
*           The data will be moved like this:
*           ANNO -> DTA_ObjAnnotation
*           AUTH -> DTA_ObjAuthor
*           FVER -> DTA_ObjVersion
*           NAME -> DTA_ObjName
*           (C)  -> DTA_ObjCopyright
*
*         - DTA_Title equals now to DTA_ObjName (e.g. a given NAME chunk). If
*           this is NULL, DTA_Name is used instead.
*
*         - If a DPAN (DPaint Anim) chunk occurs, the FPS rate is taken from
*           the "flags" field.
*           Thanks to Brian Jones (bjones@cadvision.com) for reporting this.
*           If the value taken is 0, the old, fixed rate (5 fps) is used
*           again.
*
*         - Fixed a hole in the state machine which causes problems when an
*           animation contains no frames. Should work now.
*           Fixed.
*
*         - An error in the prefs-file will now force the VERBOSE option.
*
*         - Implemented sound. A sample can now be attached to the animation
*           using the SAMPLE/VERBOSE options.
*           Note that anim.datatype is still a GID_ANIMATION (anim WITHOUT
*           sound) type datatype.
*           The GID_#? idetifiers belongs to the source data, not the
*           modifications done by a datatype code (a IFF ANIM does not contain
*           any sound data, the sound is attached later).
*
*       V1.6
*         - Disabled the optimizer. This fixes a mysterious System-Crash
*           when resizing the parent application window. Don't know why this
*           works now :-((
*
*       V1.7
*         - Reorganisation of source (includes small cleanup).
*
*         - Fixed a silly hole in the state machine which causes the
*           datatype to return errors on colormap changes (e.g. ILBM CMAPs).
*           Error codes were returned like this:
*           (Number_of_colors_in_CMAP * 3).
*
*         - Enabled the optimizer (see V1.6) again. Since reorganisation of
*           the src, the problem never occured again. Any hint ?
*
*         - Fixed a bug in LibExpunge (didn't check lib_OpenCnt), which is
*           also present in all my other external BOOSI classes (and
*           datatypes).
*           Thanks to Guenter Niki (gniki@informatik.uni-rostok.de) for
*           reporting this bug.
*
*       V1.8
*         * Shareware
*           Starting with V1.8, anim.datatype is Shareware.
*
*         - Added experimental stack swapping code. The "Need more stack"-
*           requester has been removed for this reason.
*           This also fixes a possible deadlock of "input.device" because
*           the GM_LAYOUT method was running with low stack.
*
*         - Uses OpenFromLock when obtaining a fh for random frame access.
*           This allows the usage of "virtual filesystems" (datatypes.library
*           V45 allows such things).
*
*         - Now supports saving in the local format (e.g. IFF ANIM,
*           currently only ANIM-3).
*           Note that saveing begins with the __current__ frame and can be
*           controlled with the ADTA_Frame, ADTA_Frames and
*           ADTA_FrameIncrement attributes.
*
*         - Now support DTST_RAM (creates an empty anim.datatype object).
*
*         - Now supports subclasses of anim.datatype, mainly to fill in
*           the ADTM_LOADFRAME/ADTM_UNLOADFRAME methods when using an empty
*           (e.g. created with DTST_RAM) anim.datatype object.
*
*         - Now the high order bits of each color gun are replicated through
*           the whole INT32 in the colormap.
*
*       V1.9
*         - Fixed a bug which prevents saving in local (IFF ANIM-3) format
*           using an (created using DTST_RAM) empty object (like "DTConvert"
*           does).
*           Fixed.
*
*       V1.10
*         - Moved IFF ANIM defines etc. to classdata.h include file.
*
*         - Reworked file scan. This fixes problems with many animations
*           where the second frame was treated as an ILBM instead of an DLTA.
*           Fixed.
*
*         - Implemented ADTM_START, ADTM_STOP and ADTM_PAUSE to get a more
*           smoothly start and stop, see method autodocs for details.
*
*         - Implemented Martin Tailefer's AsyncIO code (currenty broken,
*           has been disabled for now).
*
*         - Removed unnecessary GM_LAYOUT code; DTA_Total(Vert|Horiz) are now
*           set in OM_NEW.
*
*         - Fixed ADTM_LOADFRAME that if a frame can't be loaded (seek error,
*           or timestamp not found) now a matching error code will be
*           returned in Result2.
*
*         - Removed NOREMAP feature and matching DTM_FRAMEBOX code because
*           this was an ugly hack...
*
*         - XORBitMap has been rewritten, XOR mode for
*           ANIM-2/-3/-4/-5/-6/-7-/8 should now be a little bit faster;
*           this also fixes two bugs:
*           - The bitmaps were incorrectly merged together when the bitmap
*             size grows over 64k per plane (for example 1280 * 512)
*             Fixed.
*           - Due rounding errors the last 8 pixels were not processed
*             Fixed.
*
*         - Plane pointers are now on a quad-long boundary to get more speed
*           on a mc68040.
*
*         - Internal CopyBitMap has been optimized to use CopyMemQuick if
*           possible.
*
*         - Added compatibility code for DPaint ANIM Brushes, compressed with
*           ANIM-5 and XOR mode, but which don't have the XOR bit set.
*           Added NODPAINTBRUSHPATCH to disable this function.
*           This fixes the problems with the animations I found in the CanDo
*           distribution.
*
*         - Added support for GRAB chunk (grabbing point of animation).
*           Does only work with animation.datatype V41 (which implements
*           PDTA_Grab attribute).
*
*         - Stack swapping code has been improved; the stack is now only
*           swapped if stacksize falls below 16384 / 2 bytes; this saves
*           the stack allocation for ADTM_LOADFRAME / ADTM_UNLOADFRAME
*           in animation.datatype V41.
*
*         - Added WaitBlit in OM_DISPOSE to wait for blitter which may
*           use our bitmaps.
*
*         - Added seperate memory pool for frame bitmaps, which is
*           set up to have the correct size for this job.
*           This should speed up bitmap memory allocations.
*           The pool's size is reduced if there is not enougth memory.
*           The other "misc usage" pool is increased to 32k get the
*           correct size for DLTA temp mem usage.
*           The datatype now uses AllocVecPooled instead of AllocVec
*           for delta buffers.
*
*         - Cut some chars from the AnimHeader dump output. Now it fits
*           on a single-line in a 640 pixel CON: window (topaz 8 font).
*
*         - Added NOLOADALL and CMAPS switch for multi-line prefs using
*           the MATCHPROJECT option.
*
*         - Added additional checking code for CAMG chunk data. Invalid
*           flags are now removed.
*
*         - Added support for dynamic timing, e.g. anim.datatype now uses
*           AnimHeader's ah_AbsTime and ah_RelTime fields. Does only work
*           properly with animation.datatype V41 (for animation.datatype
*           V40 this option is turned off per default and turned on if
*           superclass is animation.datatype V41); matching NODYNAMICTIMING
*           and DYNAMICTIMING options have been added; modifications
*           have been made in AttachSamples and ADTM_LOADFRAME, too.
*
*         - Fixed the bug that the last sample's length wasn't calculated
*           correctly, which may have caused Enforcer read-hits (never
*           seen, but...).
*
*         - Added SAMPLESPERFRAME option to override own calculations.
*           This allows to get rid of rounding errors in such cases.
*
*         - Added "wanted" message for IFF ANIM-4 animations. If you have
*           such an animation, and if you are the FIRST who send's me a VALID
*           IFF ANIM-4 compressed animation, you'll get $10. No joke.
*
*         - ANIM-3 encoder has been cut down in functionality to get
*           (temporary) rid of a bug in run-length encoding (DPaint IV AGA
*           does not like this !?).
*
*         - The encoder does not not fail if a subclass returns
*           ERROR_OBJECT_NOT_FOUND for ADTM_LOADFRAME. It simply tries
*           to load the next frame. This fixes some problems with dynamic
*           timing.
*
*         - The encoder now checks explicitly for non-planar or interleaved
*           planar input bitmaps, which are NOT supported yet (e.g.
*           such an attempt results in ERROR_NOT_IMPLEMENTED).
*
*         - A fps value greater than 60 fps found in a DPAN chunk is now
*           treated as invalid.
*
*       V1.11
*         - Recompiled with SAS/C 6.58. May fix some mc68060 related
*           problems.
*
*         - Found the longstanding bug that animation.datatype V40.7
*           didn't free some frames. Reason is that ADTM_LOADFRAME
*           may be used like "realloc". Now alf_UserData is checked;
*           any given frame will be freed (ADTM_UNLOADFRAME).
*           Fixed.
*
*         - ADTM_UNLOADFRAME now clears alf_UserData to indicate that the
*           frame has been freed.
*
*         - Now supports anims deeper than 8 planes, but a colormap
*           (e.g. a CMAP chunk) is still expected.
*
*         - Fixed the bug that a free of the current frame caused an frame
*           to be not freed. (The idea was to hold the current frame for
*           following delta accesses; this has been replaced by the idea
*           of the "posted free").
*
*         - Implemented the idea of a "posted free". For delta accesses,
*           it's not very usefull to free the frame when the next frame
*           needs it for it's delta access. Therefore, the anim.datatype
*           now manages a "free list", where the free of the previous
*           and the previous-previous frame will be posted until
*           it is really not longer in use.
*
*         - Fixed a bug in the LOADALL mode that the wrong previous
*           frame was used (due a change in V1.10).
*
*         - The single framenodes now caches a pointer to their previous
*           frame. This should speed up loading a little bit.
*
*         - If an animation has dynamic timing and the superclass
*           (animation.datatype) has the ADTA_AdaptiveFPS flag set,
*           the playback speed now defaults to 60 fps.
*
*         - Saved an AllocVecPooled in ADTM_LOADFRAME by merging multiple
*           delta buffer allocations to one allocation (which can hold the
*           largest delta).
*
*         - The stack swapping code now allocates it's memory without
*           the MEMF_CLEAR flags, which should speed up things.
*
*         - Removed BestModeIDA code, because animation.datatype does
*           the same.
*
*         - The options setting for DPaint brush compatibility patch
*           worked wrong (the order was wrong). Now the options
*           work as described.
*           Fixed.
*
*         - Fixed a bug in the encoder that the last color of a dynamic
*           pallete as not copied (GetRGB32 got numcolors - 1 instead
*           of numcolors).
*           Fixed.
*
*       V1.12
*         - Replaced the custom stack swapping code by my "standard" module.
*
*         - Now the default FPS rate is 10 if animation.datatype is
*           < V41. If V41 is running with ADTA_AdaptiveFPS, the
*           fps is set to 60.
*
*         - Fixed ModeID handling. The previous behaviour was that
*           a 0 mode id causes the datatype to select it's own mode id.
*           But 0 means LORES. Now the default is -1 (which means
+           INVALID_ID), which causes the datatype to do it's own
*           calculations.
*           Fixed.
*
*         - OM_DISPOSE now preserves Result2 (IoErr()) to avoid that an 
*           error code may get lost.
*
*   NOTES
*       This datatype first scans the whole animation to get index
*       information (if the LOADALL switch was set in the prefs-file,
*       the entire animation will be loaded), colormaps will be loaded
*       immediately.
*
*   SEE ALSO
*       animation.datatype,
*       mpegsystem.datatype, mpegvideo.datatype,
*       picmovie.datatype,
*       cdxl.datatype. avi.datatype, quicktime.datatype,
*       moviesetter.datatype,
*       film.datatype,
*       directory.datatype,
*       markabletextdtclass
*
*******************************************************************************
*
*/


/****** anim.datatype/--input_format-- ****************************************
*
*    NAME
*        IFF ANIM -- IFF animation format
*
*    DESCRIPTION
*        <Not written yet, sorry>
*
*        acmpILBM             -- ILBM BODY
*        acmpXORILBM          -- XOR
*        acmpLongDelta        -- Long Delta
*        acmpShortDelta       -- Short Delta
*        acmpDelta            -- General Delta
*        acmpByteDelta        -- Byte Vertical
*        acmpStereoByteDelta  -- Stereo Byte Vertical (left/right channel)
*        acmpAnim7            -- Anim7 (nonstandard format)
*        acmpAnim8            -- Anim5 compression using LONG/WORD data
*        acmpAnimJ            -- Eric Grahams compression format
*
*
*    SEE ALSO
*        - "ANIM.DOC" IFF Animspecs,
*          "ANIM.BRUSH.DOC" Anim specs,
*          "ANIM.OP6" Anim specs,
*          "ANIM7.DOC" Anim specs
*          "ANIM.OP8" Anim specs
*
*        - ARKM Devices (Addison Wesley): IFF part
*
*        - iffparse.library autodocs
*
*        - AAP/AAC docs (for a complete description of the Anim7 format)
*
*        - Viewtek (VT) distribution for Anim 7 info
*
*******************************************************************************
*
*/

/*****************************************************************************/

DISPATCHERFLAGS
struct IClass *ObtainAnimEngine( REGA6 struct ClassBase *cb )
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
ADD2LIBS("datatypes/animation.datatype", 0, struct Library *, AnimationBase)

static int LibInit(struct ClassBase *cb)
{
    D(bug("[anim.datatype] %s()\n", __func__);)
#endif

    InitSemaphore( (&(cb -> cb_Lock)) );

    /* set up overload'able unpack hooks */
    cb ->unpackilbmbody = (unpack_ilbm_t) generic_unpackilbmbody;
    cb ->xorbm = (unpack_xor_t) generic_xorbm;
    cb ->unpacklongdelta = (unpack_delta_t) generic_unpacklongdelta;
    cb ->unpackshortdelta = (unpack_delta_t) generic_unpackshortdelta;
    cb ->unpackbytedelta = (unpack_delta_t) generic_unpackbytedelta;
    cb ->unpackanim4longdelta = (unpack_delta4_t) generic_unpackanim4longdelta;
    cb ->unpackanim4worddelta = (unpack_delta4_t) generic_unpackanim4worddelta;
    cb ->unpackanim7longdelta = (unpack_delta_t) generic_unpackanim7longdelta;
    cb ->unpackanim7worddelta = (unpack_delta_t) generic_unpackanim7worddelta;
    cb ->unpackanim8longdelta = (unpack_delta_t) generic_unpackanim8longdelta;
    cb ->unpackanim8worddelta =  (unpack_delta_t) generic_unpackanim8worddelta;
    cb ->unpackanimidelta = (unpack_deltabm_t) generic_unpackanimidelta;
    cb ->unpackanimjdelta = (unpack_deltabm_t) generic_unpackanimjdelta;

#if !defined(__AROS__)
    if( (cb -> cb_SysBase -> LibNode . lib_Version) >= 39UL )
    {
      /* Obtain ROM libs */
      if( cb -> cb_UtilityBase = OpenLibrary( "utility.library", 39UL ) )
      {
        if( cb -> cb_DOSBase = OpenLibrary( "dos.library", 39UL ) )
        {
          if( cb -> cb_IFFParseBase = OpenLibrary( "iffparse.library", 39UL ) )
          {
            if( cb -> cb_GfxBase = OpenLibrary( "graphics.library",  39UL ) )
            {
              if( cb -> cb_IntuitionBase = OpenLibrary( "intuition.library", 39UL ) )
              {
                return( (&(cb -> cb_Lib . cl_Lib)) );

#ifdef COMMENTED_OUT
                CloseLibrary( (cb -> cb_IntuitionBase) );
#endif /* COMMENTED_OUT */
              }

              CloseLibrary( (cb -> cb_GfxBase) );
            }

            CloseLibrary( (cb -> cb_IFFParseBase) );
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
          if( cb -> cb_SuperClassBase = OpenLibrary( "datatypes/animation.datatype", 39UL ) )
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
      CloseLibrary( (cb -> cb_GfxBase) );
      CloseLibrary( (cb -> cb_IFFParseBase) );
      CloseLibrary( (cb -> cb_DOSBase) );
      CloseLibrary( (cb -> cb_UtilityBase) );

      FreeMem( (APTR)((ULONG)(cb) - (ULONG)(cb -> cb_Lib . cl_Lib . lib_NegSize)), (ULONG)((cb -> cb_Lib . cl_Lib . lib_NegSize) + (cb -> cb_Lib . cl_Lib . lib_PosSize)) );
    }

    return( (LONG)seg );
}
#else
ADD2INITLIB(LibInit, 0)
#endif
