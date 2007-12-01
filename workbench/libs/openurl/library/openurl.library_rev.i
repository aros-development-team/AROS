VERSION EQU 7
REVISION EQU 2
DATE MACRO
    dc.b '1.12.2005'
    ENDM
VERS MACRO
    dc.b 'openurl.library 7.2'
    ENDM
VSTRING MACRO
    dc.b 'openurl.library 7.2 (1.12.2005)',13,10,0
    ENDM
VERSTAG MACRO
    dc.b 0,'$VER: openurl.library 7.2 (1.12.2005)',0
    ENDM
