/*
 * Standard (reflected, 0xEDB88320) CRC-32, as used by GPT and zlib.
 * Processed a nibble at a time to keep the lookup table small.
 */

#include "partition_support.h"

static const ULONG crcTable[16] =
{
    0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
    0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
    0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
    0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
};

unsigned int Crc32_ComputeBuf(unsigned int inCrc32, const void *buf, unsigned int bufLen)
{
    ULONG crc = ~inCrc32;
    const UBYTE *p = buf;
    while (bufLen--)
    {
        crc ^= *p++;
        crc = (crc >> 4) ^ crcTable[crc & 15];
        crc = (crc >> 4) ^ crcTable[crc & 15];
    }
    return ~crc;
}
