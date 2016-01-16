
;*
;*
;*  $VER: classbase.i 1.10 (18.8.97)
;*  anim.datatype 1.10
;*
;*  Header file for DataTypes class
;*
;*  Written 1996(1997 by Roland 'Gizzy' Mainz
;*  Original example source from David N. Junod
;*
;*


    IFND CLASSBASE_I
CLASSBASE_I SET 1

;-----------------------------------------------------------------------

    INCLUDE "exec/types.i"
    INCLUDE "exec/libraries.i"
    INCLUDE "exec/lists.i"
    INCLUDE "exec/semaphores.i"
    INCLUDE "utility/tagitem.i"
    INCLUDE "intuition/classes.i"

;-----------------------------------------------------------------------

    STRUCTURE ClassBase,ClassLibrary_SIZEOF
    ULONG    cb_SysBase
    ULONG    cb_UtilityBase
    ULONG    cb_DOSBase
    ULONG    cb_IFFParseBase;
    ULONG    cb_GfxBase
    ULONG    cb_IntuitionBase
    ULONG    cb_DataTypesBase
    ULONG    cb_SuperClassBase
    ULONG    cb_SegList
    STRUCT   cb_Lock,SS_SIZE
    LABEL ClassBase_SIZEOF

;-----------------------------------------------------------------------

    LIBINIT

    LIBDEF    _LVODispatch

;---------------------------------------------------------------------------

CALL MACRO <Function_Name>
    xref _LVO\1
     jsr _LVO\1(A6)
     ENDM

;---------------------------------------------------------------------------

GO   MACRO <Function_Name>
    xref _LVO\1
     jmp _LVO\1(A6)
     ENDM

;---------------------------------------------------------------------------

    ENDC    ; CLASSBASE_I


