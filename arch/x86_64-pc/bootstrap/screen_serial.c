static unsigned char __inb(addr)
{
    unsigned char tmp;    
    asm volatile ("inb %w1,%b0":"=a"(tmp):"Nd"(addr):"memory");
    return tmp;
}

int serPutC(unsigned char data) 
{
    while (!(__inb(0x3F8 + 0x05) & 0x40));
    asm volatile ("outb %b0,%w1"::"a"(data),"Nd"(0x3F8));
    return __inb(0x3F8 + 0x05) & (0x02|0x04|0x08|0x10);
}
