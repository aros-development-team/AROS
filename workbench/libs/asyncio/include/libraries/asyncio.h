#ifndef LIBRARIES_ASYNCIO_H
#define LIBRARIES_ASYNCIO_H


/*****************************************************************************/


#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_PORTS_H
#include <exec/ports.h>
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#ifndef DOS_DOSEXTENS_H
#include <dos/dosextens.h>
#endif


/*****************************************************************************/


/* This structure is public only by necessity, don't muck with it yourself, or
 * you're looking for trouble
 */
typedef struct AsyncFile
{
	BPTR			af_File;
	ULONG			af_BlockSize;
	struct MsgPort		*af_Handler;
	UBYTE			*af_Offset;
	LONG			af_BytesLeft;
	ULONG			af_BufferSize;
	UBYTE			*af_Buffers[2];
	struct StandardPacket	af_Packet;
	struct MsgPort		af_PacketPort;
	ULONG			af_CurrentBuf;
	ULONG			af_SeekOffset;
#ifdef ASIO_NOEXTERNALS
	struct ExecBase		*af_SysBase;
	struct DosLibrary	*af_DOSBase;
#endif
	UBYTE			af_PacketPending;
	UBYTE			af_ReadMode;
	UBYTE			af_CloseFH;
	UBYTE			af_SeekPastEOF;
	ULONG			af_LastRes1;
	ULONG			af_LastBytesLeft;
} AsyncFile;


/*****************************************************************************/


typedef enum OpenModes
{
	MODE_READ,	/* read an existing file                             */
	MODE_WRITE,	/* create a new file, delete existing file if needed */
	MODE_APPEND	/* append to end of existing file, or create new     */
} OpenModes;


typedef enum SeekModes
{
	MODE_START = -1,	/* relative to start of file         */
	MODE_CURRENT,		/* relative to current file position */
	MODE_END		/* relative to end of file           */
} SeekModes;


/*****************************************************************************/


#if defined(__SASC) && defined(ASIO_SHARED_LIB)
extern long __asiolibversion;  /* Minimum version of asyncio.library */
#endif


#endif /* ASYNCIO_H */
