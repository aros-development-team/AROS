/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_HMM_H_
#define _LINUX_HMM_H_


#include <linux/mm.h>

struct mmu_interval_notifier;

struct hmm_range {
    struct mmu_interval_notifier *notifier;
    unsigned long notifier_seq;
    unsigned long start;
    unsigned long end;
    unsigned long *hmm_pfns;
    unsigned long default_flags;
    unsigned long pfn_flags_mask;
    void *dev_private_owner;
};

static inline struct page *hmm_pfn_to_page(unsigned long hmm_pfn)
{
    return NULL;
}

#endif /* _LINUX_HMM_H_ */
