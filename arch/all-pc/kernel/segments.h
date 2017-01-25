/*
 * Standard values for segment registers.
 * This file can be included from both C and assembler sources.
 */

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

#if (__WORDSIZE==64)
# define USER_CS32 0x1b
# define USER_CS   0x2b
#else
# define USER_CS   0x1b
#endif
#define USER_DS   0x23
#define USER_GS   0x30
#define SEG_LDT   0x38
#define TASK_SEG  0x40
