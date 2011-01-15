#include <stdio.h>

/* hack for grub2 */
off_t ftello(FILE *stream)
{
    return ftell(stream);
}
