/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KFIFO_H_
#define _LINUX_KFIFO_H_

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/stddef.h>
#include <linux/scatterlist.h>

#define DECLARE_KFIFO(fifo, type, size) \
    struct { unsigned int in, out; type buf[size]; } fifo
#define INIT_KFIFO(fifo)        do { (fifo).in = (fifo).out = 0; } while (0)
#define kfifo_size(fifo)        (ARRAY_SIZE((fifo)->buf))
#define kfifo_len(fifo)         ((fifo)->in - (fifo)->out)
#define kfifo_is_empty(fifo)    ((fifo)->in == (fifo)->out)
#define kfifo_is_full(fifo)     (kfifo_len(fifo) >= kfifo_size(fifo))
#define kfifo_reset(fifo)       do { (fifo)->in = (fifo)->out = 0; } while (0)
#define kfifo_put(fifo, val) ({ \
    typeof(fifo) __f = (fifo); unsigned int __r = !kfifo_is_full(__f); \
    if (__r) { __f->buf[__f->in % kfifo_size(__f)] = (val); __f->in++; } \
    __r; })
#define kfifo_get(fifo, val) ({ \
    typeof(fifo) __f = (fifo); unsigned int __r = !kfifo_is_empty(__f); \
    if (__r) { *(val) = __f->buf[__f->out % kfifo_size(__f)]; __f->out++; } \
    __r; })

#endif /* _LINUX_KFIFO_H_ */
