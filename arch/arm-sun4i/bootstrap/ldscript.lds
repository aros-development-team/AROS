MEMORY {
    boot0  (rwx)  : org = 0x00000000 , l = 4K
    invalid (rwx) : org = 0xdead0000 , l = 0x0
}

SECTIONS 
{
    .boot0 : {
        *(.bootstrap)
    } >boot0

    .text : { *(.text*) }

    /* This should cause angry complaints if .data exists
    * We don't want .bss, and we don't need it.
    */
    .bss : {
        *(.bss)
    } >invalid

    /* This should cause angry complaints if .data exists
    * We don't want .data, and we don't need it.
    */
    .data : {
        *(.data)
    } >invalid

}
