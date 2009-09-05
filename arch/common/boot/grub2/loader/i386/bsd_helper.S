/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2005,2006,2007,2008, 2009 Free Software Foundation, Inc.
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

	.p2align	2


	.code32

/*
 * Use cdecl calling convention for *BSD kernels.
 */

FUNCTION(grub_unix_real_boot)

	/* Interrupts should be disabled.  */
        cli

	/* Discard `grub_unix_real_boot' return address.  */
        popl    %eax

        /* Fetch `entry' address ...  */
        popl	%eax

        /*
         * ... and put our return address in its place. The kernel will
         * ignore it, but it expects %esp to point to it.
         */
        call	*%eax
