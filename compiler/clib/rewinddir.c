#include <dirent.h>

void rewinddir(DIR *dir)
{
    seekdir(dir, 0);
}
