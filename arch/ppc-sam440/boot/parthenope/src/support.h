#ifndef SUPPORT_H_
#define SUPPORT_H_

typedef struct node {
	struct node *n_succ, *n_pred;
} node_t;

typedef struct list {
	struct node *l_head, *l_tail, *l_tailpred;
} list_t;

typedef struct {
	node_t			m_node;
	char *			m_name;
	char *			m_str;
	unsigned long	m_lowest;
	unsigned long	m_highest;
	list_t			m_symbols;
} module_t;

typedef struct {
	node_t          s_node;
	char *			s_name;
	unsigned long   s_lowest;
	unsigned long 	s_highest;
} symbol_t;

static inline void new_list(list_t *l)
{
        l->l_tailpred = (node_t *)l;
        l->l_tail     = (node_t *)0;
        l->l_head     = (node_t *)&l->l_tail;
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

list_t *list_new(void);
void list_append(list_t * self, node_t * node);

typedef struct tagitem {
	unsigned long ti_tag, ti_data;
} tagitem_t;

struct StackSwapStruct {
	void *stk_Pointer;	/* Stack pointer at switch point */
};

/* Markers for .bss section in the file. They will be used to clear BSS out */
extern unsigned long __bss_start;
extern unsigned long _end;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

#ifdef IN_PARTHENOPE
int strlen(const char *str);
int isblank(char c);
int isspace(char c);
int isdigit(char c);
int tolower(char c);
int strncasecmp(const char *s1, const char *s2, int max);
int strcasecmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, int max);
int strcmp(const char *s1, const char *s2);
void bzero(void *dest, int length);
#endif

int StackSwap(struct StackSwapStruct *sss);

#define __used __attribute__((used))
#define __startup __attribute__((section(".aros.startup")))

#define BOOT_DIR "boot/"
#define BOOT_FILE(x) BOOT_DIR # x

#define KERNEL_PHYS_BASE        0x00800000
#define KERNEL_VIRT_BASE        0xff800000

#define MAX_BSS_SECTIONS        1024

#ifdef IN_PARTHENOPE
#define NULL ((void*)0)
#endif

#define TAG_USER                0x80000000

#define KRN_Dummy               (TAG_USER + 0x03d00000)
#define KRN_KernelBase          (KRN_Dummy + 1)
#define KRN_KernelLowest        (KRN_Dummy + 2)
#define KRN_KernelHighest       (KRN_Dummy + 3)
#define KRN_KernelBss           (KRN_Dummy + 4)
#define KRN_GDT                 (KRN_Dummy + 5)
#define KRN_IDT                 (KRN_Dummy + 6)
#define KRN_PL4                 (KRN_Dummy + 7)
#define KRN_VBEModeInfo         (KRN_Dummy + 8)
#define KRN_VBEControllerInfo   (KRN_Dummy + 9)
#define KRN_MMAPAddress         (KRN_Dummy + 10)
#define KRN_MMAPLength          (KRN_Dummy + 11)
#define KRN_CmdLine             (KRN_Dummy + 12)
#define KRN_ProtAreaStart       (KRN_Dummy + 13)
#define KRN_ProtAreaEnd         (KRN_Dummy + 14)
#define KRN_VBEMode             (KRN_Dummy + 15)
#define KRN_VBEPaletteWidth     (KRN_Dummy + 16)
#define KRN_MEMLower         	(KRN_Dummy + 17)
#define KRN_MEMUpper          	(KRN_Dummy + 18)
#define KRN_OpenFirmwareTree	(KRN_Dummy + 19)
#define KRN_HostInterface		(KRN_Dummy + 20)
#define KRN_DebugInfo			(KRN_Dummy + 21)
#define KRN_BootLoader          (KRN_Dummy + 22)

#ifdef IN_PARTHENOPE
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, int n);
char *strchr(char *s, int c);
char *strdup(const char *s);
int strtol(const char *s);

void *calloc(int size, int n);
void *memcpy(void *dest, const void *src, int n);
void *memset(void *s, int c, int n);
#endif

#endif /*SUPPORT_H_ */
