/* cache.S - Flush the processor cache for a specific region.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/symbol.h>

        .file   "cache.S"

        .text

/*
 * void grub_arch_sync_caches (void *address, grub_size_t len)
 */
FUNCTION(grub_arch_sync_caches)
        save            %o6,    -0xC,  %o6      ! Get a new register window,
                                                ! reserve space on stack for
                                                ! %i0, %i1, %i2
        brz,pn          %i0,    return          ! Return if address == 0.
         nop
        brz,pn          %i1,    return          ! Return if len == 0.
         clr             %i2                    ! index = 0.
loop:   flush           %i0 + %i2               ! Flush address + index.
        cmp             %i1,    %i2             ! Compare len & index .
        bpos,a,pt       %xcc,   loop            ! If len > index, loop.
         add            %i2,    8,      %i2     ! Go to next doubleword.
return: ret                                     ! Restore caller's register
         restore                                ! window and return.

