#ifndef ASM_ARM_MMU_H
#define ASM_ARM_MMU_H

/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <inttypes.h>

typedef struct pte_ {
    union {
        struct {
            unsigned type:2;
            unsigned sbz:3;
            unsigned domain:4;
            unsigned imp:1;
            unsigned base_address:22;
        } coarse;
        struct {
            unsigned type:2;
            unsigned b:1;
            unsigned c:1;
            unsigned xn:1;
            unsigned domain:4;
            unsigned imp:1;
            unsigned ap:2;
            unsigned tex:3;
            unsigned apx:1;
            unsigned s:1;
            unsigned ng:1;
            unsigned supersection:1;
            unsigned sbz:1;
            unsigned base_address:12;
        } section;
        struct {
            unsigned type:2;
            unsigned b:1;
            unsigned c:1;
            unsigned xn:1;
            unsigned base_address_3:4;
            unsigned imp:1;
            unsigned ap:2;
            unsigned tex:3;
            unsigned apx:1;
            unsigned s:1;
            unsigned ng:1;
            unsigned supersection:1;
            unsigned sbz:1;
            unsigned base_address_2:4;
            unsigned base_address_1:8;
        } supersection;
        uint32_t    raw;
    };
} pde_t;

typedef struct {
    union {
        struct {
            unsigned type:2;
            unsigned b:1;
            unsigned c:1;
            unsigned ap:2;
            unsigned sbz:3;
            unsigned apx:1;
            unsigned s:1;
            unsigned ng:1;
            unsigned tex:3;
            unsigned xn:1;
            unsigned base_address:16;
        } large_page;
        struct {
            unsigned type:2;
            unsigned b:1;
            unsigned c:1;
            unsigned ap:2;
            unsigned tex:3;
            unsigned apx:1;
            unsigned s:1;
            unsigned ng:1;
            unsigned base_address:20;
        } page;
        uint32_t    raw;
    };
} pte_t;

#define PDE_TYPE_FAULT          0
#define PDE_TYPE_COARSE         1
#define PDE_TYPE_SECTION        2
#define PDE_TYPE_RESERVED       3

#define PTE_TYPE_FAULT          0
#define PTE_TYPE_LARGE_PAGE     1
#define PTE_TYPE_PAGE           2
#define PTE_TYPE_PAGE_XN        3


#endif /* ASM_ARM_MMU_H */
