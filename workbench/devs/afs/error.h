#ifndef ERROR_H
#define ERROR_H

enum {
	ERR_NONE,
	ERR_MEMORY,							// init error codes
	ERR_IOPORT,
	ERR_DEVICE,
	ERR_DOSTYPE,
	ERR_DOSENTRY,
	ERR_DISKNOTVALID,
	ERR_WRONG_DATA_BLOCK,
	ERR_CHECKSUM,						// block errors
	ERR_MISSING_BITMAP_BLOCKS,
	ERR_BLOCKTYPE,
	ERR_READWRITE,
	ERR_ALREADY_PRINTED,
	ERR_UNKNOWN
};

void showText(char *, ...);
void showError(ULONG, ...);

#endif
