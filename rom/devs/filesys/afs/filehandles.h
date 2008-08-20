#ifndef FILEHANDLES_H
#define FILEHANDLES_H

/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id: filehandles.h 28116 2008-03-24 20:17:32Z neil $
*/

#include "os.h"

struct Position
{
	ULONG block;		// current extensionblock
	UWORD filekey;		// long offset within header/extension block of
	                  // next/current datablock to read
	UWORD byte;			// bytes left in a block to read
	ULONG offset;		// offset in bytes within a file
};

struct AfsHandle
{
	struct AfsHandle *next;
	ULONG header_block;
	ULONG mode;
	struct Position current;
	ULONG filesize;	// size of file in bytes
	ULONG dirpos;		// current position for ExamineAll
	struct Volume *volume;	// the volume the handle refers to
	struct DosList *volumenode;
};

#endif

