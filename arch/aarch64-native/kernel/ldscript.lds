OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)

SECTIONS
{
    .text : {
        __text_start = .;
        *(SORT_NONE(.aros.init*))
        *(.text*)
        *(.gnu.linkonce.t.*)
        __text_end = .;
    }

    .intvecs : ALIGN(2048)
    {
        __intvecs_start = .;
        *(.intvecs)
        __intvecs_end = .;
    }

    .rodata : {
        *(.rodata*)
        *(.gnu.linkonce.r.*)
    }

    . = ALIGN(4096);

    .data : {
        *(.data*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(16);
        start_ctors = .; *(SORT(.ctor*)) QUAD(0)
        start_dtors = .; *(SORT(.dtor*)) QUAD(0)
    }

    . = ALIGN(4096);

    .bss : {
        *(.bss*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
    }
}
