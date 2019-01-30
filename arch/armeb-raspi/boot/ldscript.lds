FORCE_COMMON_ALLOCATION

SECTIONS
{
    . = 0x8000;
    
    __bootstrap_start = . ;
    
    .text :
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
    LONG(0x01000000)    /* DTOK = 1, please give us device tree */
    LONG(0x04000000)
    LONG(0x44544f4b)    /* DTOK */
    LONG(0x20000000)
    LONG(0x04000000)
    LONG(0x5250544c)
    }
}
