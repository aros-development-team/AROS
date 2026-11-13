/*

Copyright (C) 2010 Neil Cafferkey

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.

*/

#ifndef ENCRYPTION_PROTOS_H
#define ENCRYPTION_PROTOS_H


#include "device.h"

VOID WriteClearFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID EncryptWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID WriteWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID EncryptTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID WriteTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID EncryptCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID WriteCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL ReadClearFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL DecryptWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL ReadWEPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL DecryptTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL ReadTKIPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL DecryptCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
BOOL ReadCCMPFragment(struct DevUnit *unit, UBYTE *header, UBYTE *data,
   UWORD *size, UBYTE *buffer, struct DevBase *base);
VOID TKIPEncryptFrame(struct DevUnit *unit, const UBYTE *header,
   UBYTE *data, UWORD size, UBYTE *buffer, struct DevBase *base);
BOOL TKIPDecryptFrame(struct DevUnit *unit, const UBYTE *header,
   UBYTE *data, UWORD size, UBYTE *buffer, UWORD key_no,
   struct DevBase *base);
VOID RC4Encrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, UBYTE *seed, struct DevBase *base);
BOOL RC4Decrypt(struct DevUnit *unit, const UBYTE *data, UWORD size,
   UBYTE *buffer, UBYTE *seed, struct DevBase *base);


#endif


