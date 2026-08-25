#ifndef BLUETOOTH_AES128_H
#define BLUETOOTH_AES128_H
/*
 * bluetooth.library: AES-128 block encryption (FIPS-197), used by the LE
 * Security Manager (SMP) for the c1/s1/CMAC/f4-f6 functions. Arrays are in
 * the FIPS convention (most-significant octet first).
 */
#include <exec/types.h>

void bAES128Encrypt(const UBYTE key[16], const UBYTE in[16], UBYTE out[16]);

#endif /* BLUETOOTH_AES128_H */
