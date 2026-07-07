/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2021 Mellanox Technologies, Ltd.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <aros/debug.h>

#include <drm-compat/drm_compat_mem.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <string.h>
/* Undo Linux compat changes. */
#undef RB_ROOT
#undef file
#undef cdev
#define	RB_ROOT(head)	(head)->rbh_root

int
panic_cmp(struct rb_node *one, struct rb_node *two)
{
	bug("no cmp in rbtree");
}

RB_GENERATE(linux_root, rb_node, __entry, panic_cmp);

#include <stdio.h>

static char *
devm_kvasprintf(struct device *dev, gfp_t gfp, const char *fmt, va_list ap)
{
	unsigned int len;
	char *p;
	va_list aq;

	va_copy(aq, ap);
	len = vsnprintf(NULL, 0, fmt, aq);
	va_end(aq);

	p = kmalloc(len + 1, gfp);
	if (p != NULL)
		vsnprintf(p, len + 1, fmt, ap);

	return (p);
}

char *
kvasprintf(gfp_t gfp, const char *fmt, va_list ap)
{

	return (devm_kvasprintf(NULL, gfp, fmt, ap));
}

char *
kasprintf(gfp_t gfp, const char *fmt, ...)
{
	va_list ap;
	char *p;

	va_start(ap, fmt);
	p = kvasprintf(gfp, fmt, ap);
	va_end(ap);

	return (p);
}

void
list_sort(void *priv, struct list_head *head, int (*cmp)(void *priv,
    struct list_head *a, struct list_head *b))
{
    struct list_head *p, *q;
    int swapped;

    if (list_empty(head) || list_is_singular(head))
        return;

    do {
        swapped = 0;
        list_for_each(p, head) {
            q = p->next;
            if (q == head) break;
            if (cmp(priv, p, q) > 0) {
                list_del(p);
                list_add(p, q);   /* insert p after q */
                p = q;            /* advance correctly after swap */
                swapped = 1;
            }
        }
    } while (swapped);
}

void *
krealloc(const void *ptr, size_t size, gfp_t flags)
{
	void *nptr;
	size_t osize;

	/*
	 * First handle invariants based on function arguments.
	 */
	if (ptr == NULL)
		return (kmalloc(size, flags));

	if (size == 0) {
#if !defined(__AROS__)
		kfree(ptr);
		return (ZERO_SIZE_PTR);
#else
bug("FIXME: krealloc: implemented ZERO_SIZE_PTR\n");
		size = 1;
#endif
	}

	osize = ksize(ptr);
	if (size <= osize)
		return (__DECONST(void *, ptr));

	nptr = kmalloc(size, flags);
	if (nptr == NULL)
		return (NULL);

	memcpy(nptr, ptr, osize);
	kfree(ptr);
	return (nptr);
}
