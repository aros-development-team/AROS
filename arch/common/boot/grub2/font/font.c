/* font.c - Font API and font file loader.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/bufio.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/font.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/video.h>
#include <grub/bitmap.h>

#ifndef FONT_DEBUG
#define FONT_DEBUG 0
#endif

struct char_index_entry
{
  grub_uint32_t code;
  grub_uint8_t storage_flags;
  grub_uint32_t offset;

  /* Glyph if loaded, or NULL otherwise.  */
  struct grub_font_glyph *glyph;
};

#define FONT_WEIGHT_NORMAL 100
#define FONT_WEIGHT_BOLD 200

struct grub_font
{
  char *name;
  grub_file_t file;
  char *family;
  short point_size;
  short weight;
  short max_char_width;
  short max_char_height;
  short ascent;
  short descent;
  short leading;
  grub_uint32_t num_chars;
  struct char_index_entry *char_index;
};

/* Definition of font registry.  */
struct grub_font_node *grub_font_list;

static int register_font (grub_font_t font);
static void font_init (grub_font_t font);
static void free_font (grub_font_t font);
static void remove_font (grub_font_t font);

struct font_file_section
{
  /* The file this section is in.  */
  grub_file_t file;

  /* FOURCC name of the section.  */
  char name[4];

  /* Length of the section contents.  */
  grub_uint32_t length;

  /* Set by open_section() on EOF.  */
  int eof;
};

/* Font file format constants.  */
static const char pff2_magic[4] = { 'P', 'F', 'F', '2' };
static const char section_names_file[4] = { 'F', 'I', 'L', 'E' };
static const char section_names_font_name[4] = { 'N', 'A', 'M', 'E' };
static const char section_names_point_size[4] = { 'P', 'T', 'S', 'Z' };
static const char section_names_weight[4] = { 'W', 'E', 'I', 'G' };
static const char section_names_max_char_width[4] = { 'M', 'A', 'X', 'W' };
static const char section_names_max_char_height[4] = { 'M', 'A', 'X', 'H' };
static const char section_names_ascent[4] = { 'A', 'S', 'C', 'E' };
static const char section_names_descent[4] = { 'D', 'E', 'S', 'C' };
static const char section_names_char_index[4] = { 'C', 'H', 'I', 'X' };
static const char section_names_data[4] = { 'D', 'A', 'T', 'A' };

/* Replace unknown glyphs with a rounded question mark.  */
static grub_uint8_t unknown_glyph_bitmap[] =
{
  /*       76543210 */
  0x7C, /*  ooooo   */
  0x82, /* o     o  */
  0xBA, /* o ooo o  */
  0xAA, /* o o o o  */
  0xAA, /* o o o o  */
  0x8A, /* o   o o  */
  0x9A, /* o  oo o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x92, /* o  o  o  */
  0x82, /* o     o  */
  0x92, /* o  o  o  */
  0x82, /* o     o  */
  0x7C, /*  ooooo   */
  0x00  /*          */
};

/* The "unknown glyph" glyph, used as a last resort.  */
static struct grub_font_glyph *unknown_glyph;

/* The font structure used when no other font is loaded.  This functions
   as a "Null Object" pattern, so that code everywhere does not have to
   check for a NULL grub_font_t to avoid dereferencing a null pointer.  */
static struct grub_font null_font;

/* Flag to ensure module is initialized only once.  */
static grub_uint8_t font_loader_initialized;

void
grub_font_loader_init (void)
{
  /* Only initialize font loader once.  */
  if (font_loader_initialized)
    return;

  /* Make glyph for unknown glyph.  */
  unknown_glyph = grub_malloc(sizeof(struct grub_font_glyph)
                              + sizeof(unknown_glyph_bitmap));
  if (! unknown_glyph)
    return;

  unknown_glyph->width = 8;
  unknown_glyph->height = 16;
  unknown_glyph->offset_x = 0;
  unknown_glyph->offset_y = -3;
  unknown_glyph->device_width = 8;
  grub_memcpy(unknown_glyph->bitmap,
              unknown_glyph_bitmap, sizeof(unknown_glyph_bitmap));

  /* Initialize the null font.  */
  font_init (&null_font);
  null_font.name = "<No Font>";
  null_font.ascent = unknown_glyph->height-3;
  null_font.descent = 3;
  null_font.max_char_width = unknown_glyph->width;
  null_font.max_char_height = unknown_glyph->height;

  font_loader_initialized = 1;
}

/* Initialize the font object with initial default values.  */
static void
font_init (grub_font_t font)
{
  font->name = 0;
  font->file = 0;
  font->family = 0;
  font->point_size = 0;
  font->weight = 0;

  /* Default leading value, not in font file yet.  */
  font->leading = 1;

  font->max_char_width = 0;
  font->max_char_height = 0;
  font->ascent = 0;
  font->descent = 0;
  font->num_chars = 0;
  font->char_index = 0;
}

/* Open the next section in the file.

   On success, the section name is stored in section->name and the length in
   section->length, and 0 is returned.  On failure, 1 is returned and
   grub_errno is set appropriately with an error message.

   If 1 is returned due to being at the end of the file, then section->eof is
   set to 1; otherwise, section->eof is set to 0.  */
static int
open_section (grub_file_t file, struct font_file_section *section)
{
  grub_ssize_t retval;
  grub_uint32_t raw_length;

  section->file = file;
  section->eof = 0;

  /* Read the FOURCC section name.  */
  retval = grub_file_read (file, section->name, 4);
  if (retval >= 0 && retval < 4)
    {
      /* EOF encountered.  */
      section->eof = 1;
      return 1;
    }
  else if (retval < 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font format error: can't read section name");
      return 1;
    }

  /* Read the big-endian 32-bit section length.  */
  retval = grub_file_read (file, &raw_length, 4);
  if (retval >= 0 && retval < 4)
    {
      /* EOF encountered.  */
      section->eof = 1;
      return 1;
    }
  else if (retval < 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font format error: can't read section length");
      return 1;
    }

  /* Convert byte-order and store in *length.  */
  section->length = grub_be_to_cpu32 (raw_length);

  return 0;
}

/* Size in bytes of each character index (CHIX section)
   entry in the font file.  */
#define FONT_CHAR_INDEX_ENTRY_SIZE (4 + 1 + 4)

/* Load the character index (CHIX) section contents from the font file.  This
   presumes that the position of FILE is positioned immediately after the
   section length for the CHIX section (i.e., at the start of the section
   contents).  Returns 0 upon success, nonzero for failure (in which case
   grub_errno is set appropriately).  */
static int
load_font_index (grub_file_t file, grub_uint32_t sect_length, struct
                 grub_font *font)
{
  unsigned i;
  grub_uint32_t last_code;

#if FONT_DEBUG >= 2
  grub_printf("load_font_index(sect_length=%d)\n", sect_length);
#endif

  /* Sanity check: ensure section length is divisible by the entry size.  */
  if ((sect_length % FONT_CHAR_INDEX_ENTRY_SIZE) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font file format error: character index length %d "
                  "is not a multiple of the entry size %d",
                  sect_length, FONT_CHAR_INDEX_ENTRY_SIZE);
      return 1;
    }

  /* Calculate the number of characters.  */
  font->num_chars = sect_length / FONT_CHAR_INDEX_ENTRY_SIZE;

  /* Allocate the character index array.  */
  font->char_index = grub_malloc (font->num_chars
                                  * sizeof (struct char_index_entry));
  if (! font->char_index)
    return 1;

#if FONT_DEBUG >= 2
  grub_printf("num_chars=%d)\n", font->num_chars);
#endif

  last_code = 0;

  /* Load the character index data from the file.  */
  for (i = 0; i < font->num_chars; i++)
    {
      struct char_index_entry *entry = &font->char_index[i];

      /* Read code point value; convert to native byte order.  */
      if (grub_file_read (file, &entry->code, 4) != 4)
        return 1;
      entry->code = grub_be_to_cpu32 (entry->code);

      /* Verify that characters are in ascending order.  */
      if (i != 0 && entry->code <= last_code)
        {
          grub_error (GRUB_ERR_BAD_FONT,
                      "Font characters not in ascending order: %u <= %u",
                      entry->code, last_code);
          return 1;
        }

      last_code = entry->code;

      /* Read storage flags byte.  */
      if (grub_file_read (file, &entry->storage_flags, 1) != 1)
        return 1;

      /* Read glyph data offset; convert to native byte order.  */
      if (grub_file_read (file, &entry->offset, 4) != 4)
        return 1;
      entry->offset = grub_be_to_cpu32 (entry->offset);

      /* No glyph loaded.  Will be loaded on demand and cached thereafter.  */
      entry->glyph = 0;

#if FONT_DEBUG >= 5
      /* Print the 1st 10 characters.  */
      if (i < 10)
        grub_printf("c=%d o=%d\n", entry->code, entry->offset);
#endif
    }

  return 0;
}

/* Read the contents of the specified section as a string, which is
   allocated on the heap.  Returns 0 if there is an error.  */
static char *
read_section_as_string (struct font_file_section *section)
{
  char *str;
  grub_ssize_t ret;

  str = grub_malloc (section->length + 1);
  if (! str)
    return 0;

  ret = grub_file_read (section->file, str, section->length);
  if (ret < 0 || ret != (grub_ssize_t) section->length)
    {
      grub_free (str);
      return 0;
    }

  str[section->length] = '\0';
  return str;
}

/* Read the contents of the current section as a 16-bit integer value,
   which is stored into *VALUE.
   Returns 0 upon success, nonzero upon failure.  */
static int
read_section_as_short (struct font_file_section *section, grub_int16_t *value)
{
  grub_uint16_t raw_value;

  if (section->length != 2)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font file format error: section %c%c%c%c length "
                  "is %d but should be 2",
                  section->name[0], section->name[1],
                  section->name[2], section->name[3],
                  section->length);
      return 1;
    }
  if (grub_file_read (section->file, &raw_value, 2) != 2)
    return 1;

  *value = grub_be_to_cpu16 (raw_value);
  return 0;
}

/* Load a font and add it to the beginning of the global font list.
   Returns 0 upon success, nonzero upon failure.  */
int
grub_font_load (const char *filename)
{
  grub_file_t file = 0;
  struct font_file_section section;
  char magic[4];
  grub_font_t font = 0;

#if FONT_DEBUG >= 1
  grub_printf("add_font(%s)\n", filename);
#endif

  file = grub_buffile_open (filename, 1024);
  if (!file)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("file opened\n");
#endif

  /* Read the FILE section.  It indicates the file format.  */
  if (open_section (file, &section) != 0)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("opened FILE section\n");
#endif
  if (grub_memcmp (section.name, section_names_file, 4) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font file format error: 1st section must be FILE");
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("section name ok\n");
#endif
  if (section.length != 4)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Font file format error (file type ID length is %d "
                  "but should be 4)", section.length);
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("section length ok\n");
#endif
  /* Check the file format type code.  */
  if (grub_file_read (file, magic, 4) != 4)
    goto fail;

#if FONT_DEBUG >= 3
  grub_printf("read magic ok\n");
#endif

  if (grub_memcmp (magic, pff2_magic, 4) != 0)
    {
      grub_error (GRUB_ERR_BAD_FONT, "Invalid font magic %x %x %x %x",
                  magic[0], magic[1], magic[2], magic[3]);
      goto fail;
    }

#if FONT_DEBUG >= 3
  grub_printf("compare magic ok\n");
#endif

  /* Allocate the font object.  */
  font = (grub_font_t) grub_malloc (sizeof (struct grub_font));
  if (! font)
    goto fail;

  font_init (font);
  font->file = file;

#if FONT_DEBUG >= 3
  grub_printf("allocate font ok; loading font info\n");
#endif

  /* Load the font information.  */
  while (1)
    {
      if (open_section (file, &section) != 0)
        {
          if (section.eof)
            break;              /* Done reading the font file.  */
          else
            goto fail;
        }

#if FONT_DEBUG >= 2
      grub_printf("opened section %c%c%c%c ok\n",
                  section.name[0], section.name[1],
                  section.name[2], section.name[3]);
#endif

      if (grub_memcmp (section.name, section_names_font_name, 4) == 0)
        {
          font->name = read_section_as_string (&section);
          if (!font->name)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_point_size, 4) == 0)
        {
          if (read_section_as_short (&section, &font->point_size) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_weight, 4) == 0)
        {
          char *wt;
          wt = read_section_as_string (&section);
          if (!wt)
            continue;
          /* Convert the weight string 'normal' or 'bold' into a number.  */
          if (grub_strcmp (wt, "normal") == 0)
            font->weight = FONT_WEIGHT_NORMAL;
          else if (grub_strcmp (wt, "bold") == 0)
            font->weight = FONT_WEIGHT_BOLD;
          grub_free (wt);
        }
      else if (grub_memcmp (section.name, section_names_max_char_width, 4) == 0)
        {
          if (read_section_as_short (&section, &font->max_char_width) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_max_char_height, 4) == 0)
        {
          if (read_section_as_short (&section, &font->max_char_height) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_ascent, 4) == 0)
        {
          if (read_section_as_short (&section, &font->ascent) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_descent, 4) == 0)
        {
          if (read_section_as_short (&section, &font->descent) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_char_index, 4) == 0)
        {
          if (load_font_index (file, section.length, font) != 0)
            goto fail;
        }
      else if (grub_memcmp (section.name, section_names_data, 4) == 0)
        {
          /* When the DATA section marker is reached, we stop reading.  */
          break;
        }
      else
        {
          /* Unhandled section type, simply skip past it.  */
#if FONT_DEBUG >= 3
          grub_printf("Unhandled section type, skipping.\n");
#endif
          grub_off_t section_end = grub_file_tell (file) + section.length;
          if ((int) grub_file_seek (file, section_end) == -1)
            goto fail;
        }
    }

  if (! font->name)
    {
      grub_printf ("Note: Font has no name.\n");
      font->name = grub_strdup ("Unknown");
    }

#if FONT_DEBUG >= 1
  grub_printf ("Loaded font `%s'.\n"
               "Ascent=%d Descent=%d MaxW=%d MaxH=%d Number of characters=%d.\n",
               font->name,
               font->ascent, font->descent,
               font->max_char_width, font->max_char_height,
               font->num_chars);
#endif

  if (font->max_char_width == 0
      || font->max_char_height == 0
      || font->num_chars == 0
      || font->char_index == 0
      || font->ascent == 0
      || font->descent == 0)
    {
      grub_error (GRUB_ERR_BAD_FONT,
                  "Invalid font file: missing some required data.");
      goto fail;
    }

  /* Add the font to the global font registry.  */
  if (register_font (font) != 0)
    goto fail;

  return 0;

fail:
  free_font (font);
  return 1;
}

/* Read a 16-bit big-endian integer from FILE, convert it to native byte
   order, and store it in *VALUE.
   Returns 0 on success, 1 on failure.  */
static int
read_be_uint16 (grub_file_t file, grub_uint16_t * value)
{
  if (grub_file_read (file, value, 2) != 2)
    return 1;
  *value = grub_be_to_cpu16 (*value);
  return 0;
}

static int
read_be_int16 (grub_file_t file, grub_int16_t * value)
{
  /* For the signed integer version, use the same code as for unsigned.  */
  return read_be_uint16 (file, (grub_uint16_t *) value);
}

/* Return a pointer to the character index entry for the glyph corresponding to
   the codepoint CODE in the font FONT.  If not found, return zero.  */
static struct char_index_entry *
find_glyph (const grub_font_t font, grub_uint32_t code)
{
  struct char_index_entry *table;
  grub_size_t lo;
  grub_size_t hi;
  grub_size_t mid;

  /* Do a binary search in `char_index', which is ordered by code point.  */
  table = font->char_index;
  lo = 0;
  hi = font->num_chars - 1;

  if (! table)
    return 0;

  while (lo <= hi)
    {
      mid = lo + (hi - lo) / 2;
      if (code < table[mid].code)
        hi = mid - 1;
      else if (code > table[mid].code)
        lo = mid + 1;
      else
        return &table[mid];
    }

  return 0;
}

/* Get a glyph for the Unicode character CODE in FONT.  The glyph is loaded
   from the font file if has not been loaded yet.
   Returns a pointer to the glyph if found, or 0 if it is not found.  */
static struct grub_font_glyph *
grub_font_get_glyph_internal (grub_font_t font, grub_uint32_t code)
{
  struct char_index_entry *index_entry;

  index_entry = find_glyph (font, code);
  if (index_entry)
    {
      struct grub_font_glyph *glyph = 0;
      grub_uint16_t width;
      grub_uint16_t height;
      grub_int16_t xoff;
      grub_int16_t yoff;
      grub_int16_t dwidth;
      int len;

      if (index_entry->glyph)
        /* Return cached glyph.  */
        return index_entry->glyph;

      if (! font->file)
        /* No open file, can't load any glyphs.  */
        return 0;

      /* Make sure we can find glyphs for error messages.  Push active
         error message to error stack and reset error message.  */
      grub_error_push ();

      grub_file_seek (font->file, index_entry->offset);

      /* Read the glyph width, height, and baseline.  */
      if (read_be_uint16(font->file, &width) != 0
          || read_be_uint16(font->file, &height) != 0
          || read_be_int16(font->file, &xoff) != 0
          || read_be_int16(font->file, &yoff) != 0
          || read_be_int16(font->file, &dwidth) != 0)
        {
          remove_font (font);
          return 0;
        }

      len = (width * height + 7) / 8;
      glyph = grub_malloc (sizeof (struct grub_font_glyph) + len);
      if (! glyph)
        {
          remove_font (font);
          return 0;
        }

      glyph->font = font;
      glyph->width = width;
      glyph->height = height;
      glyph->offset_x = xoff;
      glyph->offset_y = yoff;
      glyph->device_width = dwidth;

      /* Don't try to read empty bitmaps (e.g., space characters).  */
      if (len != 0)
        {
          if (grub_file_read (font->file, glyph->bitmap, len) != len)
            {
              remove_font (font);
              return 0;
            }
        }

      /* Restore old error message.  */
      grub_error_pop ();

      /* Cache the glyph.  */
      index_entry->glyph = glyph;

      return glyph;
    }

  return 0;
}

/* Free the memory used by FONT.
   This should not be called if the font has been made available to
   users (once it is added to the global font list), since there would
   be the possibility of a dangling pointer.  */
static void
free_font (grub_font_t font)
{
  if (font)
    {
      if (font->file)
        grub_file_close (font->file);
      grub_free (font->name);
      grub_free (font->family);
      grub_free (font->char_index);
      grub_free (font);
    }
}

/* Add FONT to the global font registry.
   Returns 0 upon success, nonzero on failure
   (the font was not registered).  */
static int
register_font (grub_font_t font)
{
  struct grub_font_node *node = 0;

  node = grub_malloc (sizeof (struct grub_font_node));
  if (! node)
    return 1;

  node->value = font;
  node->next = grub_font_list;
  grub_font_list = node;

  return 0;
}

/* Remove the font from the global font list.  We don't actually free the
   font's memory since users could be holding references to the font.  */
static void
remove_font (grub_font_t font)
{
  struct grub_font_node **nextp, *cur;

  for (nextp = &grub_font_list, cur = *nextp;
       cur;
       nextp = &cur->next, cur = cur->next)
    {
      if (cur->value == font)
        {
          *nextp = cur->next;

          /* Free the node, but not the font itself.  */
          grub_free (cur);

          return;
        }
    }
}

/* Get a font from the list of loaded fonts.  This function will return
   another font if the requested font is not available.  If no fonts are
   loaded, then a special 'null font' is returned, which contains no glyphs,
   but is not a null pointer so the caller may omit checks for NULL.  */
grub_font_t
grub_font_get (const char *font_name)
{
  struct grub_font_node *node;

  for (node = grub_font_list; node; node = node->next)
    {
      grub_font_t font = node->value;
      if (grub_strcmp (font->name, font_name) == 0)
        return font;
    }

  /* If no font by that name is found, return the first font in the list
     as a fallback.  */
  if (grub_font_list && grub_font_list->value)
    return grub_font_list->value;
  else
    /* The null_font is a last resort.  */
    return &null_font;
}

/* Get the full name of the font.  For instance, "Helvetica Bold 12".  */
const char *
grub_font_get_name (grub_font_t font)
{
  return font->name;
}

/* Get the maximum width of any character in the font in pixels.  */
int
grub_font_get_max_char_width (grub_font_t font)
{
  return font->max_char_width;
}

/* Get the maximum height of any character in the font in pixels.  */
int
grub_font_get_max_char_height (grub_font_t font)
{
  return font->max_char_height;
}

/* Get the distance in pixels from the top of characters to the baseline.  */
int
grub_font_get_ascent (grub_font_t font)
{
  return font->ascent;
}

/* Get the distance in pixels from the baseline to the lowest descenders
   (for instance, in a lowercase 'y', 'g', etc.).  */
int
grub_font_get_descent (grub_font_t font)
{
  return font->descent;
}

/* Get the *standard leading* of the font in pixel, which is the spacing
   between two lines of text.  Specifically, it is the space between the
   descent of one line and the ascent of the next line.  This is included
   in the *height* metric.  */
int
grub_font_get_leading (grub_font_t font)
{
  return font->leading;
}

/* Get the distance in pixels between baselines of adjacent lines of text.  */
int
grub_font_get_height (grub_font_t font)
{
  return font->ascent + font->descent + font->leading;
}

/* Get the width in pixels of the specified UTF-8 string, when rendered in
   in the specified font (but falling back on other fonts for glyphs that
   are missing).  */
int
grub_font_get_string_width (grub_font_t font, const char *str)
{
  int width;
  struct grub_font_glyph *glyph;
  grub_uint32_t code;
  const grub_uint8_t *ptr;

  for (ptr = (const grub_uint8_t *) str, width = 0;
       grub_utf8_to_ucs4 (&code, 1, ptr, -1, &ptr) > 0; )
    {
      glyph = grub_font_get_glyph_with_fallback (font, code);
      width += glyph->device_width;
    }

  return width;
}

/* Get the glyph for FONT corresponding to the Unicode code point CODE.
   Returns a pointer to a glyph indicating there is no glyph available
   if CODE does not exist in the font.  The glyphs are cached once loaded.  */
struct grub_font_glyph *
grub_font_get_glyph (grub_font_t font, grub_uint32_t code)
{
  struct grub_font_glyph *glyph;
  glyph = grub_font_get_glyph_internal (font, code);
  if (glyph == 0)
    glyph = unknown_glyph;
  return glyph;
}


/* Calculate a subject value representing "how similar" two fonts are.
   This is used to prioritize the order that fonts are scanned for missing
   glyphs.  The object is to select glyphs from the most similar font
   possible, for the best appearance.
   The heuristic is crude, but it helps greatly when fonts of similar
   sizes are used so that tiny 8 point glyphs are not mixed into a string
   of 24 point text unless there is no other choice.  */
static int
get_font_diversity(grub_font_t a, grub_font_t b)
{
  int d;

  d = 0;

  if (a->ascent && b->ascent)
    d += grub_abs (a->ascent - b->ascent) * 8;
  else
    /* Penalty for missing attributes.  */
    d += 50;

  if (a->max_char_height && b->max_char_height)
    d += grub_abs (a->max_char_height - b->max_char_height) * 8;
  else
    /* Penalty for missing attributes.  */
    d += 50;

  /* Weight is a minor factor. */
  d += (a->weight != b->weight) ? 5 : 0;

  return d;
}

/* Get a glyph corresponding to the codepoint CODE.  If FONT contains the
   specified glyph, then it is returned.  Otherwise, all other loaded fonts
   are searched until one is found that contains a glyph for CODE.
   If no glyph is available for CODE in the loaded fonts, then a glyph
   representing an unknown character is returned.
   This function never returns NULL.
   The returned glyph is owned by the font manager and should not be freed
   by the caller.  The glyphs are cached.  */
struct grub_font_glyph *
grub_font_get_glyph_with_fallback (grub_font_t font, grub_uint32_t code)
{
  struct grub_font_glyph *glyph;
  struct grub_font_node *node;
  /* Keep track of next node, in case there's an I/O error in
     grub_font_get_glyph_internal() and the font is removed from the list.  */
  struct grub_font_node *next;
  /* Information on the best glyph found so far, to help find the glyph in
     the best matching to the requested one.  */
  int best_diversity;
  struct grub_font_glyph *best_glyph;

  if (font)
    {
      /* First try to get the glyph from the specified font.  */
      glyph = grub_font_get_glyph_internal (font, code);
      if (glyph)
        return glyph;
    }

  /* Otherwise, search all loaded fonts for the glyph and use the one from
     the font that best matches the requested font.  */
  best_diversity = 10000;
  best_glyph = 0;

  for (node = grub_font_list; node; node = next)
    {
      grub_font_t curfont;

      curfont = node->value;
      next = node->next;

      glyph = grub_font_get_glyph_internal (curfont, code);
      if (glyph)
        {
          int d;

          d = get_font_diversity (curfont, font);
          if (d < best_diversity)
            {
              best_diversity = d;
              best_glyph = glyph;
            }
        }
    }

  if (best_glyph)
    return best_glyph;
  else
    /* Glyph not available in any font.  Return unknown glyph.  */
    return unknown_glyph;
}


/* Draw the specified glyph at (x, y).  The y coordinate designates the
   baseline of the character, while the x coordinate designates the left
   side location of the character.  */
grub_err_t
grub_font_draw_glyph (struct grub_font_glyph *glyph,
                      grub_video_color_t color,
                      int left_x, int baseline_y)
{
  struct grub_video_bitmap glyph_bitmap;

  /* Don't try to draw empty glyphs (U+0020, etc.).  */
  if (glyph->width == 0 || glyph->height == 0)
    return GRUB_ERR_NONE;

  glyph_bitmap.mode_info.width = glyph->width;
  glyph_bitmap.mode_info.height = glyph->height;
  glyph_bitmap.mode_info.mode_type =
    (1 << GRUB_VIDEO_MODE_TYPE_DEPTH_POS)
    | GRUB_VIDEO_MODE_TYPE_1BIT_BITMAP;
  glyph_bitmap.mode_info.blit_format = GRUB_VIDEO_BLIT_FORMAT_1BIT_PACKED;
  glyph_bitmap.mode_info.bpp = 1;

  /* Really 1 bit per pixel.  */
  glyph_bitmap.mode_info.bytes_per_pixel = 0;

  /* Packed densely as bits.  */
  glyph_bitmap.mode_info.pitch = glyph->width;

  glyph_bitmap.mode_info.number_of_colors = 2;
  glyph_bitmap.mode_info.bg_red = 0;
  glyph_bitmap.mode_info.bg_green = 0;
  glyph_bitmap.mode_info.bg_blue = 0;
  glyph_bitmap.mode_info.bg_alpha = 0;
  grub_video_unmap_color(color,
                         &glyph_bitmap.mode_info.fg_red,
                         &glyph_bitmap.mode_info.fg_green,
                         &glyph_bitmap.mode_info.fg_blue,
                         &glyph_bitmap.mode_info.fg_alpha);
  glyph_bitmap.data = glyph->bitmap;

  int bitmap_left = left_x + glyph->offset_x;
  int bitmap_bottom = baseline_y - glyph->offset_y;
  int bitmap_top = bitmap_bottom - glyph->height;

  return grub_video_blit_bitmap (&glyph_bitmap, GRUB_VIDEO_BLIT_BLEND,
                                 bitmap_left, bitmap_top,
                                 0, 0,
                                 glyph->width, glyph->height);
}

/* Draw a UTF-8 string of text on the current video render target.
   The x coordinate specifies the starting x position for the first character,
   while the y coordinate specifies the baseline position.
   If the string contains a character that FONT does not contain, then
   a glyph from another loaded font may be used instead.  */
grub_err_t
grub_font_draw_string (const char *str, grub_font_t font,
                       grub_video_color_t color,
                       int left_x, int baseline_y)
{
  int x;
  struct grub_font_glyph *glyph;
  grub_uint32_t code;
  const grub_uint8_t *ptr;

  for (ptr = (const grub_uint8_t *) str, x = left_x;
       grub_utf8_to_ucs4 (&code, 1, ptr, -1, &ptr) > 0; )
    {
      glyph = grub_font_get_glyph_with_fallback (font, code);
      if (grub_font_draw_glyph (glyph, color, x, baseline_y)
          != GRUB_ERR_NONE)
        return grub_errno;
      x += glyph->device_width;
    }

  return GRUB_ERR_NONE;
}

