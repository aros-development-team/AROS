#include <devices/diskimage.h>

extern struct DiskImagePluginTable plugin_table;

void *LibNull(void)
{
    return &plugin_table;
}
