MEMORY 
{
  trap (rwx)    : org = 0x00000000 , l = 0x1000
  bss  (rw)     : org = 0x00001000 , l = 0x2000
  invalid (rwx) : org = 0xdead0000 , l = 0x0
  rom  (rx)     : org = 0x00c00000 , l = 512K
  ext  (rx)     : org = 0x00c80000 , l = 512K
}

SECTIONS 
{
  .sysbase                    : { . = 4; SysBase = .; } >trap
  .bss                        : {  _bss = .;
                                  *(.bss)
                                  _bss_end = ALIGN(16);        } >bss

  /* This should cause angry complaints if .data exists
   * We don't want .data, and we don't need it.
   */
  .data                       : { *(.data)             } >invalid

  .ext          0x00c80000    : { _ext_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/ext_objs.ld
                                  _ext_end = .; 
                                } >ext =0xffff
  .rom          0x00c00000    : { _rom_start = .;
INCLUDE ../../../bin/amiga-m68k/gen/rom_objs.ld
				  /* This is for the static libs */
				  *(.text .rodata .rodata.*)
                                  _rom_end = .;   
                                } >rom =0xffff
  .eh_frame                   : { *(.eh_frame)         } >rom =0xffff
}
