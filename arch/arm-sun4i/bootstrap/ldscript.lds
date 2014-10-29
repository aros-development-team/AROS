FORCE_COMMON_ALLOCATION

SECTIONS {
    . = 0x30;
    
    .text : {
        *(.aros.startup)
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4) : {
        *(.rodata)
        *(.rodata.*)
    }

}
