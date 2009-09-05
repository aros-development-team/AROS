/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2007,2008,2009  Free Software Foundation, Inc.
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
#include <grub/video.h>

/* Forward declaration of opaque structure grub_font.
   Users only pass struct grub_font pointers to the font module functions,
   and do not have knowledge of the structure contents.  */
struct grub_font;

/* Font type used to access font functions.  */
typedef struct grub_font *grub_font_t;

struct grub_font_node
{
  struct grub_font_node *next;
  grub_font_t value;
};

/* Global font registry.  */
extern struct grub_font_node *grub_font_list;

struct grub_font_glyph
{
  /* Reference to the font this glyph belongs to.  */
  grub_font_t font;

  /* Glyph bitmap width in pixels.  */
  grub_uint16_t width;

  /* Glyph bitmap height in pixels.  */
  grub_uint16_t height;

  /* Glyph bitmap x offset in pixels.  Add to screen coordinate.  */
  grub_int16_t offset_x;

  /* Glyph bitmap y offset in pixels.  Subtract from screen coordinate.  */
  grub_int16_t offset_y;

  /* Number of pixels to advance to start the next character.  */
  grub_uint16_t device_width;

  /* Row-major order, packed bits (no padding; rows can break within a byte).
     The length of the array is (width * height + 7) / 8.  Within a
     byte, the most significant bit is the first (leftmost/uppermost) pixel.
     Pixels are coded as bits, value 1 meaning of opaque pixel and 0 is
     transparent.  If the length of the array does not fit byte boundary, it
     will be padded with 0 bits to make it fit.  */
  grub_uint8_t bitmap[0];
};

/* Initialize the font loader.
   Must be called before any fonts are loaded or used.  */
void grub_font_loader_init (void);

/* Load a font and add it to the beginning of the global font list.
   Returns: 0 upon success; nonzero upon failure.  */
int grub_font_load (const char *filename);

/* Get the font that has the specified name.  Font names are in the form
   "Family Name Bold Italic 14", where Bold and Italic are optional.
   If no font matches the name specified, the most recently loaded font
   is returned as a fallback.  */
grub_font_t grub_font_get (const char *font_name);

const char *grub_font_get_name (grub_font_t font);

int grub_font_get_max_char_width (grub_font_t font);

int grub_font_get_max_char_height (grub_font_t font);

int grub_font_get_ascent (grub_font_t font);

int grub_font_get_descent (grub_font_t font);

int grub_font_get_leading (grub_font_t font);

int grub_font_get_height (grub_font_t font);

int grub_font_get_string_width (grub_font_t font, const char *str);

struct grub_font_glyph *grub_font_get_glyph (grub_font_t font,
                                             grub_uint32_t code);

struct grub_font_glyph *grub_font_get_glyph_with_fallback (grub_font_t font,
                                                           grub_uint32_t code);

grub_err_t grub_font_draw_glyph (struct grub_font_glyph *glyph,
                                        grub_video_color_t color,
                                        int left_x, int baseline_y);

grub_err_t grub_font_draw_string (const char *str, grub_font_t font,
                                  grub_video_color_t color,
                                  int left_x, int baseline_y);

#endif /* ! GRUB_FONT_HEADER */
