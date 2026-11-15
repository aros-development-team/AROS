/***************************************************************************

 BetterString.mcc - A better String gadget MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005-2018 BetterString.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 BetterString class Support Site:  http://www.sf.net/projects/bstring-mcc/

 $Id$

***************************************************************************/

/*
 * Reject direct program execution of this class library. On AmigaOS3 the
 * assembler entry returns 20; the C _start entry returns RETURN_FAIL.
 * Neither path initializes the class or opens a preferences interface.
 * Keep this entry first in the linked executable as required by the build.
 * This source alone does not prove the final binary's entry placement.
 */

#if defined(__amigaos3__)
asm(".text\n\
     .even\n\
     .globl _start\n\
    _start:\n\
     moveq #20,d0\n\
     rts");
#else
LONG _start(void)
{
  return RETURN_FAIL;
}
#endif
