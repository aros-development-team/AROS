MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
  .ext                        : { _ext_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/ext_objs.ld
                                  _ext_end = .; 
                                } >valid
  .rom                        : { _rom_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/rom_objs.ld
                                   /* This is for the static libs */
                                  *(.text .text.* .rodata .rodata.*)
                                  *(.eh_frame)
                                  _rom_end = .;   
                                } >valid
  /* SS, typically at 0x800-0x1000. Since MEMF_CHIP RAM starts at
   * 0x1000, this is a good place for it. Only used until after
   * all expansion ROMs have been run, at which point we'll allocate
   * a faster location for it.
   */
  .ss                         : {
                                  _ss = .;
                                  . = . + 0x800;
                                  _ss_end = .;
                                 } >valid

  /* This should cause angry complaints if .data exists
   * We don't want .bss, and we don't need it.
   */
  .bss                        : { *(.bss) } >invalid

  /* This should cause angry complaints if .data exists
   * We don't want .data, and we don't need it.
   */
  .data                       : { *(.data)             } >invalid

}
