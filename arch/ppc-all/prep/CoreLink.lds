SECTIONS
{
    .text : {
	*(.text .text.* .fixup .rodata .rodata.* .data .data.* .sdata*)
    }

    _EDATA = .;
    .bss : {
	*(.bss)
	*(.bss.*)
	*(COMMON)
    }
    _END = .;
}
