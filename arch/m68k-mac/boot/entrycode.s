# 1 "entrycode.S"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "entrycode.S"
 .section entrycodes,"aw",@progbits




 .globl initial_ssp
initial_ssp:
 .dc.l 0x100000


 .globl hardware_reset_init_1
 .section .text
 .align 4
hardware_reset_init_1:
 move.l #((_end)+0x10000),%sp
 move.l #0,%a0
 move.l #0,%d0
myloop:
 move.l %d0,%a0
 add.l #0xf9000e00,%a0
 move.l #0x00000000,(%a0)
 add.l #8,%d0
 cmpil #0x500,%d0
 bles myloop
 move.b 'A',%d0
 jsr serial_putc
 jmp main_init

 .globl software_reset
 .align 4

software_reset:
 lea initial_ssp,%a0
 move.l (%a0),%ssp

 jmp main_init
# 56 "entrycode.S"
 .globl serial_putc
 .align 4
serial_putc:
 cmpib #'\n',%d0
 jbne 1f

 move.b %d0,%d1
 move.b #'\r',%d0
 jbsr serial_putc
 move.b %d1,%d0
1:
# 76 "entrycode.S"
 movel #0x50f0c020,%a1

4: btst #2,%a1@(0x0)
 jeq 4b
 moveb %d0,%a1@(0x4)

 rts
