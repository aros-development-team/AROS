MEMORY {
 valid  (rwx)    : org = 0x00000000 , l = 2M
 tempstack (rwx) : org = 0x0003f800, l = 0x800
 invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS
{
 .start.MEMF_LOCAL           : { _ext_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/start_objs.ld
                               } >valid
 .kick.MEMF_KICK             : { _kick_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/any_objs.ld
                                 _kick_end = .;
                                 _ext_end = .;
                               } >valid
 .rom.MEMF_LOCAL             : { _rom_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/local_objs.ld
                                  /* This is for the static libs */
                                 *(.text .text.* .rodata .rodata.*)
                                 *(.eh_frame)
                                 _rom_end = .;
                               } >valid
 /* Supervisor stack, only used until after all autoconfig boards have
  * been initialized, at which point we'll allocate a faster location for it.
  */
.ss                          : {
                                _ss = .;
                                . = . + 0x800;
                                _ss_end = .;
                               } >tempstack

 /* This should cause angry complaints if .data exists
  * We don't want .bss, and we don't need it.
  */
 .bss                        : { *(.bss)              } >invalid

 /* This should cause angry complaints if .data exists
  * We don't want .data, and we don't need it.
  */
 .data                       : { *(.data)             } >invalid

}