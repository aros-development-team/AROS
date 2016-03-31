/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
 */

#define __AROS_ATOMIC_INC_B(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxrb %0, [%3]; add %0, %0, #1; stxrb %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)
#define __AROS_ATOMIC_INC_W(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxrh %0, [%3]; add %0, %0, #1; stxrh %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)
#define __AROS_ATOMIC_INC_L(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxr %0, [%3]; add %0, %0, #1; stxr %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)

#define __AROS_ATOMIC_DEC_B(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxrb %0, [%3]; sub %0, %0, #1; stxrb %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)
#define __AROS_ATOMIC_DEC_W(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxrh %0, [%2]; sub %0, %0, #1; stxrh %1, %0, [%2]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)
#define __AROS_ATOMIC_DEC_L(var) \
do { \
   unsigned long temp; \
   int result; \
   __asm__ __volatile__("\n1: ldxr %0, [%3]; sub %0, %0, #1; stxr %1, %0, [%3]; teq %1, #0; bne 1b" \
		   	   	   	   	   :"=&r"(result), "=&r"(temp), "+Qo"(var) \
		   	   	   	   	   :"r"(&var) \
		   	   	   	   	   :"cc"); \
} while (0)

#define __AROS_ATOMIC_AND_B(var, mask) \
do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxrb %0, [%3]; and %0, %0, %4; stxrb %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "I"(mask) \
                        :"cc"); \
} while(0)
#define __AROS_ATOMIC_AND_W(var, mask) \
do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxrh %0, [%3]; and %0, %0, %4; stxrh %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "Ir"(mask) \
                        :"cc"); \
} while(0)
#define __AROS_ATOMIC_AND_L(var, mask) \
do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxr %0, [%3]; and %0, %0, %4; stxr %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "Ir"(mask) \
                        :"cc"); \
} while(0)

#define __AROS_ATOMIC_OR_B(var, mask) \
do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxrb %0, [%3]; orr %0, %0, %4; stxrb %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "I"(mask) \
                        :"cc"); \
} while(0)
#define __AROS_ATOMIC_OR_W(var, mask) \
    do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxrh %0, [%3]; orr %0, %0, %4; stxrh %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "Ir"(mask) \
                        :"cc"); \
} while(0)
#define __AROS_ATOMIC_OR_L(var, mask) \
do { \
    unsigned long temp; int result; \
    __asm__ __volatile__("\n1: ldxr %0, [%3]; orr %0, %0, %4; stxr %1, %0, [%3]; teq %1, #0; bne 1b" \
                        :"=&r"(result), "=&r"(temp), "+Qo"(var) \
                        :"r"(&var), "Ir"(mask) \
                        :"cc"); \
} while(0)
