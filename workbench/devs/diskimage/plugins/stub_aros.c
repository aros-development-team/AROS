#include <devices/diskimage.h>
#include <aros/system.h>

extern struct DiskImagePluginTable plugin_table;

/* __startup will put code at start of plugin */
__startup void *LibNull(void)
{
    return &plugin_table;
}
