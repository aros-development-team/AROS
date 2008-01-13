extern unsigned long __bss_start;
extern unsigned long _end;
register void * global_data asm("r29");

int bar()
{
    return 1; 
   
}

void putc(char c)
{
    volatile char *uart = (char*)0xef600300;

    while(!(uart[5] & 0x40));
    
    uart[0] = c;
}

void puts(char *str)
{
    while (*str)
    {
        if (*str == '\n')
            putc('\r');
        putc(*str++);
    }
}

int __attribute__((section(".aros.startup"))) bootstrap() 
{
    unsigned long *ptr = &__bss_start;
    
    while(ptr < &_end)
        *ptr++ = 0;
    
    puts("[BOOT] AROS SAM440 Bootstrap\n");
    
    return 0;
}
