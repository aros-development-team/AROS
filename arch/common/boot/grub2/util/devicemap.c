#include <stdio.h>

#include <grub/util/deviceiter.h>

void
grub_util_emit_devicemap_entry (FILE *fp, char *name, int is_floppy,
				int *num_fd, int *num_hd)
{
    if (is_floppy)
      fprintf (fp, "(fd%d)\t%s\n", (*num_fd)++, name);
    else
      fprintf (fp, "(hd%d)\t%s\n", (*num_hd)++, name);
}
