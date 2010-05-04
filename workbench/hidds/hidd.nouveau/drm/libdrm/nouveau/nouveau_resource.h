/*
 * Copyright 2007 Nouveau Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NOUVEAU_RESOURCE_H__
#define __NOUVEAU_RESOURCE_H__

struct nouveau_resource {
	struct nouveau_resource *prev;
	struct nouveau_resource *next;

	int in_use;
	void *priv;

	unsigned int start;
	unsigned int size;
};

int
nouveau_resource_init(struct nouveau_resource **heap, unsigned start,
		      unsigned size);

void
nouveau_resource_destroy(struct nouveau_resource **heap);

int
nouveau_resource_alloc(struct nouveau_resource *heap, unsigned size, void *priv,
		       struct nouveau_resource **);

void
nouveau_resource_free(struct nouveau_resource **);

#endif
