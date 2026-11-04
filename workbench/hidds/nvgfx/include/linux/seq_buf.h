/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_SEQ_BUF_H_
#define _LINUX_SEQ_BUF_H_

struct seq_buf { char *buffer; size_t size; size_t len; };
static inline void seq_buf_init(struct seq_buf *s, char *buf, unsigned int size) { s->buffer = buf; s->size = size; s->len = 0; }
#define seq_buf_printf(s, fmt, ...) (0)
#define seq_buf_puts(s, str) do { } while (0)
#define seq_buf_putc(s, c) do { } while (0)
#define seq_buf_used(s) ((s)->len)
#define seq_buf_str(s) ((s)->buffer)
#define seq_buf_has_overflowed(s) (0)
#define DECLARE_SEQ_BUF(n, sz) char __##n##_buf[sz]; struct seq_buf n = { .buffer = __##n##_buf, .size = sz }

#endif /* _LINUX_SEQ_BUF_H_ */
