/* Export pnvram and some variables for runtime */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/file.h>
#include <grub/err.h>
#include <grub/normal.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/efiemu/efiemu.h>
#include <grub/efiemu/runtime.h>
#include <grub/extcmd.h>

/* Place for final location of variables */
static int nvram_handle = 0;
static int nvramsize_handle = 0;
static int high_monotonic_count_handle = 0;
static int timezone_handle = 0;
static int accuracy_handle = 0;
static int daylight_handle = 0;

/* Temporary place */
static grub_uint8_t *nvram;
static grub_size_t nvramsize;
static grub_uint32_t high_monotonic_count;
static grub_int16_t timezone;
static grub_uint8_t daylight;
static grub_uint32_t accuracy;

static const struct grub_arg_option options[] = {
  {"size", 's', 0, "number of bytes to reserve for pseudo NVRAM", 0,
   ARG_TYPE_INT},
  {"high-monotonic-count", 'm', 0,
   "Initial value of high monotonic count", 0, ARG_TYPE_INT},
  {"timezone", 't', 0,
   "Timezone, offset in minutes from GMT", 0, ARG_TYPE_INT},
  {"accuracy", 'a', 0,
   "Accuracy of clock, in 1e-12 units", 0, ARG_TYPE_INT},
  {"daylight", 'd', 0,
   "Daylight value, as per EFI specifications", 0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

/* Parse signed value */
static int
grub_strtosl (char *arg, char **end, int base)
{
  if (arg[0] == '-')
    return -grub_strtoul (arg + 1, end, base);
  return grub_strtoul (arg, end, base);
}

/* Export stuff for efiemu */
static grub_err_t
nvram_set (void * data __attribute__ ((unused)))
{
  /* Take definitive pointers */
  grub_uint8_t *nvram_def = grub_efiemu_mm_obtain_request (nvram_handle);
  grub_uint32_t *nvramsize_def
    = grub_efiemu_mm_obtain_request (nvramsize_handle);
  grub_uint32_t *high_monotonic_count_def
    = grub_efiemu_mm_obtain_request (high_monotonic_count_handle);
  grub_int16_t *timezone_def
    = grub_efiemu_mm_obtain_request (timezone_handle);
  grub_uint8_t *daylight_def
    = grub_efiemu_mm_obtain_request (daylight_handle);
  grub_uint32_t *accuracy_def
    = grub_efiemu_mm_obtain_request (accuracy_handle);

  /* Copy to definitive loaction */
  grub_dprintf ("efiemu", "preparing pnvram\n");
  grub_memcpy (nvram_def, nvram, nvramsize);
  *nvramsize_def = nvramsize;
  *high_monotonic_count_def = high_monotonic_count;
  *timezone_def = timezone;
  *daylight_def = daylight;
  *accuracy_def = accuracy;

  /* Register symbols */
  grub_efiemu_register_symbol ("efiemu_variables", nvram_handle, 0);
  grub_efiemu_register_symbol ("efiemu_varsize", nvramsize_handle, 0);
  grub_efiemu_register_symbol ("efiemu_high_monotonic_count",
			       high_monotonic_count_handle, 0);
  grub_efiemu_register_symbol ("efiemu_time_zone", timezone_handle, 0);
  grub_efiemu_register_symbol ("efiemu_time_daylight", daylight_handle, 0);
  grub_efiemu_register_symbol ("efiemu_time_accuracy",
			       accuracy_handle, 0);

  return GRUB_ERR_NONE;
}

static void
nvram_unload (void * data __attribute__ ((unused)))
{
  grub_efiemu_mm_return_request (nvram_handle);
  grub_efiemu_mm_return_request (nvramsize_handle);
  grub_efiemu_mm_return_request (high_monotonic_count_handle);
  grub_efiemu_mm_return_request (timezone_handle);
  grub_efiemu_mm_return_request (accuracy_handle);
  grub_efiemu_mm_return_request (daylight_handle);

  grub_free (nvram);
  nvram = 0;
}

/* Load the variables file It's in format
   guid1:attr1:name1:data1;
   guid2:attr2:name2:data2;
   ...
   Where all fields are in hex
*/
static grub_err_t
read_pnvram (char *filename)
{
  char *buf, *ptr, *ptr2;
  grub_file_t file;
  grub_size_t size;
  grub_uint8_t *nvramptr = nvram;
  struct efi_variable *efivar;
  grub_size_t guidlen, datalen;
  unsigned i, j;

  file = grub_file_open (filename);
  if (!file)
    return grub_error (GRUB_ERR_BAD_OS, "couldn't read pnvram");
  size = grub_file_size (file);
  buf = grub_malloc (size + 1);
  if (!buf)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY, "couldn't read pnvram");
  if (grub_file_read (file, buf, size) != (grub_ssize_t) size)
    return grub_error (GRUB_ERR_BAD_OS, "couldn't read pnvram");
  buf[size] = 0;
  grub_file_close (file);

  for (ptr = buf; *ptr; )
    {
      if (grub_isspace (*ptr))
	{
	  ptr++;
	  continue;
	}

      efivar = (struct efi_variable *) nvramptr;
      if (nvramptr - nvram + sizeof (struct efi_variable) > nvramsize)
	return grub_error (GRUB_ERR_OUT_OF_MEMORY,
			   "file is too large for reserved variable space");

      nvramptr += sizeof (struct efi_variable);

      /* look ahow long guid field is*/
      guidlen = 0;
      for (ptr2 = ptr; (grub_isspace (*ptr2)
			|| (*ptr2 >= '0' && *ptr2 <= '9')
			|| (*ptr2 >= 'a' && *ptr2 <= 'f')
			|| (*ptr2 >= 'A' && *ptr2 <= 'F'));
	   ptr2++)
	if (!grub_isspace (*ptr2))
	  guidlen++;
      guidlen /= 2;

      /* Read guid */
      if (guidlen != sizeof (efivar->guid))
	{
	  grub_free (buf);
	  return grub_error (GRUB_ERR_BAD_OS, "can't parse %s", filename);
	}
      for (i = 0; i < 2 * sizeof (efivar->guid); i++)
	{
	  int hex = 0;
	  while (grub_isspace (*ptr))
	    ptr++;
	  if (*ptr >= '0' && *ptr <= '9')
	    hex = *ptr - '0';
	  if (*ptr >= 'a' && *ptr <= 'f')
	    hex = *ptr - 'a' + 10;
	  if (*ptr >= 'A' && *ptr <= 'F')
	    hex = *ptr - 'A' + 10;

	  if (i%2 == 0)
	    ((grub_uint8_t *)&(efivar->guid))[i/2] = hex << 4;
	  else
	    ((grub_uint8_t *)&(efivar->guid))[i/2] |= hex;
	  ptr++;
	}

      while (grub_isspace (*ptr))
	ptr++;
      if (*ptr != ':')
	{
	  grub_dprintf ("efiemu", "Not colon\n");
	  grub_free (buf);
	  return grub_error (GRUB_ERR_BAD_OS, "can't parse %s", filename);
	}
      ptr++;
      while (grub_isspace (*ptr))
	ptr++;

      /* Attributes can be just parsed by existing functions */
      efivar->attributes = grub_strtoul (ptr, &ptr, 16);

      while (grub_isspace (*ptr))
	ptr++;
      if (*ptr != ':')
	{
	  grub_dprintf ("efiemu", "Not colon\n");
	  grub_free (buf);
	  return grub_error (GRUB_ERR_BAD_OS, "can't parse %s", filename);
	}
      ptr++;
      while (grub_isspace (*ptr))
	ptr++;

      /* Read name and value */
      for (j = 0; j < 2; j++)
	{
	  /* Look the length */
	  datalen = 0;
	  for (ptr2 = ptr; *ptr2 && (grub_isspace (*ptr2)
				     || (*ptr2 >= '0' && *ptr2 <= '9')
				     || (*ptr2 >= 'a' && *ptr2 <= 'f')
				     || (*ptr2 >= 'A' && *ptr2 <= 'F'));
	       ptr2++)
	    if (!grub_isspace (*ptr2))
	      datalen++;
	  datalen /= 2;

	  if (nvramptr - nvram + datalen > nvramsize)
	    {
	      grub_free (buf);
	      return grub_error (GRUB_ERR_OUT_OF_MEMORY,
				 "file is too large for reserved "
				 " variable space");
	    }

	  for (i = 0; i < 2 * datalen; i++)
	    {
	      int hex = 0;
	      while (grub_isspace (*ptr))
		ptr++;
	      if (*ptr >= '0' && *ptr <= '9')
		hex = *ptr - '0';
	      if (*ptr >= 'a' && *ptr <= 'f')
		hex = *ptr - 'a' + 10;
	      if (*ptr >= 'A' && *ptr <= 'F')
		hex = *ptr - 'A' + 10;

	      if (i%2 == 0)
		nvramptr[i/2] = hex << 4;
	      else
		nvramptr[i/2] |= hex;
	      ptr++;
	    }
	  nvramptr += datalen;
	  while (grub_isspace (*ptr))
	    ptr++;
	  if (*ptr != (j ? ';' : ':'))
	    {
	      grub_free (buf);
	      grub_dprintf ("efiemu", j?"Not semicolon\n":"Not colon\n");
	      return grub_error (GRUB_ERR_BAD_OS, "can't parse %s", filename);
	    }
	  if (j)
	    efivar->size = datalen;
	  else
	    efivar->namelen = datalen;

	  ptr++;
	}
    }
  grub_free (buf);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_efiemu_make_nvram (void)
{
  grub_err_t err;

  err = grub_efiemu_autocore ();
  if (err)
    {
      grub_free (nvram);
      return err;
    }

  err = grub_efiemu_register_prepare_hook (nvram_set, nvram_unload, 0);
  if (err)
    {
      grub_free (nvram);
      return err;
    }
  nvram_handle
    = grub_efiemu_request_memalign (1, nvramsize,
				    GRUB_EFI_RUNTIME_SERVICES_DATA);
  nvramsize_handle
    = grub_efiemu_request_memalign (1, sizeof (grub_uint32_t),
				    GRUB_EFI_RUNTIME_SERVICES_DATA);
  high_monotonic_count_handle
    = grub_efiemu_request_memalign (1, sizeof (grub_uint32_t),
				    GRUB_EFI_RUNTIME_SERVICES_DATA);
  timezone_handle
    = grub_efiemu_request_memalign (1, sizeof (grub_uint16_t),
				    GRUB_EFI_RUNTIME_SERVICES_DATA);
  daylight_handle
    = grub_efiemu_request_memalign (1, sizeof (grub_uint8_t),
				    GRUB_EFI_RUNTIME_SERVICES_DATA);
  accuracy_handle
    = grub_efiemu_request_memalign (1, sizeof (grub_uint32_t),
				    GRUB_EFI_RUNTIME_SERVICES_DATA);

  grub_efiemu_request_symbols (6);
  return GRUB_ERR_NONE;
}

grub_err_t
grub_efiemu_pnvram (void)
{
  if (nvram)
    return GRUB_ERR_NONE;

  nvramsize = 2048;
  high_monotonic_count = 1;
  timezone = GRUB_EFI_UNSPECIFIED_TIMEZONE;
  accuracy = 50000000;
  daylight = 0;

  nvram = grub_zalloc (nvramsize);
  if (!nvram)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "Couldn't allocate space for temporary pnvram storage");

  return grub_efiemu_make_nvram ();
}

static grub_err_t
grub_cmd_efiemu_pnvram (struct grub_extcmd *cmd,
			int argc, char **args)
{
  struct grub_arg_list *state = cmd->state;
  grub_err_t err;

  if (argc > 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "only one argument expected");

  nvramsize = state[0].set ? grub_strtoul (state[0].arg, 0, 0) : 2048;
  high_monotonic_count = state[1].set ? grub_strtoul (state[1].arg, 0, 0) : 1;
  timezone = state[2].set ? grub_strtosl (state[2].arg, 0, 0)
    : GRUB_EFI_UNSPECIFIED_TIMEZONE;
  accuracy = state[3].set ? grub_strtoul (state[3].arg, 0, 0) : 50000000;
  daylight = state[4].set ? grub_strtoul (state[4].arg, 0, 0) : 0;

  nvram = grub_zalloc (nvramsize);
  if (!nvram)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "Couldn't allocate space for temporary pnvram storage");

  if (argc == 1 && (err = read_pnvram (args[0])))
    {
      grub_free (nvram);
      return err;
    }
  return grub_efiemu_make_nvram ();
}

static grub_extcmd_t cmd;

void grub_efiemu_pnvram_cmd_register (void);
void grub_efiemu_pnvram_cmd_unregister (void);

void
grub_efiemu_pnvram_cmd_register (void)
{
  cmd = grub_register_extcmd ("efiemu_pnvram", grub_cmd_efiemu_pnvram,
			      GRUB_COMMAND_FLAG_BOTH,
			      "efiemu_pnvram [FILENAME]",
			      "Initialise pseudo-NVRAM and load variables "
			      "from FILE",
			      options);
}

void
grub_efiemu_pnvram_cmd_unregister (void)
{
  grub_unregister_extcmd (cmd);
}
