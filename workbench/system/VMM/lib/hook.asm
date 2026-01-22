                    INCLUDE        "utility/hooks.i"

                    XDEF           _CallHook_C

_CallHook_C:        * Calls a C hook function. Normally hook functions
                    * have to receive parameters in registers. For
                    * compilers such as GNU C, which aren't capable
                    * of specifying register arguments, this routine
                    * may be used for h_Entry, the C routine has
                    * to be stuffed into h_SubEntry
                    *
                    * Called with
                    *	A0 - pointer to hook data structure itself
                    *	A1 - pointer to parameter structure ("message")
                    *	A2 - Hook specific address data ("object")
                    *
                    * The hook function has to be specified as
                    * func (hook, message, object

                    movem.l   a0-a2,-(sp)
                    move.l    h_SubEntry(a0),a0
                    jsr       (a0)
                    add.w     #12,sp
                    rts

                    end
