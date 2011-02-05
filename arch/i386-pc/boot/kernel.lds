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
    
    . = ALIGN(64);
    
    .rodata : {
        __rodata_start = .;
        
        *(.rodata*)                 /* read-only data (ELF only) */
        *(.gnu.linkonce.r.*)
        
        __rodata_end = .;
    }
    
    . = ALIGN(4096);
    
    .data : {
        __data_start = .;
        
        *(.data*)                   /* wildcard for "gcc -fdata-sections" */
        *(.gnu.linkonce.d.*)
        . = ALIGN(16);
        start_ctors = .; *(SORT(.ctor*)) QUAD(0)
        start_dtors = .; *(SORT(.dtor*)) QUAD(0)
        
        __data_end = .;
       
    }

    .bss : {
       . = ALIGN(4096);

        __bss_start = .;
        PROVIDE(_edata = .);
        *(.bss*)                    /* wildcard for "gcc -fdata-sections" */
        *(.gnu.linkonce.b.*)
        *(COMMON)                   /* "common" variables */
        __bss_end = .;
        PROVIDE(_end = .);
    }

    
}
