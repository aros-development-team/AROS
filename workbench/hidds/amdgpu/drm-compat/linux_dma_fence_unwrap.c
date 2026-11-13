/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#include <linux/dma-fence-unwrap.h>
#include <linux/dma-fence-array.h>
#include <linux/dma-fence-chain.h>
#include <linux/slab.h>

static struct dma_fence *dma_fence_unwrap_array(struct dma_fence_unwrap *cursor)
{
    cursor->array = dma_fence_chain_contained(cursor->chain);
    cursor->index = 0;
    return dma_fence_array_first(cursor->array);
}

struct dma_fence *dma_fence_unwrap_first(struct dma_fence *head, struct dma_fence_unwrap *cursor)
{
    cursor->chain = dma_fence_get(head);
    return dma_fence_unwrap_array(cursor);
}

struct dma_fence *dma_fence_unwrap_next(struct dma_fence_unwrap *cursor)
{
    struct dma_fence *tmp;

    ++cursor->index;
    tmp = dma_fence_array_next(cursor->array, cursor->index);
    if (tmp)
        return tmp;

    dma_fence_put(cursor->chain);
    cursor->chain = NULL;
    return NULL;
}

int dma_fence_dedup_array(struct dma_fence **fences, int num_fences)
{
    int i, j, count = 0;

    for (i = 0; i < num_fences; ++i)
    {
        for (j = 0; j < count; ++j)
        {
            if (fences[j]->context == fences[i]->context)
                break;
        }
        if (j == count)
        {
            fences[count++] = fences[i];
        }
        else if (dma_fence_is_later(fences[i], fences[j]))
        {
            dma_fence_put(fences[j]);
            fences[j] = fences[i];
        }
        else
        {
            dma_fence_put(fences[i]);
        }
    }
    return count;
}

struct dma_fence *__dma_fence_unwrap_merge(unsigned int num_fences, struct dma_fence **fences,
                                           struct dma_fence_unwrap *iter)
{
    struct dma_fence_array *result;
    struct dma_fence *tmp, **array;
    unsigned int count = 0, i, j;

    for (i = 0; i < num_fences; ++i)
        dma_fence_unwrap_for_each(tmp, &iter[i], fences[i])
            if (!dma_fence_is_signaled(tmp))
                ++count;

    if (count == 0)
        return dma_fence_get_stub();

    array = kmalloc_array(count, sizeof(*array), GFP_KERNEL);
    if (!array)
        return NULL;

    count = 0;
    for (i = 0; i < num_fences; ++i)
    {
        dma_fence_unwrap_for_each(tmp, &iter[i], fences[i])
        {
            if (dma_fence_is_signaled(tmp))
                continue;
            for (j = 0; j < count; ++j)
            {
                if (array[j]->context == tmp->context)
                {
                    if (dma_fence_is_later(tmp, array[j]))
                    {
                        dma_fence_put(array[j]);
                        array[j] = dma_fence_get(tmp);
                    }
                    break;
                }
            }
            if (j == count)
                array[count++] = dma_fence_get(tmp);
        }
    }

    if (count == 1)
    {
        tmp = array[0];
        kfree(array);
        return tmp;
    }

    result = dma_fence_array_create(count, array, dma_fence_context_alloc(1), 1, false);
    if (!result)
    {
        for (i = 0; i < count; ++i)
            dma_fence_put(array[i]);
        kfree(array);
        return NULL;
    }
    return &result->base;
}
