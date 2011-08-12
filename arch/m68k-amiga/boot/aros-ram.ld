MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
  .ext                        : { _ext_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/any_objs.ld
                                  _ext_end = .; 
                                } >valid
  .rom.MEMF_LOCAL             : { _rom_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/local_objs.ld
                                   /* This is for the static libs */
                                  *(.text .rodata .rodata.*)
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

  /* BSS, typically at 0x1020
   * The '+8' is to make sure not to overlap the
   * inital memchunk with areas must be zeroed.
   */
  .bss                        : {
                                  _bss = .;
                                  . = . + 0x8; /* Reserved for memchunk */
                                  *(.bss)
                                  _bss_end = .;
                                 } >valid

  /* This should cause angry complaints if .data exists
   * We don't want .data, and we don't need it.
   */
  .data                       : { *(.data)             } >invalid

}
