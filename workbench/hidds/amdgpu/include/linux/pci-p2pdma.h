/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_PCI_P2PDMA_H_
#define _LINUX_PCI_P2PDMA_H_

#include <linux/pci.h>

static inline int pci_p2pdma_distance(struct pci_dev *provider, struct device *client, bool verbose) { return -1; }

#endif
