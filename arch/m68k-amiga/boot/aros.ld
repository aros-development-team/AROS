MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
  .rom                        : { _rom_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/rom_objs.ld
                                   /* This is for the static libs */
                                  *(.text .rodata .rodata.*)
                                  *(.eh_frame)
                                  _rom_end = .;   
                                } >valid
  .ext                        : { _ext_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/ext_objs.ld
                                  _ext_end = .; 
                                } >valid
  .bss                        : {  _bss = .;
                                  *(.bss)
                                  _bss_end = ALIGN(16);        } >valid

  /* This should cause angry complaints if .data exists
   * We don't want .data, and we don't need it.
   */
  .data                       : { *(.data)             }

}
