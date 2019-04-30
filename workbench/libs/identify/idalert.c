/*
 *  Copyright (c) 2010-2011 Matthias Rustler
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included
 *  in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 *   
 *  $Id$
 */

#include <exec/alerts.h>

#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <string.h>

#include "identify_intern.h"
#include "identify.h"
#include "locale.h"

const struct
{
    ULONG id;
    CONST_STRPTR entry;
} subsystem[] =
{
    {0x00000000, "CPU"               },
    {0x01000000, "exec.library"      },
    {0x02000000, "graphics.library"  },
    {0x03000000, "layers.library"    },
    {0x04000000, "intuition.library" },
    {0x05000000, "math#?.library"    },
    {0x07000000, "dos.library"       },
    {0x08000000, "ramlib"            },
    {0x09000000, "icon.library"      },
    {0x0A000000, "expansion.library" },
    {0x0B000000, "diskfont.library"  },
    {0x10000000, "audio.device"      },
    {0x11000000, "console.device"    },
    {0x12000000, "gameport.device"   },
    {0x13000000, "keyboard.device"   },
    {0x14000000, "trackdisk.device"  },
    {0x15000000, "timer.device"      },
    {0x20000000, "cia?.resource"     },
    {0x21000000, "disk.resource"     },
    {0x22000000, "misc.resource"     },
    {0x30000000, "bootstrap"         },
    {0x31000000, "Workbench"         },
    {0x32000000, "DiskCopy"          },
    {0x33000000, "gadtools.library"  },
    {0x34000000, "utility.library"   },
    {-1,         NULL                }
};

const struct
{
    ULONG id;
    ULONG entry;
} general[] =
{
    {0x00000000,    MSG_AG_GENERAL    }, // $00000000
    {AG_NoMemory,   MSG_AG_NOMEMORY   }, // $00010000
    {AG_MakeLib,    MSG_AG_MAKELIB    }, // $00020000
    {AG_OpenLib,    MSG_AG_OPENLIB    }, // $00030000
    {AG_OpenDev,    MSG_AG_OPENDEV    }, // $00040000
    {AG_OpenRes,    MSG_AG_OPENRES    }, // $00050000
    {AG_IOError,    MSG_AG_IOERROR    }, // $00060000
    {AG_NoSignal,   MSG_AG_NOSIGNAL   }, // $00070000
    {AG_BadParm,    MSG_AG_BADPARM    }, // $00080000
    {AG_CloseLib,   MSG_AG_CLOSELIB   }, // $00090000 Usually too many closes
    {AG_CloseDev,   MSG_AG_CLOSEDEV   }, // $000A0000 or a mismatched close
    {AG_ProcCreate, MSG_AG_PROCCREATE }, // $000B0000 Process creation failed
    {-1,            0                 }
};

const struct
{
    ULONG id;
    ULONG entry;
} alerts[] =
{
    {0x00000000,      MSG_AN_NOALERT          }, // $00000000 kein Alert
    {ACPU_BusErr,     MSG_ACPU_BUSERR         }, // $80000002 Hardware bus fault/access error
    {ACPU_AddressErr, MSG_ACPU_ADDRESSERR     }, // $80000003 Illegal address access (ie: odd)
    {ACPU_InstErr,    MSG_ACPU_INSTERR        }, // $80000004 Illegal instruction
    {ACPU_DivZero,    MSG_ACPU_DIVZERO        }, // $80000005 Divide by zero
    {ACPU_CHK,        MSG_ACPU_CHK            }, // $80000006 Check instruction error
    {ACPU_TRAPV,      MSG_ACPU_TRAPV          }, // $80000007 TrapV instruction error
    {ACPU_PrivErr,    MSG_ACPU_PRIVERR        }, // $80000008 Privilege violation error
    {ACPU_Trace,      MSG_ACPU_TRACE          }, // $80000009 Trace error
    {ACPU_LineA,      MSG_ACPU_LINEA          }, // $8000000A Line 1010 Emulator error
    {ACPU_LineF,      MSG_ACPU_LINEF          }, // $8000000B Line 1111 Emulator error
    {0x8000000C,      MSG_ACPU_EMUINTR        }, // $8000000C Emulator interrupt
    {0x8000000D,      MSG_ACPU_COPRVIOL       }, // $8000000D Coprocessor protocol violation
    {ACPU_Format,     MSG_ACPU_FORMAT         }, // $8000000E Stack frame format error
    {ACPU_Spurious,   MSG_ACPU_SPURIOUS       }, // $80000018 Spurious interrupt error
    {ACPU_AutoVec1,   MSG_ACPU_AUTOVEC1       }, // $80000019 AutoVector Level 1 interrupt error
    {ACPU_AutoVec2,   MSG_ACPU_AUTOVEC2       }, // $8000001A AutoVector Level 2 interrupt error
    {ACPU_AutoVec3,   MSG_ACPU_AUTOVEC3       }, // $8000001B AutoVector Level 3 interrupt error
    {ACPU_AutoVec4,   MSG_ACPU_AUTOVEC4       }, // $8000001C AutoVector Level 4 interrupt error
    {ACPU_AutoVec5,   MSG_ACPU_AUTOVEC5       }, // $8000001D AutoVector Level 5 interrupt error
    {ACPU_AutoVec6,   MSG_ACPU_AUTOVEC6       }, // $8000001E AutoVector Level 6 interrupt error
    {ACPU_AutoVec7,   MSG_ACPU_AUTOVEC7       }, // $8000001F AutoVector Level 7 interrupt error
    {0x80000030,      MSG_ACPU_FPCPBRANCH     }, // $80000030 FPCP branch or set on unordered condition
    {0x80000031,      MSG_ACPU_FPCPINEXACT    }, // $80000031 FPCP inexact result
    {0x80000032,      MSG_ACPU_FPCPDIVZERO    }, // $80000032 FPCP divide by zero
    {0x80000033,      MSG_ACPU_FPCPUNDER      }, // $80000033 FPCP underflow
    {0x80000034,      MSG_ACPU_FPCPOPERROR    }, // $80000034 FPCP operand error
    {0x80000035,      MSG_ACPU_FPCPOVER       }, // $80000035 FPCP overflow
    {0x80000036,      MSG_ACPU_NAN            }, // $80000036 FPCP signaling NAN
    {0x80000037,      MSG_ACPU_UNIMPLDTYPE    }, // $80000037 FPCP unimplemented data type
    {0x80000038,      MSG_ACPU_MMUCONFIG      }, // $80000038 PMMU configuration
    {0x80000039,      MSG_ACPU_MMUILLEGAL     }, // $80000039 PMMU illegal configuration
    {0x8000003A,      MSG_ACPU_MMUACCVIOL     }, // $8000003A PMMU access level violation
    {0x8000003C,      MSG_ACPU_UNIMPLEA       }, // $8000003C FPCP unimplemented effective address
    {0x8000003D,      MSG_ACPU_UNIMPLII       }, // $8000003D FPCP unimplemented integer instruction
    {AN_ExecLib,      MSG_AN_EXECLIB          }, // $01000000
    {AN_ExcptVect,    MSG_AN_EXCPTVECT        }, // $01000001 68000 exception vector checksum (obs.)
    {AN_BaseChkSum,   MSG_AN_BASECHKSUM       }, // $01000002 Execbase checksum bad (obs.)
    {AN_LibChkSum,    MSG_AN_LIBCHKSUM        }, // $01000003 Library checksum failure
    {AN_MemCorrupt,   MSG_AN_MEMCORRUPT       }, // $81000005 Corrupt memory list detected in FreeMem
    {AN_IntrMem,      MSG_AN_INTRMEM          }, // $81000006 No memory for interrupt servers
    {AN_InitAPtr,     MSG_AN_INITAPTR         }, // $01000007 InitStruct() of an APTR source (obs.)
    {AN_SemCorrupt,   MSG_AN_SEMCORRUPT       }, // $01000008 A semaphore is in an illegal state at ReleaseSemaphore()
    {AN_FreeTwice,    MSG_AN_FREETWICE        }, // $01000009 Freeing memory that is already free
    {AN_BogusExcpt,   MSG_AN_BOGUSEXCPT       }, // $8100000A Illegal 68k exception taken (obs.)
    {AN_IOUsedTwice,  MSG_AN_IOUSEDTWICE      }, // $0100000B Attempt to reuse active IORequest
    {AN_MemoryInsane, MSG_AN_MEMORYINSANE     }, // $0100000C Sanity check on memory list failed during AvailMem(MEMF_LARGEST)
    {AN_IOAfterClose, MSG_AN_IOAFTERCLOSE     }, // $0100000D IO attempted on closed IORequest
    {AN_StackProbe,   MSG_AN_STACKPROBE       }, // $0100000E Stack appears to extend out of range
    {AN_BadFreeAddr,  MSG_AN_BADFREEADDR      }, // $0100000F Memory header not located. [ Usually an invalid address passed to FreeMem() ]
    {AN_BadSemaphore, MSG_AN_BADSEMAPHORE     }, // $01000010 An attempt was made to use the old message semaphores.
    {0x810000FF,      MSG_AN_BADQUICKINT      }, // $810000FF A quick interrupt has happened to an uninitialized vector.
    {AN_GraphicsLib,  MSG_AN_GRAPHICSLIB      }, // $02000000
    {AN_GfxNewError,  MSG_AN_GFXNEWERROR      }, // $0200000C
    {AN_GfxFreeError, MSG_AN_GFXFREEERROR     }, // $0200000D
    {AN_ObsoleteFont, MSG_AN_OBSOLETEFONT     }, // $02000401 unsupported font description used
    {AN_GfxNoMem,     MSG_AN_GFXNOMEM         }, // $82010000 graphics out of memory
    {AN_GfxNoMemMspc, MSG_AN_GFXNOMEMMSPC     }, // $82010001 MonitorSpec alloc, no memory
    {AN_LongFrame,    MSG_AN_LONGFRAME        }, // $82010006 long frame, no memory
    {AN_ShortFrame,   MSG_AN_SHORTFRAME       }, // $82010007 short frame, no memory
    {AN_TextTmpRas,   MSG_AN_TEXTTMPRAS       }, // $02010009 text, no memory for TmpRas
    {AN_BltBitMap,    MSG_AN_BLTBITMAP        }, // $8201000A BltBitMap, no memory
    {AN_RegionMemory, MSG_AN_REGIONMEMORY     }, // $8201000B regions, memory not available
    {AN_MakeVPort,    MSG_AN_MAKEVPORT        }, // $82010030 MakeVPort, no memory
    {AN_GfxNoLCM,     MSG_AN_GFXNOLCM         }, // $82011234 emergency memory not available
    {AN_LayersLib,    MSG_AN_LAYERSLIB        }, // $03000000
    {AN_LayersNoMem,  MSG_AN_LAYERSNOMEM      }, // $83010000 layers out of memory
    {AN_Intuition,    MSG_AN_INTUITION        }, // $04000000
    {AN_GadgetType,   MSG_AN_GADGETTYPE       }, // $84000001 unknown gadget type
    {AN_BadGadget,    MSG_AN_BADGADGET        }, // $04000001 Recovery form of AN_GadgetType
    {AN_ItemBoxTop,   MSG_AN_ITEMBOXTOP       }, // $84000006 item box top < RelZero
    {AN_SysScrnType,  MSG_AN_SYSSCRNTYPE      }, // $84000009 open sys screen, unknown type
    {AN_BadState,     MSG_AN_BADSTATE         }, // $8400000C Bad State Return entering Intuition
    {AN_BadMessage,   MSG_AN_BADMESSAGE       }, // $8400000D Bad Message received by IDCMP
    {AN_WeirdEcho,    MSG_AN_WEIRDECHO        }, // $8400000E Weird echo causing incomprehension
    {AN_NoConsole,    MSG_AN_NOCONSOLE        }, // $8400000F couldn't open the Console Device
    {AN_NoISem,       MSG_AN_NOISEM           }, // $04000010 Intuition skipped obtaining a sem
    {AN_ISemOrder,    MSG_AN_ISEMORDER        }, // $04000011 Intuition obtained a sem in bad order
    {AN_CreatePort,   MSG_AN_CREATEPORT       }, // $84010002 create port, no memory
    {AN_ItemAlloc,    MSG_AN_ITEMALLOC        }, // $04010003 item plane alloc, no memory
    {AN_SubAlloc,     MSG_AN_SUBALLOC         }, // $04010004 sub alloc, no memory
    {AN_PlaneAlloc,   MSG_AN_PLANEALLOC       }, // $84010005 plane alloc, no memory
    {AN_OpenScreen,   MSG_AN_OPENSCREEN       }, // $84010007 open screen, no memory
    {AN_OpenScrnRast, MSG_AN_OPENSCRNRAST     }, // $84010008 open screen, raster alloc, no memory
    {AN_AddSWGadget,  MSG_AN_ADDSWGADGET      }, // $8401000A add SW gadgets, no memory
    {AN_OpenWindow,   MSG_AN_OPENWINDOW       }, // $8401000B open window, no memory
    {AN_MathLib,      MSG_AN_MATHLIB          }, // $05000000
    {AN_DOSLib,       MSG_AN_DOSLIB           }, // $07000000
    {AN_EndTask,      MSG_AN_ENDTASK          }, // $07000002 EndTask didn't
    {AN_QPktFail,     MSG_AN_QPKTFAIL         }, // $07000003 Qpkt failure
    {AN_AsyncPkt,     MSG_AN_ASYNCPKT         }, // $07000004 Unexpected packet received
    {AN_FreeVec,      MSG_AN_FREEVEC          }, // $07000005 Freevec failed
    {AN_DiskBlkSeq,   MSG_AN_DISKBLKSEQ       }, // $07000006 Disk block sequence error
    {AN_BitMap,       MSG_AN_BITMAP           }, // $07000007 Bitmap corrupt
    {AN_KeyFree,      MSG_AN_KEYFREE          }, // $07000008 Key already free
    {AN_BadChkSum,    MSG_AN_BADCHKSUM        }, // $07000009 Invalid checksum
    {AN_DiskError,    MSG_AN_DISKERROR        }, // $0700000A Disk Error
    {AN_KeyRange,     MSG_AN_KEYRANGE         }, // $0700000B Key out of range
    {AN_BadOverlay,   MSG_AN_BADOVERLAY       }, // $0700000C Bad overlay
    {AN_BadInitFunc,  MSG_AN_BADINITFUNC      }, // $0700000D Invalid init packet for cli/shell
    {AN_FileReclosed, MSG_AN_FILERECLOSED     }, // $0700000E A filehandle was closed more than once
    {AN_StartMem,     MSG_AN_STARTMEM         }, // $07010001 no memory at startup
    {AN_RAMLib,       MSG_AN_RAMLIB           }, // $08000000
    {AN_BadSegList,   MSG_AN_BADSEGLIST       }, // $08000001 overlays are illegal for library segments
    {AN_IconLib,      MSG_AN_ICONLIB          }, // $09000000
    {AN_ExpansionLib, MSG_AN_EXPANSIONLIB     }, // $0A000000
    {AN_BadExpansionFree, MSG_AN_BADEXPANSIONFREE }, // $0A000001 Freeed free region
    {AN_DiskfontLib,  MSG_AN_DISKFONTLIB      }, // $0B000000
    {AN_AudioDev,     MSG_AN_AUDIODEV         }, // $10000000
    {AN_ConsoleDev,   MSG_AN_CONSOLEDEV       }, // $11000000
    {AN_NoWindow,     MSG_AN_NOWINDOW         }, // $11000001 Console can't open initial window
    {AN_GamePortDev,  MSG_AN_GAMEPORTDEV      }, // $12000000
    {AN_KeyboardDev,  MSG_AN_KEYBOARDDEV      }, // $13000000
    {AN_TrackDiskDev, MSG_AN_TRACKDISKDEV     }, // $14000000
    {AN_TDCalibSeek,  MSG_AN_TDCALIBSEEK      }, // $14000001 calibrate: seek error
    {AN_TDDelay,      MSG_AN_TDDELAY          }, // $14000002 delay: error on timer wait
    {AN_TimerDev,     MSG_AN_TIMERDEV         }, // $15000000
    {AN_TMBadReq,     MSG_AN_TMBADREQ         }, // $15000001 bad request
    {AN_TMBadSupply,  MSG_AN_TMBADSUPPLY      }, // $15000002 power supply -- no 50/60hz ticks
    {AN_CIARsrc,      MSG_AN_CIARSRC          }, // $20000000
    {AN_DiskRsrc,     MSG_AN_DISKRSRC         }, // $21000000
    {AN_DRHasDisk,    MSG_AN_DRHASDISK        }, // $21000001 get unit: already has disk
    {AN_DRIntNoAct,   MSG_AN_DRINTNOACT       }, // $21000002 interrupt: no active unit
    {AN_MiscRsrc,     MSG_AN_MISCRSRC         }, // $22000000
    {AN_BootStrap,    MSG_AN_BOOTSTRAP        }, // $30000000
    {AN_BootError,    MSG_AN_BOOTERROR        }, // $30000001 boot code returned an error
    {AN_Workbench,    MSG_AN_WORKBENCH        }, // $31000000
    {AN_NoFonts,      MSG_AN_NOFONTS          }, // $B1000001
    {AN_WBBadStartupMsg2,MSG_AN_WBBADSTARTUPMSG2 }, // $31000002
    {AN_WBBadIOMsg,   MSG_AN_WBBADIOMSG       }, // $31000003 Hacker code?
    {AN_WBReLayoutToolMenu,MSG_AN_WBRELAYOUTTOOLMENU }, // $B1010009 GadTools broke?
    {AN_DiskCopy,     MSG_AN_DISKCOPY         }, // $32000000
    {AN_GadTools,     MSG_AN_GADTOOLS         }, // $33000000
    {AN_UtilityLib,   MSG_AN_UTILITYLIB       }, // $34000000
    {AN_Unknown,      MSG_AN_LAWBREAKER       }, // $35000000
    {AN_Aros,         MSG_AN_AROS             }, // $40000000
    {AN_OOP,          MSG_AN_OOP              }, // $41000000
    {AN_Hidd,         MSG_AN_HIDD             }, // $42000000
    {AN_HiddNoRoot,   MSG_AN_HIDDNOROOT       }, // $C2000001 /* Could not create root device */
    {-1,              0                       }
};

const struct
{
    ULONG id;
    CONST_STRPTR entry;
} objects[] =
{
    {0x00008000,     ""                      }, // $00008000
    {AO_ExecLib,     "exec.library"          }, // $00008001
    {AO_GraphicsLib, "graphics.library"      }, // $00008002
    {AO_LayersLib,   "layers.library"        }, // $00008003
    {AO_Intuition,   "intuition.library"     }, // $00008004
    {AO_MathLib,     "math#?.library"        }, // $00008005
    {AO_DOSLib,      "dos.library"           }, // $00008007
    {AO_RAMLib,      "ramlib"                }, // $00008008
    {AO_IconLib,     "icon.library"          }, // $00008009
    {AO_ExpansionLib,"expansion.library"     }, // $0000800A
    {AO_DiskfontLib, "diskfont.library"      }, // $0000800B
    {AO_UtilityLib,  "utility.library"       }, // $0000800C
    {AO_KeyMapLib,   "keymap.library"        }, // $0000800D
    {AO_AudioDev,    "audio.device"          }, // $00008010
    {AO_ConsoleDev,  "console.device"        }, // $00008011
    {AO_GamePortDev, "gameport.device"       }, // $00008012
    {AO_KeyboardDev, "keyboard.device"       }, // $00008013
    {AO_TrackDiskDev,"trackdisk.device"      }, // $00008014
    {AO_TimerDev,    "timer.device"          }, // $00008015
    {AO_CIARsrc,     "cia?.resource"         }, // $00008020
    {AO_DiskRsrc,    "disk.resource"         }, // $00008021
    {AO_MiscRsrc,    "misc.resource"         }, // $00008022
    {AO_BootStrap,   "bootstrap"             }, // $00008030
    {AO_Workbench,   "Workbench"             }, // $00008031
    {AO_DiskCopy,    "DiskCopy"              }, // $00008032
    {AO_GadTools,    "gadtools"              }, // $00008033
    {AO_ArosLib,     "aros.library"          }, // $00008040
    {AO_OOPLib,      "oop.library"           }, // $00008041
    {AO_HiddLib,     "hidd.library"          }, // $00008042
    {-1,             NULL                    }
};

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH2(LONG, IdAlert,

/*  SYNOPSIS */
        AROS_LHA(ULONG           , id     , D0),
        AROS_LHA(struct TagItem *, taglist, A0),

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 7, Identify)

/*  FUNCTION
        Get a human readable description of the alert ('Guru') code.

    INPUTS
        Code    -- (ULONG) alert code, as defined in exec/alerts.h

        TagList -- (struct TagItem *) tags that describe further
                   options.

    RESULT
        Error   -- (LONG) error code, or 0 if everything went fine.

    TAGS
        IDTAG_DeadStr   -- (STRPTR) Alert type string (deadend or
                           recoverable). You may skip this tag if you do not
                           want to get the string.

        IDTAG_SubsysStr -- (STRPTR) String of the subsystem that caused
                           the alert (CPU, exec.library, ...). You may skip this tag
                           if you do not want to get the string.

        IDTAG_GeneralStr-- (STRPTR) General alert cause. You  may skip
                           this tag if you do not want to get the string.

        IDTAG_SpecStr   -- (STRPTR) Specified alert cause. You may skip
                           this tag if you do not want to get the string.

        IDTAG_StrLength -- (UWORD) Maximum length of the string buffer,
                           including termination. Defaults to 50.

        IDTAG_Localize  -- [V8] (BOOL) FALSE to get English strings
                           only, TRUE for localized strings. This is useful for
                           applications with English as only language. Defaults to TRUE.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    STRPTR deadstr = NULL;
    STRPTR subsysstr = NULL;
    STRPTR generalstr = NULL;
    STRPTR specstr = NULL;
    BOOL localize = TRUE;
    ULONG strlength = 50;
    ULONG searchFor;
    ULONG i;
    BOOL found;

    struct TagItem *tag;
    struct TagItem *tags;

    for (tags = taglist; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case IDTAG_DeadStr:
                deadstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SubsysStr:
                subsysstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_GeneralStr:
                generalstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_SpecStr:
                specstr = (STRPTR)tag->ti_Data;
                break;

            case IDTAG_StrLength:
                strlength = tag->ti_Data;
                break;

            case IDTAG_Localize:
                localize = tag->ti_Data ? TRUE : FALSE;
                break;
        }
    }

    if (strlength == 0)
    {
        return IDERR_NOLENGTH;
    }

    if (localize) {
        /* FIXME: How should we handle this? */
    }

    if (deadstr)
    {
        if (id & (1L << 31)) // Bit 31 set? If yes: deadend
        {
            strlcpy(deadstr, _(MSG_ALERT_DEADEND), strlength);
        }
        else
        {
            strlcpy(deadstr, _(MSG_ALERT_RECOVERY), strlength);
        }
    }

    if (subsysstr)
    {
        found = FALSE;
        searchFor = id & 0x7f000000;
        for (i = 0; subsystem[i].id != -1; i++)
        {
            if (searchFor == subsystem[i].id)
            {
                strlcpy(subsysstr, subsystem[i].entry, strlength);
                found = TRUE;
                break;
            }
        }
        
        if (!found)
        {
            strlcpy(subsysstr, _(MSG_ALERT_UNKNOWN), strlength);
        }
    }

    if (generalstr)
    {
        found = FALSE;
        searchFor = id & 0xf0000;
        for (i = 0; general[i].id != -1; i++)
        {
            if (searchFor == general[i].id)
            {
                strlcpy(generalstr, _(general[i].entry), strlength);
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            strlcpy(generalstr, _(MSG_ALERT_UNKNOWN), strlength);
        }
    }

    if (specstr)
    {
        *specstr = '\0'; // because of strlcat()

        found = FALSE;

        // search for object
        if (id & (1L << 15))
        {
            searchFor = id & 0x7fff;
            for (i = 0; objects[i].id != -1; i++)
            {
                if (searchFor == objects[i].id)
                {
                    strlcpy(specstr, objects[i].entry, strlength);
                    found = TRUE;
                    break;
                }
            }
        }

        // search for alert
        searchFor = id & 0x7fffffff;
        for (i = 0; alerts[i].id != -1; i++)
        {
            if (searchFor == alerts[i].id)
            {
                strlcat(specstr, _(alerts[i].entry), strlength);
                found = TRUE;
                break;
            }
        }
        
        if (!found)
        {
            strlcpy(specstr, _(MSG_ALERT_UNKNOWN), strlength);
        }
    }

    return IDERR_OKAY;

    AROS_LIBFUNC_EXIT
} /* IdAlert */
