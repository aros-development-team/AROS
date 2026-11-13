const static char LDSCRIPT_PART1[] = 
"/*\n"
"    Script for final linking of AROS executables.\n"
"\n"
"    NOTE: This file is the result of a rearrangement of the built-in ld script.\n"
"          It's AROS-specific, in that it does constructors/destructors collecting\n"
"          and doesn't care about some sections that are not used by AROS at the moment\n"
"          or will never be.\n"
"\n"
"          It *should* be general enough to be used on many architectures.\n"
"*/\n"
"\n" \
"SECTIONS\n"
"{\n";

const static char LDSCRIPT_PART2A[] = 
"  {\n"
"    *(.tag.*)\n"
"  }";

const static char LDSCRIPT_PART2B[] = 
"  {\n"
"    *(.aros.startup)\n"
"    *(.text)\n";

static const char LDSCRIPT_PART3A[] =
"    *(.text.*)\n"
"    *(.stub)\n"
"    /* .gnu.warning sections are handled specially by elf32.em.  */\n"
"    *(.gnu.warning)\n"
"    *(.gnu.linkonce.t.*)\n"
"  }";

static const char LDSCRIPT_PART3B[] =
"  {\n"
"    *(.rodata)\n"
"    *(.rodata.*)\n"
"    *(.gnu.linkonce.r.*)\n"
"    . = ALIGN(0x10);\n";

static const char LDSCRIPT_PART4A[] =
"\n"
#ifdef TARGET_FORMAT_EXE
"  . = DATA_SEGMENT_ALIGN(0x1000,0x1000);\n"
#endif
"  /*\n"
"     Used only on PPC.\n"
"\n"
"     NOTE: these should go one after the other one, so some tricks\n"
"           must be used in the ELF loader to satisfy that requirement\n"
"  */\n";

static const char LDSCRIPT_PART4B[] =
"  {\n"
"    *(.data)\n"
"    *(.data.*)\n"
"    *(.gnu.linkonce.d.*)\n"
"  }\n";

static const char LDSCRIPT_PART4C[] =
"  .got ALIGN(8) :\n"
"  {\n"
"    PROVIDE(_GLOBAL_OFFSET_TABLE_ = .);\n"
"    KEEP(*(.got))\n"
"    KEEP(*(.got.plt))\n"
"  }\n";

static const char LDSCRIPT_PART4D[] =
"  /* ARM-specific exception stuff */\n"
"  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) }\n"
"   PROVIDE(__exidx_start = .);\n"
"  .ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }\n"
"   PROVIDE(__exidx_end = .);\n";

static const char LDSCRIPT_PART4E[] =
"  {\n"
"     PROVIDE(__eh_frame_start = .);\n"
"     KEEP (*(.eh_frame))\n";

static const char LDSCRIPT_PART5A[] =
"     PROVIDE(__eh_frame_end = .);\n"
"  }\n";

static const char LDSCRIPT_PART5B[] =
"\n"
"  /* We want the small data sections together, so single-instruction offsets\n"
"     can access them all, and initialized data all before uninitialized, so\n"
"     we can shorten the on-disk segment size.  */\n";

static const char LDSCRIPT_PART5C[] =
"  {\n"
"    *(.sdata)\n"
"    *(.sdata.*)\n"
"    *(.gnu.linkonce.s.*)\n"
"  }\n"
"\n";

static const char LDSCRIPT_PART5D[] =
"  {\n"
"    *(.sbss)\n"
"    *(.sbss.*)\n"
"    *(.gnu.linkonce.sb.*)\n"
"    *(.scommon)\n"
"  }\n"
"\n";

static const char LDSCRIPT_PART5E[] =
"  {\n"
"   PROVIDE(__bss_start = .);\n"
"   *(.bss)\n"
"   *(.bss.*)\n"
"   *(.gnu.linkonce.b.*)\n"
"   *(COMMON)\n"
"   PROVIDE(_edata = .);\n"
"   PROVIDE(edata = .);\n"
"   PROVIDE(_end = .);\n"
"  }\n"
#ifdef TARGET_FORMAT_EXE
"  . = DATA_SEGMENT_END( . ); \n"
#endif
"  /DISCARD/ : { *(.note.GNU-stack) }\n";

static const char LDSCRIPT_PART6[] =
"}\n";
