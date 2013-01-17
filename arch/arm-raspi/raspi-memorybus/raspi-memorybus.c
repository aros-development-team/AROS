/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <stdint.h>

#include <raspi_transitional/raspi.h>



IPTR ARMPhysToVCBus(IPTR address) {

    if(address<0x20000000) {
        address += 0xc0000000;      // SDRAM "C" alias, direct uncached
    }else{
        address += 0x7e000000;      // I/O peripherals
    }

    return address;
}

IPTR VCBusToARMPhys(IPTR address) {

    if(address<0xc0000000) {
        address -= 0xc0000000;      // SDRAM
    }else{
        address -= 0x7e000000;      // I/O peripherals
    }

    return address;
}

IPTR ARMPhysToVirtual(IPTR address) {

    return address;
}

IPTR ARMVirtualToPhys(IPTR address) {

    return address;
}

