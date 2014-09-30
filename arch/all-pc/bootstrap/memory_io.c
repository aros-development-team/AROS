/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <elfloader.h>
#include <string.h>

void *open_file(struct ELFNode *n, unsigned int *err)
{
    /*
     * Our files are already loaded into memory as raw data.
     * Return a pointer to the beginning of the file.
     * NULL pointer is a valid result here (on EFI machines the first
     * module can be located at address 0). This is why error code
     * is a separate value here.
     */
    *err = 0;
    return n->eh;
}

void close_file(void *file)
{
    /* No special action is needed */
}

int read_block(void *file, unsigned long offset, void *dest, unsigned long length)
{
    memcpy(dest, file + offset, length);
    return 0;
}

void *load_block(void *file, unsigned long offset, unsigned long length, unsigned int *err)
{
    *err = 0;
    return file + offset;
}

void free_block(void *addr)
{
    /* No special action is needed */
}
