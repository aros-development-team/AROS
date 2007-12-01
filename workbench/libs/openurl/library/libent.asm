
        SECTION openurl.library,CODE

        NOLIST

        INCLUDE "openurl.library_rev.i"
        INCLUDE "exec/libraries.i"
        INCLUDE "exec/resident.i"
        INCLUDE "exec/initializers.i"
        INCLUDE "exec/lists.i"
        INCLUDE "exec/semaphores.i"

        LIST

        XREF    _LinkerDB
        XREF    _lib_name
        XREF    ENDCODE

        XREF    _initLib
        XREF    _openLib
        XREF    _expungeLib
        XREF    _closeLib
        XREF    _dispatch

        XREF    _URL_OpenA
        XREF    _URL_OldGetPrefs
        XREF    _URL_OldFreePrefs
        XREF    _URL_OldSetPrefs
        XREF    _URL_OldGetDefaultPrefs
        XREF    _URL_OldLaunchPrefsApp
        XREF    _DoFunction
        XREF    _URL_GetPrefsA
        XREF    _URL_FreePrefsA
        XREF    _URL_SetPrefsA
        XREF    _URL_LaunchPrefsAppA
        XREF    _URL_GetAttr

        XDEF    _ID

PRI     EQU 0

start:  moveq   #-1,d0
        rts

romtag:
        dc.w    RTC_MATCHWORD
        dc.l    romtag
        dc.l    ENDCODE
        dc.b    RTF_AUTOINIT
        dc.b    VERSION
        dc.b    NT_LIBRARY
        dc.b    PRI
        dc.l    _lib_name
        dc.l    _ID
        dc.l    init

_ID:    VSTRING

        CNOP    0,4

init:   dc.l    LIB_SIZE
        dc.l    funcTable
        dc.l    dataTable
        dc.l    _initLib

V_DEF   MACRO
    dc.w    \1+(*-funcTable)
    ENDM

funcTable:
        DC.W    -1

        V_DEF   _openLib
        V_DEF   _closeLib
        V_DEF   _expungeLib
        V_DEF   nil

        V_DEF    _URL_OpenA
        V_DEF    _URL_OldGetPrefs
        V_DEF    _URL_OldFreePrefs
        V_DEF    _URL_OldSetPrefs
        V_DEF    _URL_OldGetDefaultPrefs
        V_DEF    _URL_OldLaunchPrefsApp
        V_DEF    query
        V_DEF    _URL_GetPrefsA
        V_DEF    _URL_FreePrefsA
        V_DEF    _URL_SetPrefsA
        V_DEF    _URL_LaunchPrefsAppA
        V_DEF    _URL_GetAttr

        DC.W    -1

dataTable:
        INITBYTE LN_TYPE,NT_LIBRARY
        INITLONG LN_NAME,_lib_name
        INITBYTE LIB_FLAGS,(LIBF_SUMUSED!LIBF_CHANGED)
        INITWORD LIB_VERSION,VERSION
        INITWORD LIB_REVISION,REVISION
        INITLONG LIB_IDSTRING,_ID
        dc.w     0

        CNOP    0,4

nil:    moveq   #0,d0
        rts

query:  movem.l a1/a4,-(sp)
        lea     _LinkerDB,a4
        subq.l  #4,sp
        movea.l sp,a1
        bsr     _dispatch
        movem.l (sp)+,a0/a1/a4
        rts

        END
