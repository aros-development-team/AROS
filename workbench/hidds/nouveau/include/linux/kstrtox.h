/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_KSTRTOX_H_
#define _LINUX_KSTRTOX_H_

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/compiler.h>

int kstrtoull(const char *s, unsigned int base, unsigned long long *res);
int kstrtoll(const char *s, unsigned int base, long long *res);
int kstrtoul(const char *s, unsigned int base, unsigned long *res);
int kstrtol(const char *s, unsigned int base, long *res);
int kstrtouint(const char *s, unsigned int base, unsigned int *res);
int kstrtoint(const char *s, unsigned int base, int *res);
int kstrtou64(const char *s, unsigned int base, u64 *res);
int kstrtos64(const char *s, unsigned int base, s64 *res);
int kstrtou32(const char *s, unsigned int base, u32 *res);
int kstrtos32(const char *s, unsigned int base, s32 *res);
int kstrtou16(const char *s, unsigned int base, u16 *res);
int kstrtos16(const char *s, unsigned int base, s16 *res);
int kstrtou8(const char *s, unsigned int base, u8 *res);
int kstrtos8(const char *s, unsigned int base, s8 *res);
int kstrtobool(const char *s, bool *res);
#define kstrtoul_from_user(a, b, c, d)  (-EINVAL)
#define strtobool(s, r)                 kstrtobool(s, r)

#endif /* _LINUX_KSTRTOX_H_ */
