MEMORY 
{
  trap (rwx)    : org = 0x00000000 , l = 0x1000
  bss  (rw)     : org = 0x00001000 , l = 0x2000
  invalid (rwx) : org = 0xdead0000 , l = 0x1000
  rom  (rx)     : org = 0x00f80000 , l = 512K
}

SECTIONS 
{
  .sysbase                    : { . = 4; SysBase = .; } >trap
  .bss                        : {  _bss = .;
                                  *(.bss)
                                  _bss_end = .;        } >bss

  /* This should cause angry complaints if .data exists
   * We don't want .data, and we don't need it.
   */
  .data                       : { *(.data)             } >invalid

  .text     0x00f80000        : { _text_start = .;
  	                              *(.text)
                                  _text_end = .;       } >rom =0x0000
  .rodata                     : { *(.rodata .rodata.*) } >rom =0x0000
  .eh_frame                   : { *(.eh_frame)         } >rom =0x0000
}
