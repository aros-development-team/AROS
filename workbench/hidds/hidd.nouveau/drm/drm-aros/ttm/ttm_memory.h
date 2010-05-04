/**************************************************************************
 *
 * Copyright (c) 2006-2009 VMware, Inc., Palo Alto, CA., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef TTM_MEMORY_H
#define TTM_MEMORY_H

struct ttm_mem_global
{
    int dummy;
};

static inline int ttm_mem_global_init(struct ttm_mem_global *glob)
{
    (void)glob;
    return 0;
}

static inline void ttm_mem_global_release(struct ttm_mem_global *glob)
{
    (void)glob;
}

static inline size_t ttm_round_pot(size_t size)
{
    if ((size & (size - 1)) == 0)
        return size;
    else if (size > PAGE_SIZE)
        return (size_t)PAGE_ALIGN(size);
    else {
        size_t tmp_size = 4;

        while (tmp_size < size)
            tmp_size <<= 1;

        return tmp_size;
    }
    return 0;    
}

#endif
