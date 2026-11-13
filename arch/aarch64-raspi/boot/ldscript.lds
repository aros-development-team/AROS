/*
 * AArch64 linker script for AROS Raspberry Pi 3 boot image
 *
 * RPi3 firmware loads kernel8.img at 0x80000 in AArch64 mode.
 */
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)

SECTIONS
{
    . = 0x80000;

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

    .trailer ALIGN(8) :
    {
    QUAD(0)
    QUAD(0)
    LONG(0x00000001)    /* DTOK = 1, please give us device tree */
    LONG(0x00000004)
    LONG(0x4b4f5444)    /* DTOK */
    LONG(0x00000020)
    LONG(0x00000004)
    LONG(0x4c545052)
    }
}
