FORCE_COMMON_ALLOCATION

SECTIONS
{
    .text : {
        __text_start = .;
	*(.aros.init)
        *(.text*)                   /* wildcard for "gcc -ffunction-sections" */
        *(.gnu.linkonce.t.*)        /* C++ templates? */
        __text_end = .;
    }

    __intvecs_start = .;
    .intvecs :
    {
        *(.intvecs)
    }
    __intvecs_end = .;

    .rodata : {
        *(.rodata*)                 /* read-only data (ELF only) */
        *(.gnu.linkonce.r.*)
        
    }

    . = ALIGN(4096);

    .data : {
        *(.data*)                   /* wildcard for "gcc -fdata-sections" */
        *(.gnu.linkonce.d.*)
        . = ALIGN(16);
        start_ctors = .; *(SORT(.ctor*)) QUAD(0)
        start_dtors = .; *(SORT(.dtor*)) QUAD(0)
    }

    . = ALIGN(4096);

    .bss : {
        *(.bss*)                    /* wildcard for "gcc -fdata-sections" */
        *(.gnu.linkonce.b.*)
        *(COMMON)                   /* "common" variables */
    }
}
