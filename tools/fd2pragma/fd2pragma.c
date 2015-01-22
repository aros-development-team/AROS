/* $Id$ */
static const char version[] =
"$VER: fd2pragma 2.194 (02.01.2011) by Dirk Stoecker <software@dstoecker.de>";

/* There are four defines, which alter the result which is produced after
   compiling this piece of code. */

/* turns on usage of Amiga ReadArgs-system (requires <proto/dos.h> include) */
/* #define FD2PRAGMA_READARGS */

/* enables Amiga style file name (only relevant for fd2pragma.types file) */
/* #define FD2PRAGMA_AMIGA */

/* debugging output */
/* #define DEBUG */

/* more debugging output */
/* #define DEBUG_OLD */

/* Programmheader

        Name:           fd2pragma
        Author:         SDI
        Distribution:   PD
        Description:    creates pragmas files, lvo files, ...
        Compileropts:   -
        Linkeropts:     -

 1.2    : added pragmas for the Dice compiler. Available via switch "Dice".
        added switches "Aztec", "SAS" and "Maxon": Maxon and Aztec just
        turn on the default (except that Maxon expects pragma files to be
        called "xxx_pragmas.h" instead of "xxx_lib.h"), SAS is equal to
        Dice, except that SAS supports the pragma tagcall.
 2.0    : Added support for tag functions. See the docs for details.
        Author until this version:
        Jochen Wiedmann
        Am Eisteich 9
        72555 Metzingen (Germany)
        Tel. 07123 / 14881
 2.1   19.08.96 : now made by SDI, added correct __MAXON__ support and
        support for StormC++, added auto recognition of tagcall functions
        changed the CLI interface completely
 2.2   21.08.96 : fixed a lot of errors, added debug code
 2.3   22.08.96 : little changes
 2.4   24.08.96 : added proto-file creation
 2.5   25.08.96 : added syscall and fix for functions ending in ...DMA
 2.6   26.08.96 : fixed some errors, added CLIB parameter (used later for
        CSTUBS)
 2.7   01.09.96 : added correct Storm definition, added CLIB scan
 2.8   02.09.96 : added assembler stub functions, added first ASM-stub code
 2.9   04.09.96 : added Comment-Support
 2.10  05.09.96 : changed CSTUB creation a bit
 2.11  07.09.96 : speeded up output, reduced number of strndup calls
 2.12  26.09.96 : pressing CTRL-C in early startup brought a wrong error
        message - fixed
 2.13  30.09.96 : made RegNames field to RegNames string - shorter Exe-file
 2.14  01.10.96 : made SPECIAL 6 default, COMMENT also in LVO files
 2.15  13.10.96 : corrected an error text
 2.16  14.10.96 : added correct comment support and PRIVATE option
 2.17  19.10.96 : now Maxon-compiled in Small data mode
 2.18  22.10.96 : removed EXTERNC in Storm, Maxon and all pragmas, corrected
        the texts, again SAS compiled
 2.19  26.10.96 : added option to create FD files out of pragma files,
        reworked a lot in the source
 2.20  27.10.96 : fixed errors of previous version
 2.21  28.10.96 : fixed error in CLIB scan
 2.22  27.11.96 : SPECIAL numbers for lib and ASM code were wrong, removed
        bug in Tag function stubs
 2.23  06.12.96 : lib and stub creation still was wrong
 2.24  31.12.96 : formed stub libs matching C++ file names, corrected CLIB
        scan errors
 2.25  04.01.97 : added HEADER option (I was asked for)
 2.26  05.01.97 : added HEADER scan (in old file) and auto inserting
 2.27  10.01.97 : stub functions missed register saving, outfuncs skip now,
        when error occured (makes lots of error checking obsolete)
 2.28  11.01.97 : forgot to add offset made by register saving
 2.29  18.01.97 : now libtags and amitags defines only, when at least 1
        tagfunc
 2.30  13.02.97 : added local library base functions, rearranged SPECIAL
        options, fixed some bugs
 2.31  15.02.97 : corrected bugs inserted in previous version
 2.32  16.02.97 : and again bug fixes, still didn't work
 2.33  18.02.97 : corrected texts, added SPECIAL 28
 2.34  25.03.97 : corrected Pragma --> FD file conversion, added ##shadow
 2.35  26.03.97 : added STORMFD option, COMMENT, PRIVATE work again
 2.36  29.03.97 : corrected *tagcall scan a bit
 2.37  20.06.97 : added PASCAL stub lib production (SPECIAL 14, 15)
 2.38  01.07.97 : fixed ##end handling
 2.39  20.07.97 : added better proto file (__GNUC__ inline and pragma call),
        removed C++ comments
 2.40  24.11.97 : added new basenames to the list (devices and resources),
        added tag-exception name checking (dos, utility libraries)
 2.41  27.11.97 : fixed little bug with private functions, CSTUBS now
        special option and no longer commandline arg, SPECIAL 10-15 got
        numbers 11-16 (Sorry)
 2.42  28.11.97 : Added two new warnings for CLIB
 2.43  12.12.97 : faster FD file scan, one new warning
 2.44  19.12.97 : fixed MODE settings for SPECIAL 15,16
 2.45  30.01.98 : added function recognition, included inline creation,
        inline stuff is based on fd2inline 1.11 (incomplete)
 2.46  31.01.98 : continued inline stuff, fixed clib functions
 2.47  05.02.98 : completed inline stuff, added alias names for dos functions
 2.48  06.02.98 : changed Func interface - flags instead of tagmode
 2.49  10.02.98 : fixed inline generation a bit, added SORTED argument,
        RegNames got strings again
 2.50  11.02.98 : bug-fixes, still did not work completely, hopefully got
        all now
 2.51  12.02.98 : and bug-fixes again :-(
 2.52  15.02.98 : changed sorting order of arguments
 2.53  20.02.98 : some code style changes
 2.54  25.02.98 : added SMALLDATA model, removed 5 global variables (better
        style), stub libs use MOVEM when possible, own MemRemember function
 2.55  26.02.98 : bug fixes
 2.56  15.03.98 : added FPU support
 2.57  17.03.98 : added NOFPU keyword
 2.58  19.03.98 : little fixes
 2.59  20.03.98 : added enum and external type definitions defines
 2.60  22.03.98 : added external types file scan
 2.61  23.03.98 : fixed SAS flibcall, added FPU stubs
 2.62  28.03.98 : bug fix with NOFPU and new option FPUONLY, total new clib
        handling
 2.63  29.03.98 : really lots of bug fixes, There are so much problems.
        A better definition format would have been wonderful.
 2.64  05.04.98 : bug fixes
 2.65  07.04.98 : fixed Enforcer hit
 2.66  08.04.98 : bug fix with type detection
 2.67  20.04.98 : added GNU-only stuff
 2.68  28.04.98 : SPECIAL 8 defaults to SAS-C names now
 2.69  25.05.98 : added PowerUP stuff support
 2.70  28.05.98 : added SAS PowerUP stuff, fixed error with function
        detection in CLIB scan
 2.71  30.05.98 : added PowerUP Inlines
 2.72  12.06.98 : sorting turns of COMMENT now
 2.73  05.07.98 : added first FPC stuff, added HEADER to PowerUP stuff,
        added PASCAL header scan
 2.74  06.07.98 : finished FPC stuff
 2.75  07.07.98 : bug fixes for FPC stuff
 2.76  09.07.98 : style changes for FPC stuff, bug fixes
 2.77  11.07.98 : hopefully last FPC bug removed
 2.78  23.07.98 : style changes and bug fixes for FPC stuff, more comments
 2.79  10.08.98 : bug fix, when TO was used with a directory, clib got
        wrong path if it was a relative path description
 2.80  16.08.98 : now prints better error when filopen failed
 2.81  26.10.98 : added BMAP files for BASIC, CODE needs to use large mode
        now :-(
 2.82  28.10.98 : optimizations and bug fixes
 2.83  31.12.98 : fixed powerup stuff a bit
 2.84  05.01.99 : fixed bug in Lib creation, when Dx/Ax and FPx were mixed
 2.85  06.01.99 : added recognition of names ending in MESA, added notagcall
        comment support, void functions no longer can be tagcall
 2.86  10.01.99 : added BGUI special funcs, fixed bug in SPECIAL 42 code
 2.87  12.01.99 : added asm-text (SECTION), moved 12-17 to 13-18
 2.88  17.01.99 : better type detection, added some more basenames, some
        little bug fixes, new makefile reduces file size a lot
 2.89  17.07.99 : added union support
 2.90  12.11.99 : added new motorola syntax, opt040 and vbcc inlines
 2.91  13.11.99 : Now supports changes in OS3.5 includes, why the hell must
        such changes be? I thought new includes will bring cleanup and not
        cleandown. And the reported bugs are still unfixed, but there are
        new ones!, bug-fixes
 2.92  14.11.99 : added PPC-WOS library text and code, FD-creation moved from
        80 to 200 (now finally! - there should be enough free number space),
        added VBCC-PUP text generation
 2.93  15.11.99 : added CheckError function, moved DisplayInfoHandle to
        types definition file
 2.94  16.11.99 : added first VBCC-PowerUP-Lib production stuff, only ELF
        tables missing
 2.95  17.11.99 : finished PowerUP stub stuff, startet PPC-ABI stuff
 2.96  18.11.99 : little bug fixes
 2.97  19.11.99 : added SECTION keyword, moved 11-18 to 12-17, ahh 3 releases
        more and we get an anniversary, my first program using third revision
        digit :-)
 2.98  20.11.99 : added VBCC-WOS-Code for PPC libs
 2.99  25.11.99 : bug fixes
 2.100 17.02.00 : fixed bug for VBCC inlines
 2.101 29.02.00 : fixed name for VBCC inlines
 2.102 13.03.00 : added new style GCC inlines
 2.103 21.03.00 : bug fixed, SPECIAL 35 has VBCC stuff now.
 2.104 25.03.00 : fixed path lock problem
 2.105 11.04.00 : library HUNK_UNIT get functionname now
 2.106 13.07.00 : added E-Modules
 2.107 06.08.00 : removed VBCC inline support from 35 and moved it to 38, 35
        does now skip pragma/inline files for VBCC
 2.108 18.08.00 : added new ppc modification proto file 39, modified protos a
        bit, support for register types and function pointer args, int got
        internally type CPP_TYPE_INT
 2.109 19.08.00 : bug fixes
 2.110 24.08.00 : fixed SPECIAL 7,40-44, added SPECIAL 80-83
 2.111 31.08.00 : bug fixes
 2.112 03.09.00 : FD2Pragma.types scanner no longer accepts multi-word types.
 2.113 29.12.00 : added extern keword support for return types.
 2.114 07.01.01 : made FD2Pragma partly portable, removed 4 direct pragma
        arguments
 2.115 14.01.01 : lots of bug fixes, renamed from FD2Pragma to fd2pragma
 2.116 28.01.01 : added internal types, SPECIAL 90, NOCPPNAMES and bug fixes,
        VBCC inlines fix for data in A-regs
 2.117 04.02.01 : changed NOCPPNAMES to ONLYCNAMES, added HUNKNAME, LocCode is
        portable, added BASENAME, added VBCCWOSInlines
 2.118 07.02.01 : added destination file printout, LIBTYPE, fixes VBCC-PUP-Code
 2.119 11.02.01 : bug fixes
 2.120 17.02.01 : added NOPPCREGNAME, bug fixes
 2.121 04.03.01 : added MorphOS text
 2.122 11.03.01 : little bug fixes
 2.123 03.04.01 : now uses EXT_DEXT16 instead of EXT_REF16 also for 68k files
 2.124 08.04.01 : bug fixes, added MorphOS binary mode, finally full portable
 2.125 28.04.01 : added LVO's for PPC, started support for SFD format
 2.126 29.05.01 : fixed PPC LVO's, removed STORMFD Option (auto detection),
        now handles up to 5 alias names, finished SFD format read, added FD
        creation, added keyword checks for argument names, lots of optimizations
        and fixes, which came in hand with SFD inclusion.
        Thanks Olaf Barthel for making the SFD stuff possible.
 2.127 30.04.01 : private comments are skipped now, finished SFD production,
        fixed bugs, removed SPECIAL 8 redirect (is replaced by 80-83)
 2.128 01.05.01 : bug fixes
 2.129 03.06.01 : included support for files previous made by vbcc genauto tool
 2.130 04.06.01 : bug fixes in genauto stuff
 2.131 11.06.01 : newer types handle cia now correct
 2.132 27.06.01 : fixed crash caused by illegal interpretation of ANSI-C :-)
 2.133 28.06.01 : added VOIDBASE argument
 2.134 01.07.01 : added MorphOS types, fixed PowerUp stuff
 2.135 28.07.01 : added VBCC inline varargs support
 2.136 30.07.01 : fixed VBCC inline varargs
 2.137 18.11.01 : little bug-fix
 2.138 30.11.01 : fixed CLIB scanning (now a preparser cleans the file a lot)
 2.139 13.12.01 : fixed ==libname scan and xvsBase
 2.140 21.12.01 : fixed some uint32 in created files, which have been wrongly
        introduced in 2.1xx versions when making tool portable
 2.141 04.01.02 : fixed problem with multiple pointer function args like in
        "void (**func)(void)"
 2.142 07.01.02 : started new direct inline types 46 and 47.
 2.143 08.01.02 : Fixed warnings, bugs, card.resouce entry and added
        ==copyright directive
 2.144 09.01.02 : Fixed MUI varargs inlines
 2.145 03.03.02 : Some bug fixes
 2.146 20.05.02 : one little bug fix, added support for missing empty () in
        defective FD files
 2.147 01.05.02 : now continues when detecting no fd-arg name
 2.148 09.06.02 : fixed problem with MorphOS stubs, added AUTOHEADER keyword,
        added auto type defaults to int, fixed bug with STACK type
 2.149 24.06.02 : fixed lots of problems found when converting amissl includes
 2.150 08.08.02 : fixed inline files a bit
 2.151 31.08.02 : fixed SPECIAL 46 files (error when no args, but return value)
 2.152 01.09.02 : bug-fix with SPECIAL 47
 2.153 11.09.02 : modified SPECIAL 46 varargs on request of Sebastian Bauer
        and Olaf Barthel
 2.154 03.10.02 : added VBCC MorphOS inlines (SPECIAL 122). Thanks Frank Wille
        for design help.
 2.155 04.10.02 : optimized VBCC MorphOS text (SPECIAL 93), fixed VBCC MorphOS
        inlines
 2.156 06.10.02 : added warning about obsolete types, fixed VBCC MorphOS Code
        (SPECIAL 78)
 2.157 12.10.02 : Fixed CLIB scan problem
 2.158 19.10.02 : added CLIB define in SPECIAL 46
 2.159 16.11.02 : bugfix with SPECIAL 46 varargs redefine
 2.160 04.12.02 : fixed bug in MorphOS-vbcc code
 2.161 15.12.02 : now no longer includes clib files for GCC, the GCC inlines
        include the needed include lines directly
 2.162 26.01.03 : bug fixes, added updated fpc code made by Nils Sjöholm (it
        is not that complicated to do fixes yourself, fd2pragma's inner
        structure is really easy)
 2.163 28.01.03 : little fixes
 2.164 15.02.03 : fixed DirectInline for GCC mode, changed FPC layout
 2.165 04.01.04 : fixed VBCC TAG inlines (SPECIAL 70), added modified MorphOS
        FD file types, fixed GCC direct inlines for GCC 3
 2.166 06.01.04 : added first set of OS4 filetypes
 2.167 09.01.04 : more OS4 stuff, added library name comment scanning for SFD
 2.168 19.01.04 : some fixes (a lot of thanks to Frank Wille)
 2.169 22.01.04 : completed OS4 stuff
 2.170 28.01.04 : some more VBCC-MOS things
 2.171 26.02.04 : finished VBCC-MOS text
 2.172 09.05.04 : (phx) fixed clib-parsing problem with splitted function
        name and arguments over two lines, more flexible "base,sysv"
        recognition for MorphOS, never use "Device *" pointer in a proto file
        - should be "Library *"
 2.173 10.05.04 : fixed MOS-base,sysv to allow autodetected Tag-Functions
 2.174 23.05.04 : some fixes for MorphOS and VBCC
 2.175 11.07.04 : (phx) has to recognize and skip 'extern "C" {', as a
        result of my modifications in 2.172.
 2.176 21.09.04 : added new MorphOS VBCC inlines and some other OS4 fixes
 2.177 23.09.04 : minor bugfix
 2.178 09.10.04 : (phx) vbcc: use __linearvarargs instead of __aos4varargs
 2.179 09.11.04 : (phx) make it compile natively under AmigaOS 4.x
 2.180 07.12.04 : (phx) don't create vbcc inlines for MUI_NewObject &
        PM_MakeItem - otherwise the preprocessor gets confused
 2.181 20.12.04 : made test for MUI_NewObject and PM_MakeItem based on a field
        containing the names - allows easier expansion
 2.182 16.01.05 : (phx) experimental MorphOS "(sysv)" support, which doesn't
        need a base-register passed as first argument
 2.183 24.01.05 : added support for long long types, nevertheless files using
        that will not produce correct results for now
 2.184 07.02.05 : (phx) Fixed FuncVBCCWOSCode() (special 73) to read the
        function ptr from (bias-2) instead from (bias) for PPC0/2-ABI.
        Picasso96 support.
 2.185 08.02.05 : (phx) Special 38 (proto with vbcc inline) always generates
        an inline-include now, and is no longer restricted to MorphOS & 68k.
        Special Warp3DPPC support.
        Marked some powerpc.library functions, which were erroneously
        detected as tag functions.
 2.186 17.02.05 : fixed PPC0-mode VBCC WOS stubs
 2.187 26.03.05 : (phx) "(sysv,r12base)" in MorphOS FD-files is supported.
        I made it identical to (sysv), which both load the library base
        to r12 (correct in (sysv,r12base) mode and can't hurt in (sysv) mode).
        Allow "(void)" instead of "()" as parameter list.
        Function-pointer types can extend over multiple lines now (does
        this make sense for other types too?).
        New SDL-types: FPSmanager, Mix_Chunk, Mix_Music, Mix_MusicType,
        Mix_EffectFunc_t, Mix_EffectDone_t, Mix_Fading, IPaddress,
        TCPsocket, UDPpacket, UDPsocket, SDLNet_SocketSet,
        SDLNet_GenericSocket, TTF_Font.
        Put some of SDL-gfx functions ("...RGBA()") in the exceptions list.
 2.188 30.03.05 : (phx) Put NewObject() into the NoInline-list.
 2.189 21.05.05 : (phx) Always include emul/emulregs.h in vbcc/MOS inlines.
 2.190 23.08.05 : (phx) Use ".file <name>.o" in assembler sources, HUNK_NAME
        and ELF ST_FILE symbols. It was "<name>.s" before, which is wrong.
 2.191 01.11.05 : (phx) Rewrote FuncVBCCWOSInline() based on the MOSInline-
        function, to be able to handle varargs functions correctly.
        Also fixed WOS-text and -code generation for PPC0-ABI. 
 2.192 06.01.10 : (phx) Do vbcc MorphOS OS-calls with BCTRL instead of BLRL
        to avoid messing up the LR-stack of more recent PowerPCs (G4+).
 2.193 18.09.10 : (phx) GLContext type (tinygl).
 2.194 03.01.11 : (mazze) Fix for building it on CYGWIN.
                          Added AROS support in the proto file.
*/

/* A short note, how fd2pragma works.
Working mode for SPECIAL 200 is a bit different!
The main function parses arguments. Switches are converted into FLAG_XXX
values and stored in global "Flags" or "Flags2" variable. SPECIAL numbers
are parsed and are used to call a CreateXXX function, with its interface
depending on the need of arguments (Some have 2, some none, ...). Before
SPECIAL arguments are parsed, fd2pragma loads (S)FD file and scans it using
Scan(S)FDFile(). If SORTED is specified, the list gets sorted nearly directly
afterwards. IF CLIB argument is given, the clib file is scanned after FD file
and a clib list is created. Now SPECIAL is parsed and mode is set to any of
the MODUS_XXX values. Also the destination file name is created if not given.
The destination file is opened now.

The mode variable is used to determine the correct CreateXXX function,
which is called afterwards. This function produces file headers and stuff
like that and calls CallFunc to process each FD entry.

CallFunc gets 3 arguments. First the workmode (TAG, NORMAL, BOTH).
Second the comment method (for C it is "/%s *\x2F\n", for ASM it is "\n%s",
no comment is reached with 0 argument). The last is most important. It is the
function pointer to a function creating the entries. These functions have
always the same interface and are called through CallFunc only! They create
an entry for the specified function (e.g. FD entry). Parsing special
functions, adding comments, checking for tag-functions, ... is done by
CallFunc. It is no problem to call CallFunc multiple with different function
pointers (as is done for SPECIAL 6 pragmas).
This is also the method if information abount the type or number of functions
is needed somewhere in the begin of the output file. A special function to
collect this data needs to be started before doing real output. Althought I
do not like it much, global variables or flags can be used to store that
information.

The functions can use DoOutput to output texts in printf style or
DoOutputDirect to output all data in fwrite style. Buffering is done
automatically.

fd2pragma has its own memory managment. All memory must be allocated using
AllocListMem and is freed automatically. This is especially useful for
DupString function, which is used in FD, SFD and CLIB scanner.

Normally this source-file is to big and should be splitted into different
files compiled alone and linked together. :-) It takes about 20 minutes to
compile it on my Amiga system with optimizations turned on.

There are lots of different input and output types and some combinations
are really useless or wrong. fd2pragma has the mechanisms to catch these
cases properly, but not all cases are really checked, as there are too many
of them and each new input type increases the number of combinations.
Also not all useful combinations me be handled correctly. If you find a
case which really matters please inform me. All the others require the
users to use their brains :-)
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* These are the only allowed variable types of all related programs! */
#ifdef __amigaos4__
#include <exec/types.h>
#else
typedef signed char         int8;       /* signed 8 bit */
typedef unsigned char      uint8;       /* unsigned 8 bit */
typedef signed short int    int16;      /* signed 16 bit */
typedef unsigned short int uint16;      /* unsigned 16 bit */
typedef signed long int     int32;      /* signed 32 bit */
typedef unsigned long int  uint32;      /* unsigned 32 bit */
#endif
typedef float                fl32;      /* 32 bit IEEE float value */
typedef double               fl64;      /* 64 bit IEEE double value */
typedef char               string;      /* the string datatype [e.g. one character of string!] */
typedef char *             strptr;      /* and an string pointer */

#define EndPutM32(a, b) {uint32 epu32 = (b); (a)[0] = (uint8) (epu32 >> 24); (a)[1] = (uint8) (epu32 >> 16); \
                                             (a)[2] = (uint8) (epu32 >> 8); (a)[3] = (uint8) epu32;}
#define EndPutM16(a, b) {uint16 epu16 = (b); (a)[0] = (uint8) (epu16 >> 8); (a)[1] = (uint8) epu16;}
#define EndPutI32(a, b) {uint32 epu32 = (b); (a)[3] = (uint8) (epu32 >> 24); (a)[2] = (uint8) (epu32 >> 16); \
                                             (a)[1] = (uint8) (epu32 >> 8); (a)[0] = (uint8) epu32;}
#define EndPutI16(a, b) {uint16 epu16 = (b); (a)[1] = (uint8) (epu16 >> 8); (a)[0] = (uint8) epu16;}

#define EndPutM32Inc(a, b) {EndPutM32(a,b); (a) += 4;}
#define EndPutM16Inc(a, b) {EndPutM16(a,b); (a) += 2;}
#define EndPutI32Inc(a, b) {EndPutI32(a,b); (a) += 4;}
#define EndPutI16Inc(a, b) {EndPutI16(a,b); (a) += 2;}

#define TEXT_SAS        "__SASC"        /* verified     */
#define TEXT_SAS_60     "__SASC_60"     /* verified     */
#define TEXT_MAXON      "__MAXON__"     /* verified     */
#define TEXT_STORM      "__STORM__"     /* verified     */
#define TEXT_DICE       "_DCC"          /* in 2.0 code  */
#define TEXT_AZTEC      "AZTEC_C"       /* verified     */
#define TEXT_GNUC       "__GNUC__"      /* verified     */
#define TEXT_VBCC       "__VBCC__"      /* verified     */

#define TEMPSIZE        20480

#define FLAG_EXTERNC     (1<< 0) /* add externc statements */
#define FLAG_SYSCALL     (1<< 1) /* create SAS-C syscall pragmas */
#define FLAG_DOCOMMENT   (1<< 2) /* do comment processing */
#define FLAG_PRIVATE     (1<< 3) /* also use private functions */
#define FLAG_LOCALREG    (1<< 4) /* local file uses register call */
#define FLAG_ISPRIVATE   (1<< 5) /* for FD creation, currently working in private mode */
#define FLAG_PASCAL      (1<< 6) /* library creation with PASCAL style */
#define FLAG_SMALLDATA   (1<< 7) /* libraries use small data modell */
#define FLAG_DONE        (1<< 8) /* destination file is not empty */
#define FLAG_INLINENEW   (1<< 9) /* produce new style inlines */
#define FLAG_INLINESTUB  (1<<10) /* produce stubs style inlines */
#define FLAG_NOFPU       (1<<11) /* do not allow FPU registers */
#define FLAG_DIDERROR    (1<<12) /* one error already printed, don't print 2nd */
#define FLAG_FPUONLY     (1<<13) /* only use FPU registers */
#define FLAG_GNUPRAG     (1<<14) /* insert inline call into pragma file */
#define FLAG_POWERUP     (1<<15) /* create Phase5 PowerUP files */
#define FLAG_ASMSECTION  (1<<16) /* create SECTIONS in Asm code */
#define FLAG_NEWSYNTAX   (1<<17) /* new motorola syntax */
#define FLAG_NOMOVEM     (1<<18) /* 68040 optimization, don't use MOVEM */
#define FLAG_WOSLIBBASE  (1<<19) /* first arg is libbase for VBCC WOS */
#define FLAG_NOPPC       (1<<20) /* do not allow PPC functions */
#define FLAG_PPCONLY     (1<<21) /* only take PPC functions */
#define FLAG_STORMGCC    (1<<22) /* special workaround for StormGCC */
#define FLAG_NOSYMBOL    (1<<23) /* do not create symbol section for libs */
#define FLAG_MORPHOS     (1<<24) /* create MorphOS files */
#define FLAG_SORTED      (1<<25) /* sort the functions by name */
#define FLAG_DIDPPCWARN  (1<<26) /* we already printed ppc warning */
#define FLAG_SINGLEFILE  (1<<27) /* create single files */
#define FLAG_ONLYCNAMES  (1<<28) /* do not create C++, ASM names */
#define FLAG_BASENAME    (1<<29) /* Basename was command-line specified */
#define FLAG_DIDM68KWARN (1<<30) /* we already printed M68K warning */
#define FLAG_ABIV4       (1<<31) /* ABI V4 design for PPC-LVO */

#define FLAG2_SFDMODE            (1<< 0) /* input file was SFD file */
#define FLAG2_LIBTYPE            (1<< 1) /* libtype was specified on command line */
#define FLAG2_CLIBOUT            (1<< 2) /* output type is CLIB */
#define FLAG2_SYSTEMRELEASE      (1<< 3) /* systemrelease special comment handling */
#define FLAG2_SFDOUT             (1<< 4) /* output type is SFD */
#define FLAG2_LIBNAME            (1<< 5) /* libname was specified on command line */
#define FLAG2_SMALLCODE          (1<< 6) /* libraries use small code modell */
#define FLAG2_VOIDBASE           (1<< 7) /* library base should be of type "void *" */
#define FLAG2_INLINEMAC          (1<< 8) /* use inline macro instead of function */
#define FLAG2_DIRECTVARARGS      (1<< 9) /* direct varargs for MorphOS stub libs */
#define FLAG2_PRELIB             (1<<10) /* MorphOS gate PRELIB flag */
#define FLAG2_POSTLIB            (1<<11) /* MorphOS gate POSTLIB flag */
#define FLAG2_REGLIB             (1<<12) /* MorphOS gate REGLIB flag */
#define FLAG2_OLDVBCC            (1<<13) /* old VBCC style */
#define FLAG2_SMALLTYPES         (1<<14) /* allow small data types */
#define FLAG2_AUTOHEADER         (1<<15) /* creates auto generated header */
#define FLAG2_LIBNAMECOM         (1<<16) /* libname was specified in SFD comment */
#define FLAG2_OS4M68KCSTUB       (1<<17) /* OS4 M68K stub needs C */
#define FLAG2_SHORTPPCVBCCINLINE (1<<18) /* shorter PPC inline using argument */

#define FUNCFLAG_NORMAL     (1<<0) /* normal function */
#define FUNCFLAG_TAG        (1<<1) /* a tagcall function */
#define FUNCFLAG_ALIAS      (1<<2) /* an alias name for previous function */
#define FUNCFLAG_EXTENDMODE (1<<3) /* name and args extension for CSTUBS */

/* Different modes the main program uses, one for each different file
   type (except for those done with one function and flag settings). */
#define MODUS_STUBTEXT           1
#define MODUS_STUBCODE           2
#define MODUS_LOCALDATA          3
#define MODUS_PRAGMA             4
#define MODUS_CSTUB              5
#define MODUS_SASPOWER           6
#define MODUS_PROTOPOWER         7
#define MODUS_BMAP               8
#define MODUS_PASCAL             9
#define MODUS_VBCCINLINE        10
#define MODUS_VBCCPUPLIB        11
#define MODUS_LVOLIB            12
#define MODUS_EMODULE           13
#define MODUS_REDIRECT          14
#define MODUS_ASMTEXTSF         15
#define MODUS_VBCCPUPTEXTSF     16
#define MODUS_VBCCWOSTEXTSF     17
#define MODUS_VBCCWOSINLINE     18
#define MODUS_VBCCMORPHTEXTSF   19
#define MODUS_VBCCMORPHCODE     20
#define MODUS_LVOLIBPPC         21
#define MODUS_FD                22
#define MODUS_CLIB              23
#define MODUS_SFD               24
#define MODUS_GATESTUBS         25
#define MODUS_VBCCMORPHINLINE   26
#define MODUS_XML               27
#define MODUS_OS4_PPCSTUBS      28
#define MODUS_OS4_68KSTUBS      29
#define MODUS_LVO               50    /* and 51 and 52 and 53   */
#define MODUS_PROTO             60    /* and 61 to 69           */
/* new protos start with 90, but are added to MODUS_PROTO ! */
#define MODUS_INLINE            80    /* and 81 to 86           */
#define MODUS_VBCC              90    /* and 91 to 94           */
#define MODUS_LVOPPC            100   /* and 101                */
#define MODUS_GENAUTO           110   /* and 111 to 113         */
#define MODUS_ERROR             200

enum ABI {ABI_M68K, ABI_PPC, ABI_PPC2, ABI_PPC0};

/* call types for CallFunc */
#define TAGMODE_NORMAL  0    /* produce normal functions only */
#define TAGMODE_TAGS    1    /* produce only tag functions */
#define TAGMODE_BOTH    2    /* produce both types */

/* types specifying name method for pragma creation */
#define PRAGMODE_PRAGLIB        1
#define PRAGMODE_PRAGSLIB       2
#define PRAGMODE_PRAGSPRAGS     3
#define PRAGMODE_NONE           4

#define BIAS_START       30 /* the library start offset */
#define BIAS_OFFSET      6  /* value to switch from one to next function */

#ifndef FD2PRAGMA_AMIGA
#define EXTTYPESFILEHIDDEN ".fd2pragma.types"
#endif
#ifndef EXTTYPESFILE
#define EXTTYPESFILE       "fd2pragma.types"
#endif
#ifndef EXTTYPESFILE2
#ifdef FD2PRAGMA_AMIGA
#define EXTTYPESFILE2      "PROGDIR:fd2pragma.types"
#else
#define EXTTYPESFILE2      "/usr/local/share/fd2pragma.types"
#endif
#endif

#define AUTOHEADERTEXT "Automatically generated header! Do not edit!"

#define FDFILEEXTENSION  "_lib.fd"
#define SFDFILEEXTENSION "_lib.sfd"

static const strptr RegNames[] = {
"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
"fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7",
};

static const strptr RegNamesUpper[] = {
"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
"FP0", "FP1", "FP2", "FP3", "FP4", "FP5", "FP6", "FP7",
};

enum Register_ID {
REG_D0, REG_D1, REG_D2, REG_D3, REG_D4, REG_D5, REG_D6, REG_D7,
REG_A0, REG_A1, REG_A2, REG_A3, REG_A4, REG_A5, REG_A6, REG_A7,
REG_FP0, REG_FP1, REG_FP2, REG_FP3, REG_FP4, REG_FP5, REG_FP6, REG_FP7
};
#define MAXREGPPC       26
#define MAXREG          24 /* maximum registers of 68K */
#define MAXREGNF        16 /* maximum register number without float regs */
#define UNDEFREGISTER   255 /* for type scanner */

struct Args {
  strptr infile;
  strptr to;
  strptr clib;
  strptr header;
  int32  special;
  int32  mode;
};

struct ShortList {
  struct ShortList    *Next;
};

struct ShortListRoot {
  struct ShortList    *First;
  struct ShortList    *Last;
  size_t               Size;
};

#define AMIPRAGFLAG_PUBLIC       (1<< 0) /* is a public function */
#define AMIPRAGFLAG_A6USE        (1<< 1) /* A6 is used for this function */
#define AMIPRAGFLAG_A5USE        (1<< 2) /* A5 is used */
#define AMIPRAGFLAG_A4USE        (1<< 3) /* A4 is used */
#define AMIPRAGFLAG_D7USE        (1<< 4) /* D7 is used */
#define AMIPRAGFLAG_ARGCOUNT     (1<< 5) /* when double args, ... */
#define AMIPRAGFLAG_DIDARGWARN   (1<< 6) /* We printed a argcount warning */
#define AMIPRAGFLAG_FLOATARG     (1<< 7) /* It has a float argument */
#define AMIPRAGFLAG_DIDFLOATWARN (1<< 8) /* We printed a float warning */
#define AMIPRAGFLAG_NOCLIB       (1<< 9) /* No clib definition found */
#define AMIPRAGFLAG_CLIBARGCNT   (1<<10) /* CLIB argument count error */
#define AMIPRAGFLAG_PPC          (1<<11) /* This is an PPC function */
#define AMIPRAGFLAG_PPC0         (1<<12) /* type PPC0 */
#define AMIPRAGFLAG_PPC2         (1<<13) /* type PPC2 */
#define AMIPRAGFLAG_M68K         (1<<14) /* This is an M68K function */
#define AMIPRAGFLAG_OWNTAGFUNC   (1<<15) /* MakeTagFunction create tag */
#define AMIPRAGFLAG_MOSSYSV      (1<<16) /* MorphOS(sysv) type */
#define AMIPRAGFLAG_MOSSYSVR12   (1<<17) /* MorphOS(sysv,r12base) type */
#define AMIPRAGFLAG_MOSBASESYSV  (1<<18) /* MorphOS(base,sysv) type */
#define AMIPRAGFLAG_VARARGS      (1<<19) /* last argument is ... */

#define AMIPRAGFLAG_MOS_ALL      (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12|AMIPRAGFLAG_MOSBASESYSV)

struct AmiArgs {
  strptr                         ArgName;
  uint16                         ArgReg;
};

#define NUMALIASNAMES            5

struct AmiPragma {
  struct ShortList               List;
  uint32                         Line;
  uint32                         Flags;
  strptr                         FuncName;
  strptr                         TagName;
  struct Pragma_AliasName *      AliasName[NUMALIASNAMES];  /* alias names */
  uint16                         NumArgs;  /* register numbers */
  uint16                         CallArgs; /* argument number in fd file */
  int16                          Bias;
  int8                           NumAlias;
  enum ABI                       Abi;
  struct AmiArgs                 Args[MAXREGPPC];
};

struct Comment {
  struct ShortList       List;
  strptr                 Data;
  int16                  Bias;
  uint16                 ReservedNum;
  uint16                 Version;
  uint8                  Private; /* is a flag only */
};

struct Include {
  struct ShortList       List;
  strptr                 Include;
};

struct PragList {
  struct ShortList      List;
  struct ShortListRoot  Data;        /* contains list of PragData */
  strptr                Basename;
};

struct PragData {
  struct ShortList      List;
  struct ShortListRoot  Name;
  uint32                NumNames;
  uint32                Bias;
  uint32                NumArgs;
  uint8                 ArgReg[MAXREG];
};

struct FDData {
  strptr Name;
  strptr Basename;
  uint32 Bias;
  uint32 Mode;         /* 0 = Normal, != 0 is TagName */
  uint32 NumArgs;
  uint8  ArgReg[MAXREG];
};

/* NOTE: string creation for CPP-Functions probably does not work at all
at the moment, as every compiler uses different systems which seem to
change constantly. */

/* These CPP types match the strings used for CPP name creation. The
defines are used both for name creation and type specification. */
#define CPP_TYPE_VOID           'v'     /* void,   VOID    */
#define CPP_TYPE_BYTE           'c'     /* char,   int8    */
#define CPP_TYPE_WORD           's'     /* short,  int16   */
#define CPP_TYPE_LONG           'j'     /* long,   int32   */
#define CPP_TYPE_FLOAT          'f'     /* float,  FLOAT   */
#define CPP_TYPE_DOUBLE         'd'     /* double, DOUBLE  */
#define CPP_TYPE_INT            'i'     /* int */
#define CPP_TYPE_STRUCTURE      0
#define CPP_TYPE_VARARGS        'e'
#define CPP_TYPE_LONGLONG       'l'     /* long long, int64 */

/* These types are for string creation only. */
#define CPP_TYPE_ENUM           'E'
#define CPP_TYPE_CONST          'C'
#define CPP_TYPE_FUNCTION       'F'
#define CPP_TYPE_POINTER        'P'
#define CPP_TYPE_UNSIGNED       'U'
#define CPP_TYPE_FUNCEND        'p'
#define CPP_TYPE_REGISTER       'r'

/* Some flags to be used in CPP_NameType->Flags. */
#define CPP_FLAG_UNSIGNED       (1<<0) /* is an unsigned variable */
#define CPP_FLAG_CONST          (1<<1) /* type is const */
#define CPP_FLAG_STRPTR         (1<<2) /* this variable contains a strptr */
#define CPP_FLAG_POINTER        (1<<3) /* the variable is a pointer */
#define CPP_FLAG_ENUM           (1<<4) /* it is a enumeration */
#define CPP_FLAG_STRUCT         (1<<5) /* it is a structure */
#define CPP_FLAG_UNION          (1<<6) /* it is a union */
#define CPP_FLAG_FUNCTION       (1<<7) /* it is a function */
#define CPP_FLAG_BOOLEAN        (1<<8) /* in truth this element is bool */
#define CPP_FLAG_REGISTER       (1<<9) /* argument is register type */
#define CPP_FLAG_TYPEDEFNAME    (1<<10) /* name is created from typedef */
#define CPP_FLAG_ARRAY          (1<<11) /* this type is an array */
#define CPP_FLAG_LONG           (1<<12) /* type is long */
/* STRPTR is defined different under C and CPP -> I have to create two
names, one time unsigned char *, one time signed char *, when somewhere
a STRPTR occurs */

#define COPYCPP_PASSES          4

struct CPP_NameType { /* structure to describe a argument type */
  strptr StructureName;     /* if a structure or enum only                */
  strptr FuncArgs;          /* arguments of function - unterminated       */
  strptr TypeStart;         /* start of this type                         */
  strptr Replace;           /* replacement of type for SFD files          */
  strptr Unknown;           /* unknown type handled as int                */
  strptr FunctionName;      /* Argument name of function argument         */
  struct ClibData *FuncPtr; /* if it is a function pointer                */
  uint16 StructureLength;   /* length of the structure name               */
  uint16 ArgsLength;        /* length of FuncArgs                         */
  uint16 TypeLength;        /* length of this type                        */
  uint16 FullLength;        /* length of complete type                    */
  uint16 PointerDepth;      /* number of * in type                        */
  uint16 FuncPointerDepth;  /* number of * in function pointer            */
  uint16 Flags;             /* see above flags                            */
  uint8  Type;              /* see above defines                          */
  uint8  Register;          /* register number                            */
};

struct ClibData { /* structure to describe data in CLIB file */
  struct ClibData *     Next;         /* The next entry in this list */
  strptr                FuncName;     /* name of the function */
  struct CPP_NameType   ReturnType;   /* data for return type */
  struct CPP_NameType   Args[MAXREGPPC+1]; /* data for argument types */
  uint16                NumArgs;      /* number of arguments */
};

struct CPP_ExternNames { /* structure for EXTTYPESFILE data */
  struct CPP_ExternNames * Next;        /* The next entry in this list */
  strptr                   Type;        /* The unknown type */
  struct CPP_NameType      NameType;    /* The replacement */
};

struct CPP_TypeField {  /* structure for internal defined types */
  strptr Text;          /* name of the type */ 
  uint16 Length;        /* length of the name string */
  uint16 Flags;         /* CPP_FLAG flags */
  uint8  Type;          /* CPP_TYPE value */
};

struct CPP_Unknown {
  struct CPP_Unknown *Next;
  strptr              Unknown;
};

struct Proto_LibType {  /* structure to define structure type of base vars */
  strptr BaseName;      /* name of the library base */
  strptr StructureName; /* name of the structure to be used (0 for default) */
  strptr LibraryName;   /* name of the library (maybe 0 for default method) */
  strptr ShortBaseName; /* short name of the library base */
};

struct Pragma_ExecpName { /* structure to specify special tagnames */
  strptr FunctionName;  /* function name */
  strptr TagName;       /* tag name to be used for this function */
}; /* TagName 0 is valid as well to disable tagfunctions */

struct Pragma_AliasName {
  strptr FunctionName;
  strptr AliasName;
  uint32 Type;
};

#define NTP_NORMAL      0    /* no tags/args    */
#define NTP_TAGS        1    /* TagFunction     */
#define NTP_ARGS        2    /* ArgFunction     */
#define NTP_UNKNOWN     3    /* CommentFunction */

struct NameList {
  struct ShortList      List;
  uint32                Type;    /* set by OptimizeFDData */
  strptr                NormName;
  strptr                PragName;
};

struct InFile {
  strptr pos;
  strptr buf;
  size_t size;
};

/* EHF definitions! */
#define HUNK_PPC_CODE   0x4E9
#define HUNK_RELRELOC26 0x4EC
#define EXT_RELREF26    229

/* ------------------------------------------------------------------ */
/* A short set of ELF definitions, see pasm sources in vbcc release for an
more complete set of stuff or get elf documentation. These are needed for
VBCCPUPCode function. */
#define ELFCLASS32      1
#define ELFDATA2MSB     2
#define EV_CURRENT      1       /* version information */
#define ET_REL          1       /* type information */
#define EM_POWERPC      20

#define SHT_NULL        0       /* inactive */
#define SHT_PROGBITS    1       /* program information */
#define SHT_SYMTAB      2       /* symbol table */
#define SHT_STRTAB      3       /* string table */
#define SHT_RELA        4       /* relocation */

#define SHF_ALLOC       0x2     /* needs memory when started */
#define SHF_EXECINSTR   0x4     /* executable instructions */

#define SHN_ABS         0xFFF1

#define EI_NIDENT       16
#define EI_MAG0         0
#define EI_MAG1         1
#define EI_MAG2         2
#define EI_MAG3         3
#define EI_CLASS        4
#define EI_DATA         5
#define EI_VERSION      6

#define STB_LOCAL       0
#define STB_GLOBAL      1
#define STT_FUNC        2
#define STT_NOTYPE      0
#define STT_SECTION     3
#define STT_FILE        4
#define ELF32_ST_INFO(b,t)      (((b)<<4)+((t)&0xf))
#define ELF32_R_INFO(s,t)       (((s)<<8)+(uint8)(t))

#define R_PPC_ADDR16_LO  4
#define R_PPC_ADDR16_HA  6
#define R_PPC_REL24     10
#define R_PPC_SDAREL16  32

struct ArHeader {
  string ar_name[16];             /* name */
  string ar_time[12];             /* modification time */
  string ar_uid[6];               /* user id */
  string ar_gid[6];               /* group id */
  string ar_mode[8];              /* octal file permissions */
  string ar_size[10];             /* size in bytes */
  string ar_fmag[2];              /* consistency check */
};

/* AmigaOS hunk structure definitions */
#ifdef __amigaos4__
#include <dos/doshunks.h>
#else
#define HUNK_UNIT       999
#define HUNK_NAME       1000
#define HUNK_CODE       1001
#define HUNK_BSS        1003
#define HUNK_ABSRELOC32 1004
#define HUNK_EXT        1007
#define HUNK_SYMBOL     1008
#define HUNK_END        1010
#define HUNK_DREL16     1016

#define EXT_DEF         1       /* normal definition */
#define EXT_ABS         2       /* Absolute definition */
#define EXT_REF32       129     /* 32 bit absolute reference to symbol */
#define EXT_DEXT16      134     /* 16 bit data relative reference */
#endif
/* ------------------------------------------------------------------ */

static struct Args             args            = {0,0,0,0,6,0};
static struct InFile           in              = {0,0,0};
static FILE *                  outfile;
static struct ClibData *       clibdata        = 0;
static struct ShortListRoot    AmiPragma       = {0,0,sizeof(struct AmiPragma)},
                               Comment         = {0,0,sizeof(struct Comment)},
                               Includes        = {0,0,sizeof(struct Include)};
static struct CPP_ExternNames *extnames        = 0;
static struct CPP_Unknown     *unknown         = 0;
static strptr                  BaseName        = 0; /* the correct basename */
/* the filename part of basename without Base */
static strptr                  ShortBaseName   = 0;
/* like ShortBaseName, but upper case */
static strptr                  ShortBaseNameUpper = 0;
static strptr                  HEADER          = 0;
static strptr                  Copyright       = 0;
static strptr                  filenamefmt     = 0;
static strptr                  libtype         = 0;
static strptr                  libname         = 0;
static strptr                  defabi          = 0;
static strptr                  hunkname        = ".text";
static strptr                  datahunkname    = "__MERGED";
static strptr                  PPCRegPrefix    = "r";
static strptr                  IDstring        = 0;
static strptr                  prefix          = "";
static strptr                  subprefix       = "";
static strptr                  premacro        = "";
static uint8 *                 tempbuf         = 0;
static size_t                  headersize      = 0;
static uint32                  Flags           = 0;
static uint32                  Flags2          = 0;
/* Output error occured when 0 */
static uint32                  Output_Error    = 1;
/* are there some tagfuncs in FD */
static uint32                  tagfuncs        = 0;
/* priority for auto libopen */
static uint32                  priority        = 5;
/* needed for filename */
static string                  filename[255];

/* Only for E-Stuff, FD, SFD, XML creation */
static int32                   LastBias        = 0;
/* Only for PPC-LVO Lib's */
static uint8 *                 elfbufpos       = 0;
/* Only for PPC-LVO Lib's */
static uint32                  symoffset       = 0;
/* Only for FD, SFD creation */
static enum ABI                CurrentABI      = ABI_M68K;

/* Prototypes for the functions */
static strptr  DupString(strptr, size_t);
static strptr  AllocListMem(size_t);
static strptr  SkipBlanks(strptr);
static strptr  SkipBlanksRet(strptr);
static strptr  SkipName(strptr);
static uint32  GetTypes(void);
static strptr  GetBaseType(void);
static strptr  GetBaseTypeLib(void);
static strptr  GetLibraryName(void);
static strptr  GetIFXName(void);
static int32   MakeShortBaseName(void);
static uint32  OpenDest(strptr);
static uint32  CloseDest(strptr);
static uint32  MakeTagFunction(struct AmiPragma *);
static void    MakeLines(strptr, uint32);
static uint32  SpecialFuncs(void);
static void    SortFDList(void);
static void    AddAliasName(struct AmiPragma *, struct Pragma_AliasName *,
               uint32);
static uint32  CheckNames(struct AmiPragma *);
static uint32  ScanSFDFile(enum ABI);
static uint32  ScanFDFile(void);
static int32   ScanTypes(strptr, uint32);
static void    FindHeader(void);
static uint32  GetRegisterData(struct AmiPragma *);
static uint16  GetFRegisterData(struct AmiPragma *);
static uint32  OutputXDEF(uint32, strptr, ...);
static uint32  OutputXREF(uint32, uint32, strptr, ...);
static uint32  OutputXREF2(uint32, uint32, uint32, strptr, ...);
static uint32  OutputSYMBOL(uint32, strptr, ...);
static uint8 * AsmStackCopy(uint8 *, struct AmiPragma *, uint32, uint32);
/* ------------------------------------------------------------------ */
static void    DoError(uint32, uint32, ...);
static uint32  CheckError(struct AmiPragma *, uint32);
static uint32  DoOutputDirect(void *, size_t);

#if defined(__GNUC__)
static uint32  DoOutput(strptr, ...) __attribute__ ((format(printf, 1, 2)));
#else
static uint32  DoOutput(strptr, ...);
#endif
/* ------------------------------------------------------------------ */
static struct ShortList *NewItem(struct ShortListRoot *);
static struct ShortList *RemoveItem(struct ShortListRoot *, struct ShortList *);
static void              AddItem(struct ShortListRoot *, struct ShortList *);
/* ------------------------------------------------------------------ */
typedef uint32 (*FuncType)(struct AmiPragma *, uint32, strptr);

uint32 FuncAMICALL         (struct AmiPragma *, uint32, strptr);
uint32 FuncLIBCALL         (struct AmiPragma *, uint32, strptr);
uint32 FuncAsmText         (struct AmiPragma *, uint32, strptr);
uint32 FuncAsmCode         (struct AmiPragma *, uint32, strptr);
uint32 FuncCSTUBS          (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOXDEF         (struct AmiPragma *, uint32, strptr);
uint32 FuncLVO             (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOPPCXDEF      (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOPPC          (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOPPCBias      (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOPPCName      (struct AmiPragma *, uint32, strptr);
uint32 FuncLVOLib          (struct AmiPragma *, uint32, strptr);
uint32 FuncLocCode         (struct AmiPragma *, uint32, strptr);
uint32 FuncLocText         (struct AmiPragma *, uint32, strptr);
uint32 FuncInline          (struct AmiPragma *, uint32, strptr);
uint32 FuncInlineDirect    (struct AmiPragma *, uint32, strptr);
uint32 FuncInlineNS        (struct AmiPragma *, uint32, strptr);
uint32 FuncPowerUP         (struct AmiPragma *, uint32, strptr);
uint32 FuncFPCUnit         (struct AmiPragma *, uint32, strptr);
uint32 FuncFPCType         (struct AmiPragma *, uint32, strptr);
uint32 FuncFPCTypeTags     (struct AmiPragma *, uint32, strptr);
uint32 FuncFPCTypeTagsUnit (struct AmiPragma *, uint32, strptr);
uint32 FuncBMAP            (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCInline      (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCWOSInline   (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCMorphInline (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCWOSText     (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCWOSCode     (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCPUPText     (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCPUPCode     (struct AmiPragma *, uint32, strptr);
uint32 FuncEModule         (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCMorphText   (struct AmiPragma *, uint32, strptr);
uint32 FuncVBCCMorphCode   (struct AmiPragma *, uint32, strptr);
uint32 FuncFD              (struct AmiPragma *, uint32, strptr);
uint32 FuncClib            (struct AmiPragma *, uint32, strptr);
uint32 FuncSFD             (struct AmiPragma *, uint32, strptr);
uint32 FuncXML             (struct AmiPragma *, uint32, strptr);
uint32 FuncOS4PPC          (struct AmiPragma *, uint32, strptr);
uint32 FuncOS4M68KCSTUB    (struct AmiPragma *, uint32, strptr);
uint32 FuncOS4M68K         (struct AmiPragma *, uint32, strptr);
uint32 FuncOS4M68KVect     (struct AmiPragma *, uint32, strptr);
uint32 FuncGateStubs       (struct AmiPragma *, uint32, strptr);
static uint32 PrintComment (struct Comment *, strptr);
static uint32 DoCallFunc   (struct AmiPragma *, uint32, strptr, FuncType);
static uint32 CallFunc     (uint32, strptr, FuncType);
static uint32 PrintIncludes(void);
/* ------------------------------------------------------------------ */
static int32   AddClibEntry(strptr, strptr, uint32);
static int32   ScanClibFile(strptr, strptr);
static int32   IsCPPType(struct CPP_NameType *, uint8);
static uint32  CheckRegisterNum(strptr, struct CPP_NameType *);
static uint32  ParseFuncPtrArgs(strptr, struct CPP_NameType *);
static int32   GetCPPType(struct CPP_NameType *, strptr, uint32, uint32);
static struct ClibData *GetClibFunc(strptr, struct AmiPragma *, uint32);
static int32   CheckKeyword(strptr, strptr, int32);
static uint32  CopyCPPType(strptr, uint32, struct ClibData *, struct AmiArgs *);
static uint32  OutClibType(struct CPP_NameType *, strptr);
static uint32  MakeClibType(strptr, struct CPP_NameType *, strptr);
static uint32  OutPASCALType(struct CPP_NameType *, strptr, uint32);
/* ------------------------------------------------------------------ */
static uint32 CallPrag(uint32, strptr, FuncType);
static uint32 CreatePragmaFile(strptr, strptr, strptr, strptr, uint32);
static uint32 CreateCSTUBSFile(void);
static uint32 CreateLVOFile(uint32);
static uint32 CreateLVOFilePPC(uint32);
static uint32 CreateAsmStubs(uint32, uint32);
static uint32 CreateProtoFile(uint32);
static uint32 CreateLocalData(strptr, uint32);
static uint32 CreateInline(uint32, uint32);
static uint32 CreateGateStubs(uint32);
static uint32 CreateSASPowerUP(uint32);
static uint32 CreateProtoPowerUP(void);
static uint32 CreateFPCUnit(void);
static uint32 CreateBMAP(void);
static uint32 CreateLVOLib(void);
static uint32 CreateLVOLibPPC(void);
static uint32 CreateVBCCInline(uint32, uint32);
static uint32 CreateVBCC(uint32, uint32);
static uint32 CreateVBCCPUPLib(uint32);
static uint32 CreateVBCCMorphCode(uint32);
static uint32 CreateEModule(uint32);
static uint32 CreateProtoRedirect(void);
static uint32 CreateFD(void);
static uint32 CreateSFD(uint32);
static uint32 CreateClib(uint32);
static uint32 CreateGenAuto(strptr, uint32);
static uint32 CreateXML(void);
static uint32 CreateOS4PPC(uint32);
static uint32 CreateOS4M68K(void);
/* ------------------------------------------------------------------ */
static uint32 GetName(struct NameList *, struct ShortListRoot *, uint32);
static uint32 MakeFD(struct PragList *);
static void   OptimizeFDData(struct PragData *);
static string GetHexValue(string);
static string GetDoubleHexValue(strptr);
static uint32 AddFDData(struct ShortListRoot *, struct FDData *);
static uint32 GetLibData(struct FDData *);
static uint32 GetFlibData(struct FDData *);
static uint32 GetAmiData(struct FDData *);
static uint32 CreateFDFile(void);
/* ------------------------------------------------------------------ */
static void GetArgs(int argc, char **argv);
static strptr mygetfile(strptr name, size_t *len);

#define ERROFFSET_CLIB  (1<<31)

enum {
ERR_TAGFUNC_NEEDS_ARGUMENT,
ERR_CANNOT_CONVERT_PRAGMA_TAGCALL,
ERR_TAG_DEF_WITHOUT_PRAGMA,
ERR_BASENAME_DECLARED_TWICE,
ERR_EXPECTED_SLASH_IN_BASENAME,
ERR_EXPECTED_BASENAME,
ERR_EXPECTED_BIAS_VALUE,
ERR_ASSUMING_POSITIVE_BIAS_VALUE,
ERR_MISSING_FUNCTION_NAME,
ERR_EXPECTED_OPEN_BRACKET,
ERR_TO_MUCH_ARGUMENTS,
ERR_EXPECTED_ARGUMENT_NAME,
ERR_EXPECTED_CLOSE_BRACKET,
ERR_EXPECTED_REGISTER_NAME,
ERR_A7_NOT_ALLOWED,
ERR_REGISTER_USED_TWICE,
ERR_ARGUMENTNUMBER_DIFFERS_FROM_REGISTERNUMBER,
ERR_ASSUMING_BIAS_OF_30,
ERR_EXTRA_CHARACTERS,
ERR_MISSING_BASENAME,
ERR_WRITING_FILE,
ERR_EXPECTED_COMMA,
ERR_DIFFERENT_TO_PREVIOUS,
ERR_UNKNOWN_VARIABLE_TYPE,
ERR_UNKNOWN_ERROR,
ERR_MISSING_END,
ERR_PROTOTYPE_MISSING,
ERR_NOPROTOTYPES_FILE,
ERR_UNKNOWN_DIRECTIVE,
ERR_INLINE_A4_AND_A5,
ERR_INLINE_D7_AND_A45,
ERR_MISSING_SHORTBASENAME,
ERR_A6_NOT_ALLOWED,
ERR_EMPTY_FILE,
ERR_FLOATARG_NOT_ALLOWED,
ERR_WRONG_TYPES_LINE,
ERR_LONG_DOUBLE,
ERR_CLIB_ARG_COUNT,
ERR_OPEN_FILE,
ERR_A5_NOT_ALLOWED,
ERR_PPC_FUNCTION_NOT_SUPPORTED,
ERR_UNKNOWN_ABI,
ERR_NO_SORTED,
ERR_ILLEGAL_FUNCTION_POSITION,
ERR_SORTED_COMMENT,
ERR_COMMENT_SINGLEFILE,
ERR_NOFD2PRAGMATYPES,
ERR_M68K_FUNCTION_NOT_SUPPORTED,
ERR_UNKNOWN_RETURNVALUE_TYPE,
ERR_SFD_AND_CLIB,
ERR_EXCPECTED_IDSTRING,
ERR_EXPECTED_ID_ENDSIGN,
ERR_MISSING_SFDEND,
ERR_EXPECTED_POSITIVE_DECIMAL_NUMBER,
ERR_IDSTRING_DECLARED_TWICE,
ERR_COMMANDLINE_LIBTYPE,
ERR_COMMANDLINE_BASENAME,
ERR_LIBTYPE_DECLARED_TWICE,
ERR_EXPECTED_LIBTYPE,
ERR_SORTED_SFD_FD,
ERR_EARLY_SHADOW,
ERR_DOUBLE_VARARGS,
ERR_VARARGS_ARGUMENTS_DIFFER,
ERR_UNEXPECTED_FILEEND,
ERR_VARARGS_ALIAS_FIRST,
ERR_ALIASNAMES,
ERR_EXPECTED_STRUCT,
ERR_EXPECTED_POINTERSIGN,
ERR_ARGNAME_KEYWORD_CONFLICT,
ERR_ARGNAME_ARGNAME_CONFLICT,
ERR_ONLYTAGMODE_NOTALLOWED,
ERR_COMMANDLINE_LIBNAME,
ERR_LIBNAME_DECLARED_TWICE,
ERR_EXPECTED_LIBNAME,
ERR_PREFIX,
ERR_MULTIPLEFUNCTION,
ERR_INLINE_AX_SWAPREG,
ERR_SFD_START,
ERR_ILLEGAL_CHARACTER_DETECTED,
ERR_UNKNOWN_VARIABLE_TYPE_INT,
ERR_UNKNOWN_RETURNVALUE_TYPE_INT,
ERR_ILLEGAL_INTERNAL_VALUE,
ERR_MOSBASESYSV_NOT_SUPPORTED,
ERR_ALIASES_NOT_SUPPORTED,
ERR_FUNCTION_NOT_SUPPORTED,
};

static const struct ErrField {
  uint8  Type;    /* 0 = Error, 1 = Warning */
  uint8  Skip;
  strptr Error;
} Errors[] = {
{1, 1, "Tag function must have arguments."},
{1, 1, "Cannot convert pragma name into tag name."},
{1, 1, "Tag definition without preceding Pragma."},
{1, 0, "Basename declared twice."},
{1, 0, "Expected preceding _ in Basename."},
{1, 1, "Expected Basename."},
{1, 0, "Expected Bias value."},
{1, 0, "Assuming positive bias value."},
{1, 1, "Missing function name."},
{1, 1, "Expected '('."},
{1, 1, "Too much arguments."},
{1, 1, "Expected argument name."},
{1, 1, "Expected ')'."},
{1, 1, "Expected register name."},
{1, 1, "A7 not allowed as argument register."},
{1, 1, "Register used twice."},
{1, 0, "Number of arguments != number of registers."},
{1, 0, "Assuming bias of 30."},
{1, 1, "Extra characters."},
{0, 0, "Missing Basename in FD file."},
{0, 0, "Failed to write destination file."},
{1, 1, "Expected ','."},
{0, 1, "Data different to previous given."},
{1, 0, "Unknown type of argument %ld."},
{0, 0, "Unknown problem: program error or corrupt input data."},
{1, 0, "Missing ##end."},
{1, 0, "Prototype for function \"%s\" not found."},
{0, 0, "No prototypes file (CLIB parameter) was specified."},
{1, 1, "Unknown directive '%s' found."},
{1, 0, "Usage of both A4 and A5 is not supported."},
{1, 0, "Usage of both D7 and A4 or A5 is not supported."},
{0, 0, "Missing Basename in FD file and FD filename."},
{1, 0, "A6 not allowed as argument register."},
{1, 0, "Empty or partial file deleted."},
{1, 1, "Floating point arguments not allowed."},
{0, 0, "Wrong definition in external type definition file."},
{1, 0, "Cannot determine if FPU argument is double or single."},
{1, 0, "CLIB argument count differs for %s (%ld != %ld)."},
{0, 0, "Could not open file \"%s\"."},
{1, 0, "A5 cannot be used as argument register."},
{1, 0, "Format supports no PPC functions."},
{1, 0, "Unknown ABI '%s' found."},
{0, 0, "SORTED cannot be used with that type."},
{1, 0, "Position of function %s not supported with that type."},
{1, 1, "COMMENT and SORTED cannot be used both. Ignoring SORTED."},
{1, 0, "COMMENT cannot be used in single file mode, ignoring."},
{1, 0, "Missing the types definition file. Using internal defaults."},
{1, 0, "Format supports no M68k functions."},
{1, 0, "Unknown type of return value."},
{1, 0, "With SFD as input CLIB file is ignored."},
{1, 0, "Expected $Id: in ID string."},
{1, 0, "Expected $ at end of ID string."},
{1, 0, "Missing ==end."},
{1, 1, "Expected positive decimal number."},
{1, 0, "ID string declared twice."},
{1, 1, "Library type of commandline overwrites file settings."},
{1, 1, "Basename of commandline overwrites file settings."},
{1, 0, "Library type declared twice."},
{1, 1, "Expected library type definition."},
{1, 1, "SORTED cannot be used with SFD and FD output."},
{1, 0, "Function expected before ##shadow."},
{1, 1, "There is already a varargs function, handling as alias."},
{1, 0, "Varargs function cannot have different arguments."},
{1, 0, "Unexpected end of file."},
{1, 0, "Commands varargs and alias cannot be at file start."},
{1, 0, "Only %d alias names supported."},
{1, 1, "Expected struct keyword in library type."},
{1, 0, "Expected '*' at end of library type definition."},
{1, 1, "Name of argument %d conflicts with keyword '%s'."},
{1, 1, "Name of argument %d conflicts with argument %d."},
{1, 0, "SFD files cannot consist only of varargs functions."},
{1, 1, "Library name of commandline overwrites file settings."},
{1, 0, "Library name declared twice."},
{1, 1, "Expected library name definition."},
{1, 0, "Neither prefix nor subprefix specified."},
{1, 0, "Format supports single function pointer only (void * used)."},
{1, 0, "No swap register left for %s."},
{1, 0, "SFD files should always start with ==id directive."},
{1, 0, "Illegal character detected."},
{1, 0, "Unknown type of argument %ld (%s) handled as int."},
{1, 0, "Unknown type of return value (%s) handled as int."},
{1, 0, "Illegal internal value."},
{1, 0, "Format supports no MorphOS (base,sysv) functions."},
{1, 0, "Format supports no alias names."},
{1, 0, "Format cannot support function %s."},
};

#ifdef __SASC
__far
#endif
static uint8 InternalTypes[] = {
"IX:struct InputXpression\n"
"Msg:struct ? *\n"
"Class:struct IClass\n"
"BootBlock:struct bootblock\n"
"ValidIDstruct:struct ValidIDstruct\n"
"DisplayInfoHandle:void *\n"
"RESOURCEFILE:void *\n"
"RESOURCEID:unsigned long\n"
"GLvoid:void\n"
"GLbitfield:unsigned long\n"
"GLbyte:signed char\n"
"GLshort:short\n"
"GLint:long\n"
"GLsizei:unsigned long\n"
"GLubyte:unsigned char\n"
"GLushort:unsigned short\n"
"GLuint:unsigned long\n"
"GLfloat:float\n"
"GLclampf:float\n"
"GLdouble:double\n"
"GLclampd:double\n"
"GLboolean:enum ?\n"
"GLenum:enum ?\n"
"GLlookAt:struct GLlookAt\n"
"GLproject:struct GLproject\n"
"GLunProject:struct GLunProject\n"
"GLfrustum:struct GLfrustum\n"
"GLortho:struct GLortho\n"
"GLbitmap:struct GLbitmap\n"
"GLUquadricObj:struct GLUquadricObj\n"
"GLUtriangulatorObj:struct GLUtriangulatorObj\n"
"GLUnurbsObj:struct GLUnurbsObj\n"
"GLvisual:struct gl_visual\n"
"GLframebuffer:struct gl_frame_buffer\n"
"GLcontext:struct gl_context\n"
"GLContext:struct !\n"
"HGIDA_Stack:unsigned long *\n"
"HGIDA_BoundedStack:unsigned long *\n"
"HGIDA_Queue:unsigned long *\n"
"HGIDA_BoundedQueue:unsigned long *\n"
"HGIDA_List:unsigned long *\n"
"HGIDA_ListItem:unsigned long *\n"
"HGIDA_Error:enum ?\n"
"HGIDA_Direction:enum ?\n"
"uid_t:long\n"
"gid_t:long\n"
"mode_t:unsigned short\n"
"pid_t:struct Task *\n"
"fd_set:struct fd_set\n"
"SerScriptCallback_t:unsigned long (*)(register __a0 void *, register __d0 "
 "unsigned long, register __a1 const unsigned char *, register __a2 struct "
 "CSource *, register __a3 struct CSource *)\n"
"pcap_t:struct pcap\n"
"pcap_dumper_t:struct pcap_dumper\n"
"pcap_handler:void (*)(unsigned char *, const struct pcap_pkthdr *, const "
 "unsigned char *)\n"
"u_char:unsigned char\n"
"bpf_u_int32:unsigned long\n"
"Fixed:long\n"
"sposition:long\n"
"MPEGA_STREAM:struct MPEGA_STREAM\n"
"MPEGA_CTRL:struct MPEGA_CTRL\n"
"W3D_Context:struct W3DContext\n"
"W3D_Driver:struct W3DDriver\n"
"W3D_Texture:struct W3DTexture\n"
"W3D_Scissor:struct W3DScissor\n"
"W3D_Line:struct W3D_Line\n"
"W3D_Point:struct W3D_Point\n"
"W3D_Triangle:struct W3D_Triangle\n"
"W3D_Triangles:struct W3D_Triangles\n"
"W3D_Float:float\n"
"W3D_Bitmap:struct W3D_Bitmap\n"
"W3D_Fog:struct W3D_Fog\n"
"W3D_Bool:short\n"
"W3D_Double:double\n"
"W3D_TriangleV:struct W3D_TriangleV\n"
"W3D_TrianglesV:struct W3D_TriangleV\n"
"W3D_ScreenMode:struct W3D_Screenmode\n"
"W3D_Color:struct W3D_Color\n"
"W3D_Lines:struct W3D_Lines\n"
"RGBFTYPE:enum ?\n"
"DITHERINFO:void *\n"
"SLayer:void *\n"
"va_list:char *\n"
"time_t:long\n"
"size_t:unsigned int\n"
"FILE:struct ? *\n"
"uint8:unsigned char\n"
"uint16:unsigned short\n"
"uint32:unsigned long\n"
"int8:char\n"
"int16:short\n"
"int32:long\n"
"AVLKey:void *\n"
"PtrBigNum:struct BigNum *\n"
"BF_KEY:struct bf_key_st\n"
"BF_LONG:unsigned long\n"
"CAST_KEY:struct cast_key_st\n"
"CAST_LONG:unsigned long\n"
"DES_LONG:unsigned long\n"
"des_key_schedule:struct des_ks_struct\n"
"const_des_cblock:unsigned char [8]\n"
"des_cblock:unsigned char [8]\n"
"IDEA_KEY_SCHEDULE:struct idea_key_st\n"
"MD2_CTX:struct MD2state_st\n"
"MD5_CTX:struct MD5state_st\n"
"MDC2_CTX:struct mdc2_ctx_st\n"
"RC2_KEY:struct rc2_key_st\n"
"RC4_KEY:struct rc4_key_st\n"
"RC5_32_KEY:struct rc5_key_st\n"
"RIPEMD160_CTX:struct RIPEMD160state_st\n"
"SHA_CTX:struct SHAstate_st\n"
"ASN1_CTX:struct asn1_ctx_st\n"
"ASN1_OBJECT:struct asn1_object_st\n"
"ASN1_STRING:struct asn1_string_st\n"
"ASN1_TYPE:struct asn1_type_st\n"
"ASN1_METHOD:struct asn1_method_st\n"
"ASN1_HEADER:struct asn1_header_st\n"
"ASN1_INTEGER:struct asn1_string_st\n"
"ASN1_ENUMERATED:struct asn1_string_st\n"
"ASN1_BIT_STRING:struct asn1_string_st\n"
"ASN1_OCTET_STRING:struct asn1_string_st\n"
"ASN1_PRINTABLESTRING:struct asn1_string_st\n"
"ASN1_T61STRING:struct asn1_string_st\n"
"ASN1_IA5STRING:struct asn1_string_st\n"
"ASN1_UTCTIME:struct asn1_string_st\n"
"ASN1_GENERALIZEDTIME:struct asn1_string_st\n"
"ASN1_TIME:struct asn1_string_st\n"
"ASN1_GENERALSTRING:struct asn1_string_st\n"
"ASN1_UNIVERSALSTRING:struct asn1_string_st\n"
"ASN1_BMPSTRING:struct asn1_string_st\n"
"ASN1_VISIBLESTRING:struct asn1_string_st\n"
"ASN1_UTF8STRING:struct asn1_string_st\n"
"BIO:struct bio_st\n"
"BIO_F_BUFFER_CTX:struct bio_f_buffer_ctx_struct\n"
"BIO_METHOD:struct bio_method_st\n"
"BIGNUM:struct bignum_st\n"
"BN_CTX:struct bignum_ctx\n"
"BN_ULONG:unsigned long\n"
"BN_MONT_CTX:struct bn_mont_ctx_st\n"
"BN_BLINDING:struct bn_blinding_st\n"
"BN_RECP_CTX:struct bn_recp_ctx_st\n"
"BUF_MEM:struct buf_mem_st\n"
"COMP_METHOD:struct comp_method_st\n"
"COMP_CTX:struct comp_ctx_st\n"
"CONF_VALUE:struct !\n"
"LHASH_NODE:struct lhash_node_st\n"
"LHASH:struct lhash_st\n"
"CRYPTO_EX_DATA:struct crypto_ex_data_st\n"
"CRYPTO_EX_DATA_FUNCS:struct crypto_ex_data_func_st\n"
"DH:struct dh_st\n"
"DSA:struct dsa_st\n"
"DSA_SIG:struct DSA_SIG_st\n"
"ERR_STATE:struct err_state_st\n"
"ERR_STRING_DATA:struct ERR_string_data_st\n"
"EVP_PKEY:struct evp_pkey_st\n"
"EVP_MD:struct env_md_st\n"
"EVP_MD_CTX:struct env_md_ctx_st\n"
"EVP_CIPHER:struct evp_cipher_st\n"
"EVP_CIPHER_INFO:struct evp_cipher_info_st\n"
"EVP_CIPHER_CTX:struct evp_cipher_ctx_st\n"
"EVP_ENCODE_CTX:struct evp_Encode_Ctx_st\n"
"EVP_PBE_KEYGEN:struct int (*)(struct evp_cipher_ctx_st *ctx, const char "
 "*pass, int passlen, struct asn1_type_st *param, struct evp_cipher_st "
 "*cipher, struct env_md_st *md, int en_de)\n"
"HMAC_CTX:struct hmac_ctx_st\n"
"OBJ_NAME:struct obj_name_st\n"
"PEM_ENCODE_SEAL_CTX:struct PEM_Encode_Seal_st\n"
"PEM_USER:struct pem_recip_st\n"
"PEM_CTX:struct pem_ctx_st\n"
"PKCS12_MAC_DATA:struct !\n"
"PKCS12:struct !\n"
"PKCS12_SAFEBAG:struct !\n"
"PKCS12_BAGS:struct pkcs12_bag_st\n"
"PKCS7_ISSUER_AND_SERIAL:struct pkcs7_issuer_and_serial_st\n"
"PKCS7_SIGNER_INFO:struct pkcs7_signer_info_st\n"
"PKCS7_RECIP_INFO:struct pkcs7_recip_info_st\n"
"PKCS7_SIGNED:struct pkcs7_signed_st\n"
"PKCS7_ENC_CONTENT:struct pkcs7_enc_content_st\n"
"PKCS7_ENVELOPE:struct pkcs7_enveloped_st\n"
"PKCS7_SIGN_ENVELOPE:struct pkcs7_signedandenveloped_st\n"
"PKCS7_DIGEST:struct pkcs7_digest_st\n"
"PKCS7_ENCRYPT:struct pkcs7_encrypted_st\n"
"PKCS7:struct pkcs7_st\n"
"RAND_METHOD:struct rand_meth_st\n"
"RSA:struct rsa_st\n"
"RSA_METHOD:struct rsa_meth_st\n"
"TXT_DB:struct txt_db_st\n"
"X509_OBJECTS:struct X509_objects_st\n"
"X509_ALGOR:struct X509_algor_st\n"
"X509_VAL:struct X509_val_st\n"
"X509_PUBKEY:struct X509_pubkey_st\n"
"X509_SIG:struct X509_sig_st\n"
"X509_NAME_ENTRY:struct X509_name_entry_st\n"
"X509_NAME:struct X509_name_st\n"
"X509_EXTENSION:struct X509_extension_st\n"
"X509_ATTRIBUTE:struct x509_attributes_st\n"
"X509_REQ_INFO:struct X509_req_info_st\n"
"X509_REQ:struct X509_req_st\n"
"X509_CINF:struct x509_cinf_st\n"
"X509:struct x509_st\n"
"X509_REVOKED:struct X509_revoked_st\n"
"X509_CRL_INFO:struct X509_crl_info_st\n"
"X509_CRL:struct X509_crl_st\n"
"X509_PKEY:struct private_key_st\n"
"X509_INFO:struct X509_info_st\n"
"NETSCAPE_SPKAC:struct Netscape_spkac_st\n"
"NETSCAPE_SPKI:struct Netscape_spki_st\n"
"NETSCAPE_CERT_SEQUENCE:struct Netscape_certificate_sequence\n"
"CBC_PARAM:struct CBCParameter_st\n"
"PBEPARAM:struct PBEPARAM_st\n"
"PBE2PARAM:struct PBE2PARAM_st\n"
"PBKDF2PARAM:struct PBKDF2PARAM_st\n"
"PKCS8_PRIV_KEY_INFO:struct pkcs8_priv_key_info_st\n"
"X509V3_CONF_METHOD:struct X509V3_CONF_METHOD_st\n"
"X509V3_EXT_METHOD:struct v3_ext_method\n"
"X509V3_CTX:struct v3_ext_ctx\n"
"X509_HASH_DIR_CTX:struct x509_hash_dir_st\n"
"X509_CERT_FILE_CTX:struct x509_file_st\n"
"X509_OBJECT:struct X509_objects_st\n"
"X509_LOOKUP:struct x509_lookup_st\n"
"X509_LOOKUP_METHOD:struct x509_lookup_method_st\n"
"X509_STORE_CTX:struct x509_store_state_st\n"
"X509_STORE:struct x509_store_st\n"
"BIT_STRING_BITNAME:struct BIT_STRING_BITNAME_st\n"
"BASIC_CONSTRAINTS:struct BASIC_CONSTRAINTS_st\n"
"PKEY_USAGE_PERIOD:struct PKEY_USAGE_PERIOD_st\n"
"GENERAL_NAME:struct GENERAL_NAME_st\n"
"DIST_POINT_NAME:struct DIST_POINT_NAME_st\n"
"DIST_POINT:struct DIST_POINT_st\n"
"AUTHORITY_KEYID:struct AUTHORITY_KEYID_st\n"
"SXNETID:struct SXNET_ID_st\n"
"SXNET:struct SXNET_st\n"
"NOTICEREF:struct NOTICEREF_st\n"
"USERNOTICE:struct USERNOTICE_st\n"
"POLICYQUALINFO:struct POLICYQUALINFO_st\n"
"POLICYINFO:struct POLICYINFO_st\n"
"pem_password_cb:int (*)(char *buf, int size, int rwflag, void *userdata)\n"
"SSL_CIPHER:struct ssl_cipher_st\n"
"SSL:struct ssl_st\n"
"SSL_CTX:struct ssl_ctx_st\n"
"SSL_METHOD:struct ssl_method_st\n"
"SSL_SESSION:struct ssl_session_st\n"
"SSL_COMP:struct ssl_comp_st\n"
"SSL2_CTX:struct ssl2_ctx_st\n"
"SSL3_RECORD:struct ssl3_record_st\n"
"SSL3_BUFFER:struct ssl3_buffer_st\n"
"SSL3_CTX:struct ssl3_ctx_st\n"
"CERT_PKEY:struct cert_pkey_st\n"
"CERT:struct cert_st\n"
"SESS_CERT:struct sess_cert_st\n"
"SSL3_ENC_METHOD:struct ssl3_enc_method\n"
"SSL3_COMP:struct ssl3_comp_st\n"
"STACK_OF(X509_ATTRIBUTE):struct stack_st_X509_ATTRIBUTE\n"
"STACK_OF(X509_INFO):struct stack_st_X509_INFO\n"
"STACK_OF(X509_NAME):struct stack_st_X509_NAME\n"
"STACK_OF(X509):struct stack_st_X509\n"
"STACK_OF(PKCS7_SIGNER_INFO):struct stack_st_PKCS7_SIGNER_INFO\n"
"STACK_OF(SSL_CIPHER):struct stack_st_SSL_CIPHER\n"
"STACK_OF(GENERAL_NAME):struct stack_st_GENERAL_NAME\n"
"STACK_OF(CONF_VALUE):struct stack_st_CONF_VALUE\n"
"STACK_OF(ASN1_OBJECT):struct stack_st_ASN1_OBJECT\n"
"STACK_OF(POLICYINFO):struct stack_st_POLICYINFO\n"
"STACK_OF(DIST_POINT):struct stack_st_DIST_POINT\n"
"STACK_OF(X509_EXTENSION):struct stack_st_X509_EXTENSION\n"
"STACK:struct stack_st\n"
"SDL_bool:enum ?\n"
"Uint8:unsigned char\n"
"Sint8:signed char\n"
"Uint16:unsigned short\n"
"Sint16:signed short\n"
"Uint32:unsigned long\n"
"Sint32:signed long\n"
"Uint64:unsigned long long\n"
"Sint64:signed long long\n"
"SDL_version:struct !\n"
"SDL_RWops:struct SDL_RWops\n"
"SDL_Rect:struct !\n"
"SDL_Color:struct !\n"
"SDL_Palette:struct !\n"
"SDL_PixelFormat:struct SDL_PixelFormat\n"
"SDL_blit:int (*)(struct SDL_Surface *src,void *srcrect,"
"struct SDL_Surface *dst,void *dstrect)\n"
"SDL_Surface:struct SDL_Surface\n"
"SDL_VideoInfo:struct !\n"
"SDL_Overlay:struct SDL_Overlay\n"
"SDL_GLattr:enum ?\n"
"SDL_GrabMode:enum ?\n"
"SDL_audiostatus:enum ?\n"
"SDL_AudioSpec:struct !\n"
"SDL_AudioCVT:struct SDL_AudioCVT\n"
"CDstatus:enum ?\n"
"SDL_CDtrack:struct !\n"
"SDL_CD:struct SDL_CD\n"
"SDL_Joystick:struct _SDL_Joystick\n"
"SDLKey:enum ?\n"
"SDLMod:enum ?\n"
"SDL_keysym:struct !\n"
"SDL_ActiveEvent:struct !\n"
"SDL_KeyboardEvent:struct !\n"
"SDL_MouseMotionEvent:struct !\n"
"SDL_MouseButtonEvent:struct !\n"
"SDL_JoyAxisEvent:struct !\n"
"SDL_JoyBallEvent:struct !\n"
"SDL_JoyHatEvent:struct !\n"
"SDL_JoyButtonEvent:struct !\n"
"SDL_ResizeEvent:struct !\n"
"SDL_ExposeEvent:struct !\n"
"SDL_QuitEvent:struct !\n"
"SDL_UserEvent:struct !\n"
"SDL_SysWMmsg:struct SDL_SysWMmsg\n"
"SDL_SysWMEvent:struct !\n"
"SDL_Event:union ?\n"
"SDL_eventaction:enum ?\n"
"SDL_EventFilter:void *\n"
"WMcursor:struct WMcursor\n"
"SDL_Cursor:struct !\n"
"SDL_mutex:struct SDL_mutex\n"
"SDL_sem:struct SDL_semaphore\n"
"SDL_cond:struct SDL_cond\n"
"SDL_Thread:struct SDL_Thread\n"
"SDL_TimerCallback:unsigned long (*)(unsigned long)\n"
"SDL_NewTimerCallback:unsigned long (*)(unsigned long,void *)\n"
"SDL_TimerID:struct _SDL_TimerID\n"
"SDL_errorcode:enum ?\n"
"FPSmanager:struct !\n"
"Mix_Chunk:struct !\n"
"Mix_Music:struct !\n"
"Mix_MusicType:enum ?\n"
"Mix_EffectFunc_t:void (*)(int,void *,int,void *)\n"
"Mix_EffectDone_t:void (*)(int,void *)\n"
"Mix_Fading:enum ?\n"
"IPaddress:struct !\n"
"TCPsocket:void *\n"
"UDPpacket:struct !\n"
"UDPsocket:void *\n"
"SDLNet_SocketSet:void *\n"
"SDLNet_GenericSocket:void *\n"
"TTF_Font:struct !\n"
};

static const struct CPP_TypeField CPP_Field[] = {
{"int",         3, 0,                                  CPP_TYPE_INT},
/* long needs special handling due to the fact it is a modifier and a type */
/*{"long",        4, 0,                                  CPP_TYPE_LONG}, */
{"LONG",        4, 0,                                  CPP_TYPE_LONG},
{"BPTR",        4, 0,                                  CPP_TYPE_LONG},
{"BSTR",        4, 0,                                  CPP_TYPE_LONG},
{"CxObj",       5, 0,                                  CPP_TYPE_LONG},
{"CxMsg",       5, 0,                                  CPP_TYPE_LONG},
{"ULONG",       5, CPP_FLAG_UNSIGNED,                  CPP_TYPE_LONG},
{"LONGBITS",    8, CPP_FLAG_UNSIGNED,                  CPP_TYPE_LONG},
{"CPTR",        4, CPP_FLAG_UNSIGNED,                  CPP_TYPE_LONG},
{"Tag",         3, CPP_FLAG_UNSIGNED,                  CPP_TYPE_LONG},
{"Object",      6, CPP_FLAG_UNSIGNED,                  CPP_TYPE_LONG},
{"short",       5, 0,                                  CPP_TYPE_WORD},
{"SHORT",       5, 0,                                  CPP_TYPE_WORD},
{"COUNT",       5, 0,                                  CPP_TYPE_WORD},
{"WORD",        4, 0,                                  CPP_TYPE_WORD},
{"USHORT",      6, CPP_FLAG_UNSIGNED,                  CPP_TYPE_WORD},
{"UWORD",       5, CPP_FLAG_UNSIGNED,                  CPP_TYPE_WORD},
{"UCOUNT",      6, CPP_FLAG_UNSIGNED,                  CPP_TYPE_WORD},
{"WORDBITS",    8, CPP_FLAG_UNSIGNED,                  CPP_TYPE_WORD},
{"RPTR",        4, CPP_FLAG_UNSIGNED,                  CPP_TYPE_WORD},
{"BOOL",        4, CPP_FLAG_BOOLEAN,                   CPP_TYPE_WORD},
{"char",        4, 0,                                  CPP_TYPE_BYTE},
{"BYTE",        4, 0,                                  CPP_TYPE_BYTE},
{"UBYTE",       5, CPP_FLAG_UNSIGNED,                  CPP_TYPE_BYTE},
{"TEXT",        4, CPP_FLAG_UNSIGNED,                  CPP_TYPE_BYTE},
{"BYTEBITS",    8, CPP_FLAG_UNSIGNED,                  CPP_TYPE_BYTE},
{"float",       5, 0,                                  CPP_TYPE_FLOAT},
{"FLOAT",       5, 0,                                  CPP_TYPE_FLOAT},
{"double",      6, 0,                                  CPP_TYPE_DOUBLE},
{"DOUBLE",      6, 0,                                  CPP_TYPE_DOUBLE},
{"void",        4, 0,                                  CPP_TYPE_VOID},
{"VOID",        4, 0,                                  CPP_TYPE_VOID},
{"APTR",        4, CPP_FLAG_POINTER,                   CPP_TYPE_VOID},
{"STRPTR",      6, CPP_FLAG_POINTER|CPP_FLAG_STRPTR,   CPP_TYPE_BYTE},
{"CONST_STRPTR",12,CPP_FLAG_POINTER|CPP_FLAG_CONST,    CPP_TYPE_BYTE},
{"ClassID",     7, CPP_FLAG_POINTER|CPP_FLAG_UNSIGNED, CPP_TYPE_BYTE},
{"PLANEPTR",    8, CPP_FLAG_POINTER|CPP_FLAG_UNSIGNED, CPP_TYPE_BYTE},
{0,0,0,0},
};

/* defaults:              "Library"         shortbname+".library"  basename-last 4 chars */
static const struct Proto_LibType Proto_LibTypes[] = {
{"DOSBase",               "DosLibrary",     0,                     0},
{"SysBase",               "ExecBase",       0,                     0},
{"ExpansionBase",         "ExpansionBase",  0,                     0},
{"GfxBase",               "GfxBase",        "graphics.library",    "graphics"},
{"IntuitionBase",         "IntuitionBase",  0,                     0},
{"LocaleBase",            "LocaleBase",     0,                     0},
{"MathIeeeDoubBasBase",   "MathIEEEBase",   0,                     0},
{"MathIeeeDoubTransBase", "MathIEEEBase",   0,                     0},
{"MathIeeeSingBasBase",   "MathIEEEBase",   0,                     0},
{"MathIeeeSingTransBase", "MathIEEEBase",   0,                     0},
{"RealTimeBase",          "RealTimeBase",   0,                     0},
{"RexxSysBase",           "RxsLib",         0,                     0},
{"UtilityBase",           "UtilityBase",    0,                     0},
{"WorkbenchBase",         0,                "workbench.library",   "wb"},
/* resources - The Node entries may be correct, but I don't know it. */
{"BattClockBase",         0/*"Node"*/,      "battclock.resource",  0},
{"BattMemBase",           0/*"Node"*/,      "battmem.resource",    0},
{"CardResource",          0/*"Node"*/,      "card.resource",       "cardres"},
{"DiskBase",              "DiskResource",   "disk.resource",       0},
{"MiscBase",              0/*"Node"*/,      "misc.resource",       0},
{"PotgoBase",             0/*"Node"*/,      "potgo.resource",      0},
/* devices */
{"ConsoleDevice",         0/*"Device"*/,    "console.device",      "console"},
{"InputBase",             0/*"Device"*/,    "input.device",        0},
{"RamdriveDevice",        0/*"Device"*/,    "ramdrive.device",     "ramdrive"},
{"TimerBase",             0/*"Device"*/,    "timer.device",        0},
/* non default Basenames */
{"DatamasterBase",        "DatamasterBase", 0,                     0},
{"PPBase",                "PPBase",         "powerpacker.library", "powerpacker"},
{"ReqToolsBase",          "ReqToolsBase",   0,                     0},
{"UnpackBase",            "UnpackLibrary",  0,                     0},
{"xfdMasterBase",         "xfdMasterBase",  0,                     0},
{"xadMasterBase",         "xadMasterBase",  0,                     0},
/*{"xvsBase",             "xvsBase",        0,                     0}, now completely private */
{"GTXBase",               "GTXBase",        "gadtoolsbox.library", "gtx"},
{"ArpBase",               "ArpBase",        0,                     0},
{"PopupMenuBase",         "PopupMenuBase",  0,                     "pm"},
{"PowerPCBase",           "PPCBase",        0,                     0},
{"MC68060Base",           0,                "68060.library",       "mc68060"},
{"MC68040Base",           0,                "68040.library",       "mc68040"},
{"MC680x0Base",           0,                "680x0.library",       "mc680x0"},
{"P96Base",               0,                "Picasso96API.library","Picasso96"},
{"Warp3DPPCBase",         0,                "Warp3DPPC.library"   ,"Warp3D"},
{"CyberGfxBase",          0,                "cybergraphics.library", "cybergraphics"},
{0, 0, 0, 0},
};

/* CachePostDMA, CachePreDMA are done by #?DMA check */
static const struct Pragma_ExecpName Pragma_ExecpNames[] = {
{"VFWritef",                    "FWritef"},
{"VFPrintf",                    "FPrintf"},
{"VPrintf",                     "Printf"},
{"ReadArgs",                    0},
{"FreeArgs",                    0},
{"CloneTagItems",               0},
{"FindTagItem",                 0},
{"FreeTagItems",                0},
{"GetTagData",                  0},
{"PackBoolTags",                0},
{"PackStructureTags",           0},
{"UnpackStructureTags",         0},
{"BGUI_PackStructureTags",      0},
{"BGUI_UnpackStructureTags",    0},
{"Inet_NtoA",                   0}, /* socket.library */
{"vsyslog",                     "syslog"},
{"NewPPCStackSwap",             0},
{"FindTagItemPPC",              0},
{"GetTagDataPPC",               0},
{"GetInfo",                     0},
{"GetHALInfo",                  0},
{"SetScheduling",               0},
{"W3D_CreateContext",           "W3D_CreateContextTags"},
{"W3D_RequestMode",             "W3D_RequestModeTags"},
{"W3D_AllocTexObj",             "W3D_AllocTexObjTags"},
{"W3D_BestModeID",              "W3D_BestModeIDTags"},
{0,0},
};

/* This field contains functions, which should not be created as inlines with
some targets. At the moment these are varargs functions, which are used in
the MUI style using complicated defines. Theses functions are disabled in
certain GCC and VBCC environments. */
static const strptr NoCreateInlineFuncs[] = {
"NewObject",
"MUI_NewObject",
"PM_MakeItem",
0,
};

/* For double tagcall names (currently only necessary for dos.library and
datatypes.library). Only one alias supported for a function! */
static const struct Pragma_AliasName Pragma_AliasNames[] = {
{"AllocDosObject",      "AllocDosObjectTagList",        FUNCFLAG_NORMAL},
{"CreateNewProc",       "CreateNewProcTagList",         FUNCFLAG_NORMAL},
{"NewLoadSeg",          "NewLoadSegTagList",            FUNCFLAG_NORMAL},
{"SystemTagList",       "System",                       FUNCFLAG_NORMAL},
{"RefreshDTObject",     "RefreshDTObjects",             FUNCFLAG_TAG},
{0,0,0},
};

/* special names, which get an x before name in BMAP files */
static const strptr BMAPSpecial[] =
{"abs", "Close", "Exit", "Input", "Open", "Output", "Read", "tan",
"Translate", "Wait", "Write", 0};

#define  FIRST_KNOWN_RELEASE    30
#define  LAST_KNOWN_RELEASE     45
static const strptr Release[] =
{
  "Release 1.0",                /* V30 */
  "Release 1.1",                /* V31 */
  "Preliminary Release 1.2",    /* V32 */
  "Release 1.2",                /* V33 */
  "Release 1.3",                /* V34 */
  "Release 1.3 A2024",          /* V35 */
  "Release 2.0",                /* V36 */
  "Release 2.04",               /* V37 */
  "Release 2.1",                /* V38 */
  "Release 3.0",                /* V39 */
  "Release 3.1",                /* V40 */
  "Transitional Release 3.2",   /* V41 */
  "Transitional Release 3.3",   /* V42 */
  "Transitional Release 3.4",   /* V43 */
  "Release 3.5",                /* V44 */
  "Release 3.9",                /* V45 */
};

/* Keywords, which cannot be argument names. They are used case_insensitive to
   be sure they make no conflicts in non-C-languages as well.
   Currently these are mostly C keywords.
*/
static const strptr Keywords[] =
{
  "and",        "and_eq",       "asm",          "auto",         "bitand",
  "bitor",      "break",        "case",         "catch",        "char",
  "class",      "compl",        "const",        "continue",     "default",
  "delete",     "do",           "double",       "else",         "enum",
  "extern",     "false",        "float",        "for",          "friend",
  "goto",       "if",           "inline",       "int",          "long",
  "new",        "not",          "not_eq",       "operator",     "or",
  "or_eq",      "private",      "protected",    "public",       "register",
  "return",     "short",        "signed",       "sizeof",       "static",
  "struct",     "switch",       "template",     "this",         "throw",
  "true",       "try",          "typedef",      "union",        "unsigned",
  "virtual",    "void",         "volatile",     "wchar_t",      "while",
  "xor",        "xor_eq",	"RastPort",	"Tag",
  0
};

#if !defined __SASC && !defined __AROS__ && !defined _WIN32 && !defined __CYGWIN__
static int stricmp(const char *a, const char *b)
{
  while(*a && tolower(*a) == tolower(*b))
  {
    ++a; ++b;
  }
  return (tolower(*a) - tolower(*b));
}

static int strnicmp(const char *a, const char *b, size_t num)
{
  while(num && *a && tolower(*a) == tolower(*b))
  {
    ++a; ++b; --num;
  }
  return num ? (tolower(*a) - tolower(*b)) : 0;
}
#endif

static strptr DupString(strptr Str, size_t Len)
{
  strptr res, r;
  if((res = r = AllocListMem(Len+1)))
  {
    while(Len-- && *Str)
      *(r++) = *(Str++);
    *r = '\0';
  }
#ifdef DEBUG_OLD
  printf("DupString %s.\n", res);
#endif
  return res;
}

static strptr AllocListMem(size_t size)
{
  strptr a;
#ifdef DEBUG_OLD
  printf("AllocListMem Size %d.\n", size);
#endif
  if((a = (strptr) malloc(size)))
    memset(a, 0, size);
  return a;
}

static strptr SkipBlanks(strptr OldPtr)
{
  while(*OldPtr == ' ' || *OldPtr == '\t')
    ++OldPtr;
  return OldPtr;
}

static strptr SkipBlanksRet(strptr OldPtr)
{
  while(*OldPtr == ' ' || *OldPtr == '\t' || *OldPtr == '\n')
    ++OldPtr;
  return OldPtr;
}

/*
  This function is used to skip over variable names.

  Inputs: OldPtr  - pointer to the beginning of a string.

  Result: Pointer to the first character of the string, that is not one
    of a-z, A-Z, 0-9 or the underscore.
*/

static strptr SkipName(strptr OldPtr)
{
  while(isalnum(*OldPtr) || *OldPtr == '_')
    ++OldPtr;
  return OldPtr;
}

static int IsNoCreateInlineFunc(const strptr name)
{
  const strptr *a;
  for(a = NoCreateInlineFuncs; *a; ++a)
  {
    if(!strcmp(name, *a))
      return 1;
  }
  return 0;
}

static uint32 GetTypes(void)
{
  strptr ptr;
  size_t len;
  uint32 i;

  if(!(ptr = mygetfile(EXTTYPESFILE, &len)))
  {
#ifdef EXTTYPESFILEHIDDEN
    if((ptr = getenv("HOME")))
    {
      strptr ptrh = EXTTYPESFILEHIDDEN;

      i = strlen(ptr);
      ptr = DupString(ptr, i + sizeof(EXTTYPESFILEHIDDEN) + 1);
      if(i && ptr[i-1] != '/')
        ptr[i++] = '/';
      while(*ptrh)
        ptr[i++] = *(ptrh++);
      ptr[i] = 0;
      ptr = mygetfile(ptr, &len);
    }
    if(!ptr) /* disabled following if ptr != 0 */
#endif
    if(!(ptr = mygetfile(EXTTYPESFILE2, &len)))
    {
      DoError(ERR_NOFD2PRAGMATYPES, 0);
      ptr = (strptr) InternalTypes;
      len = sizeof(InternalTypes)-1;
    }
  }
  if((i = ScanTypes(ptr, len)) > 0)
  {
    DoError(ERR_WRONG_TYPES_LINE, i);
    return 0;
  }
  return 1;
}

static strptr GetBaseType(void)
{
  static strptr basetype = 0;
  uint32 i;

  if(Flags2 & FLAG2_VOIDBASE)
    basetype = "void *";
  else if(!basetype)
  {
    for(i = 0; !libtype && BaseName && Proto_LibTypes[i].BaseName; ++i)
    {
      if(!(strcmp(Proto_LibTypes[i].BaseName, BaseName)))
      {
        libtype = Proto_LibTypes[i].StructureName;
        break;
      }
    }
    if(libtype && (basetype = malloc(strlen(libtype) + 9+1)))
    {
      sprintf(basetype, "struct %s *", libtype);
    }
    if(!libtype)
      basetype = "struct Library *";
  }

  return basetype;
}

static strptr GetBaseTypeLib(void)
{
  uint32 i;

  if(libtype)
    return libtype;

  for(i = 0; BaseName && Proto_LibTypes[i].BaseName; ++i)
  {
    if(Proto_LibTypes[i].StructureName &&
    !(strcmp(Proto_LibTypes[i].BaseName, BaseName)))
    {
      return Proto_LibTypes[i].StructureName;
    }
  }
  return "Library";
}

static strptr GetLibraryName(void)
{
  uint32 i;

  if(libname)
    return libname;

  for(i = 0; BaseName && Proto_LibTypes[i].BaseName; ++i)
  {
    if(Proto_LibTypes[i].LibraryName &&
    !(strcmp(Proto_LibTypes[i].BaseName, BaseName)))
    {
      return (libname = Proto_LibTypes[i].LibraryName);
    }
  }
  if(!(libname = malloc(strlen(ShortBaseName)+9)))
    return 0;

  /* auto create name */
  for(i = 0; ShortBaseName[i]; ++i)
    libname[i] = ShortBaseName[i];
  strcpy(libname+i,".library");
  return libname;
}

static strptr GetIFXName(void)
{
  static char IFXName[256];
  sprintf(IFXName, "%c%s", toupper(ShortBaseName[0]), ShortBaseName+1);
  return IFXName;
}

static int32 MakeShortBaseName(void)
{
  strptr ptr, p2;
  uint32 i;

  ptr = p2 = args.infile;
  while(*p2)
  {
    if(*p2 == '/' || *p2 == ':' || *p2 == '\\')
      ptr = p2+1;
    ++p2;
  }

  /* first get name from file */

  p2 -= (sizeof(SFDFILEEXTENSION)-1);
  if(p2 > ptr && !stricmp(p2, SFDFILEEXTENSION))
    ShortBaseName = DupString(ptr, p2-ptr);
  p2 += sizeof(SFDFILEEXTENSION)-sizeof(FDFILEEXTENSION);
  if(p2 > ptr && !stricmp(p2, FDFILEEXTENSION))
    ShortBaseName = DupString(ptr, p2-ptr);

  /* then try exceptions (overriding filename) */
  if(BaseName)
  {
    for(i = 0; BaseName && Proto_LibTypes[i].BaseName; ++i)
    {
      if(Proto_LibTypes[i].ShortBaseName &&
      !(strcmp(Proto_LibTypes[i].BaseName, BaseName)))
      {
        if(!(ShortBaseName = DupString(Proto_LibTypes[i].ShortBaseName,
        strlen(Proto_LibTypes[i].ShortBaseName))))
          return 0;
        break;
      }
    }
    /* and last use default method */
    if(!ShortBaseName)
      ShortBaseName = DupString(BaseName, strlen(BaseName)-4);
  }

  if(!ShortBaseName)
    return 0;

  ptr = ShortBaseName;
  while((*ptr = tolower(*ptr))) /* Convert to lowercase */
    ptr++;
  
  if((ShortBaseNameUpper = DupString(ShortBaseName, strlen(ShortBaseName))))
  {
    ptr = ShortBaseNameUpper;
    while((*ptr = toupper(*ptr))) /* Convert to uppercase */
      ptr++;
  }
  else
    return 0;

  return 1;
}

static uint32 OpenDest(strptr name)
{
  static uint8 printedname = 0;
  strptr b, t;

  t = (strptr) tempbuf;
  if((b = args.to) && *b)
  {
    while(*b)
      *(t++) = *(b++);
    if(*(t-1) != ':' && *(t-1) != '/')
      *(t++) = '/';
  }
  *t = 0;

  if(!(Flags & FLAG_SINGLEFILE))
    printf("ResultFile: %s%s\n", tempbuf, name);
  else if(!printedname++)
  {
    printf("ResultType: %s", tempbuf); printf(filenamefmt, "*");
    printf("\n");
  }

  while(*name)
    *(t++) = *(name++);
  *t = 0;

  if(args.header)
  {
    HEADER = mygetfile((strptr)tempbuf, &headersize);
    FindHeader();
  }

  if((outfile = fopen((strptr)tempbuf, "wb")))
    return 1;
  DoError(ERR_OPEN_FILE, 0, tempbuf);
  return 0;
}

static uint32 CloseDest(strptr name)
{
  if(outfile)
  {
    fclose(outfile);
    outfile = 0;

    if(!(Flags & FLAG_DONE) || !Output_Error)
    {
      strptr b, t;
      if(!Output_Error || !(Flags & FLAG_SINGLEFILE))
        DoError(ERR_EMPTY_FILE, 0);

      t = (strptr) tempbuf;
      if((b = args.to) && *b)
      {
        while(*b)
          *(t++) = *(b++);
        if(*(t-1) != ':' && *(t-1) != '/')
          *(t++) = '/';
      }
      while(*name)
        *(t++) = *(name++);
      *t = 0;

      remove((strptr)tempbuf);
      return 0;
    }
    Flags &= ~FLAG_DONE;        /* clear the flag */
  }
  else
    return 0;
  return 1;
}

static uint32 MakeTagFunction(struct AmiPragma *ap)
{
  size_t len = strlen(ap->FuncName);
  long i=0;

#ifdef DEBUG_OLD
  printf("MakeTagFunction:\n");
#endif

  if(!ap->NumArgs || ap->TagName)
    return 1;

  ++tagfuncs;
  ap->Flags |= AMIPRAGFLAG_OWNTAGFUNC;

  while(Pragma_ExecpNames[i].FunctionName && /* check the exception names */
  strcmp(ap->FuncName, Pragma_ExecpNames[i].FunctionName))
    ++i;

  if(Pragma_ExecpNames[i].FunctionName)
  {
    if(!(ap->TagName = Pragma_ExecpNames[i].TagName))
    {
      ap->Flags ^= AMIPRAGFLAG_OWNTAGFUNC;
      --tagfuncs;
    }
  }
  else if(ap->FuncName[len-1] == 'A')
  {
    /* skip names with DMA or MESA at end */
    if(!strcmp(ap->FuncName+len-3, "DMA") ||
    !strcmp(ap->FuncName+len-4, "MESA") ||
    !strcmp(ap->FuncName+len-4, "RGBA") ||
    !strcmp(ap->FuncName+len-5, "AMIGA"))
    { ap->Flags ^= AMIPRAGFLAG_OWNTAGFUNC; --tagfuncs; return 1;}
    if(!(ap->TagName = DupString(ap->FuncName, len-1)))
      return 0;
  }
  else if(!strcmp(ap->FuncName + len-7, "TagList"))
  {
    if(!(ap->TagName = DupString(ap->FuncName, len-3)))
      return 0;
    ap->TagName[len-4] = 's';
  }
  else if(!strcmp(ap->FuncName + len-4, "Args"))
  {
    if(!(ap->TagName = DupString(ap->FuncName, len-4)))
      return 0;
  }
  else if(!stricmp(ap->Args[ap->CallArgs-1].ArgName, "tags") ||
  !stricmp(ap->Args[ap->CallArgs-1].ArgName, "taglist"))
  {
    if(!(ap->TagName = DupString(ap->FuncName, len+4)))
      return 0;
    memcpy(ap->TagName + len, "Tags", 5);
  }
  else if(!stricmp(ap->Args[ap->CallArgs-1].ArgName, "args"))
  {
    if(!(ap->TagName = DupString(ap->FuncName, len+4)))
      return 0;
    memcpy(ap->TagName + len, "Args", 5);
  }
  else
  {
    ap->Flags ^= AMIPRAGFLAG_OWNTAGFUNC;
    --tagfuncs; /* not a tagfunction, incrementing was false, undo it */
  }

#ifdef DEBUG
  if(ap->TagName)
    printf("MakeTagFunction: %s / %s (...%s)\n", ap->TagName,
    ap->FuncName, ap->Args[ap->CallArgs-1].ArgName);
#endif

  return 1;
}

static void MakeLines(strptr buffer, uint32 size)
{
  if(size && buffer)
  {
    /* make a real C++ zero string ending line */
    while(size--)
    {
      if(*buffer == '\n')
        *buffer = '\0';
      ++buffer;
    }
    *buffer = '\0';
  }
}

/* Do any special functions, which cannot be done with other exception
   stuff - currently only dos.library DoPkt function. */
static uint32 SpecialFuncs(void)
{
  struct AmiPragma *ap = (struct AmiPragma *) AmiPragma.Last,
                   *ap2 = (struct AmiPragma *) AmiPragma.First;

  /* first let one more go away, so we can detect if the DoPkt parts are
     already inserted in the FD file */

  while(ap2 && (struct AmiPragma *)(ap2->List.Next) != ap)
    ap2 = (struct AmiPragma *)(ap2->List.Next);

  if(ap2 && ap2->Bias == 0xF0 && ap->Bias != 0xF0 &&
  !strcmp("DoPkt", ap2->FuncName))
  {
    struct AmiPragma *d;
    uint32 i;

    RemoveItem(&AmiPragma, (struct ShortList *) ap); /* add in correct order */
    for(i = 0; i < 5; ++i)
    {
      if(!(d = (struct AmiPragma *) NewItem(&AmiPragma)))
        return 0;
      memcpy(d, ap2, sizeof(struct AmiPragma));
      d->FuncName = DupString(ap2->FuncName, 6);
      d->FuncName[5] = '0'+i;
      d->NumArgs = d->CallArgs = i + 2;
      AddItem(&AmiPragma, (struct ShortList *) d);
    }
    AddItem(&AmiPragma, (struct ShortList *) ap);
  }
  return 1;
}

static void SortFDList(void)
{
  struct AmiPragma *ap = (struct AmiPragma *) AmiPragma.First, *ap2, *ap3;
  AmiPragma.First = AmiPragma.Last = 0;

  while(ap)
  {
    ap3 = 0;
    ap2 = (struct AmiPragma *) AmiPragma.First;

    /* for FD2Inline style we need to use strcmp instead of stricmp here */
    while(ap2 && stricmp(ap2->FuncName, ap->FuncName) < 0)
    {
      ap3 = ap2;
      ap2 = (struct AmiPragma *) ap2->List.Next;
    }

    ap2 = ap;
    ap = (struct AmiPragma *) ap->List.Next;

    if(ap3)
    {
      ap2->List.Next = (struct ShortList *) ap3->List.Next;
      ap3->List.Next = (struct ShortList *) ap2;
    }
    else
    {
      ap2->List.Next = AmiPragma.First;
      AmiPragma.First = (struct ShortList *) ap2;
    }
    if(ap && !ap->List.Next)
      AmiPragma.Last = (struct ShortList *) ap2;
  }
}

static void AddAliasName(struct AmiPragma *ap, struct Pragma_AliasName *alias,
uint32 linenum)
{
  int8 i;

  if(ap->NumAlias > NUMALIASNAMES)
    DoError(ERR_ALIASNAMES, linenum, NUMALIASNAMES);
  else
  {
    /* prevent double names */
    for(i = 0; i < ap->NumAlias; ++i)
    {
      if(!strcmp(ap->AliasName[i]->AliasName, alias->AliasName))
        return;
    }
    ap->AliasName[ap->NumAlias++] = alias;
  }
}

static uint32 CheckNames(struct AmiPragma *ap)
{
  uint32 i, j;
  const strptr *k;

#ifdef DEBUG_OLD
  printf("CheckNames\n");
#endif
  for(i = 0; i < ap->CallArgs; ++i)
  {
    if(!ap->Args[i].ArgName)
    {
      if(!(ap->Args[i].ArgName = (strptr)
      AllocListMem(4+strlen(RegNames[ap->Args[i].ArgReg]))))
        return 0;
      sprintf(ap->Args[i].ArgName, "%sarg", RegNames[ap->Args[i].ArgReg]);
    }
    else
    {
      for(k = Keywords; *k; ++k)
      {
        if(!stricmp(ap->Args[i].ArgName, *k))
        {
          DoError(ERR_ARGNAME_KEYWORD_CONFLICT, ap->Line, i, *k);
          if(!(ap->Args[i].ArgName = (strptr) AllocListMem(4
          +strlen(RegNames[ap->Args[i].ArgReg]))))
            return 0;
          sprintf(ap->Args[i].ArgName, "%sarg", RegNames[ap->Args[i].ArgReg]);
        }
      }
      for(j = 0; j < i; ++j)
      {
        if(!stricmp(ap->Args[i].ArgName, ap->Args[j].ArgName))
        {
          DoError(ERR_ARGNAME_ARGNAME_CONFLICT, ap->Line, i+1, j+1);
          if(!(ap->Args[i].ArgName = (strptr) AllocListMem(4
          +strlen(RegNames[ap->Args[i].ArgReg]))))
            return 0;
          sprintf(ap->Args[i].ArgName, "%sarg", RegNames[ap->Args[i].ArgReg]);
        }
      }
    }
  }
  /* NOTE: the replaced argument names aren't checked for conflicts */
  /* replaced names are of style a0arg */
  return 1;
}

static uint32 ScanSFDFile(enum ABI abi)
{
  uint32 _public = 1;
  int32 bias = -1;
  uint32 linenum;
  uint32 actcom = 0;
  uint32 functype = 0;

  Flags2 |= FLAG2_SFDMODE;

  if(strncmp("==id", in.pos, 4))
    DoError(ERR_SFD_START, 1);

  for(linenum = 1; in.pos < in.buf + in.size; ++linenum)
  {
    if(*in.pos == '*')
    {
      if(linenum < 5 && in.pos[1] == ' ' && in.pos[2] == '\"')
      {
        strptr s;
        in.pos += 3;
        for(s = in.pos; *s && *s != '"'; ++s)
          ;
        if(*s) /* library name */
        {
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found library name comment\n");
#endif
          if(!libname)
          {
            libname = in.pos;
            *(s++) = 0; /* clear the " */
            in.pos = s;
            Flags2 |= FLAG2_LIBNAMECOM;
          }
        }
      }
      else
      {
        if(actcom)
          *(in.pos-1) = '\n';
        else
        {
          struct Comment *d;
          if(!(d = (struct Comment *) NewItem(&Comment)))
            return 0;
          d->Bias = bias;
          d->Data = in.pos;
          d->ReservedNum = 0;
          d->Version = 0;
          d->Private = _public ? 0 : 1;
          AddItem(&Comment, (struct ShortList *) d);
          actcom = 1;
        }
      }
      while(*in.pos)
        ++in.pos;
    }
    else if(*in.pos == '=' && in.pos[1] == '=')
    {
      in.pos += 2;
      actcom = 0; /* no Comment */

      if(!strnicmp(in.pos, "basetype", 8))
      {
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==basetype\n");
#endif
        if(!(Flags2 & FLAG2_LIBTYPE))
        {
          if(libtype)
            DoError(ERR_LIBTYPE_DECLARED_TWICE, linenum);

          in.pos = SkipBlanks(in.pos+8);
          if(strncmp(in.pos, "struct", 6))
            DoError(ERR_EXPECTED_STRUCT, linenum);
          else
          {
            in.pos = SkipBlanks(in.pos+6);
            if(!*in.pos)
              DoError(ERR_EXPECTED_LIBTYPE, linenum);
            else
            {
              libtype = in.pos;
              in.pos = SkipName(libtype);
              if(*SkipBlanks(in.pos) != '*')
                DoError(ERR_EXPECTED_POINTERSIGN, linenum);
              if(*in.pos)
                *(in.pos++) = 0;
            }
          }
        }
        else
          DoError(ERR_COMMANDLINE_LIBTYPE, linenum);
        while(*in.pos)
          ++in.pos;
      }
      else if(!strnicmp(in.pos, "copyright", 9))
      {
        Copyright = SkipBlanks(in.pos+9);
        while(*in.pos)
          ++in.pos;
      }
      else if(!strnicmp(in.pos, "libname", 7))
      {
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==libname\n");
#endif
        if(!(Flags2 & FLAG2_LIBNAME))
        {
          if(libname && !(Flags2 & FLAG2_LIBNAMECOM))
            DoError(ERR_LIBNAME_DECLARED_TWICE, linenum);

          in.pos = SkipBlanks(in.pos+7);
          if(!*in.pos)
            DoError(ERR_EXPECTED_LIBNAME, linenum);
          else
            in.pos = SkipName(libname = in.pos);
        }
        else
          DoError(ERR_COMMANDLINE_LIBNAME, linenum);
        while(*in.pos)
          ++in.pos;
      }
      else if(!strnicmp(in.pos, "base", 4))
      {
        strptr oldptr;

#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==base\n");
#endif
        if(!(Flags & FLAG_BASENAME))
        {
          if(BaseName)
            DoError(ERR_BASENAME_DECLARED_TWICE, linenum);

          in.pos = SkipBlanks(in.pos+4);
          if(*in.pos != '_')
            DoError(ERR_EXPECTED_SLASH_IN_BASENAME, linenum);
          else
            ++in.pos;

          BaseName = oldptr = in.pos;
          in.pos = SkipName(in.pos);
          if(!(in.pos-oldptr))
            DoError(ERR_EXPECTED_BASENAME, linenum);
        }
        else
        {
          DoError(ERR_COMMANDLINE_BASENAME, linenum);
          while(*in.pos)
            ++in.pos;
        }
      }
      else if(!strnicmp(in.pos, "bias", 4))
      {
        strptr ptr;
        int32 newbias;

#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==bias\n");
#endif
        in.pos += 5;
        newbias = strtol(in.pos, &ptr, 10);
        if(ptr == in.pos)
          DoError(ERR_EXPECTED_BIAS_VALUE, linenum);
        else if(newbias < 0)
        {
          DoError(ERR_ASSUMING_POSITIVE_BIAS_VALUE, linenum);
          bias = -newbias;
        }
        else
          bias = newbias;
        in.pos = SkipName(in.pos);
      }
      else if(!strnicmp(in.pos, "end", 3))
      {
        bias = 0; break;
      }
      else if(!strnicmp(in.pos, "public", 6))
      {
        in.pos += 6;
        _public = 1;
      }
      else if(!strnicmp(in.pos, "private", 7))
      {
        in.pos += 7;
        _public = 0;
      }
      else if(!strnicmp(in.pos, "abi", 3))
      {
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==abi\n");
#endif
        in.pos = SkipBlanks(in.pos+3);
        if(!strnicmp(in.pos, "M68k", 4))
        {
          abi = ABI_M68K; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC0", 4))
        {
          abi = ABI_PPC0; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC2", 4))
        {
          abi = ABI_PPC2; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC", 3))
        {
          abi = ABI_PPC; in.pos += 3;
        }
        else
          DoError(ERR_UNKNOWN_ABI, linenum, in.pos);
      }
      else if(!strnicmp(in.pos, "id", 2))
      {
        if(IDstring)
          DoError(ERR_IDSTRING_DECLARED_TWICE, linenum);
        IDstring = in.pos = SkipBlanks(in.pos+2);
        if(strncmp(in.pos, "$Id: ", 5))
        {
          DoError(ERR_EXCPECTED_IDSTRING, linenum);
        }
        while(*in.pos)
          ++in.pos;
        if(*(in.pos-1) != '$')
          DoError(ERR_EXPECTED_ID_ENDSIGN, linenum);
      }
      else if(!strnicmp(in.pos, "include", 7))
      {
        struct Include *d;

        if(!(d = (struct Include *) NewItem(&Includes)))
          return 0;
        d->Include = SkipBlanks(in.pos+7);
        AddItem(&Includes, (struct ShortList *) d);
        while(*in.pos)
          ++in.pos;
      }
      else if(!strnicmp(in.pos, "varargs", 7))
      {
        if(bias == -1)
          DoError(ERR_VARARGS_ALIAS_FIRST, linenum);
        else
        {
          if(!functype)
            bias -= BIAS_OFFSET;
          functype |= FUNCFLAG_TAG;
        }
        in.pos += 7;
      }
      else if(!strnicmp(in.pos, "alias", 5))
      {
        if(bias == -1)
          DoError(ERR_VARARGS_ALIAS_FIRST, linenum);
        else
        {
          if(!functype)
            bias -= BIAS_OFFSET;
          functype |= FUNCFLAG_ALIAS;
        }
        in.pos += 5;
      }
      else if(!strnicmp(in.pos, "version", 7))
      {
        /* store version entries as comments */
        struct Comment *d;
        strptr ptr;
        int16 v;

        in.pos = SkipBlanks(in.pos+7);
        v = strtol(in.pos, &ptr, 10);
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==version %d\n", v);
#endif
        if(ptr == in.pos || v < 0)
          DoError(ERR_EXPECTED_POSITIVE_DECIMAL_NUMBER, linenum);
        else
        {
          if(!(d = (struct Comment *) NewItem(&Comment)))
            return 0;
          d->Bias = bias;
          d->Data = 0;
          d->ReservedNum = 0;
          d->Version = v;
          d->Private = _public ? 0 : 1;
          AddItem(&Comment, (struct ShortList *) d);
          in.pos = SkipName(in.pos);
        }
      }
      else if(!strnicmp(in.pos, "reserve", 7))
      {
        /* store reserved entries as comments */
        struct Comment *d;
        strptr ptr;
        int16 v;

        in.pos = SkipBlanks(in.pos+7);
        v = strtol(in.pos, &ptr, 10);
#ifdef DEBUG_OLD
  printf("ScanSFDFile: found ==reserve %d\n", v);
#endif
        if(bias == -1)
        {
          DoError(ERR_ASSUMING_BIAS_OF_30, linenum);
          bias = BIAS_START;
        }

        if(ptr == in.pos || v < 0)
          DoError(ERR_EXPECTED_POSITIVE_DECIMAL_NUMBER, linenum);
        else
        {
          if(!(d = (struct Comment *) NewItem(&Comment)))
            return 0;
          d->Bias = bias;
          d->Data = 0;
          d->ReservedNum = v;
          d->Version = 0;
          d->Private = _public ? 0 : 1;
          AddItem(&Comment, (struct ShortList *) d);
          in.pos = SkipName(in.pos);
          bias += BIAS_OFFSET*v;
        }
      }
      else
        DoError(ERR_UNKNOWN_DIRECTIVE, linenum, in.pos-2);
    }
    else /* function */
    {
      uint32 ft, startlinenum;
      struct AmiPragma ap, *ap2;
      struct ClibData d, *f;
      strptr oldptr;
      uint32 maxreg;
      strptr data;

      actcom = 0;
      maxreg = ((abi == ABI_M68K) ? MAXREG-2 : MAXREGPPC);
      /* join lines, if necessary */
      startlinenum = linenum;
      data = in.pos;
      /* first open bracket */
      while(*data != '('/*)*/ && data < in.buf + in.size)
      { if(!*data) {*data = ' '; ++linenum; } ++data; }
      ++data;
      ft = 0; /* this is needed for function pointer types, which have own
                 brackets */
      /* first close bracket */
      while((*data != /*(*/')' || ft) && data < in.buf + in.size)
      {
        if(!*data)
        {
          *data = ' ';
          ++linenum;
        }
        else if(*data == '('/*)*/)
          ++ft;
        else if(*data == /*(*/')')
          --ft;
        ++data;
      }
      /* second open bracket */
      while(*data != '('/*)*/ && data < in.buf + in.size)
      { if(!*data) {*data = ' '; ++linenum; } ++data; }
      /* second close bracket */
      while(*data != /*(*/')' && data < in.buf + in.size)
      { if(!*data) {*data = ' '; ++linenum; } ++data; }
      if(data == in.buf + in.size)
      {
        in.pos = data;
        DoError(ERR_UNEXPECTED_FILEEND, linenum);
        continue;
      }

      ft = functype; functype = 0;
      memset(&ap, 0, sizeof(struct AmiPragma));
      memset(&d, 0, sizeof(struct ClibData));
      if(!GetCPPType(&d.ReturnType, in.pos, 1, 1))
      {
        DoError(ERR_UNKNOWN_RETURNVALUE_TYPE, startlinenum);
        while(*(in.pos++))
          ;
        continue;
      }
      else if(d.ReturnType.Unknown)
        DoError(ERR_UNKNOWN_RETURNVALUE_TYPE_INT, startlinenum,
        d.ReturnType.Unknown);

      ap.FuncName = d.FuncName = SkipBlanks(d.ReturnType.TypeStart
      +d.ReturnType.FullLength);
      in.pos = SkipBlanks(SkipName(d.FuncName));
        ;
      if(*in.pos != '('/*)*/)
      {
        DoError(ERR_EXPECTED_OPEN_BRACKET, startlinenum);
        ++in.pos;
        continue;
      }
      *(SkipName(d.FuncName)) = 0;
      in.pos = SkipBlanks(++in.pos);

      oldptr = 0;
      while(*in.pos && *in.pos != /*(*/')')
      {
        oldptr = (strptr) 1;
        if(d.NumArgs >= maxreg)
        {
          DoError(ERR_TO_MUCH_ARGUMENTS, startlinenum);
          return 0;
        }
        else if(!GetCPPType(&d.Args[d.NumArgs++], in.pos, 0, 0))
        {
          DoError(ERR_UNKNOWN_VARIABLE_TYPE, startlinenum, d.NumArgs);
          break;
        }
        else if(d.Args[d.NumArgs-1].Unknown)
          DoError(ERR_UNKNOWN_VARIABLE_TYPE_INT, startlinenum, d.NumArgs,
          d.Args[d.NumArgs-1].Unknown);

        oldptr = in.pos = SkipBlanks(d.Args[d.NumArgs-1].TypeStart
        + d.Args[d.NumArgs-1].FullLength);
        if(d.Args[d.NumArgs-1].Type != CPP_TYPE_VARARGS)
        {
          if(d.Args[d.NumArgs-1].Flags & CPP_FLAG_FUNCTION)
          {
            oldptr = d.Args[d.NumArgs-1].FunctionName;
            if(!oldptr)
            {
              DoError(ERR_EXPECTED_ARGUMENT_NAME, startlinenum);
              break;
            }
            else if(!(oldptr = DupString(oldptr, SkipName(oldptr)-oldptr)))
              return 0;
            ap.Args[ap.CallArgs++].ArgName = oldptr;
          }
          else
          {
            ap.Args[ap.CallArgs++].ArgName = in.pos;
            in.pos = SkipName(in.pos);

            if(in.pos == oldptr)
            {
              DoError(ERR_EXPECTED_ARGUMENT_NAME, startlinenum);
              break;
            }
          }
        }
        else
          ++ap.CallArgs;

        in.pos = SkipBlanks(in.pos);
        if(*in.pos != ',' && *in.pos != /*(*/')')
        {
          DoError(ERR_EXPECTED_CLOSE_BRACKET, startlinenum);
          break;
        }
        if(*in.pos == ')')
        {
          in.pos = SkipBlanks(++in.pos);
          if(d.Args[d.NumArgs-1].Type != CPP_TYPE_VARARGS &&
          !(d.Args[d.NumArgs-1].Flags & CPP_FLAG_FUNCTION))
            *(SkipName(oldptr)) = 0;
#ifdef DEBUG_OLD
  printf("Added last argument %d (%s) for %s (%d bytes)\n", d.NumArgs,
  oldptr, d.FuncName, d.Args[d.NumArgs-1].FullLength);
#endif
          oldptr = 0;
          break;
        }
        else
        {
          in.pos = SkipBlanks(++in.pos);
          *(SkipName(oldptr)) = 0;
#ifdef DEBUG_OLD
  printf("Added argument %d (%s) for %s (%d bytes)\n", d.NumArgs,
  oldptr, d.FuncName, d.Args[d.NumArgs-1].FullLength);
#endif
        }
      }
      if(*in.pos == /*(*/')')
        ++in.pos;
      if(!oldptr) /* oldptr == 0 means parsing was valid */
      {
        if(!(f = (struct ClibData *) AllocListMem(sizeof(struct ClibData))))
          return -1;

        memcpy(f, &d, sizeof(struct ClibData));

        if(!clibdata)
          clibdata = f;
        else
        {
          struct ClibData *e = clibdata;
          while(e->Next)
            e = e->Next;
          e->Next = f;
        }

#ifdef DEBUG_OLD
  printf("Added prototype for %s (line %ld) with %d args\n", f->FuncName,
  startlinenum, f->NumArgs);
#endif
        if(*(in.pos = SkipBlanks(in.pos)) != '('/*)*/)
        {
          DoError(ERR_EXPECTED_OPEN_BRACKET, startlinenum);
          ++in.pos;
          continue;
        }

        if(bias == -1)
        {
          DoError(ERR_ASSUMING_BIAS_OF_30, startlinenum);
          bias = BIAS_START;
        }

        ap.Bias = bias;
        ap.Abi = abi;
        ap.Line = startlinenum;
        bias += BIAS_OFFSET;

        if(_public)
          ap.Flags |= AMIPRAGFLAG_PUBLIC;

        if(abi != ABI_M68K)
        {
          while(*in.pos && *in.pos != /*(*/')')
           ++in.pos;
          if(*in.pos != /*(*/')')
          {
            DoError(ERR_EXPECTED_CLOSE_BRACKET, startlinenum);
            ++in.pos;
            continue;
          }
          ++in.pos;
          ap.NumArgs = ap.CallArgs;

          ap.Flags |= AMIPRAGFLAG_PPC;
          if(abi == ABI_PPC0)
            ap.Flags |= AMIPRAGFLAG_PPC0;
          else if(abi == ABI_PPC2)
            ap.Flags |= AMIPRAGFLAG_PPC2;
        }
        else
        {
          uint32 len;

          do
          {
            uint32 i;

            oldptr = in.pos = SkipBlanks(in.pos+1);

            if(*in.pos == /*(*/')' && !ap.NumArgs)
              break;

            in.pos = SkipName(oldptr);
            len = in.pos-oldptr;

            for(i = 0; i < MAXREG; ++i)
              if(!strnicmp(RegNames[i], oldptr, len))
                break;

            if(i == MAXREG)
            {
              DoError(ERR_EXPECTED_REGISTER_NAME, startlinenum);
              break;
            }
            else if(i == REG_A6)
              ap.Flags |= AMIPRAGFLAG_A6USE;
            else if(i == REG_A5)
              ap.Flags |= AMIPRAGFLAG_A5USE;
            else if(i == REG_A4)
              ap.Flags |= AMIPRAGFLAG_A4USE;
            else if(i == REG_D7)
              ap.Flags |= AMIPRAGFLAG_D7USE;
            else if(i == REG_A7)
            {
              DoError(ERR_A7_NOT_ALLOWED, startlinenum);
              break;
            }
            else if(i >= REG_FP0)
              ap.Flags |= AMIPRAGFLAG_FLOATARG;

            ap.Args[ap.NumArgs].ArgReg = i;

            for(i = 0; i < ap.NumArgs; i++)
            {
              if(ap.Args[ap.NumArgs].ArgReg == ap.Args[i].ArgReg)
              {
                DoError(ERR_REGISTER_USED_TWICE, startlinenum);
                break;
              }
            }
            if(i < ap.NumArgs)
              break;

            ++ap.NumArgs;

            in.pos = SkipBlanks(in.pos);
            if(*in.pos != ',' && *in.pos != '-' && *in.pos != '/' &&
            *in.pos != /*(*/')')
            {
              DoError(ERR_EXPECTED_CLOSE_BRACKET, startlinenum);
              break;
            }
          } while(*in.pos != /*(*/')');

          if(*in.pos != /*(*/')')
          {
            while(*(in.pos++))
              ++in.pos;
            continue;
          }
          else
            ++in.pos;
        }
        ap2 = (struct AmiPragma *)(AmiPragma.Last);
        if(ft && !(ft & FUNCFLAG_TAG) && ap.NumArgs != ap2->NumArgs)
          ft = 0; /* like DoPkt, handle as seperate function */
        if(ft) /* handle alias and varargs */
        {
          if(ap2->TagName || (ft & FUNCFLAG_ALIAS))
          {
            struct Pragma_AliasName *p;

            if((p = (struct Pragma_AliasName *)
            AllocListMem(sizeof(struct Pragma_AliasName))))
            {
              p->FunctionName = ap2->TagName;
              p->AliasName = ap.FuncName;
              p->Type = (ft & FUNCFLAG_TAG) ? FUNCFLAG_TAG : FUNCFLAG_NORMAL;
              AddAliasName(ap2, p, startlinenum);
            }
            else
             return 0;
          }
          else
          {
            ap2->TagName = ap.FuncName;
            ++tagfuncs;
          }
          if(ap.CallArgs != ap2->CallArgs)
          {
            if(ap2->CallArgs + 1 == ap.CallArgs && d.Args[d.NumArgs-1].Type
            == CPP_TYPE_VARARGS)
            {
              --ap.CallArgs;
              if(abi != ABI_M68K)
                --ap.NumArgs;
            }
          }
          if(ap.NumArgs != ap2->NumArgs)
          {
            DoError(ERR_VARARGS_ARGUMENTS_DIFFER, startlinenum);
          }
          else if(abi == ABI_M68K)
          {
            uint32 i;

            for(i = 0; i < ap2->NumArgs; ++i)
            {
              if(ap2->Args[i].ArgReg != ap.Args[i].ArgReg)
              {
                DoError(ERR_VARARGS_ARGUMENTS_DIFFER, startlinenum);
                break;
              }
            }
          }
        }
        else if(abi == ABI_M68K)
        {
          if(ap.CallArgs != ap.NumArgs)
          { /* this is surely no longer necessary, as there wont be any
               varargs functions here */
            if(ap.CallArgs == ap.NumArgs+1 && d.Args[d.NumArgs-1].Type
            == CPP_TYPE_VARARGS)
              --ap.CallArgs;
            else
              ap.Flags |= AMIPRAGFLAG_ARGCOUNT;
          }

          ap.Flags |= AMIPRAGFLAG_M68K;

          if((Flags & FLAG_NOFPU) && (ap.Flags & AMIPRAGFLAG_FLOATARG))
            DoError(ERR_FLOATARG_NOT_ALLOWED, startlinenum);
          else if(((ap.Flags & AMIPRAGFLAG_FLOATARG) || !(Flags & FLAG_FPUONLY))
          && !(Flags & FLAG_PPCONLY))
          { /* skip all without FPU when FPUONLY and PPC when PPCONLY */
            struct AmiPragma *d;
            if(!(d = (struct AmiPragma *) NewItem(&AmiPragma)))
              return 0;
            memcpy(d, &ap, sizeof(struct AmiPragma));
            if(!CheckNames(d))
              return 0;
            AddItem(&AmiPragma, (struct ShortList *) d);
          }
        }
        else
        {
          if(!(Flags & FLAG_NOPPC))
          {
            struct AmiPragma *d;
            if(!(d = (struct AmiPragma *) NewItem(&AmiPragma)))
              return 0;
            memcpy(d, &ap, sizeof(struct AmiPragma));
            if(!CheckNames(d))
              return 0;
            AddItem(&AmiPragma, (struct ShortList *) d);
          }
        }
      }
    }

    in.pos = SkipBlanks(in.pos);
    if(*in.pos)
      DoError(ERR_EXTRA_CHARACTERS, linenum);
    ++in.pos; /* skip '\0' */
  }

  if(bias)
    DoError(ERR_MISSING_SFDEND, 0);

  return 1;
}

static uint32 ScanFDFile(void)
{
  uint32 _public = 1;
  int32 bias = -1;
  uint32 linenum;
  size_t len;
  uint32 actcom = 0;
  uint32 shadowmode = 0;
  enum ABI abi = ABI_M68K;

  if(defabi)
  {
    if(!stricmp(defabi, "M68k"))
      abi = ABI_M68K;
    else if(!stricmp(defabi, "PPC0"))
      abi = ABI_PPC0;
    else if(!stricmp(defabi, "PPC2"))
      abi = ABI_PPC2;
    else if(!stricmp(defabi, "PPC"))
      abi = ABI_PPC;
    else
      DoError(ERR_UNKNOWN_ABI, 0, defabi);
  }

  if(in.size > 10 && in.pos[0] == '=' && in.pos[1] == '=')
    return ScanSFDFile(abi);

#ifdef DEBUG_OLD
  printf("ScanFDFile:\n");
#endif

  for(linenum = 1; in.pos < in.buf + in.size; ++linenum)
  {
    if(*in.pos == '*')          /*  Comment   */
    {
      strptr oldpos = in.pos;
#ifdef DEBUG_OLD
  printf("ScanFDFile: found a comment\n");
#endif
      in.pos = SkipBlanks(in.pos+1);
      if(!strnicmp(in.pos, "notagcall", 9))
      {
        struct AmiPragma *ap = (struct AmiPragma *) AmiPragma.Last;
        
        if(ap->TagName)
        {
          --tagfuncs; ap->TagName = 0;
          ap->Flags &= ~(AMIPRAGFLAG_OWNTAGFUNC);
        }
        in.pos = SkipBlanks(in.pos + 9);
      }
      else if(!strnicmp(in.pos, "tagcall", 7))  /*  Tag to create?  */
      {
        struct AmiPragma *prevpragma = (struct AmiPragma *) AmiPragma.Last;

        in.pos = SkipBlanks(in.pos + 7);
        if(!prevpragma)
        {
          DoError(ERR_TAG_DEF_WITHOUT_PRAGMA, linenum);
          ++in.pos;
          continue;
        }

        if(!prevpragma->NumArgs)
        {
          DoError(ERR_TAGFUNC_NEEDS_ARGUMENT, linenum);
          ++in.pos;
          continue;
        }

        /* Get the tag functions name. */

        if(!prevpragma->TagName && (_public || (Flags & FLAG_PRIVATE)))
          ++tagfuncs;

        if(*in.pos)
        {
          strptr oldptr, tptr = prevpragma->TagName;
        
          len = strlen(prevpragma->FuncName)+strlen(in.pos)+1;
          if(!(prevpragma->TagName = DupString(prevpragma->FuncName, len)))
            return 0;

          if(*in.pos == '-')
          {
            strptr removeptr;

            oldptr = in.pos = SkipBlanks(in.pos+1);
            in.pos = SkipName(in.pos);
            if((len = in.pos-oldptr))
            {
              removeptr = prevpragma->TagName+strlen(prevpragma->TagName)-len;
              if(strncmp(removeptr, oldptr, len))
              {
#ifdef DEBUG_OLD
  printf("ScanFDFile: *tagcall -: %s, %s, %d\n", removeptr, oldptr, len);
#endif
                DoError(ERR_CANNOT_CONVERT_PRAGMA_TAGCALL, linenum);
                prevpragma->TagName = tptr;
                ++in.pos;
                continue;
              }

              *removeptr = '\0';
            }
            in.pos = SkipBlanks(in.pos);
          }
          if(*in.pos == '+')
            in.pos = SkipBlanks(in.pos+1);
          else
            *in.pos = toupper(*in.pos);

          in.pos = SkipName((oldptr = in.pos));
          len = in.pos-oldptr;
          if(len)
          {
            uint32 a = strlen(prevpragma->TagName);
            memcpy(prevpragma->TagName+a, oldptr, len);
            prevpragma->TagName[a+len] = '\0';
          }
        }
        else if(!prevpragma->TagName)
        {
          len = strlen(prevpragma->FuncName);
          if(!(prevpragma->TagName = DupString(prevpragma->FuncName, len+4)))
            return 0;
          memcpy(prevpragma->TagName + len, "Tags", 5);
        }
      }
      else
      {
        if(actcom)
          *(oldpos-1) = '\n';
        else
        {
          struct Comment *d;
          if(!(d = (struct Comment *) NewItem(&Comment)))
            return 0;
          d->Bias = bias;
          d->Data = oldpos;
          d->ReservedNum = 0;
          d->Version = 0;
          d->Private = _public ? 0 : 1;
          AddItem(&Comment, (struct ShortList *) d);
          actcom = 1;
        }
        while(*in.pos)
          ++in.pos;
      }
    }
    else if(*in.pos == '#' && in.pos[1] == '#')
    {
      in.pos += 2;
      actcom = 0; /* no Comment */

      if(!strnicmp(in.pos, "base", 4))
      {
        strptr oldptr;

#ifdef DEBUG_OLD
  printf("ScanFDFile: found ##base\n");
#endif
        if(!(Flags & FLAG_BASENAME))
        {
          if(BaseName)
            DoError(ERR_BASENAME_DECLARED_TWICE, linenum);

          in.pos = SkipBlanks(in.pos+4);
          if(*in.pos != '_')
            DoError(ERR_EXPECTED_SLASH_IN_BASENAME, linenum);
          else
            ++in.pos;

          BaseName = oldptr = in.pos;
          in.pos = SkipName(in.pos);
          if(!(in.pos-oldptr))
            DoError(ERR_EXPECTED_BASENAME, linenum);
        }
        else
        {
          DoError(ERR_COMMANDLINE_BASENAME, linenum);
          while(*in.pos)
            ++in.pos;
        }
      }
      else if(!strnicmp(in.pos, "bias", 4))
      {
        strptr ptr;
        int32 newbias;

#ifdef DEBUG_OLD
  printf("ScanFDFile: found ##bias\n");
#endif
        in.pos += 5;
        newbias = strtol(in.pos, &ptr, 10);
        if(ptr == in.pos)
          DoError(ERR_EXPECTED_BIAS_VALUE, linenum);
        else if(newbias < 0)
        {
          DoError(ERR_ASSUMING_POSITIVE_BIAS_VALUE, linenum);
          bias = -newbias;
        }
        else
          bias = newbias;
        in.pos = SkipName(in.pos);
      }
      else if(!strnicmp(in.pos, "end", 3))
      {
        bias = 0; break;
      }
      else if(!strnicmp(in.pos, "shadow", 6)) /* introduced by Storm */
      {
        in.pos += 6;
        if(bias == -1 || !AmiPragma.First)
          DoError(ERR_EARLY_SHADOW, linenum);
        else
        {
          bias -= BIAS_OFFSET;
          shadowmode = 1;
        }
      }
      else if(!strnicmp(in.pos, "public", 6))
      {
        in.pos += 6;
        _public = 1;
      }
      else if(!strnicmp(in.pos, "private", 7))
      {
        in.pos += 7;
        _public = 0;
      }
      else if(!strnicmp(in.pos, "abi", 3))
      {
#ifdef DEBUG_OLD
  printf("ScanFDFile: found ##abi\n");
#endif
        in.pos = SkipBlanks(in.pos+3);
        if(!strnicmp(in.pos, "M68k", 4))
        {
          abi = ABI_M68K; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC0", 4))
        {
          abi = ABI_PPC0; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC2", 4))
        {
          abi = ABI_PPC2; in.pos += 4;
        }
        else if(!strnicmp(in.pos, "PPC", 3))
        {
          abi = ABI_PPC; in.pos += 3;
        }
        else
          DoError(ERR_UNKNOWN_ABI, linenum, in.pos);
      }
      else
        DoError(ERR_UNKNOWN_DIRECTIVE, linenum, in.pos-2);
    }
    else
    {
      strptr oldptr;
      uint32 maxreg;
      struct AmiPragma ap, *ap2;

#ifdef DEBUG_OLD
  printf("ScanFDFile: scan Function\n");
#endif
      memset(&ap, 0, sizeof(struct AmiPragma));
      actcom = 0;

      oldptr = in.pos = SkipBlanks(in.pos);
      in.pos = SkipName(oldptr);
      if(!(len = in.pos-oldptr))
      {
        DoError(ERR_MISSING_FUNCTION_NAME, linenum);
        ++in.pos;
        continue;
      }

      ap.FuncName = oldptr;

      in.pos = SkipBlanks(in.pos);
      if(*in.pos != '('/*)*/)
      {
        DoError(ERR_EXPECTED_OPEN_BRACKET, linenum);
        ++in.pos;
        continue;
      }

      oldptr[len] = '\0'; /* create c string of FunctionName */

#ifdef DEBUG_OLD
  printf("ScanFDFile: found function %s\n", ap.FuncName);
#endif

      maxreg = ((abi == ABI_M68K) ? MAXREG-2 : MAXREGPPC);
      do
      {
        oldptr = in.pos = SkipBlanks(in.pos+1);

        if(*in.pos == '*') /* strange OS3.9 files */
        {
          DoError(ERR_ILLEGAL_CHARACTER_DETECTED, linenum);
          oldptr = in.pos = SkipBlanks(in.pos+1);
        }

        if(*in.pos == /*(*/')' && !ap.CallArgs)
          break;

        if(ap.CallArgs >= maxreg)
        {
          DoError(ERR_TO_MUCH_ARGUMENTS, linenum); break;
        }

        if(!strncmp(in.pos, "void", 4))
        {
          /* allows "(void)" instead of ()" */
          in.pos = SkipBlanks(in.pos + 4);
          if(*in.pos != /*(*/')')
            DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
          break;
        }

        if(!strncmp(in.pos, "...", 3))
        {
          ap.Flags = AMIPRAGFLAG_VARARGS;
          ap.Args[ap.CallArgs++].ArgName = 0;
          in.pos = SkipBlanks(in.pos+3);
          if(*in.pos != /*(*/')')
            DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
          break;
        }

        in.pos = SkipName(oldptr);
        if(*in.pos == '*')
          ++in.pos;
        if(!(len = in.pos-oldptr))
        {
          DoError(ERR_EXPECTED_ARGUMENT_NAME, linenum);
          ap.Args[ap.CallArgs++].ArgName = 0;
        }
        else
        {
          ap.Args[ap.CallArgs++].ArgName = oldptr;
          oldptr = in.pos;
          in.pos = SkipBlanks(in.pos);
        }
        if(*in.pos != ',' && *in.pos != '/' && *in.pos != /*(*/')')
        {
          DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
          break;
        }
        if(*in.pos != /*(*/')') /* create c string ending */
          *oldptr = '\0';
      } while(*in.pos != /*(*/')');

      if(*in.pos != /*(*/')')
      {
        while(*(in.pos++))
          ++in.pos;
        continue;
      }
      else
        *oldptr = '\0'; /* create c string ending for last argument */

      if(bias == -1)
      {
        DoError(ERR_ASSUMING_BIAS_OF_30, linenum);
        bias = BIAS_START;
      }

      ap.Bias = bias;
      ap.Abi = abi;
      ap.Line = linenum;
      bias += BIAS_OFFSET;

      if(_public)
        ap.Flags |= AMIPRAGFLAG_PUBLIC;

      in.pos = SkipBlanks(in.pos+1);

      if(*in.pos || ap.CallArgs)
      /* support for FD's without second empty bracket pair */
      {
        if(*in.pos != '('/*)*/)
        {
          DoError(ERR_EXPECTED_OPEN_BRACKET, linenum);
          ++in.pos;
          continue;
        }

        if(abi == ABI_M68K)
        {
          do
          {
            uint32 i;

            oldptr = in.pos = SkipBlanks(in.pos + 1);

            if(*in.pos == /*(*/')' && !ap.NumArgs)
              break;

            if(!strncmp(in.pos, "base", 4))
            {
              in.pos = SkipBlanks(in.pos + 4);
              if(*in.pos == ',')
              {
                in.pos = SkipBlanks(in.pos + 1);
                if(!strncmp(in.pos, "sysv",4))
                {
                  /* MorphOS V.4 with base in r3: (base,sysv) */
                  ap.Flags |= AMIPRAGFLAG_MOSBASESYSV;
                  ap.NumArgs = ap.CallArgs;
                  in.pos = SkipBlanks(in.pos + 4);
                  if(*in.pos != /*(*/')')
                    DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
                  break;
                }
              }
            }
            else if(!strncmp(in.pos, "sysv", 4))
            {
              in.pos = SkipBlanks(in.pos + 4);
              if(*in.pos == ',')
              {
                in.pos = SkipBlanks(in.pos + 1);
                if(!strncmp(in.pos, "r12base",7))
                {
                  /* MorphOS V.4 without passing base: (sysv) */
                  ap.Flags |= AMIPRAGFLAG_MOSSYSVR12;
                  ap.NumArgs = ap.CallArgs;
                  in.pos = SkipBlanks(in.pos + 7);
                  if(*in.pos != /*(*/')')
                    DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
                  break;
                }
              }
              else if (*in.pos == /*(*/')')
              {
                /* MorphOS V.4 without passing base: (sysv) */
                ap.Flags |= AMIPRAGFLAG_MOSSYSV;
                ap.NumArgs = ap.CallArgs;
                break;
              }
              else
              {
                DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
                break;
              }
            }

            in.pos = SkipName(oldptr);
            len = in.pos-oldptr;

            for(i = 0; i < MAXREG; ++i)
              if(!strnicmp(RegNames[i], oldptr, len))
                break;

            if(i == MAXREG)
            {
              DoError(ERR_EXPECTED_REGISTER_NAME, linenum);
             break;
            }
            else if(i == REG_A6)
              ap.Flags |= AMIPRAGFLAG_A6USE;
            else if(i == REG_A5)
              ap.Flags |= AMIPRAGFLAG_A5USE;
            else if(i == REG_A4)
              ap.Flags |= AMIPRAGFLAG_A4USE;
            else if(i == REG_D7)
              ap.Flags |= AMIPRAGFLAG_D7USE;
            else if(i == REG_A7)
            {
              DoError(ERR_A7_NOT_ALLOWED, linenum);
              break;
            }
            else if(i >= REG_FP0)
              ap.Flags |= AMIPRAGFLAG_FLOATARG;

            ap.Args[ap.NumArgs].ArgReg = i;

            for(i = 0; i < ap.NumArgs; i++)
            {
              if(ap.Args[ap.NumArgs].ArgReg == ap.Args[i].ArgReg)
              {
                DoError(ERR_REGISTER_USED_TWICE, linenum);
                break;
              }
            }
            if(i < ap.NumArgs)
              break;

            ++ap.NumArgs;

            in.pos = SkipBlanks(in.pos);
            if(*in.pos != ',' && *in.pos != '/' && *in.pos != /*(*/')')
            {
              DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
              break;
            }
          } while(*in.pos != /*(*/')');

          if(*in.pos != /*(*/')')
          {
            while(*(in.pos++))
              ++in.pos;
            continue;
          }
          else
            ++in.pos;
        }
        else
        {
          while(*in.pos && *in.pos != /*(*/')')
           ++in.pos;
          if(*in.pos != /*(*/')')
          {
            DoError(ERR_EXPECTED_CLOSE_BRACKET, linenum);
            ++in.pos;
            continue;
          }
          ++in.pos;
          ap.NumArgs = ap.CallArgs;

          ap.Flags |= AMIPRAGFLAG_PPC;
          if(abi == ABI_PPC0)
            ap.Flags |= AMIPRAGFLAG_PPC0;
          else if(abi == ABI_PPC2)
            ap.Flags |= AMIPRAGFLAG_PPC2;
        }
      }
      else
        DoError(ERR_EXPECTED_OPEN_BRACKET, linenum);

      ap2 = (struct AmiPragma *)(AmiPragma.Last);
      if(shadowmode)
      {
        if(ap2->TagName && !(ap2->Flags & AMIPRAGFLAG_OWNTAGFUNC))
        {
          struct Pragma_AliasName *p;
          DoError(ERR_DOUBLE_VARARGS, linenum);
          
          if((p = (struct Pragma_AliasName *)
          AllocListMem(sizeof(struct Pragma_AliasName))))
          {
            p->FunctionName = ap2->TagName;
            p->AliasName = ap.FuncName;
            p->Type = FUNCFLAG_TAG;
            AddAliasName(ap2, p, linenum);
          }
          else
            return 0;
#ifdef DEBUG_OLD
  printf("ScanFDFile: StormFD mode, tag func alias: %s\n", ap2->TagName);
#endif
        }
        else
        {
#ifdef DEBUG_OLD
  printf("ScanFDFile: StormFD mode, tag func: %s\n", ap2->TagName);
#endif
          ap2->Flags &= ~(AMIPRAGFLAG_OWNTAGFUNC);
          ap2->TagName = ap.FuncName;
          ++tagfuncs;
        }
        if(ap.NumArgs != ap2->NumArgs)
          DoError(ERR_VARARGS_ARGUMENTS_DIFFER, linenum);
        else if(abi == ABI_M68K)
        {
          uint32 i;

          for(i = 0; i < ap2->NumArgs; ++i)
          {
            if(ap2->Args[i].ArgReg != ap.Args[i].ArgReg)
            {
              DoError(ERR_VARARGS_ARGUMENTS_DIFFER, linenum);
              break;
            }
          }
        }
      }
      /* handle them as alias instead seperate */
      else if(ap2 && ap2->Bias == ap.Bias && ap2->NumArgs == ap.NumArgs)
      {
        struct Pragma_AliasName *p;

        if((p = (struct Pragma_AliasName *)
        AllocListMem(sizeof(struct Pragma_AliasName))))
        {
          p->FunctionName = ap2->TagName;
          p->AliasName = ap.FuncName;
          p->Type = FUNCFLAG_NORMAL;
          AddAliasName(ap2, p, linenum);
        }
        if(abi == ABI_M68K)
        {
          uint32 i;

          for(i = 0; i < ap2->NumArgs; ++i)
          {
            if(ap2->Args[i].ArgReg != ap.Args[i].ArgReg)
            {
              DoError(ERR_VARARGS_ARGUMENTS_DIFFER, linenum);
	      break;
            }
          }
        }
      }
      else
      {
        if(ap.Flags & AMIPRAGFLAG_VARARGS)
        {
          ap.TagName = ap.FuncName;
          ap.FuncName = 0;
        }
        else if((_public || (Flags & FLAG_PRIVATE)) &&
        !MakeTagFunction(&ap))
          return 0;
        else      /* check the alias names */
        {
          uint32 i = 0;

          while(Pragma_AliasNames[i].FunctionName)
          {
            if(!strcmp(ap.FuncName, Pragma_AliasNames[i].FunctionName) ||
            (ap.TagName && !strcmp(ap.TagName,
            Pragma_AliasNames[i].FunctionName)))
            {
              AddAliasName(&ap, (struct Pragma_AliasName *)
              &Pragma_AliasNames[i], linenum);
            }
            ++i;
          }
        }

        if(abi == ABI_M68K)
        {
          if(ap.CallArgs != ap.NumArgs)
            ap.Flags |= AMIPRAGFLAG_ARGCOUNT;

          ap.Flags |= AMIPRAGFLAG_M68K;

          if((Flags & FLAG_NOFPU) && (ap.Flags & AMIPRAGFLAG_FLOATARG))
            DoError(ERR_FLOATARG_NOT_ALLOWED, linenum);
          else if(((ap.Flags & AMIPRAGFLAG_FLOATARG) || !(Flags & FLAG_FPUONLY))
          && !(Flags & FLAG_PPCONLY))
          { /* skip all without FPU when FPUONLY and PPC when PPCONLY */
            struct AmiPragma *d;
            if(!(d = (struct AmiPragma *) NewItem(&AmiPragma)))
              return 0;
            memcpy(d, &ap, sizeof(struct AmiPragma));
            if(!CheckNames(d))
              return 0;
            AddItem(&AmiPragma, (struct ShortList *) d);
            if(!SpecialFuncs())
              return 0;
          }
        }
        else
        {
          if(!(Flags & FLAG_NOPPC))
          {
            struct AmiPragma *d;
            if(!(d = (struct AmiPragma *) NewItem(&AmiPragma)))
              return 0;
            memcpy(d, &ap, sizeof(struct AmiPragma));
            if(!CheckNames(d))
              return 0;
            AddItem(&AmiPragma, (struct ShortList *) d);
/*
            if(!SpecialFuncs())
              return 0;
*/
          }
        }
      }
      shadowmode = 0;
    }

    in.pos = SkipBlanks(in.pos);
    if(*in.pos)
      DoError(ERR_EXTRA_CHARACTERS, linenum);
    ++in.pos; /* skip '\0' */
  }

  if(bias)
    DoError(ERR_MISSING_END, 0);

  return 1;
}

static int32 ScanTypes(strptr ptr, uint32 size)
{
  struct CPP_ExternNames *a = 0, *b = 0;
  strptr endptr = ptr+size;
  int32 line;

  for(line = 1; ptr < endptr; ++line)
  {
    struct CPP_ExternNames *n;

    if(*ptr == '*') /* skip comments */
    {
      while(ptr < endptr && *(ptr++) != '\n')
        ;
    }
    else if((n = (struct CPP_ExternNames *)
      AllocListMem(sizeof(struct CPP_ExternNames))))
    {
      strptr wptr;

      n->Type = ptr; /* store start */

      while(ptr < endptr && *ptr != ':' && *ptr != '\n' && *ptr != '\t'
      && *ptr != ' ')
        ++ptr;
      wptr = SkipBlanks(ptr);
      if(*(wptr++) != ':')
        return line;
      *ptr = 0;

      n->NameType.StructureName = n->Type;
      n->NameType.StructureLength = ptr-n->Type; /* for struct ! types */

      if(!GetCPPType(&n->NameType, (ptr = SkipBlanks(wptr)), 0, 1))
        return line;
#ifdef DEBUG_OLD
  printf("'%20s', slen %2d, typelen %3d, pntd %d, type %c, sn '%.3s'\n",
  n->Type, n->NameType.StructureLength, n->NameType.FullLength,
  n->NameType.PointerDepth, n->NameType.Type ? n->NameType.Type : 's',
  n->NameType.StructureName ? n->NameType.StructureName : "<e>");
#endif
      ptr = SkipBlanks(n->NameType.TypeStart+n->NameType.FullLength);
      if(*(ptr++) != '\n')
      {
#ifdef DEBUG_OLD
  printf("%.30s\n", ptr);
#endif
        return line;
      }

      if(!a)
        b = n;
      else
        a->Next = n;
      a = n;
    }
    else
      return -1;
  }
  extnames = b; /* now store the list */
  return 0;
}

static void FindHeader(void)
{
  strptr str = HEADER;
  uint32 mode = 0;

  do
  {
    if(!mode)
      HEADER = str;

    if(*str == '/')
    {
      ++str;
      if(*str == '*')
      {
        mode = 2; break;
      }
      else if(*str == '/')
        mode = 1;
    }
    else if(*str == '*' || *str == ';')
      mode = 1;
    else if(*str == '{'/*}*/)
    {
      mode = 3; break;
    }
    else if(*str == '('/*)*/ && *(++str) == '*')
    {
      mode = 4; break;
    }
    else if(mode)
      break;
    while(*str && *(str++) != '\n')
      ;
  } while(*str);

  if(mode == 2)
  {
    while(*str && (*(str-1) != '*' || *str != '/'))
      ++str;
    while(*str && *(str++) != '\n')
      ;
  }
  else if(mode == 3)
  {
    while(*str && *str != /*{*/'}')
      ++str;
    while(*str && *(str++) != '\n')
      ;
  }
  else if(mode == 4)
  {
    while(*str && (*(str-1) != '*' || *str != /*(*/')'))
      ++str;
    while(*str && *(str++) != '\n')
      ;
  }

  if(mode)
    headersize = str-HEADER;
  else
  {
    HEADER = 0; headersize = 0;
  }
}

/* returns decrement data in bits 0-15 and increment data in bits 16-31 */
static uint32 GetRegisterData(struct AmiPragma *ap)
{
/* usage of result:
    48E7 <lower word>    MOVEM.L <registers>,-(A7) ; D0 is bit 15
    4CDF <upper word>    MOVEM.L (A7)+,<registers> ; D0 is bit 0
*/
  register uint32 i, data = 0, reg;

  for(i = 0; i < ap->NumArgs; ++i)
  {
    if((reg = ap->Args[i].ArgReg) <= REG_FP0)
    {
      if(reg >= 10 || (reg >= 2 &&  reg <= 7)) /* A2-A7 and D2-D7 */
        data |= (1 << (reg + 16)) + (1 << (15 - reg));
    }
  }
  if(data)      /* set A6 only when other register used */
    data |= 0x40000002;
  return data;
}

static uint16 GetFRegisterData(struct AmiPragma *ap)
{
/* usage of result:
    F227 <upper byte>    FMOVEM.X <registers>,-(A7) ; FP0 is bit 0
    F21F <lower byte>    FMOVEM.X (A7)+,<registers> ; FP0 is bit 7
*/
  register uint32 i, reg;
  register uint16 data = 0;

  for(i = 0; i < ap->NumArgs; ++i)
  {
    if((reg = ap->Args[i].ArgReg) >= REG_FP2)
    {
      reg -= REG_FP0;
      data |= (1 << (reg + 8)) + (1 << (7 - reg));
    }
  }
  return data;
}

static uint32 OutputXDEF(uint32 offset, strptr format, ...)
{
  uint8 buf[150];
  va_list a;
  size_t i;

  va_start(a, format);
  i = vsprintf((strptr)(buf+4), format, a);
  va_end(a);
  while(i&3)
    buf[4+i++] = 0;
  EndPutM32(buf+4+i, offset); /* the definition offset */

  EndPutM32(buf, (EXT_DEF<<24) + (i>>2));

  return DoOutputDirect(buf, i+8);
}

static uint32 OutputXREF(uint32 offset, uint32 type, strptr format, ...)
{
  uint8 buf[150];
  va_list a;
  size_t i;

  va_start(a, format);
  i = vsprintf((strptr)(buf+4), format, a);
  va_end(a);
  while(i&3)
    buf[4+i++] = 0;
  EndPutM32(buf+4+i, 1); /* 1 reference */
  EndPutM32(buf+8+i, offset); /* the definition offset */

  EndPutM32(buf, (type << 24) + (i>>2));

  return DoOutputDirect(buf, i+12);
}

static uint32 OutputXREF2(uint32 offset1, uint32 offset2, uint32 type,
strptr format, ...)
{
  uint8 buf[150];
  va_list a;
  size_t i;

  va_start(a, format);
  i = vsprintf((strptr)(buf+4), format, a);
  va_end(a);
  while(i&3)
    buf[4+i++] = 0;
  EndPutM32(buf+4+i, 2); /* 2 references */
  EndPutM32(buf+8+i, offset1); /* the definition offset */
  EndPutM32(buf+12+i, offset2); /* the definition offset */

  EndPutM32(buf, (type << 24) + (i>>2));

  return DoOutputDirect(buf, i+16);
}

static uint32 OutputSYMBOL(uint32 offset, strptr format, ...)
{
  va_list a;
  uint8 buf[150];
  size_t i;

  va_start(a, format);
  i = vsprintf((strptr)(buf+4), format, a);
  va_end(a);
  while(i&3)
    buf[4+i++] = 0;
  EndPutM32(buf+4+i, offset);

  EndPutM32(buf, (0 << 24) + (i>>2));

  return DoOutputDirect(buf, i+8);
}

static uint8 *AsmStackCopy(uint8 *data, struct AmiPragma *ap, uint32 flags,
uint32 ofs)
{
  uint32 i, j, k, l, tofs;

  if(Flags & FLAG_PASCAL)
  {
    k = ap->NumArgs;

    while(k)
    {
      if(ap->Args[k-1].ArgReg >= REG_FP0)
      {
        struct ClibData *cd; 

        cd = GetClibFunc(ap->FuncName, ap, flags);
        EndPutM16Inc(data, 0xF22F);     /* FMOVE.? offs(A7),FPx */

        if(cd && IsCPPType(&cd->Args[k-1], CPP_TYPE_DOUBLE))
        {
          EndPutM16Inc(data, 0x5400 + ((ap->Args[--k].ArgReg-REG_FP0)<<7));
          EndPutM16Inc(data, ofs<<2); /* one double needs two longs */
          ofs += 2;
        }
        else
        {
          if(!cd || !IsCPPType(&cd->Args[k-1], CPP_TYPE_FLOAT))
            DoError(ERR_LONG_DOUBLE, ap->Line);
          EndPutM16Inc(data, 0x4400 + ((ap->Args[--k].ArgReg-REG_FP0)<<7));
          EndPutM16Inc(data, (ofs++) << 2);
        }
      }
      else if((k >= 2) && (ap->Args[k-1].ArgReg < ap->Args[k-2].ArgReg)
      && ap->Args[k-2].ArgReg < REG_FP0 && !(Flags & FLAG_NOMOVEM))
      {
        l = 0; tofs = ofs;
        do
        {
          j = ap->Args[--k].ArgReg;

          ++ofs;
          l |= 1 << j;
        } while(k && j < ap->Args[k-1].ArgReg
        && ap->Args[k-1].ArgReg < REG_FP0);
        EndPutM16Inc(data, 0x4CEF);     /* MOVEM.L offs(A7),xxx */
        EndPutM16Inc(data, l);
        EndPutM16Inc(data, tofs << 2);   /* store start offset */
      }
      else
      {
        l = 0x202F;                     /* MOVE.L offs(A7),xxx */

        if((j = ap->Args[--k].ArgReg) > 7)
        {
          l |= (1<<6); j -= 8;          /* set MOVEA bit */
        }
        /* set destination register and store */
        EndPutM16Inc(data, l | (j << 9));
        EndPutM16Inc(data, (ofs++) << 2);
      }
    }
  }
  else
  {
    i = 0;

    k = ap->NumArgs - ((flags & FUNCFLAG_TAG) ? 1 : 0);

    while(i < k)
    {
      if(ap->Args[i].ArgReg >= REG_FP0)
      {
        struct ClibData *cd; 

        cd = GetClibFunc(ap->FuncName, ap, flags);
        EndPutM16Inc(data, 0xF22F);     /* FMOVE.? offs(A7),FPx */

        if(cd && IsCPPType(&cd->Args[i], CPP_TYPE_DOUBLE))
        {
          EndPutM16Inc(data, 0x5400 + ((ap->Args[i++].ArgReg-REG_FP0)<<7));
          EndPutM16Inc(data, ofs<<2); /* one double needs two longs */
          ofs += 2;
        }
        else
        {
          if(!cd || !IsCPPType(&cd->Args[i], CPP_TYPE_FLOAT))
            DoError(ERR_LONG_DOUBLE, ap->Line);
          EndPutM16Inc(data, 0x4400 + ((ap->Args[i++].ArgReg-REG_FP0)<<7));
          EndPutM16Inc(data, (ofs++) << 2);
        }
      }
      else if(((k - i) >= 2) && (ap->Args[i].ArgReg < ap->Args[i+1].ArgReg)
      && ap->Args[i+1].ArgReg < REG_FP0 && !(Flags & FLAG_NOMOVEM))
      {
        l = 0; tofs = ofs;
        do
        {
          j = ap->Args[i++].ArgReg;

          ++ofs;
          l |= 1 << j;
        } while(i < k && j < ap->Args[i].ArgReg
        && ap->Args[i].ArgReg < REG_FP0);
        EndPutM16Inc(data, 0x4CEF);     /* MOVEM.L offs(A7),xxx */
        EndPutM16Inc(data, l);          /* Store MOVEM.L data */
        EndPutM16Inc(data, tofs << 2);  /* store start offset */
      }
      else
      {
        l = 0x202F;                     /* MOVE.L offs(A7),xxx */

        if((j = ap->Args[i++].ArgReg) > 7)
        {
          l |= (1<<6); j -= 8;          /* set MOVEA bit */
        }
        /* set destination register and store */
        EndPutM16Inc(data, l | (j << 9));
        EndPutM16Inc(data, (ofs++) << 2);
      }
    }

    if(i < ap->NumArgs)
    {
      if((j = ap->Args[i].ArgReg) > 7)
      {
        EndPutM16Inc(data, 0x41EF | ((j-8) << 9)); /* LEA xxx(A7),Ax */
        EndPutM16Inc(data, ofs << 2);
      }
      else if(ofs == 2)
      {
        EndPutM16Inc(data, 0x200F | (j << 9));  /* MOVE.L A7,Dx */
        EndPutM16Inc(data, 0x5080 | j);         /* ADDQ.L #8,Dx */
      }
      else
      {
        EndPutM16Inc(data, 0x486F);             /* PEA xxx(A7) */
        EndPutM16Inc(data, ofs << 2);
        EndPutM16Inc(data, 0x201F | j << 9);    /* MOVE.L offs(A7),Dx */
      }
    }
  }

  return data;
}
/* ------------------------------------------------------------------ */

static void DoError(uint32 errnum, uint32 line, ...)
{
  uint32 err = errnum & 0xFFFF;
  va_list a;

  if(Flags & FLAG_DIDERROR)
    return;

  if(!Errors[err].Type)
    Flags |= FLAG_DIDERROR;

  va_start(a, line);
  if (line)
  {
    printf("%s %ld in line %ld%s: ",
    (Errors[err].Type ? "Warning" : "Error"),
    err, line,
    errnum & ERROFFSET_CLIB ? " of clib file" : "");
  }
  else
  {
    printf("%s %ld : ", (Errors[err].Type ? "Warning" : "Error"), err);
  }
  vprintf(Errors[err].Error, a);
  printf("\n");
  if(line && Errors[err].Skip)
  {
    while(*in.pos)
      ++in.pos;
  }
  va_end(a);
}

static uint32 CheckError(struct AmiPragma *ap, uint32 errflags)
{
  errflags &= ap->Flags;

  if(errflags & AMIPRAGFLAG_ARGCOUNT)
  {
    if(!(ap->Flags & AMIPRAGFLAG_DIDARGWARN))
    {
      DoError(ERR_ARGUMENTNUMBER_DIFFERS_FROM_REGISTERNUMBER, ap->Line);
      ap->Flags |= AMIPRAGFLAG_DIDARGWARN;
    }
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_FLOATARG)
  {
    if(!(ap->Flags & AMIPRAGFLAG_DIDFLOATWARN))
    {
      DoError(ERR_FLOATARG_NOT_ALLOWED, ap->Line);
      ap->Flags |= AMIPRAGFLAG_DIDFLOATWARN;
    }
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_A6USE)
  {
    DoError(ERR_A6_NOT_ALLOWED, ap->Line);
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_A5USE)
  {
    DoError(ERR_A5_NOT_ALLOWED, ap->Line);
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_PPC)
  {
    if(!(Flags & FLAG_DIDPPCWARN))
    {
      DoError(ERR_PPC_FUNCTION_NOT_SUPPORTED, 0/*ap->Line*/);
      Flags |= FLAG_DIDPPCWARN;
    }
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_M68K)
  {
    if(!(Flags & FLAG_DIDM68KWARN))
    {
      DoError(ERR_M68K_FUNCTION_NOT_SUPPORTED, 0/*ap->Line*/);
      Flags |= FLAG_DIDM68KWARN;
    }
    return 1;
  }
  else if(errflags & AMIPRAGFLAG_MOSBASESYSV)
  {
    DoError(ERR_MOSBASESYSV_NOT_SUPPORTED, ap->Line);
    return 1;
  }

  return 0;
}

static uint32 DoOutput(strptr format, ...)
{
  va_list a;

  if(!Output_Error)
    return 0;

  va_start(a, format);
  if(vfprintf(outfile, format, a) < 0)
    Output_Error = 1;
  va_end(a);

  return Output_Error;
}

static uint32 DoOutputDirect(void * data, size_t size)
{
  if(!Output_Error)
    return 0;
  if(size)
  {
    if(fwrite(data, size, 1, outfile) != 1)
      Output_Error = 0;
  }
  return Output_Error;
}

/* ------------------------------------------------------------------ */

static struct ShortList *NewItem(struct ShortListRoot *list)
{
  struct ShortList *item;
  if(!list || !list->Size)
    return 0;
  if(!(item = (struct ShortList *) AllocListMem(list->Size)))
    return 0;
  return item;
}

static struct ShortList *RemoveItem(struct ShortListRoot *list,
struct ShortList *item)
{
  struct ShortList *n = list->First;

  if(n == item)
    list->First = item->Next;
  else
  {
    while(n && n->Next != item)
      n = n->Next;
    if(!n)
      return 0;
    if(!(n->Next = item->Next))
      list->Last = n;
  }
  item->Next = 0;
  return item;
}

static void AddItem(struct ShortListRoot *list, struct ShortList *item)
{
  if(!list->First)
    list->First = list->Last = item;
  else
  {
    list->Last->Next = item;
    list->Last = item;
  }
}

/* ------------------------------------------------------------------ */

uint32 FuncAMICALL(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  DoOutput("#pragma %s(%s,0x%03x,%s("/*))*/, flags & FUNCFLAG_TAG ?
  "tagcall" : "amicall", BaseName, ap->Bias, name);

  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("%s",RegNames[ap->Args[i].ArgReg]);
    if(i+1 < ap->NumArgs)
      DoOutput(",");
  }

  return DoOutput(/*((*/"))\n");
}

uint32 FuncLIBCALL(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(ap->Flags & AMIPRAGFLAG_FLOATARG)
  {
    DoOutput("#pragma flibcall %s %-22s %03x ", BaseName, name, ap->Bias);
    for(i = ap->NumArgs-1; i >= 0; --i)
      DoOutput("%02x", ap->Args[i].ArgReg);

    return DoOutput("00%02x\n", ap->NumArgs);
  }

  if((Flags & FLAG_SYSCALL) && !strcmp(BaseName,"SysBase") &&
  (flags & FUNCFLAG_NORMAL))
    DoOutput("#pragma  syscall %-22s %03x ", name, ap->Bias);
  else
    DoOutput("#pragma  %s %s %-22s %03x ", (flags & FUNCFLAG_TAG) ?
    "tagcall" : "libcall", BaseName, name, ap->Bias);

  for(i = ap->NumArgs-1; i >= 0; --i)
    DoOutput("%x", ap->Args[i].ArgReg);

  return DoOutput("0%x\n", ap->NumArgs);
}

uint32 FuncAsmText(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;
  uint32 registers;
  uint16 fregs;
  uint32 offset = 1;
  strptr c1, c2;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  c1 = Flags & FLAG_NEWSYNTAX ? "(" : ""; /*)*/
  c2 = Flags & FLAG_NEWSYNTAX ? "," : "("; /*)*/

  if(Flags & FLAG_SINGLEFILE)
  {
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("* %s\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
  }

  if(Flags & (FLAG_ASMSECTION|FLAG_SINGLEFILE))
  {
    DoOutput("\n\tSECTION\t\"%s\",CODE\n\t%sREF\t_%s\n", hunkname,
    Flags & FLAG_SMALLDATA ? "N" : "X", BaseName);
  }

  DoOutput("\n\tXDEF\t_%s\n_%s:\n",name, name);
  if(!(Flags & (FLAG_PASCAL|FLAG_ONLYCNAMES)))
  {
    DoOutput("\tXDEF\t%s\n%s:\n",name, name);
    if(clibdata)
    {
      if(!ap->NumArgs)
        DoOutput("\tXDEF\t%s_\n%s_:\n",name, name);
      else if((cd = GetClibFunc(name, ap, flags)))
      {
        string txt[300];
        uint32 i;

        for(i = 0; i < COPYCPP_PASSES; ++i)
        {
          if(CopyCPPType(txt, i, cd, ap->Args))
            DoOutput("\tXDEF\t%s__%s\n%s__%s:\n", name, txt, name, txt);
        }
      }
    }
  }

  if((registers = GetRegisterData(ap) >> 16))
  {
    if(Flags & FLAG_NOMOVEM)
    {
      for(i = 0; i <= 15; ++i)
      {
        if(registers & (1 << i))
        {
          ++offset;
          DoOutput("\tMOVE.L\t%s,-(A7)\n", RegNamesUpper[i]);
        }
      }
    }
    else
    {
      uint16 l = registers;

      DoOutput("\tMOVEM.L\t");

      for(i = 0; i <= 15; ++i)
      {
        if(l & (1 << i))
        {
          ++offset;
          l ^= 1 << i;
          DoOutput("%s",RegNamesUpper[i]);
          if(l)
            DoOutput("/");
        }
      }
      DoOutput(",-(A7)\n");
    }
  }
  else
  {
    DoOutput("\tMOVE.L\tA6,-(A7)\n"); ++offset;
  }

  if((fregs = GetFRegisterData(ap) >> 8))
  {
    uint8 l = fregs;

    DoOutput("\tFMOVEM.X\t");

    for(i = 0; i <= 7; ++i)
    {
      if(l & (1 << i))
      {
        offset += 3;
        l ^= 1 << i;
        DoOutput("%s",RegNamesUpper[REG_FP0 + i]);
        if(l)
          DoOutput("/");
      }
    }
    DoOutput(",-(A7)\n");
  }

  if(Flags & FLAG_SMALLDATA)
  {
    DoOutput(/*(*/"\tMOVEA.L\t%s_%s%sA4),A6\n", c1, BaseName, c2);
  }
  else
    DoOutput("\tMOVEA.L\t_%s,A6\n", BaseName);

  if(!(Flags & FLAG_PASCAL))
  {
    int32 k;

    k = ap->NumArgs - ((flags & FUNCFLAG_TAG) ? 1 : 0);

    for(i = 0; i < k;)
    {
      if(ap->Args[i].ArgReg >= REG_FP0)
      {
        uint32 t;
        struct ClibData *cd;

        cd = GetClibFunc(name, ap, flags);
        if(cd && IsCPPType(&cd->Args[i], CPP_TYPE_DOUBLE))
          t = CPP_TYPE_DOUBLE;
        else if(cd && IsCPPType(&cd->Args[i], CPP_TYPE_FLOAT))
          t = CPP_TYPE_FLOAT;
        else
        {
          DoError(ERR_LONG_DOUBLE, ap->Line);
          t = CPP_TYPE_FLOAT;
        }

        DoOutput(/*(*/"\tFMOVE.%c\t%s%02ld%sA7),%s\n", t == CPP_TYPE_DOUBLE
        ? 'D' : 'S', c1, offset<<2, c2, RegNamesUpper[ap->Args[i++].ArgReg]);

        if(t == CPP_TYPE_DOUBLE)
          ++offset;
        ++offset;
      }
      else if(((k - i) >= 2) && (ap->Args[i].ArgReg < ap->Args[i+1].ArgReg) &&
      ap->Args[i+1].ArgReg < REG_FP0 && !(Flags & FLAG_NOMOVEM))
      {
        DoOutput(/*(*/"\tMOVEM.L\t%s%02ld%sA7),%s", c1, (offset++)<<2, c2,
        RegNamesUpper[ap->Args[i++].ArgReg]);

        do
        {
          DoOutput("/%s", RegNamesUpper[ap->Args[i++].ArgReg]);
          ++offset;
        } while((i < k) && (ap->Args[i-1].ArgReg < ap->Args[i].ArgReg) &&
        ap->Args[i].ArgReg < REG_FP0);
        DoOutput("\n");
      }
      else
      {
        DoOutput(/*(*/"\tMOVE%s.L\t%s%02ld%sA7),%s\n",
        ap->Args[i].ArgReg >= REG_A0 ? "A" : "", c1, (offset++)<<2, c2,
        RegNamesUpper[ap->Args[i].ArgReg]);
        ++i;
      }
    }

    if(i < ap->NumArgs)
    {
      if(ap->Args[i].ArgReg > 7)
        DoOutput(/*(*/"\tLEA\t%s%02ld%sA7),%s\n", c1, offset<<2, c2,
        RegNamesUpper[ap->Args[i].ArgReg]);
      else if(offset <= 2)
        DoOutput("\tMOVE.L\tA7,%s\n\tADDQ.L\t#%02ld,%s\n",
        RegNamesUpper[ap->Args[i].ArgReg],offset<<2,
        RegNamesUpper[ap->Args[i].ArgReg]);
      else
        DoOutput(/*(*/"\tPEA\t%s%ld%sA7)\n\tMOVE.L\t(A7)+,%s\n",c1,
        offset<<2, c2,RegNamesUpper[ap->Args[i].ArgReg]);
    }
  }
  else
  {
    i = ap->NumArgs;

    while(i)
    {
      if(ap->Args[i-1].ArgReg >= REG_FP0)
      {
        uint32 t;
        struct ClibData *cd;

        cd = GetClibFunc(name, ap, flags);

        if(cd && IsCPPType(&cd->Args[i-1], CPP_TYPE_DOUBLE))
          t = CPP_TYPE_DOUBLE;
        else if(cd && IsCPPType(&cd->Args[i-1], CPP_TYPE_FLOAT))
          t = CPP_TYPE_FLOAT;
        else
        {
          DoError(ERR_LONG_DOUBLE, ap->Line);
          t = CPP_TYPE_FLOAT;
        }

        DoOutput(/*(*/"\tFMOVE.%c\t%s%02ld%sA7),%s\n", t == CPP_TYPE_DOUBLE ?
        'D' : 'S', c1, offset<<2, c2, RegNamesUpper[ap->Args[--i].ArgReg]);
        if(t == CPP_TYPE_DOUBLE)
          ++offset;
        ++offset;
      }
      else if((i >= 2) && (ap->Args[i-1].ArgReg < ap->Args[i-2].ArgReg) &&
      ap->Args[i-2].ArgReg < REG_FP0 && !(Flags & FLAG_NOMOVEM))
      {
        DoOutput(/*(*/"\tMOVEM.L\t%s%02ld%sA7),%s", c1, (offset++)<<2, c2,
        RegNamesUpper[ap->Args[--i].ArgReg]);

        do
        {
          DoOutput("/%s", RegNamesUpper[ap->Args[--i].ArgReg]);
          ++offset;
        } while(i && (ap->Args[i].ArgReg < ap->Args[i-1].ArgReg) &&
        ap->Args[i-1].ArgReg < REG_FP0);
        DoOutput("\n");
      }
      else
      {
        --i;
        DoOutput(/*(*/"\tMOVE%s.L\t%s%02ld%sA7),%s\n",
        ap->Args[i].ArgReg >= REG_A0 ? "A" : "", c1, (offset++)<<2, c2,
        RegNamesUpper[ap->Args[i].ArgReg]);
      }
    }
  }

  DoOutput(/*(*/"\tJSR\t%s-%03d%sA6)\n", c1, ap->Bias, c2);

  if(fregs)
  {
    DoOutput("\tFMOVEM.X\t(A7)+,");

    for(i = 0; i <= 7; ++i)
    {
      if(fregs & (1 << i))
      {
        fregs ^= 1 << i;
        DoOutput("%s",RegNamesUpper[REG_FP0 + i]);
        if(fregs)
          DoOutput("/");
      }
    }
    DoOutput("\n");
  }

  if(registers)
  {
    if(Flags & FLAG_NOMOVEM)
    {
      for(i = 15; i >= 0; --i)
      {
        if(registers & (1 << i))
          DoOutput("\tMOVE%s.L\t(A7)+,%s\n", i >= REG_A0 ? "A" : "",
          RegNamesUpper[i]);
      }
    }
    else
    {
      DoOutput("\tMOVEM.L\t(A7)+,");

      for(i = 0; i <= 15; ++i)
      {
        if(registers & (1 << i))
        {
          registers ^= 1 << i;
          DoOutput("%s",RegNamesUpper[i]);
          if(registers)
            DoOutput("/");
        }
      }
      DoOutput("\n");
    }
  }
  else
    DoOutput("\tMOVEA.L\t(A7)+,A6\n");

  return DoOutput("\tRTS\n");
}

uint32 FuncAsmCode(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 registers, offset = 1, baseref;
  size_t i;
  uint8 *data;
  uint16 fregs;

  if(CheckError(ap, AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  registers = GetRegisterData(ap);
  fregs = GetFRegisterData(ap);

  i = strlen(name) + 2;
  EndPutM32(tempbuf, HUNK_UNIT);
  EndPutM32(tempbuf+4, (i+3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutput("%s.o", name);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  i = strlen(hunkname);
  EndPutM32(tempbuf, HUNK_NAME);
  EndPutM32(tempbuf+4, (i + 3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutputDirect(hunkname, i);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  data = tempbuf+8; /* we need HUNK_CODE + size at start */

  if(!registers)
  {
    EndPutM16Inc(data, 0x2F0E);                 /* MOVE.L A6,-(A7) */
    ++offset;                                   /* one long more on stack */
  }
  else
  {
    if(Flags & FLAG_NOMOVEM)
    {
      for(i = 0; i <= 15; ++i)
      {
        if(registers & (1<< (16+i)))
        {
          EndPutM16Inc(data, 0x2F00 + i);       /* MOVE.L xxx,-(A7) */
          ++offset;
        }
      }
    }
    else
    {
      uint32 l;
      EndPutM16Inc(data, 0x48E7);               /* MOVEM.L xxx,-(A7) */
      EndPutM16Inc(data, registers);            /* store MOVEM.L registers */
      for(l = (uint16) registers; l; l >>= 1)
      {
        if(l & 1)
          ++offset;                             /* get offset addition */
      }
    }
  }

  if(fregs)
  {
    uint32 l;
    EndPutM16Inc(data, 0xF227);                 /* FMOVEM.X xxx,-(A7) */
    EndPutM16Inc(data, 0xE000 + ((fregs>>8)&0xFF));
    for(l = (uint8) fregs; l; l >>= 1)
    {
      if(l & 1)
        offset+=3;                              /* get offset addition */
    }
  }

  baseref = (data-tempbuf)-8+2;   /* one word later (MOVE) - 2 header longs */
  if(Flags & FLAG_SMALLDATA)
  {
    EndPutM16Inc(data, 0x2C6C);                 /* MOVEA.L base(A4),A6 */
    EndPutM16Inc(data, 0);                      /* place for base reference */
  }
  else
  {
    EndPutM16Inc(data, 0x2C79);                 /* MOVEA.L base,A6 */
    EndPutM32Inc(data, 0);                      /* place for base reference */
  }

  data = AsmStackCopy(data, ap, flags, offset);

  /* here comes the base reference */
  EndPutM16Inc(data, 0x4EAE);                   /* JSR xxx(A6) */
  EndPutM16Inc(data, -ap->Bias);                /* JSR offset */

  if(fregs)
  {
    EndPutM16Inc(data, 0xF21F);                 /* FMOVEM.X (A7)+,xxx */
    EndPutM16Inc(data, 0xD000 + (fregs&0xFF));
  }

  if(registers)
  {
    if(Flags & FLAG_NOMOVEM)
    {
      int32 i;
      for(i = 15; i >= 0; --i)
      {
        if(registers & (1<<(16+i)))             /* MOVE.L (A7)+,xxx */
          EndPutM16Inc(data, 0x201F + ((i&7)<<9) + ((i>>3)<<6));
      }
    }
    else
    {
      EndPutM16Inc(data, 0x4CDF);               /* MOVEM.L (A7)+,xxx */
      EndPutM16Inc(data, (registers >> 16));    /* store MOVEM.L registers */
    }
  }
  else
    EndPutM16Inc(data, 0x2C5F);                 /* MOVE.L (A7)+,A6 */
  EndPutM16Inc(data, 0x4E75);                   /* RTS */

  EndPutM16Inc(data, 0);            /* get longword assignment if not yet */

  EndPutM32(tempbuf, HUNK_CODE);
  EndPutM32(tempbuf+4, (data-tempbuf-8)>>2)
  DoOutputDirect(tempbuf, (size_t)(data-tempbuf)&(~3));

  EndPutM32(tempbuf, HUNK_EXT);
  DoOutputDirect(tempbuf, 4);

  OutputXREF(baseref, (Flags & FLAG_SMALLDATA ? EXT_DEXT16 : EXT_REF32),
  "_%s", BaseName);
  /* here come the XDEF name references */
  OutputXDEF(0, "_%s", name);                      /* C name */

  if(!(Flags & (FLAG_PASCAL|FLAG_ONLYCNAMES)))
  {
    struct ClibData *cd;
    OutputXDEF(0, "%s", name);                     /* ASM name */

    if(clibdata)
    {
      if(!ap->NumArgs)
        OutputXDEF(0, "%s_", name);                /* C++ name no parameters */
      else if((cd = GetClibFunc(name, ap, flags)))
      {
        for(i = 0; i < COPYCPP_PASSES; ++i) /* C++ name with parameters */
        {
          if(CopyCPPType((strptr)tempbuf, i, cd, ap->Args))
            OutputXDEF(0, "%s__%s", name, tempbuf);
        }
      }
    }
  }

  EndPutM32(tempbuf, 0);
  DoOutputDirect(tempbuf, 4);
  if(!(Flags & FLAG_NOSYMBOL))
  {
    EndPutM32(tempbuf, HUNK_SYMBOL);
    DoOutputDirect(tempbuf, 4);
    OutputSYMBOL(0, "_%s", name);              /* C name */

    if(!(Flags & (FLAG_PASCAL|FLAG_ONLYCNAMES)))
    {
      struct ClibData *cd;
      OutputSYMBOL(0, "%s", name);             /* ASM name */

      if(clibdata)
      {
        if(!ap->NumArgs)
          OutputSYMBOL(0, "%s_", name);        /* C++ name no parameters */
        else if((cd = GetClibFunc(name, ap, flags)))
        {
          for(i = 0; i < COPYCPP_PASSES; ++i) /* C++ name with parameters */
          {
            if(CopyCPPType((strptr) data, i, cd, ap->Args))
              OutputSYMBOL(0, "%s__%s", name, (strptr) data);
          }
        }
      }
    }

    EndPutM32(tempbuf, 0);
    DoOutputDirect(tempbuf, 4);
  }

  EndPutM32(tempbuf, HUNK_END);
  return DoOutputDirect(tempbuf, 4);
}

/* Directly called by FuncInline and FuncInlineDirect also! */
uint32 FuncCSTUBS(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *f, *t;
  strptr ret = "return ";
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(!(f = GetClibFunc(ap->FuncName, ap, 0)))
    return 1;
  t = GetClibFunc(name, ap, flags);

  if(flags & FUNCFLAG_EXTENDMODE)
  {
    sprintf(tempbuf, "___%s", name);
    name = tempbuf;
  }

  if(IsCPPType(&f->ReturnType, CPP_TYPE_VOID))
    ret = "";

  if(!OutClibType(&f->ReturnType, name) || !DoOutput("("/*)*/))
    return 0;
  if(flags & FUNCFLAG_EXTENDMODE)
  {
    DoOutput("%s %s, ", GetBaseType(), BaseName);
  }

  for(i = 0; i < ap->NumArgs-1; i++)
  {
    if(!OutClibType(&f->Args[i], ap->Args[i].ArgName) || !DoOutput(", "))
      return 0;
  }
  if(t && t->Args[i].Type != CPP_TYPE_VARARGS)
  {
    if(!OutClibType(&t->Args[i], ap->Args[i].ArgName) || !DoOutput(", "))
      return 0;
  }
  else if(ap->NumArgs == 1 && !DoOutput("ULONG tag, "))
    return 0;

  if(!DoOutput(/*(*/"...)\n{\n  %s%s("/*)*/, ret, ap->FuncName))
    return 0;
  for(i = 0; i < ap->NumArgs-1; i++)
  {
    if(!DoOutput("%s, ", ap->Args[i].ArgName))
      return 0;
  }
  if(!DoOutput("("/*)*/) || !OutClibType(&f->Args[ap->NumArgs-1],0))
    return 0;

  if(t && t->Args[i].Type != CPP_TYPE_VARARGS)
  {
    if(!DoOutput(/*((*/") &%s);\n}\n\n", ap->Args[ap->NumArgs-1].ArgName))
      return 0;
  }
  else if(ap->NumArgs == 1)
  {
    if(!DoOutput(/*((*/") &tag);\n}\n\n"))
      return 0;
  }
  else if (!DoOutput(/*(*/") ((ULONG) &%s + sizeof("/*))*/,
  ap->Args[ap->NumArgs-2].ArgName) || !OutClibType(&f->Args[ap->NumArgs-2],0)
  || !DoOutput(/*(((*/")));\n}\n\n"))
    return 0;
  return 1;
}

uint32 FuncLVOXDEF(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */
  return DoOutput("\t\tXDEF\t_LVO%s\n", name);
}

uint32 FuncLVO(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */
  return DoOutput("\n_LVO%-24s\tEQU\t-%d", name, ap->Bias);
}

uint32 FuncLVOPPCXDEF(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */
  return DoOutput("\t.globl\t%sLVO%s\n", Flags & FLAG_ABIV4 ? "" : "_", name);
}

uint32 FuncLVOPPC(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */
  return DoOutput(".set\t%sLVO%s,-%d\n", Flags & FLAG_ABIV4 ? "" : "_", name,
  ap->Bias);
}

uint32 FuncLVOPPCBias(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */

  EndPutM32Inc(elfbufpos, symoffset);                    /* st_name */
  symoffset += strlen(name) + 3 + (Flags & FLAG_ABIV4 ? 0 : 1) + 1;
  EndPutM32Inc(elfbufpos, -ap->Bias);                    /* st_value */
  EndPutM32Inc(elfbufpos, 0);                            /* st_size */
  *(elfbufpos++) = ELF32_ST_INFO(STB_GLOBAL,STT_NOTYPE); /* st_info */
  *(elfbufpos++) = 0;                                    /* st_other */
  EndPutM16Inc(elfbufpos, SHN_ABS);                      /* st_shndx */

  return 1;
}

uint32 FuncLVOPPCName(struct AmiPragma *ap, uint32 flags, strptr name)
{
  Flags |= FLAG_DONE; /* We did something */
  DoOutput("%sLVO%s", Flags & FLAG_ABIV4 ? "" : "_", name);
  return DoOutputDirect("", 1);
}

uint32 FuncLVOLib(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 j;

  Flags |= FLAG_DONE; /* We did something */
  j = strlen(name) + 3 + (Flags & FLAG_ABIV4 ? 0 : 1);
  EndPutM32(tempbuf, (EXT_ABS << 24) + ((j+3)>>2));

  DoOutputDirect(tempbuf, 4);
  DoOutput("%sLVO%s", Flags & FLAG_ABIV4 ? "" : "_", name);
  DoOutputDirect("\0\0\0", ((j+3)&(~3))-j);
  EndPutM32(tempbuf, -ap->Bias);
  return DoOutputDirect(tempbuf, 4);
}

uint32 FuncLocCode(struct AmiPragma *ap, uint32 flags, strptr name)
{
  strptr str2 = Flags & FLAG_LOCALREG ? "rE" : "";
  uint8 *data;
  int32 i;
  struct ClibData *cd = 0;

  if(CheckError(ap, AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  i = strlen(name) + 2;
  EndPutM32(tempbuf, HUNK_UNIT);
  EndPutM32(tempbuf+4, (i+3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutput("%s.o", name);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  i = strlen(hunkname);
  EndPutM32(tempbuf, HUNK_NAME);
  EndPutM32(tempbuf+4, (i + 3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutputDirect(hunkname, i);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  data = tempbuf+8; /* we need HUNK_CODE + size at start */

  if(Flags & FLAG_LOCALREG)
  {
    if((flags & FUNCFLAG_TAG))
    {
      i = ap->Args[ap->NumArgs-1].ArgReg;
      EndPutM16Inc(data, 0x2F00 + i);                   /* MOVE <ea>,-(A7) */

      if(i > 7)
      {
        EndPutM16Inc(data, 0x41EF | ((i-8) << 9));      /* LEA 8(A7),Ax */
        EndPutM16Inc(data, 8);
      }
      else
      {
        EndPutM16Inc(data, 0x200F | (i << 9));          /* MOVE.L A7,Dx */
        EndPutM16Inc(data, 0x5080 | i);                 /* ADDQ.L #8,Dx */
      }

      EndPutM16Inc(data, 0x4EAE);
      EndPutM16Inc(data, -ap->Bias);                    /* JSR instruction */

      /* MOVE (A7)+,<ea> */
      EndPutM16Inc(data, 0x201F + ((i&7)<<9) + ((i>>3)<<6));
      EndPutM16Inc(data, 0x4E75);                       /* RTS */
    }
    else
    {
      EndPutM16Inc(data, 0x4EEE);
      EndPutM16Inc(data, -ap->Bias);                    /* JMP instruction */
    }
  }
  else
  {
    uint32 registers, offset = 1;

    registers = GetRegisterData(ap);

    if(!registers) /* happens only when !(ap->Flags & AMIPRAG_A6USE) */
    {
      EndPutM16Inc(data, 0x2F0E);                       /* MOVE.L A6,-(A7) */
      ++offset;                                  /* one long more on stack */
    }
    else
    {
      if(Flags & FLAG_NOMOVEM)
      {
        for(i = 0; i <= 15; ++i)
        {
          if(registers & (1<< (16+i)))
          {
            EndPutM16Inc(data, 0x2F00 + i);             /* MOVE.L xxx,-(A7) */
            ++offset;
          }
        }
      }
      else
      {
        EndPutM16Inc(data, 0x48E7);                     /* MOVEM.L xxx,-(A7) */
        EndPutM16Inc(data, registers);            /* store MOVEM.L registers */
        for(i = registers&0xFFFF; i; i >>= 1)
        {
          if(i & 1)
            ++offset;                                 /* get offset addition */
        }
      }
    }

    if(!(ap->Flags & AMIPRAGFLAG_A6USE)) /* store library base in A6 */
    {
      EndPutM16Inc(data, 0x2C6F);                       /* MOVE.L ofs(A7),A6 */
      EndPutM16Inc(data, (offset++) << 2);
    }

    data = AsmStackCopy(data, ap, flags, offset);

    /* here comes the base reference */
    EndPutM16Inc(data, 0x4EAE);                         /* JSR xxx(A6) */
    EndPutM16Inc(data, -ap->Bias);                      /* JSR offset */
    if(registers)
    {
      if(Flags & FLAG_NOMOVEM)
      {
        for(i = 15; i >= 0; --i)
        {
          if(registers & (1<<(16+i)))                   /* MOVE.L (A7)+,xxx */
            EndPutM16Inc(data, 0x201F + ((i&7)<<9) + ((i>>3)<<6));
        }
      }
      else
      {
        EndPutM16Inc(data, 0x4CDF);                     /* MOVEM.L (A7)+,xxx */
        EndPutM16Inc(data, registers >> 16);      /* store MOVEM.L registers */
      }
    }
    else
      EndPutM16Inc(data, 0x2C5F);                       /* MOVE.L (A7)+,A6 */

    EndPutM16Inc(data, 0x4E75);                         /* RTS */
  }

  EndPutM16Inc(data, 0);             /* get longword assignment if not yet */

  EndPutM32(tempbuf, HUNK_CODE);
  EndPutM32(tempbuf+4, (data-tempbuf-8)>>2)
  DoOutputDirect(tempbuf, (data-tempbuf)&(~3));

  EndPutM32(tempbuf, HUNK_EXT);
  DoOutputDirect(tempbuf,4);

  /* here come the XDEF name references */

  if(!(Flags & FLAG_ONLYCNAMES))
  {
    OutputXDEF(0, "%s", name);             /* ASM names */
    OutputXDEF(0, "LOC_%s", name);
  }

  OutputXDEF(0, "_%s", name);              /* C names */
  OutputXDEF(0, "_LOC_%s", name);

  if(clibdata && !(Flags & FLAG_ONLYCNAMES))
  {
    if(!ap->NumArgs)
    {
      /* C++ names no parameters */
      OutputXDEF(0, "%s__%sP07Library", name, str2);
      OutputXDEF(0, "LOC_%s__%sP07Library", name, str2);
    }
    else if((cd = GetClibFunc(name, ap, flags)))
    {
      strptr txt;

      txt = (strptr) tempbuf;
  
      for(i = 0; i < COPYCPP_PASSES; ++i)
      {
        if(CopyCPPType(txt, i, cd, ap->Args))
        {                                 /* C++ names with parameters */
          if(!(ap->Flags & AMIPRAGFLAG_A6USE))
          {
            OutputXDEF(0, "%s__%sP07Library%s", name, str2, txt);
            OutputXDEF(0, "LOC_%s__%sP07Library%s", name, str2, txt);
          }
          else
          {
            OutputXDEF(0, "%s__%s", name, txt);
            OutputXDEF(0, "LOC_%s__%s", name, txt);
          }
        }
      }
    }
  }

  EndPutM32(tempbuf, 0);
  DoOutputDirect(tempbuf,4);
  if(!(Flags & FLAG_NOSYMBOL))
  {
    EndPutM32(tempbuf, HUNK_SYMBOL);
    DoOutputDirect(tempbuf,4);
    if(!(Flags & FLAG_ONLYCNAMES))
    {
      OutputSYMBOL(0, "%s", name);             /* ASM names */
      OutputSYMBOL(0, "LOC_%s", name);
    }

    OutputSYMBOL(0, "_%s", name);              /* C names */
    OutputSYMBOL(0, "_LOC_%s", name);

    if(clibdata && !(Flags & FLAG_ONLYCNAMES))
    {
      if(!ap->NumArgs)
      {
        /* C++ names no parameters */
        OutputSYMBOL(0, "%s__%sP07Library", name, str2);
        OutputSYMBOL(0, "LOC_%s__%sP07Library", name, str2);
      }
      else if(cd)
      {
        strptr txt;

        txt = (strptr) tempbuf;
  
        for(i = 0; i < COPYCPP_PASSES; ++i)
        {
          if(CopyCPPType(txt, i, cd, ap->Args))
          {                                 /* C++ names with parameters */
            if(!(ap->Flags & AMIPRAGFLAG_A6USE))
            {
              OutputSYMBOL(0, "%s__%sP07Library%s", name, str2, txt);
              OutputSYMBOL(0, "LOC_%s__%sP07Library%s", name, str2, txt);
            }
            else
            {
              OutputSYMBOL(0, "%s__%s", name, txt);
              OutputSYMBOL(0, "LOC_%s__%s", name, txt);
            }
          }
        }
      }
    }

    EndPutM32(tempbuf, 0);
    DoOutputDirect(tempbuf,4);
  }
  EndPutM32(tempbuf, HUNK_END);
  return DoOutputDirect(tempbuf,4);
}

uint32 FuncLocText(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  OutClibType(&cd->ReturnType, 0);
  DoOutput(" LOC_%s("/*)*/, name);
  if(!(ap->Flags & AMIPRAGFLAG_A6USE))
  {
    if(Flags & FLAG_LOCALREG)
      DoOutput("register __a6 ");
    DoOutput("%s libbase", GetBaseType());
    if(ap->NumArgs)
      DoOutput(", ");
  }

  if(ap->NumArgs)
  {
    for(i = 0; i < ap->NumArgs-1; i++)
    {
      if(((Flags & FLAG_LOCALREG &&
      !DoOutput("register __%s ", RegNames[ap->Args[i].ArgReg]))) ||
      !OutClibType(&cd->Args[i], ap->Args[i].ArgName) || !DoOutput(", "))
        return 0;
    }

    if(flags & FUNCFLAG_NORMAL)
    {
      if(((Flags & FLAG_LOCALREG &&
      !DoOutput("register __%s ", RegNames[ap->Args[i].ArgReg]))) ||
      !OutClibType(&cd->Args[i], ap->Args[i].ArgName) ||
      !DoOutput(/*(*/");\n"))
        return 0;
      if(BaseName)
      {
        DoOutput("#define %s("/*)*/, name);
        for(i = 0; i < ap->NumArgs-1; ++i)
          DoOutput("%c, ", 'a'+(char)i);
        DoOutput(/*(*/"%c) LOC_%s(%s, "/*)*/,'a'+(char)i, name, BaseName);
        for(i = 0; i < ap->NumArgs-1; ++i)
          DoOutput("%c, ",'a'+(char)i);
        return DoOutput(/*(*/"%c)\n\n",'a'+(char)i);
      }
    }
    else
      return DoOutput(/*(*/"...);\n");
  }
  else if(BaseName)
    return DoOutput(/*(*/");\n#define %s(a) LOC_%s(a)\n\n",
    name, name);
  else
    return DoOutput(/*(*/");\n");
  return 1;
}

uint32 FuncInlineDirect(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 a4 = 0, a5 = 0;
  int32 noret = 0;
  int32 i, maxargs, reg=0;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC|AMIPRAGFLAG_VARARGS))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_ALIAS)
  {
    if(flags & FUNCFLAG_TAG)
      return DoOutput("#ifndef NO_INLINE_STDARG\n#define %s %s\n#endif\n\n",
      name, ap->TagName);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) %s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"(%s))\n\n", ap->Args[i].ArgName);
  }

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(flags & FUNCFLAG_TAG)
  {
    /* do not create some strange functions */
    if(IsNoCreateInlineFunc(name))
      DoOutput("#if !defined(NO_INLINE_STDARG) "
      "&& defined(SPECIALMACRO_INLINE_STDARG)\n");
    else
      DoOutput("#ifndef NO_INLINE_STDARG\n");
    DoOutput("static __inline__ ");
    FuncCSTUBS(ap, flags|FUNCFLAG_EXTENDMODE, name);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
    {
      DoOutput("%s%s", ap->Args[i].ArgName, i < ap->NumArgs-2 ? ", " : "");
    }
    if(ap->NumArgs < 2)
      DoOutput("tags");
    DoOutput(/*(*/"...) ___%s(%s_BASE_NAME, "/*)*/, name, ShortBaseNameUpper);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s%s", ap->Args[i].ArgName, i < ap->NumArgs-2 ? ", " : "");
    if(ap->NumArgs < 2)
      DoOutput("tags");
    DoOutput(/*(*/")\n");
    return DoOutput("#endif\n\n");
  }

  DoOutput("#define %s("/*)*/, name);
  if(ap->NumArgs)
  {
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput("%s", ap->Args[i].ArgName);
  }
  DoOutput(/*(*/") ({ \\\n"/*})*/);

  for(i = 0; i < ap->NumArgs; ++i)
  {
    sprintf((strptr)tempbuf, "_%s_%s", name, ap->Args[i].ArgName);
    DoOutput("  ");
    OutClibType(&cd->Args[i], (strptr) tempbuf);
    DoOutput(" = (%s); \\\n", ap->Args[i].ArgName);
  }

  if(Flags & FLAG_INLINENEW)
  {
    if(ap->NumArgs)
      DoOutput("  ({ \\\n"/*})*/);
    DoOutput("  register char * _%s__bn __asm(\"a6\") = (char *) ", name);
    if(BaseName)
      DoOutput("(%s_BASE_NAME);\\\n", ShortBaseNameUpper);
    else
    {
      for(i = 0; i < ap->NumArgs && ap->Args[i].ArgReg != REG_A6; ++i)
        ;
      if(i == ap->NumArgs)
        return 1;
      DoOutput("(%s);\\\n", ap->Args[i].ArgName);
    }

    DoOutput("  (("/*))*/);
    OutClibType(&cd->ReturnType, 0);
    DoOutput(" (*)("/*)*/);
    if(BaseName)
    {
      DoOutput("char * __asm(\"a6\")");
      if(ap->NumArgs)
        DoOutput(", ");
    }
    for(i = 0; i < ap->NumArgs; ++i)
    {
      OutClibType(&cd->Args[i], 0);
      DoOutput(" __asm(\"%s\")", RegNames[ap->Args[i].ArgReg]);
      if(i < ap->NumArgs-1)
        DoOutput(", ");
    }
    DoOutput(/*((*/")) \\\n");
    DoOutput(/*(*/"  (_%s__bn - %d))("/*)*/, name, ap->Bias);
    if(BaseName)
    {
      DoOutput("_%s__bn", name);
      if(ap->NumArgs)
        DoOutput(", ");
    }
    for(i = 0; i < ap->NumArgs; ++i)
    {
      if(ap->Args[i].ArgReg == REG_A6)
        DoOutput("_%s__bn", name);
      else
        DoOutput("_%s_%s", name, ap->Args[i].ArgName);
      if(i < ap->NumArgs-1)
        DoOutput(", ");
    }
    DoOutput(/*(*/"); \\\n");
    if(ap->NumArgs)
      DoOutput(/*({*/"});");
  }
  else
  {
    /* do A5 first, as it is more important */
    if(ap->Flags & AMIPRAGFLAG_A5USE)
    {
      a5 = 0x303; /* D0-D1,A0-A1 are scratch and cannot be used */
      for(i = 0; i < ap->NumArgs; ++i)
        a5 |= 1<<ap->Args[i].ArgReg;
      a5 &= 0xFFF;
      if(a5 == 0xFFF)
      {
        DoError(ERR_INLINE_AX_SWAPREG, ap->Line, RegNamesUpper[REG_A5]);
        a5 = 0;
      }
      else
      {
        for(i = 0; (a5 & 1) && a5; ++i)
          a5 >>= 1;
        a5 = i; /* this is our A5 swap register */
      }
    }
    if(ap->Flags & AMIPRAGFLAG_A4USE)
    {
      a4 = 0x303; /* D0-D1,A0-A1 are scratch and cannot be used */
      if(a5)
        a4 |= (1<<a5);
      for(i = 0; i < ap->NumArgs; ++i)
        a4 |= 1<<ap->Args[i].ArgReg;
      a4 &= 0xFFF;
      if(a4 == 0xFFF)
      {
        DoError(ERR_INLINE_AX_SWAPREG, ap->Line, RegNamesUpper[REG_A4]);
        a4 = 0;
      }
      else
      {
        for(i = 0; (a4 & 1) && a4; ++i)
          a4 >>= 1;
        a4 = i; /* this is our A4 swap register */
      }
    }
    if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
      noret = 1; /* this is a void function */

    if(ap->NumArgs || !noret)
      DoOutput("  %s{ \\\n", noret ? "" : "(" /*})*/);

    if(noret)
      DoOutput("  register int _d0 __asm(\"d0\"); \\\n");
    DoOutput(
    "  register int _d1 __asm(\"d1\"); \\\n"
    "  register int _a0 __asm(\"a0\"); \\\n"
    "  register int _a1 __asm(\"a1\"); \\\n");

    if(BaseName)
      DoOutput("  register %s const __%s__bn __asm(\"a6\") = %s_BASE_NAME;\\\n",
      GetBaseType(), name, ShortBaseNameUpper);

    if(!noret)
    {
      sprintf((strptr)tempbuf, "__%s__re", name);
      DoOutput("  register ");
      OutClibType(&cd->ReturnType, (strptr) tempbuf);
      DoOutput(" __asm(\"d0\"); \\\n");
    }
    if((maxargs = ap->NumArgs) >= 9 && (Flags & FLAG_STORMGCC))
      maxargs = 7;
    for(i = 0; i < maxargs; ++i)
    {
      reg = ap->Args[i].ArgReg;
      if(a5 && reg == REG_A5) reg = a5; /* we need to switch */
      if(a4 && reg == REG_A4) reg = a4; /* we need to switch */

      sprintf((strptr)tempbuf, "__%s_%s", name, ap->Args[i].ArgName);
      DoOutput("  register ");
      OutClibType(&cd->Args[i], (strptr) tempbuf);
      DoOutput(" __asm(\"%s\") = (%s); \\\n", RegNames[reg],
      (strptr) (tempbuf+1));
    }
    if(i != ap->NumArgs) /* StormGCC mode */
    {
      DoOutput("  const struct __%s__ArgsStr { \\\n"/*}*/, name);
      for(i = maxargs; i < ap->NumArgs; ++i)
      {
        DoOutput("    ULONG __%s_%s; \\\n", name, ap->Args[i].ArgName);        
        reg = ap->Args[i].ArgReg;
        if(reg == REG_A4)
        {
          reg = a4; a4 = 0;
        }
        else if(reg == REG_A5)
        {
          reg = a5; a5 = 0;
        }
      }
      /* reg is now either the last register argument or its a4/a5 redirect */
      DoOutput(/*{*/"  } __%s__Args = {"/*}*/, name);
      for(i = maxargs; i < ap->NumArgs; ++i)
      {
        sprintf((strptr)tempbuf, "_%s_%s", name, ap->Args[i].ArgName);
        DoOutput("(ULONG)(%s)%s", (strptr)tempbuf, i == ap->NumArgs-1 ?
        "" : ", ");
      }
      DoOutput(/*{*/"}; \\\n  register const struct __%s__ArgsStr "
      "*__%s__ArgsPtr __asm(\"%s\") = &(__%s__Args); \\\n", name, name,
      RegNames[reg], name);
    }
    DoOutput("  __asm volatile (\""/*)*/);
    if(a5) DoOutput("exg a5,%s\\n\\t", RegNames[a5]);
    if(a4) DoOutput("exg a4,%s\\n\\t", RegNames[a4]);
    if(maxargs != ap->NumArgs) /* StormGCC mode */
    {
      DoOutput("movem.l ");
      for(i = maxargs; i < ap->NumArgs; ++i)
      {
        DoOutput("%s%s", RegNames[ap->Args[i].ArgReg],
        i == ap->NumArgs-1 ? "" : "/");
      }
      DoOutput(",-(a7)\\n\\t");
      for(i = maxargs; i < ap->NumArgs; ++i)
      {
        if(i == maxargs)
          DoOutput("move.l (%s),%s\\n\\t", RegNames[reg],
          RegNames[ap->Args[i].ArgReg]);
        else
          DoOutput("move.l %ld(%s),%s\\n\\t", (i-maxargs)*4, RegNames[reg],
          RegNames[ap->Args[i].ArgReg]);
      }
    }
    DoOutput("jsr a6@(-%d:W)", ap->Bias);
    if(maxargs != ap->NumArgs) /* StormGCC mode */
    {
      DoOutput("\\n\\tmovem.l (a7)+,");
      for(i = maxargs; i < ap->NumArgs; ++i)
      {
        DoOutput("%s%s", RegNames[ap->Args[i].ArgReg],
        i == ap->NumArgs-1 ? "" : "/");
      }
    }
    if(a4) DoOutput("\\n\\texg a4,%s", RegNames[a4]);
    if(a5) DoOutput("\\n\\texg a5,%s", RegNames[a5]);
    DoOutput("\" \\\n");

    if(noret)
      DoOutput("  : \"=r\" (_d0)");
    else
      DoOutput("  : \"=r\"(__%s__re)", name);
    DoOutput(", \"=r\" (_d1), \"=r\" (_a0), \"=r\" (_a1) \\\n  :");
    if(BaseName)
      DoOutput(" \"r\"(__%s__bn)%s", name, ap->NumArgs ? "," : "");
    for(i = 0; i < maxargs; ++i)
    {
      DoOutput(" \"rf\"(__%s_%s)", name, ap->Args[i].ArgName);
      if(i < ap->NumArgs-1)
        DoOutput(",");
    }
    if(i != ap->NumArgs) /* StormGCC mode */
      DoOutput(" \"r\"(__%s__ArgsPtr)", name);
    DoOutput(/*(*/" \\\n  : \"fp0\", \"fp1\", \"cc\", \"memory\"); \\\n");

    if(ap->NumArgs || !noret)
      DoOutput(/*({*/"  }%s \\\n", noret ? "" : ");");
  }

  return DoOutput(/*({*/"})\n\n");
}

uint32 FuncInline(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 noret = 0, a45 = 0, j;
  int32 fp = -1, i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  if(!(Flags & FLAG_INLINENEW) && CheckError(ap, AMIPRAGFLAG_MOSBASESYSV))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_ALIAS)
  {
    if(flags & FUNCFLAG_TAG)
      return DoOutput("#ifndef NO_%sINLINE_STDARG\n#define %s %s\n#endif\n\n",
      Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", name, ap->TagName);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) %s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"(%s))\n\n", ap->Args[i].ArgName);
  }

  if(!(cd = GetClibFunc(ap->Flags & AMIPRAGFLAG_VARARGS ?
  ap->TagName : ap->FuncName, ap, flags)))
    return 1;

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
    noret = 1; /* this is a void function */

  if(ap->Flags & AMIPRAGFLAG_A5USE)
    a45 = REG_A5;
  if(ap->Flags & AMIPRAGFLAG_A4USE)
  {
    if(a45)
    {
      DoError(ERR_INLINE_A4_AND_A5, ap->Line);
      return 1; /* skip this entry */
    }
    a45 = REG_A4;
  }
  if(a45 && (ap->Flags & AMIPRAGFLAG_D7USE))
  {
    DoError(ERR_INLINE_D7_AND_A45, ap->Line);
    return 1; /* skip this entry */
  }

  if((flags & FUNCFLAG_TAG) && !(ap->Flags & AMIPRAGFLAG_MOSBASESYSV))
  {
    if(Flags & FLAG_INLINENEW)
    {
      DoOutput("#ifndef NO_%sINLINE_STDARG\n",
      Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "");
      if(IsNoCreateInlineFunc(name))
      {
        DoOutput("__inline ");
        FuncCSTUBS(ap, flags, name);
        /* call CSTUBS, as this equals the method used there */
      }
      else
      {
        DoOutput("#define %s("/*)*/, name);
        for(i = 0; i < ap->NumArgs-1; ++i)
        {
          DoOutput("%s, ", ap->Args[i].ArgName);
        }
        DoOutput(/*(*/"tags...) \\\n\t({ULONG _tags[] = {tags}; %s("/*}))*/,
        ap->FuncName);
        for(i = 0; i < ap->NumArgs-1; ++i)
          DoOutput("(%s), ", ap->Args[i].ArgName);
        DoOutput("("/*)*/);
        OutClibType(&cd->Args[i], 0);
        DoOutput(/*({((*/") _tags);})\n");
      }
      return DoOutput("#endif\n\n");
    }
    else if((Flags & (FLAG_INLINESTUB|FLAG_MORPHOS))
    == (FLAG_INLINESTUB|FLAG_MORPHOS))
    {
      int32 n, d, tagl, local;

      n = 9-ap->NumArgs;
      d = n & 1 ? 4 : 0;
      tagl = 8 + (Flags2 & FLAG2_DIRECTVARARGS ? 0 : 64);
      local = (n * 4+d+8+15) & ~15;   /* size of the stack frame */

      /* Stack frame:
       *   0- 3: next frame ptr
       *   4- 7: save lr
       *   8-71: struct Caos
       *  72-72+n*4+d+8-1: tag list start
       *   ?-local-1: padding
       */

       DoOutput("asm(\"\n"/*)*/
       "\t.align\t2\n"
       "\t.globl\t%s\n"
       "\t.type\t%s,@function\n"
       "%s:\n"
       "\tstwu\t1,-%ld(1)\n" /* create stack frame */
       "\tmflr\t0\n"
       "\tstw\t0,%ld(1)\n",
       name, name, name, local, local+4);

       /* If n is odd, one tag is split between regs and stack.
        * Copy its ti_Data together with the ti_Tag. */
       if(d)
         DoOutput("\tlwz\t0,%ld(1)\n", local+8); /* read ti_Data */

       /* Save the registers */
       for(i = ap->NumArgs; i <= 8; ++i)
         DoOutput("\tstw\t%ld,%ld(1)\n", i+2, (i-ap->NumArgs) * 4+tagl);

       if(d)
         DoOutput("\tstw\t0,%ld(1)\n", tagl+n * 4); /* write ti_Data */

       /* Add TAG_MORE */
       DoOutput("\tli\t0,2\n"
       "\tstw\t0,%ld(1)\n" /* add TAG_MORE */
       "\taddi\t0,1,%ld\n"
       "\tstw\t0,%ld(1)\n", /* ti_Data=&stack_params */
       tagl+n * 4+d, local+8+d, tagl+n * 4+d+4);

       if(Flags2 & FLAG2_DIRECTVARARGS)
       {
         DoOutput("\taddi\t%d,1,%ld\n" /* vararg_reg=&saved regs */
         "\tbl\t%s\n", ap->NumArgs+2, tagl, name);
       }
       else
       {

         if(!BaseName)
         {
           DoError(ERR_MISSING_BASENAME, ap->Line);
           return 1;
         }
         /* Caos.Offset = -fD_GetOffset(obj) */
         DoOutput("\tli\t0,%d\n"
         "\tstw\t0,8(1)\n", -ap->Bias);

         /* Save the non-varargs registers in the Caos struct. */
         for(i=0; i < ap->NumArgs-1; ++i)
         {
           DoOutput("\tstw\t%ld,%d(1)\n", i+3, 8+4+(ap->Args[i].ArgReg * 4));
         }

         DoOutput("\taddi\t0,1,%ld\n"
         "\tlis\t3,%s@ha\n"
         "\tstw\t0,%d(1)\n" /* Caos.reg_xx = taglist */
         "\tlwz\t12,%s@l(3)\n"
         "\tlwz\t11,88(2)\n"
         "\tstw\t12,68(1)\n" /* Caos.reg_a6=libbase */
         "\tmtctr\t11\n"
         "\taddi\t3,1,8\n"
         "\tbctrl\n", /* EmulCallOS() */
         tagl, BaseName, 12+(4 * ap->Args[i].ArgReg), BaseName);
      }
      DoOutput("\tlwz\t0,%ld(1)\n" /* clear stack frame & return */
      "\tmtlr\t0\n"
      "\taddi\t1,1,%ld\n"
      "\tblr\n"
      /*(*/"\t.size\t%s,$-%s\n\");\n\n", local+4, local, name, name);
    }
    else
    {
      DoOutput("%s%s__inline ", Flags & FLAG_INLINESTUB ? "" : "extern ",
      Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "static " : "");
      return FuncCSTUBS(ap, flags, name);
      /* call CSTUBS, as this equals the method used there */
    }
  }

  if(Flags & FLAG_INLINENEW) /* new style */
  {
    strptr funcpar = "";
    DoOutput("#define %s("/*)*/, name);

    for(i = 0; i < cd->NumArgs; ++i)
    {
      if(cd->Args[i].Flags & CPP_FLAG_FUNCTION)
        funcpar = "FP";
    }

    if(ap->NumArgs)
    {
      for(i = 0; i < ap->NumArgs-1; ++i)
        DoOutput("%s, ", ap->Args[i].ArgName);
      DoOutput("%s", ap->Args[i].ArgName);
    }
    DoOutput(/*(*/") \\\n\t");
    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      DoOutput("((("/*)))*/);
      OutClibType(&cd->ReturnType, 0);
      DoOutput(" (*)("/*)*/);
      DoOutput("%s", GetBaseType());
      for(i = 0; i < ap->NumArgs; ++i)
      {
        DoOutput(", ");
        OutClibType(&cd->Args[i], 0);
      }
      DoOutput(/*(((*/"))*(void**)((long)(%s_BASE_NAME) -%d))"
      "(%s_BASE_NAME",/*)*/
      ShortBaseNameUpper, ap->Bias-2, ShortBaseNameUpper);

      for(i = 0; i < ap->NumArgs - ((flags & FUNCFLAG_TAG) ? 1 : 0); ++i)
        DoOutput(", %s", ap->Args[i].ArgName);
      if(flags & FUNCFLAG_TAG)
        DoOutput(", __VA_ARGS__");
      return DoOutput(/*((*/"))\n\n");
    }
    DoOutput("LP%d%s%s%s%s(0x%x, "/*)*/, ap->NumArgs,
    (noret ? "NR" : ""), (a45 ? RegNamesUpper[a45] : (strptr) ""),
    (BaseName ? "" : "UB"), funcpar, ap->Bias);
    if(!noret)
    {
      OutClibType(&cd->ReturnType, 0);
      DoOutput(", ");
    }
    DoOutput("%s, ", name);

    for(i = 0; i < ap->NumArgs; ++i)
    {
      j = ap->Args[i].ArgReg;
      if(a45 && (j == REG_A4 || j == REG_A5))
        j = REG_D7;
      if(cd->Args[i].Flags & CPP_FLAG_FUNCTION)
      {
        if(fp != -1)
        {
          DoError(ERR_MULTIPLEFUNCTION, ap->Line);
          DoOutput("void *");
        }
        else
        {
          DoOutput("__fpt"); fp = i;
        }
      }
      else
        OutClibType(&cd->Args[i], 0);
  
      DoOutput(", %s, %s%s", ap->Args[i].ArgName, RegNames[j],
      (i == ap->NumArgs-1 && !BaseName ? "" : ", "));
    }

    if(BaseName) /* was "##base" used? */
      DoOutput("\\\n\t, %s_BASE_NAME", ShortBaseNameUpper);

    if(fp >= 0)
    {
      DoOutput(", ");
      OutClibType(&cd->Args[fp], "__fpt");
    }

    if(Flags & (FLAG_POWERUP|FLAG_MORPHOS))
      DoOutput(", IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0");

    return DoOutput(/*(*/")\n\n");
  }

  /* old mode or stubs mode */

  if((Flags & (FLAG_INLINESTUB|FLAG_MORPHOS)) != (FLAG_INLINESTUB|FLAG_MORPHOS))
    DoOutput("%s%s__inline ", Flags & (FLAG_INLINESTUB|FLAG_MORPHOS) ?
    "" : "extern ", Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "static " : "");
  OutClibType(&cd->ReturnType, 0);
  DoOutput("\n%s(%s"/*)*/, name, (BaseName ?
  (ap->NumArgs ? "BASE_PAR_DECL " : "BASE_PAR_DECL0") : ""));

  for(i = 0; i < ap->NumArgs; ++i)
  {
    OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }

  if(Flags & FLAG_POWERUP)
  {
    DoOutput(/*(*/")\n{\n\tstruct Caos MyCaos;\n"/*}*/
    "\tMyCaos.M68kCacheMode\t= IF_CACHEFLUSHALL;\n"
    "/*\tMyCaos.M68kStart\t= NULL;\t*/\n"
    "/*\tMyCaos.M68kSize\t\t= 0;\t*/\n"
    "\tMyCaos.PPCCacheMode\t= IF_CACHEFLUSHALL;\n"
    "/*\tMyCaos.PPCStart\t\t= NULL;\t*/\n"
    "/*\tMyCaos.PPCSize\t\t= 0;\t*/\n");

    if(ap->NumArgs)
    {
      for(i = 0; i < ap->NumArgs; ++i)
      {
        DoOutput("\tMyCaos.%s\t\t= (ULONG) %s;\n",
        RegNames[ap->Args[i].ArgReg], ap->Args[i].ArgName);
      }
    }

    DoOutput("\tMyCaos.caos_Un.Offset\t= %d;\n", -ap->Bias);

    if(BaseName)
      DoOutput("\tMyCaos.a6\t\t= (ULONG) %s_BASE_NAME;\n", ShortBaseNameUpper);
    if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
      DoOutput(/*{*/"\tPPCCallOS(&MyCaos);\n}\n\n");
    else
    {
      DoOutput("\treturn(("/*))*/);
      OutClibType(&cd->ReturnType, 0);
      DoOutput(/*{((*/")PPCCallOS(&MyCaos));\n}\n\n");
    }
    return Output_Error;
  }
  else if(Flags & FLAG_MORPHOS)
  {
    DoOutput(/*(*/")\n{\n\tstruct EmulCaos MyCaos;\n"/*}*/);

    if(ap->NumArgs)
    {
      for(i = 0; i < ap->NumArgs; ++i)
      {
        DoOutput("\tMyCaos.reg_%s\t\t= (ULONG) %s;\n",
        RegNames[ap->Args[i].ArgReg], ap->Args[i].ArgName);
      }
    }

    DoOutput("\tMyCaos.caos_Un.Offset\t= %d;\n", -ap->Bias);
    if(BaseName)
      DoOutput("\tMyCaos.reg_a6\t\t= (ULONG) %s_BASE_NAME;\n",
      ShortBaseNameUpper);

    if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
      DoOutput(/*{*/"\t(*MyEmulHandle->EmulCallOS)(&MyCaos);\n}\n\n");
    else
    {
      DoOutput("\treturn(("/*))*/);
      OutClibType(&cd->ReturnType, 0);
      DoOutput(/*{((*/")(*MyEmulHandle->EmulCallOS)(&MyCaos));\n}\n\n");
    }
    return Output_Error;
  }

  DoOutput(/*(*/")\n{\n%s"/*}*/, (BaseName ? "  BASE_EXT_DECL\n" : ""));

  if(!noret)
  {
    DoOutput("  register ");
    OutClibType(&cd->ReturnType, "res");
    DoOutput(" __asm(\"d0\");\n");
  }

  if(BaseName)
    DoOutput("  register %s a6 __asm(\"a6\") = %s_BASE_NAME;\n",
    GetBaseType(), ShortBaseNameUpper);

  for(i = 0; i < ap->NumArgs; ++i)
  {
    j = ap->Args[i].ArgReg;
    if(a45 && (j == REG_A4 || j == REG_A5))
      j = REG_D7;

    DoOutput("  register ");
    OutClibType(&cd->Args[i], RegNames[j]);
    DoOutput(" __asm(\"%s\") = %s;\n", RegNames[j], ap->Args[i].ArgName);
  }

  if(a45)
  {
    DoOutput("  __asm volatile (\"exg d7,%s\\n\\t"/*)*/
    "jsr a6@(-0x%x:W)\\n\\texg d7,%s\"\n", RegNames[a45],
    ap->Bias, RegNames[a45]);
  }
  else
    DoOutput("  __asm volatile (\"jsr a6@(-0x%x:W)\"\n"/*)*/, ap->Bias);

  DoOutput(noret ? "  : /* No Output */\n" : "  : \"=r\" (res)\n");

  DoOutput("  : ");
  if(BaseName)
    DoOutput("\"r\" (a6)%s", (ap->NumArgs ? ", ": ""));

  for(i = 0; i < ap->NumArgs; ++i)
  {
    j = ap->Args[i].ArgReg;
    if(a45 && (j == REG_A4 || j == REG_A5))
      j = REG_D7;

    DoOutput("\"r\" (%s)%s", RegNames[j], (i < ap->NumArgs-1 ? ", " : ""));
  }

  DoOutput("\n  : \"d0\", \"d1\", \"a0\", \"a1\", \"fp0\", \"fp1\"");

  if(noret)
    return DoOutput(/*({*/", \"cc\", \"memory\");\n}\n\n");
  else
    return DoOutput(/*({*/", \"cc\", \"memory\");\n  return res;\n}\n\n");
}


/* new style inlines designed by Bernardo Innocenti */
uint32 FuncInlineNS(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_ALIAS)
  {
    if(flags & FUNCFLAG_TAG)
      return DoOutput("#ifndef NO_INLINE_STDARG\n#define %s %s\n#endif\n\n",
      name, ap->TagName);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) %s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"(%s))\n\n", ap->Args[i].ArgName);
  }

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if((flags & FUNCFLAG_TAG))
  {
    if(!(Flags2 & FLAG2_INLINEMAC))
    {
      DoOutput("static __inline ");
      return FuncCSTUBS(ap, flags, name);
      /* call CSTUBS, as this equals the method used there */
    }
    else
    {
      DoOutput("#ifndef NO_%sINLINE_STDARG\n#define %s("/*)*/,
      Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", name);
      for(i = 0; i < ap->NumArgs-1; ++i)
        DoOutput("%s, ", ap->Args[i].ArgName);
      DoOutput(/*(*/"tags...) \\\n\t({ULONG _tags[] = {tags}; %s("/*}))*/,
      ap->FuncName);
      for(i = 0; i < ap->NumArgs-1; ++i)
        DoOutput("(%s), ", ap->Args[i].ArgName);
      DoOutput("("/*)*/);
      OutClibType(&cd->Args[i], 0);
      return DoOutput(/*({((*/") _tags);})\n#endif\n\n");
    }
  }

  if(Flags2 & FLAG2_INLINEMAC)
  {
    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs; ++i)
    {
      DoOutput("%s", ap->Args[i].ArgName);
      if(i < ap->NumArgs-1)
        DoOutput(", ");
    }
    DoOutput(/*(*/") \\\n\t");
  }
  else
  {
    DoOutput("static __inline ");
    OutClibType(&cd->ReturnType, 0);
    DoOutput(" %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs; ++i)
    {
      OutClibType(&cd->Args[i], ap->Args[i].ArgName);
      if(i < ap->NumArgs-1)
        DoOutput(", ");
    }
    DoOutput(/*(*/")\n{\n  "/*}*/);
    if(!IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
      DoOutput("return ");
  }
  DoOutput("(("/*))*/);
  OutClibType(&cd->ReturnType, 0);
  DoOutput(" (*)("/*)*/);
  for(i = 0; i < ap->NumArgs; ++i)
  {
    OutClibType(&cd->Args[i], 0);
    DoOutput(" __asm(\"%s\")", RegNames[ap->Args[i].ArgReg]);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(BaseName)
  {
    if(ap->NumArgs)
      DoOutput(", ");
    DoOutput("%s __asm(\"a6\")", GetBaseType());
  }
  DoOutput(/*((*/"))");
  if(Flags2 & FLAG2_INLINEMAC)
    DoOutput(" \\");
  if(BaseName)
    DoOutput(/*(*/"\n  (((char *) %s_BASE_NAME) - %d))("/*)*/,
    ShortBaseNameUpper, ap->Bias);
  else
  {
    for(i = 0; i < ap->NumArgs && ap->Args[i].ArgReg != REG_A6; ++i)
      ;
    if(i == ap->NumArgs)
      return 1;
    DoOutput(/*(*/"\n  (((char *) %s) - %d))("/*)*/, ap->Args[i].ArgName,
    ap->Bias);
  }
  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("%s", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(BaseName)
  {
    if(ap->NumArgs)
      DoOutput(", ");
    DoOutput("%s_BASE_NAME", ShortBaseNameUpper);
  }

  if(Flags2 & FLAG2_INLINEMAC)
    DoOutput(/*(*/")\n");
  else
    DoOutput(/*{(*/");\n}\n");

  return DoOutput("\n");
}

uint32 FuncPowerUP(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_ALIAS)
  {
    if(flags & FUNCFLAG_TAG)
      return DoOutput("#ifndef NO_PPCINLINE_STDARG\n#define %s %s\n#endif\n\n",
      name, ap->TagName);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) %s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"(%s))\n\n", ap->Args[i].ArgName);
  }

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(flags & FUNCFLAG_TAG)
  {
    DoOutput("#ifndef NO_PPCINLINE_STDARG\n#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"tags...) \\\n\t({ULONG _tags[] = {tags}; %s("/*)})*/,
    ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    DoOutput("("/*)*/);
    OutClibType(&cd->Args[i], 0);
    return DoOutput(/*({((*/") _tags);})\n#endif\n\n");
  }

  DoOutput("#define\t%s("/*)*/, name);

  if(ap->NumArgs)
  {
  
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s)\t_%s("/*)*/, ap->Args[i].ArgName, name);

    if(BaseName)
      DoOutput("%s_BASE_NAME, ", ShortBaseNameUpper);

    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s)\n\n", ap->Args[i].ArgName);
  }
  else if(BaseName)
    DoOutput(/*(*/")\t_%s(%s_BASE_NAME)\n\n", name, ShortBaseNameUpper);
  else
    DoOutput(/*(*/")\t_%s()\n\n", name);

  DoOutput("static __inline ");
  OutClibType(&cd->ReturnType, 0);
  
  DoOutput("\n_%s("/*)*/, name);
  if(BaseName)
    DoOutput("void * %s%s", BaseName, ap->NumArgs ? ", " : "");

  for(i = 0; i < ap->NumArgs; ++i)
  {
    OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }

  DoOutput(/*(*/")\n{\n\tstruct Caos MyCaos;\n"/*}*/
  "\tMyCaos.M68kCacheMode\t= IF_CACHEFLUSHALL;\n"
  "/*\tMyCaos.M68kStart\t= NULL;\t*/\n"
  "/*\tMyCaos.M68kSize\t\t= 0;\t*/\n"
  "\tMyCaos.PPCCacheMode\t= IF_CACHEFLUSHALL;\n"
  "/*\tMyCaos.PPCStart\t\t= NULL;\t*/\n"
  "/*\tMyCaos.PPCSize\t\t= 0;\t*/\n");

  if(ap->NumArgs)
  {
    for(i = 0; i < ap->NumArgs; ++i)
    {
      DoOutput("\tMyCaos.%s\t\t= (ULONG) %s;\n",
      RegNames[ap->Args[i].ArgReg], ap->Args[i].ArgName);
    }
  }

  DoOutput("\tMyCaos.caos_Un.Offset\t= %d;\n", -ap->Bias);

  if(BaseName)
    DoOutput("\tMyCaos.a6\t\t= (ULONG) %s;\n", BaseName);
  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
    DoOutput(/*{*/"\tPPCCallOS(&MyCaos);\n}\n\n");
  else
  {
    DoOutput("\treturn(("/*))*/);
    OutClibType(&cd->ReturnType, 0);
    DoOutput(/*{((*/")PPCCallOS(&MyCaos));\n}\n\n");
  }
  return Output_Error;
}

uint32 FuncFPCUnit(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;
  else if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(!FuncFPCType(ap, flags, name))
    return 0;

  DoOutput("BEGIN\n  ASM\n\tMOVE.L\tA6,-(A7)\n");

  for(i = 0; i < ap->NumArgs; ++i)
    DoOutput("\tMOVE%s.L\t%s,%s\n", ap->Args[i].ArgReg >= REG_A0 ? "A" : "",
    ap->Args[i].ArgName, RegNamesUpper[ap->Args[i].ArgReg]);

  if(BaseName)
    DoOutput("\tMOVEA.L\t%s,A6\n", BaseName);
  DoOutput("\tJSR\t-%03d(A6)\n\tMOVEA.L\t(A7)+,A6\n", ap->Bias);

  if(!IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
  {
    if(!cd->ReturnType.PointerDepth &&
    cd->ReturnType.Flags == CPP_FLAG_BOOLEAN)
      DoOutput("\tTST.W\tD0\n\tBEQ.B\t@end\n\tMOVEQ\t#1,D0\n"
      "  @end:\tMOVE.B\tD0,@RESULT\n");
    else
      DoOutput("\tMOVE.L\tD0,@RESULT\n");
  }
  return DoOutput("  END;\nEND;\n\n");
}

uint32 FuncFPCType(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 ret = 1;
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;
  else if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
  {
    ret = 0; DoOutput("PROCEDURE %s", name);
  }
  else
    DoOutput("FUNCTION %s", name);

  if(ap->NumArgs)
  {
    DoOutput("("/*)*/);
    for(i = 0; i < ap->NumArgs;)
    {
      OutPASCALType(&cd->Args[i], ap->Args[i].ArgName, 0);
      if(++i != ap->NumArgs)
        DoOutput("; ");
    }
    DoOutput(/*(*/")");
  }

  if(ret)
    OutPASCALType(&cd->ReturnType, "", 1);

  Flags |= FLAG_DONE; /* We did something */

  return DoOutput(";\n");
}

uint32 FuncFPCTypeTags(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 ret = 1;
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;
  else if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
  {
    ret = 0; DoOutput("PROCEDURE %s", name);
  }
  else
    DoOutput("FUNCTION %s", name);

  if(ap->NumArgs)
  {
    DoOutput("("/*)*/);
    for(i = 0; i < ap->NumArgs-1;)
    {
      OutPASCALType(&cd->Args[i], ap->Args[i].ArgName, 0);
      if(++i != ap->NumArgs)
        DoOutput("; ");
    }
    DoOutput("const %s : Array Of Const",ap->Args[i].ArgName);
    DoOutput(/*(*/")");
  }

  if(ret)
    OutPASCALType(&cd->ReturnType, "", 1);

  Flags |= FLAG_DONE; /* We did something */

  return DoOutput(";\n");
}

uint32 FuncFPCTypeTagsUnit(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 ret = 1;
  int32 i;
  struct ClibData *cd;

  if(CheckError(ap, AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;
  else if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
  {
    ret = 0; DoOutput("PROCEDURE %s", name);
  }
  else
    DoOutput("FUNCTION %s", name);

  if(ap->NumArgs)
  {
    DoOutput("("/*)*/);
    for(i = 0; i < ap->NumArgs-1;)
    {
      OutPASCALType(&cd->Args[i], ap->Args[i].ArgName, 0);
      if(++i != ap->NumArgs)
        DoOutput("; ");
    }
    DoOutput("const %s : Array Of Const",ap->Args[i].ArgName);
    DoOutput(/*(*/")");
  }

  if(ret)
    OutPASCALType(&cd->ReturnType, "", 1);

  DoOutput(";\nbegin\n");
  
  if(ret)
    DoOutput("    %s := %s",name, ap->FuncName);
  else DoOutput("    %s", ap->FuncName);

  if(ap->NumArgs)
  {
    DoOutput("("/*)*/);
    for(i = 0; i < ap->NumArgs-1;)
    {
      DoOutput("%s ", ap->Args[i].ArgName);
      if(++i != ap->NumArgs)
        DoOutput(", ");
    }
    DoOutput("readintags(%s)",ap->Args[i].ArgName);
    DoOutput(/*(*/");");
  }

  DoOutput("\nend");

  Flags |= FLAG_DONE; /* We did something */

  return DoOutput(";\n\n");
}


uint32 FuncBMAP(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint8 reg, i;

  if(CheckError(ap, AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_A6USE|AMIPRAGFLAG_A5USE
  |AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  for(i = 0; BMAPSpecial[i]; ++i)
  {
    if(!stricmp(name, BMAPSpecial[i]))
    {
      DoOutput("x"); break;
    }
  }

  DoOutput("%s",name);
  reg = 0;                      DoOutputDirect(&reg, 1);
  reg = (-ap->Bias)>>8;         DoOutputDirect(&reg, 1);
  reg = -ap->Bias;              DoOutputDirect(&reg, 1);
  for(i = 0; i < ap->NumArgs; ++i)
  {
    reg = 1+ap->Args[i].ArgReg; DoOutputDirect(&reg, 1);
  }
  reg = 0;
  return DoOutputDirect(&reg, 1);
}

uint32 FuncVBCCInline(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  strptr c1, c2;
  int32 i, k;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  c1 = Flags & FLAG_NEWSYNTAX ? "(" : ""; /*)*/
  c2 = Flags & FLAG_NEWSYNTAX ? "," : "("; /*)*/

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_TAG)
  {
    if(IsNoCreateInlineFunc(name))
      return 1; /* do not create some strange functions */
    DoOutput("#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && "
    "(__STDC_VERSION__ >= 199901L)\n");
  }

  if(flags & FUNCFLAG_ALIAS)
  {
    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) __%s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    if(Flags2 & FLAG2_OLDVBCC)
    {
      for(i = 0; i < ap->NumArgs; ++i)
        DoOutput("(%s), ", ap->Args[i].ArgName);
      return DoOutput(/*(*/"%s)\n%s\n", BaseName, flags & FUNCFLAG_TAG ?
      "#endif\n" : "");
    }
    else
    {
      DoOutput("%s", BaseName);
      for(i = 0; i < ap->NumArgs; ++i)
        DoOutput(", (%s)", ap->Args[i].ArgName);
      return DoOutput(/*(*/")\n%s\n", flags & FUNCFLAG_TAG ?
      "#endif\n" : "");
    }
  }

  OutClibType(&cd->ReturnType, 0);
  DoOutput(" __%s("/*)*/, name);

  if(!(Flags2 & FLAG2_OLDVBCC) && BaseName)
  {
    DoOutput("__reg(\"a6\") %s", GetBaseType());
    if(ap->NumArgs)
      DoOutput(", ");
  }

  k = (flags & FUNCFLAG_TAG) ? ap->NumArgs-1 : ap->NumArgs;
  for(i = 0; i < k; ++i)
  {
    DoOutput("__reg(\"%s\") ", RegNames[ap->Args[i].ArgReg]);
    if(ap->Args[i].ArgReg >= REG_A0 && ap->Args[i].ArgReg <= REG_A7
    && !(cd->Args[i].Flags & (CPP_FLAG_POINTER|CPP_FLAG_FUNCTION)))
    {
      DoOutput("void * %s", ap->Args[i].ArgName);
    }
    else
      OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }

  if((Flags2 & FLAG2_OLDVBCC) && BaseName)
  {
    if(ap->NumArgs)
      DoOutput(", ");
    DoOutput("__reg(\"a6\") %s", GetBaseType());
  }

  if(flags & FUNCFLAG_TAG)
  {
    if(cd->Args[k].Type != CPP_TYPE_VARARGS)
    {
      OutClibType(&cd->Args[k], ap->Args[k].ArgName);
      DoOutput(", ");
    }
    DoOutput(/*(*/"...)=\"\\tmove.l\\t%s,-(a7)\\n", 
    RegNames[ap->Args[k].ArgReg]);

    if(ap->Args[k].ArgReg > 7)
      DoOutput(/*(*/"\\tlea\\t%s4%sa7),%s\\n", c1, c2,
      RegNames[ap->Args[k].ArgReg]);
    else
      DoOutput("\\tmove.l\\ta7,%s\\n\\taddq.l\\t#4,%s\\n",
      RegNames[ap->Args[k].ArgReg], RegNames[ap->Args[k].ArgReg]);

    DoOutput(/*(*/"\\tjsr\\t%s-%d%sa6)\\n"
    "\\tmove%s.l\\t(a7)+,%s\";\n", c1, ap->Bias, c2,
    ap->Args[k].ArgReg >= REG_A0 ? "a" : "", RegNames[ap->Args[k].ArgReg]);
  }
  else
    DoOutput(/*((*/")=\"\\tjsr\\t%s-%d%sa6)\";\n", c1, ap->Bias, c2);

  k = (flags & FUNCFLAG_TAG) ? ap->NumArgs-2 : ap->NumArgs;
  DoOutput("#define %s("/*)*/, name);
  for(i = 0; i < k; ++i)
  {
    DoOutput("%s", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("%s, ", ap->Args[k].ArgName);
    DoOutput("...");
  }
  DoOutput(/*(*/") __%s("/*)*/, name);
  if(!(Flags2 & FLAG2_OLDVBCC) && BaseName)
  {
    DoOutput("%s", BaseName);
    if(ap->NumArgs)
      DoOutput(", ");
  }
  for(i = 0; i < k; ++i)
  {
    if(ap->Args[i].ArgReg >= REG_A0 && ap->Args[i].ArgReg <= REG_A7
    && !(cd->Args[i].Flags & (CPP_FLAG_POINTER|CPP_FLAG_FUNCTION)))
    {
      DoOutput("(void *)");
    }
    DoOutput("(%s)", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if((Flags2 & FLAG2_OLDVBCC) && BaseName)
  {
    if(ap->NumArgs)
      DoOutput(", ");
    DoOutput("%s", BaseName);
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("(%s), ", ap->Args[k].ArgName);
    DoOutput("__VA_ARGS__");
  }

  return DoOutput(/*(*/")\n%s\n", flags & FUNCFLAG_TAG ? "#endif\n" : "");
}

uint32 FuncVBCCWOSInline(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, k;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_M68K))
    return 1;

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_TAG)
  {
    if(IsNoCreateInlineFunc(name))
      return 1; /* do not create some strange functions */
    DoOutput("#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && "
    "(__STDC_VERSION__ >= 199901L)\n");
  }

  if(flags & FUNCFLAG_ALIAS)
  {
    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) __%s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"%s)\n%s\n", BaseName, flags & FUNCFLAG_TAG ?
    "#endif\n" : "");
  }

  OutClibType(&cd->ReturnType, 0);
  DoOutput(" __%s("/*)*/, name);

  if(!(ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2)))
  {
    DoOutput("%s", GetBaseType());
    if(ap->NumArgs)
      DoOutput(", ");
  }

  k = ap->NumArgs;
  for(i = 0; i < k; ++i)
  {
    OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    if(i < k-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
    DoOutput(", ...");  /* a standalone ... is not allowed in C */

  DoOutput(/*(*/")=\"");
  if(ap->Flags & AMIPRAGFLAG_PPC0)
  {
    DoOutput("\\t.extern\\t_%s\\n", BaseName);
    if ((flags & FUNCFLAG_TAG) && k>0)
    {
      /* save tag1 and load taglist-pointer */
      DoOutput("\\tstw\\t%s%d,%d(%s1)\\n"
               "\\taddi\\t%s%d,%s1,%d\\n",
      PPCRegPrefix, (int)k+2, 20+(int)k*4, PPCRegPrefix, PPCRegPrefix,
      (int)k+2, PPCRegPrefix, 20+(int)k*4);
    }
    DoOutput("\\tlwz\\t%s11,_%s(%s2)\\n"
             "\\tlwz\\t%s0,-%d(%s11)\\n"
             "\\tmtlr\\t%s0\\n"
             "\\tblrl",
             PPCRegPrefix, BaseName, PPCRegPrefix, PPCRegPrefix,
             ap->Bias-2, PPCRegPrefix, PPCRegPrefix);
  }
  else if(ap->Flags & AMIPRAGFLAG_PPC2)
  {
    /* @@@ tagcall handling? */
    DoOutput("\\tstw\\t%s2,20(%s1)\\n"
             "\\t.extern\\t_%s\\n"
             "\\tlwz\\t%s2,_%s(%s2)\\n"
             "\\tlwz\\t%s0,-%d(%s2)\\n"
             "\\tmtlr\\t%s0\\n"
             "\\tblrl\\n"
             "\\tlwz\\t%s2,20(%s1)",
             PPCRegPrefix, PPCRegPrefix, BaseName, PPCRegPrefix, BaseName,
             PPCRegPrefix, PPCRegPrefix, ap->Bias-2, PPCRegPrefix,
             PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
  }
  else
  {
    if ((flags & FUNCFLAG_TAG) && k>0)
    {
      /* save tag1 and load taglist-pointer */
      DoOutput("\\tstw\\t%s%d,%d(%s1)\\n"
               "\\taddi\\t%s%d,%s1,%d\\n",
      PPCRegPrefix, (int)k+3, 24+(int)k*4, PPCRegPrefix, PPCRegPrefix,
      (int)k+3, PPCRegPrefix, 24+(int)k*4);
    }
    DoOutput("\\tlwz\\t%s0,-%d(%s3)\\n"
             "\\tmtlr\\t%s0\\n"
             "\\tblrl",
             PPCRegPrefix, ap->Bias-2, PPCRegPrefix, PPCRegPrefix);
  }
  DoOutput("\";\n");

  k = (flags & FUNCFLAG_TAG) ? ap->NumArgs-2 : ap->NumArgs;
  DoOutput("#define %s("/*)*/, name);
  for(i = 0; i < k; ++i)
  {
    DoOutput("%s", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("%s, ", ap->Args[k].ArgName);
    DoOutput("...");
  }
  DoOutput(/*(*/") __%s("/*)*/, name);
  if(!(ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2)))
  {
    DoOutput("%s", BaseName);
    if(ap->NumArgs)
      DoOutput(", ");
  }
  for(i = 0; i < k; ++i)
  {
    DoOutput("(%s)", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("(%s), ", ap->Args[k].ArgName);
    DoOutput("__VA_ARGS__");
  }

  return DoOutput(/*(*/")\n%s\n", flags & FUNCFLAG_TAG ? "#endif\n" : "");
}

uint32 FuncVBCCMorphInline(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, k;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_TAG)
  {
    if(IsNoCreateInlineFunc(name))
      return 1; /* do not create some strange functions */
    DoOutput("#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && "
    "(__STDC_VERSION__ >= 199901L)\n");
  }

  if(flags & FUNCFLAG_ALIAS)
  {
    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) __%s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"%s)\n%s\n", BaseName, flags & FUNCFLAG_TAG ?
    "#endif\n" : "");
  }

  OutClibType(&cd->ReturnType, 0);
  if((flags & FUNCFLAG_TAG) && (Flags2 & FLAG2_SHORTPPCVBCCINLINE))
    DoOutput(" __linearvarargs");

  DoOutput(" __%s("/*)*/, name);

  if(BaseName && !(ap->Flags & (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12)))
  {
    DoOutput("%s", GetBaseType());
    if(ap->NumArgs)
      DoOutput(", ");
  }

  if(!(Flags2 & FLAG2_SHORTPPCVBCCINLINE))
  {
    if((flags & FUNCFLAG_TAG) &&
       !(ap->Flags & AMIPRAGFLAG_MOS_ALL))
    {
      for(i = ap->NumArgs+(BaseName?1:0); i <= 8; ++i)
        DoOutput("long, ");
    }
  }

  k = (flags & FUNCFLAG_TAG) ? ap->NumArgs-1 : ap->NumArgs;
  for(i = 0; i < k; ++i)
  {
    OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }

  if(flags & FUNCFLAG_TAG)
  {
    if(!(Flags2 & FLAG2_SHORTPPCVBCCINLINE))
    {
      if((ap->Flags & AMIPRAGFLAG_MOS_ALL)
      && !(ap->Flags & AMIPRAGFLAG_VARARGS))
      {
        for(i = ap->NumArgs+(BaseName?1:0); i <= 8; ++i)
          DoOutput("long, ");
      }
      if(cd->Args[k].Type != CPP_TYPE_VARARGS)
      {
        OutClibType(&cd->Args[k], ap->Args[k].ArgName);
        DoOutput(", ");
      }
    }

    DoOutput("...");
  }

  if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
  {
    DoOutput(/*(*/") =\n\t\"\\tlwz\\t%s0,-%d(%s3)\\n\"\n",
    PPCRegPrefix, ap->Bias-2, PPCRegPrefix);
    if((ap->Flags != AMIPRAGFLAG_VARARGS) && (flags & FUNCFLAG_TAG))
    {
      DoOutput("\t\"\\taddi\\t%s%ld,%s1,8\\n\"\n",
      PPCRegPrefix,3+k+1,PPCRegPrefix);
    }
    DoOutput("\t\"\\tmtctr\\t%s0\\n\"\n"
    "\t\"\\tbctrl\";\n", PPCRegPrefix);
  }
  else if(ap->Flags & (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12))
  {
    if (BaseName)
    {
      DoOutput(/*(*/") =\n\t\"\\tlis\\t%s11,%s@ha\\n\"\n"
               "\t\"\\tlwz\\t%s12,%s@l(%s11)\\n\"\n"
               "\t\"\\tlwz\\t%s0,-%d(%s12)\\n\"\n",
               PPCRegPrefix, BaseName,
               PPCRegPrefix, BaseName, PPCRegPrefix,
               PPCRegPrefix, ap->Bias-2, PPCRegPrefix);
    }
    if((ap->Flags != AMIPRAGFLAG_VARARGS) && (flags & FUNCFLAG_TAG))
    {
      DoOutput("\t\"\\taddi\\t%s%ld,%s1,8\\n\"\n",
      PPCRegPrefix,3+k+1,PPCRegPrefix);
    }
    DoOutput("\t\"\\tmtctr\\t%s0\\n\"\n"
             "\t\"\\tbctrl\";\n", PPCRegPrefix);
  }
  else
  {
    int ofs = 4, fix = 0;
    DoOutput(/*(*/") =\n\t\"\\tlwz\\t%s11,100(%s2)\\n\"\n",
    PPCRegPrefix, PPCRegPrefix);
    k = 3;
    if(BaseName)
    {
      DoOutput("\t\"\\tstw\\t%s%ld,56(%s2)\\n\"\n", PPCRegPrefix, k++,
      PPCRegPrefix);
    }
    if(Flags2 & FLAG2_SHORTPPCVBCCINLINE)
    {
      ofs = 12;
      if((i = ap->NumArgs+(BaseName?1:0)) <= 8)
        fix = 8+1-i;
    }
    else if(flags & FUNCFLAG_TAG)
    {
      if((i = ap->NumArgs+(BaseName?1:0)) <= 8)
        k += 8+1-i;
    }

    DoOutput("\t\"\\tmtctr\\t%s11\\n\"\n", PPCRegPrefix);
    for(i = 0; i < ap->NumArgs; ++i)
    {
      if(!(flags & FUNCFLAG_TAG) || i < ap->NumArgs-1)
      {
        if(k <= 7+3)
          DoOutput("\t\"\\tstw\\t%s%ld,", PPCRegPrefix, k++);
        else
          DoOutput("\t\"\\tlwz\\t%s11,%ld(%s1)\\n\"\n\t\"\\tstw\\t%s11,",
          PPCRegPrefix, 8+(k++-11)*4, PPCRegPrefix, PPCRegPrefix);
      }
      else
      {
        DoOutput("\t\"\\taddi\\t%s%d,%s1,%ld\\n\"\n\t\"\\tstw\\t%s%d,",
        PPCRegPrefix, ofs, PPCRegPrefix, (2+k+fix-11)*4, PPCRegPrefix, ofs);
      }
      DoOutput("%d(%s2)\\n\"\n", 4*ap->Args[i].ArgReg, PPCRegPrefix);
    }
    DoOutput("\t\"\\tli\\t%s3,-%d\\n\"\n\t\"\\tbctrl\";\n", PPCRegPrefix,
    ap->Bias);
  }

  k = (flags & FUNCFLAG_TAG) ? ap->NumArgs-2 : ap->NumArgs;
  DoOutput("#define %s("/*)*/, name);
  for(i = 0; i < k; ++i)
  {
    DoOutput("%s", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("%s, ", ap->Args[k].ArgName);
    DoOutput("...");
  }
  DoOutput(/*(*/") __%s("/*)*/, name);
  if(BaseName && !(ap->Flags & (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12)))
  {
    DoOutput("%s", BaseName);
    if(ap->NumArgs)
      DoOutput(", ");
  }
  if(!(Flags2 & FLAG2_SHORTPPCVBCCINLINE))
  {
    if(flags & FUNCFLAG_TAG && !(ap->Flags & AMIPRAGFLAG_MOSBASESYSV))
    {
      for(i = ap->NumArgs+(BaseName?1:0); i <= 8; ++i)
        DoOutput("0, ");
    }
  }
  for(i = 0; i < k; ++i)
  {
    DoOutput("(%s)", ap->Args[i].ArgName);
    if(i < ap->NumArgs-1)
      DoOutput(", ");
  }
  if(flags & FUNCFLAG_TAG)
  {
    if(ap->NumArgs > 1 && cd->Args[ap->NumArgs-1].Type != CPP_TYPE_VARARGS)
      DoOutput("(%s), ", ap->Args[k].ArgName);
    DoOutput("__VA_ARGS__");
  }

  return DoOutput(/*(*/")\n%s\n", flags & FUNCFLAG_TAG ? "#endif\n" : "");
}

uint32 FuncVBCCWOSText(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 i, k, count, ofs;
  struct ClibData *cd = 0;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG))
    return 1;

  if((flags & FUNCFLAG_TAG) && !(ap->Flags & AMIPRAGFLAG_PPC) &&
  !(cd = GetClibFunc(name, ap, flags)))
    return 1;

  if((ap->Flags & AMIPRAGFLAG_PPC) && !BaseName &&
  ((ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2)) ||
  !(Flags & FLAG_WOSLIBBASE)))
  {
    DoError(ERR_MISSING_BASENAME, ap->Line);
    return 1;
  }

  Flags |= FLAG_DONE;

  if(Flags & FLAG_SINGLEFILE)
  {
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
  }

  if(Flags & (FLAG_ASMSECTION|FLAG_SINGLEFILE))
    DoOutput("\t.section %s,\"acrx4\"\n", hunkname);

  if(Flags & FLAG_SINGLEFILE)
    DoOutput("\t.file\t\"%s.o\"\n", name);
  DoOutput("\t.align\t3\n");
  if(Flags & FLAG_WOSLIBBASE)  /* PPCBase already in r3, LibBase in r4 */
  {
    if(ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2))
      DoOutput("\t.extern _%s\n", BaseName);
    DoOutput("\t.global __%s\n__%s:\n", name, name);
  }
  else
  {
    if(BaseName)
      DoOutput("\t.extern _%s\n", BaseName);
    if(!(ap->Flags & AMIPRAGFLAG_PPC))
      DoOutput("\t.extern _PowerPCBase\n");
    DoOutput("\t.global _%s\n_%s:\n", name, name);
  }

  if(ap->Flags & AMIPRAGFLAG_PPC2)
  {
    DoOutput("\tstw\t%s2,20(%s1)\n"
             "\tmflr\t%s0\n"
             "\tstw\t%s0,16(%s1)\n"
             "\tlwz\t%s2,_%s(%s2)\n"
             "\tlwz\t%s0,-%d(%s2)\n"
             "\tmtlr\t%s0\n"
             "\tblrl\n"
             "\tlwz\t%s0,16(%s1)\n"
             "\tlwz\t%s2,20(%s1)\n"
             "\tmtlr\t%s0\n"
             "\tblr\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, BaseName, PPCRegPrefix, PPCRegPrefix, ap->Bias-2,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix);
  }
  else if(ap->Flags & AMIPRAGFLAG_PPC0)
  {
    DoOutput("\tmflr\t%s0\n", PPCRegPrefix);
    if((flags & FUNCFLAG_TAG) && ap->NumArgs>0)
    {
      DoOutput("\tstw\t%s%d,%d(%s1)\n"  /* store first tag */
               "\taddi\t%s%d,%s1,%d\n", /* TagItem pointer */
      PPCRegPrefix, (int)ap->NumArgs+2,
      20+(int)ap->NumArgs*4, PPCRegPrefix, PPCRegPrefix,
      (int)ap->NumArgs+2, PPCRegPrefix, 20+(int)ap->NumArgs*4);
    }
    DoOutput("\tstw\t%s0,8(%s1)\n"      /* store LR */
             "\tstwu\t%s1,-32(%s1)\n"   /* new stack frame */
             "\tlwz\t%s11,_%s(%s2)\n"
             "\tlwz\t%s0,-%d(%s11)\n"
             "\tmtlr\t%s0\n"
             "\tblrl\n"
             "\tlwz\t%s0,40(%s1)\n"
             "\taddi\t%s1,%s1,32\n"
             "\tmtlr\t%s0\n"
             "\tblr\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    BaseName, PPCRegPrefix, PPCRegPrefix, ap->Bias-2, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, 
    PPCRegPrefix);
  }
  else if(ap->Flags & AMIPRAGFLAG_PPC)
  {
    count = ap->NumArgs;
    if(Flags & FLAG_WOSLIBBASE) /* LibBase already in r3 */
    {
      /* init stack frame */
      i = (count <= 8) ? 32 : ((56+(count-8)*8+15)&~15); /* stksize */
      DoOutput("\tmflr\t%s0\n\tstw\t%s0,8(%s1)\n\tstwu\t%s1,-%ld(%s1)\n",
      PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, i, PPCRegPrefix);

      if(count > 8)
      {
        /* extra arguments must be passed on the stack */
        k = 32-(count-8); /* firstreg */

        DoOutput("\tstmw\t%s%ld,%ld(%s1)\n\tlmw\t%s%ld,%ld(%s1)\n",
        PPCRegPrefix, k, 56+(count-8)*4, PPCRegPrefix, PPCRegPrefix, k,
        i+56, PPCRegPrefix);
        if(flags & FUNCFLAG_TAG)
          DoOutput("\taddi\t%s31,%s1,%ld\n", PPCRegPrefix, PPCRegPrefix,
          i+20+count*4);
        DoOutput("\tstmw\t%s%ld,56(%s1)\n", PPCRegPrefix, k, PPCRegPrefix);
      }
      else if(flags & FUNCFLAG_TAG)
      {
        DoOutput("\taddi\t%s%ld,%s1,%ld\n", PPCRegPrefix, count+3,
        PPCRegPrefix, i+20+count*4);
        --count;
      }
    }
    else        /* Args must be shifted! */
    {
      /* init stack frame */
      i = (count < 8) ? 32 : ((56+(count-7)*8+15)&~15); /* stksize */
      DoOutput("\tmflr\t%s0\n\tstw\t%s0,8(%s1)\n\tstwu\t%s1,-%ld(%s1)\n",
      PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, i, PPCRegPrefix);

      if(count > 7)
      {
        /* extra arguments must be passed on the stack */
        if(count == 8)
        {
          /* special case: move 8th argument into stack frame */
          if(flags & FUNCFLAG_TAG)
            DoOutput("\taddi\t%s10,%s1,%ld\n", PPCRegPrefix, PPCRegPrefix,
            i+20+count*4);
          DoOutput("\tstw\t%s10,56(%s1)\n", PPCRegPrefix, PPCRegPrefix);
        }
        else
        {
          k = 32-(count-7); /* firstreg */

          DoOutput("\tstmw\t%s%ld,%ld(%s1)\n"
                   "\tmr\t%s%ld,%s10\n"
                   "\tlmw\t%s%ld,%ld(%s1)\n",
                   PPCRegPrefix, k, 56+(count-7)*4, PPCRegPrefix,
                   PPCRegPrefix, k, PPCRegPrefix, PPCRegPrefix, k+1,
                   i+56, PPCRegPrefix);
          if(flags & FUNCFLAG_TAG)
            DoOutput("\taddi\t%s31,%s1,%ld\n", PPCRegPrefix,
            PPCRegPrefix, i+20+count*4);
          DoOutput("\tstmw\t%s%ld,56(%s1)\n", PPCRegPrefix, k, PPCRegPrefix);
        }
      }
      else if(flags & FUNCFLAG_TAG)
      {
        DoOutput("\taddi\t%s%ld,%s1,%ld\n", PPCRegPrefix, count+3,
        PPCRegPrefix, i+20+count*4);
        --count;
      }

      /* shift all arguments into their following register */
      for(k=(count<8)?count:7; k > 0; --k)
        DoOutput("\tmr\t%s%ld,%s%ld\n", PPCRegPrefix, 3+k, PPCRegPrefix, 2+k);

      /* load library base and LVO, then call LVO via LR */
      DoOutput("\tlwz\t%s3,_%s(%s2)\n", PPCRegPrefix, BaseName, PPCRegPrefix);
    }

    /* call LVO */
    DoOutput("\tlwz\t%s0,-%d(%s3)\n\tmtlr\t%s0\n\tblrl\n", PPCRegPrefix,
    ap->Bias-2, PPCRegPrefix, PPCRegPrefix);

    /* cleanup stack frame and return */
    if(count > 8)
    {
      k = Flags & FLAG_WOSLIBBASE ? 8 : 7;      /* restore saved regs */
      DoOutput("\tlmw\t%s%ld,%ld(%s1)\n", PPCRegPrefix, 32-(count-k),
      56+(count-k)*4, PPCRegPrefix);
    }

    DoOutput("\taddi\t%s1,%s1,%ld\n\tlwz\t%s0,8(%s1)\n\tmtlr\t%s0\n\tblr\n",
    PPCRegPrefix, PPCRegPrefix, i, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
  }
  else
  {
    DoOutput("\tmflr\t%s0\n\tstw\t%s0,8(%s1)\n\tstwu\t%s1,-0xB0(%s1)\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

    /* clear PP_Flags, PP_Stack and PP_StackSize */
    DoOutput("\tli\t%s11,0\n\tstw\t%s11,0x28(%s1)\n\tstw\t%s11,0x2C(%s1)\n"
    "\tstw\t%s11,0x30(%s1)\n", PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

    if(Flags & FLAG_WOSLIBBASE)
      DoOutput("\tli\t%s11,-%d\n\tstw\t%s4,0x20(%s1)\n\tstw\t%s11,0x24(%s1)\n"
      "\tstw\t%s4,0x6C(%s1)\n", PPCRegPrefix, ap->Bias, PPCRegPrefix,
      PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
    else if(!BaseName)
      DoOutput("\tli\t%s11,-%d\n\tstw\t%s11,0x24(%s1)\n", PPCRegPrefix,
      ap->Bias, PPCRegPrefix, PPCRegPrefix);
    else
      DoOutput("\tlwz\t%s0,_%s(%s2)\n\tli\t%s11,-%d\n"
      "\tstw\t%s0,0x20(%s1)\n\tstw\t%s11,0x24(%s1)\n\tstw\t%s0,0x6c(%s1)\n",
      PPCRegPrefix, BaseName, PPCRegPrefix, PPCRegPrefix, ap->Bias,
      PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
      PPCRegPrefix);

    ofs = Flags & FLAG_WOSLIBBASE ? 2 : 0;
    k = ap->NumArgs - (flags & FUNCFLAG_TAG ? 1 : 0);
    for(i = 0; i < k; ++i)
    {
      if(i + ofs <= 7)
      {
        if(ap->Args[i].ArgReg == REG_A6)
          DoOutput("\tstw\t%s%ld,0x20(%s1)\n", PPCRegPrefix, i+3+ofs,
          PPCRegPrefix);
        DoOutput("\tstw\t%s%ld,", PPCRegPrefix, i+3+ofs);
      }
      else
      {
        DoOutput("\tlwz\t%s11,%ld(%s1)\n", PPCRegPrefix, (i+1+ofs)*4+196,
        PPCRegPrefix);
        if(ap->Args[i].ArgReg == REG_A6)
          DoOutput("\tstw\t%s11,0x20(%s1)\n", PPCRegPrefix, PPCRegPrefix);
        DoOutput("\tstw\t%s11,", PPCRegPrefix);
      }
      DoOutput("%d(%s1)\n", 0x34+4*ap->Args[i].ArgReg, PPCRegPrefix);
    }
    if(flags & FUNCFLAG_TAG)
    {
      if((i+ofs) <= 7 && cd->Args[i].Type != CPP_TYPE_VARARGS)
        DoOutput("\tstw\t%s%ld,%ld(%s1)\n", PPCRegPrefix, i+3+ofs,
        0xC4+(ap->NumArgs+ofs)*4, PPCRegPrefix);
      DoOutput("\taddi\t%s11,%s1,%ld\n\tstw\t%s11,", PPCRegPrefix,
      PPCRegPrefix, 0xC4+(ap->NumArgs+ofs)*4, PPCRegPrefix);
      DoOutput("%d(%s1)\n", 0x34+4*ap->Args[i].ArgReg, PPCRegPrefix);
    }

    if(!(Flags & FLAG_WOSLIBBASE))
      DoOutput("\tlwz\t%s3,_PowerPCBase(%s2)\n", PPCRegPrefix, PPCRegPrefix);

    DoOutput("\taddi\t%s4,%s1,0x20\n\tlwz\t%s0,-298(%s3)\n\tmtlr\t%s0\n"
    "\tblrl\n\tlwz\t%s3,0x34(%s1)\n\taddi\t%s1,%s1,0xB0\n\tlwz\t%s0,8(%s1)\n"
    "\tmtlr\t%s0\n\tblr\n", PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
  }

  if(Flags & FLAG_WOSLIBBASE)
    return DoOutput("\t.type\t__%s,@function\n\t.size\t__%s,$-__%s\n\n",
    name, name, name);
  else
    return DoOutput("\t.type\t_%s,@function\n\t.size\t_%s,$-_%s\n\n",
    name, name, name);
}

uint32 FuncVBCCWOSCode(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint32 i, j, k, ofs, count;
  uint8 *data, *basepos = 0, *pbasepos = 0;
  struct ClibData *cd = 0;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG))
    return 1;

  if((flags & FUNCFLAG_TAG) && !(ap->Flags & AMIPRAGFLAG_PPC) &&
  !(cd = GetClibFunc(name, ap, flags)))
    return 1;

  if((ap->Flags & AMIPRAGFLAG_PPC) && !BaseName &&
  ((ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2)) ||
  !(Flags & FLAG_WOSLIBBASE)))
  {
    DoError(ERR_MISSING_BASENAME, ap->Line);
    return 1;
  }

  Flags |= FLAG_DONE; /* We did something */

  i = strlen(name) + 2;
  EndPutM32(tempbuf, HUNK_UNIT);
  EndPutM32(tempbuf+4, (i+3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutput("%s.o", name);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  i = strlen(hunkname);
  EndPutM32(tempbuf, HUNK_NAME);
  EndPutM32(tempbuf+4, (i + 3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutputDirect(hunkname, i);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  data = tempbuf+8; /* we need HUNK_PPC_CODE + size at start */

  if(ap->Flags & AMIPRAGFLAG_PPC2)
  {
    EndPutM32Inc(data, 0x90410014);             /* stw r2,20(r1) */
    /* mflr r0 = mfspr r0,8 = get link register */
    EndPutM32Inc(data, 0x7C0802A6);
    EndPutM32Inc(data, 0x90010010);             /* stw r0,16(r1) */
    basepos = data;
    EndPutM32Inc(data, 0x80420000);             /* lwz r2,BaseName(r2) */
    EndPutM32Inc(data, 0x80030000-(ap->Bias-2));/* lwz r0,-ap->Bias-2(r2) */
    /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x7C0803A6);
    EndPutM32Inc(data, 0x4E800021);             /* blrl = bclrl 20,0 = jump */
    EndPutM32Inc(data, 0x80010010);             /* lwz r0,16(r1) */
    EndPutM32Inc(data, 0x80410014);             /* lwz r2,20(r1) */
    /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x7C0803A6);
    EndPutM32Inc(data, 0x4E800020);             /* blr = bclr 20,0 = jump */
  }
  else if(ap->Flags & AMIPRAGFLAG_PPC0)
  {
    basepos = data;
    /* mflr r0 = mfspr r0,8 = get link register */
    EndPutM32Inc(data, 0x7C0802A6);
    if((flags & FUNCFLAG_TAG) && ap->NumArgs>0)
    {
      EndPutM32Inc(data, 0x90010000 + (((uint32)ap->NumArgs+2) << 21) +
                   (uint32)(20+ap->NumArgs*4)); /* stw rN,d(r1) */
      EndPutM32Inc(data, 0x38010000 + (((uint32)ap->NumArgs+2) << 21) +
                   (uint32)(20+ap->NumArgs*4)); /* addi rN,r1,d */
    }
    EndPutM32Inc(data, 0x90010008);             /* stw r0,8(r1) */
    EndPutM32Inc(data, 0x9421FFCE);             /* stwu r1,-32(r1) */
    EndPutM32Inc(data, 0x81620000);             /* lwz r11,BaseName(r2) */
    EndPutM32Inc(data, 0x800C0000-(ap->Bias-2));/* lwz r0,-ap->Bias-2(r11) */
    EndPutM32Inc(data, 0x7C0803A6);             /* mtlr r0 = mtspr 8,r0 = store link register */
    EndPutM32Inc(data, 0x4E800021);             /* blrl = bclrl 20,0 = jump */
    EndPutM32Inc(data, 0x80010028);             /* lwz r0,40(r1) */
    EndPutM32Inc(data, 0x38210020);             /* addi r1,r1,32 */
    /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x7C0803A6);
    EndPutM32Inc(data, 0x4E800020);             /* blr = bclr 20,0 = jump */
  }
  else if(ap->Flags & AMIPRAGFLAG_PPC)
  {
    count = ap->NumArgs;
    if(Flags & FLAG_WOSLIBBASE) /* LibBase already in r3 */
    {
      /* init stack frame */
      i = (count <= 8) ? 32 : ((56+(count-8)*8+15)&~15);        /* stksize */
      /* mflr r0 = mfspr r0,8 = get link register */
      EndPutM32Inc(data, 0x7C0802A6);
      EndPutM32Inc(data, 0x90010008);                      /* stw r0,8(r1) */
      EndPutM32Inc(data, 0x94220000 - i);                /* stwu r1,-i(r1) */

      if(count > 8)
      {
        /* extra arguments must be passed on the stack */
        k = 32-(count-8);                                      /* firstreg */
        /* stmw rk,X(r1) */
        EndPutM32Inc(data, 0xBC010000 + (k << 21) + (56+(count-8)*4));
        EndPutM32Inc(data, 0xB8010000 + (k << 21) + (i+56)); /* lmw rk,Y(r1) */
        if(flags & FUNCFLAG_TAG)
          EndPutM32Inc(data, 0x3BE10000 + (i+20+count*4));  /* addi r31,r1,X */
        EndPutM32Inc(data, 0xBC010038 + (k << 21));        /* stmw rk,56(r1) */
      }
      else if(flags & FUNCFLAG_TAG)
      {
        /* addi rX,r1,Y */
        EndPutM32Inc(data, 0x38010000 + ((count+3)<<21) + (i+20+count*4));
        --count;
      }
    }
    else        /* Args must be shifted! */
    {
      /* init stack frame */
      i = (count < 8) ? 32 : ((56+(count-7)*8+15)&~15);         /* stksize */
      /* mflr r0 = mfspr r0,8 = get link register */
      EndPutM32Inc(data, 0x7C0802A6);
      EndPutM32Inc(data, 0x90010008);                      /* stw r0,8(r1) */
      EndPutM32Inc(data, 0x94220000 - i);                /* stwu r1,-i(r1) */

      if(count > 7)
      {
        /* extra arguments must be passed on the stack */
        if(count == 8)
        {
          /* special case: move 8th argument into stack frame */
          if(flags & FUNCFLAG_TAG)
            EndPutM32Inc(data, 0x39410000 + (i+20+count*4)); /* addi r10,r1,X */
          EndPutM32Inc(data, 0x91410038);                /* stw r10,56(r1) */
        }
        else
        {
          k = 32-(count-7); /* firstreg */

          /* stmw rk,X(r1) */
          EndPutM32Inc(data, 0xBC010000 + (k << 21) + (56+(count-7)*4));
          /* mr rk,r10 = or rk,r10,r10 */
          EndPutM32Inc(data, 0x7D405378 + (k<<16));
          /* lmw rk,Y(r1) */
          EndPutM32Inc(data, 0xB8010000 + ((k+1) << 21) + (i+56));
          if(flags & FUNCFLAG_TAG)
          {
            /* addi r31,r1,X */
            EndPutM32Inc(data, 0x3BE10000 + (i+20+count*4));
          }
          EndPutM32Inc(data, 0xBC010038 + (k << 21));     /* stmw rk,56(r1) */
        }
      }
      else if(flags & FUNCFLAG_TAG)
      {
        /* addi rX,r1,Y */
        EndPutM32Inc(data, 0x38010000 + ((count+3)<<21) + (i+20+count*4));
        --count;
      }

      /* shift all arguments into their following register */
      for(k=(count<8)?count:7; k > 0; --k)
        EndPutM32Inc(data, 0x7C000378 + ((3+k)<<16) + ((2+k)<<21) + ((2+k)<<11)); /* mr rX,rY = or rX,rY,rY */

      /* load library base and LVO, then call LVO via LR */
      basepos = data;
      EndPutM32Inc(data, 0x80620000);                                   /* lwz r3,BaseName(r2) */
    }
    /* call LVO */
    EndPutM32Inc(data, 0x80040000 - (ap->Bias-2));                      /* lwz r0,-(ap->Bias-2)(r3) */
    EndPutM32Inc(data, 0x7C0803A6);                                     /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x4E800021);                                     /* blrl = bclrl 20,0 = jump */


    /* cleanup stack frame and return */
    if(count > 8)
    {
      k = Flags & FLAG_WOSLIBBASE ? 8 : 7;      /* restore saved regs */
      EndPutM32Inc(data, 0xB8010000 + ((32-(count-k))<<21) + (56+(count-k)*4)); /* lmw rX,Y(r1) */
    }
    EndPutM32Inc(data, 0x38210000 + i);         /* addi r1,r1,i */
    EndPutM32Inc(data, 0x80010008);             /* lwz r0,8(r1) */
    EndPutM32Inc(data, 0x7C0803A6);             /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x4E800020);             /* blr = bclr 20,0 = jump */
  }
  else
  {
    EndPutM32Inc(data, 0x7C0802A6);             /* mflr r0 = mfspr r0,8 = get link register */
    EndPutM32Inc(data, 0x90010008);             /* stw r0,8(r1) = save link register in 8(r1) */
    EndPutM32Inc(data, 0x9421FF50);             /* stwu r1,-0xB0(r1) = store word from r1 in -0xB0(r1) and update r1 */

    EndPutM32Inc(data, 0x39600000);             /* li r11,0 = addi r11,r0,0 = clear r11 */
    EndPutM32Inc(data, 0x91610028);             /* stwu r11,0x28(r1) = clear PP_Flags */
    EndPutM32Inc(data, 0x9161002C);             /* stwu r11,0x2C(r1) = clear PP_Stack */
    EndPutM32Inc(data, 0x91610030);             /* stwu r11,0x30(r1) = clear PP_StackSize */

    if(Flags & FLAG_WOSLIBBASE)
    {
      EndPutM32Inc(data, 0x39610000 -ap->Bias); /* li r11,ap->Bias */
      EndPutM32Inc(data, 0x90810020);           /* stw r4,0x20(r1) = set PP_Code to Librarybase */
      EndPutM32Inc(data, 0x91610024);           /* stw r11,0x24(r1) = set PP_Offset to Bias value */
      EndPutM32Inc(data, 0x9081006C);           /* stw r4,0x6C(r1) = set A6 register */
    }
    else if(!BaseName)
    {
      EndPutM32Inc(data, 0x39610000 -ap->Bias); /* li r11,ap->Bias */
      EndPutM32Inc(data, 0x91610024);           /* stw r11,0x24(r1) = set PP_Offset to Bias value */
    }
    else
    {
      basepos = data;
      EndPutM32Inc(data, 0x80020000);           /* lwz r0,BaseName(r2)  --> 16BIT RELOC! */
      EndPutM32Inc(data, 0x39610000 -ap->Bias); /* li r11,ap->Bias */
      EndPutM32Inc(data, 0x90010020);           /* stw r0,0x20(r1) = set PP_Code to Librarybase */
      EndPutM32Inc(data, 0x91610024);           /* stw r11,0x24(r1) = set PP_Offset to Bias value */
      EndPutM32Inc(data, 0x9001006C);           /* stw r4,0x6C(r1) = set A6 register */
    }
  
    ofs = Flags & FLAG_WOSLIBBASE ? 2 : 0;
    k = ap->NumArgs - (flags & FUNCFLAG_TAG ? 1 : 0);
    for(i = 0; i < k; ++i)
    {
      j = 0x34+4*ap->Args[i].ArgReg; /* PP_Regs offset */
      if(i + ofs <= 7)
      {
        if(ap->Args[i].ArgReg == REG_A6)
          EndPutM32Inc(data, 0x90010020 + ((i+3+ofs)<<21));     /* stw rX,0x20(r1) */
        EndPutM32Inc(data, 0x90010000 + ((i+3+ofs)<<21) + j);   /* stw rX,j(r1) */
      }
      else
      {
        EndPutM32Inc(data, 0x81610000 + ((i+1+ofs)*4+0xC4));    /* lwz r11,X(r1) = get data from stack */
        if(ap->Args[i].ArgReg == REG_A6)
          EndPutM32Inc(data, 0x91610020);                       /* stw r11,0x20(r1) */
        EndPutM32Inc(data, 0x91610000 + j);                     /* stw r11,j(r1) */
      }
    }
    if(flags & FUNCFLAG_TAG)
    {
      j = (ap->NumArgs+ofs)*4+0xC4;

      if((i+ofs) <= 7 && cd->Args[i].Type != CPP_TYPE_VARARGS)
        EndPutM32Inc(data, 0x90010000 + ((i+3+ofs)<<21) + j);           /* stw rX,j(r1) */
      EndPutM32Inc(data, 0x39610000 + j);                               /* addi r11,r1,j */
      EndPutM32Inc(data, 0x91610000 + (0x34+4*ap->Args[i].ArgReg));     /* stw r11,X(r1) */
    }

    if(!(Flags & FLAG_WOSLIBBASE))
    {
      pbasepos = data;                  /* store 16BIT reloc offset */
      EndPutM32Inc(data, 0x80620000);   /* lwz r3,_PowerPCBase(r2) = get librarybase */
    }
    EndPutM32Inc(data, 0x38810020);     /* addi r4,r1,0x20 = {r4 := 0x20(r1)} */
    EndPutM32Inc(data, 0x8003FED6);     /* lwz r0,-298(r3) = load jumpin base */
    EndPutM32Inc(data, 0x7C0803A6);     /* mtlr r0 = mtspr 8,r0 = store link register */
    EndPutM32Inc(data, 0x4E800021);     /* blrl = bclrl 20,0 = jump */
    EndPutM32Inc(data, 0x80610034);     /* lwz r3,0x34(r1) = get result D0 */
    EndPutM32Inc(data, 0x382100B0);     /* addi r1,r1,0xB0 = free PRCArgs structure */
    EndPutM32Inc(data, 0x80010008);     /* lwz r0,8(r1) = get old link register */
    EndPutM32Inc(data, 0x7C0803A6);     /* mtlr r0 = mtspr 8,r0 = restore link register */
    EndPutM32Inc(data, 0x4E800020);     /* blr = bclr 20,0 = jump back */
  }

  EndPutM32(tempbuf, HUNK_PPC_CODE);
  EndPutM32(tempbuf+4, (data-tempbuf-8)>>2)
  DoOutputDirect(tempbuf, (data-tempbuf)&(~3));

  EndPutM32(tempbuf, HUNK_EXT);
  DoOutputDirect(tempbuf,4);

  /* here come the XDEF name references */

  if(Flags & FLAG_WOSLIBBASE)
  {
    if(ap->Flags & (AMIPRAGFLAG_PPC0|AMIPRAGFLAG_PPC2))
      OutputXREF((basepos-tempbuf-8)+2, EXT_DEXT16, "_%s", BaseName);
    OutputXDEF(0, "__%s", name);
  }
  else
  {
    if(BaseName)
      OutputXREF((basepos-tempbuf-8)+2, EXT_DEXT16, "_%s", BaseName);
    if(!(ap->Flags & AMIPRAGFLAG_PPC))
      OutputXREF((pbasepos-tempbuf-8)+2, EXT_DEXT16, "_PowerPCBase");
    OutputXDEF(0, "_%s", name);
  }
  EndPutM32(tempbuf, 0);
  DoOutputDirect(tempbuf,4);
  if(!(Flags & FLAG_NOSYMBOL))
  {
    EndPutM32(tempbuf, HUNK_SYMBOL);
    DoOutputDirect(tempbuf,4);
    if(Flags & FLAG_WOSLIBBASE)
      OutputSYMBOL(0, "__%s", name);
    else
      OutputSYMBOL(0, "_%s", name);
    EndPutM32(tempbuf, 0);
    DoOutputDirect(tempbuf,4);
  }
  EndPutM32(tempbuf, HUNK_END);

  return DoOutputDirect(tempbuf,4);
}

uint32 FuncVBCCPUPText(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE;

  if(Flags & FLAG_SINGLEFILE)
  {
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
  }

  if(Flags & (FLAG_ASMSECTION|FLAG_SINGLEFILE))
    DoOutput("\t.section %s,\"acrx4\"\n", hunkname);

  if(Flags & FLAG_SINGLEFILE)
    DoOutput("\t.file\t\"%s.o\"\n", name);
  if(BaseName)
    DoOutput("\t.global %s\n", BaseName);
  DoOutput("\t.global PPCCallOS\n\t.global %s\n"
  "\t.align\t3\n%s:\n",name, name);

  if(flags & FUNCFLAG_TAG)
  {
    /* Hack the stack-frame for varargs.
       Build stack-frame, but save LR in our own stack-frame,
       because we have to overwrite the lower 8 bytes of the
       caller's frame. */
    DoOutput("\tstwu\t%s1,-128(%s1)\n\tmflr\t%s11\n\tstw\t%s11,100(%s1)\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

    /* Save the caller's saved SP in our own stack-frame. */
    DoOutput("\tlwz\t%s11,128(%s1)\n\tstw\t%s11,96(%s1)\n", PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

    /* Store r3-r8 at the top of our stack-frame and r9-r10
       at the low 8 bytes of the caller's frame. This way all
       arguments will reside in one continuous area. */
    for(i=3+ap->NumArgs-1; i <= 10; ++i)
      DoOutput("\tstw\t%s%ld,%ld(%s1)\n", PPCRegPrefix, i, 104+4*(i-3),
      PPCRegPrefix);
  }
  else
    DoOutput("\tstwu\t%s1,-96(%s1)\n\tmflr\t%s11\n\tstw\t%s11,100(%s1)\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

  for(i = 0; i < ap->NumArgs; ++i)
  {
    if(!(flags & FUNCFLAG_TAG) || i < ap->NumArgs-1)
    {
      if(i <= 7)
        DoOutput("\tstw\t%s%ld,", PPCRegPrefix, i+3);
      else
        DoOutput("\tlwz\t%s11,%ld(%s1)\n\tstw\t%s11,", PPCRegPrefix,
        100+(i+1-8)*4, PPCRegPrefix, PPCRegPrefix);
    }
    else
      DoOutput("\taddi\t%s11,%s1,%d\n\tstw\t%s11,", PPCRegPrefix,
      PPCRegPrefix, 100+ap->NumArgs*4, PPCRegPrefix);
    DoOutput("%d(%s1)\n", 36+4*ap->Args[i].ArgReg, PPCRegPrefix);
  }

  /* Now place the real function call */
  /* store offset in Chaos->caos_Un.Offset */
  DoOutput("\tli\t%s11,-%d\n\tstw\t%s11,8(%s1)\n"
  "\tli\t%s11,1\n\tstw\t%s11,12(%s1)\n\tstw\t%s11,24(%s1)\n", PPCRegPrefix,
  ap->Bias, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
  /* set M68kCacheMode and PPCCacheMode to IF_CACHEFLUSHALL */

  if(BaseName)
  {
    if(Flags & FLAG_SMALLDATA)
      DoOutput("\tlwz\t%s11,%s@sdarx(%s13)\n", PPCRegPrefix, BaseName,
      PPCRegPrefix);
    else
      DoOutput("\tlis\t%s11,%s@ha\n\tlwz\t%s11,%s@l(%s11)\n", PPCRegPrefix,
      BaseName, PPCRegPrefix, BaseName, PPCRegPrefix);
    /* store basepointer in A6 */
    DoOutput("\tstw\t%s11,92(%s1)\n", PPCRegPrefix, PPCRegPrefix);
  }

  DoOutput("\taddi\t%s3,%s1,8\n\tbl\tPPCCallOS\n", PPCRegPrefix, PPCRegPrefix);
  if(flags & FUNCFLAG_TAG) /* Varargs. Rebuild the caller's stack-frame. */
    DoOutput("\tlwz\t%s11,96(%s1)\n\tstw\t%s11,128(%s1)\n"
    "\tlwz\t%s11,100(%s1)\n\tmtlr\t%s11\n\taddi\t%s1,%s1,128\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);
  else
    DoOutput("\tlwz\t%s11,100(%s1)\n\tmtlr\t%s11\n\taddi\t%s1,%s1,96\n",
    PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

  return DoOutput("\tblr\n\t.type\t%s,@function\n\t.size\t%s,$-%s\n\n", name,
  name, name);
}

uint32 FuncVBCCPUPCode(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i, j=0, k, size;
  uint8 *data, *data2, *data3;
  struct ArHeader *arh;

  data = tempbuf;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE;

  *(data++) = 0x7F;                                             /* eeh->e_ident[EI_MAG0] */
  *(data++) = 'E';                                              /* eeh->e_ident[EI_MAG1] */
  *(data++) = 'L';                                              /* eeh->e_ident[EI_MAG2] */
  *(data++) = 'F';                                              /* eeh->e_ident[EI_MAG3] */
  *(data++) = ELFCLASS32;                                       /* eeh->e_ident[EI_CLASS] */
  *(data++) = ELFDATA2MSB;                                      /* eeh->e_ident[EI_DATA] */
  *(data++) = EV_CURRENT;                                       /* eeh->e_ident[EI_VERSION] */
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  EndPutM16Inc(data, ET_REL);                                   /* eeh->e_type */
  EndPutM16Inc(data, EM_POWERPC);                               /* eeh->e_machine */
  EndPutM32Inc(data, EV_CURRENT);                               /* eeh->e_version */
  EndPutM32Inc(data, 0);                                        /* eeh->e_entry */
  EndPutM32Inc(data, 0);                                        /* eeh->e_phoff */
  data2 = data; data += 4;
  EndPutM32Inc(data, 0);                                        /* eeh->e_flags */
  EndPutM16Inc(data, 52);                                       /* eeh->e_ehsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phentsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phnum */
  EndPutM16Inc(data, 40);                                       /* eeh->e_shentsize */
  EndPutM16Inc(data, 6);                                        /* eeh->e_shnum */
  EndPutM16Inc(data, 3);                                        /* eeh->e_shstrndx - fourth table is string table */

  data3 = data;
  if(flags & FUNCFLAG_TAG)
  {
    /* Hack the stack-frame for varargs.
       Build stack-frame, but save LR in our own stack-frame,
       because we have to overwrite the lower 8 bytes of the
       caller's frame. */
    EndPutM32Inc(data, 0x9421FF80);                             /* stwu r1,-128(r1) */
    EndPutM32Inc(data, 0x7D6802A6);                             /* mflr r11 = mfspr r11,8 = get link register */
    EndPutM32Inc(data, 0x91610064);                             /* stw r11,100(r1) */

    /* Save the caller's saved SP in our own stack-frame. */
    EndPutM32Inc(data, 0x81610080);                             /* lwz r11,128(r1) */
    EndPutM32Inc(data, 0x91610060);                             /* stw r11,96(r1) */

    /* Store r3-r8 at the top of our stack-frame and r9-r10
       at the low 8 bytes of the caller's frame. This way all
       arguments will reside in one continuous area. */
    for(i=3+ap->NumArgs-1; i <= 10; ++i)
      EndPutM32Inc(data, 0x90010000 + (i<<21) + (104+4*(i-3))); /* stw rX,Y(r1) */
  }
  else
  {
    EndPutM32Inc(data, 0x9421FFA0);                             /* stwu r1,-96(r1) */
    EndPutM32Inc(data, 0x7D6802A6);                             /* mflr r11 = mfspr r11,8 = get link register */
    EndPutM32Inc(data, 0x91610064);                             /* stw r11,100(r1) */
  }

  for(i = 0; i < ap->NumArgs; ++i)
  {
    j = 36+4*ap->Args[i].ArgReg;
    if(!(flags & FUNCFLAG_TAG) || i < ap->NumArgs-1)
    {
      if(i <= 7)
      {
        EndPutM32Inc(data, 0x90010000 + ((i+3)<<21) + j);       /* stw rX,j(r1) */
      }
      else
      {
        EndPutM32Inc(data, 0x81610000 + (100+(i+1-8)*4));       /* lwz r11,X(r1) = get data from stack */
        EndPutM32Inc(data, 0x91610000 + j);                     /* stw r11,j(r1) */
      }
    }
    else
    {
      EndPutM32Inc(data, 0x39610000 + (100+ap->NumArgs*4));     /* addi r11,r1,X */
      EndPutM32Inc(data, 0x91610000 + j);                       /* stw r11,X(r1) */
    }
  }

  /* Now place the real function call */
  EndPutM32Inc(data, 0x39610000 - ap->Bias);                    /* li r11,-(ap->Bias) = addi r11,0,-ap->Bias */
  EndPutM32Inc(data, 0x91610008);                               /* stw r11,8(r1) */
  EndPutM32Inc(data, 0x39600001);                               /* li r11,1 = addi r11,0,1 = get IF_CACHEFLUSHALL */
  EndPutM32Inc(data, 0x9161000C);                               /* stw r11,12(r1) = set M68kCacheMode */
  EndPutM32Inc(data, 0x91610018);                               /* stw r11,24(r1) = set PPCCacheMode */

  if(BaseName)
  {
    if(Flags & FLAG_SMALLDATA)
    {
      j = (data-data3)+2;                                       /* store reloc offset */
      EndPutM32Inc(data, 0x816D0000);                           /* lwz r11,BaseName@sdarx(r13) */
    }
    else
    {
      j = (data-data3)+2;                                       /* store reloc offset */
      EndPutM32Inc(data, 0x3D600000);                           /* lis r11,BaseName@ha = addis r11,0,BaseName@ha */
      EndPutM32Inc(data, 0x816B0000);                           /* lwz r11,BaseName@l(r11) */
    }
    EndPutM32Inc(data, 0x9161005C);                             /* stw r11,92(r1) */
  }

  EndPutM32Inc(data, 0x38610008);                               /* addi r3,r1,8 */
  k = (data-data3);                                             /* store reloc offset */
  EndPutM32Inc(data, 0x48000001);                               /* bl PPCCallOS */
  if(flags & FUNCFLAG_TAG)                                      /* Varargs. Rebuild the caller's stack-frame. */
  {
    EndPutM32Inc(data, 0x81610060);                             /* lwz r11,96(r1) */
    EndPutM32Inc(data, 0x91610080);                             /* stw r11,128(r1) */
    EndPutM32Inc(data, 0x81610064);                             /* lwz r11,100(r1) */
    EndPutM32Inc(data, 0x7D6803A6);                             /* mtlr r11 = mtspr 8,r11 = restore link register */
    EndPutM32Inc(data, 0x38210080);                             /* addi r1,r1,128 */
  }
  else
  {
    EndPutM32Inc(data, 0x81610064);                             /* lwz r11,100(r1) */
    EndPutM32Inc(data, 0x7D6803A6);                             /* mtlr r11 = mtspr 8,r11 = restore link register */
    EndPutM32Inc(data, 0x38210060);                             /* addi r1,r1,96 */
  }

  EndPutM32Inc(data, 0x4E800020);                               /* blr = bclr 20,0 */

  memcpy(data, "\0.symtab\0.strtab\0.shstrtab\0.text\0.rela.text\0", 44);
  data += 44;  /* 1        9        17         27     33 */
  
  EndPutM32(data2, data-tempbuf);                               /* eeh->e_shoff */
  data2 = data-44;

  EndPutM32Inc(data, 0);                                        /* esh[0].sh_name */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_offset */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  size = data2-data3;
  EndPutM32Inc(data, 27);                                       /* esh[1].sh_name = .text */
  EndPutM32Inc(data, SHT_PROGBITS);                             /* esh[1].sh_type */
  EndPutM32Inc(data, SHF_ALLOC|SHF_EXECINSTR);                  /* esh[1].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_addr */
  EndPutM32Inc(data, data3-tempbuf);                            /* esh[1].sh_offset */
  EndPutM32Inc(data, size);                                     /* esh[1].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_info */
  EndPutM32Inc(data, 16);                                       /* esh[1].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_entsize */

  data3 = data;
  EndPutM32Inc(data, 33);                                       /* esh[2].sh_name = .rela.text */
  EndPutM32Inc(data, SHT_RELA);                                 /* esh[2].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[2].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[2].sh_addr */
  data += 4;                                                    /* esh[2].sh_offset */
  data += 4;                                                    /* esh[2].sh_size */
  EndPutM32Inc(data, 4);                                        /* esh[2].sh_link - the fifth entry is symbol table */
  EndPutM32Inc(data, 1);                                        /* esh[2].sh_info - the second entry is programm data */
  EndPutM32Inc(data, 4);                                        /* esh[2].sh_addralign */
  EndPutM32Inc(data, 12);                                       /* esh[2].sh_entsize - sizeof(struct Elf32_Rela) */

  EndPutM32Inc(data, 17);                                       /* esh[3].sh_name = .shstrtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[3].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_addr */
  EndPutM32Inc(data, data2-tempbuf);                            /* esh[3].sh_offset */
  EndPutM32Inc(data, 44);                                       /* esh[3].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[3].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_entsize */

  EndPutM32Inc(data, 1);                                        /* esh[4].sh_name = .symtab */
  EndPutM32Inc(data, SHT_SYMTAB);                               /* esh[4].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_addr */
  data += 4;                                                    /* esh[4].sh_offset */
  data += 4;                                                    /* esh[4].sh_size */
  EndPutM32Inc(data, 5);                                        /* esh[4].sh_link - the sixth entry is our string table */
  EndPutM32Inc(data, 3);                                        /* esh[4].sh_info - One greater than index of last LOCAL symbol*/
  EndPutM32Inc(data, 4);                                        /* esh[4].sh_addralign */
  EndPutM32Inc(data, 16);                                       /* esh[4].sh_entsize = sizeof(struct Elf32_Sym) */

  EndPutM32Inc(data, 9);                                        /* esh[0].sh_name = .strtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  data += 4;                                                    /* esh[0].sh_offset */
  data += 4;                                                    /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  EndPutM32(data3+(2*40)+(4*4), data-tempbuf);                  /* esh[4].sh_offset */
  EndPutM32(data3+(2*40)+(5*4), BaseName ? 6*16 : 5*16);        /* esh[4].sh_size */

  data2 = data;
  data += BaseName ? 6*16 : 5*16;

  EndPutM32(data3+(3*40)+(4*4), data-tempbuf);                  /* esh[5].sh_offset */

  i = 0;
  EndPutM32Inc(data2, i);                                       /* esym[0].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[0].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[0].st_size */
  *(data2++) = 0;                                               /* esym[0].st_info */
  *(data2++) = 0;                                               /* esym[0].st_other */
  EndPutM16Inc(data2, 0);                                       /* esym[0].st_shndx */
  data[0] = 0;

  i += 1;
  EndPutM32Inc(data2, i);                                       /* esym[1].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[1].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[1].st_size */
  *(data2++) = ELF32_ST_INFO(STB_LOCAL,STT_FILE);               /* esym[1].st_info */
  *(data2++) = 0;                                               /* esym[1].st_other */
  EndPutM16Inc(data2, SHN_ABS);                                 /* esym[1].st_shndx */

  sprintf((strptr)data+i, "%s.o", name); while(data[i]) { i++;} ; /* get next store space */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_size */
  *(data2++) = ELF32_ST_INFO(STB_LOCAL,STT_SECTION);            /* esym[2].st_info */
  *(data2++) = 0;                                               /* esym[2].st_other */
  EndPutM16Inc(data2, 1);                                       /* esym[2].st_shndx - the second entry is program section! */

  EndPutM32Inc(data2, i);                                       /* esym[3].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[3].st_value */
  EndPutM32Inc(data2, size);                                    /* esym[3].st_size */
  *(data2++) = ELF32_ST_INFO(STB_GLOBAL,STT_FUNC);              /* esym[3].st_info */
  *(data2++) = 0;                                               /* esym[3].st_other */
  EndPutM16Inc(data2, 1);                                       /* esym[3].st_shndx - the second entry is program section! */

  sprintf((strptr)data+i, "%s", name); while(data[i]) { i++;} ; /* get next store space */
  EndPutM32Inc(data2, i);                                       /* esym[4].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[4].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[4].st_size */
  *(data2++) = ELF32_ST_INFO(STB_GLOBAL,STT_NOTYPE);            /* esym[4].st_info */
  *(data2++) = 0;                                               /* esym[4].st_other */
  EndPutM16Inc(data2, 0);                                       /* esym[4].st_shndx */

  sprintf((strptr)data+i, "PPCCallOS"); while(data[i]) { i++;} ; /* get next store space */
  if(BaseName)
  {
    EndPutM32Inc(data2, i);                                     /* esym[5].st_name */
    EndPutM32Inc(data2, 0);                                     /* esym[5].st_value */
    EndPutM32Inc(data2, 0);                                     /* esym[5].st_size */
    *(data2++) = ELF32_ST_INFO(STB_GLOBAL,STT_NOTYPE);          /* esym[5].st_info */
    *(data2++) = 0;                                             /* esym[5].st_other */
    EndPutM16/*Inc*/(data2, 0);                                 /* esym[5].st_shndx */

    sprintf((strptr)data+i, "%s", BaseName); while(data[i]) { i++;} ; /* get next store space */
  }
  EndPutM32(data3+(3*40)+(5*4), i);                             /* esh[5].sh_size */
  while(i&3) /* long aligned */
   data[i++] = 0;
  data += i;

  EndPutM32(data3+(4*4), data-tempbuf);                         /* esh[2].sh_offset */

  data2 = data;

  EndPutM32Inc(data, k);                                        /* erel[0].r_offset */
  EndPutM32Inc(data, ELF32_R_INFO(4,R_PPC_REL24));              /* erel[0].r_info - entry 4, type 10 */
  EndPutM32Inc(data, 0);                                        /* erel[0].r_addend */

  if(BaseName)
  {
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM32Inc(data, j);                                    /* erel[1].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(5,R_PPC_SDAREL16));       /* erel[1].r_info - entry 5, type 32 */
      EndPutM32Inc(data, 0);                                    /* erel[1].r_addend */
    }
    else
    {
      EndPutM32Inc(data, j);                                    /* erel[1].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(5,R_PPC_ADDR16_HA));      /* erel[1].r_info - entry 5, type 6 */
      EndPutM32Inc(data, 0);                                    /* erel[1].r_addend */
      EndPutM32Inc(data, j+4);                                  /* erel[2].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(5,R_PPC_ADDR16_LO));      /* erel[2].r_info - entry 5, type 4 */
      EndPutM32Inc(data, 0);                                    /* erel[2].r_addend */
    }
  }
  EndPutM32(data3+(5*4), data-data2);                           /* esh[2].sh_size */

  /* make ar header and store all */
  arh = (struct ArHeader *) (data+20);
  memset(arh, ' ', sizeof(struct ArHeader));

  arh->ar_time[sprintf(arh->ar_time, "%lu", (uint32) time(0))] = ' ';
  arh->ar_uid[0] = arh->ar_gid[0] = arh->ar_mode[1] =
  arh->ar_mode[2] = '0';
  arh->ar_mode[0] = '6';
  arh->ar_fmag[0] = 96;
  arh->ar_fmag[1] = '\n';

  if((k = strlen(name) + 2) >= 16)
  {
    arh->ar_name[sprintf(arh->ar_name, "#1/%ld", k)] = ' ';
  }
  else
  {
    k = 0;
    arh->ar_name[sprintf(arh->ar_name, "%s.o", name)] = ' ';
  }

  j = k + (data-tempbuf);
  for(i = 9; j; --i) /* make decimal number */
  {
    data[i] = (j%10)+'0';
    j /= 10;
  }
  for(j = 0; i < 9; ++j)
    arh->ar_size[j] = data[++i];
  
  DoOutputDirect(arh, sizeof(struct ArHeader));

  if(k)
  {
    DoOutput("%s.o", name);
    if(k & 1)
      *(data++) = 0x0A; /* alignment byte! */
  }

  return DoOutputDirect(tempbuf, data-tempbuf);
}

uint32 FuncVBCCMorphText(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i, nrcopyar = 0, stcksize = 16, basereg = 12;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE;

  if(Flags & FLAG_SINGLEFILE)
  {
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
  }

  if(Flags & (FLAG_ASMSECTION|FLAG_SINGLEFILE))
    DoOutput("\t.section %s,\"acrx4\"\n", hunkname);

  if(Flags & FLAG_SINGLEFILE)
    DoOutput("\t.file\t\"%s.o\"\n", name);
  if(BaseName)
    DoOutput("\t.global %s\n", BaseName);
  DoOutput("\t.global %s\n\t.align\t4\n%s:\n",name, name);

  if(ap->Flags & (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12))
  {
    if(Flags & FLAG_SMALLDATA)
      DoOutput("\tlwz\t%s12,%s@sdarx(%s13)\n",
               PPCRegPrefix, BaseName, PPCRegPrefix);
    else
      DoOutput("\tlis\t%s11,%s@ha\n"
               "\tlwz\t%s12,%s@l(%s11)\n",
               PPCRegPrefix, BaseName, PPCRegPrefix, BaseName, PPCRegPrefix);

    DoOutput("\tlwz\t%s0,-%d(%s12)\n\tmtctr\t%s0\n\tbctr\n",
             PPCRegPrefix, ap->Bias-2, PPCRegPrefix, PPCRegPrefix);
  }

  else
  {
    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      if(flags & FUNCFLAG_TAG)
      {
        DoOutput(
        "\tmflr\t%s0\n"
        "\tlwz\t%s11,0(%s1)\n"    /* backchain to next frame */
        "\tsub\t%s12,%s11,%s1\n"  /* difference = size of frame to copy */
        "\tstw\t%s0,4(%s1)\n"
        "\tsub\t%s11,%s1,%s12\n"
        "\tsubi\t%s11,%s11,16\n"  /* r11 Start of new frame, +16 size */
        "\tstw\t%s1,0(%s11)\n"    /* Backchain to last frame */
        "\tsrwi\t%s12,%s12,2\n"
        "\tsubi\t%s0,%s12,2\n"    /* size/4-2 = number of longwords to copy */
        "\taddi\t%s12,%s1,4\n"
        "\tmr\t%s1,%s11\n"        /* new stack frame */
        "\taddi\t%s11,%s11,8\n"
        "\tmtctr\t%s0\n"
        ".copyloop_%s:\n"
        "\tlwzu\t%s0,4(%s12)\n"   /* copy stack frame with offset 8 */
        "\tstwu\t%s0,4(%s11)\n"
        "\tbdnz\t.copyloop_%s\n"
        "\tstw\t%s10,8(%s1)\n",    /* last register into stack */
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix, name,
        PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        name, PPCRegPrefix, PPCRegPrefix);
      }
      else if(ap->NumArgs >= 8)
      {
        stcksize = ((8 + (ap->NumArgs-7)*4 + 15) & (~15));
        DoOutput(
        "\tmflr\t%s0\n"
        "\tstwu\t%s1,-%ld(%s1)\n"
        "\tstw\t%s0,%ld(%s1)\n",
        PPCRegPrefix, PPCRegPrefix, stcksize, PPCRegPrefix,
        PPCRegPrefix, stcksize+4, PPCRegPrefix);
      }
      basereg = 3;
    }
    else if(flags & FUNCFLAG_TAG)
    {
      nrcopyar = ap->NumArgs > 8 ? 0 : 8 + 1 - ap->NumArgs;
      stcksize = (((nrcopyar + 2 + 3)&(~3))-nrcopyar)*4;
    }
    if(!(ap->Flags & AMIPRAGFLAG_MOSBASESYSV) || !((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
    {
      DoOutput("\tstwu\t%s1,-%ld(%s1)\n"
               "\tmflr\t%s0\n",
      PPCRegPrefix, stcksize+nrcopyar*4, PPCRegPrefix, PPCRegPrefix);
    }

    if(nrcopyar)
    {
      /* Hack the stack-frame for varargs.
         Build stack-frame, but save LR in our own stack-frame,
         because we have to overwrite the lower 8 bytes of the
         caller's frame. */
      /* Save the caller's saved SP in our own stack-frame. */
      DoOutput("\tlwz\t%s11,%ld(%s1)\n\tstw\t%s11,%ld(%s1)\n", PPCRegPrefix,
      stcksize+nrcopyar*4, PPCRegPrefix, PPCRegPrefix, stcksize, PPCRegPrefix);

      /* Store r3-r8 at the top of our stack-frame and r9-r10
         at the low 8 bytes of the caller's frame. This way all
         arguments will reside in one continuous area.
         Only copy the really relevant parts. */
      for(i = 10; i > 10-nrcopyar; --i)
        DoOutput("\tstw\t%s%ld,%ld(%s1)\n", PPCRegPrefix, i,
        stcksize+4*(i-1+nrcopyar-8),PPCRegPrefix);
    }

    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      if(flags & FUNCFLAG_TAG || ap->NumArgs >= 8)
      {
        for(i = ap->NumArgs-1; i; --i)
        {
          if(i < 7)
          {
            DoOutput("\tmr\t%s%ld,%s%ld\n",PPCRegPrefix, 3+i, PPCRegPrefix,
            3+i-1);
          }
          else if(i == 7)
          {
            DoOutput("\tstw\t%s10,8(%s1)\n", PPCRegPrefix, PPCRegPrefix);
          }
          else
          {
            DoOutput("\tlwz\t%s11,%ld(%s1)\n\tstw\t%s11,%ld(%s1)\n",
            PPCRegPrefix, stcksize+((i-8)+3)*4, PPCRegPrefix, PPCRegPrefix,
            ((i-8)+3)*4, PPCRegPrefix);
          }
        }
      }
      else
      {
        /* shift all the arguments one field */
        for(i = ap->NumArgs+3; i > 3; --i)
        {
          DoOutput("\tmr\t%s%ld,%s%ld\n",PPCRegPrefix, i, PPCRegPrefix, i-1);
        }
      }
    }

    if(!(ap->Flags & AMIPRAGFLAG_MOSBASESYSV) || !((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
      DoOutput("\tstw\t%s0,%ld(%s1)\n", PPCRegPrefix, stcksize+4, PPCRegPrefix);

    if(BaseName)
    {
      if(Flags & FLAG_SMALLDATA)
        DoOutput("\tlwz\t%s%ld,%s@sdarx(%s13)\n", PPCRegPrefix, basereg,
        BaseName, PPCRegPrefix);
      else
        DoOutput("\tlis\t%s%ld,%s@ha\n\tlwz\t%s%ld,%s@l(%s%ld)\n",
        PPCRegPrefix, basereg, BaseName, PPCRegPrefix, basereg, BaseName,
        PPCRegPrefix, basereg);
    }

    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      DoOutput("\tlwz\t%s0,-%d(%s3)\n\tmtctr\t%s0\n\tbctrl\n",
      PPCRegPrefix, ap->Bias-2, PPCRegPrefix, PPCRegPrefix);
    }
    else
    {
      for(i = 0; i < ap->NumArgs; ++i)
      {
        if(!(flags & FUNCFLAG_TAG) || i < ap->NumArgs-1)
        {
          if(i <= 7)
            DoOutput("\tstw\t%s%ld,", PPCRegPrefix, i+3);
          else
            DoOutput("\tlwz\t%s11,%ld(%s1)\n\tstw\t%s11,", PPCRegPrefix,
            stcksize+(i+2-8)*4, PPCRegPrefix, PPCRegPrefix);
        }
        else
          DoOutput("\taddi\t%s4,%s1,%ld\n\tstw\t%s4,", PPCRegPrefix,
          PPCRegPrefix, stcksize+8+(ap->NumArgs > 8 ? (ap->NumArgs-8)*4 : 0),
          PPCRegPrefix);
        DoOutput("%d(%s2)\n", 4*ap->Args[i].ArgReg, PPCRegPrefix);
      }

      DoOutput("\tlwz\t%s11,100(%s2)\n",         /* EmulCallDirectOS */
               PPCRegPrefix, PPCRegPrefix);

      /* store basepointer in A6 */
      if(BaseName)
        DoOutput("\tstw\t%s12,56(%s2)\n", PPCRegPrefix, PPCRegPrefix);

      /* Now place the real function call */
      DoOutput("\tli\t%s3,-%d\n", /* store offset in EmulHandle */
      PPCRegPrefix, ap->Bias);

      DoOutput("\tmtctr\t%s11\n\tbctrl\n", PPCRegPrefix);
    }

    if(nrcopyar) /* Varargs. Rebuild the caller's stack-frame. */
    {
      DoOutput("\tlwz\t%s11,%ld(%s1)\n\tstw\t%s11,%ld(%s1)\n",
      PPCRegPrefix, stcksize, PPCRegPrefix, PPCRegPrefix,
      stcksize+nrcopyar*4,PPCRegPrefix);
    }

    if((ap->Flags & AMIPRAGFLAG_MOSBASESYSV) && ((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
    {
      if(ap->NumArgs >= 8)
      {
        DoOutput(
        "\tlwz\t%s0,%ld(%s1)\n"
        "\taddi\t%s1,%s1,%ld\n"
        "\tmtlr\t%s0\n",
        PPCRegPrefix,stcksize+4,PPCRegPrefix,PPCRegPrefix,PPCRegPrefix,
        stcksize,PPCRegPrefix);
      }
      else
      {
        DoOutput(
        "\tlwz\t%s1,0(%s1)\n"       /* restore old stack frame */
        "\tlwz\t%s0,4(%s1)\n"
        "\tmtlr\t%s0\n", PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
        PPCRegPrefix, PPCRegPrefix);
      }
    }
    else
    {
      DoOutput("\tlwz\t%s0,%ld(%s1)\n"
             "\taddi\t%s1,%s1,%ld\n"
             "\tmtlr\t%s0\n",
      PPCRegPrefix, stcksize+4, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
      stcksize+nrcopyar*4, PPCRegPrefix);
    }

    DoOutput("\tblr\n");
  }

  return DoOutput("\t.type\t%s,@function\n\t.size\t%s,$-%s\n\n",
  name, name, name);
}

uint32 FuncVBCCMorphCode(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i, j, k=0, size, nrcopyar = 0, stcksize = 16, basereg = 12;
  uint8 *data, *data2, *data3;
  struct ArHeader *arh;

  data = tempbuf;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_PPC))
    return 1;

  Flags |= FLAG_DONE;

  *(data++) = 0x7F;                                             /* eeh->e_ident[EI_MAG0] */
  *(data++) = 'E';                                              /* eeh->e_ident[EI_MAG1] */
  *(data++) = 'L';                                              /* eeh->e_ident[EI_MAG2] */
  *(data++) = 'F';                                              /* eeh->e_ident[EI_MAG3] */
  *(data++) = ELFCLASS32;                                       /* eeh->e_ident[EI_CLASS] */
  *(data++) = ELFDATA2MSB;                                      /* eeh->e_ident[EI_DATA] */
  *(data++) = EV_CURRENT;                                       /* eeh->e_ident[EI_VERSION] */
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  EndPutM16Inc(data, ET_REL);                                   /* eeh->e_type */
  EndPutM16Inc(data, EM_POWERPC);                               /* eeh->e_machine */
  EndPutM32Inc(data, EV_CURRENT);                               /* eeh->e_version */
  EndPutM32Inc(data, 0);                                        /* eeh->e_entry */
  EndPutM32Inc(data, 0);                                        /* eeh->e_phoff */
  data2 = data; data += 4;
  EndPutM32Inc(data, 0);                                        /* eeh->e_flags */
  EndPutM16Inc(data, 52);                                       /* eeh->e_ehsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phentsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phnum */
  EndPutM16Inc(data, 40);                                       /* eeh->e_shentsize */
  EndPutM16Inc(data, 6);                                        /* eeh->e_shnum */
  EndPutM16Inc(data, 3);                                        /* eeh->e_shstrndx - fourth table is string table */

  data3 = data;

  if(ap->Flags & (AMIPRAGFLAG_MOSSYSV|AMIPRAGFLAG_MOSSYSVR12))
  {
    if(Flags & FLAG_SMALLDATA)
    {
      k = (data-data3)+2;                                       /* store reloc offset */
      EndPutM32Inc(data, 0x818D0000);                           /* lwz r12,BaseName@sdarx(r13) */
    }
    else
    {
      k = (data-data3)+2;                                       /* store reloc offset */
      EndPutM32Inc(data, 0x3D600000);                           /* lis r11,BaseName@ha = addis r11,0,BaseName@ha */
      EndPutM32Inc(data, 0x818B0000);                           /* lwz r12,BaseName@l(r11) */
    }

    EndPutM32Inc(data, 0x800c0000 - (ap->Bias-2));
    EndPutM32Inc(data, 0x7c0903a6);                             /* mtctr r0 */
    EndPutM32Inc(data, 0x4e800420);                             /* bctr */
  }

  else
  {
    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      if(flags & FUNCFLAG_TAG)
      {
        /* mflr r0 = mfspr r0,8 = get link register */
        EndPutM32Inc(data, 0x7C0802A6);
        /* backchain to next frame: lwz r11,0(r1) */
        EndPutM32Inc(data, 0x81610000);
        /* difference = size of frame to copy: sub r12,r11,r1 = subf r12,r1,r11 */
        EndPutM32Inc(data, 0x7D815850);
        EndPutM32Inc(data, 0x90010004); /* stw r0,4(r1) */
        EndPutM32Inc(data, 0x7D6C0850); /* sub r11,r1,r12 */
        /* subi r11,r11,16 - r11 Start of new frame, +16 size */
        EndPutM32Inc(data, 0x396BFFF0);
        EndPutM32Inc(data, 0x902B0000); /* stw r1,0(r11) - Backchain to last frame */
        EndPutM32Inc(data, 0x558CF0BE); /* srwi r12,r12,2 */
        /* subi r0,r12,2 - size/4-2 = number of longwords to copy */
        EndPutM32Inc(data, 0x380CFFFE);
        EndPutM32Inc(data, 0x39810004); /* addi r12,r1,4 */
        EndPutM32Inc(data, 0x7D615B78); /* mr r1,r11 - new stack frame */
        EndPutM32Inc(data, 0x396B0008); /* addi r11,r11,8 */
        EndPutM32Inc(data, 0x7C0903A6); /* mtctr r0 */
        /* .l: lwzu r0,4(r12) - copy stack frame with offset 8 */
        EndPutM32Inc(data, 0x840C0004);
        EndPutM32Inc(data, 0x940B0004); /* stwu r0,4(r11) */
        EndPutM32Inc(data, 0x4200FFF8); /* bdnz .l */
        /* stw r10,8(r1) - last register into stack */
        EndPutM32Inc(data, 0x91410008);
      }
      else if(ap->NumArgs >= 8)
      {
        stcksize = ((8 + (ap->NumArgs-7)*4 + 15) & (~15));
        EndPutM32Inc(data, 0x7C0802A6);                /* mflr r0 */
        EndPutM32Inc(data, 0x94220000 - stcksize);     /* stwu r1,-X(r1) */
        EndPutM32Inc(data, 0x90010000 + stcksize + 4); /* stw r0,Y(r1) */
      }
      basereg = 3;
    }
    else if(flags & FUNCFLAG_TAG)
    {
      nrcopyar = ap->NumArgs > 8 ? 0 : 8 + 1 - ap->NumArgs;
      stcksize = (((nrcopyar + 2 + 3)&(~3))-nrcopyar)*4;
    }

    if(!(ap->Flags & AMIPRAGFLAG_MOSBASESYSV) || !((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
    {
      EndPutM32Inc(data, 0x94210000+0x10000-(stcksize+nrcopyar*4)); /* stwu r1,-%d(r1) */
      /* mflr r0 = mfspr r0,8 = get link register */
      EndPutM32Inc(data, 0x7C0802A6);
    }

    if(nrcopyar)
    {
      /* Hack the stack-frame for varargs.
         Build stack-frame, but save LR in our own stack-frame,
         because we have to overwrite the lower 8 bytes of the
         caller's frame. */
      /* Save the caller's saved SP in our own stack-frame. */
      EndPutM32Inc(data, 0x81610000+stcksize+nrcopyar*4);         /* lwz r11,%d(r1) */
      EndPutM32Inc(data, 0x91610000+stcksize);                    /* stw r11,%d(r1) */
  
      /* Store r3-r8 at the top of our stack-frame and r9-r10
       at the low 8 bytes of the caller's frame. This way all
       arguments will reside in one continuous area.
       Only copy the really relevant parts. */
      for(i = 10; i > 10-nrcopyar; --i)
        EndPutM32Inc(data, 0x90010000 + (i<<21) + (stcksize+4*(i-1+nrcopyar-8))); /* stw rX,Y(r1) */
    }

    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      if(flags & FUNCFLAG_TAG || ap->NumArgs >= 8)
      {
        for(i = ap->NumArgs-1; i; --i)
        {
          if(i < 7)
          {
            /* mr rX,rY */
            EndPutM32Inc(data, 0x7C000378 + ((3+i)<<21) + ((3+i-1)<<16) + ((3+i-1)<<11));
          }
          else if(i == 7)
          {
            /* stw r10,8(r1) */
            EndPutM32Inc(data, 0x91410008);
          }
          else
          {
            /* lwz r11,X(r1) */
            EndPutM32Inc(data, 0x81610000 + (stcksize+((i-8)+3)*4));
            EndPutM32Inc(data, 0x91620000 + ((i-8)+3)*4); /* stw r11,j(r1) */
          }
        }
      }
      else
      {
        /* shift all the arguments one field */
        for(i = ap->NumArgs+3; i > 3; --i)
        {
          /* mr rX,rY */
          EndPutM32Inc(data, 0x7C000378 + (i<<21) + ((i-1)<<16) + ((i-1)<<11));
        }
      }
    }

    if(!(ap->Flags & AMIPRAGFLAG_MOSBASESYSV) || !((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
      EndPutM32Inc(data, 0x90010000+stcksize+4); /* stw r0,%d(r1) */

    if(BaseName)
    {
      if(Flags & FLAG_SMALLDATA)
      {
        k = (data-data3)+2;                                       /* store reloc offset */
        EndPutM32Inc(data, 0x818D0000);                           /* lwz r12,BaseName@sdarx(r13) */
      }
      else
     {
        k = (data-data3)+2;                                       /* store reloc offset */
        EndPutM32Inc(data, 0x3D800000);                           /* lis r12,BaseName@ha = addis r12,0,BaseName@ha */
        EndPutM32Inc(data, 0x818C0000);                           /* lwz r12,BaseName@l(r12) */
      }
    }

    if(ap->Flags & AMIPRAGFLAG_MOSBASESYSV)
    {
      /* lwz r0,X(r3) */
      EndPutM32Inc(data, 0x80040000 - (ap->Bias-2));
      EndPutM32Inc(data, 0x7C0903A6); /* mtctr r0 */
      EndPutM32Inc(data, 0x4E800421); /* bctrl */
    }
    else
    {
      for(i = 0; i < ap->NumArgs; ++i)
      {
        j = 4*ap->Args[i].ArgReg;
        if(!(flags & FUNCFLAG_TAG) || i < ap->NumArgs-1)
        {
          if(i <= 7)
          {
            EndPutM32Inc(data, 0x90020000 + ((i+3)<<21) + j);       /* stw rX,j(r2) */
          }
          else
          {
            EndPutM32Inc(data, 0x81610000 + (stcksize+(i+2-8)*4));  /* lwz r11,X(r1) = get data from stack */
            EndPutM32Inc(data, 0x91620000 + j);                     /* stw r11,j(r1) */
          }
        }
        else
        {
          EndPutM32Inc(data, 0x38810000 + (stcksize+8+(ap->NumArgs > 8 ? (ap->NumArgs-8)*4 : 0))); /* addi r4,r1,X */
          EndPutM32Inc(data, 0x90820000 + j);                       /* stw r4,X(r2) */
        }
      }
    }

    EndPutM32Inc(data, 0x81620064);                               /* lwz r11,100(r2) */

    if(BaseName)
      EndPutM32Inc(data, 0x91820038);                             /* stw r12,56(r2) */

    /* Now place the real function call */
    EndPutM32Inc(data, 0x38600000 + 0x10000 - ap->Bias);          /* li r3,-(ap->Bias) = addi r3,0,-ap->Bias */

    EndPutM32Inc(data, 0x7D6903A6);                               /* mtctr r11 */
    EndPutM32Inc(data, 0x4E800421);                               /* bctrl */

    if(nrcopyar) /* Varargs. Rebuild the caller's stack-frame. */
    {
      EndPutM32Inc(data, 0x81610000 + stcksize);                  /* lwz r11,X(r1) */
      EndPutM32Inc(data, 0x91610000 + (stcksize+nrcopyar*4));     /* stw r11,Y(r1) */
    }

    if((ap->Flags & AMIPRAGFLAG_MOSBASESYSV) && ((flags & FUNCFLAG_TAG)
    || ap->NumArgs >= 8))
    {
      if(ap->NumArgs >= 8)
      {
        EndPutM32Inc(data, 0x80010000 + stcksize+4); /* lwz r0,X(r1) */
        EndPutM32Inc(data, 0x38210000 + stcksize);   /* addi r1,r1,Y */
        /* mtlr r0 = mtspr 8,r0 = restore link register */
        EndPutM32Inc(data, 0x7C0803A6);
      }
      else
      {
        /* restore old stack frame: lwz r1,0(r1) */
        EndPutM32Inc(data, 0x80210000);
        EndPutM32Inc(data, 0x80010004); /* lwz r0,4(r1) */
        /* mtlr r0 = mtspr 8,r0 = restore link register */
        EndPutM32Inc(data, 0x7C0803A6);
      }
    }
    else
    {
      EndPutM32Inc(data, 0x80010000 + stcksize+4);                /* lwz r0,X(r1) */
      EndPutM32Inc(data, 0x38210000 + (stcksize+nrcopyar*4));     /* addi r1,r1,Y */
      EndPutM32Inc(data, 0x7C0803A6);                             /* mtlr r0 = mtspr 8,r0 = restore link register */
    }

    EndPutM32Inc(data, 0x4E800020);                               /* blr = bclr 20,0 */
  }

  memcpy(data, "\0.symtab\0.strtab\0.shstrtab\0.text\0.rela.text\0", 44);
  data += 44;  /* 1        9        17         27     33 */
  
  EndPutM32(data2, data-tempbuf);                               /* eeh->e_shoff */
  data2 = data-44;

  EndPutM32Inc(data, 0);                                        /* esh[0].sh_name */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_offset */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  size = data2-data3;
  EndPutM32Inc(data, 27);                                       /* esh[1].sh_name = .text */
  EndPutM32Inc(data, SHT_PROGBITS);                             /* esh[1].sh_type */
  EndPutM32Inc(data, SHF_ALLOC|SHF_EXECINSTR);                  /* esh[1].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_addr */
  EndPutM32Inc(data, data3-tempbuf);                            /* esh[1].sh_offset */
  EndPutM32Inc(data, size);                                     /* esh[1].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_info */
  EndPutM32Inc(data, 16);                                       /* esh[1].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[1].sh_entsize */

  data3 = data;
  EndPutM32Inc(data, 33);                                       /* esh[2].sh_name = .rela.text */
  EndPutM32Inc(data, SHT_RELA);                                 /* esh[2].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[2].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[2].sh_addr */
  data += 4;                                                    /* esh[2].sh_offset */
  data += 4;                                                    /* esh[2].sh_size */
  EndPutM32Inc(data, 4);                                        /* esh[2].sh_link - the fifth entry is symbol table */
  EndPutM32Inc(data, 1);                                        /* esh[2].sh_info - the second entry is programm data */
  EndPutM32Inc(data, 4);                                        /* esh[2].sh_addralign */
  EndPutM32Inc(data, 12);                                       /* esh[2].sh_entsize - sizeof(struct Elf32_Rela) */

  EndPutM32Inc(data, 17);                                       /* esh[3].sh_name = .shstrtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[3].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_addr */
  EndPutM32Inc(data, data2-tempbuf);                            /* esh[3].sh_offset */
  EndPutM32Inc(data, 44);                                       /* esh[3].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[3].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_entsize */

  EndPutM32Inc(data, 1);                                        /* esh[4].sh_name = .symtab */
  EndPutM32Inc(data, SHT_SYMTAB);                               /* esh[4].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_addr */
  data += 4;                                                    /* esh[4].sh_offset */
  data += 4;                                                    /* esh[4].sh_size */
  EndPutM32Inc(data, 5);                                        /* esh[4].sh_link - the sixth entry is our string table */
  EndPutM32Inc(data, 3);                                        /* esh[4].sh_info - One greater than index of last LOCAL symbol*/
  EndPutM32Inc(data, 4);                                        /* esh[4].sh_addralign */
  EndPutM32Inc(data, 16);                                       /* esh[4].sh_entsize = sizeof(struct Elf32_Sym) */

  EndPutM32Inc(data, 9);                                        /* esh[0].sh_name = .strtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  data += 4;                                                    /* esh[0].sh_offset */
  data += 4;                                                    /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  EndPutM32(data3+(2*40)+(4*4), data-tempbuf);                  /* esh[4].sh_offset */
  EndPutM32(data3+(2*40)+(5*4), BaseName ? 5*16 : 4*16);        /* esh[4].sh_size */

  data2 = data;
  data += BaseName ? 5*16 : 4*16;

  EndPutM32(data3+(3*40)+(4*4), data-tempbuf);                  /* esh[5].sh_offset */

  i = 0;
  EndPutM32Inc(data2, i);                                       /* esym[0].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[0].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[0].st_size */
  *(data2++) = 0;                                               /* esym[0].st_info */
  *(data2++) = 0;                                               /* esym[0].st_other */
  EndPutM16Inc(data2, 0);                                       /* esym[0].st_shndx */
  data[0] = 0;

  i += 1;
  EndPutM32Inc(data2, i);                                       /* esym[1].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[1].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[1].st_size */
  *(data2++) = ELF32_ST_INFO(STB_LOCAL,STT_FILE);               /* esym[1].st_info */
  *(data2++) = 0;                                               /* esym[1].st_other */
  EndPutM16Inc(data2, SHN_ABS);                                 /* esym[1].st_shndx */

  sprintf((strptr)data+i, "%s.o", name); while(data[i]) { i++;} ; /* get next store space */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_value */
  EndPutM32Inc(data2, 0);                                       /* esym[2].st_size */
  *(data2++) = ELF32_ST_INFO(STB_LOCAL,STT_SECTION);            /* esym[2].st_info */
  *(data2++) = 0;                                               /* esym[2].st_other */
  EndPutM16Inc(data2, 1);                                       /* esym[2].st_shndx - the second entry is program section! */

  EndPutM32Inc(data2, i);                                       /* esym[3].st_name */
  EndPutM32Inc(data2, 0);                                       /* esym[3].st_value */
  EndPutM32Inc(data2, size);                                    /* esym[3].st_size */
  *(data2++) = ELF32_ST_INFO(STB_GLOBAL,STT_FUNC);              /* esym[3].st_info */
  *(data2++) = 0;                                               /* esym[3].st_other */
  EndPutM16Inc(data2, 1);                                       /* esym[3].st_shndx - the second entry is program section! */

  sprintf((strptr)data+i, "%s", name); while(data[i]) { i++;} ; /* get next store space */
  if(BaseName)
  {
    EndPutM32Inc(data2, i);                                     /* esym[4].st_name */
    EndPutM32Inc(data2, 0);                                     /* esym[4].st_value */
    EndPutM32Inc(data2, 0);                                     /* esym[4].st_size */
    *(data2++) = ELF32_ST_INFO(STB_GLOBAL,STT_NOTYPE);          /* esym[4].st_info */
    *(data2++) = 0;                                             /* esym[4].st_other */
    EndPutM16/*Inc*/(data2, 0);                                 /* esym[4].st_shndx */

    sprintf((strptr)data+i, "%s", BaseName); while(data[i]) { i++;} ; /* get next store space */
  }
  EndPutM32(data3+(3*40)+(5*4), i);                             /* esh[5].sh_size */
  while(i&3) /* long aligned */
   data[i++] = 0;
  data += i;

  EndPutM32(data3+(4*4), data-tempbuf);                         /* esh[2].sh_offset */

  data2 = data;

  if(BaseName)
  {
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM32Inc(data, k);                                    /* erel[0].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(4,R_PPC_SDAREL16));       /* erel[0].r_info - entry 4, type 32 */
      EndPutM32Inc(data, 0);                                    /* erel[0].r_addend */
    }
    else
    {
      EndPutM32Inc(data, k);                                    /* erel[0].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(4,R_PPC_ADDR16_HA));      /* erel[0].r_info - entry 4, type 6 */
      EndPutM32Inc(data, 0);                                    /* erel[0].r_addend */
      EndPutM32Inc(data, k+4);                                  /* erel[1].r_offset */
      EndPutM32Inc(data, ELF32_R_INFO(4,R_PPC_ADDR16_LO));      /* erel[1].r_info - entry 4, type 4 */
      EndPutM32Inc(data, 0);                                    /* erel[1].r_addend */
    }
  }
  EndPutM32(data3+(5*4), data-data2);                           /* esh[2].sh_size */

  /* make ar header and store all */
  arh = (struct ArHeader *) (data+20);
  memset(arh, ' ', sizeof(struct ArHeader));

  arh->ar_time[sprintf(arh->ar_time, "%lu", (uint32) time(0))] = ' ';
  arh->ar_uid[0] = arh->ar_gid[0] = arh->ar_mode[1] =
  arh->ar_mode[2] = '0';
  arh->ar_mode[0] = '6';
  arh->ar_fmag[0] = 96;
  arh->ar_fmag[1] = '\n';

  if((k = strlen(name) + 2) >= 16)
  {
    arh->ar_name[sprintf(arh->ar_name, "#1/%ld", k)] = ' ';
  }
  else
  {
    k = 0;
    arh->ar_name[sprintf(arh->ar_name, "%s.o", name)] = ' ';
  }

  j = k + (data-tempbuf);
  for(i = 9; j; --i) /* make decimal number */
  {
    data[i] = (j%10)+'0';
    j /= 10;
  }
  for(j = 0; i < 9; ++j)
    arh->ar_size[j] = data[++i];
  
  DoOutputDirect(arh, sizeof(struct ArHeader));

  if(k)
  {
    DoOutput("%s.o", name);
    if(k & 1)
      *(data++) = 0x0A; /* alignment byte! */
  }

  return DoOutputDirect(tempbuf, data-tempbuf);
}

uint32 FuncEModule(struct AmiPragma *ap, uint32 flags, strptr name)
{
  uint8 i, r;

  if(CheckError(ap, AMIPRAGFLAG_FLOATARG|AMIPRAGFLAG_A6USE|AMIPRAGFLAG_PPC) ||
  (flags & FUNCFLAG_ALIAS))
    return 1;

  if(LastBias >= ap->Bias)
    DoError(ERR_ILLEGAL_FUNCTION_POSITION, ap->Line, name);
  else
  {
    Flags |= FLAG_DONE; /* We did something */

    for(LastBias += BIAS_OFFSET; LastBias < ap->Bias; LastBias += BIAS_OFFSET)
      DoOutputDirect("Dum\x10", 4);

    DoOutput("%c", toupper(name[0]));
    if(name[1])
    {
      DoOutput("%c", tolower(name[1]));
      if(name[2])
        DoOutput("%s", name+2);
    }
    if(!ap->NumArgs)
      DoOutputDirect("\x10", 1);
    else
    {
      for(i = 0; i < ap->NumArgs; ++i)
      {
        r = ap->Args[i].ArgReg;
        DoOutputDirect(&r, 1);
      }
    }
  }
  return 1;
}

uint32 FuncFD(struct AmiPragma *ap, uint32 flags, strptr name)
{
  int32 i;

  Flags |= FLAG_DONE; /* We did something */

  if(ap->Flags & AMIPRAGFLAG_PUBLIC)
  {
    if(Flags & FLAG_ISPRIVATE)
    {
      Flags ^= FLAG_ISPRIVATE;
      DoOutput("##public\n");
    }
  }
  else
  {
    if(!(Flags & FLAG_ISPRIVATE))
      DoOutput("##private\n");
    Flags |= FLAG_ISPRIVATE;
  }

  LastBias += BIAS_OFFSET;
  if(LastBias != ap->Bias)
  {
    DoOutput("##bias %d\n", ap->Bias);
    LastBias = ap->Bias;
  }

  if(ap->Abi != CurrentABI)
  {
    switch(ap->Abi)
    {
    case ABI_M68K: DoOutput("##abi M68k\n"); break;
    case ABI_PPC0: DoOutput("##abi PPC0\n"); break;
    case ABI_PPC2: DoOutput("##abi PPC2\n"); break;
    case ABI_PPC:  DoOutput("##abi PPC\n"); break;
    }
    CurrentABI = ap->Abi;
  }

  DoOutput("%s("/*)*/, name);
  for(i = 0; i < ap->CallArgs; i++)
    DoOutput("%s%s", ap->Args[i].ArgName, i < ap->CallArgs-1 ? "," : "");
  DoOutput(/*(*/")("/*)*/);

  if(!(ap->Flags & AMIPRAGFLAG_PPC))
  {
    for(i = 0; i < ap->CallArgs; i++)
    {
      DoOutput("%s%s", RegNames[ap->Args[i].ArgReg], i < ap->CallArgs-1 ? 
      (ap->Args[i].ArgReg < ap->Args[i+1].ArgReg ? "/" : ",") : "");
    }
  }
  return DoOutput(/*(*/")\n");
}

/* called from FuncSFD directly */
uint32 FuncClib(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, s, c;

  Flags |= FLAG_DONE; /* We did something */

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  s = MakeClibType(tempbuf, &cd->ReturnType, 0);
  DoOutputDirect(tempbuf, s);
  DoOutput(" %s("/*)*/, name);

  if(ap->NumArgs)
  {
    for(i = 0; i < cd->NumArgs; i++)
    {
      c = MakeClibType(tempbuf, &cd->Args[i], ap->Args[i].ArgName);
      if(s+c+2 > 75 && s)
      {
        DoOutput(i ? ",\n\t" : "\n\t"); s = 8;
      }
      else if(i)
      {
        DoOutput(", "); s += 2;
      }
      DoOutputDirect(tempbuf, c);
      s += c;
    }
  }
  else if(Flags2 & FLAG2_CLIBOUT)
    DoOutput("void");
  return DoOutput(/*(*/")%s", Flags2 & FLAG2_CLIBOUT ? ";\n" : "");
}

uint32 FuncSFD(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, j;

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  if(ap->Flags & AMIPRAGFLAG_PUBLIC)
  {
    if(Flags & FLAG_ISPRIVATE)
    {
      Flags ^= FLAG_ISPRIVATE;
      DoOutput("==public\n");
    }
  }
  else
  {
    if(!(Flags & FLAG_ISPRIVATE))
      DoOutput("==private\n");
    Flags |= FLAG_ISPRIVATE;
  }

  if(ap->Abi != CurrentABI)
  {
    switch(ap->Abi)
    {
    case ABI_M68K: DoOutput("==abi M68k\n"); break;
    case ABI_PPC0: DoOutput("==abi PPC0\n"); break;
    case ABI_PPC2: DoOutput("==abi PPC2\n"); break;
    case ABI_PPC:  DoOutput("==abi PPC\n"); break;
    }
    CurrentABI = ap->Abi;
  }

  if(LastBias+BIAS_OFFSET < ap->Bias)
  {
    DoOutput("==reserve %ld\n", ((ap->Bias-LastBias)/BIAS_OFFSET)-1);
    LastBias = ap->Bias;
  }
  else if(flags & FUNCFLAG_TAG)
    DoOutput("==varargs\n");
  else if((flags & FUNCFLAG_ALIAS) || LastBias == ap->Bias)
    DoOutput("==alias\n");
  else
    LastBias += BIAS_OFFSET;
  
  if(!FuncClib(ap, flags, name))
    return 0;

  DoOutput(" ("/*)*/);
  if(!(ap->Flags & AMIPRAGFLAG_PPC))
  {
    strptr s;

    /* j runs in steps of two. If CPP_TYPE_DOUBLE is stored in data registers, it runs
       in step one, so the "-" can be placed at proper position. */
    for(j = i = 0; i < ap->NumArgs; i++)
    {
      if(i == ap->NumArgs-1)
      {
        s = ""; j += 2;
      }
      else if(IsCPPType(&cd->Args[j>>1], CPP_TYPE_DOUBLE) && ap->Args[i].ArgReg < REG_FP0)
      {
        s = (j&1) ? "," : "-"; ++j;
      }
      else
      {
        s = ","; j += 2;
      }
      DoOutput("%s%s", RegNames[ap->Args[i].ArgReg], s);
    }
  }
  return DoOutput(/*(*/")\n");
}

uint32 FuncOS4PPC(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, noret = 0, registers;

  if(CheckError(ap, AMIPRAGFLAG_PPC|AMIPRAGFLAG_MOS_ALL))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
    noret = 1; /* this is a void function */

  sprintf((strptr)tempbuf, "_%s_%s", GetIFXName(), name);
  OutClibType(&cd->ReturnType, (strptr)tempbuf);

  DoOutput("(  \n  struct %sIFace *Self%s\n"/*)*/,GetIFXName(),
  ap->NumArgs ? "," : "");
  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("  ");
    if(i == ap->NumArgs - 1 && (flags & FUNCFLAG_TAG))
    {
      DoOutput("...\n");
    }
    else
    {
      OutClibType(&cd->Args[i], ap->Args[i].ArgName);
      if(i < ap->NumArgs-1)
        DoOutput(",\n");
    }
  }
  if(flags & FUNCFLAG_TAG)
  {
    struct ClibData *cd2;

    if(!(cd2 = GetClibFunc(ap->FuncName, ap, flags)))
      return 1;

    DoOutput(/*(*/")\n{\n"/*}*/
    "  va_list ap;\n"
    "  va_startlinear(ap, colorMap);\n"
    "  ");
    if(!noret)
      DoOutput("return ");
    DoOutput("Self->%s(\n"/*)*/, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
    {
      DoOutput("    %s,\n", ap->Args[i].ArgName);
    }
    DoOutput("    va_getlinearva(ap, "/*)*/);
    OutClibType(&cd2->Args[i], 0);
    DoOutput(/*((*/"));\n");
  }
  else
  {
    DoOutput(/*(*/")\n{\n"/*}*/
    "  struct Library *LibBase = Self->Data.LibBase;\n"
    "  struct ExecIFace *IExec = (struct ExecIFace *)"
    "Self->Data.IExecPrivate;\n");

    if(!noret)
    {
      DoOutput("  ");
      OutClibType(&cd->ReturnType, "retval");
      DoOutput(";\n");
    }

    DoOutput(
    "  ULONG *regs = (ULONG *)(((struct ExecBase *)(IExec->Data.LibBase))"
    "->EmuWS);\n");
    for(i = 0; i < ap->NumArgs; ++i)
    {
    }
    registers = GetRegisterData(ap) | 0x40000002; /* set A6 everytime */
    for(i = 0; i <= 15; ++i)
    {
      if(registers & (1<< (16+i)))
        DoOutput("  ULONG save_%s = regs[%ld];\n", RegNames[i], i);
    }
    DoOutput("\n  ");

    if(!noret)
    {
      DoOutput("retval = ("/*)*/);
      OutClibType(&cd->ReturnType, 0);
      DoOutput(/*(*/")");
    }

    DoOutput("IExec->EmulateTags((APTR)LibBase,\n"
    "    ET_Offset,     -%d,\n"/*)*/,ap->Bias);
    for(i = 0; i < ap->NumArgs; ++i)
    {
      DoOutput("    ET_Register%s, %s,\n", RegNamesUpper[ap->Args[i].ArgReg],
      ap->Args[i].ArgName);
    }
    DoOutput(/*(*/"    ET_RegisterA6, LibBase,\n    TAG_DONE);\n\n");
    for(i = 0; i <= 15; ++i)
    {
      if(registers & (1<< (16+i)))
        DoOutput("  regs[%ld] = save_%s;\n", i, RegNames[i]);
    }

    if(!noret)
      DoOutput("  return retval;\n");
  }
  return DoOutput(/*{*/"}\n\n");
}

uint32 FuncOS4M68KCSTUB(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i, noret = 0;

  if(ap->NumArgs <= 7)
    return 1;

  if(CheckError(ap, AMIPRAGFLAG_PPC|AMIPRAGFLAG_MOS_ALL))
    return 1;

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
    noret = 1; /* this is a void function */

  sprintf((strptr)tempbuf, "Cstub_%s", name);
  OutClibType(&cd->ReturnType, (strptr)tempbuf);

  DoOutput("(struct %sIFace *Interface)\n{\n"/*}*/
  "  struct ExecIFace *IExec = (struct ExecIFace *)"
  "Interface->Data.IExecPrivate;\n"
  "  struct ExecBase *SysBase = (struct ExecBase *)IExec->Data.LibBase;\n"
  "  ULONG *regarray = (ULONG *)SysBase->EmuWS;\n  ",GetIFXName());

  if(!noret)
    DoOutput("return ");

  DoOutput("Interface->%s(\n"/*)*/,(flags & FUNCFLAG_TAG) ? ap->FuncName
  : name);
  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("    ("/*)*/);
    OutClibType(&cd->Args[i], 0);
    DoOutput(/*(*/") regarray[%d]", ap->Args[i].ArgReg);
    if(i < ap->NumArgs-1)
      DoOutput(",\n");
  }
  return DoOutput(/*{(*/");\n}\n\n");
}

uint32 FuncOS4M68K(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int i;

  if(CheckError(ap, AMIPRAGFLAG_PPC|AMIPRAGFLAG_MOS_ALL))
    return 1;

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  DoOutput(
  "\t.section .data\n"
  "\t.globl\tstub_%s\n"
  "\t.type\tstub_%s,@function\n"
  "\n"
  "stub_%s:\n"
  "\t.short\t0x4ef8\n"
  "\t.short 0\n"
  "\t.short 1\n"
  "\t.globl\tstub_%sPPC\n"
  "\t.long\tstub_%sPPC\n",name, name, name, name, name);

  if(ap->NumArgs > 7)
  {
    Flags2 |= FLAG2_OS4M68KCSTUB;
    DoOutput(
    "\t.byte\t2\n"              /* Rest of parameters in C */
    "\t.byte\t1,REG68K_A7\n"    /* r1<-A7 */
    "\t.byte\t3,REG68K_A6\n");  /* r6<-A6 */
  }
  else
  {
    DoOutput(
    "\t.byte\t%d\n"             /* Rest of parameters in C */
    "\t.byte\t1,REG68K_A7\n"    /* r1<-A7 */
    "\t.byte\t3,REG68K_A6\n",   /* r6<-A6 */
    ap->NumArgs+2);
    for(i = 0; i < ap->NumArgs; ++i)
    {
      DoOutput("\t.byte\t%d,REG68K_%s\n",i+4,
      RegNamesUpper[ap->Args[i].ArgReg]);
    }
  }
  DoOutput(
  "\t.section .text\n"
  "\t.align\t2\n"
  "\n"
  "stub_%sPPC:\n"
  "\taddi\t%s12,%s1,-16\n"            /* Calculate stackframe size */
  "\trlwinm\t%s12,%s12,0,0,27\n"      /* Align it */
  "\tstw\t%s1,0(%s12)\n"              /* Store backchain pointer */
  "\tmr\t%s1,%s12\n"                  /* Set real stack pointer */
  "\tstw\t%s11,12(%s1)\n"             /* Store Enter68kQuick vector */
  "\tlhz\t%s12,LIB_POSSIZE(%s3)\n"
  "\tadd\t%s3,%s3,%s12\n"             /* by addind posSize */
  "\tlwz\t%s3,ExtLib_MainIFace(%s3)\n", /* Get the real interface pointer */
  name, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

  if(ap->NumArgs > 7)
  {
    /* Since this function has 11 arguments, we need a C stub */
    DoOutput(
    "\t.globl\tCstub_%s\n"
    "\tlis\t%s4,Cstub_%s@h\n"
    "\tori\t%s4,%s4,Cstub_%s@l\n"
    "\tmtlr\t%s4\n"
    "\tblrl\n",
    name, PPCRegPrefix, name, PPCRegPrefix, PPCRegPrefix, name, PPCRegPrefix);
  }
  else
  {
    DoOutput("\tCallLib\tI%s_%s\n", GetIFXName(), name);
  }
  DoOutput(
  "\tlwz\t%s11,12(%s1)\n"
  "\tmtlr\t%s11\n"
  "\tlwz\t%s1,0(%s1)\n"               /* Cleanup stack frame */
  "\tblrl\n"                          /* Return to emulation */
  "\n"
  "\t.globl\tstub_%s68K\n"
  "\t.long\tstub_%s68K\n"
  "\t.byte\t0\n",                     /* Flags */
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, name,
  name);

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
  {
    DoOutput(
    "\t.byte\t1\n"                      /* One register (a7 only) */
    "\t.byte\t1,REG68K_A7\n");          /* Map r1 to A7 */
  }
  else
  {
    DoOutput(
    "\t.byte\t2\n"
    "\t.byte\t1,REG68K_A7\n"
    "\t.byte\t3,REG68K_D0\n");
  }

  return DoOutput(
  "\t.section .data\n"
  "\t.align\t4\n"
  "\n"
  "stub_%s68K:\n"
  "\t.short\t0x4e75\n"                /* RTS */
  "\n", name);
}

uint32 FuncOS4M68KVect(struct AmiPragma *ap, uint32 flags, strptr name)
{
  if(CheckError(ap, AMIPRAGFLAG_PPC|AMIPRAGFLAG_MOS_ALL))
    return 1;

  while(LastBias + BIAS_OFFSET < ap->Bias)
  {
    DoOutput("\t.long\tstub_Reserved\n");
    LastBias += BIAS_OFFSET;
  }
  LastBias = ap->Bias;

  return DoOutput("\t.long\tstub_%s\n", name);
}

uint32 FuncXML(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_PPC|AMIPRAGFLAG_MOS_ALL))
    return 1;

  if(flags & FUNCFLAG_ALIAS)
    DoError(ERR_ALIASES_NOT_SUPPORTED, ap->Line);

  Flags |= FLAG_DONE; /* We did something */

  if(!(cd = GetClibFunc(name, ap, flags)))
    return 1;

  while(LastBias+BIAS_OFFSET < ap->Bias)
  {
    LastBias += BIAS_OFFSET;
    DoOutput("\t\t<method name=\"Reserved%ld\" result=\"void\""
    " status=\"unimplemented\"/>\n", LastBias);
  }
  LastBias = ap->Bias;

  DoOutput("\t\t<method name=\"%s\" result=\"", name);
  OutClibType(&cd->ReturnType, 0);
  DoOutput("\">\n");
  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("\t\t\t<%sarg name=\"%s\" type=\"", 
    i == ap->NumArgs-1 && (flags & FUNCFLAG_TAG) ? "var" : "",
    ap->Args[i].ArgName);
    OutClibType(&cd->Args[i], 0);
    DoOutput("\"/>\n");
  }
  return DoOutput("\t\t</method>\n");
}

uint32 FuncGateStubs(struct AmiPragma *ap, uint32 flags, strptr name)
{
  struct ClibData *cd;
  strptr ret = "return ";
  int32 i;

  if(CheckError(ap, AMIPRAGFLAG_ARGCOUNT|AMIPRAGFLAG_PPC))
    return 1;

  if(!(cd = GetClibFunc(ap->FuncName, ap, flags)))
    return 1;

  Flags |= FLAG_DONE; /* We did something */

  if(flags & FUNCFLAG_ALIAS)
  {
    if(flags & FUNCFLAG_TAG)
      return DoOutput("#ifndef NO_%sINLINE_STDARG\n#define %s %s\n#endif\n\n",
      Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", name, ap->TagName);

    DoOutput("#define %s("/*)*/, name);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("%s, ", ap->Args[i].ArgName);
    DoOutput(/*(*/"%s) %s("/*)*/, ap->Args[i].ArgName, ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    return DoOutput(/*(*/"(%s))\n\n", ap->Args[i].ArgName);
  }

  if((flags & FUNCFLAG_TAG))
  {
    DoOutput("#ifndef NO_%sINLINE_STDARG\n#define %s("/*)*/, 
    Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", name);
    for(i = 0; i < ap->NumArgs-1; ++i)
    {
      DoOutput("%s, ", ap->Args[i].ArgName);
    }
    DoOutput(/*(*/"tags...) \\\n\t({ULONG _tags[] = {tags}; %s("/*}))*/,
    ap->FuncName);
    for(i = 0; i < ap->NumArgs-1; ++i)
      DoOutput("(%s), ", ap->Args[i].ArgName);
    DoOutput("("/*)*/);
    OutClibType(&cd->Args[i], 0);
    return DoOutput(/*({((*/") _tags);})\n#endif\n\n");
  }

  if(IsCPPType(&cd->ReturnType, CPP_TYPE_VOID))
    ret = "";

  if(!OutClibType(&cd->ReturnType, 0))
    return 0;

  DoOutput(" %s%s(void)\n{\n"/*}*/, prefix, name);

  for(i = 0; i < ap->NumArgs; ++i)
  {
    DoOutput("  ");
    OutClibType(&cd->Args[i], ap->Args[i].ArgName);
    DoOutput(" = ("/*)*/);
    OutClibType(&cd->Args[i], 0);
    DoOutput(/*(*/") REG_%s;\n", RegNamesUpper[ap->Args[i].ArgReg]);
    if((Flags2 & (FLAG2_PRELIB|FLAG2_POSTLIB)) && (Flags2 & FLAG2_REGLIB))
      DoOutput("  %s ___RegBase = (%s) REG_A6;\n", GetBaseType(), GetBaseType());
  }
  DoOutput("  %s%s%s("/*)*/, ret, subprefix, name);
  if(ap->NumArgs)
  {
    if(Flags2 & FLAG2_PRELIB)
    {
      if(Flags2 & FLAG2_REGLIB)
        DoOutput("___RegBase,");
      else
        DoOutput("%s_BASE_NAME,", ShortBaseNameUpper);
    }

    for(i = 0; i < ap->NumArgs-1; ++i)
    {
      DoOutput("%s, ", ap->Args[i].ArgName);
    }
    if(Flags2 & FLAG2_POSTLIB)
    {
      if(Flags2 & FLAG2_REGLIB)
        DoOutput("%s, ___RegBase", ap->Args[i].ArgName);
      else
        DoOutput("%s, %s_BASE_NAME", ap->Args[i].ArgName, ShortBaseNameUpper);
    }
    else
      DoOutput("%s", ap->Args[i].ArgName);
  }
  else
  {
    if(Flags2 & (FLAG2_PRELIB|FLAG2_POSTLIB))
    {
      if(Flags2 & FLAG2_REGLIB)
        DoOutput("___RegBase");
      else
        DoOutput("%s_BASE_NAME", ShortBaseNameUpper);
    }
  }
  return DoOutput(/*(({*/"));\n}\n");
}

static uint32 DoCallFunc(struct AmiPragma *ap, uint32 flags, strptr name, FuncType Func)
{
  uint32 res;

  if(Flags & FLAG_SINGLEFILE)
  {
    sprintf(filename, filenamefmt, name);
    if(!OpenDest(filename))
      return 0;
  }
  res = Func(ap, flags, name);
  if(Flags & FLAG_SINGLEFILE)
  {
    CloseDest(filename);
  }
  return res;
}

static uint32 PrintComment(struct Comment *com, strptr comment)
{
  if(com->Private && !(Flags & FLAG_PRIVATE))
    return 1;
  else if((Flags2 & FLAG2_SFDOUT) && com->Version)
  {
    return DoOutput("==version %d\n", com->Version);
  }
  else if((Flags2 & FLAG2_SFDOUT) && com->ReservedNum)
  {
    LastBias += BIAS_OFFSET*com->ReservedNum;
    return DoOutput("==reserve %d\n", com->ReservedNum);
  }
  else if(!(Flags & FLAG_DOCOMMENT) || !comment)
    return 1;

  if(com->Data)
  {
    if(!DoOutput(comment, com->Data))
      return 0;
  }
  else if(com->ReservedNum)
  {
    string temp[256];
    sprintf(temp, "* --- (%u function slot%s reserved here) ---", com->ReservedNum,
    com->ReservedNum == 1 ? "" : "s");
    if(!DoOutput(comment, temp))
      return 0;
  }
  else if(com->Version)
  {
    string temp[256];
    if(com->Version >= FIRST_KNOWN_RELEASE && com->Version <= LAST_KNOWN_RELEASE &&
    (Flags2 & FLAG2_SYSTEMRELEASE))
      sprintf(temp, "* --- functions in V%u or higher %s ---", com->Version,
      Release[com->Version-FIRST_KNOWN_RELEASE]);
    else
      sprintf(temp, "* --- functions in V%u or higher ---", com->Version);

    if(!DoOutput(comment, temp))
      return 0;
  }
  return 1;
}

static uint32 CallFunc(uint32 tagmode, strptr comment, FuncType Func)
{
  struct Comment *com;
  int32 i;
  struct AmiPragma *ap;

  com = (struct Comment *) Comment.First;

  for(ap = (struct AmiPragma *) AmiPragma.First; ap;
  ap = (struct AmiPragma *) ap->List.Next)
  {
    if(BaseName && (ap->Flags & AMIPRAGFLAG_A6USE))
    {
      DoError(ERR_A6_NOT_ALLOWED, ap->Line);
    }
    else if((ap->Flags & AMIPRAGFLAG_PUBLIC) || (Flags & FLAG_PRIVATE))
    {
      while(com && com->Bias <= ap->Bias)
      {
        if(!PrintComment(com, comment))
          return 0;
        com = (struct Comment *) com->List.Next;
      } /* comment loop */

#ifdef DEBUG_OLD
  printf("Processing %s - %s\n", ap->FuncName ? ap->FuncName : "",
  ap->TagName ? ap->TagName : "");
#endif

      if(tagmode != TAGMODE_TAGS)
      {
        if(ap->FuncName && !DoCallFunc(ap, FUNCFLAG_NORMAL, ap->FuncName, Func))
          return 0;

        for(i = 0; i < ap->NumAlias; ++i)
        {
          if(ap->AliasName[i]->Type & FUNCFLAG_NORMAL)
          {
            if(!DoCallFunc(ap, FUNCFLAG_ALIAS|ap->AliasName[i]->Type, ap->AliasName[i]->AliasName, Func))
              return 0;
          }
        }
      } 

      if(tagmode)
      {
        if(ap->TagName && !DoCallFunc(ap, FUNCFLAG_TAG, ap->TagName, Func))
          return 0;

        for(i = 0; i < ap->NumAlias; ++i)
        {
          if(ap->AliasName[i]->Type & FUNCFLAG_TAG)
          {
            if(!DoCallFunc(ap, FUNCFLAG_ALIAS|ap->AliasName[i]->Type, ap->AliasName[i]->AliasName, Func))
              return 0;
          }
        }
      }
    }
  }
  while(com)
  {
    if(!PrintComment(com, comment))
      return 0;
    com = (struct Comment *) com->List.Next;
  } /* comment loop */
  return 1;
}

static uint32 PrintIncludes(void) /* copies the include lines */
{
  struct Include *inc;
  strptr s, s2;

  inc = (struct Include *) Includes.First;
  
  while(inc)
  {
    s2 = (strptr) tempbuf;
    for(s = inc->Include; *s; ++s)
    {
      switch(*s)
      {
      case '<': *(s2++) = ' '; break;
      case '/':
      case '.': *(s2++) = '_'; break;
      case '>': break;
      default: *(s2++) = toupper(*s);
      }
      *s2 = 0;
    }
    DoOutput("#ifndef %s\n#include %s\n#endif\n", tempbuf, inc->Include);
    inc = (struct Include *) inc->List.Next;
  }
  if(!Includes.First)
    DoOutput("#include <exec/types.h>\n");
  return DoOutput("\n");
}

/* ------------------------------------------------------------------ */

static int32 AddClibEntry(strptr buffer, strptr bufend, uint32 linenum)
{
  strptr buf = buffer;
  struct ClibData d, *f;

  memset(&d, 0, sizeof(struct ClibData));
  buf = SkipBlanksRet(buf);
  if(*buf == '#') /* preprocessor lines */
  {
#ifdef DEBUG_OLD
  printf("Found non-function bracket in preprocessor line %ld\n", linenum);
#endif
    while(buf < bufend && *buf != '\n')
      ++buf;
    return buf-buffer;
  }
  if(!strnicmp(buf, "ASM", 3))
    buf = SkipBlanks(buf+3);
/*  else if(!strnicmp(buf, "STACK", 5))
    buf = SkipBlanks(buf+5);
*/
  else if(!strnicmp(buf, "REGS", 4))
    buf = SkipBlanks(buf+4);

  if(!strnicmp(buf, "extern", 6))
  {
    buf = SkipBlanksRet(buf+6);
    if(!strnicmp(buf, "\"C\"", 3))  /* CPP: extern "C" */
    {
      buf = SkipBlanksRet(buf+3);
      if (*buf == '{')
      {
        buf = SkipBlanksRet(buf+1);
      }
    }
  }

  if(!GetCPPType(&d.ReturnType, buf, 1, 1))
  {
    DoError(ERROFFSET_CLIB | ERR_UNKNOWN_RETURNVALUE_TYPE, linenum);
    return 0;
  }
  else if(d.ReturnType.Unknown)
    DoError(ERROFFSET_CLIB | ERR_UNKNOWN_RETURNVALUE_TYPE_INT, linenum,
    d.ReturnType.Unknown);

  if(d.ReturnType.Flags & CPP_FLAG_FUNCTION)
  {
    strptr r = d.ReturnType.TypeStart;
    while(*r != '('/*)*/) ++r;
    r = SkipBlanks(++r); /* the bracket */
    d.FuncName = SkipBlanks(++r); /* the asterisk */
  }
  else
    d.FuncName = SkipBlanks(d.ReturnType.TypeStart+d.ReturnType.FullLength);
  buf = d.FuncName;
  while(*(buf++) != '('/*)*/)
    ;
  *(SkipName(d.FuncName)) = 0;
  if(!(*d.FuncName))
  {
#ifdef DEBUG_OLD
  printf("Found non-function bracket in line %ld\n", linenum);
#endif
    while(buf < bufend && *buf != '\n')
      ++buf;
    return buf-buffer;
  }
  buf = SkipBlanksRet(buf);

  while(*buf != /*(*/')' && buf < bufend)
  {
    if(d.NumArgs == MAXREGPPC+1)
    {
      DoError(ERROFFSET_CLIB | ERR_TO_MUCH_ARGUMENTS, linenum);
      return 0;
    }
    else if(!GetCPPType(&d.Args[d.NumArgs++], buf, 0, 1))
    {
      DoError(ERROFFSET_CLIB | ERR_UNKNOWN_VARIABLE_TYPE, linenum, d.NumArgs);
      return 0;
    }
    else if(d.Args[d.NumArgs-1].Unknown)
      DoError(ERROFFSET_CLIB | ERR_UNKNOWN_VARIABLE_TYPE_INT, linenum,
      d.NumArgs, d.Args[d.NumArgs-1].Unknown);

    buf = d.Args[d.NumArgs-1].TypeStart + d.Args[d.NumArgs-1].FullLength;
    while(*buf != ',' && *buf != /*(*/')' && buf < bufend)
      ++buf;
#ifdef DEBUG
  printf("Added argument %d for %s (%d bytes)\n", d.NumArgs, d.FuncName,
  d.Args[d.NumArgs-1].FullLength);
#endif
    if(*buf == ',')
      buf = SkipBlanksRet(++buf);
  }

  if(d.NumArgs == 1 && IsCPPType(&d.Args[0], CPP_TYPE_VOID))
    d.NumArgs = 0; /* void arguments are no arguments */

  if(!(f = (struct ClibData *) AllocListMem(sizeof(struct ClibData))))
    return -1;

  memcpy(f, &d, sizeof(struct ClibData));

  if(!clibdata)
    clibdata = f;
  else
  {
    struct ClibData *e = clibdata;
    while(e->Next)
      e = e->Next;
    e->Next = f;
  }
  if(d.ReturnType.Flags & CPP_FLAG_FUNCTION)
  {
    int numclose = 2, numopen = 1;
    while(buf < bufend && (numclose || numopen > 0))
    {
      if(*buf == '('/*)*/) { ++numclose; --numopen; }
      else if(*buf == /*(*/')') --numclose;
      ++buf;
    }
  }

#ifdef DEBUG
  printf("Added prototype for %s (line %ld, %d bytes) with %d args\n",
  f->FuncName, linenum, buf-buffer, f->NumArgs);
#endif
  return buf-buffer;
}

static int32 ScanClibFile(strptr buf, strptr bufend)
{
  strptr linestart = buf;
  uint32 linenum = 1;
  int added = 0;

  /* remove comments and other not so nice characters */
  while(buf < bufend)
  {
    if(*buf == '\t' || *buf == '\r' || *buf == (string)0xA0)
      *(buf++) = ' ';
    else if(buf[0] == '/' && buf < bufend-1)
    {
      if(buf[1] == '*')
      {
        while(buf < bufend-1 && (buf[0] != '*' || buf[1] != '/'))
        {
          if(*buf != '\n')
            *buf = ' ';
          ++buf;
        }
        *(buf++) = ' ';
        *(buf++) = ' ';
      }
      else if(buf[1] == '/')
      {
        while(buf < bufend && buf[0] != '\n')
          *(buf++) = ' ';
        ++buf;
      }
      else
        ++buf;
    }
    else if(buf[0] == '#' && strncmp("#include", buf, 8))
    {
      while(buf < bufend && buf[0] != '\n')
        *(buf++) = ' ';
      ++buf;
    }
    else
      ++buf;
  }

#ifdef DEBUG_OLD
  printf("-----------\n%s-----------\n", linestart);
#endif

  buf = linestart;
  while(buf < bufend)
  {
    if(*buf == '\n')
    {
      ++buf; ++linenum;
      if(added)
      {
        linestart = buf;
        added = 0;
      }
    }
    else if(!strncmp("#include", buf, 8))
    {
      struct Include *d;

      if(!(d = (struct Include *) NewItem(&Includes)))
        return 0;
      d->Include = buf = SkipBlanks(buf+8);
      AddItem(&Includes, (struct ShortList *) d);
      while(*buf && *buf != '>' && *buf != '\n')
        ++buf;
      if(*buf == '>')
        ++buf;
      if(*buf == '\n')
        ++linenum;
      *(buf++) = 0;
#ifdef DEBUG_OLD
  printf("Added Include line %s\n", d->Include);
#endif
      added = 1;
    }
    else if(*buf == '('/*)*/)
    {
      int32 i;

      if((i = AddClibEntry(linestart, bufend, linenum)) == -1) /* no memory */
        return 0;
      else if(!i)
      {
        while(buf < bufend && *buf != '\n')
          ++buf; /* skip this line */
      }
      else
      {
        i -= buf-linestart;
        while(buf < bufend && i-- > 0)
        {
          if(*(buf++) == '\n')
          {
            linestart = buf;
            ++linenum;
          } /* skip this function */
        }
      }
      added = 1;
    }
    else
      ++buf;
  } /* while */
  return 1;
}

static int32 IsCPPType(struct CPP_NameType *data, uint8 type)
{
  if(!data || data->Flags || data->Type != type || data->PointerDepth)
    return 0;
  return type;
}

static uint32 CheckRegisterNum(strptr string, struct CPP_NameType *data)
{
  uint32 i, j;
  
  for(i = 0; i < MAXREG; ++i)
  {
    j = strlen(RegNames[i]);
    if(!strnicmp(string, RegNames[i], j))
    {
      string += j;
      if(*string == ' ' || *string == '\t' || *string == '\n' || *string == /*(*/')')
      {
        data->Register = i;
        data->Flags |= CPP_FLAG_REGISTER;
        return j;
      }
    }
  }
  return 0;
}

static uint32 ParseFuncPtrArgs(strptr buffer, struct CPP_NameType *data)
{
  strptr buf = buffer;
  struct ClibData d;

  memset(&d, 0, sizeof(struct ClibData));
  while(*buf != /*(*/')')
  {
    if(d.NumArgs == MAXREGPPC+1)
      return 0;
    else if(!GetCPPType(&d.Args[d.NumArgs++], buf, 1, 1))
      return 0;

    buf += d.Args[d.NumArgs-1].FullLength;
    while(*buf != ',' && *buf != /*(*/')')
      ++buf;
    if(*buf == ',')
      buf = SkipBlanksRet(++buf);
  }

  if(d.NumArgs == 1 && IsCPPType(&d.Args[0], CPP_TYPE_VOID))
    d.NumArgs = 0; /* void arguments are no arguments */

  if(d.NumArgs) /* no need to allocate structure for nothing */
  {
    if(!(data->FuncPtr = (struct ClibData *) AllocListMem(sizeof(struct ClibData))))
      return 0;

    memcpy(data->FuncPtr, &d, sizeof(struct ClibData));
  }
  return (uint32) (buf+1-buffer);
}

/* rettype turns on usage of "extern" specifier */
static int32 GetCPPType(struct CPP_NameType *data, strptr start, uint32 rettype, uint32 small)
{
  uint32 ok = 1, j;
  strptr u;

  data->Unknown = 0;
  data->Replace = 0;
  data->TypeStart = start = SkipBlanks(start);

  if(!strncmp(start, "REG", 3) && (start[3] == ' ' || start[3] == '\t' || start[3] == '\n' || start[3] == '('/*)*/))
  {
    u = SkipBlanksRet(start+3);
    if(*u == '('/*)*/)
    {
      u = SkipBlanks(u+1);
      if((j = CheckRegisterNum(u, data)))
      {
        u = SkipBlanks(u+j);
        if(*u == ')')
          start = SkipBlanks(u+1);
      }
    }
  }
  data->TypeStart = start;

  do
  {
    start = SkipBlanks((u = start));
    if(!strncmp("...",start,3))
    {
      data->Type = CPP_TYPE_VARARGS;
      data->TypeLength = start+3 - (data->TypeStart);
      data->FullLength = data->TypeLength;
      return 1;
    }
    if(CheckKeyword(start, "const", 5) || CheckKeyword(start, "CONST", 5))
    {
      data->Flags |= CPP_FLAG_CONST; start += 5;
    }
    else if(rettype && CheckKeyword(start, "extern", 6))
    {
      start += 6; /* ignore it */
    }
    else if(CheckKeyword(start, "long", 4))
    {
      if(data->Flags & CPP_FLAG_LONG)
        data->Type = CPP_TYPE_LONGLONG;
      else
        data->Flags |= CPP_FLAG_LONG;
      
      start += 4;
    }
    else if(CheckKeyword(start, "signed", 6))
      start += 6;
    else if(CheckKeyword(start, "unsigned", 8))
    {
      data->Flags |= CPP_FLAG_UNSIGNED; start += 8;
    }
    else if(CheckKeyword(start, "register", 8))
    {
      data->Flags |= CPP_FLAG_REGISTER; start += 8;
      data->Register = UNDEFREGISTER;
    }
    else if(CheckKeyword(start, "struct", 6))
    {
      start = SkipBlanks(start+6);
      data->Flags |= CPP_FLAG_STRUCT;
      if(*start == '?') /* ? for external types */
      {
        data->StructureLength = 0;
        data->StructureName = "";
        ++start;
      }
      else if(*start == '!') /* ! for typedef types */
      {
        data->Flags |= CPP_FLAG_TYPEDEFNAME;
        ++start;
        /* structure name and length already set */
      }
      else
      {
        start = SkipName((data->StructureName = start));
        data->StructureLength = start-data->StructureName;
      }
    }
    else if(CheckKeyword(start, "union", 5))
    {
      start = SkipBlanks(start+5);
      data->Flags |= CPP_FLAG_UNION;
      if(*start != '?') /* ? for external types */
      {
        start = SkipName((data->StructureName = start));
        data->StructureLength = start-data->StructureName;
      }
      else
      {
        data->StructureLength = 0;
        data->StructureName = "";
        ++start;
      }
    }
    else if(CheckKeyword(start, "enum", 4))
    {
      start = SkipBlanks(start+4);
      data->Flags |= CPP_FLAG_ENUM;
      if(*start != '?') /* ? for external types */
      {
        start = SkipName((data->StructureName = start));
        data->StructureLength = start-data->StructureName;
      }
      else
      {
        data->StructureLength = 0;
        data->StructureName = "";
        ++start;
      }
    }
    else if(*start == '*')
    {
      ++start; ++data->PointerDepth;
    }
    else if(*start == '[')
    {
      data->Flags |= CPP_FLAG_ARRAY;
      while(*start && *start != ']')
        ++start;
      if(*start)
        ++start;
    }
    else if(start[0] == '_' && start[1] == '_' && (j = CheckRegisterNum(start+2, data)))
      start += 2 + j;
    else if(!data->Type)
    {
      uint32 i;

      for(i = 0; CPP_Field[i].Text; ++i)
      {
        if(!strncmp(start, CPP_Field[i].Text, CPP_Field[i].Length) &&
        (start[CPP_Field[i].Length] == ' ' ||
        start[CPP_Field[i].Length] == '\t' ||
        start[CPP_Field[i].Length] == '\n' ||
        start[CPP_Field[i].Length] == ',' ||
        start[CPP_Field[i].Length] == /*(*/')' ||
        start[CPP_Field[i].Length] == '('/*)*/ ||
        start[CPP_Field[i].Length] == '*'))
        {
          start += CPP_Field[i].Length;
          data->Type = CPP_Field[i].Type;
          data->Flags |= CPP_Field[i].Flags;
          if(CPP_Field[i].Flags & CPP_FLAG_POINTER)
            ++data->PointerDepth;
          break;
        }
      }
      if(CPP_Field[i].Text)
        continue;
      else if(extnames)
      {
        struct CPP_ExternNames *a = extnames;
       
        while(a)
        {
          i = strlen(a->Type);
          if(!strncmp(a->Type, start, i) && !isalnum(start[i]) && 
          start[i] != '_')
          {
            start += i;
            data->StructureName = a->NameType.StructureName;
            data->FuncPtr = a->NameType.FuncPtr;
            data->StructureLength = a->NameType.StructureLength;
            data->PointerDepth += a->NameType.PointerDepth;
            data->Type = a->NameType.Type;
            data->Flags |= a->NameType.Flags;
            data->FuncArgs = a->NameType.FuncArgs;
            data->ArgsLength = a->NameType.ArgsLength;
            break;
          }
     
          /* check types here */
          a = a->Next;
        }
        if(a)
          continue;
        else if((!data->Type) && (!data->Flags))
        {
          long size;
          struct CPP_Unknown *u;

          data->Type = CPP_TYPE_INT;
          size = SkipName(start)-start;
          for(u = unknown; u && strncmp(u->Unknown, start, size); u = u->Next)
            ;
          if(!u)
          {
            data->Unknown = DupString(start, size);
            if((u = (struct CPP_Unknown *) AllocListMem(sizeof(struct CPP_Unknown))))
            {
              u->Next = unknown;
              u->Unknown = data->Unknown;
              unknown = u;
            }
          }
          start += size;
          continue;
        }
      }
      break;
    }
    else
      break;
  }while(1);

  if(start != SkipBlanks(u)) /* we broke the loop after increasing start */
    u = start;

  data->TypeLength = u - (data->TypeStart);
  data->FullLength = data->TypeLength;

  u = SkipBlanks(u);

  if(*u == '('/*)*/)
  {
    ok = 0;
    u = SkipBlanksRet(++u);
    if(*u == '*')
    {
      while(*u == '*')
      {
        ++data->FuncPointerDepth; ++u;
      }
      u = SkipBlanksRet(u);
      if(CheckKeyword(u, "const", 5) || CheckKeyword(u, "CONST", 5))
      {
        data->Flags |= CPP_FLAG_CONST; u += 6;
      }
      u = SkipBlanksRet(u);
      if(*u != /*(*/')')
        data->FunctionName = u;
      u = SkipBlanksRet(SkipName(u));
      if(*u == '('/*)*/)
      {
        int numclose = 1;
        ++u;
        while(*u && numclose)
        {
          if(*u == '('/*)*/) ++numclose;
          else if(*u == /*(*/')') --numclose;
          ++u;
        }
      }
      if(*u == /*(*/')')
      {
        u = SkipBlanksRet(++u);
        if(*u == '('/*)*/)
        {
          data->Flags |= CPP_FLAG_FUNCTION;

          if((j = ParseFuncPtrArgs(u+1, data)))
            ok = 1;
          data->FuncArgs = u;
          data->ArgsLength = j+1;
          data->FullLength = u+data->ArgsLength - (data->TypeStart);
        }
      }
    }
  }

  if(data->PointerDepth)
    data->Flags |= CPP_FLAG_POINTER;

  if(!(Flags2 & FLAG2_SMALLTYPES) && !small)
  {
    if(!(data->Flags & (CPP_FLAG_STRPTR|CPP_FLAG_POINTER|CPP_FLAG_ENUM
    |CPP_FLAG_STRUCT|CPP_FLAG_UNION|CPP_FLAG_FUNCTION|CPP_FLAG_REGISTER)))
    {
      if(data->Type == CPP_TYPE_BYTE || data->Type == CPP_TYPE_WORD || data->Type == CPP_TYPE_INT)
      {
        if(data->Flags & CPP_FLAG_UNSIGNED)
          data->Replace = "const ULONG";
        else
          data->Replace = "const LONG";
        data->Type = CPP_TYPE_LONG;
        if(!(data->Flags & CPP_FLAG_CONST))
          data->Replace += 6;
      }
    }
  }

  if(!data->Type && (data->Flags & CPP_FLAG_LONG))
    data->Type = CPP_TYPE_LONG;

  if((!data->Type && !data->Flags) || !ok)
    return 0;
  return 1;
}

static struct ClibData *GetClibFunc(strptr name, struct AmiPragma *ap, uint32 flags)
{
  struct ClibData *d = clibdata;

  if(!name)
  {
    DoError(ERR_ILLEGAL_INTERNAL_VALUE, 0);
    return 0;
  }

  while(d && strcmp(name, d->FuncName))
    d = d->Next;

  if(!d)
  {
    if(!(ap->Flags & AMIPRAGFLAG_NOCLIB))
    {
      DoError(ERR_PROTOTYPE_MISSING, 0, name);
      ap->Flags |= AMIPRAGFLAG_NOCLIB;
    }
  }
  else if(ap->CallArgs != d->NumArgs && (!(flags & FUNCFLAG_TAG) ||
  ap->CallArgs+1 != d->NumArgs))
  {
    if(!(ap->Flags & (AMIPRAGFLAG_CLIBARGCNT|AMIPRAGFLAG_DIDARGWARN)))
    {
      DoError(ERR_CLIB_ARG_COUNT, 0, name, d->NumArgs, ap->NumArgs);
      ap->Flags |= AMIPRAGFLAG_CLIBARGCNT;
    }
    return 0;
  }

  return d;  
}

static int32 CheckKeyword(strptr string, strptr keyword, int32 size)
{
  if(!strncmp(string, keyword, size))
  {
    string += size;
    if(*string == ' ' || *string == '\t' || *string == '\n')
      return size;
  }
  return 0;
}

/* return nonzero, when usable, zero, when string already emitted */
static uint32 CopyCPPType(strptr buffer, uint32 pass, struct ClibData *cd,
struct AmiArgs *args)
{
  uint32 ret = 0, reg;
  uint32 i, j, k = 0;

/* pass 0:   signed strptr, MaxonC++ high args */
/* pass 1: unsigned strptr, MaxonC++ high args */
/* pass 2:   signed strptr, StormC++ high args */
/* pass 3: unsigned strptr, StormC++ high args */

  for(i = 0; i < cd->NumArgs; ++i)
  {
    struct CPP_NameType *nt;

    nt = &cd->Args[i];

    if(args && (Flags & FLAG_LOCALREG) && (nt->Type != CPP_TYPE_VARARGS))
      reg = 1 + args[k].ArgReg;
    else if((nt->Flags & CPP_FLAG_REGISTER) && nt->Register != UNDEFREGISTER)
      reg = 1 + nt->Register;
    else
      reg = 0;

    if(reg--) /* subtract the added 1 */
    {
      *(buffer++) = CPP_TYPE_REGISTER;
      if(reg >= 10)
      {
        if(pass & 2)
        {
          *(buffer++) = reg/10 + '0';
          *(buffer++) = reg%10 + '0';
          ret |= 2;
        }
        else
          *(buffer++) = reg + (reg < 10 ? '0' : 'A'-10);
      }
      else
        *(buffer++) = reg + '0';
    }

    if(nt->Flags & CPP_FLAG_FUNCTION)
    {
      for(j = 0; j < nt->FuncPointerDepth; ++j)
        *(buffer++) = CPP_TYPE_POINTER;
      *(buffer++) = CPP_TYPE_FUNCTION;
    }
    for(j = 0; j < nt->PointerDepth; ++j)
      *(buffer++) = CPP_TYPE_POINTER;
    if(nt->Flags & CPP_FLAG_CONST)
      *(buffer++) = CPP_TYPE_CONST;
    if(nt->Flags & CPP_FLAG_UNSIGNED)
      *(buffer++) = CPP_TYPE_UNSIGNED;
    else if((nt->Flags & CPP_FLAG_STRPTR) && (pass & 1))
    {
      *(buffer++) = CPP_TYPE_UNSIGNED;
      ret |= 1; /* we really use this pass */
    }
    if(nt->Flags & CPP_FLAG_ENUM)
      *(buffer++) = CPP_TYPE_ENUM;
    if(nt->Type)
      *(buffer++) = cd->Args[i].Type;
    else
    {
      uint32 i;
      sprintf(buffer, "%02lu", (uint32) nt->StructureLength); buffer += 2;
      for(i = 0; i < nt->StructureLength; ++i)
        *(buffer++) = nt->StructureName[i];
    }
    if(nt->Flags & CPP_FLAG_FUNCTION)
    {
      if(nt->FuncPtr)
      {
        ret |= CopyCPPType(buffer, pass, nt->FuncPtr, 0);
        while(*buffer)
          ++buffer; /* skip to the new end */
      }
      *(buffer++) = CPP_TYPE_FUNCEND;
    }
    ++k;
    if(IsCPPType(nt, CPP_TYPE_DOUBLE))  /* double needs 2 registers */
      ++k;
  }

  *(buffer) = 0;

  if(ret != pass)
    ret = 0;
  if(!pass)
    ret = 0x80;

  return ret;  /* return nozero if this pass should be used */
}

static uint32 OutClibType(struct CPP_NameType *nt, strptr txt)
{
  if(nt->Replace)
    DoOutput("%s", nt->Replace);
  else
    DoOutputDirect(nt->TypeStart, nt->TypeLength);
  if(nt->Type != CPP_TYPE_VARARGS)
  {
    if(nt->Flags & CPP_FLAG_FUNCTION)
    {
      uint32 i;
      DoOutput(" ("/*)*/);
      for(i = 0; i < nt->FuncPointerDepth; ++i)
        DoOutput("*");
      if (txt)
        DoOutput("%s)", txt);
      else
        DoOutput(")");
      if(nt->FuncArgs)
        return DoOutputDirect(nt->FuncArgs, nt->ArgsLength);
      else
        return DoOutput("()");
    }
    else if(txt)
      return DoOutput(" %s", txt);
  }

  return 1;
}

static uint32 MakeClibType(strptr dest, struct CPP_NameType *nt, strptr txt)
{
  strptr a;

  a = dest;
  if(nt->Replace)
  {
    uint32 i;
    i = strlen(nt->Replace);
    memcpy(a, nt->Replace, i);
    a += i;
  }
  else
  {
    memcpy(a, nt->TypeStart, nt->TypeLength);
    a += nt->TypeLength;
  }

  if(nt->Type != CPP_TYPE_VARARGS)
  {
    if(nt->Flags & CPP_FLAG_FUNCTION)
    {
      if (txt)
        a += sprintf(a, " (*%s)", txt);
      else
        a += sprintf(a, " (*)");
      if(nt->FuncArgs)
      {
        memcpy(a, nt->FuncArgs, nt->ArgsLength);
        a += nt->ArgsLength;
      }
      else
        a += sprintf(a, "()");
    }
    else if(txt)
      a += sprintf(a, " %s", txt);
  }
  return (uint32)(a-dest);
}

static uint32 OutPASCALType(struct CPP_NameType *t, strptr txt, uint32 ret)
{
  int32 i = t->PointerDepth;

  if(t->Flags & CPP_FLAG_CONST)
    DoOutput("CONST ");
  if(!ret && i == 1 &&
  (t->Type == CPP_TYPE_LONG || t->Type == CPP_TYPE_WORD))
  {
    DoOutput("VAR "); --i;
  }

  DoOutput("%s : ", txt);

  if(!i && t->Flags == CPP_FLAG_BOOLEAN)
    return DoOutput("BOOLEAN");
  else if(i && t->Type == CPP_TYPE_VOID)
    return DoOutput("POINTER");
  else if(t->Flags & CPP_FLAG_FUNCTION)
    return DoOutput("tPROCEDURE");

  while(i--)
    DoOutput("p");

  if((t->Flags & (CPP_FLAG_STRUCT|CPP_FLAG_UNION)) && t->StructureLength)
  {
    if(!t->PointerDepth)
      DoOutput("t");
    return DoOutputDirect(t->StructureName, t->StructureLength);
  }

  if(t->Flags & CPP_FLAG_UNSIGNED)
  {
    if(t->Type == CPP_TYPE_LONG)
      return DoOutput("CARDINAL");
    if(t->Type == CPP_TYPE_WORD)
      return DoOutput("int16");
    if(t->Type == CPP_TYPE_BYTE)
      return DoOutput(t->PointerDepth == 1 ? "CHAR" : "int8");
  }
  else if(t->Type == CPP_TYPE_WORD)
    return DoOutput("INTEGER");
  else if(t->Type == CPP_TYPE_BYTE)
    return DoOutput("SHORTINT");
  return DoOutput("int32INT");
}

/* ------------------------------------------------------------------ */

static uint32 CallPrag(uint32 tagmode, strptr type, FuncType Func)
{
  if(type)
    if((*type && !DoOutput("#if%s\n", type)) ||
    !(CallFunc(tagmode, tagmode ? 0 : "/%s */\n", Func)) ||
    (*type && !DoOutput("#endif\n")))
      return 0;
  return 1;
}

static uint32 CreatePragmaFile(strptr amicall, strptr libcall, strptr amitags,
strptr libtags, uint32 mode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  switch(mode)
  {
  case PRAGMODE_PRAGLIB: DoOutput("#ifndef _INCLUDE_PRAGMA_%s_LIB_H\n"
    "#define _INCLUDE_PRAGMA_%s_LIB_H\n", ShortBaseNameUpper,
    ShortBaseNameUpper); break;
  case PRAGMODE_PRAGSLIB: DoOutput("#ifndef PRAGMAS_%s_LIB_H\n#define "
    "PRAGMAS_%s_LIB_H\n", ShortBaseNameUpper, ShortBaseNameUpper); break;
  case PRAGMODE_PRAGSPRAGS: DoOutput("#ifndef PRAGMAS_%s_PRAGMAS_H\n#define "
    "PRAGMAS_%s_PRAGMAS_H\n", ShortBaseNameUpper, ShortBaseNameUpper); break;
  case PRAGMODE_NONE: break;
  default: return 0;
  }

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  if(mode != PRAGMODE_NONE && !DoOutput("\n#ifndef CLIB_%s_PROTOS_H\n#include "
  "<clib/%s_protos.h>\n#endif\n\n", ShortBaseNameUpper, ShortBaseName))
    return 0;

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"))
    return 0;

  if(Flags & FLAG_GNUPRAG)
  {
    DoOutput("#ifdef " TEXT_GNUC "\n#ifdef NO_OBSOLETE\n"
    "#error \"Please include the proto file and not the compiler specific file!\"\n"
    "#endif\n#include <inline/%s.h>\n#endif\n\n", ShortBaseName);
    Flags |= FLAG_DONE;
  }

  if(
  !CallPrag(TAGMODE_NORMAL, amicall, FuncAMICALL) ||
  !CallPrag(TAGMODE_NORMAL, libcall, FuncLIBCALL))
    return 0;

  if(tagfuncs)
  {
    if(
    !CallPrag(TAGMODE_TAGS, amitags, FuncAMICALL) ||
    !CallPrag(TAGMODE_TAGS, libtags, FuncLIBCALL))
      return 0;
  }

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("\n#ifdef __cplusplus\n}\n#endif\n"))
    return 0;

  switch(mode)
  {
  case PRAGMODE_PRAGLIB: DoOutput("\n#endif\t/*  _INCLUDE_PRAGMA_%s_LIB_H  */\n",
    ShortBaseNameUpper); break;
  case PRAGMODE_PRAGSLIB: DoOutput("\n#endif\t/*  PRAGMAS_%s_LIB_H  */\n",
    ShortBaseNameUpper); break;
  case PRAGMODE_PRAGSPRAGS: DoOutput("\n#endif\t/*  PRAGMAS_%s_PRAGMA_H  */\n",
    ShortBaseNameUpper); break;
  case PRAGMODE_NONE: break;
  default: return 0;
  }
  return Output_Error;
}

static uint32 CreateCSTUBSFile(void)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef _INCLUDE_%s_CSTUBS_H\n#define _INCLUDE_%s_CSTUBS_H\n",
    ShortBaseNameUpper, ShortBaseNameUpper);

  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  if(!DoOutput("\n#ifndef CLIB_%s_PROTOS_H\n#include "
  "<clib/%s_protos.h>\n#endif\n\n", ShortBaseNameUpper, ShortBaseName))
    return 0;

  if(!CallFunc(TAGMODE_TAGS, "/%s */\n", FuncCSTUBS))
    return 0;

  return DoOutput("#endif\t/*  _INCLUDE_%s_CSTUBS_H  */\n",
  ShortBaseNameUpper);
}

static uint32 CreateLVOFile(uint32 mode)
{
  strptr data = "_LVO_I";

  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("* %s\n\n", AUTOHEADERTEXT);

  if(mode == 2 || mode == 4)
    data = "_LIB_I";

  if(!DoOutput("\t\tIFND LIBRARIES_%s%s\nLIBRARIES_%s%s\tSET\t1\n\n",
  ShortBaseNameUpper, data, ShortBaseNameUpper, data) ||
  (HEADER && (!DoOutput("\n") || !DoOutputDirect(HEADER, headersize))) ||
  (mode <= 2 && !CallFunc(TAGMODE_NORMAL, 0, FuncLVOXDEF)) ||
  !CallFunc(TAGMODE_NORMAL, "\n%s", FuncLVO) ||
  !DoOutput("\n\n\t\tENDC\n"))
    return 0;

  return 1;
}

static uint32 CreateLVOFilePPC(uint32 mode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("* %s\n\n", AUTOHEADERTEXT);

  if(!DoOutput("\t.ifndef LIBRARIES_%s_LIB_I\n.set\tLIBRARIES_%s_LIB_I,1\n\n",
  ShortBaseNameUpper, ShortBaseNameUpper))
    return 0;
  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }
  switch(mode)
  {
  case 0: CallFunc(TAGMODE_NORMAL, 0, FuncLVOPPCXDEF);
  case 1: CallFunc(TAGMODE_NORMAL, "\n%s", FuncLVOPPC);
  }
  return DoOutput("\n\t.endif\n");
}

static uint32 CreateAsmStubs(uint32 mode, uint32 callmode)
{
  if(mode == 1 && (Flags2 & FLAG2_AUTOHEADER)) DoOutput("* %s\n\n", AUTOHEADERTEXT);

  /* 1 = Text, 2 = Code */
  switch(mode)
  {
  case 1:
    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
    
    if(!(Flags & FLAG_ASMSECTION))
      DoOutput("\tSECTION\t\"%s\",CODE\n\t%sREF\t_%s\n", hunkname,
      Flags & FLAG_SMALLDATA ? "N" : "X", BaseName);
    if(!CallFunc(callmode, "\n%s", FuncAsmText))
      return 0;
    break;
  case 2:
    if(!CallFunc(callmode, 0, FuncAsmCode))
      return 0;
    break;
  }

  return 1;
}

static uint32 CreateProtoFile(uint32 Type)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef _PROTO_%s_H\n#define _PROTO_%s_H\n", ShortBaseNameUpper,
  ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n#ifndef EXEC_TYPES_H\n#include <exec/types.h>\n#endif\n");
  if(Type != 5)
    DoOutput("#if !defined(CLIB_%s_PROTOS_H) && !defined(" TEXT_GNUC ")\n"
    "#include <clib/%s_protos.h>\n#endif\n",
    ShortBaseNameUpper, ShortBaseName);

  if(BaseName)
  {
    DoOutput("\n#ifndef __NOLIBBASE__\nextern %s", GetBaseType());
    if(Type == 7)
      DoOutput("\n#ifdef __CONSTLIBBASEDECL__\n__CONSTLIBBASEDECL__\n"
      "#endif\n");
    DoOutput("%s;\n#endif\n", BaseName);
  }

  if(Type != 8)
  {
    if(Type >= 6)
    {
      DoOutput("\n#ifdef " TEXT_GNUC "\n");
      if(Type == 10)
        DoOutput("#ifndef __cplusplus\n");
      DoOutput("#ifdef __AROS__\n");
      DoOutput("#include <defines/%s.h>\n", ShortBaseName);
      DoOutput("#else\n");
      DoOutput("#include <inline/%s.h>\n", ShortBaseName);
      DoOutput("#endif\n");
      if(Type == 10)
        DoOutput("#endif\n");
      if(Type != 7)
      {
        if(Type == 9)
          DoOutput("#elif defined(" TEXT_VBCC ")\n"
          "#include <inline/%s_protos.h>\n#else", ShortBaseName);
        else
          DoOutput("#elif !defined(" TEXT_VBCC ")");
      }
    }
    if(Type == 10)
      DoOutput("\n#ifndef __PPC__");
    if(Type != 7)
    {
      strptr str1 = "pragma", str2 = "lib";

      switch(Type)
      {
        case 4: str1 = "pragmas"; /* no break; */
        case 2: str2 = "pragmas"; break;
        case 3: str1 = "pragmas"; break;
        case 5: str1 = "local"; str2 = "loc"; break;
      }
      DoOutput("\n#include <%s/%s_%s.h>\n", str1, ShortBaseName, str2);
    }
    if(Type == 10)
      DoOutput("#endif\n");
    if(Type >= 6)
      DoOutput("#endif\n");
  }

  Flags |= FLAG_DONE;

  return DoOutput("\n#endif\t/*  _PROTO_%s_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateLocalData(strptr to, uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef _INCLUDE_PROTO_%s_LOC_H\n"
  "#define _INCLUDE_PROTO_%s_LOC_H\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n");
  PrintIncludes();

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"))
    return 0;

  if(!CallFunc(callmode, "/%s */\n", FuncLocText))
    return 0;

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("#ifdef __cplusplus\n}\n#endif\n\n"))
    return 0;

  DoOutput("#endif\t/*  _INCLUDE_PROTO_%s_LOC_H  */\n", ShortBaseNameUpper);

  sprintf(filename, "%s_loc.lib", ShortBaseName);
  if(!CloseDest(to) || !OpenDest(filename))
    return 0;

  CallFunc(callmode, 0, FuncLocCode);
  
  return CloseDest(filename);
}

static uint32 CreateInline(uint32 mode, uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  DoOutput("#ifndef _%sINLINE_%s_H\n#define _%sINLINE_%s_H\n",
  Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", ShortBaseNameUpper,
  Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n");

  /* prevent loading of clib-file after inline */
  DoOutput("#ifndef CLIB_%s_PROTOS_H\n#define CLIB_%s_PROTOS_H\n#endif\n\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  if(!mode)
  {
    if(Flags & (FLAG_POWERUP|FLAG_MORPHOS))
      DoOutput("#ifndef __PPCINLINE_MACROS_H\n"
      "#include <ppcinline/macros.h>\n#endif\n\n");
    else
      DoOutput("#ifndef __INLINE_MACROS_H\n"
      "#include <inline/macros.h>\n#endif\n\n");
    Flags |= FLAG_INLINENEW;
  }
  else if(mode <= 2)
  {
    if(Flags & (FLAG_POWERUP|FLAG_MORPHOS))
      DoOutput("#ifndef __PPCINLINE_STUB_H\n"
      "#include <ppcinline/stubs.h>\n#endif\n\n");
    else
      DoOutput("#ifndef __INLINE_STUB_H\n"
      "#include <inline/stubs.h>\n#endif\n\n");
    if(mode == 2)
      Flags |= FLAG_INLINESTUB;
  }
  else if(mode == 3)
    Flags2 |= FLAG2_INLINEMAC;

  PrintIncludes();

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"))
    return 0;

  if(BaseName)
  {
    if(mode && mode <= 2)
    {
      if(Flags & FLAG_MORPHOS)
        DoOutput("#include <emul/emulregs.h>\n");
      DoOutput("#ifndef BASE_EXT_DECL\n#define BASE_EXT_DECL\n"
      "#define BASE_EXT_DECL0 extern %s %s;\n#endif\n"
      "#ifndef BASE_PAR_DECL\n#define BASE_PAR_DECL\n"
      "#define BASE_PAR_DECL0 void\n#endif\n"
      "#ifndef %s_BASE_NAME\n#define %s_BASE_NAME %s\n#endif\n\n"
      "BASE_EXT_DECL0\n\n", GetBaseType(), BaseName, ShortBaseNameUpper, ShortBaseNameUpper, BaseName);
    }
    else
      DoOutput("#ifndef %s_BASE_NAME\n#define %s_BASE_NAME %s\n#endif\n\n",
      ShortBaseNameUpper, ShortBaseNameUpper, BaseName);
  }

  if(mode <= 2)
  {
    if(!CallFunc(callmode, "/%s */\n", FuncInline))
      return 0;
  }
  else if(mode >= 6)
  {
    if(mode == 7)
      Flags |= FLAG_INLINENEW;
    if(!CallFunc(callmode, "/%s */\n", FuncInlineDirect))
      return 0;
  }
  else
  {
    if(!CallFunc(callmode, "/%s */\n", FuncInlineNS))
      return 0;
  }

  if(mode && mode <= 2 && BaseName)
    DoOutput("#undef BASE_EXT_DECL\n#undef BASE_EXT_DECL0\n"
    "#undef BASE_PAR_DECL\n#undef BASE_PAR_DECL0\n#undef %s_BASE_NAME\n\n", ShortBaseNameUpper);

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("\n#ifdef __cplusplus\n}\n#endif\n"))
    return 0;

  return DoOutput("#endif /*  _%sINLINE_%s_H  */\n",
  Flags & (FLAG_POWERUP|FLAG_MORPHOS) ? "PPC" : "", ShortBaseNameUpper);
}

static uint32 CreateGateStubs(uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }
  if(!prefix[0] && !subprefix[0])
  {
    DoError(ERR_PREFIX, 0); return 1;
  }

  DoOutput("#ifndef _GATESTUBS_%s_H\n#define _GATESTUBS_%s_H\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  DoOutput("%s\n#include <clib/%s_protos.h>\n#include <emul/emulregs.h>\n",
  premacro, ShortBaseName);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  if(BaseName)
  {
    DoOutput("#ifndef BASE_EXT_DECL\n#define BASE_EXT_DECL\n"
    "#define BASE_EXT_DECL0 extern %s %s;\n#endif\n"
    "#ifndef BASE_PAR_DECL\n#define BASE_PAR_DECL\n"
    "#define BASE_PAR_DECL0 void\n#endif\n"
    "#ifndef %s_BASE_NAME\n#define %s_BASE_NAME %s\n#endif\n\n"
    "BASE_EXT_DECL0\n", GetBaseType(), BaseName, ShortBaseNameUpper, ShortBaseNameUpper, BaseName);
  }

  DoOutput("\n");

  if(!CallFunc(callmode, "/%s */\n", FuncGateStubs))
    return 0;

  return DoOutput("#endif /*  _GATESTUBS_%s_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateSASPowerUP(uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef _PPCPRAGMA_%s_H\n#define _PPCPRAGMA_%s_H\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n#ifdef __GNUC__\n"
  "#ifndef _PPCINLINE__%s_H\n"
  "#include <ppcinline/%s.h>\n"
  "#endif\n"
  "#else\n\n"
  "#ifndef POWERUP_PPCLIB_INTERFACE_H\n"
  "#include <ppclib/interface.h>\n"
  "#endif\n\n"
  "#ifndef POWERUP_GCCLIB_PROTOS_H\n"
  "#include <gcclib/powerup_protos.h>\n"
  "#endif\n\n"
  "#ifndef NO_PPCINLINE_STDARG\n"
  "#define NO_PPCINLINE_STDARG\n"
  "#endif /* SAS-C PPC inlines */\n\n",
  ShortBaseNameUpper, ShortBaseName);

  if(BaseName)
  {
    DoOutput("#ifndef %s_BASE_NAME\n#define %s_BASE_NAME %s\n#endif\n\n",
    ShortBaseNameUpper, ShortBaseNameUpper, BaseName);
  }

  if(!CallFunc(callmode, "/%s */\n", FuncPowerUP))
    return 0;

  return DoOutput("#endif /* SAS-C PPC pragmas */\n"
  "#endif /*  _PPCPRAGMA_%s_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateProtoPowerUP(void)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef _PROTO_%s_H\n#define _PROTO_%s_H\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n#include <clib/%s_protos.h>\n", ShortBaseName);

  if(BaseName)
  {
    DoOutput("\n#ifndef __NOLIBBASE__\nextern %s", GetBaseType());
    DoOutput("\n#ifdef __CONSTLIBBASEDECL__\n__CONSTLIBBASEDECL__\n"
    "#endif\n%s;\n#endif\n", BaseName);
  }

  DoOutput("\n#ifdef " TEXT_GNUC "\n"
  "#ifdef __PPC__\n#include <ppcinline/%s.h>\n"
  "#else\n#include <inline/%s.h>\n#endif\n"
  "#else /* SAS-C */\n"
  "#ifdef __PPC__\n#include <ppcpragmas/%s_pragmas.h>\n"
  "#else\n#include <pragmas/%s_pragmas.h>\n#endif\n#endif\n",
  ShortBaseName, ShortBaseName, ShortBaseName, ShortBaseName);

  Flags |= FLAG_DONE;

  return DoOutput("\n#endif\t/*  _PROTO_%s_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateFPCUnit(void)
{

  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("(* %s *)\n\n", AUTOHEADERTEXT);

  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  DoOutput("{\n");
  DoOutput("  This is a unit for %s.library\n\n",ShortBaseName);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("**********************************************************************}\n\n");
  DoOutput("\n{\n  If there is no array of const in the unit\n   remove this compilerswitch \n}\n");
  DoOutput("{$mode objfpc}\n");
  DoOutput("{$I useamigasmartlink.inc}\n");
  DoOutput("{$ifdef use_amiga_smartlink}\n");
  DoOutput("   {$smartlink on}\n");
  DoOutput("{$endif use_amiga_smartlink}\n\n");

  DoOutput("UNIT %s;\n", ShortBaseNameUpper);

  DoOutput("\nINTERFACE\nUSES Exec;\n\nVAR %s : p%s;\n\n", BaseName, GetBaseTypeLib());

  DoOutput("const\n    %sNAME : PChar = '%s.library';\n\n",ShortBaseNameUpper,ShortBaseName);
  DoOutput("{\n  Here we read const, types and records for %s\n}\n",ShortBaseName);
  DoOutput("{$I %s.inc}\n\n",ShortBaseName);

  if(!CallFunc(TAGMODE_NORMAL, 0, FuncFPCType))
    return 0;

  DoOutput("{\n Functions and procedures with array of const go here\n}\n");
  if(!CallFunc(TAGMODE_TAGS, 0, FuncFPCTypeTags))
    return 0;

  DoOutput("\n{Here we read how to compile this unit}\n");
  DoOutput("{You can remove this include and use a define instead}\n");
  DoOutput("{$I useautoopenlib.inc}\n");
  DoOutput("{$ifdef use_init_openlib}\n");
  DoOutput("procedure Init%sLibrary;\n",ShortBaseNameUpper);
  DoOutput("{$endif use_init_openlib}\n");
  DoOutput("\n{This is a variable that knows how the unit is compiled}\n");
  DoOutput("var\n    %sIsCompiledHow : longint;\n",ShortBaseNameUpper);
  DoOutput("\nIMPLEMENTATION\n\n");
  DoOutput("{\n If you don't use array of const then just remove tagsarray \n}\n");
  DoOutput("uses \n");
  DoOutput("{$ifndef dont_use_openlib}\n");
  DoOutput("msgbox, \n");
  DoOutput("{$endif dont_use_openlib}\n");
  DoOutput("tagsarray;\n\n");

  if(!CallFunc(TAGMODE_NORMAL, "(%s *)\n", FuncFPCUnit))
    return 0;

  DoOutput("{\n Functions and procedures with array of const go here\n}\n");
  if(!CallFunc(TAGMODE_TAGS,"(%s *)\n", FuncFPCTypeTagsUnit))
    return 0;

  DoOutput("const\n    { Change VERSION and LIBVERSION to proper values }\n\n");
  DoOutput("    VERSION : string[2] = '0';\n");
  DoOutput("    LIBVERSION : Cardinal = 0;\n\n");

  DoOutput("{$ifdef use_init_openlib}\n");
  DoOutput("  {$Info Compiling initopening of %s.library}\n",ShortBaseName);
  DoOutput("  {$Info don't forget to use Init%sLibrary in the beginning of your program}\n",ShortBaseNameUpper);

  DoOutput("\nvar\n    %s_exit : Pointer;\n\n",ShortBaseName);
  DoOutput("procedure Close%sLibrary;\n",ShortBaseName);
  DoOutput("begin\n");
  DoOutput("    ExitProc := %s_exit;\n",ShortBaseName);
  DoOutput("    if %s <> nil then begin\n",BaseName);
  DoOutput("        CloseLibrary(%s);\n",BaseName);
  DoOutput("        %s := nil;\n",BaseName);
  DoOutput("    end;\n");
  DoOutput("end;\n\n");
  DoOutput("procedure Init%sLibrary;\n",ShortBaseNameUpper);
  DoOutput("begin\n    %s := nil;\n",BaseName);
  DoOutput("    %s := OpenLibrary(%sNAME,LIBVERSION);\n",BaseName, ShortBaseNameUpper);
  DoOutput("    if %s <> nil then begin\n",BaseName);
  DoOutput("        %s_exit := ExitProc;\n", ShortBaseName);
  DoOutput("        ExitProc := @Close%sLibrary;\n", ShortBaseName);
  DoOutput("    end else begin\n");
  DoOutput("        MessageBox('FPC Pascal Error',\n");
  DoOutput("        'Can''t open %s.library version ' + VERSION + #10 +\n",ShortBaseName);
  DoOutput("        'Deallocating resources and closing down',\n");
  DoOutput("        'Oops');\n");
  DoOutput("        halt(20);\n");
  DoOutput("    end;\n");
  DoOutput("end;\n\n");
  DoOutput("begin\n");
  DoOutput("    %sIsCompiledHow := 2;\n",ShortBaseNameUpper);
  DoOutput("{$endif use_init_openlib}\n\n");

  DoOutput("{$ifdef use_auto_openlib}\n");
  DoOutput("  {$Info Compiling autoopening of %s.library}\n",ShortBaseName);

  DoOutput("\nvar\n    %s_exit : Pointer;\n\n",ShortBaseName);
  DoOutput("procedure Close%sLibrary;\n",ShortBaseName);
  DoOutput("begin\n");
  DoOutput("    ExitProc := %s_exit;\n",ShortBaseName);
  DoOutput("    if %s <> nil then begin\n",BaseName);
  DoOutput("        CloseLibrary(%s);\n",BaseName);
  DoOutput("        %s := nil;\n",BaseName);
  DoOutput("    end;\n");
  DoOutput("end;\n\n");
  DoOutput("begin\n    %s := nil;\n",BaseName);
  DoOutput("    %s := OpenLibrary(%sNAME,LIBVERSION);\n",BaseName, ShortBaseNameUpper);
  DoOutput("    if %s <> nil then begin\n",BaseName);
  DoOutput("        %s_exit := ExitProc;\n", ShortBaseName);
  DoOutput("        ExitProc := @Close%sLibrary;\n", ShortBaseName);
  DoOutput("        %sIsCompiledHow := 1;\n",ShortBaseNameUpper);
  DoOutput("    end else begin\n");
  DoOutput("        MessageBox('FPC Pascal Error',\n");
  DoOutput("        'Can''t open %s.library version ' + VERSION + #10 +\n",ShortBaseName);
  DoOutput("        'Deallocating resources and closing down',\n");
  DoOutput("        'Oops');\n");
  DoOutput("        halt(20);\n");
  DoOutput("    end;\n\n");
  DoOutput("{$endif use_auto_openlib}\n\n");

  DoOutput("{$ifdef dont_use_openlib}\n");
  DoOutput("begin\n");
  DoOutput("    %sIsCompiledHow := 3;\n",ShortBaseNameUpper);
  DoOutput("   {$Warning No autoopening of %s.library compiled}\n",ShortBaseName);
  DoOutput("   {$Warning Make sure you open %s.library yourself}\n",ShortBaseName);
  DoOutput("{$endif dont_use_openlib}\n\n");

  return DoOutput("END. (* UNIT %s *)\n", ShortBaseNameUpper);
}

static uint32 CreateBMAP(void)
{
  return CallFunc(TAGMODE_NORMAL, 0, FuncBMAP);
}

static uint32 CreateLVOLib(void)
{
  uint32 i;

  i = strlen(ShortBaseNameUpper);
  EndPutM32(tempbuf, HUNK_UNIT);
  EndPutM32(tempbuf+4, (i+3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutputDirect(ShortBaseNameUpper, i);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  i = strlen(hunkname);
  EndPutM32(tempbuf, HUNK_NAME);
  EndPutM32(tempbuf+4, (i + 3)>>2);
  DoOutputDirect(tempbuf, 8);
  DoOutputDirect(hunkname, i);
  DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

  EndPutM32(tempbuf, HUNK_CODE);
  EndPutM32(tempbuf+4, 0);
  EndPutM32(tempbuf+8, HUNK_EXT);
  DoOutputDirect(tempbuf, 12);

  if(!CallFunc(TAGMODE_NORMAL, 0, FuncLVOLib))
    return 0;

  EndPutM32(tempbuf, 0);
  EndPutM32(tempbuf+4, HUNK_END);
  return DoOutputDirect(tempbuf, 8);
}

static uint32 CreateLVOLibPPC(void)
{
  uint8 *data = tempbuf, *data2, *data3;

  *(data++) = 0x7F;                                             /* eeh->e_ident[EI_MAG0] */
  *(data++) = 'E';                                              /* eeh->e_ident[EI_MAG1] */
  *(data++) = 'L';                                              /* eeh->e_ident[EI_MAG2] */
  *(data++) = 'F';                                              /* eeh->e_ident[EI_MAG3] */
  *(data++) = ELFCLASS32;                                       /* eeh->e_ident[EI_CLASS] */
  *(data++) = ELFDATA2MSB;                                      /* eeh->e_ident[EI_DATA] */
  *(data++) = EV_CURRENT;                                       /* eeh->e_ident[EI_VERSION] */
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  *(data++) = 0; *(data++) = 0; *(data++) = 0;
  EndPutM16Inc(data, ET_REL);                                   /* eeh->e_type */
  EndPutM16Inc(data, EM_POWERPC);                               /* eeh->e_machine */
  EndPutM32Inc(data, EV_CURRENT);                               /* eeh->e_version */
  EndPutM32Inc(data, 0);                                        /* eeh->e_entry */
  EndPutM32Inc(data, 0);                                        /* eeh->e_phoff */
  data2 = data; data += 4;
  EndPutM32Inc(data, 0);                                        /* eeh->e_flags */
  EndPutM16Inc(data, 52);                                       /* eeh->e_ehsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phentsize */
  EndPutM16Inc(data, 0);                                        /* eeh->e_phnum */
  EndPutM16Inc(data, 40);                                       /* eeh->e_shentsize */
  EndPutM16Inc(data, 4);                                        /* eeh->e_shnum */
  EndPutM16Inc(data, 1);                                        /* eeh->e_shstrndx - first table is string table */

  data3 = data;
  memcpy(data, "\0.symtab\0.strtab\0.shstrtab\0\0", 28);
  data += 28;  /* 1        9        17*/
  EndPutM32(data2, data-tempbuf); /* store the entry */

  EndPutM32Inc(data, 0);                                        /* esh[0].sh_name */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_offset */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  EndPutM32Inc(data, 17);                                       /* esh[3].sh_name = .shstrtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[3].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_addr */
  EndPutM32Inc(data, data3-tempbuf);                            /* esh[3].sh_offset */
  EndPutM32Inc(data, 27);                                       /* esh[3].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[3].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[3].sh_entsize */

  EndPutM32Inc(data, 1);                                        /* esh[4].sh_name = .symtab */
  EndPutM32Inc(data, SHT_SYMTAB);                               /* esh[4].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[4].sh_addr */
  data2 = data;
  data += 4;                                                    /* esh[4].sh_offset */
  data += 4;                                                    /* esh[4].sh_size */
  EndPutM32Inc(data, 3);                                        /* esh[4].sh_link - the third entry is our string table */
  EndPutM32Inc(data, 1);                                        /* esh[4].sh_info - One greater than index of last LOCAL symbol*/
  EndPutM32Inc(data, 4);                                        /* esh[4].sh_addralign */
  EndPutM32Inc(data, 16);                                       /* esh[4].sh_entsize = sizeof(struct Elf32_Sym) */

  EndPutM32Inc(data, 9);                                        /* esh[0].sh_name = .strtab */
  EndPutM32Inc(data, SHT_STRTAB);                               /* esh[0].sh_type */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_flags */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_addr */
  data3 = data;
  data += 4;                                                    /* esh[0].sh_offset */
  data += 4;                                                    /* esh[0].sh_size */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_link */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_info */
  EndPutM32Inc(data, 1);                                        /* esh[0].sh_addralign */
  EndPutM32Inc(data, 0);                                        /* esh[0].sh_entsize */

  EndPutM32Inc(data2, data-tempbuf);

  EndPutM32Inc(data,0);
  EndPutM32Inc(data,0); /* first entry is empty */
  EndPutM32Inc(data,0);
  EndPutM32Inc(data,0);

  symoffset = 1; /* initial value */
  elfbufpos = data;

  if(!CallFunc(TAGMODE_NORMAL, 0, FuncLVOPPCBias))
    return 0;
  EndPutM32(data2, elfbufpos-data+16);
  EndPutM32Inc(data3, elfbufpos-tempbuf);
  EndPutM32(data3, symoffset);

  *(elfbufpos++) = 0; /* first sym entry */
  if(!DoOutputDirect(tempbuf, elfbufpos-tempbuf))
    return 0;

  if(!CallFunc(TAGMODE_NORMAL, 0, FuncLVOPPCName))
    return 0;

  while((symoffset++)&3)
  {
    if(!DoOutputDirect("", 1))
      return 0;
  }

  return 1;
}

static uint32 CreateVBCCInline(uint32 mode, uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  DoOutput("#ifndef _VBCCINLINE_%s_H\n#define _VBCCINLINE_%s_H\n",
  ShortBaseNameUpper, ShortBaseNameUpper);

  DoOutput("\n#ifndef EXEC_TYPES_H\n#include <exec/types.h>\n#endif\n");
  if (mode == 2)
  {
    /* always include emul/emulregs.h in MorphOS inlines,
       gcc-based sources might depend on it :| */
    DoOutput("#ifndef EMUL_EMULREGS_H\n#include <emul/emulregs.h>\n#endif\n");
  }

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  DoOutput("\n");

  if(!CallFunc(callmode, "/%s */\n", mode ? (mode == 2 ? FuncVBCCMorphInline
  : FuncVBCCWOSInline) : FuncVBCCInline))
    return 0;

  return DoOutput("#endif /*  _VBCCINLINE_%s_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateVBCC(uint32 mode, uint32 callmode)
{
  uint32 res = 0;

  if(mode != 2 && mode != 3)
  {
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }
  }

  switch(mode)
  {
  case 4: res = CallFunc(callmode, 0, FuncVBCCPUPText); break;

  case 3: Flags |= FLAG_WOSLIBBASE; /* no break! */
  case 2: res = CallFunc(callmode, 0, FuncVBCCWOSCode); break;

  case 1: Flags |= FLAG_WOSLIBBASE; /* no break! */
  case 0: res = CallFunc(callmode, "\n%s", FuncVBCCWOSText); break;
  }
  return res;
}

static uint32 CreateVBCCPUPLib(uint32 callmode)
{
  /* output header */
  DoOutput("!<arch>\n");

  return CallFunc(callmode, 0, FuncVBCCPUPCode);
}

static uint32 CreateVBCCMorphCode(uint32 callmode)
{
  /* output header */
  DoOutput("!<arch>\n");

  return CallFunc(callmode, 0, FuncVBCCMorphCode);
}

static uint32 CreateEModule(uint32 sorted)
{
  uint32 res = 0, i;
  if(sorted)
    DoError(ERR_NO_SORTED, 0);
  else
  {
    DoOutputDirect("EMOD\0\x06", 6);
    for(res = 0; res < 2; ++res)
    {
      for(i = 0; BaseName[i]; ++i)
        DoOutput("%c", tolower(BaseName[i]));
      DoOutputDirect("\x00",1);
    }
    LastBias = BIAS_START-BIAS_OFFSET;
    CallFunc(TAGMODE_NORMAL, 0, FuncEModule);
    res = DoOutputDirect("\xFF",1);
  }
  return res;
}

static uint32 CreateProtoRedirect(void)
{
  Flags |= FLAG_DONE;
  return DoOutput("#ifdef NO_OBSOLETE\n"
  "#error \"Please include the proto file and not the compiler specific file!\"\n"
  "#endif\n\n#include <proto/%s.h>\n", ShortBaseName);
}

static uint32 CreateSFD(uint32 callmode)
{
  struct Include *inc;
  struct AmiPragma *ap;
  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  if((ap = (struct AmiPragma *) AmiPragma.First))
    LastBias = ap->Bias-BIAS_OFFSET;
  else /* only security, never used normally */
    LastBias = 0;
  CurrentABI = ABI_M68K;

  if(IDstring)
    DoOutput("==id %s\n", IDstring);
  else
  {
    time_t t;
    struct tm * tim;

    t = time(&t);
    tim = localtime(&t);
 
    DoOutput("==id %cId: %s,v 1.0 %04d/%02d/%02d %02d:%02d:%02d "
    "noname Exp $\n", '$', filename, tim->tm_year+1900, tim->tm_mon+1,
    tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
  }

  if(BaseName)
    DoOutput("* \"%s\"\n==base _%s\n==basetype %s\n==libname %s\n",
    GetLibraryName(), BaseName, GetBaseType(), GetLibraryName());
  DoOutput("==bias %ld\n==public\n", LastBias+BIAS_OFFSET);

  for(inc = (struct Include *) Includes.First; inc; inc = (struct Include *) inc->List.Next)
    DoOutput("==include %s\n", inc->Include);
  if(!Includes.First)
    DoOutput("==include <exec/types.h>\n");

  CallFunc(callmode, "%s\n", FuncSFD);

  return DoOutput("==end\n");
}

static uint32 CreateClib(uint32 callmode)
{
  if(!clibdata)
  {
    DoError(ERR_NOPROTOTYPES_FILE, 0); return 1;
  }

  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  DoOutput("#ifndef CLIB_%s_PROTOS_H\n#define CLIB_%s_PROTOS_H\n\n", ShortBaseNameUpper,
  ShortBaseNameUpper);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }
  else
  {
    strptr s = 0;
    time_t t;
    struct tm * tim;

    t = time(&t);
    tim = localtime(&t);

    if(IDstring)
    {
      s = SkipBlanks(IDstring+4);
      while(*s && *s != ' ')
        ++s;
      s=SkipBlanks(s);
    }
    if(!s || !*s)
      s = "1.0";

    if(Flags2 & FLAG2_SYSTEMRELEASE)
    {
      DoOutput("\n/*\n**\t$Id: %s %s\n", filename, s);
    }
    else
    {
      strptr t;

      t = s;
      while(*t && *t != ' ')
        ++t;
      *t = 0;
      
      DoOutput("\n/*\n**\t$%s: %s %s (%02d.%02d.%04d)\n", "VER", filename, s,
      tim->tm_mday, tim->tm_mon+1, tim->tm_year+1900);
    }
    DoOutput("**\n**\tC prototypes. For use with 32 bit integers only.\n**\n**\t");
    if(!Copyright || (Copyright && strncmp("Copyright ", Copyright, 10)))
      DoOutput("Copyright %c %d ", 0xa9, tim->tm_year+1900);
    DoOutput("%s\n", Copyright ? Copyright : Flags2 & FLAG2_SYSTEMRELEASE ?
    "Amiga, Inc." : "");
    DoOutput("**\tAll Rights Reserved\n*/\n\n");
  }

  PrintIncludes();

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n"))
    return 0;

  CallFunc(callmode, "\n/%s */\n\n", FuncClib);

  if((Flags & FLAG_EXTERNC) &&
  !DoOutput("\n#ifdef __cplusplus\n}\n#endif\n"))
    return 0;

  return DoOutput("\n#endif\t/*  CLIB_%s_PROTOS_H  */\n", ShortBaseNameUpper);
}

static uint32 CreateFD(void)
{
  LastBias = 0;
  CurrentABI = ABI_M68K;

  if(BaseName)
    DoOutput("##base _%s\n", BaseName);
  DoOutput("##public\n");

  CallFunc(TAGMODE_NORMAL, "%s\n", FuncFD);

  return DoOutput("##end\n");
}

static uint32 CreateGenAuto(strptr to, uint32 type)
{
  strptr name, btype;
  uint8 *data;
  uint32 i, verref, exitfuncref, sysref2, exitref, rel1, rel2, nameref;
  if(!(name = GetLibraryName()))
    return 0;
  btype = GetBaseType();

  switch(type)
  {
  case 0:
    Flags |= FLAG_DONE;
    if(!(DoOutput("#include <exec/libraries.h>\n#include <proto/exec.h>\n\n"
    "%s %s = 0;\nextern unsigned long _%sVer;\n\n"
    "void _INIT_%ld_%s(void)\n{\n  if(!(%s = %sOpenLibrary(\"%s\", _%sVer)))\n    exit(20);\n}\n\n"
    "void _EXIT_%ld_%s(void)\n{\n  if(%s)\n    CloseLibrary(%s%s);\n}\n",
    btype, BaseName, BaseName,
    priority, BaseName, BaseName, !strcmp("struct Library *", btype) ? "" : "(struct Library *) ", name, BaseName,
    priority, BaseName, BaseName, !strcmp("struct Library *", btype) ? "" : "(struct Library *) ", BaseName)))
      return 0;
    sprintf(filename, "%s_autoopenver.c", ShortBaseName);
    if(!CloseDest(to) || !OpenDest(filename))
      return 0;
    Flags |= FLAG_DONE;
    return DoOutput("unsigned long _%sVer = 0;\n", BaseName);
    break;
  case 1: /* m68k */
    Flags |= FLAG_DONE;
    i = strlen(filename)-4; /* remove .lib extension */
    EndPutM32(tempbuf, HUNK_UNIT);
    EndPutM32(tempbuf+4, (i+3)>>2);
    DoOutputDirect(tempbuf, 8);
    DoOutputDirect(filename, i);
    DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

    i = strlen(hunkname);
    EndPutM32(tempbuf, HUNK_NAME);
    EndPutM32(tempbuf+4, (i + 3)>>2);
    DoOutputDirect(tempbuf, 8);
    DoOutputDirect(hunkname, i);
    DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

    data = tempbuf+8; /* we need HUNK_CODE + size at start */
    EndPutM16Inc(data, 0x2F0E);                 /* MOVE.L A6,-(A7) */
    /* SysBase */
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM16Inc(data, 0x2C6C);                 /* MOVEA.L base(A4),A6 */
      EndPutM16Inc(data, 0);                      /* place for sysbase reference */
    }
    else
    {
      EndPutM16Inc(data, 0x2C79);                 /* MOVEA.L base,A6 */
      EndPutM32Inc(data, 0);                      /* place for sysbase reference */
    }
    verref = data-tempbuf-8+2;
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM16Inc(data, 0x202C);                 /* MOVE.L xxx(A4),D0 */
      EndPutM16Inc(data, 0);                      /* place for basevers reference */
    }
    else
    {
      EndPutM16Inc(data, 0x2039);                 /* MOVE.L xxx,D0 */
      EndPutM32Inc(data, 0);                      /* place for basevers reference */
    }
    EndPutM32Inc(data, 0x43FA0030 + ((Flags2 & FLAG2_SMALLCODE) ? 0 : 2) + ((Flags & FLAG_SMALLDATA) ? 0 : 6));
    EndPutM32Inc(data, 0x4EAEFDD8);               /* JSR _LVOOpenLibrary(A6) */

    rel1 = data-tempbuf-8+2;
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM16Inc(data, 0x2940);                 /* MOVE.L D0,xxx(A4) */
      EndPutM16Inc(data, 0);
    }
    else
    {
      EndPutM16Inc(data, 0x23C0);                 /* MOVE.L D0,xxx */
      EndPutM32Inc(data, 0);
    }
    EndPutM16Inc(data, 0x660A + ((Flags2 & FLAG2_SMALLCODE) ? 0 : 2)); /* BNE.B .lib */
    EndPutM32Inc(data, 0x48780014);               /* PEA 20 */

    exitfuncref = data-tempbuf-8+2;
    if(Flags2 & FLAG2_SMALLCODE)
    {
      EndPutM16Inc(data, 0x4EBA);                 /* JSR _exit(PC) */
      EndPutM16Inc(data, 0);                      /* place for base reference */
    }
    else
    {
      EndPutM16Inc(data, 0x4EB9);                 /* JSR _exit */
      EndPutM32Inc(data, 0);                      /* place for base reference */
    }
    EndPutM16Inc(data, 0x584F);                   /* ADDQ.W, #4,A7 */
    EndPutM16Inc(data, 0x2C5F);                   /* MOVE.L (A7)+,A6 */
    EndPutM16Inc(data,0x4E75);                    /* RTS */
    exitref = data-tempbuf-8;
    
    EndPutM16Inc(data, 0x2F0E);                   /* MOVE.L A6,-(A7) */
    sysref2 = data-tempbuf-8+2;
    /* SysBase */
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM16Inc(data, 0x2C6C);                 /* MOVEA.L base(A4),A6 */
      EndPutM16Inc(data, 0);                      /* place for sysbase reference */
    }
    else
    {
      EndPutM16Inc(data, 0x2C79);                 /* MOVEA.L base,A6 */
      EndPutM32Inc(data, 0);                      /* place for sysbase reference */
    }
    rel2 = data-tempbuf-8+2;
    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM16Inc(data, 0x202C);                 /* MOVE.L xxx(A4),D0 */
      EndPutM16Inc(data, 0);                      /* place for base reference */
    }
    else
    {
      EndPutM16Inc(data, 0x2039);                 /* MOVE.L xxx,D0 */
      EndPutM32Inc(data, 0);                      /* place for base reference */
    }
    EndPutM16Inc(data, 0x6606);                   /* BNE.B .nolib */
    EndPutM16Inc(data, 0x2240);                   /* MOVEA.L D0,A1 */

    EndPutM32Inc(data, 0x4EAEFE62);               /* JSR _LVOCloseLibrary(A6) */
    EndPutM16Inc(data, 0x2C5F);                   /* MOVE.L (A7)+,A6 */
    EndPutM16Inc(data,0x4E75);                    /* RTS */
    nameref = data-tempbuf-8;
    memcpy(data, name, strlen(name));
    data += strlen(name);
    do { *(data++) = 0; } while((data-tempbuf)&3);

    EndPutM32(tempbuf, HUNK_CODE);
    EndPutM32(tempbuf+4, (data-tempbuf-8)>>2)
    DoOutputDirect(tempbuf, (size_t)(data-tempbuf)&(~3));

    if(Flags & FLAG_SMALLDATA)
    {
      EndPutM32(tempbuf, HUNK_DREL16);
    }
    else
    {
      EndPutM32(tempbuf, HUNK_ABSRELOC32);
    }
    EndPutM32(tempbuf+4, 2); /* 2 entries */
    EndPutM32(tempbuf+8, 1); /* to hunk 1 */
    EndPutM32(tempbuf+12, rel1); /* address 0 */
    EndPutM32(tempbuf+16, rel2); /* address 0 */
    EndPutM32(tempbuf+20, 0); /* end of reloc hunk */
    DoOutputDirect(tempbuf, 24);

    /* extern references */
    EndPutM32(tempbuf, HUNK_EXT);
    DoOutputDirect(tempbuf, 4);

    OutputXREF2(4, sysref2, (Flags & FLAG_SMALLDATA ? EXT_DEXT16 : EXT_REF32), "_SysBase");
    OutputXREF(verref, (Flags & FLAG_SMALLDATA ? EXT_DEXT16 : EXT_REF32), "__%sVer", BaseName);
    OutputXREF(exitfuncref, (Flags2 & FLAG2_SMALLCODE ? EXT_DEXT16 : EXT_REF32), "_exit");
    OutputXDEF(0, "__INIT_%ld_%s", priority, BaseName);
    OutputXDEF(exitref, "__EXIT_%ld_%s", priority, BaseName);
    OutputXDEF(nameref, "%sname", ShortBaseName);
    EndPutM32(tempbuf, 0); /* ext end */
    DoOutputDirect(tempbuf, 4);

    if(!(Flags & FLAG_NOSYMBOL))
    {
      EndPutM32(tempbuf, HUNK_SYMBOL);
      DoOutputDirect(tempbuf, 4);
      OutputSYMBOL(0, "__INIT_%ld_%s", priority, BaseName);
      OutputSYMBOL(exitref, "__EXIT_%ld_%s", priority, BaseName);
      OutputSYMBOL(nameref, "%sname", ShortBaseName);
      EndPutM32(tempbuf, 0);
      DoOutputDirect(tempbuf, 4);
    }

    EndPutM32(tempbuf, HUNK_END);
    DoOutputDirect(tempbuf, 4);

    i = strlen(datahunkname);
    EndPutM32(tempbuf, HUNK_NAME);
    EndPutM32(tempbuf+4, (i + 3)>>2);
    DoOutputDirect(tempbuf, 8);
    DoOutputDirect(datahunkname, i);
    DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

    EndPutM32(tempbuf, HUNK_BSS);
    EndPutM32(tempbuf+4, 1);
    DoOutputDirect(tempbuf, 8);

    EndPutM32(tempbuf, HUNK_EXT);
    DoOutputDirect(tempbuf, 4);
    OutputXDEF(0, "_%s", BaseName);
    EndPutM32(tempbuf, 0); /* ext end */
    DoOutputDirect(tempbuf, 4);

    if(!(Flags & FLAG_NOSYMBOL))
    {
      EndPutM32(tempbuf, HUNK_SYMBOL);
      DoOutputDirect(tempbuf, 4);
      OutputSYMBOL(0, "_%s", BaseName);
      EndPutM32(tempbuf, 0);
      DoOutputDirect(tempbuf, 4);
    }

    EndPutM32(tempbuf, HUNK_END);
    DoOutputDirect(tempbuf, 4);

    sprintf(filename, "%s_autoopenver", ShortBaseName);
    i = strlen(filename);
    EndPutM32(tempbuf, HUNK_UNIT);
    EndPutM32(tempbuf+4, (i+3)>>2);
    DoOutputDirect(tempbuf, 8);
    DoOutputDirect(filename, i);
    DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

    i = strlen(datahunkname);
    EndPutM32(tempbuf, HUNK_NAME);
    EndPutM32(tempbuf+4, (i + 3)>>2);
    DoOutputDirect(tempbuf, 8);
    DoOutputDirect(datahunkname, i);
    DoOutputDirect("\0\0\0", ((i+3)&(~3))-i);

    EndPutM32(tempbuf, HUNK_BSS);
    EndPutM32(tempbuf+4, 1);
    DoOutputDirect(tempbuf, 8);

    EndPutM32(tempbuf, HUNK_EXT);
    DoOutputDirect(tempbuf, 4);
    OutputXDEF(0, "_%sVer", BaseName);
    EndPutM32(tempbuf, 0); /* ext end */
    DoOutputDirect(tempbuf, 4);

    if(!(Flags & FLAG_NOSYMBOL))
    {
      EndPutM32(tempbuf, HUNK_SYMBOL);
      DoOutputDirect(tempbuf, 4);
      OutputSYMBOL(0, "_%sVer", BaseName);
      EndPutM32(tempbuf, 0);
      DoOutputDirect(tempbuf, 4);
    }

    EndPutM32(tempbuf, HUNK_END);
    return DoOutputDirect(tempbuf, 4);

    break;
  }
  return 0;
}

static uint32 CreateXML(void)
{
  struct Include *inc;
  char *basetypelib;

  LastBias = 30;
  DoOutput(
  "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n"
  "<!DOCTYPE library SYSTEM \"library.dtd\">\n"
  "<library name=\"%s\" basename=\"%s\" openname=\"%s\"",
  ShortBaseName, BaseName, GetLibraryName());
  basetypelib = GetBaseTypeLib();
  if(basetypelib && (strcmp(basetypelib, "Library") != 0))
    DoOutput(" basetype=\"%s\"", basetypelib);
  DoOutput(">\n");
  for(inc = (struct Include *) Includes.First; inc;
  inc = (struct Include *) inc->List.Next)
    DoOutput("\t<include>%.*s</include>\n", (int)(strlen(inc->Include)-2),
    inc->Include+1);
  if(!Includes.First)
    DoOutput("\t<include>exec/types.h</include>\n");

  DoOutput("\t<interface name=\"main\" version=\"1.0\" struct=\"%sIFace\""
  " prefix=\"_%s_\" asmprefix=\"I%s\" global=\"I%s\">\n",
  GetIFXName(), GetIFXName(), GetIFXName(), GetIFXName());
  DoOutput(
  "\t\t<method name=\"Obtain\" result=\"ULONG\"/>\n"
  "\t\t<method name=\"Release\" result=\"ULONG\"/>\n"
  "\t\t<method name=\"Expunge\" result=\"void\" status=\"unimplemented\"/>\n"
  "\t\t<method name=\"Clone\" result=\"struct Interface *\""
  " status=\"unimplemented\"/>\n");

  CallFunc(TAGMODE_BOTH, 0, FuncXML);

  return DoOutput("\t</interface>\n</library>\n");
}

static uint32 CreateOS4PPC(uint32 callmode)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }

  PrintIncludes();

  DoOutput(
  "#include <stdarg.h>\n"
  "#include <exec/types.h>\n"
  "#include <exec/interfaces.h>\n"
  "#include <exec/emulation.h>\n"
  "#include <interfaces/exec.h>\n");
  
  if(!stricmp("exec",ShortBaseName))
    DoOutput("#include <interfaces/%s.h>\n", ShortBaseName);

  DoOutput("#include \"%s_vectors.c\"\n\n", ShortBaseName);

  CallFunc(callmode, "\n/%s */\n\n", FuncOS4PPC);

  DoOutput(
  "ULONG _%s_Obtain(struct %sIFace *Self)\n{\n"
  "  return Self->Data.RefCount++;\n}\n\n"
  "ULONG _%s_Release(struct %sIFace *Self)\n{\n"
  "  return Self->Data.RefCount--;\n}\n\n"
  "#define LIBNAME \"%s\"\n"
  "#define LIBVERSION 0\n"
  "#define IFACENAME \"%s.main\"\n\n",
  GetIFXName(), GetIFXName(), GetIFXName(), GetIFXName(),
  GetLibraryName(), GetLibraryName());

  /* following text is constant */
  return DoOutput(
  "static void InitFunction(APTR dummy, ULONG SegList, "
   "struct ExecBase *ExecBase)\n{\n"
  "  struct Library *LibBase;\n"
  "  struct ExecIFace *IExec = (struct ExecIFace *)"
   "ExecBase->MainInterface;\n"
  "  if((LibBase = IExec->OpenLibrary(LIBNAME, LIBVERSION)))\n"
  "  {\n"
  "    struct Interface *NewInterface;\n"
  "    if((NewInterface = IExec->MakeInterfaceTags(LibBase,\n"
  "      MIT_VectorTable, main_vectors,\n"
  "      MIT_Version,     1,\n"
  "      MIT_Name,        IFACENAME,\n"
  "      TAG_DONE)))\n"
  "    {\n"
  "      NewInterface->Data.IExecPrivate = (APTR)IExec;\n"
  "      IExec->AddInterface(LibBase, NewInterface);\n"
  "    }\n"
  "  }\n"
  "}\n\n"
  "volatile static struct Resident MyResident =\n{\n"
  "  RTC_MATCHWORD,\n"
  "  (struct Resident *)&MyResident,\n"
  "  (APTR)(&MyResident+1),\n"
  "  RTF_NATIVE,\n"
  "  LIBVERSION,\n"
  "  NT_UNKNOWN,\n"
  "  -120,\n"
  "  IFACENAME,\n"
  "  IFACENAME,\n"
  "  InitFunction\n"
  "};\n\n"
  "void _start(void)\n"
  "{\n  /* printf(\"This program cannot be run in DOS mode :-)\\n\"); */"
  "\n}\n");
}

static uint32 CreateOS4M68K(void)
{
  if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

  if(HEADER)
  {
    DoOutput("\n");
    DoOutputDirect(HEADER, headersize);
  }
  DoOutput(
  "#include \"exec/interfaces.i\"\n"
  "#include \"exec/libraries.i\"\n"
  "#include \"exec/emulation.i\"\n"
  "#include \"interfaces/%s.i\"\n\n",ShortBaseName);

  DoOutput(
  "\t.section .data\n"
  "\t.globl\tstub_Open\n"
  "\t.type\tstub_Open,@function\n"
  "\n"
  "stub_Open:\n"
  "\t.short\t0x4ef8\n"                /* JMP.w */
  "\t.short\t0\n"                     /* Indicate switch */
  "\t.short\t1\n"                     /* Trap type */
  "\t.globl\tstub_OpenPPC\n"
  "\t.long\tstub_OpenPPC\n"
  "\t.byte\t2\n"                      /* Register mapping */
  "\t.byte\t1,REG68K_A7\n"          
  "\t.byte\t3,REG68K_A6\n"
  "\t.section .text\n"
  "\t.align\t4\n"
  "\n"
  "stub_OpenPPC:\n"
  "\taddi\t%s12,%s1,-16\n"            /* Calculate stackframe size */
  "\trlwinm\t%s12,%s12,0,0,27\n"      /* Align it */
  "\tstw\t%s1,0(%s12)\n"              /* Store backchain pointer */
  "\tmr\t%s1,%s12\n"                  /* Set real stack pointer */
  "\tstw\t%s11,12(%s1)\n"             /* Store Enter68kQuick vector */
  "\tlhz\t%s12,LIB_POSSIZE(%s3)\n"
  "\tadd\t%s3,%s3,%s12\n"             /* by addind posSize */
  "\tlwz\t%s3,ExtLib_ILibrary(%s3)\n" /* Get the real interface pointer */
  "\tCallLib\tlmi_Open\n"
  "\tlwz\t%s11,%s12(%s1)\n"
  "\tmtlr\t%s11\n"
  "\tlwz\t%s1,0(%s1)\n"               /* Cleanup stack frame */
  "\tblrl\n"                          /* Return to emulation */
  "\n"
  "\t.globl\tstub_Open68K\n"
  "\t.long\tstub_Open68K\n"
  "\t.byte\t0\n"                      /* Flags */
  "\t.byte\t2\n"                      /* Two registers (a7 and d0) */
  "\t.byte\t1,REG68K_A7\n"            /* Map r1 to A7 */
  "\t.byte\t3,REG68K_D0\n"            /* Map r3 to D0 */
  "\t.section .data\n"
  "\t.align\t4\n"
  "\n"
  "stub_Open68K:\n"
  "\t.short\t0x4e75\n"                /* RTS */
  "\n"
  "\t.section .data\n"
  "\t.globl\tstub_Close\n"
  "\t.type\tstub_Close,@function\n"
  "\n"
  "stub_Close:\n"
  "\t.short\t0x4ef8\n"                /* JMP.w */
  "\t.short\t0\n"                     /* Indicate switch */
  "\t.short\t1\n"                     /* Trap type */
  "\t.globl\tstub_ClosePPC\n"
  "\t.long\tstub_ClosePPC\n"
  "\t.byte\t2\n"                      /* Register mapping */
  "\t.byte\t1,REG68K_A7\n"            /* map r1 to a7 */
  "\t.byte\t3,REG68K_A6\n"
  "\t.section .text\n"
  "\t.align\t4\n"
  "\n"
  "stub_ClosePPC:\n"
  "\taddi\t%s12,%s1,-16\n"            /* Calculate stackframe size */
  "\trlwinm\t%s12,%s12,0,0,27\n"      /* Align it */
  "\tstw\t%s1,0(%s12)\n"              /* Store backchain pointer */
  "\tmr\t%s1,%s12\n"                  /* Set real stack pointer */
  "\tstw\t%s11,12(%s1)\n"             /* Store Enter68kQuick vector */
  "\tlhz\t%s12,LIB_POSSIZE(%s3)\n"
  "\tadd\t%s3,%s3,%s12\n"             /* by addind posSize */
  "\tlwz\t%s3,ExtLib_ILibrary(%s3)\n" /* Get the real interface pointer */
  "\tCallLib\tlmi_Close\n"
  "\tlwz\t%s11,12(%s1)\n"
  "\tmtlr\t%s11\n"
  "\tlwz\t%s1,0(%s1)\n"               /* Cleanup stack frame */
  "\tblrl\n"                          /* Return to emulation */
  "\n"
  "\t.globl\tstub_Close68K\n"
  "\t.long\tstub_Close68K\n"
  "\t.byte\t0\n"                      /* Flags */
  "\t.byte\t1\n"                      /* One register (a7 only) */
  "\t.byte\t1,REG68K_A7\n"            /* Map r1 to A7 */
  "\t.section .data\n"
  "\t.align\t4\n"
  "\n"
  "stub_Close68K:\n"
  "\t.short\t0x4e75\n"                /* RTS */
  "\n"
  "\t.section .data\n"
  "\t.globl\tstub_Expunge\n"
  "\t.type\tstub_Expunge,@function\n"
  "\n"
  "stub_Expunge:\n"
  "\t.short\t0x7000\n"                /* moveq #0, d0 */
  "\t.short\t0x4e75\n"                /* RTS */
  "\n"
  "\t.section .data\n"
  "\t.globl\tstub_Reserved\n"
  "\t.type\tstub_Reserved,@function\n"
  "\n"
  "stub_Reserved:\n"
  "\t.short\t0x7000\n"                /* moveq #0, d0 */
  "\t.short\t0x4e75\n\n",             /* RTS */
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix,
  PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix, PPCRegPrefix);

  CallFunc(TAGMODE_NORMAL, "\n/%s */\n\n", FuncOS4M68K);
  DoOutput("\n"
  "\t.globl\tVector68K\n"
  "\t.globl\tVecTable68K\n"
  "Vector68K:\n"
  "\t.long\tVecTable68K\n"
  "VecTable68K:\n"
  "\t.long\tstub_Open\n"
  "\t.long\tstub_Close\n"
  "\t.long\tstub_Expunge\n"
  "\t.long\tstub_Reserved\n");

  LastBias = 30;
  CallFunc(TAGMODE_NORMAL, 0, FuncOS4M68KVect);
  DoOutput("\t.long\t-1\n");

  CloseDest(filename);

  if(Flags2 & FLAG2_OS4M68KCSTUB)
  {
    sprintf(filename, "%s_68k.c", ShortBaseName);
    if(!OpenDest(filename))
      exit(20);
    Flags &= ~(FLAG_DONE);
    if(Flags2 & FLAG2_AUTOHEADER) DoOutput("/* %s */\n\n", AUTOHEADERTEXT);

    if(HEADER)
    {
      DoOutput("\n");
      DoOutputDirect(HEADER, headersize);
    }

    DoOutput(
    "#ifdef __USE_INLINE__\n"
    "#undef __USE_INLINE__\n"
    "#endif\n"
    "#ifndef __NOGLOBALIFACE__\n"
    "#define __NOGLOBALIFACE__\n"
    "#endif\n\n"
    "#include <exec/interfaces.h>\n"
    "#include <exec/libraries.h>\n"
    "#include <exec/emulation.h>\n"
    "#include <interfaces/exec.h>\n"
    "#include <interfaces/%s.h>\n"
    "#include <proto/%s.h>\n\n", ShortBaseName, ShortBaseName);

    CallFunc(TAGMODE_NORMAL, "\n/%s */\n\n", FuncOS4M68KCSTUB);
  }
  return Output_Error;
}

/* ------------------------------------------------------------------ */

static uint32 GetName(struct NameList *t, struct ShortListRoot *p, uint32 args)
{
  struct NameList *p2 = (struct NameList *) p->First;
  struct AmiPragma ap;
  memset(&ap, 0, sizeof(struct AmiPragma));
  ap.FuncName = t->NormName;
  ap.NumArgs = 1;
  ap.CallArgs = 1;
  ap.Args[0].ArgName = (args ? "args" : "tags");
  if(!MakeTagFunction(&ap))
    return 0;

  if(!ap.TagName)
    return 0;

  while(p2 && (!p2->PragName || strcmp(p2->PragName, ap.TagName)))
   p2 = (struct NameList *) p2->List.Next;

  if(!p2)
    return 0;

  t->Type = (args ? NTP_ARGS : NTP_TAGS);
  t->PragName = ap.TagName;
  RemoveItem(p, (struct ShortList *) p2);

#ifdef DEBUG_OLD
  printf("GetName: name matches - %s _ %s\n", t->NormName, t->PragName);
#endif

  return 1;
}

static void OptimizeFDData(struct PragData *pd)
{
#ifdef DEBUG_OLD
  printf("OptimizeFDData\n");
#endif

  while(pd)
  {
    if(pd->NumNames > 1)
    {
      struct ShortListRoot n = {0,0,0}, p = {0,0,0};
      struct NameList *t;
      while(pd->Name.First)     /* sorts in AmiCall and TagCall */
      {
        t = (struct NameList *) pd->Name.First;

        RemoveItem(&pd->Name, (struct ShortList *) t);
        AddItem(t->PragName ? &p : &n, (struct ShortList *) t);
      }

      if(p.First)
      {
        t = (struct NameList *) n.First;
        while(p.First && t)
        {
          if(!GetName(t, &p, 0))
          {
            GetName(t, &p, 1);
          }
          if(t->PragName)
          {
            struct NameList *t2 = (struct NameList *) t->List.Next;
            RemoveItem(&n, (struct ShortList *)t);
            AddItem(&pd->Name, (struct ShortList *) t);
            t = t2;
          }
          else
            t = (struct NameList *) t->List.Next;
        }
        while(p.First)
        {
          if(n.First)
          {
            t = (struct NameList *) n.First;
            t->PragName = ((struct NameList *)(p.First))->PragName;
            RemoveItem(&n, (struct ShortList *) t);
#ifdef DEBUG_OLD
  printf("OptimizeFDData: names together - %s _ %s\n", t->NormName, t->PragName);
#endif
            t->Type = NTP_UNKNOWN;
          }
          else
          {
            uint32 i;

            t = (struct NameList *) p.First;
            i = strlen(t->PragName);
            t->NormName = DupString(t->PragName, i+1);
            t->NormName[i++] = 'A';
            t->NormName[i] = 0;
            t->Type = NTP_TAGS;
#ifdef DEBUG_OLD
  printf("OptimizeFDData: NormName created - %s _ %s\n", t->NormName, t->PragName);
#endif
          }

          AddItem(&pd->Name, (struct ShortList *) t);
          RemoveItem(&p, p.First);
        }
      }

      AddItem(&pd->Name, n.First); /* add left NormNames */
    }
    pd = (struct PragData *) pd->List.Next;
  }
}

static uint32 MakeFD(struct PragList *pl)
{
  struct PragData *pd = (struct PragData *) pl->Data.First;
  uint32 bias;

#ifdef DEBUG_OLD
  printf("MakeFD\n");
#endif
  bias = pd->Bias;

  OptimizeFDData(pd);
#ifdef DEBUG_OLD
  printf("MakeFD: after Optimizing\n");
#endif
  DoOutput("##base _%s\n##bias %ld\n##public\n", pl->Basename, bias);

  while(pd && Output_Error)
  {
    struct NameList *n = (struct NameList *) pd->Name.First;

    if(bias != pd->Bias)
      DoOutput("##bias %ld\n", (bias = pd->Bias));

    while(n)
    {
      strptr lastpar = "last";
      uint32 i;

      if(n->Type == NTP_TAGS)
        lastpar = "tags";
      else if(n->Type == NTP_ARGS)
        lastpar = "args";

      DoOutput("%s("/*)*/,n->NormName);
      if(!pd->NumArgs)
        DoOutput(/*(*/")()\n");
      else
      {
        for(i = 0; i < pd->NumArgs-1; ++i)
          DoOutput("par%ld,",i+1);
        DoOutput(/*(*/"%s)("/*)*/, lastpar);
        for(i = 0; i < pd->NumArgs-1; ++i)
          DoOutput("%s,", RegNames[pd->ArgReg[i]]);
        DoOutput(/*(*/"%s)\n", RegNames[pd->ArgReg[i]]);

        if(n->Type == NTP_UNKNOWN)
        {
          uint32 i;
          for(i = 0; n->NormName[i] == n->PragName[i]; ++i)
            ;
          DoOutput("*tagcall");
          if(n->NormName[i])
            DoOutput("-%s", n->NormName+i);
          if(n->PragName[i])
            DoOutput("+%s", n->PragName+i);

          DoOutput("\n");
        }
      }
      if((n = (struct NameList *) n->List.Next))
        DoOutput("##bias %ld\n", pd->Bias);
      Flags |= FLAG_DONE;
    }

    pd = (struct PragData *)pd->List.Next; bias += BIAS_OFFSET;
  }

  DoOutput("##end\n");

  return Output_Error;
}

static uint32 AddFDData(struct ShortListRoot *pls, struct FDData *fd)
{
  struct NameList *t;
  struct PragList *pl = (struct PragList *) pls->First;
  struct PragData *pd;

  while(pl && strcmp(pl->Basename, fd->Basename))
    pl = (struct PragList *) pl->List.Next;

  if(!pl)
  {
#ifdef DEBUG_OLD
  printf("AddFDData: New PragList - %s\n", fd->Basename);
#endif
    if(!(pl = (struct PragList *) NewItem(pls)))
      return 100;
    pl->Basename = fd->Basename;
    pl->Data.Size = sizeof(struct PragData);
    AddItem(pls, (struct ShortList *) pl);
  }

  if((pd = (struct PragData *) pl->Data.First))
  {
    while(pd->List.Next && ((struct PragData *) pd->List.Next)->Bias
    <= fd->Bias)
      pd = (struct PragData *) pd->List.Next;
  }

  if(!pd || pd->Bias != fd->Bias)
  {
    struct PragData *pd2;
#ifdef DEBUG_OLD
  printf("AddFDData: New PragData - %ld, %ld\n", fd->Bias, fd->NumArgs);
#endif
    if(!(pd2 = (struct PragData *) NewItem(&pl->Data)))
      return 100;
    pd2->Bias = fd->Bias;
    memcpy(pd2->ArgReg, fd->ArgReg, MAXREG);
    pd2->NumArgs = fd->NumArgs;
    pd2->Name.Size = sizeof(struct NameList);
    if(!pd)
      AddItem(&pl->Data, (struct ShortList *) pd2);
    else if(pd->Bias > fd->Bias) /* Insert at start */
    {
      pd2->List.Next = pl->Data.First;
      pl->Data.First = (struct ShortList *) pd2;
    }
    else /* Insert the entry */
    {
      pd2->List.Next = pd->List.Next;
      pd->List.Next = (struct ShortList *) pd2;
    }
    pd = pd2;
  }
  else
  {
    uint32 i = fd->NumArgs;
    if(fd->NumArgs != pd->NumArgs)
    {
#ifdef DEBUG_OLD
  printf("ArgNum %ld != %ld\n", fd->NumArgs, pd->NumArgs);
#endif
      return ERR_DIFFERENT_TO_PREVIOUS;
    }

    while(i--)
    {
      if(fd->ArgReg[i] != pd->ArgReg[i])
      {
#ifdef DEBUG_OLD
  printf("ArgReg %x != %x\n", fd->ArgReg[i], pd->ArgReg[i]);
#endif
        return ERR_DIFFERENT_TO_PREVIOUS;
      }
    }
  }

  t = (struct NameList *) pd->Name.First;       /* skips same names */
  while(t && (!(fd->Mode ? t->PragName : t->NormName) ||
  strcmp(fd->Name, fd->Mode ? t->PragName : t->NormName)))
    t = (struct NameList *) t->List.Next;

  if(t)
    return 0;

  if(!(t = (struct NameList *) NewItem(&pd->Name)))
    return 100;
  if(fd->Mode)
    t->PragName = fd->Name;
  else
    t->NormName = fd->Name;
  AddItem(&pd->Name, (struct ShortList *) t);
  ++(pd->NumNames);
#ifdef DEBUG_OLD
  printf("AddFDData: New NameList - %s\n", fd->Name);
#endif
  return 0;
}

static string GetHexValue(string data)
{
  if(data >= 'a')
    return (string) (data - 'a' + 10);
  else if(data >= 'A')
    return (string) (data - 'A' + 10);
  else
    return (string) (data - '0');
}

static string GetDoubleHexValue(strptr data)
{
  return (string)((GetHexValue(*data)<<4)+GetHexValue(data[1]));
}

static uint32 GetLibData(struct FDData *fd)
{
  uint32 i;
  fd->Name = SkipBlanks(in.pos);
  in.pos = SkipName(fd->Name); *(in.pos++) = 0;
  in.pos = SkipBlanks(in.pos);
  fd->Bias = strtoul(in.pos, 0, 16);
  in.pos = SkipName(SkipBlanks(SkipName(in.pos)));
  if((fd->NumArgs = GetHexValue(*(--in.pos))) > MAXREGNF - 2)
    return ERR_TO_MUCH_ARGUMENTS;
  --in.pos; /* skips return register */
  for(i = 0; i < fd->NumArgs; ++i)
  {
    if((fd->ArgReg[i] = GetHexValue(*(--in.pos))) > REG_A5)
      return ERR_EXPECTED_REGISTER_NAME;
  }
  return 0;
}

static uint32 GetFlibData(struct FDData *fd)
{
  uint32 i;
  fd->Name = SkipBlanks(in.pos);
  in.pos = SkipName(fd->Name); *(in.pos++) = 0;
  in.pos = SkipBlanks(in.pos);
  fd->Bias = strtoul(in.pos, 0, 16);
  in.pos = SkipName(SkipBlanks(SkipName(in.pos))) - 2;
  if((fd->NumArgs = GetDoubleHexValue(in.pos)) > MAXREG-2)
    return ERR_TO_MUCH_ARGUMENTS;
  in.pos -= 2; /* skips return register */
  for(i = 0; i < fd->NumArgs; ++i)
  {
    in.pos -= 2;
    if((fd->ArgReg[i] = GetDoubleHexValue(in.pos)) >= MAXREG)
      return ERR_EXPECTED_REGISTER_NAME;
    else if(fd->ArgReg[i] >= REG_FP0 && (Flags & FLAG_NOFPU))
      return ERR_FLOATARG_NOT_ALLOWED;
  }
  return 0;
}

static uint32 GetAmiData(struct FDData *fd)
{
  strptr endptr;
  in.pos = SkipBlanks(in.pos);
  if(*in.pos != '('/*)*/)
    return ERR_EXPECTED_OPEN_BRACKET;
  fd->Basename = ++in.pos;
  in.pos = SkipBlanks(endptr = SkipName(in.pos));
  if(*in.pos != ',')
    return ERR_EXPECTED_COMMA;
  *endptr = 0;
  in.pos = SkipBlanks(++in.pos);
  if(!strncmp(in.pos, "0x", 2))
    fd->Bias = strtoul(in.pos+2, 0, 16);
  else
    fd->Bias = strtoul(in.pos, 0, 10);

  in.pos = SkipBlanks(SkipName(in.pos));
  if(*in.pos != ',')
    return ERR_EXPECTED_COMMA;
  fd->Name = in.pos = SkipBlanks(++in.pos);
  in.pos = SkipBlanks(endptr = SkipName(in.pos));
  if(*in.pos != '('/*)*/)
    return ERR_EXPECTED_OPEN_BRACKET;
  *endptr = 0;
  in.pos = SkipBlanks(++in.pos);
  if(*in.pos == /*(*/')')
    return 0;
  --in.pos;
  while(*in.pos != /*(*/')')
  {
    uint32 i;
    in.pos = SkipBlanks(in.pos+1);

    for(i = 0; i < REG_FP0; i++)
      if(!strnicmp(RegNames[i], in.pos, 2))
        break;
    if(i == REG_FP0)
    {
      for(; i < MAXREG; i++)
        if(!strnicmp(RegNames[i], in.pos, 3))
          break;
    }

    if(i == MAXREG)
      return ERR_EXPECTED_REGISTER_NAME;
    else if(i >= REG_FP0 && (Flags & FLAG_NOFPU))
      return ERR_FLOATARG_NOT_ALLOWED;

    fd->ArgReg[fd->NumArgs] = i; ++fd->NumArgs;

    if(fd->NumArgs > MAXREG-2)
      return ERR_TO_MUCH_ARGUMENTS;

    in.pos = SkipBlanks(in.pos+(i >= REG_FP0 ? 3 : 2));

    if(*in.pos != ',' && *in.pos != /*(*/')')
      return ERR_EXPECTED_CLOSE_BRACKET;
  }
  in.pos = SkipBlanks(in.pos+1);
  if(*in.pos != /*(*/')')
    return ERR_EXPECTED_CLOSE_BRACKET;
  return 0;
}

static uint32 CreateFDFile(void)
{
  struct ShortListRoot pl = {0, 0, sizeof(struct PragList)};
  uint32 linenum, err = 0, skip;
  strptr ptr, p2;

  ptr = p2 = args.infile;
  while(*p2)
  {
    if(*p2 == '/' || *p2 == ':' || *p2 == '\\')
      ptr = p2+1;
    ++p2;
  }
  for(p2 = ptr; *p2 && *p2 != '_' && *p2 != '.'; ++p2)
    ;
  if(p2 != ptr)
  {
    ShortBaseName = ptr;
    *p2 = '\0';
  }

  for(linenum = 1; in.pos < in.buf + in.size; ++linenum)
  {
    in.pos = SkipBlanks(in.pos);
    if(!strncmp("#pragma", in.pos, 7))
    {
      struct FDData fd;

      skip = 0;
      memset(&fd, 0, sizeof(struct FDData));

      in.pos = SkipBlanks(in.pos+7);
      if(!strncmp("tagcall", in.pos, 7))
      {
        fd.Mode = 1;
        in.pos = SkipBlanks(in.pos+7);
        if(*in.pos == '(' /*)*/)                /* Storm method */
          err = GetAmiData(&fd);
        else                                    /* SAS method */
        {
          fd.Basename = in.pos;
          in.pos = SkipName(fd.Basename); *(in.pos++) = 0;
          err = GetLibData(&fd);
        }
      }
      else if(!strncmp("amicall", in.pos, 7))   /* Storm method */
      {
        in.pos += 7;
        err = GetAmiData(&fd);
      }
      else if(!strncmp("libcall", in.pos, 7))   /* SAS method */
      {
        fd.Basename = SkipBlanks(in.pos+7);
        in.pos = SkipName(fd.Basename); *(in.pos++) = 0;
        err = GetLibData(&fd);
      }
      else if(!strncmp("flibcall", in.pos, 8))  /* SAS method */
      {
        fd.Basename = SkipBlanks(in.pos+8);
        in.pos = SkipName(fd.Basename); *(in.pos++) = 0;
        err = GetFlibData(&fd);
      }
      else if(!strncmp("syscall", in.pos, 7))   /* SAS method */
      {
        fd.Basename = "SysBase";
        err = GetLibData(&fd);
      }
      else
        skip = 1;

      if(err)
        DoError(err, linenum);
      else if(skip)
        ;
      else if((err = AddFDData(&pl, &fd)))
      {
        if(err != 100)
          DoError(err, linenum);
        return 0;
      }
    }
    while(*(in.pos++))  /* jumps to first char of next line */
      ;
  }

  if(pl.First)
  {
    struct PragList *p = (struct PragList *) pl.First;
    if(!p->List.Next)
    {
      strptr text, to;
      uint32 i;

      if(ShortBaseName)
      {
        text = ShortBaseName; i = strlen(text);
      }
      else
      {
        text = p->Basename; i = strlen(text)-4;
      }

      to = DupString(text, i + sizeof(FDFILEEXTENSION) - 1);
      memcpy(to+i, FDFILEEXTENSION, sizeof(FDFILEEXTENSION));
      if(!OpenDest(to))
        return 0;
      
      err = MakeFD(p);
      CloseDest(to);
      if(!err)
        return 0;
    }
    else
    {
      while(p)
      {
        strptr to;
        uint32 i;
        i = strlen(p->Basename) - 4;
        to = DupString(p->Basename, i + sizeof(FDFILEEXTENSION) - 1);
        memcpy(to+i, FDFILEEXTENSION, sizeof(FDFILEEXTENSION));
        if(!OpenDest(to))
          return 0;
        i = MakeFD(p);
        CloseDest(to);
        if(!i)
          return 0;
        p = (struct PragList *) p->List.Next;
      }
    }
  }

  return 1;
}

#ifdef FD2PRAGMA_READARGS
#include <proto/dos.h>

#define PARAM   "FROM=INFILE/A,SPECIAL/N,MODE/N,"                       \
                "TO/K,ABI/K,CLIB/K,COPYRIGHT/K,HEADER/K,HUNKNAME/K,"    \
                "BASENAME/K,LIBTYPE/K,LIBNAME/K,PRIORITY/N/K,"          \
                "PREFIX/K,SUBPREFIX/K,PREMACRO/K,"                      \
                "AUTOHEADER/S,COMMENT/S,EXTERNC/S,FPUONLY/S,"           \
                "NEWSYNTAX/S,"                                          \
                "NOFPU/S,NOPPC/S,NOPPCREGNAME/S,NOSYMBOL/S,"            \
                "ONLYCNAMES/S,OPT040/S,PPCONLY/S,"                      \
                "PRIVATE/S,SECTION/S,SMALLCODE/S,SMALLDATA/S,"          \
                "SMALLTYPES/S,SORTED/S,SYSTEMRELEASE/S,USESYSCALL/S,"   \
                "VOIDBASE/S"

struct AmiArg
{
  strptr INFILE;
  uint32* SPECIAL;
  uint32* MODE;
  strptr  TO;
  strptr  ABI;
  strptr  CLIB;
  strptr  COPYRIGHT;
  strptr  HEADER;
  strptr  HUNKNAME;
  strptr  BASENAME;
  strptr  LIBTYPE;
  strptr  LIBNAME;
  uint32* PRIORITY;
  strptr  PREFIX;
  strptr  SUBPREFIX;
  strptr  PREMACRO;
  uint32  AUTOHEADER;
  uint32  COMMENT;
  uint32  EXTERNC;
  uint32  FPUONLY;
  uint32  NEWSYNTAX;
  uint32  NOFPU;
  uint32  NOPPC;
  uint32  NOPPCREGNAME;
  uint32  NOSYMBOL;
  uint32  ONLYCNAMES;
  uint32  OPT040;
  uint32  PPCONLY;
  uint32  PRIVATE;
  uint32  SECTION;
  uint32  SMALLCODE;
  uint32  SMALLDATA;
  uint32  SMALLTYPES;
  uint32  SORTED;
  uint32  SYSTEMRELEASE;
  uint32  USESYSCALL;
  uint32  VOIDBASE;
};

static const strptr helptext =
"INFILE:  the input file which should be used\n"
"SPECIAL: 1 - Aztec compiler (xxx_lib.h,     MODE 2, AMICALL)\n"
"\t 2 - DICE compiler  (xxx_pragmas.h, MODE 3, LIBCALL)\n"
"\t 3 - SAS compiler   (xxx_pragmas.h, MODE 3, LIBCALL,LIBTAGS)\n"
"\t 4 - MAXON compiler (xxx_lib.h,     MODE 1, AMICALL)\n"
"\t 5 - STORM compiler (xxx_lib.h,     MODE 1, AMITAGS,AMICALL)\n"
"\t 6 - pragma for all compilers [default]\n"
"\t 7 - all compilers with pragma to inline redirect for GCC\n"
"\t10 - stub-functions for C - C text\n"
"\t11 - stub-functions for C - assembler text\n"
"\t12 - stub-functions for C - link library\n"
"\t13 - defines and link library for local library base (register call)\n"
"\t14 - defines and link library for local library base (stack call)\n"
"\t15 - stub-functions for Pascal - assembler text\n"
"\t16 - stub-functions for Pascal - link library\n"
"\t17 - BMAP file for AmigaBASIC and MaxonBASIC\n"
"\t18 - module for AmigaE\n"
"\t20 - assembler lvo _lvo.i file\n"
"\t21 - assembler lvo _lib.i file\n"
"\t22 - assembler lvo _lvo.i file no XDEF\n"
"\t23 - assembler lvo _lib.i file no XDEF\n"
"\t24 - assembler lvo link library\n"
"\t30 - proto file with pragma/..._lib.h call\n"
"\t31 - proto file with pragma/..._pragmas.h call\n"
"\t32 - proto file with pragmas/..._lib.h call\n"
"\t33 - proto file with pragmas/..._pragmas.h call\n"
"\t34 - proto file with local/..._loc.h call\n"
"\t35 - proto file for all compilers (VBCC stubs)\n"
"\t36 - proto file for GNU-C compiler only\n"
"\t37 - proto file without lib definitions\n"
"\t38 - proto file for all compilers (VBCC inline)\n"
"\t39 - proto file with special PPC related checks\n"
"\t40 - GCC inline file (preprocessor based)\n"
"\t41 - GCC inline file (old type - inline based)\n"
"\t42 - GCC inline file (library stubs)\n"
"\t43 - GCC inline file (new style - macro)\n"
"\t44 - GCC inline file (new style - inline)\n"
"\t45 - GCC inline file (new style - inline with include lines)\n"
"\t46 - GCC inline file (preprocessor based, direct)\n"
"\t47 - GCC inline file (new style, direct)\n"
"\t48 - GCC inline file (preprocessor based, direct, StormGCC)\n"
"\t50 - GCC inline files for PowerUP (preprocessor based)\n"
"\t51 - GCC inline files for PowerUP (old type - inline based)\n"
"\t52 - GCC inline files for PowerUP (library stubs)\n"
"\t53 - SAS-C include file for PowerUP\n"
"\t54 - Proto file for PowerUP\n"
"\t60 - FPC pascal unit text\n"
"\t70 - VBCC inline files\n"
"\t71 - VBCC WOS stub-functions - assembler text\n"
"\t72 - VBCC WOS stub-functions - assembler text (libbase)\n"
"\t73 - VBCC WOS stub-functions - link library\n"
"\t74 - VBCC WOS stub-functions - link library (libbase)\n"
"\t75 - VBCC PowerUP stub-functions - assembler text\n"
"\t76 - VBCC PowerUP stub-functions - link library\n"
"\t77 - VBCC WOS inline files\n"
"\t78 - VBCC MorphOS stub-functions - link library\n"
"\t79 - VBCC old inline files\n"
"\t80 - pragma/proto redirect (xxx_pragmas.h, SAS/Dice)\n"
"\t81 - pragma/proto redirect (xxx_lib.h, Aztec/Maxon/Storm)\n"
"\t82 - pragma/proto redirect (xxx.h, GCC)\n"
"\t83 - pragma/proto redirect (xxx_protos.h, VBCC)\n"
"\t90 - stub-functions for C - assembler text (multiple files)\n"
"\t91 - VBCC PowerUP stub-functions - assembler text (multiple files)\n"
"\t92 - VBCC WOS stub-functions - assembler text (multiple files)\n"
"\t93 - VBCC MorphOS stub-functions - assembler text (multiple files)\n"
"       100 - PPC assembler lvo file\n"
"       101 - PPC assembler lvo file no XDEF\n"
"       102 - PPC assembler lvo ELF link library\n"
"       103 - PPC assembler lvo EHF link library\n"
"       104 - PPC V.4-ABI assembler file\n"
"       105 - PPC V.4-ABI assembler file no XDEF\n"
"       106 - PPC V.4-ABI assembler lvo ELF link library\n"
"       107 - PPC V.4-ABI assembler lvo EHF link library\n"
"       110 - FD file\n"
"       111 - CLIB file\n"
"       112 - SFD file\n"
"       120 - VBCC auto libopen files (C source)\n"
"       121 - VBCC auto libopen files (m68k link library)\n"
"       122 - VBCC MorphOS inline files\n"
"       123 - VBCC new MorphOS inline files\n"
"       130 - GCC inline files for MorphOS (preprocessor based)\n"
"       131 - GCC inline files for MorphOS (old type - inline based)\n"
"       132 - GCC inline files for MorphOS (library stubs)\n"
"       133 - GCC inline files for MorphOS (library stubs, direct varargs)\n"
"       134 - MorphOS gate stubs\n"
"       135 - MorphOS gate stubs (prelib)\n"
"       136 - MorphOS gate stubs (postlib)\n"
"       137 - MorphOS gate stubs (reglib, prelib)\n"
"       138 - MorphOS gate stubs (reglib, postlib)\n"
"       140 - OS4 XML file\n"
"       141 - OS4 PPC->M68K cross-call stubs\n"
"       142 - OS4 M68K->PPC cross-call stubs\n"
"       200 - FD file (source is a pragma file!)\n"
"MODE:          SPECIAL 1-7:\n"
"                 1: _INCLUDE_PRAGMA_..._LIB_H definition method [default]\n"
"                 2: _PRAGMAS_..._LIB_H definition method\n"
"                 3: _PRAGMAS_..._PRAGMAS_H definition method\n"
"                 4: no definition\n"
"               SPECIAL 11-14,40-45,50-53,70-76,78,90-91,111-112,122,\n"
"                       130-138,141:\n"
"                 1: all functions, normal interface\n"
"                 2: only tag-functions, tagcall interface\n"
"                 3: all functions, normal and tagcall interface [default]\n"
"TO:            the destination directory (self creation of filename)\n"
"ABI:           set ABI type (m68k|ppc|ppc0|ppc2)\n"
"CLIB:          name of the prototypes file in clib directory\n"
"COPYRIGHT:     the copyright text for CLIB files\n"
"HEADER:        inserts given file into header of created file (\"\" is scan)\n"
"HUNKNAME:      use this name for HUNK_NAME instead of default 'text'\n"
"BASENAME:      name of library base without '_'\n"
"LIBNAME:       name of the library (.e.g. dos.library)\n"
"LIBTYPE:       type of base library structure\n"
"PRIORITY:      priority for auto open files\n"
"PREFIX:        MorphOS gate prefix\n"
"SUBPREFIX:     MorphOS gate sub prefix\n"
"PREMACRO:      MorphOS gate file start macro\n"
"Switches:\n"
"AUTOHEADER     add the typical automatic generated header\n"
"COMMENT:       copy comments found in input file\n"
"EXTERNC:       add a #ifdef __cplusplus ... statement to pragma file\n"
"FPUONLY:       work only with functions using FPU register arguments\n"
"NEWSYNTAX:     uses new Motorola syntax for asm files\n"
"NOFPU:         disable usage of FPU register arguments\n"
"NOPPC:         disable usage of PPC-ABI functions\n"
"NOPPCREGNAME:  do not add 'r' to PPC register names\n"
"NOSYMBOL:      prevents creation of SYMBOL hunks for link libraries\n"
"ONLYCNAMES:    do not create C++ or ASM names\n"
"OPT040:        optimize for 68040, do not use MOVEM for stubs\n"
"PPCONLY:       only use PPC-ABI functions\n"
"PRIVATE:       includes private declared functions\n"
"SECTION:       add section statements to asm texts\n"
"SMALLCODE:     generate small code link libraries or assembler text\n"
"SMALLDATA:     generate small data link libraries or assembler text\n"
"SMALLTYPES:    allow 8 and 16 bit types in registers\n"
"SORTED:        sort generated files by name and not by bias value\n"
"SYSTEMRELEASE: special handling of comments for system includes\n"
"USESYSCALL:    uses syscall pragma instead of libcall SysBase\n"
"VOIDBASE:      library bases are of type void *\n";

/* print the help text */
static void printhelp(void)
{
  printf("%s\n%s\n\n%s", version+6, PARAM, helptext);
  exit(20);
}

/* initializes the arguments and starts argument parsing */
static void GetArgs(int argc, char **argv)
{
  struct RDArgs *rda;
  struct AmiArg amiargs;
  int res = 0;

  if((rda = (struct RDArgs *) AllocDosObject(DOS_RDARGS, 0)))
  {
    rda->RDA_ExtHelp = helptext;
    memset(&amiargs, 0, sizeof(struct AmiArg));
    if(ReadArgs(PARAM, (int32 *) &amiargs, rda))
    {
      int l;
      strptr d, s;

      l = strlen(amiargs.TO ? amiargs.TO : "") + 1
        + strlen(amiargs.CLIB ? amiargs.CLIB : "") + 1
        + strlen(amiargs.HEADER ? amiargs.HEADER :  "") + 1
        + strlen(amiargs.ABI ? amiargs.ABI : "") + 1
        + strlen(amiargs.HUNKNAME ? amiargs.HUNKNAME : "") + 1
        + strlen(amiargs.BASENAME ? amiargs.BASENAME : "") + 1
        + strlen(amiargs.LIBTYPE ? amiargs.LIBTYPE : "") + 1
        + strlen(amiargs.LIBNAME ? amiargs.LIBNAME : "") + 1
        + strlen(amiargs.COPYRIGHT ? amiargs.COPYRIGHT : "") + 1
        + strlen(amiargs.PREFIX ? amiargs.PREFIX : "") + 1
        + strlen(amiargs.SUBPREFIX ? amiargs.SUBPREFIX : "") + 1
        + strlen(amiargs.PREMACRO ? amiargs.PREMACRO : "") + 1
        + strlen(amiargs.INFILE) + 1;
      if((d = AllocListMem(l)))
      {
        res = 1;

        s = amiargs.INFILE;
        args.infile = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        if((s = amiargs.TO))
        {
          args.to = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.HEADER))
        {
          args.header = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.CLIB))
        {
          args.clib = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.HUNKNAME))
        {
          hunkname = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.BASENAME))
        {
          Flags |= FLAG_BASENAME;
          BaseName = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.LIBTYPE))
        {
          Flags2 |= FLAG2_LIBTYPE;
          libtype = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.LIBNAME))
        {
          Flags2 |= FLAG2_LIBNAME;
          libname = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.PREFIX))
        {
          prefix = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.SUBPREFIX))
        {
          subprefix = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.PREMACRO))
        {
          premacro = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.COPYRIGHT))
        {
          Copyright = d; while(*s) *(d++) = *(s++); *(d++) = 0;
        }
        if((s = amiargs.ABI))
        {
          defabi = d; while(*s) *(d++) = *(s++); *d = 0;
        }
        if(amiargs.EXTERNC)      Flags ^= FLAG_EXTERNC;
        if(amiargs.PRIVATE)      Flags ^= FLAG_PRIVATE;
        if(amiargs.NEWSYNTAX)    Flags ^= FLAG_NEWSYNTAX;
        if(amiargs.SMALLDATA)    Flags ^= FLAG_SMALLDATA;
        if(amiargs.SMALLCODE)    Flags2 ^= FLAG2_SMALLCODE;
        if(amiargs.SMALLTYPES)   Flags2 ^= FLAG2_SMALLTYPES;
        if(amiargs.USESYSCALL)   Flags ^= FLAG_SYSCALL;
        if(amiargs.OPT040)       Flags ^= FLAG_NOMOVEM;
        if(amiargs.NOFPU)        Flags ^= FLAG_NOFPU;
        if(amiargs.FPUONLY)      Flags ^= FLAG_FPUONLY;
        if(amiargs.NOPPC)        Flags ^= FLAG_NOPPC;
        if(amiargs.NOSYMBOL)     Flags ^= FLAG_NOSYMBOL;
        if(amiargs.PPCONLY)      Flags ^= FLAG_PPCONLY;
        if(amiargs.SECTION)      Flags ^= FLAG_ASMSECTION;
        if(amiargs.COMMENT)      Flags ^= FLAG_DOCOMMENT;
        if(amiargs.SORTED)       Flags ^= FLAG_SORTED;
        if(amiargs.ONLYCNAMES)   Flags ^= FLAG_ONLYCNAMES;
        if(amiargs.SYSTEMRELEASE) Flags2 ^= FLAG2_SYSTEMRELEASE;
        if(amiargs.VOIDBASE)     Flags2 ^= FLAG2_VOIDBASE;
        if(amiargs.NOPPCREGNAME) PPCRegPrefix = "";
        if(amiargs.AUTOHEADER)   Flags2 ^= FLAG2_AUTOHEADER;
        if(amiargs.SPECIAL)
          args.special = *amiargs.SPECIAL;
        if(amiargs.MODE)
          args.mode = *amiargs.MODE;
        if(amiargs.PRIORITY)
          priority = *amiargs.PRIORITY;
      }
      FreeArgs(rda);
    }
    else
      PrintFault(IoErr(), 0);
    FreeDosObject(DOS_RDARGS, rda);
  }

  if(!res)
/*    printhelp(); */
    exit(20);
}

#else
static const strptr helptext =
" -h,--help\n"
" -i,--infile   <input filename>\n"
" -s,--special  <number>\n"
" -m,--mode     <number>\n"
" -t,--to       <destination directory>\n"
" -a,--abi      <m68k|ppc|ppc0|ppc2>\n"
" -c,--clib     <clib prototypes filename>\n"
" -d,--header   <header file or \"\">\n"
" -i,--libname  <name of library>\n"
" -n,--hunkname <name of HUNK_NAME, default is 'text'>\n"
" -b,--basename <name of library base without '_'>\n"
" -l,--libtype  <name of base library type>\n"
" -p,--priority <priority for auto open files>\n"
" -r,--copyright<copyright text>\n"
"    --prefix   <MorphOS gate prefix>\n"
"    --subprefix<MorphOS gate sub prefix>\n"
"    --premacro <MorphOS gate file start macro>\n"
"\n"
"Switches:\n"
"--autoheader    add the typical automatic generated header\n"
"--comment       copy comments found in input file\n"
"--externc       add a #ifdef __cplusplus ... statement to pragma file\n"
"--fpuonly       work only with functions using FPU register arguments\n"
"--newsyntax     uses new Motorola syntax for asm files\n"
"--nofpu         disable usage of FPU register arguments\n"
"--noppc         disable usage of PPC-ABI functions\n"
"--noppcregname  do not add 'r' to PPC register names\n"
"--nosymbol      prevents creation of SYMBOL hunks for link libraries\n"
"--onlycnames    do not create C++ or ASM names\n"
"--opt040        optimize for 68040, do not use MOVEM for stubs\n"
"--ppconly       only use PPC-ABI functions\n"
"--private       includes private declared functions\n"
"--section       add section statements to asm texts\n"
"--smallcode     generate small code link libraries or assembler text\n"
"--smalldata     generate small data link libraries or assembler text\n"
"--smalltypes    allow 8 and 16 bit types in registers\n"
"--sorted        sort generated files by name and not by bias value\n"
"--systemrelease special handling of comments for system includes\n"
"--usesyscall    uses syscall pragma instead of libcall SysBase\n"
"--voidbase      library bases are of type void *\n"
"\n"
"special: 1 - Aztec compiler (xxx_lib.h,     MODE 2, AMICALL)\n"
"         2 - DICE compiler  (xxx_pragmas.h, MODE 3, LIBCALL)\n"
"         3 - SAS compiler   (xxx_pragmas.h, MODE 3, LIBCALL,LIBTAGS)\n"
"         4 - MAXON compiler (xxx_lib.h,     MODE 1, AMICALL)\n"
"         5 - STORM compiler (xxx_lib.h,     MODE 1, AMITAGS,AMICALL)\n"
"         6 - pragma for all compilers [default]\n"
"         7 - all compilers with pragma to inline redirect for GCC\n"
"        10 - stub-functions for C - C text\n"
"        11 - stub-functions for C - assembler text\n"
"        12 - stub-functions for C - link library\n"
"        13 - defines and link library for local library base (register call)\n"
"        14 - defines and link library for local library base (stack call)\n"
"        15 - stub-functions for Pascal - assembler text\n"
"        16 - stub-functions for Pascal - link library\n"
"        17 - BMAP file for AmigaBASIC and MaxonBASIC\n"
"        18 - module for AmigaE\n"
"        20 - assembler lvo _lvo.i file\n"
"        21 - assembler lvo _lib.i file\n"
"        22 - assembler lvo _lvo.i file no XDEF\n"
"        23 - assembler lvo _lib.i file no XDEF\n"
"        24 - assembler lvo link library\n"
"        30 - proto file with pragma/..._lib.h call\n"
"        31 - proto file with pragma/..._pragmas.h call\n"
"        32 - proto file with pragmas/..._lib.h call\n"
"        33 - proto file with pragmas/..._pragmas.h call\n"
"        34 - proto file with local/..._loc.h call\n"
"        35 - proto file for all compilers (VBCC stubs)\n"
"        36 - proto file for GNU-C compiler only\n"
"        37 - proto file without lib definitions\n"
"        38 - proto file for all compilers (VBCC inline)\n"
"        39 - proto file with special PPC related checks\n"
"        40 - GCC inline file (preprocessor based)\n"
"        41 - GCC inline file (old type - inline based)\n"
"        42 - GCC inline file (library stubs)\n"
"        43 - GCC inline file (new style - macro)\n"
"        44 - GCC inline file (new style - inline)\n"
"        45 - GCC inline file (new style - inline with include lines)\n"
"        46 - GCC inline file (preprocessor based, direct)\n"
"        47 - GCC inline file (new style, direct)\n"
"        48 - GCC inline file (preprocessor based, direct, StormGCC)\n"
"        50 - GCC inline files for PowerUP (preprocessor based)\n"
"        51 - GCC inline files for PowerUP (old type - inline based)\n"
"        52 - GCC inline files for PowerUP (library stubs)\n"
"        53 - SAS-C include file for PowerUP\n"
"        54 - Proto file for PowerUP\n"
"        60 - FPC pascal unit text\n"
"        70 - VBCC inline files\n"
"        71 - VBCC WOS stub-functions - assembler text\n"
"        72 - VBCC WOS stub-functions - assembler text (libbase)\n"
"        73 - VBCC WOS stub-functions - link library\n"
"        74 - VBCC WOS stub-functions - link library (libbase)\n"
"        75 - VBCC PowerUP stub-functions - assembler text\n"
"        76 - VBCC PowerUP stub-functions - link library\n"
"        77 - VBCC WOS inline files\n"
"        78 - VBCC MorphOS stub-functions - link library\n"
"        79 - VBCC old inline files\n"
"        80 - pragma/proto redirect (xxx_pragmas.h, SAS/Dice)\n"
"        81 - pragma/proto redirect (xxx_lib.h, Aztec/Maxon/Storm)\n"
"        82 - pragma/proto redirect (xxx.h, GCC)\n"
"        83 - pragma/proto redirect (xxx_protos.h, VBCC)\n"
"        90 - stub-functions for C - assembler text (multiple files)\n"
"        91 - VBCC PowerUP stub-functions - assembler text (multiple files)\n"
"        92 - VBCC WOS stub-functions - assembler text (multiple files)\n"
"        93 - VBCC MorphOS stub-functions - assembler text (multiple files)\n"
"       100 - PPC assembler lvo file\n"
"       101 - PPC assembler lvo file no XDEF\n"
"       102 - PPC assembler lvo ELF link library\n"
"       103 - PPC assembler lvo EHF link library\n"
"       104 - PPC V.4-ABI assembler file\n"
"       105 - PPC V.4-ABI assembler file no XDEF\n"
"       106 - PPC V.4-ABI assembler lvo ELF link library\n"
"       107 - PPC V.4-ABI assembler lvo EHF link library\n"
"       110 - FD file\n"
"       111 - CLIB file\n"
"       112 - SFD file\n"
"       120 - VBCC auto libopen files (C source)\n"
"       121 - VBCC auto libopen files (m68k link library)\n"
"       122 - VBCC MorphOS inline files\n"
"       123 - VBCC new MorphOS inline files\n"
"       130 - GCC inline files for MorphOS (preprocessor based)\n"
"       131 - GCC inline files for MorphOS (old type - inline based)\n"
"       132 - GCC inline files for MorphOS (library stubs)\n"
"       133 - GCC inline files for MorphOS (library stubs, direct varargs)\n"
"       134 - MorphOS gate stubs\n"
"       135 - MorphOS gate stubs (prelib)\n"
"       136 - MorphOS gate stubs (postlib)\n"
"       137 - MorphOS gate stubs (reglib, prelib)\n"
"       138 - MorphOS gate stubs (reglib, postlib)\n"
"       140 - OS4 XML file\n"
"       141 - OS4 PPC->M68K cross-call stubs\n"
"       142 - OS4 M68K->PPC cross-call stubs\n"
"       200 - FD file (source is a pragma file!)\n"
"mode:    special 1-7\n"
"         1 - _INCLUDE_PRAGMA_..._LIB_H definition method [default]\n"
"         2 - _PRAGMAS_..._LIB_H definition method\n"
"         3 - _PRAGMAS_..._PRAGMAS_H definition method\n"
"         4 - no definition\n"
"         special 11-14,40-45,50-53,70-76,78,90-93,111-112,122,\n"
"                 130-138,141:\n"
"         1 - all functions, normal interface\n"
"         2 - only tag-functions, tagcall interface\n"
"         3 - all functions, normal and tagcall interface [default]\n";

/* print the help text */
static void printhelp(void)
{
  printf("%s\n%s", version+6, helptext);
  exit(20);
}

struct ArgData
{
  strptr ArgName;
  uint8  ArgChar;
  uint8  ArgNameLen;
  uint8  ArgNum;
};

enum ArgNums {
ARG_HELP, ARG_INFILE, ARG_SPECIAL, ARG_MODE, ARG_TO, ARG_CLIB, ARG_ABI, ARG_COPYRIGHT,
ARG_HEADER, ARG_HUNKNAME, ARG_BASENAME, ARG_LIBTYPE,
ARG_COMMENT, ARG_EXTERNC, ARG_FPUONLY, ARG_NEWSYNTAX, ARG_NOFPU, ARG_NOPPC,
ARG_NOSYMBOL, ARG_ONLYCNAMES, ARG_OPT040, ARG_PPCONLY, ARG_PRIVATE, ARG_SECTION,
ARG_SMALLDATA, ARG_SORTED, ARG_USESYSCALL, ARG_NOPPCREGNAME,
ARG_SYSTEMRELEASE, ARG_PRIORITY, ARG_LIBNAME, ARG_SMALLCODE, ARG_VOIDBASE,
ARG_PREFIX, ARG_SUBPREFIX, ARG_PREMACRO, ARG_SMALLTYPES, ARG_AUTOHEADER
};

/* argument definition array */
static const struct ArgData argtexts[] = {
{"help",        'h',  4, ARG_HELP},
{"infile",      'i',  6, ARG_INFILE},
{"special",     's',  7, ARG_SPECIAL},
{"mode",        'm',  4, ARG_MODE},
{"to",          't',  2, ARG_TO},
{"clib",        'c',  4, ARG_CLIB},
{"abi",         'a',  3, ARG_ABI},
{"copyright",   'r',  9, ARG_COPYRIGHT},
{"header",      'd',  6, ARG_HEADER},
{"hunkname",    'n',  8, ARG_HUNKNAME},
{"basename",    'b',  8, ARG_BASENAME},
{"libtype",     'l',  7, ARG_LIBTYPE},
{"libname",     'i',  7, ARG_LIBNAME},
{"priority",    'p',  8, ARG_PRIORITY},
{"autoheader",    0, 10, ARG_AUTOHEADER},
{"comment",       0,  7, ARG_COMMENT},
{"externc",       0,  7, ARG_EXTERNC},
{"fpuonly",       0,  7, ARG_FPUONLY},
{"newsyntax",     0,  9, ARG_NEWSYNTAX},
{"nofpu",         0,  5, ARG_NOFPU},
{"noppc",         0,  5, ARG_NOPPC},
{"noppcregname",  0, 12, ARG_NOPPCREGNAME},
{"nosymbol",      0,  8, ARG_NOSYMBOL},
{"onlycnames",    0, 10, ARG_ONLYCNAMES},
{"opt040",        0,  6, ARG_OPT040},
{"ppconly",       0,  7, ARG_PPCONLY},
{"private",       0,  7, ARG_PRIVATE},
{"section",       0,  7, ARG_SECTION},
{"smalldata",     0,  9, ARG_SMALLDATA},
{"smalltypes",    0, 10, ARG_SMALLTYPES},
{"smallcode",     0,  9, ARG_SMALLCODE},
{"sorted",        0,  6, ARG_SORTED},
{"systemrelease", 0, 13, ARG_SYSTEMRELEASE},
{"usesyscall",    0, 10, ARG_USESYSCALL},
{"voidbase",      0,  8, ARG_VOIDBASE},
{"prefix",        0,  6, ARG_PREFIX},
{"subprefix",     0,  9, ARG_SUBPREFIX},
{"premacro",      0,  8, ARG_PREMACRO},
{0,0,0,0}, /* end marker */
};

/* parse on argument entry, returns number of used entries, 0 for error, -1 for error without error printout */
static uint32 ParseArgEntry(uint32 argc, strptr *argv)
{
  uint32 numentries = 1, l;
  strptr a, b;
  const struct ArgData *ad;

  if((*argv)[0] != '-' || !(*argv)[1])
    return 0;

  ad = argtexts;
  while(ad->ArgName)
  {
    if((*argv)[1] == ad->ArgChar || ((*argv)[1] == '-' && !strncmp(ad->ArgName, (*argv)+2, ad->ArgNameLen)))
      break;
    ++ad;
  }
  if(!ad->ArgName)
    return 0;
  switch(ad->ArgNum)
  {
  case ARG_HELP: printhelp(); break;
  case ARG_EXTERNC:        Flags ^= FLAG_EXTERNC; break;
  case ARG_PRIVATE:        Flags ^= FLAG_PRIVATE; break;
  case ARG_NEWSYNTAX:      Flags ^= FLAG_NEWSYNTAX; break;
  case ARG_SMALLDATA:      Flags ^= FLAG_SMALLDATA; break;
  case ARG_SMALLCODE:      Flags2 ^= FLAG2_SMALLCODE; break;
  case ARG_SMALLTYPES:     Flags2 ^= FLAG2_SMALLTYPES; break;
  case ARG_USESYSCALL:     Flags ^= FLAG_SYSCALL; break;
  case ARG_OPT040:         Flags ^= FLAG_NOMOVEM; break;
  case ARG_NOFPU:          Flags ^= FLAG_NOFPU; break;
  case ARG_FPUONLY:        Flags ^= FLAG_FPUONLY; break;
  case ARG_NOPPC:          Flags ^= FLAG_NOPPC; break;
  case ARG_NOSYMBOL:       Flags ^= FLAG_NOSYMBOL; break;
  case ARG_PPCONLY:        Flags ^= FLAG_PPCONLY; break;
  case ARG_SECTION:        Flags ^= FLAG_ASMSECTION; break;
  case ARG_COMMENT:        Flags ^= FLAG_DOCOMMENT; break;
  case ARG_SORTED:         Flags ^= FLAG_SORTED; break;
  case ARG_ONLYCNAMES:     Flags ^= FLAG_ONLYCNAMES; break;
  case ARG_SYSTEMRELEASE:  Flags2 ^= FLAG2_SYSTEMRELEASE; break;
  case ARG_VOIDBASE:       Flags2 ^= FLAG2_VOIDBASE; break;
  case ARG_AUTOHEADER:     Flags2 ^= FLAG2_AUTOHEADER; break;
  case ARG_NOPPCREGNAME:   PPCRegPrefix = "";
  default:
    a = *argv+((*argv)[1] == '-' ? ad->ArgNameLen+2 : 2);
    if(!(*a))
    {
      if(argc > 1) { a = argv[1]; numentries = 2; }
      else { a = 0; numentries = 0;}
    }
    else if(*a == '=')
      ++a;
    if(a)
    {
      if(*a == '\"')
      {
        l = strlen(++a);
        if(a[l-1] == '\"')
          a[--l] = 0; /* remove second " */
      }
      switch(ad->ArgNum)
      {
      case ARG_INFILE: args.infile = a; break;
      case ARG_COPYRIGHT: Copyright = a; break;
      case ARG_TO: args.to = a; break;
      case ARG_ABI: defabi = a; break;
      case ARG_CLIB: args.clib = a; break;
      case ARG_HEADER: args.header = a; break;
      case ARG_HUNKNAME: hunkname = a; break;
      case ARG_PREFIX: prefix = a; break;
      case ARG_SUBPREFIX: subprefix = a; break;
      case ARG_PREMACRO: premacro = a; break;
      case ARG_LIBTYPE: libtype = a; Flags2 |= FLAG2_LIBTYPE; break;
      case ARG_LIBNAME: libname = a; Flags2 |= FLAG2_LIBNAME; break;
      case ARG_BASENAME: BaseName = a; Flags |= FLAG_BASENAME; break;
      case ARG_SPECIAL:
        args.special = strtoul(a, &b, 10);
        if(*b)
          numentries = 0;
        break;
      case ARG_PRIORITY:
        priority = strtoul(a, &b, 10);
        if(*b)
          numentries = 0;
        break;
      case ARG_MODE:
        args.mode = strtoul(a, &b, 10);
        if(*b || args.mode < 1 || args.mode > 3)
          numentries = 0;
        break;
      }
    }
  }
  return numentries;
}

/* initializes the arguments and starts argument parsing */
static void GetArgs(int argc, char **argv)
{
  int res = 1;
  int i = 1, j;

  while(i < argc && res)
  {
    if((j = ParseArgEntry(argc-i, argv+i)) < 1)
      res = 0;
    else
      i += j;
  }
  if(!res || !args.infile)
    printhelp();
}

#endif

static strptr mygetfile(strptr name, size_t *len)
{
  strptr ptr = 0;
  FILE *infile;

  if((infile = fopen(name, "rb")))
  {
    if(!fseek(infile, 0, SEEK_END))
    {
      *len = ftell(infile);
      if(!fseek(infile, 0, SEEK_SET))
      {
        if((ptr = AllocListMem(*len+1)))
        {
          ptr[*len] = 0;
#ifdef DEBUG_OLD
  printf("mygetfile: '%s' size %d\n", name, *len);
#endif
          if(fread(ptr, *len, 1, infile) != 1)
            ptr = 0;
        }
      }
    }
    fclose(infile);
  }
  return ptr;
}

int main(int argc, char **argv)
{
  uint32 mode = 0, pragmode = PRAGMODE_PRAGLIB, callmode = TAGMODE_BOTH;
  strptr amicall = 0, libcall = 0, amitags = 0, libtags = 0;
  strptr clibbuf;
  size_t clibsize = 0;

  GetArgs(argc, argv);

  if((tempbuf = (uint8 *) AllocListMem(TEMPSIZE)))
  {
    if(!(in.pos = in.buf = mygetfile(args.infile, &in.size)))
    {
      if(args.special == 200)
      {
        DoError(ERR_OPEN_FILE, 0, args.infile);
        exit(20);
      }
      else
      {
        sprintf((strptr)tempbuf, "%s" SFDFILEEXTENSION, args.infile);
        if(!(in.pos = in.buf = mygetfile((strptr)tempbuf, &in.size)))
        {
          sprintf((strptr)tempbuf, "%s" FDFILEEXTENSION, args.infile);
          if(!(in.pos = in.buf = mygetfile((strptr)tempbuf, &in.size)))
          {
            DoError(ERR_OPEN_FILE, 0, args.infile);
            exit(20);
          }
          else
            args.infile = DupString((strptr) tempbuf, strlen((strptr) tempbuf));
        }
        else
          args.infile = DupString((strptr) tempbuf, strlen((strptr) tempbuf));
      }
    }
    printf("SourceFile: %s\n", args.infile);

    MakeLines(in.pos, in.size);

    if((Flags & FLAG_DOCOMMENT) && (Flags & FLAG_SORTED)) /* is not possible to use both */
    {
      DoError(ERR_SORTED_COMMENT, 0);
      Flags &= (~FLAG_SORTED);
    }

    if(args.special == 200)
    {
      CreateFDFile();
      exit(0);
    }

    if(!GetTypes())
      exit(20);

    if(!ScanFDFile())
      exit(20);

    if(args.clib)
    {
      if(Flags2 & FLAG2_SFDMODE)
        DoError(ERR_SFD_AND_CLIB, 0);
      else
      {
        sprintf((strptr)tempbuf, "%s_protos.h", args.clib);
        if(!(clibbuf = mygetfile(args.clib, &clibsize)) && !(clibbuf = mygetfile((strptr)tempbuf, &clibsize)))
        {
          DoError(ERR_OPEN_FILE, 0, args.clib);
          exit(20);
        }
        ScanClibFile(clibbuf, clibbuf+clibsize);
      }
    }

    if(!MakeShortBaseName())
    {
      DoError(ERR_MISSING_SHORTBASENAME, 0);
      exit(20);
    }

    /* WARN when requesting obsolete types! */
    switch(args.special)
    {
    case 1: case 2: case 3: case 4: case 5: case 7:
      printf("You use obsolete data type %ld, better use type 6!\n", args.special);
      break;
    case 11: case 15: case 71: case 72: case 75:
      printf("You use obsolete assembler text type %ld, better use 90 to 99 or "
      "link libraries!\n", args.special);
      break;
    case 30: case 31: case 32: case 33: case 34: case 36: case 37: case 39:
      printf("You use obsolete proto type %ld, better us type 38 or 35!\n", args.special);
      break;
    case 79:
      printf("Obsolete inline file 79 used, better take type 70 instead!\n");
      break;
    }

    if(args.special < 10) /* the pragma area is up to 9 */
    {
      mode = MODUS_PRAGMA;
      sprintf(filename, "%s_lib.h", ShortBaseName);

      switch(args.special)
      {
        case 0: break;
        case 1: pragmode = PRAGMODE_PRAGSLIB; amicall = ""; break;
        case 2: sprintf(filename, "%s_pragmas.h", ShortBaseName);
                pragmode = PRAGMODE_PRAGSPRAGS; libcall = ""; break;
        case 3: sprintf(filename, "%s_pragmas.h", ShortBaseName);
                pragmode = PRAGMODE_PRAGSPRAGS; libcall = "";
                libtags = "def " TEXT_SAS_60; break;
        case 4: amicall = ""; break;
        case 5: amicall = amitags = ""; break;
        case 7: Flags |= FLAG_GNUPRAG; /* no break ! */
        case 6: amicall = " defined(" TEXT_AZTEC ") || defined("
                TEXT_MAXON ") || defined(" TEXT_STORM ")";
                libcall = " defined(" TEXT_DICE ") || defined(" TEXT_SAS ")";
                libtags = "def " TEXT_SAS_60; amitags ="def " TEXT_STORM; break;
        default: mode = MODUS_ERROR; break;
      }

      if(args.mode > 0 && args.mode < 5)
        pragmode = args.mode;
    }
    else if(args.special < 20) /* the misc area is up to 19 */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;
      switch(args.special)
      {
      case 10: mode = MODUS_CSTUB;
        sprintf(filename, "%s_cstub.h", ShortBaseName); break;
      case 11: mode = MODUS_STUBTEXT;
        sprintf(filename, "%s_stub.s", ShortBaseName); break;
      case 12: mode = MODUS_STUBCODE;
        sprintf(filename, "%s.lib", ShortBaseName); break;
      case 13: Flags |= FLAG_LOCALREG; /* no break ! */
      case 14: mode = MODUS_LOCALDATA;
        sprintf(filename, "%s_loc.h", ShortBaseName); break;
      case 15: mode = MODUS_STUBTEXT; callmode = TAGMODE_NORMAL;
        Flags ^= FLAG_PASCAL;
        sprintf(filename, "%s_stub.s", ShortBaseName); break;
      case 16: mode = MODUS_STUBCODE; callmode = TAGMODE_NORMAL;
        Flags ^= FLAG_PASCAL;
        sprintf(filename, "%s.lib", ShortBaseName); break;
      case 17: mode = MODUS_BMAP; callmode = TAGMODE_NORMAL;
        sprintf(filename, "%s.bmap", ShortBaseName); break;
      case 18: mode = MODUS_EMODULE;
        sprintf(filename, "%s.m", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 30) /* the lvo area is up to 29 */
    {
      switch(args.special)
      {
      case 20: case 22: mode = MODUS_LVO+args.special-20;
        sprintf(filename, "%s_lvo.i", ShortBaseName); break;
      case 21: case 23: mode = MODUS_LVO+args.special-20;
        sprintf(filename, "%s_lib.i", ShortBaseName); break;
      case 24: mode = MODUS_LVOLIB;
        sprintf(filename, "%slvo.o", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 40) /* the proto area is up to 39 */
    {
      if(args.special < 40)
      {
        mode = MODUS_PROTO+args.special-30;
        sprintf(filename, "%s.h", ShortBaseName);
      }
      else
        mode = MODUS_ERROR;
    }
    else if(args.special < 50) /* the inline area is up to 49 */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 40: case 41: case 42: case 43: case 44: case 45: case 46:
      case 47:
        mode = MODUS_INLINE+args.special-40;
        sprintf(filename, "%s.h", ShortBaseName); break;
      case 48:
        Flags |= FLAG_STORMGCC;
        /* the same mode as for 46, but additional flag */
        mode = MODUS_INLINE+args.special-40-2;
        sprintf(filename, "%s.h", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 60) /* the PowerUP area is up to 59 */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 50: case 51: case 52: mode = MODUS_INLINE+args.special-50;
        sprintf(filename, "%s.h", ShortBaseName); Flags |= FLAG_POWERUP;
        break;
      case 53:
        sprintf(filename, "%s_pragmas.h", ShortBaseName);
        mode = MODUS_SASPOWER; break;
      case 54:
        sprintf(filename, "%s.h", ShortBaseName);
        mode = MODUS_PROTOPOWER; break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 70) /* the PASCAL stuff */
    {
      if(args.special == 60)
      {
        mode = MODUS_PASCAL;
        sprintf(filename, "%s.pas", ShortBaseName);
      }
      else
        mode = MODUS_ERROR;
    }
    else if(args.special < 80) /* the VBCC stuff */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 70: mode = MODUS_VBCCINLINE;
        sprintf(filename, "%s_protos.h", ShortBaseName); break;
      case 71: case 72: case 75:
        mode = MODUS_VBCC+args.special-71;
        sprintf(filename, "%s_stub.s", ShortBaseName); break;
      case 73: case 74:
        mode = MODUS_VBCC+args.special-71;
        sprintf(filename, "%s.lib", ShortBaseName); break;
      case 76:
        mode = MODUS_VBCCPUPLIB;
        sprintf(filename, "lib%s.a", ShortBaseName); break;
      case 77: mode = MODUS_VBCCWOSINLINE;
        sprintf(filename, "%s_protos.h", ShortBaseName); break;
      case 78: mode = MODUS_VBCCMORPHCODE;
        sprintf(filename, "lib%s.a", ShortBaseName); break;
      case 79: mode = MODUS_VBCCINLINE;
        Flags2 |= FLAG2_OLDVBCC;
        callmode = TAGMODE_NORMAL;
        sprintf(filename, "%s_protos.h", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 90) /* redirect stuff */
    {
      mode = MODUS_REDIRECT;
      switch(args.special)
      {
      case 80: sprintf(filename, "%s_pragmas.h", ShortBaseName); break;
      case 81: sprintf(filename, "%s_lib.h", ShortBaseName); break;
      case 82: sprintf(filename, "%s.h", ShortBaseName); break;
      case 83: sprintf(filename, "%s_protos.h", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 100) /* multifile stuff */
    {
      Flags |= FLAG_SINGLEFILE;
      switch(args.special)
      {
      case 90:
        if(args.mode > 0 && args.mode < 4) callmode = args.mode - 1;
        mode = MODUS_ASMTEXTSF; filenamefmt = "%s.s";
        break;
      case 91:
        if(args.mode > 0 && args.mode < 4) callmode = args.mode - 1;
        mode = MODUS_VBCCPUPTEXTSF; filenamefmt = "%s.s";
        break;
      case 92:
        if(args.mode > 0 && args.mode < 4) callmode = args.mode - 1;
        mode = MODUS_VBCCWOSTEXTSF; filenamefmt = "%s.s";
        break;
      case 93:
        if(args.mode > 0 && args.mode < 4) callmode = args.mode - 1;
        mode = MODUS_VBCCMORPHTEXTSF; filenamefmt = "%s.s";
        break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 110) /* PPC lvo's */
    {
      switch(args.special)
      {
      case 100: case 101: mode = MODUS_LVOPPC+args.special-100;
        sprintf(filename, "%s_lib.i", ShortBaseName);
        break;
      case 104: case 105: mode = MODUS_LVOPPC+args.special-104;
        Flags |= FLAG_ABIV4;
        sprintf(filename, "%s_lib.i", ShortBaseName);
        break;
      case 103: mode = MODUS_LVOLIB;
        sprintf(filename, "%slvo.o", ShortBaseName);
        break;
      case 107: mode = MODUS_LVOLIB;
        Flags |= FLAG_ABIV4;
        sprintf(filename, "%slvo.o", ShortBaseName);
        break;
      case 102: mode = MODUS_LVOLIBPPC;
        sprintf(filename, "%slvo.o", ShortBaseName); break;
      case 106: mode = MODUS_LVOLIBPPC;
        Flags |= FLAG_ABIV4;
        sprintf(filename, "%slvo.o", ShortBaseName); break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 120) /* different files */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 110: mode = MODUS_FD;
        sprintf(filename, "%s_lib.fd", ShortBaseName);
        if(Flags & FLAG_SORTED) /* is not possible to use here */
        {
          DoError(ERR_SORTED_SFD_FD, 0);
          Flags &= (~FLAG_SORTED);
        }
        break;
      case 111: mode = MODUS_CLIB; Flags2 |= FLAG2_CLIBOUT;
        sprintf(filename, "%s_protos.h", ShortBaseName);
        break;
      case 112: mode = MODUS_SFD; Flags2 |= FLAG2_SFDOUT;
        sprintf(filename, "%s_lib.sfd", ShortBaseName);
        if(callmode == 1)
        {
          callmode = 2;
          DoError(ERR_ONLYTAGMODE_NOTALLOWED, 0);
        }
        
        if(Flags & FLAG_SORTED) /* is not possible to use here */
        {
          DoError(ERR_SORTED_SFD_FD, 0);
          Flags &= (~FLAG_SORTED);
        }
        break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 130) /* auto libopen files */
    {
      if(args.mode > 0 && args.mode < 4) /* for 122 */
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 120: mode = MODUS_GENAUTO;
        sprintf(filename, "%s_autoopenlib.c", ShortBaseName);
        break;
      case 121: mode = MODUS_GENAUTO+(args.special-120);
        sprintf(filename, "%s_autoopenlib.lib", ShortBaseName);
        break;
      case 123: Flags2 |= FLAG2_SHORTPPCVBCCINLINE; /* no break */
      case 122: mode = MODUS_VBCCMORPHINLINE;
        PPCRegPrefix = ""; /* no "r" allowed */
        sprintf(filename, "%s_protos.h", ShortBaseName);
        break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 140) /* the MorphOS area is up to 139 */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 130: case 131: case 132: mode = MODUS_INLINE+args.special-130;
        sprintf(filename, "%s.h", ShortBaseName); Flags |= FLAG_MORPHOS;
        break;
      case 133: mode = MODUS_INLINE+2;
        sprintf(filename, "%s.h", ShortBaseName); Flags |= FLAG_MORPHOS;
        Flags2 |= FLAG2_DIRECTVARARGS;
        break;
      case 134: mode = MODUS_GATESTUBS;
        sprintf(filename, "%s_gates.h", ShortBaseName);
        break;
      case 135: mode = MODUS_GATESTUBS; Flags2 |= FLAG2_PRELIB;
        sprintf(filename, "%s_gates.h", ShortBaseName);
        break;
      case 136: mode = MODUS_GATESTUBS; Flags2 |= FLAG2_POSTLIB;
        sprintf(filename, "%s_gates.h", ShortBaseName);
        break;
      case 137: mode = MODUS_GATESTUBS; Flags2 |= FLAG2_PRELIB|FLAG2_REGLIB;
        sprintf(filename, "%s_gates.h", ShortBaseName);
        break;
      case 138: mode = MODUS_GATESTUBS; Flags2 |= FLAG2_POSTLIB|FLAG2_REGLIB;
        sprintf(filename, "%s_gates.h", ShortBaseName);
        break;
      default: mode = MODUS_ERROR; break;
      }
    }
    else if(args.special < 150) /* the OS4 area is up to 139 */
    {
      if(args.mode > 0 && args.mode < 4)
        callmode = args.mode - 1;

      switch(args.special)
      {
      case 140:
        mode = MODUS_XML;
        sprintf(filename, "%s.xml", ShortBaseName);
        break;
      case 141: /* OS4 PPC->M68K cross-call stubs */
        mode = MODUS_OS4_PPCSTUBS;
        sprintf(filename, "%s.c", ShortBaseName);
        break;
      case 142: /* OS4 M68K->PPC cross-call stubs */
        mode = MODUS_OS4_68KSTUBS;
        sprintf(filename, "%s_68k.s", ShortBaseName);
        break;
      default: mode = MODUS_ERROR; break;
      }
    }
    if(Flags & FLAG_SORTED)
      SortFDList();

    if((Flags & FLAG_DOCOMMENT) && (Flags & FLAG_SINGLEFILE)) /* is not possible to use both */
    {
      DoError(ERR_COMMENT_SINGLEFILE, 0);
      Flags &= (~FLAG_DOCOMMENT);
    }

    if(!mode || mode == MODUS_ERROR)
      printhelp();

    /* These modes need BaseName always. */
    if(!BaseName && (mode == MODUS_PRAGMA || mode == MODUS_STUBTEXT ||
    mode == MODUS_STUBCODE || mode == MODUS_EMODULE || (mode >= MODUS_GENAUTO &&
    mode <= MODUS_GENAUTO+9)))
    {
      DoError(ERR_MISSING_BASENAME, 0);
      exit(20);
    }

    if(args.header && args.header[0] && (args.header[0] != '@' || args.header[1]))
    {
      HEADER = mygetfile(args.header, &headersize);
      args.header = 0;
    }

    if(!(Flags & FLAG_SINGLEFILE))
    {
      if(!OpenDest(filename))
        exit(20);
    }

    /* from here mode is used as return result */
    if(mode >= MODUS_GENAUTO)
      mode = CreateGenAuto(filename, mode-MODUS_GENAUTO);
    else if(mode >= MODUS_LVOPPC)
      mode = CreateLVOFilePPC(mode-MODUS_LVOPPC);
    else if(mode >= MODUS_VBCC)
      mode = CreateVBCC(mode-MODUS_VBCC, callmode);
    else if(mode >= MODUS_INLINE)
      mode = CreateInline(mode-MODUS_INLINE, callmode);
    else if(mode >= MODUS_PROTO)
      mode = CreateProtoFile(mode-MODUS_PROTO+1);
    else if(mode >= MODUS_LVO)
      mode = CreateLVOFile(mode-MODUS_LVO+1);
    else if(mode == MODUS_VBCCMORPHINLINE)
      mode = CreateVBCCInline(2, callmode);
    else if(mode == MODUS_XML)
      mode = CreateXML();
    else if(mode == MODUS_OS4_PPCSTUBS)
      mode = CreateOS4PPC(callmode);
    else if(mode == MODUS_OS4_68KSTUBS)
      mode = CreateOS4M68K();
    else if(mode == MODUS_GATESTUBS)
      mode = CreateGateStubs(callmode);
    else if(mode == MODUS_SFD)
      mode = CreateSFD(callmode);
    else if(mode == MODUS_CLIB)
      mode = CreateClib(callmode);
    else if(mode == MODUS_FD)
      mode = CreateFD();
    else if(mode == MODUS_LVOLIBPPC)
      mode = CreateLVOLibPPC();
    else if(mode == MODUS_VBCCMORPHCODE)
      mode = CreateVBCCMorphCode(callmode);
    else if(mode == MODUS_VBCCMORPHTEXTSF) /* single files */
      mode = CallFunc(callmode, "\n%s", FuncVBCCMorphText);
    else if(mode == MODUS_VBCCWOSINLINE)
      mode = CreateVBCCInline(1, callmode);
    else if(mode == MODUS_VBCCWOSTEXTSF) /* single files */
      mode = CallFunc(callmode, "\n%s", FuncVBCCWOSText);
    else if(mode == MODUS_VBCCPUPTEXTSF) /* single files */
      mode = CallFunc(callmode, "\n%s", FuncVBCCPUPText);
    else if(mode == MODUS_ASMTEXTSF) /* single files */
      mode = CallFunc(callmode, "\n%s", FuncAsmText);
    else if(mode == MODUS_REDIRECT)
      mode = CreateProtoRedirect();
    else if(mode == MODUS_EMODULE)
      mode = CreateEModule(Flags & FLAG_SORTED);
    else if(mode == MODUS_LVOLIB)
      mode = CreateLVOLib();
    else if(mode == MODUS_VBCCPUPLIB)
      mode = CreateVBCCPUPLib(callmode);
    else if(mode == MODUS_VBCCINLINE)
      mode = CreateVBCCInline(0, callmode);
    else if(mode == MODUS_PASCAL)
      mode = CreateFPCUnit();
    else if(mode == MODUS_BMAP)
      mode = CreateBMAP();
    else if(mode == MODUS_PROTOPOWER)
      mode = CreateProtoPowerUP();
    else if(mode == MODUS_SASPOWER)
      mode = CreateSASPowerUP(callmode);
    else if(mode == MODUS_CSTUB)
      mode = CreateCSTUBSFile();
    else if(mode == MODUS_PRAGMA)
      mode = CreatePragmaFile(amicall, libcall, amitags, libtags, pragmode);
    else if(mode == MODUS_LOCALDATA)
      mode = CreateLocalData(filename, callmode);
    else if(mode)             /* MODUS_STUBTEXT starts with 1 */
      mode = CreateAsmStubs(mode, callmode);

    CloseDest(filename);

    if(!mode)
    {
      DoError(Output_Error ? ERR_UNKNOWN_ERROR : ERR_WRITING_FILE, 0);
      exit(20);
    }
    free(tempbuf);
  }

  return 0;
}

