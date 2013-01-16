MEMORY {
  valid  (rwx)    : org = 0x00000000 , l = 2M
  invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS {

    . = 0x4000;
    SysBase = .;

    . = 0x6000;
    .bootheap ALIGN(0x1000) : {
        bootheap_start = .;
        *(.heap)
        . = 0x1000;
        bootheap_end = .;
    }

    . = 0x8000;
    .kernel : {
        *(.aros.startup)
        *(.text .rodata .rodata.*)
         kernel_end = .;   
    } >valid

    /* This should cause angry complaints if .data exists
     * We don't want .data, and we don't need it.
    */
    .data : { *(.data) } >invalid

    /* This should cause angry complaints if .data exists
     * We don't want .bss, and we don't need it.
    */
    .bss : { *(.bss) } >invalid 
    _end = . ;
}
