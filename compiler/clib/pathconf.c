#include <limits.h>	/* _POSIX_*_MAX */
#include <stdio.h>	/* FILENAME_MAX */
#include <unistd.h>	/* _PC_*	*/

/* just a hack for grub2 */
long pathconf(const char *path, int name)
{
    switch (name) {
    case _POSIX_NAME_MAX:
	return FILENAME_MAX;
    case _PC_PATH_MAX:
	return PATH_MAX;
    default:
	return -1;
    }
}
