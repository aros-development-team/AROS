#define STUBCODE                     \
                ".text\n\t"          \
                ".globl %s\n\t"      \
		"%s:\n\t"            \
		"movl %s,%%eax\n\t"  \
		"jmp *%d(%%eax)\n\t"
