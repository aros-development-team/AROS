#ifndef KERN_RANDOM_PROTOS_H
#define KERN_RANDOM_PROTOS_H

/* kern/kern_random.c: random bytes, from the C runtime's arc4random_buf() */
void kern_random(void *buf, size_t len);
u_int32_t kern_random32(void);

#endif
