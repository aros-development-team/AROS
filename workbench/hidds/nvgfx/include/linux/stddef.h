/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_STDDEF_H_
#define _LINUX_STDDEF_H_

#include <stddef.h>
#include <linux/types.h>

#undef offsetof
#define offsetof(TYPE, MEMBER)  __builtin_offsetof(TYPE, MEMBER)
#define offsetofend(TYPE, MEMBER) (offsetof(TYPE, MEMBER) + sizeof(((TYPE *)0)->MEMBER))
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#define __struct_group(TAG, NAME, ATTRS, MEMBERS...) union { struct { MEMBERS } ATTRS; struct TAG { MEMBERS } ATTRS NAME; } ATTRS
#define struct_group(NAME, MEMBERS...) __struct_group(/* no tag */, NAME, /* no attrs */, MEMBERS)
#define struct_group_attr(NAME, ATTRS, MEMBERS...) __struct_group(/* no tag */, NAME, ATTRS, MEMBERS)
#define struct_group_tagged(TAG, NAME, MEMBERS...) __struct_group(TAG, NAME, /* no attrs */, MEMBERS)
#define __DECLARE_FLEX_ARRAY(TYPE, NAME) struct { struct { } __empty_ ## NAME; TYPE NAME[]; }
#define DECLARE_FLEX_ARRAY(TYPE, NAME) __DECLARE_FLEX_ARRAY(TYPE, NAME)

#endif /* _LINUX_STDDEF_H_ */
