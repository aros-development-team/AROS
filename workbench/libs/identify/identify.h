/*
**      $VER: identify.i 13.0 (11.09.2001)
**
**      identify.library definitions
**
**      (C) Copyright 1996-2000 Richard Koerber
**      All Rights Reserved.
*/

#ifndef LIBRARIES_IDENTIFY_H
#define LIBRARIES_IDENTIFY_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#define _IDTAGS (0xCD450000ul)

/* ------------------------------------------------------------------------ *
**  Generic library informations
*/
#define IDENTIFYVERSION (13)

#define IDENTIFYBUFLEN  (50)            /*  default buffer length */

/* ------------------------------------------------------------------------ *
**  Expansion() tags
*/
#define IDTAG_ConfigDev   (_IDTAGS+0x00)  /* (struct ConfigDev *) ConfigDev */
                                          /*  structure to be evaluated */
#define IDTAG_ManufID     (_IDTAGS+0x01)  /* UWORD manufacturer ID if no */
                                          /*  ConfigDev is available */
#define IDTAG_ProdID      (_IDTAGS+0x02)  /* UBYTE product ID if no */
                                          /*  ConfigDev is available */
#define IDTAG_StrLength   (_IDTAGS+0x03)  /* UWORD of maximum buffer length, */
                                          /*  including termination. Default */
                                          /*  is 50. */
#define IDTAG_ManufStr    (_IDTAGS+0x04)  /* STRPTR of manufacturer name */
                                          /*  puffer, or NULL */
#define IDTAG_ProdStr     (_IDTAGS+0x05)  /* STRPTR of product name */
                                          /*  puffer, or NULL */
#define IDTAG_ClassStr    (_IDTAGS+0x06)  /* STRPTR of product class */
                                          /*  puffer, or NULL */
#define IDTAG_DeadStr     (_IDTAGS+0x07)  /* STRPTR deadend or recoverable alert? */
#define IDTAG_SubsysStr   (_IDTAGS+0x08)  /* STRPTR alert subsystem */
#define IDTAG_GeneralStr  (_IDTAGS+0x09)  /* STRPTR alert general cause */
#define IDTAG_SpecStr     (_IDTAGS+0x0A)  /* STRPTR alert specific cause */
#define IDTAG_FuncNameStr (_IDTAGS+0x0B)  /* STRPTR function name */
#define IDTAG_Expansion   (_IDTAGS+0x0C)  /* (struct ConfigDev **) for a */
                                          /*  complete expansion check. Init */
                                          /*  the variable with NULL. */
#define IDTAG_Secondary   (_IDTAGS+0x0D)  /* BOOL warn for secondary expansion */
                                          /*  boards (defaults to FALSE) */
#define IDTAG_ClassID     (_IDTAGS+0x0E)  /* ULONG * class ID of the provided */
                                          /*  expansion board (see below) [V8] */
#define IDTAG_Localize    (_IDTAGS+0x0F)  /* BOOL return localized strings */
                                          /*  (defaults to TRUE) [V8] */
#define IDTAG_NULL4NA     (_IDTAGS+0x10)  /* BOOL return NULL for not available */
                                          /*  else return a string (defaults to */
                                          /*  FALSE) [V8] */

/* ------------------------------------------------------------------------ *
**  Hardware description types
*/
#define IDHW_SYSTEM       (0)     /*  System (Amiga,DraCo,...)            [V2] */
#define IDHW_CPU          (1)     /*  CPU (68000,68010,...,68060) */
#define IDHW_FPU          (2)     /*  FPU (---,68881,68882,68040,68060) */
#define IDHW_MMU          (3)     /*  MMU (---,68852,68030,68040,68060) */
#define IDHW_OSVER        (4)     /*  OS Version (V*.*) */
#define IDHW_EXECVER      (5)     /*  Exec Version (V*.*) */
#define IDHW_WBVER        (6)     /*  Workbench Version (---,V*.*) */
#define IDHW_ROMSIZE      (7)     /*  OS ROM Size (*KB, *MB) */
#define IDHW_CHIPSET      (8)     /*  Chipset (OCS,ECS,AGA,DraCo) */
#define IDHW_GFXSYS       (9)     /*  Graphic OS (AmigaOS, CyberGraphX, ...) */
#define IDHW_CHIPRAM      (10)    /*  All Chip RAM (*KB, *MB, *GB) */
#define IDHW_FASTRAM      (11)    /*  All Fast RAM (*KB, *MB, *GB) */
#define IDHW_RAM          (12)    /*  All Total RAM (*KB, *MB, *GB) */
#define IDHW_SETPATCHVER  (13)    /*  SetPatch Version (---,V*.*)         [V4] */
#define IDHW_AUDIOSYS     (14)    /*  Audio OS (AmigaOS, AHI, ...)        [V5] */
#define IDHW_OSNR         (15)    /*  AmigaOS (2.04, 3.1, ...) */
#define IDHW_VMMCHIPRAM   (16)    /*  VMM Chip RAM (*KB, *MB, *GB) */
#define IDHW_VMMFASTRAM   (17)    /*  VMM Fast RAM (*KB, *MB, *GB) */
#define IDHW_VMMRAM       (18)    /*  VMM Total RAM (*KB, *MB, *GB) */
#define IDHW_PLNCHIPRAM   (19)    /*  Plain Chip RAM (*KB, *MB, *GB) */
#define IDHW_PLNFASTRAM   (20)    /*  Plain Fast RAM (*KB, *MB, *GB) */
#define IDHW_PLNRAM       (21)    /*  Plain Total RAM (*KB, *MB, *GB) */
#define IDHW_VBR          (22)    /*  Vector Base Register                [V6] */
#define IDHW_LASTALERT    (23)    /*  Last Alert code */
#define IDHW_VBLANKFREQ   (24)    /*  VBlank Frequency */
#define IDHW_POWERFREQ    (25)    /*  Power Frequency */
#define IDHW_ECLOCK       (26)    /*  EClock */
#define IDHW_SLOWRAM      (27)    /*  Plain Slow RAM (*KB, *MB, *GB) */
#define IDHW_GARY         (28)    /*  Gary (---,Normal,...) */
#define IDHW_RAMSEY       (29)    /*  RAMSEY (---,D,F) */
#define IDHW_BATTCLOCK    (30)    /*  Battery Backed Up Clock (---,Found) */
#define IDHW_CHUNKYPLANAR (31)    /*  Chunky To Planar Hardware (---,Found) [V7] */
#define IDHW_POWERPC      (32)    /*  PowerPC present? (---,Found) */
#define IDHW_PPCCLOCK     (33)    /*  PowerPC clock (unit MHz) */
#define IDHW_CPUREV       (34)    /*  CPU revision                          [V8] */
#define IDHW_CPUCLOCK     (35)    /*  CPU clock (unit MHz) */
#define IDHW_FPUCLOCK     (36)    /*  FPU clock (unit MHz) */
#define IDHW_RAMACCESS    (37)    /*  Main board RAM access time (unit ns) */
#define IDHW_RAMWIDTH     (38)    /*  Main board RAM width (bit) */
#define IDHW_RAMCAS       (39)    /*  Main board RAM CAS mode */
#define IDHW_RAMBANDWIDTH (40)    /*  Main board RAM bandwidth */
#define IDHW_TCPIP        (41)    /*  TCP/IP stack                          [V9] */
#define IDHW_PPCOS        (42)    /*  PowerPC OS */
#define IDHW_AGNUS        (43)    /*  Agnus chip revision */
#define IDHW_AGNUSMODE    (44)    /*  Agnus chip mode */
#define IDHW_DENISE       (45)    /*  Denise chip version                   [V10] */
#define IDHW_DENISEREV    (46)    /*  Denise chip revision */
#define IDHW_BOINGBAG     (47)    /*  BoingBag number                       [V12] */
#define IDHW_EMULATED     (48)    /*  Emulated Amiga                        [V13] */
#define IDHW_XLVERSION    (49)    /*  AmigaXL version */
#define IDHW_HOSTOS       (50)    /*  when emulated: Host OS (see autodocs!) */
#define IDHW_HOSTVERS     (51)    /*  when emulated: Host Version (see autodocs!) */
#define IDHW_HOSTMACHINE  (52)    /*  when emulated: Host Machine (see autodocs!) */
#define IDHW_HOSTCPU      (53)    /*  when emulated: Host CPU (see autodocs!) */
#define IDHW_HOSTSPEED    (54)    /*  when emulated: Host CPU speed (unit MHz, see autodocs!) */
#define IDHW_LASTALERTTASK (55)   /*  Last Alert task */
#define IDHW_NUMBEROF     (56)    /*  Number of types, PRIVATE! */

/* ------------------------------------------------------------------------ *
**  IDHW_SYSTEM numerical result codes
*/
#define IDSYS_AMIGA1000   (0)     /*  Amiga 1000 */
#define IDSYS_AMIGAOCS    (1)     /*  OCS Amiga 500/2000 */
#define IDSYS_AMIGAECS    (2)     /*  ECS Amiga 500/2000 */
#define IDSYS_AMIGA500    (3)     /*  Amiga 500 */
#define IDSYS_AMIGA2000   (4)     /*  Amiga 2000 */
#define IDSYS_AMIGA3000   (5)     /*  Amiga 3000 */
#define IDSYS_CDTV        (6)     /*  CDTV */
#define IDSYS_AMIGA600    (7)     /*  Amiga 600 */
#define IDSYS_CD32        (8)     /*  CD32 */
#define IDSYS_AMIGA1200   (9)     /*  Amiga 1200 */
#define IDSYS_AMIGA4000   (10)    /*  Amiga 4000 */
#define IDSYS_DRACO       (11)    /*  DraCo */
#define IDSYS_UAE         (12)    /*  Ubiquitous Amiga Emulator */
#define IDSYS_AMIGA4000T  (13)    /*  Amiga 4000 Tower */
#define IDSYS_AMIGAXL     (14)    /*  AmigaXL */

/* AROS extensions */
#define IDSYS_AROS        (100)   /*  AROS */

/* ------------------------------------------------------------------------ *
**  IDHW_CPU numerical result codes
*/
#define IDCPU_68000     (0)       /*  68000 */
#define IDCPU_68010     (1)       /*  68010 */
#define IDCPU_68020     (2)       /*  68020 */
#define IDCPU_68030     (3)       /*  68030 */
#define IDCPU_68EC030   (4)       /*  68EC030 (without MMU) */
#define IDCPU_68040     (5)       /*  68040 */
#define IDCPU_68LC040   (6)       /*  68LC040 (without FPU) */
#define IDCPU_68060     (7)       /*  68060 */
#define IDCPU_68LC060   (8)       /*  68LC060 (without FPU) */

/* AROS extensions */
#define IDCPU_unknown   (100)     /*  unknown */
#define IDCPU_i386      (101)     /*  i386 */
#define IDCPU_x86_64    (102)     /*  x86_64 */
#define IDCPU_ppc       (103)     /*  ppc */
#define IDCPU_arm       (104)     /*  arm */

/* ------------------------------------------------------------------------ *
**  IDHW_FPU numerical result codes
*/
#define IDFPU_NONE      (0)       /*  no FPU */
#define IDFPU_68881     (1)       /*  68881 */
#define IDFPU_68882     (2)       /*  68882 */
#define IDFPU_68040     (3)       /*  68040 */
#define IDFPU_68060     (4)       /*  68060 */

/* ------------------------------------------------------------------------ *
**  IDHW_MMU numerical result codes
*/
#define IDMMU_NONE      (0)       /*  no MMU */
#define IDMMU_68851     (1)       /*  68851 */
#define IDMMU_68030     (2)       /*  68030 */
#define IDMMU_68040     (3)       /*  68040 */
#define IDMMU_68060     (4)       /*  68060 */

/* ------------------------------------------------------------------------ *
**  IDHW_OSNR numerical result codes
*/
#define IDOS_UNKNOWN    (0)       /*  <V36 or >V40 */
#define IDOS_2_0        (1)       /*  OS2.0  (V36) */
#define IDOS_2_04       (2)       /*  OS2.04 (V37) */
#define IDOS_2_1        (3)       /*  OS2.1  (V38) */
#define IDOS_3_0        (4)       /*  OS3.0  (V39) */
#define IDOS_3_1        (5)       /*  OS3.1  (V40) */
#define IDOS_3_5        (6)       /*  OS3.5  (V44) */
#define IDOS_3_9        (7)       /*  OS3.9  (V45) */

/* ------------------------------------------------------------------------ *
**  IDHW_CHIPSET numerical result codes
*/
#define IDCS_OCS        (0)       /*  OCS */
#define IDCS_ECS        (1)       /*  ECS */
#define IDCS_AGA        (2)       /*  AGA */
#define IDCS_ALTAIS     (3)       /*  DraCo Altais */
#define IDCS_NECS       (4)       /* Nearly ECS (no HR-Agnus or HR-Denise) */

/* ------------------------------------------------------------------------ *
**  IDHW_GFXSYS numerical result codes
*/
#define IDGOS_AMIGAOS   (0)       /*  Plain AmigaOS */
#define IDGOS_EGS       (1)       /*  EGS */
#define IDGOS_RETINA    (2)       /*  Retina */
#define IDGOS_GRAFFITI  (3)       /*  Graffiti */
#define IDGOS_TIGA      (4)       /*  TIGA */
#define IDGOS_PROBENCH  (5)       /*  Merlin ProBench */
#define IDGOS_PICASSO   (6)       /*  Picasso */
#define IDGOS_CGX       (7)       /*  CyberGraphX */
#define IDGOS_CGX3      (8)       /*  CyberGraphX 3D */
#define IDGOS_PICASSO96 (9)       /*  Picasso96 */
#define IDGOS_CGX4      (10)      /*  CyberGraphX V4 */

/* ------------------------------------------------------------------------ *
**  IDHW_AUDIOSYS numerical result codes
*/
#define IDAOS_AMIGAOS    (0)      /*  Plain AmigaOS */
#define IDAOS_MAESTIX    (1)      /*  MaestroPro driver */
#define IDAOS_TOCCATA    (2)      /*  Toccata driver */
#define IDAOS_PRELUDE    (3)      /*  Prelude driver */
#define IDAOS_AHI        (4)      /*  AHI */
#define IDAOS_MACROAUDIO (5)      /*  MacroAudio */

/* ------------------------------------------------------------------------ *
**  IDHW_GARY numerical result codes
*/
#define IDGRY_NONE      (0)       /*  No Gary available */
#define IDGRY_NORMAL    (1)       /*  Normal Gary */
#define IDGRY_ENHANCED  (2)       /*  Enhanced Gary */

/* ------------------------------------------------------------------------ *
**  IDHW_RAMSEY numerical result codes
*/
#define IDRSY_NONE      (0)       /*  No RamSey available */
#define IDRSY_REVD      (1)       /*  RamSey Revision D */
#define IDRSY_REVF      (2)       /*  RamSey Revision F */

/* ------------------------------------------------------------------------ *
** IDHW_POWERPC numerical result codes
*/
#define IDPPC_NONE      (0)       /*  No PowerPC implemented */
#define IDPPC_OTHER     (1)       /*  Another PowerPC */
#define IDPPC_602       (2)       /*  602 */
#define IDPPC_603       (3)       /*  603 */
#define IDPPC_603E      (4)       /*  603e */
#define IDPPC_603P      (5)       /*  603p */
#define IDPPC_604       (6)       /*  604 */
#define IDPPC_604E      (7)       /*  604e */
#define IDPPC_620       (8)       /*  620 */

/* ------------------------------------------------------------------------ *
** IDHW_RAMCAS numerical result codes
*/
#define IDCAS_NONE      (0)       /*  Not available */
#define IDCAS_NORMAL    (1)       /*  Normal access */
#define IDCAS_DOUBLE    (2)       /*  Double access */

/* ------------------------------------------------------------------------ *
** IDHW_TCPIP numerical result codes
*/
#define IDTCP_NONE      (0)       /* Not available */
#define IDTCP_AMITCP    (1)       /* AmiTCP */
#define IDTCP_MIAMI     (2)       /* Miami */
#define IDTCP_TERMITE   (3)       /* TermiteTCP */
#define IDTCP_GENESIS   (4)       /* GENESiS */
#define IDTCP_MIAMIDX   (5)       /* MiamiDx */

/* ------------------------------------------------------------------------ *
** IDHW_PPCOS numerical result codes
*/
#define IDPOS_NONE      (0)       /* None */
#define IDPOS_POWERUP   (1)       /* PowerUP (Phase 5) */
#define IDPOS_WARPOS    (2)       /* WarpOS (Haage&Partner) */

/* ------------------------------------------------------------------------ *
** IDHW_AGNUS numerical result codes
*/
#define IDAG_NONE       (0)       /* None */
#define IDAG_UNKNOWN    (1)       /* Unknown Agnus */
#define IDAG_8361       (2)       /* Agnus 8361 */
#define IDAG_8367       (3)       /* Agnus 8367 */
#define IDAG_8370       (4)       /* Agnus 8370 */
#define IDAG_8371       (5)       /* Agnus 8371 */
#define IDAG_8372_R4    (6)       /* Agnus 8372 Rev. 1-4 */
#define IDAG_8372_R5    (7)       /* Agnus 8372 Rev. 5 */
#define IDAG_8374_R2    (8)       /* Alice 8374 Rev. 1-2 */
#define IDAG_8374_R3    (9)       /* Alice 8374 Rev. 3-4 */

/* ------------------------------------------------------------------------ *
** IDHW_DENISE numerical result codes
*/
#define IDDN_NONE       (0)       /* None */
#define IDDN_UNKNOWN    (1)       /* Unknown Denise (new model?! :-) */
#define IDDN_8362       (2)       /* Denise 8362 */
#define IDDN_8373       (3)       /* Denise 8373 */
#define IDDN_8364       (4)       /* Lisa 8364 */

/* ------------------------------------------------------------------------ *
** IDHW_AGNUSMODE numerical result codes
*/
#define IDAM_NONE       (0)       /* None */
#define IDAM_NTSC       (1)       /* NTSC Agnus */
#define IDAM_PAL        (2)       /* PAL Agnus */

/* ------------------------------------------------------------------------ *
** IDTAG_ClassID numerical result codes
*/
#define IDCID_UNKNOWN   (0)     /*  unknown functionality */
#define IDCID_MISC      (1)     /*  miscellaneous expansion */
#define IDCID_TURBO     (2)     /*  accelerator */
#define IDCID_TURBORAM  (3)     /*  accelerator RAM */
#define IDCID_TURBOHD   (4)     /*  accelerator + some HD controller */
#define IDCID_TURBOIDE  (5)     /*  accelerator + AT-IDE controller */
#define IDCID_TURBOSCSIHD (6)   /*  accelerator + SCSI host adapter */
#define IDCID_NET       (7)     /*  some network */
#define IDCID_ARCNET    (8)     /*  ArcNet */
#define IDCID_ETHERNET  (9)     /*  Ethernet */
#define IDCID_AUDIO     (10)    /*  audio */
#define IDCID_VIDEO     (11)    /*  video (video digitizer etc) */
#define IDCID_MULTIMEDIA (12)   /*  multimedia (audio, video, etc) */
#define IDCID_DSP       (13)    /*  multi purpose DSP */
#define IDCID_BUSBRIDGE (14)    /*  ISA bus bridge */
#define IDCID_BRIDGE    (15)    /*  PC bridge */
#define IDCID_EMULATOR  (16)    /*  some Emulator hardware */
#define IDCID_MACEMU    (17)    /*  Macintosh emulator hardware */
#define IDCID_ATARIEMU  (18)    /*  Atari emulator hardware */
#define IDCID_GFX       (19)    /*  graphics (register) */
#define IDCID_GFXRAM    (20)    /*  graphics (video RAM) */
#define IDCID_HD        (21)    /*  some HD controller */
#define IDCID_HDRAM     (22)    /*  some HD controller + RAM */
#define IDCID_IDEHD     (23)    /*  IDE controller */
#define IDCID_IDEHDRAM  (24)    /*  IDE controller + RAM */
#define IDCID_SCSIHD    (25)    /*  SCSI host adapter */
#define IDCID_RAMSCSIHD (26)    /*  SCSI host adapter + RAM */
#define IDCID_CDROM     (27)    /*  CD-ROM interface */
#define IDCID_MODEM     (28)    /*  internal modem */
#define IDCID_ISDN      (29)    /*  internal ISDN terminal adapter */
#define IDCID_MULTIIO   (30)    /*  multi I/O (serial + parallel) */
#define IDCID_SERIAL    (31)    /*  multi serial (no parallel) */
#define IDCID_PARALLEL  (32)    /*  multi parallel (no serial) */
#define IDCID_SCANIF    (33)    /*  scanner interface */
#define IDCID_TABLET    (34)    /*  drawing tablet interface */
#define IDCID_RAM       (35)    /*  plain RAM expansion */
#define IDCID_FLASHROM  (36)    /*  Flash ROM */
#define IDCID_SCSIIDE   (37)    /*  combined SCSI/IDE controller */
#define IDCID_RAMFPU    (38)    /*  RAM expansion + FPU */
#define IDCID_GUESS     (39)    /*  name was unknown but guessed */
#define IDCID_KICKSTART (40)    /*  KickStart */
#define IDCID_RAM32     (41)    /*  32bit RAM expansion */
#define IDCID_TURBOANDRAM (42)  /*  accelerator + RAM expansion */
#define IDCID_ACCESS    (43)    /*  access control */
#define IDCID_INTERFACE (44)    /*  some interface */
#define IDCID_MFMHD     (45)    /*  MFM HD controller */
#define IDCID_FLOPPY    (46)    /*  floppy disk controller */
    /*  Be prepared to get other numbers as well. In this case you */
    /*  should assume IDCID_MISC. */

/* ------------------------------------------------------------------------ *
**  Error codes
*/
/* Positive error codes are DOS errors! */
#define IDERR_OKAY      (0)     /* No error */
#define IDERR_NOLENGTH  (-1)    /* Buffer length is 0 ?? */
#define IDERR_BADID     (-2)    /* Missing or bad board ID */
#define IDERR_NOMEM     (-3)    /* Not enough memory */
#define IDERR_NOFD      (-4)    /* No fitting FD file found */
#define IDERR_OFFSET    (-5)    /* Function offset not found */
#define IDERR_DONE      (-6)    /* Done with traversing (not an error) */
#define IDERR_SECONDARY (-7)    /* Secondary expansion board (not an error) */

/* ------------------------------------------------------------------------ *
**  That's all...
*/

#endif

