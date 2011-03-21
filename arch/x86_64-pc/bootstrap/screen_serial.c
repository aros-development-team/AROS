static unsigned short base = 0x03F8;

/* Standard base addresses for four PC AT serial ports */
static unsigned short standard_ports[] = {
    0x03F8,
    0x02F8,
    0x03E8,
    0x02E8
};

static unsigned char __inb(addr)
{
    unsigned char tmp;    
    asm volatile ("inb %w1,%b0":"=a"(tmp):"Nd"(addr):"memory");
    return tmp;
}

void initSerial(char *opts)
{
    /* Command line option format: debug=serial[:N], where N - port number (1 - 4) */
    if (opts[0] == ':')
    {
    	unsigned char portnum = opts[1] - '1';
    	
    	if (portnum < 4)
	    base = standard_ports[portnum];
    }
    /* TODO: set baud rate */
}

int serPutC(unsigned char data) 
{
    while (!(__inb(base + 0x05) & 0x40));
    asm volatile ("outb %b0,%w1"::"a"(data),"Nd"(base));
    return __inb(base + 0x05) & (0x02|0x04|0x08|0x10);
}
