/*
 * support.h
 *
 *  Created on: Aug 13, 2008
 *      Author: misc
 */

#ifndef SUPPORT_H_
#define SUPPORT_H_

#include <inttypes.h>
#include <stddef.h>

typedef struct __node {
	struct __node 	*n_succ,
					*n_pred;
} node_t;

typedef struct __list {
    node_t 	*l_head,
			*l_tail,
			*l_tailpred;
} list_t;

struct of_region
{
    uint8_t *base;
    int32_t size;
};

static inline void new_list(list_t *l)
{
	l->l_tailpred = (node_t *)l;
	l->l_tail	  = NULL;
	l->l_head	  = (node_t *)&l->l_tail;
}

static inline void add_head(list_t *l, node_t *n)
{
	n->n_succ = l->l_head;
	n->n_pred = (node_t *)&l->l_head;

	l->l_head->n_pred = n;
	l->l_head = n;
}

static inline void add_tail(list_t *l, node_t *n)
{
	n->n_succ = (node_t *)&l->l_tail;
	n->n_pred = l->l_tailpred;

	l->l_tailpred->n_succ = n;
	l->l_tailpred = n;
}

static inline node_t *remove(node_t *n)
{
	n->n_pred->n_succ = n->n_succ;
	n->n_succ->n_pred = n->n_pred;

	return n;
}

typedef struct {
	node_t 		m_node;
	char        *m_name;
	char        *m_str;
	intptr_t    m_lowest;
	intptr_t    m_highest;
	list_t		m_symbols;
} module_t;

typedef struct {
	node_t		s_node;
	char        *s_name;
	intptr_t    s_lowest;
	intptr_t    s_highest;
} symbol_t;

extern void *stdin;
extern void *stdout;
extern void *stderr;

int32_t strlen(const char *c);
int isblank(char c);
int isspace(char c);
int isdigit(char c);
int tolower(char c);
int strncasecmp(const char *s1, const char *s2, int max);
int strcasecmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int max);
void bzero(void *dest, int length);
void memcpy(void *dest, const void *src, int length);
char *remove_path(const char *in);

void printf(char *str, ...);
void sprintf(char *dest, char *str, ...);

#define LOAD_SIZE	(0x00500000)

#define KERNEL_PHYS_BASE        0x07800000
#define KERNEL_VIRT_BASE        0xff800000

#define MAX_BSS_SECTIONS        1024


static inline uint32_t rdmsr() {
    uint32_t msr; asm volatile("mfmsr %0":"=r"(msr)); return msr;
}

static inline void wrmsr(uint32_t msr) {
    asm volatile("mtmsr %0"::"r"(msr));
}


#define IBAT0U  528
#define IBAT0L  529
#define IBAT1U  530
#define IBAT1L  531
#define IBAT2U  532
#define IBAT2L  533
#define IBAT3U  534
#define IBAT3L  535

#define IBAT4U  560
#define IBAT4L  561
#define IBAT5U  562
#define IBAT5L  563
#define IBAT6U  564
#define IBAT6L  565
#define IBAT7U  566
#define IBAT7L  567

#define DBAT0U  536
#define DBAT0L  537
#define DBAT1U  538
#define DBAT1L  539
#define DBAT2U  540
#define DBAT2L  541
#define DBAT3U  542
#define DBAT3L  543

#define DBAT4U  568
#define DBAT4L  569
#define DBAT5U  570
#define DBAT5L  571
#define DBAT6U  572
#define DBAT6L  573
#define DBAT7U  574
#define DBAT7L  575


#define rdspr(reg) \
    ({ unsigned long val; asm volatile("mfspr %0,%1":"=r"(val):"i"(reg)); val; })

#define wrspr(reg, val) \
    do { asm volatile("mtspr %0,%1"::"i"(reg),"r"(val)); } while(0)


#define rddcr(reg) \
    ({ unsigned long val; asm volatile("mfdcr %0,%1":"=r"(val):"i"(reg)); val; })

#define wrdcr(reg, val) \
    do { asm volatile("mtdcr %0,%1"::"i"(reg),"r"(val)); } while(0)

#endif /* SUPPORT_H_ */
