#include "debug.h"

#ifdef DEBUG
void dumpmem_pciusb(void *mem, unsigned long int len)
{
    unsigned char *p;

    if(!mem || !len) {
        return;
    }

    p = (unsigned char *) mem;

#if !defined(AROS_USE_LOGRES)
    KPrintF("\n");

    do {
        unsigned char b, c, str[17];

        for(b = 0; b < 16; b++) {
            c = *p++;
            str[b] = ((c >= ' ') && (c <= 'z')) ? c : '.';
            str[b + 1] = 0;
            KPrintF("%02lx ", c);
            if(--len == 0) break;
        }

        while(++b < 16) {
            KPrintF("   ");
        }

        KPrintF("  %s\n", str);
    } while(len);

    KPrintF("\n\n");
#else
#endif
}

#endif /* DEBUG */
