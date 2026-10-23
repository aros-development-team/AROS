MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
  /* String literals, one pool per bank. The final link appends each pool to
   * its bank, merging duplicate strings and shared tails across modules
   * (merging only happens in a final link, not in this -r one). The .ext
   * pool takes only the .ext modules' strings, so that no .rom code points
   * into the extended ROM; everything else, static libs included, goes to
   * the .rom pool.
   */
  .extstr                     : {
INCLUDE ext_strs.ld
                                } >valid
  .romstr                     : { *(.romstrings .rodata.str1.1) } >valid
  .ext                        : { _ext_start = .;
INCLUDE ext_objs.ld
  /* Static-lib code used ONLY by .ext modules. The catch-all in .rom below
   * would otherwise pull these into the main bank, where nothing calls them.
   * Keep this list to objects with no .rom-resident caller - a .rom module
   * calling into .ext would make the main bank depend on the extended ROM.
   */
  *libamiga.a:andrectrect.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:callhook.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:coercemethod.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:createport.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:domethod.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:dosupermethod.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:hookentry.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:liballocaligned.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:sprintf.o(.text .text.* .rodata .rodata.*)
  *libamiga.a:timedelay.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:deinitrastport.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:freestruct.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readbyte.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readdouble.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readfloat.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readlong.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readstring.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readstruct.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:readword.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:rkprintf.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writebyte.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writedouble.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writefloat.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writelong.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writestring.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writestruct.o(.text .text.* .rodata .rodata.*)
  *libarossupport.a:writeword.o(.text .text.* .rodata .rodata.*)
  *libstdc.static.a:abs.o(.text .text.* .rodata .rodata.*)
  *libstdc.static.a:atol.o(.text .text.* .rodata .rodata.*)
  *libstdc.static.a:strlcat.o(.text .text.* .rodata .rodata.*)
  *libstdc.static.a:strlcpy.o(.text .text.* .rodata .rodata.*)
  *libstdc.static.a:strncat.o(.text .text.* .rodata .rodata.*)
                                  _ext_end = .; 
                                } >valid
  .rom                        : { _rom_start = .;
INCLUDE rom_objs.ld
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
