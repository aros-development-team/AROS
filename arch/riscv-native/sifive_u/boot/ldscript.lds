SECTIONS
{
    . = 0x80000000;

    __bootstrap_start = . ;

    .text ALIGN(16) :
    {
        *(.aros.startup)
        *(.text)
        *(.text.*)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t.*)
    }

    .rodata ALIGN(0x1000) :
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }
    .rodata1 : { *(.rodata1) }

    .sdata2 : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }
    .sbss2  : { *(.sbss2) *(.sbss2.*) *(.gnu.linkonce.sb2.*) }

    .data ALIGN(0x1000) :
    {
        *core.bin.o(.data)
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
    }

    .data1              : { *(.data1) }
    .eh_frame           : { KEEP (*(.eh_frame)) }
    .gcc_except_table   : { *(.gcc_except_table) }

    .sdata ALIGN(0x1000) :
    {
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
    }

    __bss_start = . ;

    .sbss ALIGN(0x1000) :
    {
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
    }

    .bss ALIGN(0x1000) :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }

    _end = . ;

    __bootstrap_end = . ;

    .trailer ALIGN(4) :
    {
    LONG(0)
    LONG(0)
    LONG(0x00000001)    /* DTOK = 1, please give us device tree */
    LONG(0x00000004)
    LONG(0x4b4f5444)    /* DTOK */
    LONG(0x00000020)
    LONG(0x00000004)
    LONG(0x4c545052)
    }
}
