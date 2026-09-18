#include <stdarg.h>
#include "genmodule.h"

char *make_output_path(const char *format, ...)
{
    va_list args;
    char *path;
    int length;
    size_t size;

    va_start(args, format);
    length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (length < 0)
    {
        fprintf(stderr, "Could not format output path\n");
        exit(20);
    }

    size = (size_t)length + 1;
    path = malloc(size);
    if (path == NULL)
    {
        fprintf(stderr, "Out of memory\n");
        exit(20);
    }

    va_start(args, format);
    if (vsnprintf(path, size, format, args) != length)
    {
        va_end(args);
        free(path);
        fprintf(stderr, "Could not format output path\n");
        exit(20);
    }
    va_end(args);

    return path;
}

void generate_argtype_name_part(FILE *out, int argtype, int consecutive_args)
{
    if (argtype == TYPE_DOUBLE) {
        fprintf(out, "DOUBLE%d", consecutive_args);
    } else if (argtype == TYPE_QUAD) {
        fprintf(out, "QUAD%d", consecutive_args);
    } else {
        // This will look odd if normal args are last. Don't put them last!
        fprintf(out, "%d", consecutive_args);
    }
}
