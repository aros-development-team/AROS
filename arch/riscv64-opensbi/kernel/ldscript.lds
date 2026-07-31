/*
    Linker script for the opensbi-riscv64 early core.

    OpenSBI's fw_jump (and QEMU's -kernel loader) hand over to the payload
    at 0x80200000 on RV64, in S-mode.
*/

OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x80200000;

    .text : {
        __text_start = .;
        *(SORT_NONE(.aros.init*))
        *(.text*)                   /* wildcard for "gcc -ffunction-sections" */
        *(.gnu.linkonce.t.*)
        __text_end = .;
    }

    .rodata : {
        *(.rodata*)                 /* read-only data (ELF only) */
        *(.srodata*)
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

    .sdata : {
        __global_pointer$ = . + 0x800;
        *(.sdata*)
    }

    . = ALIGN(4096);

    .bss : {
        . = ALIGN(8);
        __bss_start = .;
        *(.sbss*)
        *(.bss*)                    /* wildcard for "gcc -fdata-sections" */
        *(.gnu.linkonce.b.*)
        *(COMMON)                   /* "common" variables */
        . = ALIGN(8);
        __bss_end = .;
    }

    __kernel_end = .;
}
