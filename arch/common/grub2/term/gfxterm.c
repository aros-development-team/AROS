/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/machine/memory.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/font.h>
#include <grub/arg.h>
#include <grub/mm.h>
#include <grub/env.h>
#include <grub/video.h>
#include <grub/bitmap.h>

#define DEFAULT_VIDEO_WIDTH	640
#define DEFAULT_VIDEO_HEIGHT	480
#define DEFAULT_VIDEO_FLAGS	0

#define DEFAULT_CHAR_WIDTH  	8
#define DEFAULT_CHAR_HEIGHT 	16

#define DEFAULT_BORDER_WIDTH	10

#define DEFAULT_STANDARD_COLOR  0x07
#define DEFAULT_NORMAL_COLOR    0x07
#define DEFAULT_HIGHLIGHT_COLOR 0x70
#define DEFAULT_CURSOR_COLOR	0x07

struct grub_dirty_region
{
  int top_left_x;
  int top_left_y;
  int bottom_right_x;
  int bottom_right_y;
};

struct grub_colored_char
{
  /* An Unicode codepoint.  */
  grub_uint32_t code;

  /* Color values.  */
  grub_video_color_t fg_color;
  grub_video_color_t bg_color;

  /* The width of this character minus one.  */
  unsigned char width;

  /* The column index of this character.  */
  unsigned char index;
};

struct grub_virtual_screen
{
  /* Dimensions of the virtual screen.  */
  unsigned int width;
  unsigned int height;

  /* Offset in the display.  */
  unsigned int offset_x;
  unsigned int offset_y;

  /* TTY Character sizes.  */
  unsigned int char_width;
  unsigned int char_height;

  /* Virtual screen TTY size.  */
  unsigned int columns;
  unsigned int rows;

  /* Current cursor details.  */
  unsigned int cursor_x;
  unsigned int cursor_y;
  int cursor_state;

  /* Terminal color settings.  */
  grub_uint8_t standard_color_setting;
  grub_uint8_t normal_color_setting;
  grub_uint8_t highlight_color_setting;
  grub_uint8_t term_color;
  
  /* Color settings.  */
  grub_video_color_t fg_color;
  grub_video_color_t bg_color;
  grub_video_color_t cursor_color;

  /* Text buffer for virtual screen.  Contains (columns * rows) number
     of entries.  */
  struct grub_colored_char *text_buffer;
};

static struct grub_virtual_screen virtual_screen;

static grub_dl_t my_mod;
static struct grub_video_mode_info mode_info;

static struct grub_video_render_target *text_layer;

static unsigned int bitmap_width;
static unsigned int bitmap_height;
static struct grub_video_bitmap *bitmap;

static struct grub_dirty_region dirty_region;

static void dirty_region_reset (void);

static int dirty_region_is_empty (void);

static void dirty_region_add (int x, int y, 
                              unsigned int width, unsigned int height);

static void
set_term_color (grub_uint8_t term_color)
{
  struct grub_video_render_target *old_target;

  /* Save previous target and switch to text layer.  */
  grub_video_get_active_render_target (&old_target);
  grub_video_set_active_render_target (text_layer);

  /* Map terminal color to text layer compatible video colors.  */
  virtual_screen.fg_color = grub_video_map_color(term_color & 0x0f);
  
  /* Special case: use black as transparent color.  */
  if (((term_color >> 4) & 0x0f) == 0)
    {
      virtual_screen.bg_color = grub_video_map_rgba(0, 0, 0, 0);
    } 
  else
    {
      virtual_screen.bg_color = grub_video_map_color((term_color >> 4) & 0x0f);
    }

  /* Restore previous target.  */
  grub_video_set_active_render_target (old_target);
}

static void
grub_virtual_screen_free (void)
{
  /* If virtual screen has been allocated, free it.  */
  if (virtual_screen.text_buffer != 0)
    grub_free (virtual_screen.text_buffer);

  /* Reset virtual screen data.  */
  grub_memset (&virtual_screen, 0, sizeof (virtual_screen));

  /* Free render targets.  */
  grub_video_delete_render_target (text_layer);
  text_layer = 0;
}

static grub_err_t
grub_virtual_screen_setup (unsigned int x, unsigned int y,
                           unsigned int width, unsigned int height)
{
  /* Free old virtual screen.  */
  grub_virtual_screen_free ();

  /* Initialize with default data.  */
  virtual_screen.width = width;
  virtual_screen.height = height;
  virtual_screen.offset_x = x;
  virtual_screen.offset_y = y;
  virtual_screen.char_width = DEFAULT_CHAR_WIDTH;
  virtual_screen.char_height = DEFAULT_CHAR_HEIGHT;
  virtual_screen.cursor_x = 0;
  virtual_screen.cursor_y = 0;
  virtual_screen.cursor_state = 1;

  /* Calculate size of text buffer.  */
  virtual_screen.columns = virtual_screen.width / virtual_screen.char_width;
  virtual_screen.rows = virtual_screen.height / virtual_screen.char_height;

  /* Allocate memory for text buffer.  */
  virtual_screen.text_buffer =
    (struct grub_colored_char *) grub_malloc (virtual_screen.columns
                                              * virtual_screen.rows
                                              * sizeof (*virtual_screen.text_buffer));
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* Create new render target for text layer.  */
  grub_video_create_render_target (&text_layer,
                                   virtual_screen.width,
                                   virtual_screen.height,
                                   GRUB_VIDEO_MODE_TYPE_RGB
                                   | GRUB_VIDEO_MODE_TYPE_ALPHA);
  if (grub_errno != GRUB_ERR_NONE)
    return grub_errno;

  /* As we want to have colors compatible with rendering target,
     we can only have those after mode is initialized.  */
  grub_video_set_active_render_target (text_layer);

  virtual_screen.standard_color_setting = DEFAULT_STANDARD_COLOR;
  virtual_screen.normal_color_setting = DEFAULT_NORMAL_COLOR;
  virtual_screen.highlight_color_setting = DEFAULT_HIGHLIGHT_COLOR;
  
  virtual_screen.term_color = virtual_screen.normal_color_setting;
  
  set_term_color (virtual_screen.term_color);

  virtual_screen.cursor_color = grub_video_map_color (DEFAULT_CURSOR_COLOR);

  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  return grub_errno;
}

static grub_err_t
grub_gfxterm_init (void)
{
  char *modevar;
  int width = DEFAULT_VIDEO_WIDTH;
  int height = DEFAULT_VIDEO_HEIGHT;
  int depth = -1;
  int flags = DEFAULT_VIDEO_FLAGS;
  grub_video_color_t color;

  /* Parse gfxmode environment variable if set.  */
  modevar = grub_env_get ("gfxmode");
  if (modevar)
    {
      char *tmp;
      char *next_mode;
      char *current_mode;
      char *param;
      char *value;
      int mode_found = 0;

      /* Take copy of env.var. as we don't want to modify that.  */
      tmp = grub_strdup (modevar);
      modevar = tmp;

      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;
        
      /* Initialize next mode.  */
      next_mode = modevar;
      
      /* Loop until all modes has been tested out.  */
      while (next_mode != NULL)
        {
          /* Use last next_mode as current mode.  */
          tmp = next_mode;
          
          /* Reset video mode settings.  */
          width = DEFAULT_VIDEO_WIDTH;
          height = DEFAULT_VIDEO_HEIGHT;
          depth = -1;
          flags = DEFAULT_VIDEO_FLAGS;
        
          /* Save position of next mode and separate modes.  */
          next_mode = grub_strchr(next_mode, ';');
          if (next_mode)
            {
              *next_mode = 0;
              next_mode++;
            }

          /* Skip whitespace.  */
          while (grub_isspace (*tmp))
            tmp++;

          /* Initialize token holders.  */
          current_mode = tmp;
          param = tmp;
          value = NULL;

          /* Parse <width>x<height>[x<depth>]*/

          /* Find width value.  */
          value = param;
          param = grub_strchr(param, 'x');
          if (param == NULL)
            {
              grub_err_t rc;
              
              /* First setup error message.  */
              rc = grub_error (GRUB_ERR_BAD_ARGUMENT, 
                               "Invalid mode: %s\n",
                               current_mode);
              
              /* Free memory before returning.  */
              grub_free (modevar);
              
              return rc;
            }

          *param = 0;
          param++;

          width = grub_strtoul (value, 0, 0);
          if (grub_errno != GRUB_ERR_NONE)
            {
              grub_err_t rc;
              
              /* First setup error message.  */
              rc = grub_error (GRUB_ERR_BAD_ARGUMENT, 
                               "Invalid mode: %s\n",
                               current_mode);
              
              /* Free memory before returning.  */
              grub_free (modevar);
              
              return rc;
            }

          /* Find height value.  */
          value = param;
          param = grub_strchr(param, 'x');
          if (param == NULL)
            {
              height = grub_strtoul (value, 0, 0);
              if (grub_errno != GRUB_ERR_NONE)
                {
                  grub_err_t rc;
                  
                  /* First setup error message.  */
                  rc = grub_error (GRUB_ERR_BAD_ARGUMENT, 
                                   "Invalid mode: %s\n",
                                   current_mode);
                                   
                  /* Free memory before returning.  */
                  grub_free (modevar);
                  
                  return rc;
                }
            }
          else
            {
              /* We have optional color depth value.  */
              *param = 0;
              param++;

              height = grub_strtoul (value, 0, 0);
              if (grub_errno != GRUB_ERR_NONE)
                {
                  grub_err_t rc;
                  
                  /* First setup error message.  */
                  rc = grub_error (GRUB_ERR_BAD_ARGUMENT, 
                                   "Invalid mode: %s\n",
                                   current_mode);
                                   
                  /* Free memory before returning.  */
                  grub_free (modevar);
                  
                  return rc;
                }

              /* Convert color depth value.  */
              value = param;
              depth = grub_strtoul (value, 0, 0);
              if (grub_errno != GRUB_ERR_NONE)
                {
                  grub_err_t rc;
                  
                  /* First setup error message.  */
                  rc = grub_error (GRUB_ERR_BAD_ARGUMENT, 
                                   "Invalid mode: %s\n",
                                   current_mode);
                                   
                  /* Free memory before returning.  */
                  grub_free (modevar);
                  
                  return rc;
                }
            }
          
          /* Try out video mode.  */
          
          /* If we have 8 or less bits, then assume that it is indexed color mode.  */
          if ((depth <= 8) && (depth != -1))
            flags |= GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;

          /* We have more than 8 bits, then assume that it is RGB color mode.  */
          if (depth > 8)
            flags |= GRUB_VIDEO_MODE_TYPE_RGB;

          /* If user requested specific depth, forward that information to driver.  */
          if (depth != -1)
            flags |= (depth << GRUB_VIDEO_MODE_TYPE_DEPTH_POS)
                     & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK;

          /* Try to initialize requested mode.  Ignore any errors.  */
          grub_error_push ();
          if (grub_video_setup (width, height, flags) != GRUB_ERR_NONE)
            {
              grub_error_pop ();
              continue;
            }
            
          /* Figure out what mode we ended up.  */
          if (grub_video_get_info (&mode_info) != GRUB_ERR_NONE)
            {
              /* Couldn't get video mode info, restore old mode and continue to next one.  */
              grub_error_pop ();
              
              grub_video_restore ();
              continue;
            }
          
          /* Restore state of error stack.  */
          grub_error_pop ();
          
          /* Mode found!  Exit loop.  */
          mode_found = 1;
          break;
        }

      /* Free memory.  */
      grub_free (modevar);
      
      if (!mode_found)
        return grub_error (GRUB_ERR_BAD_ARGUMENT,
                           "No suitable mode found.");
    }
  else
    {
      /* No gfxmode variable set, use defaults.  */
      
      /* If we have 8 or less bits, then assume that it is indexed color mode.  */
      if ((depth <= 8) && (depth != -1))
        flags |= GRUB_VIDEO_MODE_TYPE_INDEX_COLOR;

      /* We have more than 8 bits, then assume that it is RGB color mode.  */
      if (depth > 8)
        flags |= GRUB_VIDEO_MODE_TYPE_RGB;

      /* If user requested specific depth, forward that information to driver.  */
      if (depth != -1)
        flags |= (depth << GRUB_VIDEO_MODE_TYPE_DEPTH_POS)
                 & GRUB_VIDEO_MODE_TYPE_DEPTH_MASK;

      /* Initialize user requested mode.  */
      if (grub_video_setup (width, height, flags) != GRUB_ERR_NONE)
        return grub_errno;

      /* Figure out what mode we ended up.  */
      if (grub_video_get_info (&mode_info) != GRUB_ERR_NONE)
        {
          grub_video_restore ();
          return grub_errno;
        }
    }

  /* Make sure screen is black.  */
  color = grub_video_map_rgb (0, 0, 0);
  grub_video_fill_rect (color, 0, 0, mode_info.width, mode_info.height);
  bitmap = 0;

  /* Leave borders for virtual screen.  */
  width = mode_info.width - (2 * DEFAULT_BORDER_WIDTH);
  height = mode_info.height - (2 * DEFAULT_BORDER_WIDTH);

  /* Create virtual screen.  */
  if (grub_virtual_screen_setup (DEFAULT_BORDER_WIDTH, DEFAULT_BORDER_WIDTH,
                                 width, height) != GRUB_ERR_NONE)
    {
      grub_video_restore ();
      return grub_errno;
    }

  /* Mark whole screen as dirty.  */
  dirty_region_reset ();
  dirty_region_add (0, 0, mode_info.width, mode_info.height);

  return (grub_errno = GRUB_ERR_NONE);
}

static grub_err_t
grub_gfxterm_fini (void)
{
  if (bitmap)
    {
      grub_video_bitmap_destroy (bitmap);
      bitmap = 0;
    }

  grub_virtual_screen_free ();

  grub_video_restore ();

  return GRUB_ERR_NONE;
}

static void
redraw_screen_rect (unsigned int x, unsigned int y, 
                    unsigned int width, unsigned int height)
{
  grub_video_color_t color;

  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);


  if (bitmap)
    {
      /* Render bitmap as background.  */
      grub_video_blit_bitmap (bitmap, GRUB_VIDEO_BLIT_REPLACE, x, y, 
                              x, y, 
                              width, height);
      
      /* If bitmap is smaller than requested blit area, use background 
         color.  */
      color = virtual_screen.bg_color;

      /* Fill right side of the bitmap if needed.  */
      if ((x + width >= bitmap_width) && (y < bitmap_height))
        {
          int w = (x + width) - bitmap_width;
          int h = height;
          unsigned int tx = x;

          if (y + height >= bitmap_height)
            {
              h = bitmap_height - y;
            }
          
          if (bitmap_width > tx)
            {
              tx = bitmap_width;
            }
          
          /* Render background layer.  */
          grub_video_fill_rect (color, tx, y, w, h);        
        }
      
      /* Fill bottom side of the bitmap if needed.  */
      if (y + height >= bitmap_height)
        {
          int h = (y + height) - bitmap_height;
          unsigned int ty = y;
          
          if (bitmap_height > ty)
            {
              ty = bitmap_height;
            }
          
          /* Render background layer.  */
          grub_video_fill_rect (color, x, ty, width, h);        
        }

      /* Render text layer as blended.  */
      grub_video_blit_render_target (text_layer, GRUB_VIDEO_BLIT_BLEND, x, y,
                                     x - virtual_screen.offset_x,
                                     y - virtual_screen.offset_y,
                                     width, height);
    }
  else
    {
      /* Render background layer.  */
      color = virtual_screen.bg_color;
      grub_video_fill_rect (color, x, y, width, height);

      /* Render text layer as replaced (to get texts background color).  */
      grub_video_blit_render_target (text_layer, GRUB_VIDEO_BLIT_REPLACE, x, y,
                                     x - virtual_screen.offset_x,
                                     y - virtual_screen.offset_y,
                                     width, height);      
    }
}

static void
dirty_region_reset (void)
{
  dirty_region.top_left_x = -1;
  dirty_region.top_left_y = -1;
  dirty_region.bottom_right_x = -1;
  dirty_region.bottom_right_y = -1;
}

static int
dirty_region_is_empty (void)
{
  if ((dirty_region.top_left_x == -1)
      || (dirty_region.top_left_y == -1)
      || (dirty_region.bottom_right_x == -1)
      || (dirty_region.bottom_right_y == -1))
    return 1;
  return 0;
}

static void
dirty_region_add (int x, int y, unsigned int width, unsigned int height)
{
  if ((width == 0) || (height == 0))
    return;

  if (dirty_region_is_empty ())
    {
      dirty_region.top_left_x = x;
      dirty_region.top_left_y = y;
      dirty_region.bottom_right_x = x + width - 1;
      dirty_region.bottom_right_y = y + height - 1;
    } 
  else
    {
      if (x < dirty_region.top_left_x)
        dirty_region.top_left_x = x;
      if (y < dirty_region.top_left_y)
        dirty_region.top_left_y = y;
      if ((x + (int)width - 1) > dirty_region.bottom_right_x)
        dirty_region.bottom_right_x = x + width - 1;
      if ((y + (int)height - 1) > dirty_region.bottom_right_y)
        dirty_region.bottom_right_y = y + height - 1;
    }
} 

static void
dirty_region_add_virtualscreen (void)
{
  /* Mark virtual screen as dirty.  */
  dirty_region_add (virtual_screen.offset_x, virtual_screen.offset_y,
                    virtual_screen.width, virtual_screen.height);
}


static void
dirty_region_redraw (void)
{
  int x;
  int y;
  int width;
  int height;

  if (dirty_region_is_empty ())
    return;

  x = dirty_region.top_left_x;
  y = dirty_region.top_left_y;

  width = dirty_region.bottom_right_x - x + 1;
  height = dirty_region.bottom_right_y - y + 1;

  redraw_screen_rect (x, y, width, height);

  dirty_region_reset ();
}

static void
write_char (void)
{
  struct grub_colored_char *p;
  struct grub_font_glyph glyph;
  grub_video_color_t color;
  grub_video_color_t bgcolor;
  unsigned int x;
  unsigned int y;

  /* Find out active character.  */
  p = (virtual_screen.text_buffer
       + virtual_screen.cursor_x
       + (virtual_screen.cursor_y * virtual_screen.columns));

  p -= p->index;

  /* Get glyph for character.  */
  grub_font_get_glyph (p->code, &glyph);

  color = p->fg_color;
  bgcolor = p->bg_color;

  x = virtual_screen.cursor_x * virtual_screen.char_width;
  y = virtual_screen.cursor_y * virtual_screen.char_height;

  /* Render glyph to text layer.  */
  grub_video_set_active_render_target (text_layer);
  grub_video_fill_rect (bgcolor, x, y, glyph.width, glyph.height);
  grub_video_blit_glyph (&glyph, color, x, y);
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  /* Mark character to be drawn.  */
  dirty_region_add (virtual_screen.offset_x + x, virtual_screen.offset_y + y,
                    glyph.width, glyph.height);
}

static void
write_cursor (void)
{
  unsigned int x;
  unsigned int y;
  unsigned int width;
  unsigned int height;
  grub_video_color_t color;

  /* Determine cursor properties and position on text layer. */
  x = virtual_screen.cursor_x * virtual_screen.char_width;
  y = ((virtual_screen.cursor_y + 1) * virtual_screen.char_height) - 3;  
  width = virtual_screen.char_width;
  height = 2;

  color = virtual_screen.cursor_color;

  /* Render cursor to text layer.  */
  grub_video_set_active_render_target (text_layer);
  grub_video_fill_rect (color, x, y, width, height);
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  /* Mark cursor to be redrawn.  */
  dirty_region_add (virtual_screen.offset_x + x, virtual_screen.offset_y + y,
                    width, height);
}

static void
scroll_up (void)
{
  unsigned int i;
  grub_video_color_t color;

  /* If we don't have background bitmap, remove cursor. */
  if (!bitmap)
    {
      /* Remove cursor.  */
      write_char ();

      /* Redraw only changed regions.  */
      dirty_region_redraw ();
    }
  
  /* Scroll text buffer with one line to up.  */
  grub_memmove (virtual_screen.text_buffer,
                virtual_screen.text_buffer + virtual_screen.columns,
                sizeof (*virtual_screen.text_buffer)
                * virtual_screen.columns
                * (virtual_screen.rows - 1));

  /* Clear last line in text buffer.  */
  for (i = virtual_screen.columns * (virtual_screen.rows - 1);
       i < virtual_screen.columns * virtual_screen.rows;
       i++)
    {
      virtual_screen.text_buffer[i].code = ' ';
      virtual_screen.text_buffer[i].fg_color = virtual_screen.fg_color;
      virtual_screen.text_buffer[i].bg_color = virtual_screen.bg_color;
      virtual_screen.text_buffer[i].width = 0;
      virtual_screen.text_buffer[i].index = 0;
    }

  /* Scroll physical screen.  */
  grub_video_set_active_render_target (text_layer);
  color = virtual_screen.bg_color;
  grub_video_scroll (color, 0, -virtual_screen.char_height);
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);
  
  /* If we have bitmap, re-draw screen, otherwise scroll physical screen too.  */
  if (bitmap)
    {
      /* Mark virtual screen to be redrawn.  */
      dirty_region_add_virtualscreen ();
    }
  else
    {      
      /* Clear new border area.  */
      grub_video_fill_rect (color, 
                            virtual_screen.offset_x, virtual_screen.offset_y, 
                            virtual_screen.width, virtual_screen.char_height);
      
      /* Scroll physical screen.  */
      grub_video_scroll (color, 0, -virtual_screen.char_height);      

      /* Draw cursor if visible.  */
      if (virtual_screen.cursor_state)
        write_cursor ();
    }
}

static void
grub_gfxterm_putchar (grub_uint32_t c)
{
  if (c == '\a')
    /* FIXME */
    return;

  if (c == '\b' || c == '\n' || c == '\r')
    {
      /* Erase current cursor, if any.  */
      if (virtual_screen.cursor_state)
        write_char ();

      switch (c)
        {
        case '\b':
          if (virtual_screen.cursor_x > 0)
            virtual_screen.cursor_x--;
          break;

        case '\n':
          if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
            scroll_up ();
          else
            virtual_screen.cursor_y++;
          break;

        case '\r':
          virtual_screen.cursor_x = 0;
          break;
        }

      /* Redraw cursor if visible.  */
      if (virtual_screen.cursor_state)
        write_cursor ();
    }
  else
    {
      struct grub_font_glyph glyph;
      struct grub_colored_char *p;

      /* Get properties of the character.  */    
      grub_font_get_glyph (c, &glyph);

      /* If we are about to exceed line length, wrap to next line.  */
      if (virtual_screen.cursor_x + glyph.char_width > virtual_screen.columns)
        grub_putchar ('\n');

      /* Find position on virtual screen, and fill information.  */
      p = (virtual_screen.text_buffer +
           virtual_screen.cursor_x +
           virtual_screen.cursor_y * virtual_screen.columns);
      p->code = c;
      p->fg_color = virtual_screen.fg_color;
      p->bg_color = virtual_screen.bg_color;
      p->width = glyph.char_width - 1;
      p->index = 0;

      /* If we have large glyph, add fixup info.  */
      if (glyph.char_width > 1)
        {
          unsigned i;

          for (i = 1; i < glyph.char_width; i++)
            {
              p[i].code = ' ';
              p[i].width = glyph.char_width - 1;
              p[i].index = i;
            }
        }

      /* Draw glyph.  */
      write_char ();

      /* Make sure we scroll screen when needed and wrap line correctly.  */
      virtual_screen.cursor_x += glyph.char_width;
      if (virtual_screen.cursor_x >= virtual_screen.columns)
        {
          virtual_screen.cursor_x = 0;

          if (virtual_screen.cursor_y >= virtual_screen.rows - 1)
            scroll_up ();
          else
            virtual_screen.cursor_y++;
        }

      /* Draw cursor if visible.  */
      if (virtual_screen.cursor_state)
        write_cursor ();
    }
}

static grub_ssize_t
grub_gfxterm_getcharwidth (grub_uint32_t c)
{
  struct grub_font_glyph glyph;

  grub_font_get_glyph (c, &glyph);

  return glyph.char_width;
}

static grub_uint16_t
grub_virtual_screen_getwh (void)
{
  return (virtual_screen.columns << 8) | virtual_screen.rows;
}

static grub_uint16_t
grub_virtual_screen_getxy (void)
{
  return ((virtual_screen.cursor_x << 8) | virtual_screen.cursor_y);
}

static void
grub_gfxterm_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  if (x >= virtual_screen.columns)
    x = virtual_screen.columns - 1;

  if (y >= virtual_screen.rows)
    y = virtual_screen.rows - 1;

  if (virtual_screen.cursor_state)
    write_char ();

  virtual_screen.cursor_x = x;
  virtual_screen.cursor_y = y;

  if (virtual_screen.cursor_state)
    write_cursor ();
}

static void
grub_virtual_screen_cls (void)
{
  grub_uint32_t i;

  for (i = 0; i < virtual_screen.columns * virtual_screen.rows; i++)
    {
      virtual_screen.text_buffer[i].code = ' ';
      virtual_screen.text_buffer[i].fg_color = virtual_screen.fg_color;
      virtual_screen.text_buffer[i].bg_color = virtual_screen.bg_color;
      virtual_screen.text_buffer[i].width = 0;
      virtual_screen.text_buffer[i].index = 0;
    }

  virtual_screen.cursor_x = virtual_screen.cursor_y = 0;
}

static void
grub_gfxterm_cls (void)
{
  grub_video_color_t color;

  /* Clear virtual screen.  */
  grub_virtual_screen_cls ();

  /* Clear text layer.  */
  grub_video_set_active_render_target (text_layer);
  color = virtual_screen.bg_color;
  grub_video_fill_rect (color, 0, 0, mode_info.width, mode_info.height);
  grub_video_set_active_render_target (GRUB_VIDEO_RENDER_TARGET_DISPLAY);

  /* Mark virtual screen to be redrawn.  */
  dirty_region_add_virtualscreen ();
}

static void
grub_virtual_screen_setcolorstate (grub_term_color_state state)
{
  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
      virtual_screen.term_color = virtual_screen.standard_color_setting;
      break;
      
    case GRUB_TERM_COLOR_NORMAL:
      virtual_screen.term_color = virtual_screen.normal_color_setting;
      break;
      
    case GRUB_TERM_COLOR_HIGHLIGHT:
      virtual_screen.term_color = virtual_screen.highlight_color_setting;
      break;
      
    default:
      break;
    }

  /* Change color to virtual terminal.  */
  set_term_color (virtual_screen.term_color);
}

static void
grub_virtual_screen_setcolor (grub_uint8_t normal_color,
                              grub_uint8_t highlight_color)
{
  virtual_screen.normal_color_setting = normal_color;
  virtual_screen.highlight_color_setting = highlight_color;
}

static void
grub_virtual_screen_getcolor (grub_uint8_t *normal_color,
                              grub_uint8_t *highlight_color)
{
  *normal_color = virtual_screen.normal_color_setting;
  *highlight_color = virtual_screen.highlight_color_setting;
}

static void
grub_gfxterm_setcursor (int on)
{
  if (virtual_screen.cursor_state != on)
    {
      if (virtual_screen.cursor_state)
        write_char ();
      else
        write_cursor ();

      virtual_screen.cursor_state = on;
    }
}

static void
grub_gfxterm_refresh (void)
{
  /* Redraw only changed regions.  */
  dirty_region_redraw ();
}

static grub_err_t
grub_gfxterm_background_image_cmd (struct grub_arg_list *state __attribute__ ((unused)),
                                   int argc,
                                   char **args)
{
  /* Check that we have video adapter active.  */
  if (grub_video_get_info(NULL) != GRUB_ERR_NONE)
    return grub_errno;
  
  /* Destroy existing background bitmap if loaded.  */
  if (bitmap)
    {
      grub_video_bitmap_destroy (bitmap);
      bitmap = 0;
      
      /* Mark whole screen as dirty.  */
      dirty_region_reset ();
      dirty_region_add (0, 0, mode_info.width, mode_info.height);
    }

  /* If filename was provided, try to load that.  */
  if (argc >= 1)
    {
    /* Try to load new one.  */
    grub_video_bitmap_load (&bitmap, args[0]);    
    if (grub_errno != GRUB_ERR_NONE)
      return grub_errno;

    /* If bitmap was loaded correctly, display it.  */
    if (bitmap)
      {
        /* Determine bitmap dimensions.  */
        bitmap_width = grub_video_bitmap_get_width (bitmap);
        bitmap_height = grub_video_bitmap_get_width (bitmap);
        
        /* Mark whole screen as dirty.  */
        dirty_region_reset ();
        dirty_region_add (0, 0, mode_info.width, mode_info.height);
      }
    }
  
  /* All was ok.  */
  grub_errno = GRUB_ERR_NONE;
  return grub_errno;
}

static struct grub_term grub_video_term =
  {
    .name = "gfxterm",
    .init = grub_gfxterm_init,
    .fini = grub_gfxterm_fini,
    .putchar = grub_gfxterm_putchar,
    .getcharwidth = grub_gfxterm_getcharwidth,
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
    .getwh = grub_virtual_screen_getwh,
    .getxy = grub_virtual_screen_getxy,
    .gotoxy = grub_gfxterm_gotoxy,
    .cls = grub_gfxterm_cls,
    .setcolorstate = grub_virtual_screen_setcolorstate,
    .setcolor = grub_virtual_screen_setcolor,
    .getcolor = grub_virtual_screen_getcolor,
    .setcursor = grub_gfxterm_setcursor,
    .refresh = grub_gfxterm_refresh,
    .flags = 0,
    .next = 0
  };

GRUB_MOD_INIT(term_gfxterm)
{
  my_mod = mod;
  grub_term_register (&grub_video_term);

  grub_register_command ("background_image",
                         grub_gfxterm_background_image_cmd,
                         GRUB_COMMAND_FLAG_BOTH,
                         "background_image",
                         "Load background image for active terminal",
                         0);
}

GRUB_MOD_FINI(term_gfxterm)
{
  grub_unregister_command ("bgimage");
  grub_term_unregister (&grub_video_term);
}
