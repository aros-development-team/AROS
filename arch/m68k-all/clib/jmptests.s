
        .macro  CMP     value,reg
        cmp.l   \value,\reg
        beq     0f
        trap    #1
0:
        .endm

        .macro  CMPS    slot,reg
        cmp.l   %sp@(4 * \slot),\reg
        beq     0f
        trap    #1
0:
        .endm

        .section ".aros.startup"
        .global _main
_main:
        move.l  #0xd20000d2,%d2
        move.l  #0xd30000d2,%d3
        move.l  #0xd40000d2,%d4
        move.l  #0xd50000d2,%d5
        move.l  #0xd60000d2,%d6
        move.l  #0xd70000d2,%d7
        move.l  #0xa20000d2,%a2
        move.l  #0xa30000d2,%a3
        move.l  #0xa40000d2,%a4
        move.l  #0xa50000d2,%a5
        move.l  #0xa60000d2,%a6
        move.l  #0xa7a7a7a7,%sp@-

        jsr     vfork
        CMP     #0xdead0001,%d0
        CMP     #0xd20000d2,%d2
        CMP     #0xd30000d2,%d3
        CMP     #0xd40000d2,%d4
        CMP     #0xd50000d2,%d5
        CMP     #0xd60000d2,%d6
        CMP     #0xd70000d2,%d7
        CMP     #0xa20000d2,%a2
        CMP     #0xa30000d2,%a3
        CMP     #0xa40000d2,%a4
        CMP     #0xa50000d2,%a5
        CMP     #0xa60000d2,%a6
        move.l  %sp@,%d0
        CMP     #0xa7a7a7a7,%d0

        move.l  #0xd2000022,%d2
        jsr     vfork
        CMP     #0xdead0001,%d0
        CMP     #0xd2000022,%d2
        CMP     #0xd30000d2,%d3
        CMP     #0xd40000d2,%d4
        CMP     #0xd50000d2,%d5
        CMP     #0xd60000d2,%d6
        CMP     #0xd70000d2,%d7
        CMP     #0xa20000d2,%a2
        CMP     #0xa30000d2,%a3
        CMP     #0xa40000d2,%a4
        CMP     #0xa50000d2,%a5
        CMP     #0xa60000d2,%a6
        move.l  %sp@,%d0
        CMP     #0xa7a7a7a7,%d0


        lea.l   %sp@(-4 * 16),%sp
        move.l  %sp,%sp@-
        jsr     setjmp
        addq.l  #4, %sp
        cmp.l   #0,%d0
        bne     0f
        move.l  %sp,%a0
        move.l  #0,%sp@-
        move.l  %a0,%sp@-
        jsr     longjmp
        move.l  #0xdeadbad,%d0
0:      
        CMPS    1,%d2
        CMPS    2,%d3
        CMPS    3,%d4
        CMPS    4,%d5
        CMPS    5,%d6
        CMPS    6,%d7
        CMPS    7,%a2
        CMPS    8,%a3
        CMPS    9,%a4
        CMPS    10,%a5
        CMPS    11,%a6

        cmp.l   #2,%d0
        beq     1f

        CMP     #0x1,%d0
        move.l  %sp,%a0
        move.l  #2,%sp@-
        move.l  %a0,%sp@-
        jsr     vfork_longjmp
1:

        lea.l   %sp@(4 * 16),%sp
        move.l  %sp@+,%d0
        CMP     #0xa7a7a7a7,%d0
        moveq.l #0,%d0
        rts

        .global __vfork
__vfork:
        move.l  %sp@(4),%a0
        move.l  #0xdead0001,%sp@-
        move.l  %a0,%sp@-
        cmp.l   #0xd2000022,%a0@(4)
        beq     0f
        jsr     vfork_longjmp
0:
        jsr     longjmp
