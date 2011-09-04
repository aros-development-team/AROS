/*
 * Probe a memory region from start to end.
 * This is a non-destructive test.
*/ 
unsigned long RamCheck(unsigned long start, unsigned long limit)
{
    unsigned int volatile tmp;
    unsigned int volatile *ptr = (void *)((start + 3) & ~3);

    /* No memory info from bios, do a scan */
    do
    {
        tmp = *ptr;
        *ptr = 0xdeadbeef;
        if (*ptr != 0xdeadbeef)
            break;
        *ptr = tmp;
        ptr += 4;

    } while ((unsigned long)ptr < limit);

    return (unsigned long)ptr;
}
