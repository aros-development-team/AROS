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
        . = ALIGN(4096);            /* section permission boundary (MMU) */
        __text_end = .;
    }

    .rodata : {
        __rodata_start = .;
        *(.rodata*)                 /* read-only data (ELF only) */
        *(.srodata*)
        *(.gnu.linkonce.r.*)
        *(.eh_frame_hdr)
        *(.eh_frame)
        . = ALIGN(4096);            /* section permission boundary (MMU) */
        __rodata_end = .;
    }

    .data : {
        __data_start = .;
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
