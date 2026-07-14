/*
 * Partial-link (ld -Ur) section-ordering script for amiga-m68k kernel modules.
 *
 * Orders each module's contents as:
 *   .text.romtag   - the Resident tag, at the module HEAD
 *   .text          - the module body
 *   .rodata        - folded in here (before the End marker) so it is covered by
 *                    the tag->End leap; otherwise big-.rodata modules (e.g. icon's
 *                    ~44KB of bitmaps) would still be walked word-by-word.
 *   .text.moduleend - the End marker = rt_EndSkip target = TRUE module tail
 * Combined with the ROM link emitting each module's .text before its .rodata
 * (arch/m68k-amiga/boot/mmakefile.src), the romtag scanner finds the tag on the
 * first read and leaps the whole module (body + rodata) to the next module's tag
 * via rt_EndSkip - instead of walking every word of the ROM.
 *
 * NOTE: AROS linker-set sub-sections (__INITLIB_LIST__, __aros_set_*, etc.) stay
 * in .rodata deliberately - the KOBJ link's OBJCOPY -L step collects/renames them,
 * so they must not be folded away. They are only a few dozen bytes per module.
 *
 * Wired in via KERNEL_KOBJ_LDSCRIPT in config/make.cfg (amiga-m68k only).
 */
SECTIONS {
  .text : {
    *(.text.romtag)
    *(.text .text.* .stub)
    *(.rodata .rodata.*)
    *(.text.moduleend)
  }
}
