/*
 * Final link of the ROM image (addresses come from --section-start). Each
 * bank gets its string pool appended here, where the linker merges the
 * strings; see aros-rom.ld.
 */
SECTIONS
{
  .ext : { *(.ext) *(.extstr) }
  .rom : { *(.rom) *(.romstr) }
  .ss  : { *(.ss) }
}
