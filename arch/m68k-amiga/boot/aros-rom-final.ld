/*
 * Final link of the ROM image (addresses come from --section-start). Each
 * bank gets its string and constant pools appended here, where the linker
 * merges them; see aros-rom.ld.
 */
SECTIONS
{
  .ext : { *(.ext) *(.extstr) *(.extsets) }
  .rom : { *(.rom) *(.romstr) *(.romsets) }
  .ss  : { *(.ss) }
}
