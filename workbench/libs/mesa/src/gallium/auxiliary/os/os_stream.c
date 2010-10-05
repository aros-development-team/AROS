/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_config.h"

#include "os_stream.h"
#include "util/u_memory.h"
#include "util/u_string.h"

int
os_default_stream_vprintf (struct os_stream* stream, const char *format, va_list ap)
{
   char buf[1024];
   int retval;
   va_list ap2;
   va_copy(ap2, ap);
   retval = util_vsnprintf(buf, sizeof(buf), format, ap2);
   va_end(ap2);
   if(retval <= 0)
   {}
   else if(retval < sizeof(buf))
      stream->write(stream, buf, retval);
   else
   {
      char* str = MALLOC(retval + 1);
      if(!str)
         return -1;
      retval = util_vsnprintf(str, retval + 1, format, ap);
      if(retval > 0)
         stream->write(stream, str, retval);
      FREE(str);
   }

   return retval;
}
