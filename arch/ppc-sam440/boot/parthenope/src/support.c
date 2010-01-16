#include "context.h"
#include "support.h"

int strlen(const char *str)
{
	int len = 0;
	while (*str++)
		len++;
	return len;
}

int isblank(char c)
{
	return (c == ' ' || c == '\t');
}

int isspace(char c)
{
	return (c == ' ' || c == '\t' || c == '\f' || c == '\n' || c == '\r'
		|| c == '\v');
}

int isdigit(char c)
{
	return (c >= '0' && c <= '9');
}

int tolower(char c)
{
	if (c >= 'A' && c <= 'Z')
		c -= ('A' - 'a');

	return c;
}

int strncasecmp(const char *s1, const char *s2, int max)
{
	int diff = 0;
	for (; max && !(diff = tolower(*s2) - tolower(*s1)) && *s1;
	     max--, s1++, s2++) ;
	return diff;
}

int strcasecmp(const char *s1, const char *s2)
{
	int diff = 0;
	for (; !(diff = tolower(*s2) - tolower(*s1)) && *s1; s1++, s2++) ;
	return diff;
}

int strncmp(const char *s1, const char *s2, int max)
{
	int diff = 0;
	for (; max && !(diff = *s2 - *s1) && *s1; max--, s1++, s2++) ;
	return diff;
}

int strcmp(const char *s1, const char *s2)
{
	int diff = 0;
	for (; !(diff = *s2 - *s1) && *s1; s1++, s2++) ;
	return diff;
}

void bzero(void *dest, int length)
{
	char *d = dest;

	while (length--)
		*d++ = 0;
}

char *strcpy(char *dest, const char *src)
{
	return (char *)memmove(dest, src, strlen(src) + 1);

}

char *strncpy(char *dest, const char *src, int n)
{
	return (char *)memmove(dest, src, n);
}

int StackSwap(struct StackSwapStruct *sss)
{
	register uint32_t real_sp asm("r1");
	uint32_t *sp;
	uint32_t *src;
	uint32_t *dst;

	asm volatile ("wrteei 0");

	/* Get the real stack pointer */
	asm volatile ("mr %0,%1":"=r" (sp):"r"(real_sp));

	/* Go one stack frame upper - now src points to the stackframe of caller */
	src = (uint32_t *) * sp;

	/* Go one more stack frame up. Now you may copy from src to dst (src - sp) IPTR's */
	src = (uint32_t *) * src;

	/* If no StackSwapStruct is given, return the "reserved" area behind the stk_Pointer */
	if (!sss)
		return (src - sp) * sizeof(uint32_t);

	dst = sss->stk_Pointer;

	/* Rewind the dst pointer too. */
	dst += (src - sp);

	/* Copy the two stack frames */
	while (src != sp) {
		*--dst = *--src;
	}

	sss->stk_Pointer = sp;

	asm volatile ("mr %0,%1"::"r" (real_sp), "r"(dst));

	asm volatile ("wrteei 1");

	return 0;
}

void *calloc(int size, int n)
{
	return malloc(size * n);
}

void *memcpy(void *dest, const void *src, int n)
{
	return memmove(dest, src, n);
}

void *memset(void *dest, int c, int n)
{
	char *ptr = (char *)dest;

	while (n-- > 0)
		*ptr++ = c;
	return dest;
}

char *strchr(char *s, int c)
{
	char *sptr;
	for (sptr = s; *sptr != 0 && *sptr != c; sptr++) ;
	if (*sptr == 0)
		return NULL;
	return sptr;
}

char *strdup(const char *s)
{
	char *dup;
	dup = malloc(strlen(s) + 1);
	strcpy(dup, s);
	return dup;
}

int strtol(const char *s)
{
	int l, i, m;
	l = 0;
	for (i = strlen(s) - 1, m = 1; i >= 0; i--, m *= 10) {
		l += (s[i] - 48) * m;
	}
	return l;
}

list_t *list_new(void)
{
	list_t *self;

	self = malloc(sizeof(list_t));

	self->l_head = (node_t *) & self->l_tail;
	self->l_tail = NULL;
	self->l_tailpred = (node_t *) & self->l_head;

	return self;
}

void list_append(list_t * self, node_t * node)
{
	node->n_succ = (node_t *) & self->l_tail;
	node->n_pred = self->l_tailpred;
	node->n_pred->n_succ = node;
	self->l_tailpred = node;
}
