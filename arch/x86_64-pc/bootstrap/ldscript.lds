FORCE_COMMON_ALLOCATION

SECTIONS
{
    . = 0x100000;

    .tables :
    {
	*(.aros.tables)
    }

    .bsstables :
    {
	*(.bss.aros.tables)
    }
    
    .text ALIGN(4096) :
    {
        *(.aros.startup)
        *(.text)
        *(.text.*)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t.*)
    } = 0x90909090

    .rodata ALIGN(0x10) :
    {
        *(.rodata)
        *(.rodata.*)
        *(.gnu.linkonce.r.*)
    }
    .rodata1 : { *(.rodata1) }

    .sdata2 : { *(.sdata2) *(.sdata2.*) *(.gnu.linkonce.s2.*) }
    .sbss2  : { *(.sbss2) *(.sbss2.*) *(.gnu.linkonce.sb2.*) }

    .data ALIGN(0x10) :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d.*)
    }

    .data1              : { *(.data1) }
    .eh_frame           : { KEEP (*(.eh_frame)) }
    .gcc_except_table   : { *(.gcc_except_table) }

    .sdata ALIGN(0x10) :
    {
        *(.sdata)
        *(.sdata.*)
        *(.gnu.linkonce.s.*)
    }

    .sbss ALIGN(0x10) :
    {
        *(.sbss)
        *(.sbss.*)
        *(.gnu.linkonce.sb.*)
        *(.scommon)
    }

    .bss ALIGN(0x10) :
    {
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }

}

