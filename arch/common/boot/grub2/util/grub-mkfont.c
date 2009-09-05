/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <config.h>
#include <grub/types.h>
#include <grub/util/misc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsynth.h>

#define GRUB_FONT_DEFAULT_SIZE		16

#define GRUB_FONT_RANGE_BLOCK		1024

struct grub_glyph_info
{
  struct grub_glyph_info *next;
  grub_uint32_t char_code;
  int width;
  int height;
  int x_ofs;
  int y_ofs;
  int device_width;
  int bitmap_size;
  grub_uint8_t bitmap[0];
};

#define GRUB_FONT_FLAG_BOLD		1
#define GRUB_FONT_FLAG_NOBITMAP		2
#define GRUB_FONT_FLAG_NOHINTING	4
#define GRUB_FONT_FLAG_FORCEHINT	8

struct grub_font_info
{
  char* name;
  int style;
  int desc;
  int size;
  int max_width;
  int max_height;
  int min_y;
  int flags;
  int num_range;
  grub_uint32_t *ranges;
  struct grub_glyph_info *glyph;
};

static struct option options[] =
{
  {"output", required_argument, 0, 'o'},
  {"name", required_argument, 0, 'n'},
  {"index", required_argument, 0, 'i'},
  {"range", required_argument, 0, 'r'},
  {"size", required_argument, 0, 's'},
  {"desc", required_argument, 0, 'd'},
  {"bold", no_argument, 0, 'b'},
  {"no-bitmap", no_argument, 0, 0x100},
  {"no-hinting", no_argument, 0, 0x101},
  {"force-autohint", no_argument, 0, 'a'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

int font_verbosity;

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-mkfont --help'' for more information.\n");
  else
    printf ("\
Usage: grub-mkfont [OPTIONS] FONT_FILES\n\
\nOptions:\n\
  -o, --output=FILE_NAME    set output file name\n\
  -i, --index=N             set face index\n\
  -r, --range=A-B[,C-D]     set font range\n\
  -n, --name=S              set font family name\n\
  -s, --size=N              set font size\n\
  -d, --desc=N              set font descent\n\
  -b, --bold                convert to bold font\n\
  -a, --force-autohint      force autohint\n\
  --no-hinting              disable hinting\n\
  --no-bitmap               ignore bitmap strikes when loading\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", PACKAGE_BUGREPORT);

  exit (status);
}

void
add_pixel (grub_uint8_t **data, int *mask, int not_blank)
{
  if (*mask == 0)
    {
      (*data)++;
      **data = 0;
      *mask = 128;
    }

  if (not_blank)
    **data |= *mask;

  *mask >>= 1;
}

void
add_char (struct grub_font_info *font_info, FT_Face face,
	  grub_uint32_t char_code)
{
  struct grub_glyph_info *glyph_info, **p_glyph;
  int width, height;
  grub_uint8_t *data;
  int mask, i, j, bitmap_size;
  FT_GlyphSlot glyph;
  int flag = FT_LOAD_RENDER | FT_LOAD_MONOCHROME;

  if (font_info->flags & GRUB_FONT_FLAG_NOBITMAP)
    flag |= FT_LOAD_NO_BITMAP;

  if (font_info->flags & GRUB_FONT_FLAG_NOHINTING)
    flag |= FT_LOAD_NO_HINTING;
  else if (font_info->flags & GRUB_FONT_FLAG_FORCEHINT)
    flag |= FT_LOAD_FORCE_AUTOHINT;

  if (FT_Load_Char (face, char_code, flag))
    return;

  glyph = face->glyph;

  if (font_info->flags & GRUB_FONT_FLAG_BOLD)
    FT_GlyphSlot_Embolden (glyph);

  p_glyph = &font_info->glyph;
  while ((*p_glyph) && ((*p_glyph)->char_code > char_code))
    {
      p_glyph = &(*p_glyph)->next;
    }

  /* Ignore duplicated glyph.  */
  if ((*p_glyph) && ((*p_glyph)->char_code == char_code))
    return;

  width = glyph->bitmap.width;
  height = glyph->bitmap.rows;

  bitmap_size = ((width * height + 7) / 8);
  glyph_info = xmalloc (sizeof (struct grub_glyph_info) + bitmap_size);
  glyph_info->bitmap_size = bitmap_size;

  glyph_info->next = *p_glyph;
  *p_glyph = glyph_info;

  glyph_info->char_code = char_code;
  glyph_info->width = width;
  glyph_info->height = height;
  glyph_info->x_ofs = glyph->bitmap_left;
  glyph_info->y_ofs = glyph->bitmap_top - height;
  glyph_info->device_width = glyph->metrics.horiAdvance / 64;

  if (width > font_info->max_width)
    font_info->max_width = width;

  if (height > font_info->max_height)
    font_info->max_height = height;

  if (glyph_info->y_ofs < font_info->min_y)
    font_info->min_y = glyph_info->y_ofs;

  mask = 0;
  data = &glyph_info->bitmap[0] - 1;
  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      add_pixel (&data, &mask,
		 glyph->bitmap.buffer[i / 8 + j * glyph->bitmap.pitch] &
		 (1 << (7 - (i & 7))));
}

void
add_font (struct grub_font_info *font_info, FT_Face face)
{
  if (font_info->num_range)
    {
      int i;
      grub_uint32_t j;

      for (i = 0; i < font_info->num_range; i++)
	for (j = font_info->ranges[i * 2]; j <= font_info->ranges[i * 2 + 1];
	     j++)
	  add_char (font_info, face, j);
    }
  else
    {
      grub_uint32_t char_code, glyph_index;

      for (char_code = FT_Get_First_Char (face, &glyph_index);
	   glyph_index;
	   char_code = FT_Get_Next_Char (face, char_code, &glyph_index))
	add_char (font_info, face, char_code);
    }
}

void
write_string_section (char *name, char *str, int* offset, FILE* file)
{
  grub_uint32_t leng, leng_be32;

  leng = strlen (str) + 1;
  leng_be32 = grub_cpu_to_be32 (leng);

  grub_util_write_image (name, 4, file);
  grub_util_write_image ((char *) &leng_be32, 4, file);
  grub_util_write_image (str, leng, file);

  *offset += 8 + leng;
}

void
write_be16_section (char *name, grub_uint16_t data, int* offset, FILE* file)
{
  grub_uint32_t leng;

  leng = grub_cpu_to_be32 (2);
  data = grub_cpu_to_be16 (data);
  grub_util_write_image (name, 4, file);
  grub_util_write_image ((char *) &leng, 4, file);
  grub_util_write_image ((char *) &data, 2, file);

  *offset += 10;
}

void
print_glyphs (struct grub_font_info *font_info)
{
  int num;
  struct grub_glyph_info *glyph;
  char line[512];

  for (glyph = font_info->glyph, num = 0; glyph; glyph = glyph->next, num++)
    {
      int x, y, xmax, xmin, ymax, ymin;
      grub_uint8_t *bitmap, mask;

      printf ("\nGlyph #%d, U+%04x\n", num, glyph->char_code);
      printf ("Width %d, Height %d, X offset %d, Y offset %d, Device width %d\n",
	      glyph->width, glyph->height, glyph->x_ofs, glyph->y_ofs,
	      glyph->device_width);

      xmax = glyph->x_ofs + glyph->width;
      if (xmax < glyph->device_width)
	xmax = glyph->device_width;

      xmin = glyph->x_ofs;
      if (xmin > 0)
	xmin = 0;

      ymax = glyph->y_ofs + glyph->height;
      if (ymax < font_info->size - font_info->desc)
	ymax = font_info->size - font_info->desc;

      ymin = glyph->y_ofs;
      if (ymin > - font_info->desc)
	ymin = - font_info->desc;

      bitmap = glyph->bitmap;
      mask = 0x80;
      for (y = ymax - 1; y >= ymin; y--)
	{
	  int line_pos;

	  line_pos = 0;
	  for (x = xmin; x < xmax; x++)
	    {
	      if ((x >= glyph->x_ofs) &&
		  (x < glyph->x_ofs + glyph->width) &&
		  (y >= glyph->y_ofs) &&
		  (y < glyph->y_ofs + glyph->height))
		{
		  line[line_pos++] = (*bitmap & mask) ? '#' : '_';
		  mask >>= 1;
		  if (mask == 0)
		    {
		      mask = 0x80;
		      bitmap++;
		    }
		}
	      else if ((x >= 0) &&
		       (x < glyph->device_width) &&
		       (y >= - font_info->desc) &&
		       (y < font_info->size - font_info->desc))
		{
		  line[line_pos++] = ((x == 0) || (y == 0)) ? '+' : '.';
		}
	      else
		line[line_pos++] = '*';
	    }
	  line[line_pos] = 0;
	  printf ("%s\n", line);
	}
    }
}

void
write_font (struct grub_font_info *font_info, char *output_file)
{
  FILE *file;
  grub_uint32_t leng, data;
  char style_name[20], *font_name;
  struct grub_glyph_info *cur, *pre;
  int num, offset;

  file = fopen (output_file, "wb");
  if (! file)
    grub_util_error ("Can\'t write to file %s.", output_file);

  offset = 0;

  leng = grub_cpu_to_be32 (4);
  grub_util_write_image ("FILE", 4, file);
  grub_util_write_image ((char *) &leng, 4, file);
  grub_util_write_image ("PFF2", 4, file);
  offset += 12;

  if (! font_info->name)
    font_info->name = "Unknown";

  if (font_info->flags & GRUB_FONT_FLAG_BOLD)
    font_info->style |= FT_STYLE_FLAG_BOLD;

  style_name[0] = 0;
  if (font_info->style & FT_STYLE_FLAG_BOLD)
    strcpy (style_name, " Bold");

  if (font_info->style & FT_STYLE_FLAG_ITALIC)
    strcat (style_name, " Italic");

  if (! style_name[0])
    strcpy (style_name, " Regular");

  asprintf (&font_name, "%s %s %d", font_info->name, &style_name[1],
	    font_info->size);

  write_string_section ("NAME", font_name, &offset, file);
  write_string_section ("FAMI", font_info->name, &offset, file);
  write_string_section ("WEIG",
			(font_info->style & FT_STYLE_FLAG_BOLD) ?
			"bold" : "normal",
			&offset, file);
  write_string_section ("SLAN",
			(font_info->style & FT_STYLE_FLAG_ITALIC) ?
			"italic" : "normal",
			&offset, file);

  write_be16_section ("PTSZ", font_info->size, &offset, file);
  write_be16_section ("MAXW", font_info->max_width, &offset, file);
  write_be16_section ("MAXH", font_info->max_height, &offset, file);

  if (! font_info->desc)
    {
      if (font_info->min_y >= 0)
	font_info->desc = 1;
      else
	font_info->desc = - font_info->min_y;
    }

  write_be16_section ("ASCE", font_info->size - font_info->desc, &offset, file);
  write_be16_section ("DESC", font_info->desc, &offset, file);

  if (font_verbosity > 0)
    {
      printf ("Font name: %s\n", font_name);
      printf ("Max width: %d\n", font_info->max_width);
      printf ("Max height: %d\n", font_info->max_height);
      printf ("Font ascent: %d\n", font_info->size - font_info->desc);
      printf ("Font descent: %d\n", font_info->desc);
    }

  num = 0;
  pre = 0;
  cur = font_info->glyph;
  while (cur)
    {
      struct grub_glyph_info *nxt;

      nxt = cur->next;
      cur->next = pre;
      pre = cur;
      cur = nxt;
      num++;
    }

  font_info->glyph = pre;

  if (font_verbosity > 0)
    printf ("Number of glyph: %d\n", num);

  leng = grub_cpu_to_be32 (num * 9);
  grub_util_write_image ("CHIX", 4, file);
  grub_util_write_image ((char *) &leng, 4, file);
  offset += 8 + num * 9 + 8;

  for (cur = font_info->glyph; cur; cur = cur->next)
    {
      data = grub_cpu_to_be32 (cur->char_code);
      grub_util_write_image ((char *) &data, 4, file);
      data = 0;
      grub_util_write_image ((char *) &data, 1, file);
      data = grub_cpu_to_be32 (offset);
      grub_util_write_image ((char *) &data, 4, file);
      offset += 10 + cur->bitmap_size;
    }

  leng = 0xffffffff;
  grub_util_write_image ("DATA", 4, file);
  grub_util_write_image ((char *) &leng, 4, file);

  for (cur = font_info->glyph; cur; cur = cur->next)
    {
      data = grub_cpu_to_be16 (cur->width);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->height);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->x_ofs);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->y_ofs);
      grub_util_write_image ((char *) &data, 2, file);
      data = grub_cpu_to_be16 (cur->device_width);
      grub_util_write_image ((char *) &data, 2, file);
      grub_util_write_image ((char *) &cur->bitmap[0], cur->bitmap_size, file);
    }

  if (font_verbosity > 1)
    print_glyphs (font_info);

  fclose (file);
}

int
main (int argc, char *argv[])
{
  struct grub_font_info font_info;
  FT_Library ft_lib;
  int font_index = 0;
  int font_size = 0;
  char *output_file = NULL;

  memset (&font_info, 0, sizeof (font_info));

  progname = "grub-mkfont";

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "bao:n:i:s:d:r:hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'b':
	    font_info.flags |= GRUB_FONT_FLAG_BOLD;
	    break;

	  case 0x100:
	    font_info.flags |= GRUB_FONT_FLAG_NOBITMAP;
	    break;

	  case 0x101:
	    font_info.flags |= GRUB_FONT_FLAG_NOHINTING;
	    break;

	  case 'a':
	    font_info.flags |= GRUB_FONT_FLAG_FORCEHINT;
	    break;

	  case 'o':
	    output_file = optarg;
	    break;

	  case 'n':
	    font_info.name = optarg;
	    break;

	  case 'i':
	    font_index = strtoul (optarg, NULL, 0);
	    break;

	  case 's':
	    font_size = strtoul (optarg, NULL, 0);
	    break;

	  case 'r':
	    {
	      char *p = optarg;

	      while (1)
		{
		  grub_uint32_t a, b;

		  a = strtoul (p, &p, 0);
		  if (*p != '-')
		    grub_util_error ("Invalid font range");
		  b = strtoul (p + 1, &p, 0);
		  if ((font_info.num_range & (GRUB_FONT_RANGE_BLOCK - 1)) == 0)
		    font_info.ranges = xrealloc (font_info.ranges,
						 (font_info.num_range +
						  GRUB_FONT_RANGE_BLOCK) *
						 sizeof (int) * 2);

		  font_info.ranges[font_info.num_range * 2] = a;
		  font_info.ranges[font_info.num_range * 2 + 1] = b;
		  font_info.num_range++;

		  if (*p)
		    {
		      if (*p != ',')
			grub_util_error ("Invalid font range");
		      else
			p++;
		    }
		  else
		    break;
		}
	      break;
	    }

	  case 'd':
	    font_info.desc = strtoul (optarg, NULL, 0);
	    break;

	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", progname, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    font_verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  if (! output_file)
    grub_util_error ("No output file is specified.");

  if (FT_Init_FreeType (&ft_lib))
    grub_util_error ("FT_Init_FreeType fails");

  for (; optind < argc; optind++)
    {
      FT_Face ft_face;
      int size;

      if (FT_New_Face (ft_lib, argv[optind], font_index, &ft_face))
	{
	  grub_util_info ("Can't open file %s, index %d\n", argv[optind],
			  font_index);
	  continue;
	}

      if ((! font_info.name) && (ft_face->family_name))
	font_info.name = xstrdup (ft_face->family_name);

      size = font_size;
      if (! size)
	{
	  if ((ft_face->face_flags & FT_FACE_FLAG_SCALABLE) ||
	      (! ft_face->num_fixed_sizes))
	    size = GRUB_FONT_DEFAULT_SIZE;
	  else
	    size = ft_face->available_sizes[0].height;
	}

      font_info.style = ft_face->style_flags;
      font_info.size = size;

      FT_Set_Pixel_Sizes (ft_face, size, size);
      add_font (&font_info, ft_face);
      FT_Done_Face (ft_face);
    }

  FT_Done_FreeType (ft_lib);

  write_font (&font_info, output_file);

  return 0;
}
