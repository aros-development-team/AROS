FORCE_COMMON_ALLOCATION
SECTIONS
{
    .text : {
        Exec_Start = .;

        *(.text)
        *(.text.*)
        *(.fixup)
        *(.rodata)
    }

    .data : {
        *(.data)
        *(.data.*)

        Exec_End = .;
    }

    .bss : {
        *(.bss)
        *(COMMON)
    }
}
