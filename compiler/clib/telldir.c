#include <dirent.h>

off_t telldir(const DIR *dir)
{
    return dir->pos;
}
