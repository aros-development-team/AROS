/*
    Copyright 2009-2026, The AROS Development Team. All rights reserved.
*/

#ifndef _LINUX_CLEANUP_H_
#define _LINUX_CLEANUP_H_

#include <linux/compiler.h>
#include <linux/args.h>

/*
 * Scope-based resource management on top of the cleanup attribute: a
 * variable declared with __free(name) or through a CLASS/guard has its
 * destructor run when it goes out of scope.
 */
#define DEFINE_FREE(_name, _type, _free)                                \
    static inline void __free_##_name(void *p) { _type _T = *(_type *)p; _free; }
#define __free(_name)           __attribute__((__cleanup__(__free_##_name)))
#define __get_and_null(p, nullvalue) ({ typeof(p) *__p = &(p); typeof(p) __val = *__p; *__p = nullvalue; __val; })
#define no_free_ptr(p)          ((typeof(p))__get_and_null(p, NULL))
#define return_ptr(p)           return no_free_ptr(p)
#define __must_check_fn(x)      (x)

#define DEFINE_CLASS(_name, _type, _exit, _init, _init_args...)         \
    typedef _type class_##_name##_t;                                    \
    static inline void class_##_name##_destructor(_type *p) { _type _T = *p; _exit; } \
    static inline _type class_##_name##_constructor(_init_args) { _type t = _init; return t; }
#define EXTEND_CLASS(_name, ext, _init, _init_args...)                  \
    typedef class_##_name##_t class_##_name##ext##_t;                   \
    static inline void class_##_name##ext##_destructor(class_##_name##_t *p) { class_##_name##_destructor(p); } \
    static inline class_##_name##_t class_##_name##ext##_constructor(_init_args) { class_##_name##_t t = _init; return t; }
#define CLASS(_name, var)                                               \
    class_##_name##_t var __attribute__((__cleanup__(class_##_name##_destructor))) = class_##_name##_constructor

#define DEFINE_GUARD(_name, _type, _lock, _unlock)                      \
    DEFINE_CLASS(_name, _type, if (_T) { _unlock; }, ({ _lock; _T; }), _type _T) \
    static inline void *class_##_name##_lock_ptr(class_##_name##_t *_T) { return (void *)(IPTR)*_T; }
#define DEFINE_GUARD_COND(_name, _ext, _condlock)                       \
    EXTEND_CLASS(_name, _ext, ({ void *_t = _T; if (_T && !(_condlock)) _t = NULL; _t; }), class_##_name##_t _T) \
    static inline void *class_##_name##_ext##_lock_ptr(class_##_name##_t *_T) { return class_##_name##_lock_ptr(_T); }
#define guard(_name)            CLASS(_name, __UNIQUE_ID(guard))
#define __guard_ptr(_name)      class_##_name##_lock_ptr
#define __is_cond_ptr(_name)    0
#define __scoped_guard(_name, _label, args...)                          \
    for (CLASS(_name, scope)(args); __guard_ptr(_name)(&scope) || !__is_cond_ptr(_name); ({ goto _label; })) \
        if (0) { _label: break; } else
#define scoped_guard(_name, args...) __scoped_guard(_name, __UNIQUE_ID(label), args)
#define scoped_cond_guard(_name, _fail, args...) scoped_guard(_name, args)

#define __DEFINE_UNLOCK_GUARD(_name, _type, _unlock, ...)               \
    typedef struct { _type *lock; __VA_ARGS__; } class_##_name##_t;     \
    static inline void class_##_name##_destructor(class_##_name##_t *_T) { if (_T->lock) { _unlock; } } \
    static inline void *class_##_name##_lock_ptr(class_##_name##_t *_T) { return (void *)(IPTR)_T->lock; }
#define __DEFINE_LOCK_GUARD_1(_name, _type, _lock)                      \
    static inline class_##_name##_t class_##_name##_constructor(_type *l) { class_##_name##_t _t = { .lock = l }, *_T = &_t; _lock; return _t; }
#define __DEFINE_LOCK_GUARD_0(_name, _lock)                             \
    static inline class_##_name##_t class_##_name##_constructor(void) { class_##_name##_t _t = { .lock = (void *)1 }, *_T __maybe_unused = &_t; _lock; return _t; }
#define DEFINE_LOCK_GUARD_1(_name, _type, _lock, _unlock, ...)          \
    __DEFINE_UNLOCK_GUARD(_name, _type, _unlock, __VA_ARGS__)           \
    __DEFINE_LOCK_GUARD_1(_name, _type, _lock)
#define DEFINE_LOCK_GUARD_0(_name, _lock, _unlock, ...)                 \
    __DEFINE_UNLOCK_GUARD(_name, void, _unlock, __VA_ARGS__)            \
    __DEFINE_LOCK_GUARD_0(_name, _lock)
#define DEFINE_LOCK_GUARD_1_COND(_name, _ext, _condlock)                \
    static inline class_##_name##_t class_##_name##_ext##_constructor(void *l) { class_##_name##_t _t = { .lock = l }, *_T = &_t; if (!(_condlock)) _t.lock = NULL; return _t; } \
    typedef class_##_name##_t class_##_name##_ext##_t;                  \
    static inline void class_##_name##_ext##_destructor(class_##_name##_t *_T) { class_##_name##_destructor(_T); } \
    static inline void *class_##_name##_ext##_lock_ptr(class_##_name##_t *_T) { return class_##_name##_lock_ptr(_T); }

#endif /* _LINUX_CLEANUP_H_ */
