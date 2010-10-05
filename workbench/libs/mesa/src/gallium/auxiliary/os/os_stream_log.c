/**************************************************************************
 *
 * Copyright 2008-2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Debug logging stream implementation.
 */

#include "os_memory.h"
#include "os_misc.h"
#include "os_stream.h"


static void
os_log_stream_close(struct os_stream *stream)
{
   (void)stream;
}


static boolean
os_log_stream_write(struct os_stream *stream, const void *data, size_t size)
{
   char *str;

   str = os_malloc(size + 1);
   if (!str)
      return FALSE;

   memcpy(str, data, size);
   str[size] = 0;

   os_log_message(str);

   os_free(str);

   return TRUE;
}


static void
os_log_stream_flush(struct os_stream *stream)
{
   (void)stream;
}


static struct os_stream
os_log_stream_struct = {
   &os_log_stream_close,
   &os_log_stream_write,
   &os_log_stream_flush,
   &os_default_stream_vprintf,
};


struct os_stream *
os_log_stream = &os_log_stream_struct;
