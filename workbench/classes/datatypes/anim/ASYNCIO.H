#ifndef ASYNCIO_H
#define ASYNCIO_H

/*
**
**  $VER: asyncio.h 1.10 (3.8.97)
**  anim.datatype 1.10
**
**  Async I/O includes for anim.datatype
**
**  Written 1996/97 by Roland 'Gizzy' Mainz
**  Based on asyncio.c by Martin Tailefer from 3.1_Examples1:AsyncIO/asyncio.h
**
*/

/* This structure is public only by necessity, don't muck with it yourself, or
 * you're looking for trouble
 */
typedef struct AsyncFile
{
    BPTR                  af_File;
    ULONG                 af_BlockSize;
    struct MsgPort       *af_Handler;
    APTR                  af_Offset;
    LONG                  af_BytesLeft;
    ULONG                 af_BufferSize;
    APTR                  af_Buffers[ 2 ];
    struct StandardPacket af_Packet;
    struct MsgPort        af_PacketPort;
    ULONG                 af_CurrentBuf;
    ULONG                 af_SeekOffset;
    UBYTE                 af_PacketPending;
} AsyncFile;


/*****************************************************************************/

#endif /* ASYNCIO_H */
