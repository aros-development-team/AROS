MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
  .start.MEMF_LOCAL           : {
                                } >valid
  .kick.MEMF_KICK             : { _kick_start = .;
                                  _kick_end = .; 
                                } >valid
  .rom.MEMF_LOCAL             : { _rom_start = .;
                                   /* This is for the static libs */
                                  *(.text .text.* .rodata .rodata.*)
                                  *(.eh_frame)
                                  _rom_end = .;   
                                  _ext_start = .;   
                                  /* Intentionally empty */
                                  _ext_end = .;   
                                } >valid
  /* SS, typically at 0x800-0x1000. Since MEMF_CHIP RAM starts at
   * 0x1000, this is a good place for it. Only used until after
   * all expansion ROMs have been run, at which point we'll allocate
   * a faster location for it.
   */
  .ss.MEMF_LOCAL              : {
                                  _ss = .;
                                  . = . + 0x800;
                                  _ss_end = .;
                                 } >valid
}
