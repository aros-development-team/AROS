#ifndef _STAB_H
#define _STAB_H

/* Global variable.  Only the name is significant.
   To find the address, look in the corresponding external symbol.  */
#define N_GSYM	0x20

/* Function name or text-segment variable for C.  Value is its address.
   Desc is supposedly starting line number, but GCC doesn't set it
   and DBX seems not to miss it.  */
#define N_FUN 0x24

/* Data-segment variable with internal linkage.  Value is its address.
   "Static Sym".  */
#define N_STSYM 0x26

/* BSS-segment variable with internal linkage.	Value is its address.  */
#define N_LCSYM 0x28

/* Name of main routine.  Only the name is significant.
   This is not used in C.  */
#define N_MAIN 0x2a

/* Register variable.  Value is number of register.  */
#define N_RSYM 0x40

/* Line number in text segment.  Desc is the line number;
   value is corresponding address.  */
#define N_SLINE 0x44

/* Similar, for data segment.  */
#define N_DSLINE 0x46

/* Similar, for bss segment.  */
#define N_BSLINE 0x48

/* Structure or union element.	Value is offset in the structure.  */
#define N_SSYM 0x60

/* Name of main source file.
   Value is starting text address of the compilation.  */
#define N_SO 0x64

/* Automatic variable in the stack.  Value is offset from frame pointer.
   Also used for type descriptions.  */
#define N_LSYM 0x80

/* Name of sub-source file (#include file).
   Value is starting text address of the compilation.  */
#define N_SOL 0x84

/* Parameter variable.	Value is offset from argument pointer.
   (On most machines the argument pointer is the same as the frame pointer.  */
#define N_PSYM 0xa0

/* End of an include file.  No name.
   This and N_BINCL act as brackets around the file's output.
   In an object file, there is no significant data in this entry.
   The Sun linker puts data into some of the fields.  */
#define N_EINCL 0xa2

/* Alternate entry point.  Value is its address.  */
#define N_ENTRY 0xa4

/* Beginning of lexical block.
   The desc is the nesting level in lexical blocks.
   The value is the address of the start of the text for the block.
   The variables declared inside the block *precede* the N_LBRAC symbol.  */
#define N_LBRAC 0xc0

/* End of a lexical block.  Desc matches the N_LBRAC's desc.
   The value is the address of the end of the text for the block.  */
#define N_RBRAC 0xe0

#endif /* _STAB_H */

