#define LDSCRIPT_PART1 \
"/*\n"\
"    Script for final linking of AROS executables.\n"\
"\n"\
"    NOTE: This file is the result of a rearragement of the built-in ld script.\n"\
"          It's AROS-specific, in that it does constructors/destructors collecting\n"\
"          and doesn't care about some sections that are not used by AROS at the moment\n"\
"          or will never be.\n"\
"\n"\
"          It *should* be general enough to be used on many architectures.\n"\
"*/\n"\
"\n"\
"SECTIONS\n"\
"{\n"\
"  .text 0 :\n"\
"  {\n"\
"    *(.text)\n"\
"    *(.stub)\n"\
"    /* .gnu.warning sections are handled specially by elf32.em.  */\n"\
"    *(.gnu.warning)\n"\
"    *(.gnu.linkonce.t.*)\n"\
"  } =0x90909090\n"\
"\n"\
"  .rodata  0 : { *(.rodata) *(.rodata.*) *(.gnu.linkonce.r.*) }\n"\
"  .rodata1 0 : { *(.rodata1) }\n"\
"\n"\
"  /*\n"\
"     Used only on PPC.\n"\
"\n"\
"     NOTE: these should go one after the other one, so some tricks\n"\
"           must be used in the ELF loader to satisfy that requirement\n"\
"  */\n"\
"  .sdata2  0 : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }\n"\
"  .sbss2   0 : { *(.sbss2)  *(.sbss2.*)  *(.gnu.linkonce.sb2.*) }\n"\
"\n"\
"  .data  0 :\n"\
"  {\n"\
"    *(.data)\n"\
"    *(.data.*)\n"\
"    *(.gnu.linkonce.d.*)\n"

#define LDSCRIPT_PART2 \
"  }\n"\
"  .data1            0 : { *(.data1) }\n"\
"  .eh_frame         0 : { KEEP (*(.eh_frame)) }\n"\
"  .gcc_except_table 0 : { *(.gcc_except_table) }\n"\
"\n"\
"  /* We want the small data sections together, so single-instruction offsets\n"\
"     can access them all, and initialized data all before uninitialized, so\n"\
"     we can shorten the on-disk segment size.  */\n"\
"  .sdata   0 :\n"\
"  {\n"\
"    *(.sdata)\n"\
"    *(.sdata.*)\n"\
"    *(.gnu.linkonce.s.*)\n"\
"  }\n"\
"\n"\
"  .sbss 0 :\n"\
"  {\n"\
"    *(.sbss)\n"\
"    *(.sbss.*)\n"\
"    *(.gnu.linkonce.sb.*)\n"\
"    *(.scommon)\n"\
"  }\n"\
"\n"\
"  .bss 0 :\n"\
"  {\n"\
"   *(.bss)\n"\
"   *(.bss.*)\n"\
"   *(.gnu.linkonce.b.*)\n"\
"   *(COMMON)\n"\
"  }\n"\
"\n"\
"  .rel.text    0 : { *(rel.text)     }\n"\
"  .rel.rodata  0 : { *(.rel.rodata)  }\n"\
"  .rel.data    0 : { *(.rel.data)    }\n"\
"  .rel.ctors   0 : { *(.rel.ctors)   }\n"\
"  .rel.dtors   0 : { *(.rel.dtors)   }\n"\
"  .rel.sbss    0 : { *(.rel.sbss)    }\n"\
"  .rel.sbss2   0 : { *(.rel.sbss2)   }\n"\
"  .rel.bss     0 : { *(.rel.bss)     }\n"\
"  .rel.sdata   0 :\n"\
"  {\n"\
"    *(.rel.sdata)\n"\
"    *(.rel.sdata2)\n"\
"    *(.rel.gnu.linkonce.s.*)\n"\
"  }\n"\
"\n"\
"  .rela.text   0 : { *(.rela.text)   }\n"\
"  .rela.rodata 0 : { *(.rela.rodata) }\n"\
"  .rela.data   0 : { *(.rela.data)   }\n"\
"  .rela.ctors  0 : { *(.rela.ctors)  }\n"\
"  .rela.dtors  0 : { *(.rela.dtors)  }\n"\
"  .rela.sbss   0 : { *(.rela.sbss)   }\n"\
"  .rela.sbss2  0 : { *(.rela.sbss2)  }\n"\
"  .rela.bss    0 : { *(.rela.bss)    }\n"\
"  .rela.sdata  0 :\n"\
"  {\n"\
"    *(.rela.sdata)\n"\
"    *(.rela.sdata2)\n"\
"    *(.rela.gnu.linkonce.s.*)\n"\
"  }\n"\
"\n"\
"  /* Stabs debugging sections.  */\n"\
"  .stab 0 : { *(.stab) }\n"\
"  .stabstr 0 : { *(.stabstr) }\n"\
"  .stab.excl 0 : { *(.stab.excl) }\n"\
"  .stab.exclstr 0 : { *(.stab.exclstr) }\n"\
"  .stab.index 0 : { *(.stab.index) }\n"\
"  .stab.indexstr 0 : { *(.stab.indexstr) }\n"\
"  .comment 0 : { *(.comment) }\n"\
"  /* DWARF debug sections.\n"\
"     Symbols in the DWARF debugging sections are relative to the beginning\n"\
"     of the section so we begin them at 0.  */\n"\
"  /* DWARF 1 */\n"\
"  .debug          0 : { *(.debug) }\n"\
"  .line           0 : { *(.line) }\n"\
"  /* GNU DWARF 1 extensions */\n"\
"  .debug_srcinfo  0 : { *(.debug_srcinfo) }\n"\
"  .debug_sfnames  0 : { *(.debug_sfnames) }\n"\
"  /* DWARF 1.1 and DWARF 2 */\n"\
"  .debug_aranges  0 : { *(.debug_aranges) }\n"\
"  .debug_pubnames 0 : { *(.debug_pubnames) }\n"\
"  /* DWARF 2 */\n"\
"  .debug_info     0 : { *(.debug_info) *(.gnu.linkonce.wi.*) }\n"\
"  .debug_abbrev   0 : { *(.debug_abbrev) }\n"\
"  .debug_line     0 : { *(.debug_line) }\n"\
"  .debug_frame    0 : { *(.debug_frame) }\n"\
"  .debug_str      0 : { *(.debug_str) }\n"\
"  .debug_loc      0 : { *(.debug_loc) }\n"\
"  .debug_macinfo  0 : { *(.debug_macinfo) }\n"\
"  /* SGI/MIPS DWARF 2 extensions */\n"\
"  .debug_weaknames 0 : { *(.debug_weaknames) }\n"\
"  .debug_funcnames 0 : { *(.debug_funcnames) }\n"\
"  .debug_typenames 0 : { *(.debug_typenames) }\n"\
"  .debug_varnames  0 : { *(.debug_varnames) }\n"\
"  /* These must appear regardless of .  */\n"\
"}\n"
