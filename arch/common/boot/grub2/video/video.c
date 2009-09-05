/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/video.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/mm.h>

/* The list of video adapters registered to system.  */
static grub_video_adapter_t grub_video_adapter_list;

/* Active video adapter.  */
static grub_video_adapter_t grub_video_adapter_active;

/* Register video driver.  */
void
grub_video_register (grub_video_adapter_t adapter)
{
  adapter->next = grub_video_adapter_list;
  grub_video_adapter_list = adapter;
}

/* Unregister video driver.  */
void
grub_video_unregister (grub_video_adapter_t adapter)
{
  grub_video_adapter_t *p, q;

  for (p = &grub_video_adapter_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == adapter)
      {
        *p = q->next;
        break;
      }
}

/* Iterate thru all registered video drivers.  */
void
grub_video_iterate (int (*hook) (grub_video_adapter_t adapter))
{
  grub_video_adapter_t p;

  for (p = grub_video_adapter_list; p; p = p->next)
    if (hook (p))
      break;
}

/* Restore back to initial mode (where applicable).  */
grub_err_t
grub_video_restore (void)
{
  if (grub_video_adapter_active)
    {
      grub_video_adapter_active->fini ();
      if (grub_errno != GRUB_ERR_NONE)
        return grub_errno;

      grub_video_adapter_active = 0;
    }
  return GRUB_ERR_NONE;
}

/* Get information about active video mode.  */
grub_err_t
grub_video_get_info (struct grub_video_mode_info *mode_info)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  /* If mode_info is NULL just report that video adapter is active.  */
  if (! mode_info)
    {
      grub_errno = GRUB_ERR_NONE;
      return grub_errno;
    }

  return grub_video_adapter_active->get_info (mode_info);
}

/* Get information about active video mode.  */
grub_err_t
grub_video_get_info_and_fini (struct grub_video_mode_info *mode_info,
			      void **framebuffer)
{
  grub_err_t err;

  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  err = grub_video_adapter_active->get_info_and_fini (mode_info, framebuffer);
  if (err)
    return err;

  grub_video_adapter_active = 0;
  return GRUB_ERR_NONE;
}

/* Determine optimized blitting formation for specified video mode info.  */
enum grub_video_blit_format
grub_video_get_blit_format (struct grub_video_mode_info *mode_info)
{
  /* Check if we have any known 32 bit modes.  */
  if (mode_info->bpp == 32)
    {
      if ((mode_info->red_mask_size == 8)
	  && (mode_info->red_field_pos == 16)
	  && (mode_info->green_mask_size == 8)
	  && (mode_info->green_field_pos == 8)
	  && (mode_info->blue_mask_size == 8)
	  && (mode_info->blue_field_pos == 0))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_BGRA_8888;
	}
      else if ((mode_info->red_mask_size == 8)
	       && (mode_info->red_field_pos == 0)
	       && (mode_info->green_mask_size == 8)
	       && (mode_info->green_field_pos == 8)
	       && (mode_info->blue_mask_size == 8)
	       && (mode_info->blue_field_pos == 16))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_RGBA_8888;
	}
    }
  /* Check if we have any known 24 bit modes.  */
  else if (mode_info->bpp == 24)
    {
      if ((mode_info->red_mask_size == 8)
	  && (mode_info->red_field_pos == 16)
	  && (mode_info->green_mask_size == 8)
	  && (mode_info->green_field_pos == 8)
	  && (mode_info->blue_mask_size == 8)
	  && (mode_info->blue_field_pos == 0))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_BGR_888;
	}
      else if ((mode_info->red_mask_size == 8)
	       && (mode_info->red_field_pos == 0)
	       && (mode_info->green_mask_size == 8)
	       && (mode_info->green_field_pos == 8)
	       && (mode_info->blue_mask_size == 8)
	       && (mode_info->blue_field_pos == 16))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_RGB_888;
	}
    }
  /* Check if we have any known 16 bit modes.  */
  else if (mode_info->bpp == 16)
    {
      if ((mode_info->red_mask_size == 5)
	  && (mode_info->red_field_pos == 11)
	  && (mode_info->green_mask_size == 6)
	  && (mode_info->green_field_pos == 5)
	  && (mode_info->blue_mask_size == 5)
	  && (mode_info->blue_field_pos == 0))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_BGR_565;
	}
      else if ((mode_info->red_mask_size == 5)
	       && (mode_info->red_field_pos == 0)
	       && (mode_info->green_mask_size == 6)
	       && (mode_info->green_field_pos == 5)
	       && (mode_info->blue_mask_size == 5)
	       && (mode_info->blue_field_pos == 11))
	{
	  return GRUB_VIDEO_BLIT_FORMAT_RGB_565;
	}
    }
  else if (mode_info->bpp == 1)
    return GRUB_VIDEO_BLIT_FORMAT_1BIT_PACKED;

  /* Backup route.  Unknown format.  */

  /* If there are more than 8 bits per color, assume RGB(A) mode.  */
  if (mode_info->bpp > 8)
    {
      if (mode_info->reserved_mask_size > 0)
	{
	  return GRUB_VIDEO_BLIT_FORMAT_RGBA;
	}
      else
	{
	  return GRUB_VIDEO_BLIT_FORMAT_RGB;
	}
    }

  /* Assume as indexcolor mode.  */
  return GRUB_VIDEO_BLIT_FORMAT_INDEXCOLOR;
}

/* Set new indexed color palette entries.  */
grub_err_t
grub_video_set_palette (unsigned int start, unsigned int count,
                        struct grub_video_palette_data *palette_data)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->set_palette (start, count, palette_data);
}

/* Get indexed color palette entries.  */
grub_err_t
grub_video_get_palette (unsigned int start, unsigned int count,
                        struct grub_video_palette_data *palette_data)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->get_palette (start, count, palette_data);
}

/* Set viewport dimensions.  */
grub_err_t
grub_video_set_viewport (unsigned int x, unsigned int y,
                         unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->set_viewport (x, y, width, height);
}

/* Get viewport dimensions.  */
grub_err_t
grub_video_get_viewport (unsigned int *x, unsigned int *y,
                         unsigned int *width, unsigned int *height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->get_viewport (x, y, width, height);
}

/* Map color name to adapter specific color.  */
grub_video_color_t
grub_video_map_color (grub_uint32_t color_name)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_color (color_name);
}

/* Map RGB value to adapter specific color.  */
grub_video_color_t
grub_video_map_rgb (grub_uint8_t red, grub_uint8_t green, grub_uint8_t blue)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_rgb (red, green, blue);
}

/* Map RGBA value to adapter specific color.  */
grub_video_color_t
grub_video_map_rgba (grub_uint8_t red, grub_uint8_t green, grub_uint8_t blue,
                     grub_uint8_t alpha)
{
  if (! grub_video_adapter_active)
    return 0;

  return grub_video_adapter_active->map_rgba (red, green, blue, alpha);
}

/* Unmap video color back to RGBA components.  */
grub_err_t
grub_video_unmap_color (grub_video_color_t color, grub_uint8_t *red,
                        grub_uint8_t *green, grub_uint8_t *blue,
                        grub_uint8_t *alpha)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->unmap_color (color,
                                                 red,
                                                 green,
                                                 blue,
                                                 alpha);
}

/* Fill rectangle using specified color.  */
grub_err_t
grub_video_fill_rect (grub_video_color_t color, int x, int y,
                      unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->fill_rect (color, x, y, width, height);
}

/* Blit bitmap to screen.  */
grub_err_t
grub_video_blit_bitmap (struct grub_video_bitmap *bitmap,
                        enum grub_video_blit_operators oper,
                        int x, int y, int offset_x, int offset_y,
                        unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->blit_bitmap (bitmap, oper, x, y,
                                                 offset_x, offset_y,
                                                 width, height);
}

/* Blit render target to active render target.  */
grub_err_t
grub_video_blit_render_target (struct grub_video_render_target *target,
                               enum grub_video_blit_operators oper,
                               int x, int y, int offset_x, int offset_y,
                               unsigned int width, unsigned int height)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->blit_render_target (target, oper, x, y,
                                                        offset_x, offset_y,
                                                        width, height);
}

/* Scroll viewport and fill new areas with specified color.  */
grub_err_t
grub_video_scroll (grub_video_color_t color, int dx, int dy)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->scroll (color, dx, dy);
}

/* Swap buffers (swap active render target).  */
grub_err_t
grub_video_swap_buffers (void)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->swap_buffers ();
}

/* Create new render target.  */
grub_err_t
grub_video_create_render_target (struct grub_video_render_target **result,
                                 unsigned int width, unsigned int height,
                                 unsigned int mode_type)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->create_render_target (result,
                                                          width, height,
                                                          mode_type);
}

/* Delete render target.  */
grub_err_t
grub_video_delete_render_target (struct grub_video_render_target *target)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->delete_render_target (target);
}

/* Set active render target.  */
grub_err_t
grub_video_set_active_render_target (struct grub_video_render_target *target)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->set_active_render_target (target);
}

/* Get active render target.  */
grub_err_t
grub_video_get_active_render_target (struct grub_video_render_target **target)
{
  if (! grub_video_adapter_active)
    return grub_error (GRUB_ERR_BAD_DEVICE, "No video mode activated");

  return grub_video_adapter_active->get_active_render_target (target);
}

grub_err_t
grub_video_set_mode (const char *modestring,
		     int NESTED_FUNC_ATTR (*hook) (grub_video_adapter_t p,
						   struct grub_video_mode_info *mode_info))
{
  char *tmp;
  char *next_mode;
  char *current_mode;
  char *param;
  char *value;
  char *modevar;
  int width = -1;
  int height = -1;
  int depth = -1;
  int flags = 0;

  /* Take copy of env.var. as we don't want to modify that.  */
  modevar = grub_strdup (modestring);

  /* Initialize next mode.  */
  next_mode = modevar;

  if (! modevar)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "couldn't allocate space for local modevar copy");

  if (grub_memcmp (next_mode, "keep", sizeof ("keep")) == 0
      || grub_memcmp (next_mode, "keep,", sizeof ("keep,") - 1) == 0
      || grub_memcmp (next_mode, "keep;", sizeof ("keep;") - 1) == 0)
    {
      struct grub_video_mode_info mode_info;
      int suitable = 1;
      grub_err_t err;

      grub_memset (&mode_info, 0, sizeof (mode_info));

      if (grub_video_adapter_active)
	{
	  err = grub_video_get_info (&mode_info);
	  if (err)
	    {
	      suitable = 0;
	      grub_errno = GRUB_ERR_NONE;
	    }
	}
      else
	mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_PURE_TEXT;

      if (suitable && hook)
	suitable = hook (grub_video_adapter_active, &mode_info);
      if (suitable)
	{
	  grub_free (modevar);
	  return GRUB_ERR_NONE;
	}
      next_mode += sizeof ("keep") - 1;
      if (! *next_mode)
	{
	  grub_free (modevar);

	  return grub_error (GRUB_ERR_BAD_ARGUMENT,
			     "No suitable mode found.");
	}

      /* Skip separator. */
      next_mode++;
    }

  /* De-activate last set video adapter.  */
  if (grub_video_adapter_active)
    {
      /* Finalize adapter.  */
      grub_video_adapter_active->fini ();
      if (grub_errno != GRUB_ERR_NONE)
	grub_errno = GRUB_ERR_NONE;

      /* Mark active adapter as not set.  */
      grub_video_adapter_active = 0;
    }

  /* Loop until all modes has been tested out.  */
  while (next_mode != NULL)
    {
      /* Use last next_mode as current mode.  */
      tmp = next_mode;

      /* Reset video mode settings.  */
      width = -1;
      height = -1;
      depth = -1;
      flags = 0;

      /* Save position of next mode and separate modes.  */
      for (; *next_mode; next_mode++)
	if (*next_mode == ',' || *next_mode == ';')
	  break;
      if (*next_mode)
	{
	  *next_mode = 0;
	  next_mode++;
	}
      else
	next_mode = 0;

      /* Skip whitespace.  */
      while (grub_isspace (*tmp))
	tmp++;

      /* Initialize token holders.  */
      current_mode = tmp;
      param = tmp;
      value = NULL;

      /* XXX: we assume that we're in pure text mode if
	 no video mode is initialized. Is it always true? */
      if (grub_strcmp (param, "text") == 0)
	{
	  struct grub_video_mode_info mode_info;

	  grub_memset (&mode_info, 0, sizeof (mode_info));
	  mode_info.mode_type = GRUB_VIDEO_MODE_TYPE_PURE_TEXT;

	  if (! hook || hook (0, &mode_info))
	    {
	      /* Valid mode found from adapter, and it has been activated.
		 Specify it as active adapter.  */
	      grub_video_adapter_active = NULL;

	      /* Free memory.  */
	      grub_free (modevar);

	      return GRUB_ERR_NONE;
	    }
	}

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
      grub_video_adapter_t p;

      /* Loop thru all possible video adapter trying to find requested mode.  */
      for (p = grub_video_adapter_list; p; p = p->next)
	{
	  grub_err_t err;
	  struct grub_video_mode_info mode_info;

	  grub_memset (&mode_info, 0, sizeof (mode_info));

	  /* Try to initialize adapter, if it fails, skip to next adapter.  */
	  err = p->init ();
	  if (err != GRUB_ERR_NONE)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  /* Try to initialize video mode.  */
	  err = p->setup (width, height, flags);
	  if (err != GRUB_ERR_NONE)
	    {
	      p->fini ();
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  err = p->get_info (&mode_info);
	  if (err != GRUB_ERR_NONE)
	    {
	      p->fini ();
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  if (hook && ! hook (p, &mode_info))
	    {
	      p->fini ();
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }

	  /* Valid mode found from adapter, and it has been activated.
	     Specify it as active adapter.  */
	  grub_video_adapter_active = p;

	  /* Free memory.  */
	  grub_free (modevar);

	  return GRUB_ERR_NONE;
	}

    }

  /* Free memory.  */
  grub_free (modevar);

  return grub_error (GRUB_ERR_BAD_ARGUMENT,
		     "No suitable mode found.");
}

/* Initialize Video API module.  */
GRUB_MOD_INIT(video_video)
{
}

/* Finalize Video API module.  */
GRUB_MOD_FINI(video_video)
{
}
