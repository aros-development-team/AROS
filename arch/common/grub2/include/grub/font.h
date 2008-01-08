/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007  Free Software Foundation, Inc.
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

#ifndef GRUB_FONT_HEADER
#define GRUB_FONT_HEADER	1

#include <grub/types.h>

#define GRUB_FONT_MAGIC	"PPF\x7f"

struct grub_font_glyph
{
  /* Glyph width in pixels.  */
  grub_uint8_t width;
  
  /* Glyph height in pixels.  */
  grub_uint8_t height;
  
  /* Glyph width in characters.  */
  grub_uint8_t char_width;
  
  /* Glyph baseline position in pixels (from up).  */
  grub_uint8_t baseline;
  
  /* Glyph bitmap data array of bytes in ((width + 7) / 8) * height.
     Bitmap is formulated by height scanlines, each scanline having
     width number of pixels. Pixels are coded as bits, value 1 meaning
     of opaque pixel and 0 is transparent. If width does not fit byte
     boundary, it will be padded with 0 to make it fit.  */
  grub_uint8_t bitmap[32];
};

typedef struct grub_font_glyph *grub_font_glyph_t;

int grub_font_get_glyph (grub_uint32_t code,
			 grub_font_glyph_t glyph);

#endif /* ! GRUB_FONT_HEADER */
