#ifndef _VBCCINLINE_AMISSL_H
#define _VBCCINLINE_AMISSL_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

long __InitAmiSSLA(__reg("a6") struct Library *, __reg("a0") struct TagItem * tagList)="\tjsr\t-36(a6)";
#define InitAmiSSLA(tagList) __InitAmiSSLA(AmiSSLBase, (tagList))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
long __InitAmiSSL(__reg("a6") struct Library *, Tag tagList, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-36(a6)\n\tmovea.l\t(a7)+,a0";
#define InitAmiSSL(...) __InitAmiSSL(AmiSSLBase, __VA_ARGS__)
#endif

long __CleanupAmiSSLA(__reg("a6") struct Library *, __reg("a0") struct TagItem * tagList)="\tjsr\t-42(a6)";
#define CleanupAmiSSLA(tagList) __CleanupAmiSSLA(AmiSSLBase, (tagList))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
long __CleanupAmiSSL(__reg("a6") struct Library *, Tag tagList, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-42(a6)\n\tmovea.l\t(a7)+,a0";
#define CleanupAmiSSL(...) __CleanupAmiSSL(AmiSSLBase, __VA_ARGS__)
#endif

long __IsCipherAvailable(__reg("a6") struct Library *, __reg("d0") long cipher)="\tjsr\t-48(a6)";
#define IsCipherAvailable(cipher) __IsCipherAvailable(AmiSSLBase, (cipher))

ASN1_TYPE * __ASN1_TYPE_new(__reg("a6") struct Library *)="\tjsr\t-102(a6)";
#define ASN1_TYPE_new() __ASN1_TYPE_new(AmiSSLBase)

void __ASN1_TYPE_free(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a)="\tjsr\t-108(a6)";
#define ASN1_TYPE_free(a) __ASN1_TYPE_free(AmiSSLBase, (a))

ASN1_TYPE * __d2i_ASN1_TYPE(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-114(a6)";
#define d2i_ASN1_TYPE(a, in, len) __d2i_ASN1_TYPE(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_TYPE(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("a1") unsigned char ** out)="\tjsr\t-120(a6)";
#define i2d_ASN1_TYPE(a, out) __i2d_ASN1_TYPE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_ANY_it(__reg("a6") struct Library *)="\tjsr\t-126(a6)";
#define ASN1_ANY_it() __ASN1_ANY_it(AmiSSLBase)

int __ASN1_TYPE_get(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a)="\tjsr\t-132(a6)";
#define ASN1_TYPE_get(a) __ASN1_TYPE_get(AmiSSLBase, (a))

void __ASN1_TYPE_set(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("d0") LONG type, __reg("a1") void * value)="\tjsr\t-138(a6)";
#define ASN1_TYPE_set(a, type, value) __ASN1_TYPE_set(AmiSSLBase, (a), (type), (value))

ASN1_OBJECT * __ASN1_OBJECT_new(__reg("a6") struct Library *)="\tjsr\t-144(a6)";
#define ASN1_OBJECT_new() __ASN1_OBJECT_new(AmiSSLBase)

void __ASN1_OBJECT_free(__reg("a6") struct Library *, __reg("a0") ASN1_OBJECT * a)="\tjsr\t-150(a6)";
#define ASN1_OBJECT_free(a) __ASN1_OBJECT_free(AmiSSLBase, (a))

int __i2d_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") ASN1_OBJECT * a, __reg("a1") unsigned char ** pp)="\tjsr\t-156(a6)";
#define i2d_ASN1_OBJECT(a, pp) __i2d_ASN1_OBJECT(AmiSSLBase, (a), (pp))

ASN1_OBJECT * __c2i_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") ASN1_OBJECT ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-162(a6)";
#define c2i_ASN1_OBJECT(a, pp, length) __c2i_ASN1_OBJECT(AmiSSLBase, (a), (pp), (length))

ASN1_OBJECT * __d2i_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") ASN1_OBJECT ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-168(a6)";
#define d2i_ASN1_OBJECT(a, pp, length) __d2i_ASN1_OBJECT(AmiSSLBase, (a), (pp), (length))

const ASN1_ITEM * __ASN1_OBJECT_it(__reg("a6") struct Library *)="\tjsr\t-174(a6)";
#define ASN1_OBJECT_it() __ASN1_OBJECT_it(AmiSSLBase)

ASN1_STRING * __ASN1_STRING_new(__reg("a6") struct Library *)="\tjsr\t-180(a6)";
#define ASN1_STRING_new() __ASN1_STRING_new(AmiSSLBase)

void __ASN1_STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a)="\tjsr\t-186(a6)";
#define ASN1_STRING_free(a) __ASN1_STRING_free(AmiSSLBase, (a))

ASN1_STRING * __ASN1_STRING_dup(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a)="\tjsr\t-192(a6)";
#define ASN1_STRING_dup(a) __ASN1_STRING_dup(AmiSSLBase, (a))

ASN1_STRING * __ASN1_STRING_type_new(__reg("a6") struct Library *, __reg("d0") LONG type)="\tjsr\t-198(a6)";
#define ASN1_STRING_type_new(type) __ASN1_STRING_type_new(AmiSSLBase, (type))

int __ASN1_STRING_cmp(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a, __reg("a1") ASN1_STRING * b)="\tjsr\t-204(a6)";
#define ASN1_STRING_cmp(a, b) __ASN1_STRING_cmp(AmiSSLBase, (a), (b))

int __ASN1_STRING_set(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * str, __reg("a1") const void * data, __reg("d0") LONG len)="\tjsr\t-210(a6)";
#define ASN1_STRING_set(str, data, len) __ASN1_STRING_set(AmiSSLBase, (str), (data), (len))

int __ASN1_STRING_length(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * x)="\tjsr\t-216(a6)";
#define ASN1_STRING_length(x) __ASN1_STRING_length(AmiSSLBase, (x))

void __ASN1_STRING_length_set(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * x, __reg("d0") LONG n)="\tjsr\t-222(a6)";
#define ASN1_STRING_length_set(x, n) __ASN1_STRING_length_set(AmiSSLBase, (x), (n))

int __ASN1_STRING_type(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * x)="\tjsr\t-228(a6)";
#define ASN1_STRING_type(x) __ASN1_STRING_type(AmiSSLBase, (x))

unsigned char * __ASN1_STRING_data(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * x)="\tjsr\t-234(a6)";
#define ASN1_STRING_data(x) __ASN1_STRING_data(AmiSSLBase, (x))

ASN1_BIT_STRING * __ASN1_BIT_STRING_new(__reg("a6") struct Library *)="\tjsr\t-240(a6)";
#define ASN1_BIT_STRING_new() __ASN1_BIT_STRING_new(AmiSSLBase)

void __ASN1_BIT_STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a)="\tjsr\t-246(a6)";
#define ASN1_BIT_STRING_free(a) __ASN1_BIT_STRING_free(AmiSSLBase, (a))

ASN1_BIT_STRING * __d2i_ASN1_BIT_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-252(a6)";
#define d2i_ASN1_BIT_STRING(a, in, len) __d2i_ASN1_BIT_STRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_BIT_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-258(a6)";
#define i2d_ASN1_BIT_STRING(a, out) __i2d_ASN1_BIT_STRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_BIT_STRING_it(__reg("a6") struct Library *)="\tjsr\t-264(a6)";
#define ASN1_BIT_STRING_it() __ASN1_BIT_STRING_it(AmiSSLBase)

int __i2c_ASN1_BIT_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a, __reg("a1") unsigned char ** pp)="\tjsr\t-270(a6)";
#define i2c_ASN1_BIT_STRING(a, pp) __i2c_ASN1_BIT_STRING(AmiSSLBase, (a), (pp))

ASN1_BIT_STRING * __c2i_ASN1_BIT_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-276(a6)";
#define c2i_ASN1_BIT_STRING(a, pp, length) __c2i_ASN1_BIT_STRING(AmiSSLBase, (a), (pp), (length))

int __ASN1_BIT_STRING_set(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a, __reg("a1") unsigned char * d, __reg("d0") LONG length)="\tjsr\t-282(a6)";
#define ASN1_BIT_STRING_set(a, d, length) __ASN1_BIT_STRING_set(AmiSSLBase, (a), (d), (length))

int __ASN1_BIT_STRING_set_bit(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a, __reg("d0") LONG n, __reg("d1") LONG value)="\tjsr\t-288(a6)";
#define ASN1_BIT_STRING_set_bit(a, n, value) __ASN1_BIT_STRING_set_bit(AmiSSLBase, (a), (n), (value))

int __ASN1_BIT_STRING_get_bit(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * a, __reg("d0") LONG n)="\tjsr\t-294(a6)";
#define ASN1_BIT_STRING_get_bit(a, n) __ASN1_BIT_STRING_get_bit(AmiSSLBase, (a), (n))

int __ASN1_BIT_STRING_name_print(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") ASN1_BIT_STRING * bs, __reg("a2") BIT_STRING_BITNAME * tbl, __reg("d0") LONG indent)="\tjsr\t-300(a6)";
#define ASN1_BIT_STRING_name_print(out, bs, tbl, indent) __ASN1_BIT_STRING_name_print(AmiSSLBase, (out), (bs), (tbl), (indent))

int __ASN1_BIT_STRING_num_asc(__reg("a6") struct Library *, __reg("a0") char * name, __reg("a1") BIT_STRING_BITNAME * tbl)="\tjsr\t-306(a6)";
#define ASN1_BIT_STRING_num_asc(name, tbl) __ASN1_BIT_STRING_num_asc(AmiSSLBase, (name), (tbl))

int __ASN1_BIT_STRING_set_asc(__reg("a6") struct Library *, __reg("a0") ASN1_BIT_STRING * bs, __reg("a1") char * name, __reg("d0") LONG value, __reg("a2") BIT_STRING_BITNAME * tbl)="\tjsr\t-312(a6)";
#define ASN1_BIT_STRING_set_asc(bs, name, value, tbl) __ASN1_BIT_STRING_set_asc(AmiSSLBase, (bs), (name), (value), (tbl))

int __i2d_ASN1_BOOLEAN(__reg("a6") struct Library *, __reg("d0") LONG a, __reg("a0") unsigned char ** pp)="\tjsr\t-318(a6)";
#define i2d_ASN1_BOOLEAN(a, pp) __i2d_ASN1_BOOLEAN(AmiSSLBase, (a), (pp))

int __d2i_ASN1_BOOLEAN(__reg("a6") struct Library *, __reg("a0") int * a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-324(a6)";
#define d2i_ASN1_BOOLEAN(a, pp, length) __d2i_ASN1_BOOLEAN(AmiSSLBase, (a), (pp), (length))

ASN1_INTEGER * __ASN1_INTEGER_new(__reg("a6") struct Library *)="\tjsr\t-330(a6)";
#define ASN1_INTEGER_new() __ASN1_INTEGER_new(AmiSSLBase)

void __ASN1_INTEGER_free(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * a)="\tjsr\t-336(a6)";
#define ASN1_INTEGER_free(a) __ASN1_INTEGER_free(AmiSSLBase, (a))

ASN1_INTEGER * __d2i_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-342(a6)";
#define d2i_ASN1_INTEGER(a, in, len) __d2i_ASN1_INTEGER(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * a, __reg("a1") unsigned char ** out)="\tjsr\t-348(a6)";
#define i2d_ASN1_INTEGER(a, out) __i2d_ASN1_INTEGER(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_INTEGER_it(__reg("a6") struct Library *)="\tjsr\t-354(a6)";
#define ASN1_INTEGER_it() __ASN1_INTEGER_it(AmiSSLBase)

int __i2c_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * a, __reg("a1") unsigned char ** pp)="\tjsr\t-360(a6)";
#define i2c_ASN1_INTEGER(a, pp) __i2c_ASN1_INTEGER(AmiSSLBase, (a), (pp))

ASN1_INTEGER * __c2i_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-366(a6)";
#define c2i_ASN1_INTEGER(a, pp, length) __c2i_ASN1_INTEGER(AmiSSLBase, (a), (pp), (length))

ASN1_INTEGER * __d2i_ASN1_UINTEGER(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-372(a6)";
#define d2i_ASN1_UINTEGER(a, pp, length) __d2i_ASN1_UINTEGER(AmiSSLBase, (a), (pp), (length))

ASN1_INTEGER * __ASN1_INTEGER_dup(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * x)="\tjsr\t-378(a6)";
#define ASN1_INTEGER_dup(x) __ASN1_INTEGER_dup(AmiSSLBase, (x))

int __ASN1_INTEGER_cmp(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * x, __reg("a1") ASN1_INTEGER * y)="\tjsr\t-384(a6)";
#define ASN1_INTEGER_cmp(x, y) __ASN1_INTEGER_cmp(AmiSSLBase, (x), (y))

ASN1_ENUMERATED * __ASN1_ENUMERATED_new(__reg("a6") struct Library *)="\tjsr\t-390(a6)";
#define ASN1_ENUMERATED_new() __ASN1_ENUMERATED_new(AmiSSLBase)

void __ASN1_ENUMERATED_free(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED * a)="\tjsr\t-396(a6)";
#define ASN1_ENUMERATED_free(a) __ASN1_ENUMERATED_free(AmiSSLBase, (a))

ASN1_ENUMERATED * __d2i_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-402(a6)";
#define d2i_ASN1_ENUMERATED(a, in, len) __d2i_ASN1_ENUMERATED(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED * a, __reg("a1") unsigned char ** out)="\tjsr\t-408(a6)";
#define i2d_ASN1_ENUMERATED(a, out) __i2d_ASN1_ENUMERATED(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_ENUMERATED_it(__reg("a6") struct Library *)="\tjsr\t-414(a6)";
#define ASN1_ENUMERATED_it() __ASN1_ENUMERATED_it(AmiSSLBase)

int __ASN1_UTCTIME_check(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME * a)="\tjsr\t-420(a6)";
#define ASN1_UTCTIME_check(a) __ASN1_UTCTIME_check(AmiSSLBase, (a))

ASN1_UTCTIME * __ASN1_UTCTIME_set(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME * s, __reg("d0") time_t t)="\tjsr\t-426(a6)";
#define ASN1_UTCTIME_set(s, t) __ASN1_UTCTIME_set(AmiSSLBase, (s), (t))

int __ASN1_UTCTIME_set_string(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME * s, __reg("a1") char * str)="\tjsr\t-432(a6)";
#define ASN1_UTCTIME_set_string(s, str) __ASN1_UTCTIME_set_string(AmiSSLBase, (s), (str))

int __ASN1_UTCTIME_cmp_time_t(__reg("a6") struct Library *, __reg("a0") const ASN1_UTCTIME * s, __reg("d0") time_t t)="\tjsr\t-438(a6)";
#define ASN1_UTCTIME_cmp_time_t(s, t) __ASN1_UTCTIME_cmp_time_t(AmiSSLBase, (s), (t))

int __ASN1_GENERALIZEDTIME_check(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * a)="\tjsr\t-444(a6)";
#define ASN1_GENERALIZEDTIME_check(a) __ASN1_GENERALIZEDTIME_check(AmiSSLBase, (a))

ASN1_GENERALIZEDTIME * __ASN1_GENERALIZEDTIME_set(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * s, __reg("d0") time_t t)="\tjsr\t-450(a6)";
#define ASN1_GENERALIZEDTIME_set(s, t) __ASN1_GENERALIZEDTIME_set(AmiSSLBase, (s), (t))

int __ASN1_GENERALIZEDTIME_set_string(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * s, __reg("a1") char * str)="\tjsr\t-456(a6)";
#define ASN1_GENERALIZEDTIME_set_string(s, str) __ASN1_GENERALIZEDTIME_set_string(AmiSSLBase, (s), (str))

ASN1_OCTET_STRING * __ASN1_OCTET_STRING_new(__reg("a6") struct Library *)="\tjsr\t-462(a6)";
#define ASN1_OCTET_STRING_new() __ASN1_OCTET_STRING_new(AmiSSLBase)

void __ASN1_OCTET_STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING * a)="\tjsr\t-468(a6)";
#define ASN1_OCTET_STRING_free(a) __ASN1_OCTET_STRING_free(AmiSSLBase, (a))

ASN1_OCTET_STRING * __d2i_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-474(a6)";
#define d2i_ASN1_OCTET_STRING(a, in, len) __d2i_ASN1_OCTET_STRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-480(a6)";
#define i2d_ASN1_OCTET_STRING(a, out) __i2d_ASN1_OCTET_STRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_OCTET_STRING_it(__reg("a6") struct Library *)="\tjsr\t-486(a6)";
#define ASN1_OCTET_STRING_it() __ASN1_OCTET_STRING_it(AmiSSLBase)

ASN1_OCTET_STRING * __ASN1_OCTET_STRING_dup(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING * a)="\tjsr\t-492(a6)";
#define ASN1_OCTET_STRING_dup(a) __ASN1_OCTET_STRING_dup(AmiSSLBase, (a))

int __ASN1_OCTET_STRING_cmp(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING * a, __reg("a1") ASN1_OCTET_STRING * b)="\tjsr\t-498(a6)";
#define ASN1_OCTET_STRING_cmp(a, b) __ASN1_OCTET_STRING_cmp(AmiSSLBase, (a), (b))

int __ASN1_OCTET_STRING_set(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING * str, __reg("a1") unsigned char * data, __reg("d0") LONG len)="\tjsr\t-504(a6)";
#define ASN1_OCTET_STRING_set(str, data, len) __ASN1_OCTET_STRING_set(AmiSSLBase, (str), (data), (len))

ASN1_VISIBLESTRING * __ASN1_VISIBLESTRING_new(__reg("a6") struct Library *)="\tjsr\t-510(a6)";
#define ASN1_VISIBLESTRING_new() __ASN1_VISIBLESTRING_new(AmiSSLBase)

void __ASN1_VISIBLESTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_VISIBLESTRING * a)="\tjsr\t-516(a6)";
#define ASN1_VISIBLESTRING_free(a) __ASN1_VISIBLESTRING_free(AmiSSLBase, (a))

ASN1_VISIBLESTRING * __d2i_ASN1_VISIBLESTRING(__reg("a6") struct Library *, __reg("a0") ASN1_VISIBLESTRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-522(a6)";
#define d2i_ASN1_VISIBLESTRING(a, in, len) __d2i_ASN1_VISIBLESTRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_VISIBLESTRING(__reg("a6") struct Library *, __reg("a0") ASN1_VISIBLESTRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-528(a6)";
#define i2d_ASN1_VISIBLESTRING(a, out) __i2d_ASN1_VISIBLESTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_VISIBLESTRING_it(__reg("a6") struct Library *)="\tjsr\t-534(a6)";
#define ASN1_VISIBLESTRING_it() __ASN1_VISIBLESTRING_it(AmiSSLBase)

ASN1_UNIVERSALSTRING * __ASN1_UNIVERSALSTRING_new(__reg("a6") struct Library *)="\tjsr\t-540(a6)";
#define ASN1_UNIVERSALSTRING_new() __ASN1_UNIVERSALSTRING_new(AmiSSLBase)

void __ASN1_UNIVERSALSTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_UNIVERSALSTRING * a)="\tjsr\t-546(a6)";
#define ASN1_UNIVERSALSTRING_free(a) __ASN1_UNIVERSALSTRING_free(AmiSSLBase, (a))

ASN1_UNIVERSALSTRING * __d2i_ASN1_UNIVERSALSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_UNIVERSALSTRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-552(a6)";
#define d2i_ASN1_UNIVERSALSTRING(a, in, len) __d2i_ASN1_UNIVERSALSTRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_UNIVERSALSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_UNIVERSALSTRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-558(a6)";
#define i2d_ASN1_UNIVERSALSTRING(a, out) __i2d_ASN1_UNIVERSALSTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_UNIVERSALSTRING_it(__reg("a6") struct Library *)="\tjsr\t-564(a6)";
#define ASN1_UNIVERSALSTRING_it() __ASN1_UNIVERSALSTRING_it(AmiSSLBase)

ASN1_UTF8STRING * __ASN1_UTF8STRING_new(__reg("a6") struct Library *)="\tjsr\t-570(a6)";
#define ASN1_UTF8STRING_new() __ASN1_UTF8STRING_new(AmiSSLBase)

void __ASN1_UTF8STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_UTF8STRING * a)="\tjsr\t-576(a6)";
#define ASN1_UTF8STRING_free(a) __ASN1_UTF8STRING_free(AmiSSLBase, (a))

ASN1_UTF8STRING * __d2i_ASN1_UTF8STRING(__reg("a6") struct Library *, __reg("a0") ASN1_UTF8STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-582(a6)";
#define d2i_ASN1_UTF8STRING(a, in, len) __d2i_ASN1_UTF8STRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_UTF8STRING(__reg("a6") struct Library *, __reg("a0") ASN1_UTF8STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-588(a6)";
#define i2d_ASN1_UTF8STRING(a, out) __i2d_ASN1_UTF8STRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_UTF8STRING_it(__reg("a6") struct Library *)="\tjsr\t-594(a6)";
#define ASN1_UTF8STRING_it() __ASN1_UTF8STRING_it(AmiSSLBase)

ASN1_NULL * __ASN1_NULL_new(__reg("a6") struct Library *)="\tjsr\t-600(a6)";
#define ASN1_NULL_new() __ASN1_NULL_new(AmiSSLBase)

void __ASN1_NULL_free(__reg("a6") struct Library *, __reg("a0") ASN1_NULL * a)="\tjsr\t-606(a6)";
#define ASN1_NULL_free(a) __ASN1_NULL_free(AmiSSLBase, (a))

ASN1_NULL * __d2i_ASN1_NULL(__reg("a6") struct Library *, __reg("a0") ASN1_NULL ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-612(a6)";
#define d2i_ASN1_NULL(a, in, len) __d2i_ASN1_NULL(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_NULL(__reg("a6") struct Library *, __reg("a0") ASN1_NULL * a, __reg("a1") unsigned char ** out)="\tjsr\t-618(a6)";
#define i2d_ASN1_NULL(a, out) __i2d_ASN1_NULL(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_NULL_it(__reg("a6") struct Library *)="\tjsr\t-624(a6)";
#define ASN1_NULL_it() __ASN1_NULL_it(AmiSSLBase)

ASN1_BMPSTRING * __ASN1_BMPSTRING_new(__reg("a6") struct Library *)="\tjsr\t-630(a6)";
#define ASN1_BMPSTRING_new() __ASN1_BMPSTRING_new(AmiSSLBase)

void __ASN1_BMPSTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_BMPSTRING * a)="\tjsr\t-636(a6)";
#define ASN1_BMPSTRING_free(a) __ASN1_BMPSTRING_free(AmiSSLBase, (a))

ASN1_BMPSTRING * __d2i_ASN1_BMPSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_BMPSTRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-642(a6)";
#define d2i_ASN1_BMPSTRING(a, in, len) __d2i_ASN1_BMPSTRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_BMPSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_BMPSTRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-648(a6)";
#define i2d_ASN1_BMPSTRING(a, out) __i2d_ASN1_BMPSTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_BMPSTRING_it(__reg("a6") struct Library *)="\tjsr\t-654(a6)";
#define ASN1_BMPSTRING_it() __ASN1_BMPSTRING_it(AmiSSLBase)

int __UTF8_getc(__reg("a6") struct Library *, __reg("a0") const unsigned char * str, __reg("d0") LONG len, __reg("a1") unsigned long * val)="\tjsr\t-660(a6)";
#define UTF8_getc(str, len, val) __UTF8_getc(AmiSSLBase, (str), (len), (val))

int __UTF8_putc(__reg("a6") struct Library *, __reg("a0") unsigned char * str, __reg("d0") LONG len, __reg("d1") unsigned long value)="\tjsr\t-666(a6)";
#define UTF8_putc(str, len, value) __UTF8_putc(AmiSSLBase, (str), (len), (value))

ASN1_STRING * __ASN1_PRINTABLE_new(__reg("a6") struct Library *)="\tjsr\t-672(a6)";
#define ASN1_PRINTABLE_new() __ASN1_PRINTABLE_new(AmiSSLBase)

void __ASN1_PRINTABLE_free(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a)="\tjsr\t-678(a6)";
#define ASN1_PRINTABLE_free(a) __ASN1_PRINTABLE_free(AmiSSLBase, (a))

ASN1_STRING * __d2i_ASN1_PRINTABLE(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-684(a6)";
#define d2i_ASN1_PRINTABLE(a, in, len) __d2i_ASN1_PRINTABLE(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_PRINTABLE(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-690(a6)";
#define i2d_ASN1_PRINTABLE(a, out) __i2d_ASN1_PRINTABLE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_PRINTABLE_it(__reg("a6") struct Library *)="\tjsr\t-696(a6)";
#define ASN1_PRINTABLE_it() __ASN1_PRINTABLE_it(AmiSSLBase)

ASN1_STRING * __DIRECTORYSTRING_new(__reg("a6") struct Library *)="\tjsr\t-702(a6)";
#define DIRECTORYSTRING_new() __DIRECTORYSTRING_new(AmiSSLBase)

void __DIRECTORYSTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a)="\tjsr\t-708(a6)";
#define DIRECTORYSTRING_free(a) __DIRECTORYSTRING_free(AmiSSLBase, (a))

ASN1_STRING * __d2i_DIRECTORYSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-714(a6)";
#define d2i_DIRECTORYSTRING(a, in, len) __d2i_DIRECTORYSTRING(AmiSSLBase, (a), (in), (len))

int __i2d_DIRECTORYSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-720(a6)";
#define i2d_DIRECTORYSTRING(a, out) __i2d_DIRECTORYSTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __DIRECTORYSTRING_it(__reg("a6") struct Library *)="\tjsr\t-726(a6)";
#define DIRECTORYSTRING_it() __DIRECTORYSTRING_it(AmiSSLBase)

ASN1_STRING * __DISPLAYTEXT_new(__reg("a6") struct Library *)="\tjsr\t-732(a6)";
#define DISPLAYTEXT_new() __DISPLAYTEXT_new(AmiSSLBase)

void __DISPLAYTEXT_free(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a)="\tjsr\t-738(a6)";
#define DISPLAYTEXT_free(a) __DISPLAYTEXT_free(AmiSSLBase, (a))

ASN1_STRING * __d2i_DISPLAYTEXT(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-744(a6)";
#define d2i_DISPLAYTEXT(a, in, len) __d2i_DISPLAYTEXT(AmiSSLBase, (a), (in), (len))

int __i2d_DISPLAYTEXT(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-750(a6)";
#define i2d_DISPLAYTEXT(a, out) __i2d_DISPLAYTEXT(AmiSSLBase, (a), (out))

const ASN1_ITEM * __DISPLAYTEXT_it(__reg("a6") struct Library *)="\tjsr\t-756(a6)";
#define DISPLAYTEXT_it() __DISPLAYTEXT_it(AmiSSLBase)

ASN1_PRINTABLESTRING * __ASN1_PRINTABLESTRING_new(__reg("a6") struct Library *)="\tjsr\t-762(a6)";
#define ASN1_PRINTABLESTRING_new() __ASN1_PRINTABLESTRING_new(AmiSSLBase)

void __ASN1_PRINTABLESTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_PRINTABLESTRING * a)="\tjsr\t-768(a6)";
#define ASN1_PRINTABLESTRING_free(a) __ASN1_PRINTABLESTRING_free(AmiSSLBase, (a))

ASN1_PRINTABLESTRING * __d2i_ASN1_PRINTABLESTRING(__reg("a6") struct Library *, __reg("a0") ASN1_PRINTABLESTRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-774(a6)";
#define d2i_ASN1_PRINTABLESTRING(a, in, len) __d2i_ASN1_PRINTABLESTRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_PRINTABLESTRING(__reg("a6") struct Library *, __reg("a0") ASN1_PRINTABLESTRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-780(a6)";
#define i2d_ASN1_PRINTABLESTRING(a, out) __i2d_ASN1_PRINTABLESTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_PRINTABLESTRING_it(__reg("a6") struct Library *)="\tjsr\t-786(a6)";
#define ASN1_PRINTABLESTRING_it() __ASN1_PRINTABLESTRING_it(AmiSSLBase)

ASN1_T61STRING * __ASN1_T61STRING_new(__reg("a6") struct Library *)="\tjsr\t-792(a6)";
#define ASN1_T61STRING_new() __ASN1_T61STRING_new(AmiSSLBase)

void __ASN1_T61STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_T61STRING * a)="\tjsr\t-798(a6)";
#define ASN1_T61STRING_free(a) __ASN1_T61STRING_free(AmiSSLBase, (a))

ASN1_T61STRING * __d2i_ASN1_T61STRING(__reg("a6") struct Library *, __reg("a0") ASN1_T61STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-804(a6)";
#define d2i_ASN1_T61STRING(a, in, len) __d2i_ASN1_T61STRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_T61STRING(__reg("a6") struct Library *, __reg("a0") ASN1_T61STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-810(a6)";
#define i2d_ASN1_T61STRING(a, out) __i2d_ASN1_T61STRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_T61STRING_it(__reg("a6") struct Library *)="\tjsr\t-816(a6)";
#define ASN1_T61STRING_it() __ASN1_T61STRING_it(AmiSSLBase)

ASN1_IA5STRING * __ASN1_IA5STRING_new(__reg("a6") struct Library *)="\tjsr\t-822(a6)";
#define ASN1_IA5STRING_new() __ASN1_IA5STRING_new(AmiSSLBase)

void __ASN1_IA5STRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_IA5STRING * a)="\tjsr\t-828(a6)";
#define ASN1_IA5STRING_free(a) __ASN1_IA5STRING_free(AmiSSLBase, (a))

ASN1_IA5STRING * __d2i_ASN1_IA5STRING(__reg("a6") struct Library *, __reg("a0") ASN1_IA5STRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-834(a6)";
#define d2i_ASN1_IA5STRING(a, in, len) __d2i_ASN1_IA5STRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_IA5STRING(__reg("a6") struct Library *, __reg("a0") ASN1_IA5STRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-840(a6)";
#define i2d_ASN1_IA5STRING(a, out) __i2d_ASN1_IA5STRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_IA5STRING_it(__reg("a6") struct Library *)="\tjsr\t-846(a6)";
#define ASN1_IA5STRING_it() __ASN1_IA5STRING_it(AmiSSLBase)

ASN1_GENERALSTRING * __ASN1_GENERALSTRING_new(__reg("a6") struct Library *)="\tjsr\t-852(a6)";
#define ASN1_GENERALSTRING_new() __ASN1_GENERALSTRING_new(AmiSSLBase)

void __ASN1_GENERALSTRING_free(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALSTRING * a)="\tjsr\t-858(a6)";
#define ASN1_GENERALSTRING_free(a) __ASN1_GENERALSTRING_free(AmiSSLBase, (a))

ASN1_GENERALSTRING * __d2i_ASN1_GENERALSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALSTRING ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-864(a6)";
#define d2i_ASN1_GENERALSTRING(a, in, len) __d2i_ASN1_GENERALSTRING(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_GENERALSTRING(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALSTRING * a, __reg("a1") unsigned char ** out)="\tjsr\t-870(a6)";
#define i2d_ASN1_GENERALSTRING(a, out) __i2d_ASN1_GENERALSTRING(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_GENERALSTRING_it(__reg("a6") struct Library *)="\tjsr\t-876(a6)";
#define ASN1_GENERALSTRING_it() __ASN1_GENERALSTRING_it(AmiSSLBase)

ASN1_UTCTIME * __ASN1_UTCTIME_new(__reg("a6") struct Library *)="\tjsr\t-882(a6)";
#define ASN1_UTCTIME_new() __ASN1_UTCTIME_new(AmiSSLBase)

void __ASN1_UTCTIME_free(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME * a)="\tjsr\t-888(a6)";
#define ASN1_UTCTIME_free(a) __ASN1_UTCTIME_free(AmiSSLBase, (a))

ASN1_UTCTIME * __d2i_ASN1_UTCTIME(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-894(a6)";
#define d2i_ASN1_UTCTIME(a, in, len) __d2i_ASN1_UTCTIME(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_UTCTIME(__reg("a6") struct Library *, __reg("a0") ASN1_UTCTIME * a, __reg("a1") unsigned char ** out)="\tjsr\t-900(a6)";
#define i2d_ASN1_UTCTIME(a, out) __i2d_ASN1_UTCTIME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_UTCTIME_it(__reg("a6") struct Library *)="\tjsr\t-906(a6)";
#define ASN1_UTCTIME_it() __ASN1_UTCTIME_it(AmiSSLBase)

ASN1_GENERALIZEDTIME * __ASN1_GENERALIZEDTIME_new(__reg("a6") struct Library *)="\tjsr\t-912(a6)";
#define ASN1_GENERALIZEDTIME_new() __ASN1_GENERALIZEDTIME_new(AmiSSLBase)

void __ASN1_GENERALIZEDTIME_free(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * a)="\tjsr\t-918(a6)";
#define ASN1_GENERALIZEDTIME_free(a) __ASN1_GENERALIZEDTIME_free(AmiSSLBase, (a))

ASN1_GENERALIZEDTIME * __d2i_ASN1_GENERALIZEDTIME(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-924(a6)";
#define d2i_ASN1_GENERALIZEDTIME(a, in, len) __d2i_ASN1_GENERALIZEDTIME(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_GENERALIZEDTIME(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * a, __reg("a1") unsigned char ** out)="\tjsr\t-930(a6)";
#define i2d_ASN1_GENERALIZEDTIME(a, out) __i2d_ASN1_GENERALIZEDTIME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_GENERALIZEDTIME_it(__reg("a6") struct Library *)="\tjsr\t-936(a6)";
#define ASN1_GENERALIZEDTIME_it() __ASN1_GENERALIZEDTIME_it(AmiSSLBase)

ASN1_TIME * __ASN1_TIME_new(__reg("a6") struct Library *)="\tjsr\t-942(a6)";
#define ASN1_TIME_new() __ASN1_TIME_new(AmiSSLBase)

void __ASN1_TIME_free(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * a)="\tjsr\t-948(a6)";
#define ASN1_TIME_free(a) __ASN1_TIME_free(AmiSSLBase, (a))

ASN1_TIME * __d2i_ASN1_TIME(__reg("a6") struct Library *, __reg("a0") ASN1_TIME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-954(a6)";
#define d2i_ASN1_TIME(a, in, len) __d2i_ASN1_TIME(AmiSSLBase, (a), (in), (len))

int __i2d_ASN1_TIME(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * a, __reg("a1") unsigned char ** out)="\tjsr\t-960(a6)";
#define i2d_ASN1_TIME(a, out) __i2d_ASN1_TIME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ASN1_TIME_it(__reg("a6") struct Library *)="\tjsr\t-966(a6)";
#define ASN1_TIME_it() __ASN1_TIME_it(AmiSSLBase)

ASN1_TIME * __ASN1_TIME_set(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * s, __reg("d0") time_t t)="\tjsr\t-972(a6)";
#define ASN1_TIME_set(s, t) __ASN1_TIME_set(AmiSSLBase, (s), (t))

int __ASN1_TIME_check(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * t)="\tjsr\t-978(a6)";
#define ASN1_TIME_check(t) __ASN1_TIME_check(AmiSSLBase, (t))

ASN1_GENERALIZEDTIME * __ASN1_TIME_to_generalizedtime(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * t, __reg("a1") ASN1_GENERALIZEDTIME ** out)="\tjsr\t-984(a6)";
#define ASN1_TIME_to_generalizedtime(t, out) __ASN1_TIME_to_generalizedtime(AmiSSLBase, (t), (out))

int __i2d_ASN1_SET(__reg("a6") struct Library *, __reg("a0") STACK * a, __reg("a1") unsigned char ** pp, __reg("a2") int (*func)(), __reg("d0") LONG ex_tag, __reg("d1") LONG ex_class, __reg("d2") LONG is_set)="\tjsr\t-990(a6)";
#define i2d_ASN1_SET(a, pp, func, ex_tag, ex_class, is_set) __i2d_ASN1_SET(AmiSSLBase, (a), (pp), (func), (ex_tag), (ex_class), (is_set))

STACK * __d2i_ASN1_SET(__reg("a6") struct Library *, __reg("a0") STACK ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length, __reg("a2") char * (*func)(), __reg("a3") void (*free_func)(void *), __reg("d1") LONG ex_tag, __reg("d2") LONG ex_class)="\tjsr\t-996(a6)";
#define d2i_ASN1_SET(a, pp, length, func, free_func, ex_tag, ex_class) __d2i_ASN1_SET(AmiSSLBase, (a), (pp), (length), (func), (free_func), (ex_tag), (ex_class))

int __i2a_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_INTEGER * a)="\tjsr\t-1002(a6)";
#define i2a_ASN1_INTEGER(bp, a) __i2a_ASN1_INTEGER(AmiSSLBase, (bp), (a))

int __a2i_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_INTEGER * bs, __reg("a2") char * buf, __reg("d0") LONG size)="\tjsr\t-1008(a6)";
#define a2i_ASN1_INTEGER(bp, bs, buf, size) __a2i_ASN1_INTEGER(AmiSSLBase, (bp), (bs), (buf), (size))

int __i2a_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_ENUMERATED * a)="\tjsr\t-1014(a6)";
#define i2a_ASN1_ENUMERATED(bp, a) __i2a_ASN1_ENUMERATED(AmiSSLBase, (bp), (a))

int __a2i_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_ENUMERATED * bs, __reg("a2") char * buf, __reg("d0") LONG size)="\tjsr\t-1020(a6)";
#define a2i_ASN1_ENUMERATED(bp, bs, buf, size) __a2i_ASN1_ENUMERATED(AmiSSLBase, (bp), (bs), (buf), (size))

int __i2a_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_OBJECT * a)="\tjsr\t-1026(a6)";
#define i2a_ASN1_OBJECT(bp, a) __i2a_ASN1_OBJECT(AmiSSLBase, (bp), (a))

int __a2i_ASN1_STRING(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_STRING * bs, __reg("a2") char * buf, __reg("d0") LONG size)="\tjsr\t-1032(a6)";
#define a2i_ASN1_STRING(bp, bs, buf, size) __a2i_ASN1_STRING(AmiSSLBase, (bp), (bs), (buf), (size))

int __i2a_ASN1_STRING(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_STRING * a, __reg("d0") LONG type)="\tjsr\t-1038(a6)";
#define i2a_ASN1_STRING(bp, a, type) __i2a_ASN1_STRING(AmiSSLBase, (bp), (a), (type))

int __i2t_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG buf_len, __reg("a1") ASN1_OBJECT * a)="\tjsr\t-1044(a6)";
#define i2t_ASN1_OBJECT(buf, buf_len, a) __i2t_ASN1_OBJECT(AmiSSLBase, (buf), (buf_len), (a))

int __a2d_ASN1_OBJECT(__reg("a6") struct Library *, __reg("a0") unsigned char * out, __reg("d0") LONG olen, __reg("a1") const char * buf, __reg("d1") LONG num)="\tjsr\t-1050(a6)";
#define a2d_ASN1_OBJECT(out, olen, buf, num) __a2d_ASN1_OBJECT(AmiSSLBase, (out), (olen), (buf), (num))

ASN1_OBJECT * __ASN1_OBJECT_create(__reg("a6") struct Library *, __reg("d0") LONG nid, __reg("a0") unsigned char * data, __reg("d1") LONG len, __reg("a1") const char * sn, __reg("a2") const char * ln)="\tjsr\t-1056(a6)";
#define ASN1_OBJECT_create(nid, data, len, sn, ln) __ASN1_OBJECT_create(AmiSSLBase, (nid), (data), (len), (sn), (ln))

int __ASN1_INTEGER_set(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * a, __reg("d0") long v)="\tjsr\t-1062(a6)";
#define ASN1_INTEGER_set(a, v) __ASN1_INTEGER_set(AmiSSLBase, (a), (v))

long __ASN1_INTEGER_get(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * a)="\tjsr\t-1068(a6)";
#define ASN1_INTEGER_get(a) __ASN1_INTEGER_get(AmiSSLBase, (a))

ASN1_INTEGER * __BN_to_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") BIGNUM * bn, __reg("a1") ASN1_INTEGER * ai)="\tjsr\t-1074(a6)";
#define BN_to_ASN1_INTEGER(bn, ai) __BN_to_ASN1_INTEGER(AmiSSLBase, (bn), (ai))

BIGNUM * __ASN1_INTEGER_to_BN(__reg("a6") struct Library *, __reg("a0") ASN1_INTEGER * ai, __reg("a1") BIGNUM * bn)="\tjsr\t-1080(a6)";
#define ASN1_INTEGER_to_BN(ai, bn) __ASN1_INTEGER_to_BN(AmiSSLBase, (ai), (bn))

int __ASN1_ENUMERATED_set(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED * a, __reg("d0") long v)="\tjsr\t-1086(a6)";
#define ASN1_ENUMERATED_set(a, v) __ASN1_ENUMERATED_set(AmiSSLBase, (a), (v))

long __ASN1_ENUMERATED_get(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED * a)="\tjsr\t-1092(a6)";
#define ASN1_ENUMERATED_get(a) __ASN1_ENUMERATED_get(AmiSSLBase, (a))

ASN1_ENUMERATED * __BN_to_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") BIGNUM * bn, __reg("a1") ASN1_ENUMERATED * ai)="\tjsr\t-1098(a6)";
#define BN_to_ASN1_ENUMERATED(bn, ai) __BN_to_ASN1_ENUMERATED(AmiSSLBase, (bn), (ai))

BIGNUM * __ASN1_ENUMERATED_to_BN(__reg("a6") struct Library *, __reg("a0") ASN1_ENUMERATED * ai, __reg("a1") BIGNUM * bn)="\tjsr\t-1104(a6)";
#define ASN1_ENUMERATED_to_BN(ai, bn) __ASN1_ENUMERATED_to_BN(AmiSSLBase, (ai), (bn))

int __ASN1_PRINTABLE_type(__reg("a6") struct Library *, __reg("a0") const unsigned char * s, __reg("d0") LONG max)="\tjsr\t-1110(a6)";
#define ASN1_PRINTABLE_type(s, max) __ASN1_PRINTABLE_type(AmiSSLBase, (s), (max))

int __i2d_ASN1_bytes(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * a, __reg("a1") unsigned char ** pp, __reg("d0") LONG t, __reg("d1") LONG xclass)="\tjsr\t-1116(a6)";
#define i2d_ASN1_bytes(a, pp, t, xclass) __i2d_ASN1_bytes(AmiSSLBase, (a), (pp), (t), (xclass))

ASN1_STRING * __d2i_ASN1_bytes(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length, __reg("d1") LONG Ptag, __reg("d2") LONG Pclass)="\tjsr\t-1122(a6)";
#define d2i_ASN1_bytes(a, pp, length, Ptag, Pclass) __d2i_ASN1_bytes(AmiSSLBase, (a), (pp), (length), (Ptag), (Pclass))

unsigned long __ASN1_tag2bit(__reg("a6") struct Library *, __reg("d0") LONG t)="\tjsr\t-1128(a6)";
#define ASN1_tag2bit(t) __ASN1_tag2bit(AmiSSLBase, (t))

ASN1_STRING * __d2i_ASN1_type_bytes(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length, __reg("d1") LONG type)="\tjsr\t-1134(a6)";
#define d2i_ASN1_type_bytes(a, pp, length, type) __d2i_ASN1_type_bytes(AmiSSLBase, (a), (pp), (length), (type))

int __asn1_Finish(__reg("a6") struct Library *, __reg("a0") ASN1_CTX * c)="\tjsr\t-1140(a6)";
#define asn1_Finish(c) __asn1_Finish(AmiSSLBase, (c))

int __ASN1_get_object(__reg("a6") struct Library *, __reg("a0") unsigned char ** pp, __reg("a1") long * plength, __reg("a2") int * ptag, __reg("a3") int * pclass, __reg("d0") long omax)="\tjsr\t-1146(a6)";
#define ASN1_get_object(pp, plength, ptag, pclass, omax) __ASN1_get_object(AmiSSLBase, (pp), (plength), (ptag), (pclass), (omax))

int __ASN1_check_infinite_end(__reg("a6") struct Library *, __reg("a0") unsigned char ** p, __reg("d0") long len)="\tjsr\t-1152(a6)";
#define ASN1_check_infinite_end(p, len) __ASN1_check_infinite_end(AmiSSLBase, (p), (len))

void __ASN1_put_object(__reg("a6") struct Library *, __reg("a0") unsigned char ** pp, __reg("d0") LONG constructed, __reg("d1") LONG length, __reg("d2") LONG t, __reg("d3") LONG xclass)="\tjsr\t-1158(a6)";
#define ASN1_put_object(pp, constructed, length, t, xclass) __ASN1_put_object(AmiSSLBase, (pp), (constructed), (length), (t), (xclass))

int __ASN1_object_size(__reg("a6") struct Library *, __reg("d0") LONG constructed, __reg("d1") LONG length, __reg("d2") LONG t)="\tjsr\t-1164(a6)";
#define ASN1_object_size(constructed, length, t) __ASN1_object_size(AmiSSLBase, (constructed), (length), (t))

char * __ASN1_dup(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") char * (*d2i)(), __reg("a2") char * x)="\tjsr\t-1170(a6)";
#define ASN1_dup(i2d, d2i, x) __ASN1_dup(AmiSSLBase, (i2d), (d2i), (x))

void * __ASN1_item_dup(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") void * x)="\tjsr\t-1176(a6)";
#define ASN1_item_dup(it, x) __ASN1_item_dup(AmiSSLBase, (it), (x))

int __ASN1_STRING_to_UTF8(__reg("a6") struct Library *, __reg("a0") unsigned char ** out, __reg("a1") ASN1_STRING * in)="\tjsr\t-1182(a6)";
#define ASN1_STRING_to_UTF8(out, in) __ASN1_STRING_to_UTF8(AmiSSLBase, (out), (in))

char * __ASN1_d2i_bio(__reg("a6") struct Library *, __reg("a0") char * (*xnew)(), __reg("a1") char * (*d2i)(), __reg("a2") BIO * bp, __reg("a3") unsigned char ** x)="\tjsr\t-1188(a6)";
#define ASN1_d2i_bio(xnew, d2i, bp, x) __ASN1_d2i_bio(AmiSSLBase, (xnew), (d2i), (bp), (x))

void * __ASN1_item_d2i_bio(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") BIO * in, __reg("a2") void * x)="\tjsr\t-1194(a6)";
#define ASN1_item_d2i_bio(it, in, x) __ASN1_item_d2i_bio(AmiSSLBase, (it), (in), (x))

int __ASN1_i2d_bio(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") BIO * out, __reg("a2") unsigned char * x)="\tjsr\t-1200(a6)";
#define ASN1_i2d_bio(i2d, out, x) __ASN1_i2d_bio(AmiSSLBase, (i2d), (out), (x))

int __ASN1_item_i2d_bio(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") BIO * out, __reg("a2") void * x)="\tjsr\t-1206(a6)";
#define ASN1_item_i2d_bio(it, out, x) __ASN1_item_i2d_bio(AmiSSLBase, (it), (out), (x))

int __ASN1_UTCTIME_print(__reg("a6") struct Library *, __reg("a0") BIO * fp, __reg("a1") ASN1_UTCTIME * a)="\tjsr\t-1212(a6)";
#define ASN1_UTCTIME_print(fp, a) __ASN1_UTCTIME_print(AmiSSLBase, (fp), (a))

int __ASN1_GENERALIZEDTIME_print(__reg("a6") struct Library *, __reg("a0") BIO * fp, __reg("a1") ASN1_GENERALIZEDTIME * a)="\tjsr\t-1218(a6)";
#define ASN1_GENERALIZEDTIME_print(fp, a) __ASN1_GENERALIZEDTIME_print(AmiSSLBase, (fp), (a))

int __ASN1_TIME_print(__reg("a6") struct Library *, __reg("a0") BIO * fp, __reg("a1") ASN1_TIME * a)="\tjsr\t-1224(a6)";
#define ASN1_TIME_print(fp, a) __ASN1_TIME_print(AmiSSLBase, (fp), (a))

int __ASN1_STRING_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ASN1_STRING * v)="\tjsr\t-1230(a6)";
#define ASN1_STRING_print(bp, v) __ASN1_STRING_print(AmiSSLBase, (bp), (v))

int __ASN1_STRING_print_ex(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") ASN1_STRING * str, __reg("d0") unsigned long flags)="\tjsr\t-1236(a6)";
#define ASN1_STRING_print_ex(out, str, flags) __ASN1_STRING_print_ex(AmiSSLBase, (out), (str), (flags))

int __ASN1_parse(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") unsigned char * pp, __reg("d0") long len, __reg("d1") LONG indent)="\tjsr\t-1242(a6)";
#define ASN1_parse(bp, pp, len, indent) __ASN1_parse(AmiSSLBase, (bp), (pp), (len), (indent))

int __ASN1_parse_dump(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") unsigned char * pp, __reg("d0") long len, __reg("d1") LONG indent, __reg("d2") LONG dump)="\tjsr\t-1248(a6)";
#define ASN1_parse_dump(bp, pp, len, indent, dump) __ASN1_parse_dump(AmiSSLBase, (bp), (pp), (len), (indent), (dump))

const char * __ASN1_tag2str(__reg("a6") struct Library *, __reg("d0") LONG t)="\tjsr\t-1254(a6)";
#define ASN1_tag2str(t) __ASN1_tag2str(AmiSSLBase, (t))

int __i2d_ASN1_HEADER(__reg("a6") struct Library *, __reg("a0") ASN1_HEADER * a, __reg("a1") unsigned char ** pp)="\tjsr\t-1260(a6)";
#define i2d_ASN1_HEADER(a, pp) __i2d_ASN1_HEADER(AmiSSLBase, (a), (pp))

ASN1_HEADER * __d2i_ASN1_HEADER(__reg("a6") struct Library *, __reg("a0") ASN1_HEADER ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-1266(a6)";
#define d2i_ASN1_HEADER(a, pp, length) __d2i_ASN1_HEADER(AmiSSLBase, (a), (pp), (length))

ASN1_HEADER * __ASN1_HEADER_new(__reg("a6") struct Library *)="\tjsr\t-1272(a6)";
#define ASN1_HEADER_new() __ASN1_HEADER_new(AmiSSLBase)

void __ASN1_HEADER_free(__reg("a6") struct Library *, __reg("a0") ASN1_HEADER * a)="\tjsr\t-1278(a6)";
#define ASN1_HEADER_free(a) __ASN1_HEADER_free(AmiSSLBase, (a))

int __ASN1_UNIVERSALSTRING_to_string(__reg("a6") struct Library *, __reg("a0") ASN1_UNIVERSALSTRING * s)="\tjsr\t-1284(a6)";
#define ASN1_UNIVERSALSTRING_to_string(s) __ASN1_UNIVERSALSTRING_to_string(AmiSSLBase, (s))

ASN1_METHOD * __X509_asn1_meth(__reg("a6") struct Library *)="\tjsr\t-1290(a6)";
#define X509_asn1_meth() __X509_asn1_meth(AmiSSLBase)

ASN1_METHOD * __RSAPrivateKey_asn1_meth(__reg("a6") struct Library *)="\tjsr\t-1296(a6)";
#define RSAPrivateKey_asn1_meth() __RSAPrivateKey_asn1_meth(AmiSSLBase)

ASN1_METHOD * __ASN1_IA5STRING_asn1_meth(__reg("a6") struct Library *)="\tjsr\t-1302(a6)";
#define ASN1_IA5STRING_asn1_meth() __ASN1_IA5STRING_asn1_meth(AmiSSLBase)

ASN1_METHOD * __ASN1_BIT_STRING_asn1_meth(__reg("a6") struct Library *)="\tjsr\t-1308(a6)";
#define ASN1_BIT_STRING_asn1_meth() __ASN1_BIT_STRING_asn1_meth(AmiSSLBase)

int __ASN1_TYPE_set_octetstring(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("a1") unsigned char * data, __reg("d0") LONG len)="\tjsr\t-1314(a6)";
#define ASN1_TYPE_set_octetstring(a, data, len) __ASN1_TYPE_set_octetstring(AmiSSLBase, (a), (data), (len))

int __ASN1_TYPE_get_octetstring(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("a1") unsigned char * data, __reg("d0") LONG max_len)="\tjsr\t-1320(a6)";
#define ASN1_TYPE_get_octetstring(a, data, max_len) __ASN1_TYPE_get_octetstring(AmiSSLBase, (a), (data), (max_len))

int __ASN1_TYPE_set_int_octetstring(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("d0") long num, __reg("a1") unsigned char * data, __reg("d1") LONG len)="\tjsr\t-1326(a6)";
#define ASN1_TYPE_set_int_octetstring(a, num, data, len) __ASN1_TYPE_set_int_octetstring(AmiSSLBase, (a), (num), (data), (len))

int __ASN1_TYPE_get_int_octetstring(__reg("a6") struct Library *, __reg("a0") ASN1_TYPE * a, __reg("a1") long * num, __reg("a2") unsigned char * data, __reg("d0") LONG max_len)="\tjsr\t-1332(a6)";
#define ASN1_TYPE_get_int_octetstring(a, num, data, max_len) __ASN1_TYPE_get_int_octetstring(AmiSSLBase, (a), (num), (data), (max_len))

STACK * __ASN1_seq_unpack(__reg("a6") struct Library *, __reg("a0") unsigned char * buf, __reg("d0") LONG len, __reg("a1") char * (*d2i)(), __reg("a2") void (*free_func)(void *))="\tjsr\t-1338(a6)";
#define ASN1_seq_unpack(buf, len, d2i, free_func) __ASN1_seq_unpack(AmiSSLBase, (buf), (len), (d2i), (free_func))

unsigned char * __ASN1_seq_pack(__reg("a6") struct Library *, __reg("a0") STACK * safes, __reg("a1") int (*i2d)(), __reg("a2") unsigned char ** buf, __reg("a3") int * len)="\tjsr\t-1344(a6)";
#define ASN1_seq_pack(safes, i2d, buf, len) __ASN1_seq_pack(AmiSSLBase, (safes), (i2d), (buf), (len))

void * __ASN1_unpack_string(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * oct, __reg("a1") char * (*d2i)())="\tjsr\t-1350(a6)";
#define ASN1_unpack_string(oct, d2i) __ASN1_unpack_string(AmiSSLBase, (oct), (d2i))

void * __ASN1_item_unpack(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * oct, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1356(a6)";
#define ASN1_item_unpack(oct, it) __ASN1_item_unpack(AmiSSLBase, (oct), (it))

ASN1_STRING * __ASN1_pack_string(__reg("a6") struct Library *, __reg("a0") void * obj, __reg("a1") int (*i2d)(), __reg("a2") ASN1_OCTET_STRING ** oct)="\tjsr\t-1362(a6)";
#define ASN1_pack_string(obj, i2d, oct) __ASN1_pack_string(AmiSSLBase, (obj), (i2d), (oct))

ASN1_STRING * __ASN1_item_pack(__reg("a6") struct Library *, __reg("a0") void * obj, __reg("a1") const ASN1_ITEM * it, __reg("a2") ASN1_OCTET_STRING ** oct)="\tjsr\t-1368(a6)";
#define ASN1_item_pack(obj, it, oct) __ASN1_item_pack(AmiSSLBase, (obj), (it), (oct))

void __ASN1_STRING_set_default_mask(__reg("a6") struct Library *, __reg("d0") unsigned long mask)="\tjsr\t-1374(a6)";
#define ASN1_STRING_set_default_mask(mask) __ASN1_STRING_set_default_mask(AmiSSLBase, (mask))

int __ASN1_STRING_set_default_mask_asc(__reg("a6") struct Library *, __reg("a0") char * p)="\tjsr\t-1380(a6)";
#define ASN1_STRING_set_default_mask_asc(p) __ASN1_STRING_set_default_mask_asc(AmiSSLBase, (p))

unsigned long __ASN1_STRING_get_default_mask(__reg("a6") struct Library *)="\tjsr\t-1386(a6)";
#define ASN1_STRING_get_default_mask() __ASN1_STRING_get_default_mask(AmiSSLBase)

int __ASN1_mbstring_copy(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** out, __reg("a1") const unsigned char * in, __reg("d0") LONG len, __reg("d1") LONG inform, __reg("d2") unsigned long mask)="\tjsr\t-1392(a6)";
#define ASN1_mbstring_copy(out, in, len, inform, mask) __ASN1_mbstring_copy(AmiSSLBase, (out), (in), (len), (inform), (mask))

int __ASN1_mbstring_ncopy(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** out, __reg("a1") const unsigned char * in, __reg("d0") LONG len, __reg("d1") LONG inform, __reg("d2") unsigned long mask, __reg("d3") long minsize, __reg("d4") long maxsize)="\tjsr\t-1398(a6)";
#define ASN1_mbstring_ncopy(out, in, len, inform, mask, minsize, maxsize) __ASN1_mbstring_ncopy(AmiSSLBase, (out), (in), (len), (inform), (mask), (minsize), (maxsize))

ASN1_STRING * __ASN1_STRING_set_by_NID(__reg("a6") struct Library *, __reg("a0") ASN1_STRING ** out, __reg("a1") const unsigned char * in, __reg("d0") LONG inlen, __reg("d1") LONG inform, __reg("d2") LONG nid)="\tjsr\t-1404(a6)";
#define ASN1_STRING_set_by_NID(out, in, inlen, inform, nid) __ASN1_STRING_set_by_NID(AmiSSLBase, (out), (in), (inlen), (inform), (nid))

ASN1_STRING_TABLE * __ASN1_STRING_TABLE_get(__reg("a6") struct Library *, __reg("d0") LONG nid)="\tjsr\t-1410(a6)";
#define ASN1_STRING_TABLE_get(nid) __ASN1_STRING_TABLE_get(AmiSSLBase, (nid))

int __ASN1_STRING_TABLE_add(__reg("a6") struct Library *, __reg("d0") LONG a, __reg("d1") long b, __reg("d2") long c, __reg("d3") unsigned long d, __reg("d4") unsigned long e)="\tjsr\t-1416(a6)";
#define ASN1_STRING_TABLE_add(a, b, c, d, e) __ASN1_STRING_TABLE_add(AmiSSLBase, (a), (b), (c), (d), (e))

void __ASN1_STRING_TABLE_cleanup(__reg("a6") struct Library *)="\tjsr\t-1422(a6)";
#define ASN1_STRING_TABLE_cleanup() __ASN1_STRING_TABLE_cleanup(AmiSSLBase)

ASN1_VALUE * __ASN1_item_new(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it)="\tjsr\t-1428(a6)";
#define ASN1_item_new(it) __ASN1_item_new(AmiSSLBase, (it))

void __ASN1_item_free(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE * val, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1434(a6)";
#define ASN1_item_free(val, it) __ASN1_item_free(AmiSSLBase, (val), (it))

ASN1_VALUE * __ASN1_item_d2i(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** val, __reg("a1") unsigned char ** in, __reg("d0") long len, __reg("a2") const ASN1_ITEM * it)="\tjsr\t-1440(a6)";
#define ASN1_item_d2i(val, in, len, it) __ASN1_item_d2i(AmiSSLBase, (val), (in), (len), (it))

int __ASN1_item_i2d(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE * val, __reg("a1") unsigned char ** out, __reg("a2") const ASN1_ITEM * it)="\tjsr\t-1446(a6)";
#define ASN1_item_i2d(val, out, it) __ASN1_item_i2d(AmiSSLBase, (val), (out), (it))

void __ASN1_add_oid_module(__reg("a6") struct Library *)="\tjsr\t-1452(a6)";
#define ASN1_add_oid_module() __ASN1_add_oid_module(AmiSSLBase)

void __ERR_load_ASN1_strings(__reg("a6") struct Library *)="\tjsr\t-1458(a6)";
#define ERR_load_ASN1_strings() __ERR_load_ASN1_strings(AmiSSLBase)

int __asn1_GetSequence(__reg("a6") struct Library *, __reg("a0") ASN1_CTX * c, __reg("a1") long * length)="\tjsr\t-1464(a6)";
#define asn1_GetSequence(c, length) __asn1_GetSequence(AmiSSLBase, (c), (length))

void __asn1_add_error(__reg("a6") struct Library *, __reg("a0") unsigned char * address, __reg("d0") LONG offset)="\tjsr\t-1470(a6)";
#define asn1_add_error(address, offset) __asn1_add_error(AmiSSLBase, (address), (offset))

const ASN1_ITEM * __ASN1_BOOLEAN_it(__reg("a6") struct Library *)="\tjsr\t-1476(a6)";
#define ASN1_BOOLEAN_it() __ASN1_BOOLEAN_it(AmiSSLBase)

const ASN1_ITEM * __ASN1_TBOOLEAN_it(__reg("a6") struct Library *)="\tjsr\t-1482(a6)";
#define ASN1_TBOOLEAN_it() __ASN1_TBOOLEAN_it(AmiSSLBase)

const ASN1_ITEM * __ASN1_FBOOLEAN_it(__reg("a6") struct Library *)="\tjsr\t-1488(a6)";
#define ASN1_FBOOLEAN_it() __ASN1_FBOOLEAN_it(AmiSSLBase)

const ASN1_ITEM * __ASN1_SEQUENCE_it(__reg("a6") struct Library *)="\tjsr\t-1494(a6)";
#define ASN1_SEQUENCE_it() __ASN1_SEQUENCE_it(AmiSSLBase)

const ASN1_ITEM * __CBIGNUM_it(__reg("a6") struct Library *)="\tjsr\t-1500(a6)";
#define CBIGNUM_it() __CBIGNUM_it(AmiSSLBase)

const ASN1_ITEM * __BIGNUM_it(__reg("a6") struct Library *)="\tjsr\t-1506(a6)";
#define BIGNUM_it() __BIGNUM_it(AmiSSLBase)

const ASN1_ITEM * __LONG_it(__reg("a6") struct Library *)="\tjsr\t-1512(a6)";
#define LONG_it() __LONG_it(AmiSSLBase)

const ASN1_ITEM * __ZLONG_it(__reg("a6") struct Library *)="\tjsr\t-1518(a6)";
#define ZLONG_it() __ZLONG_it(AmiSSLBase)

int __ASN1_item_ex_new(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1524(a6)";
#define ASN1_item_ex_new(pval, it) __ASN1_item_ex_new(AmiSSLBase, (pval), (it))

void __ASN1_item_ex_free(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1530(a6)";
#define ASN1_item_ex_free(pval, it) __ASN1_item_ex_free(AmiSSLBase, (pval), (it))

int __ASN1_template_new(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_TEMPLATE * tt)="\tjsr\t-1536(a6)";
#define ASN1_template_new(pval, tt) __ASN1_template_new(AmiSSLBase, (pval), (tt))

int __ASN1_primitive_new(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1542(a6)";
#define ASN1_primitive_new(pval, it) __ASN1_primitive_new(AmiSSLBase, (pval), (it))

void __ASN1_template_free(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_TEMPLATE * tt)="\tjsr\t-1548(a6)";
#define ASN1_template_free(pval, tt) __ASN1_template_free(AmiSSLBase, (pval), (tt))

int __ASN1_template_d2i(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char ** in, __reg("d0") long len, __reg("a2") const ASN1_TEMPLATE * tt)="\tjsr\t-1554(a6)";
#define ASN1_template_d2i(pval, in, len, tt) __ASN1_template_d2i(AmiSSLBase, (pval), (in), (len), (tt))

int __ASN1_item_ex_d2i(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char ** in, __reg("d0") long len, __reg("a2") const ASN1_ITEM * it, __reg("d1") LONG t, __reg("d2") LONG aclass, __reg("d3") LONG opt, __reg("a3") ASN1_TLC * ctx)="\tjsr\t-1560(a6)";
#define ASN1_item_ex_d2i(pval, in, len, it, t, aclass, opt, ctx) __ASN1_item_ex_d2i(AmiSSLBase, (pval), (in), (len), (it), (t), (aclass), (opt), (ctx))

int __ASN1_item_ex_i2d(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char ** out, __reg("a2") const ASN1_ITEM * it, __reg("d0") LONG t, __reg("d1") LONG aclass)="\tjsr\t-1566(a6)";
#define ASN1_item_ex_i2d(pval, out, it, t, aclass) __ASN1_item_ex_i2d(AmiSSLBase, (pval), (out), (it), (t), (aclass))

int __ASN1_template_i2d(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char ** out, __reg("a2") const ASN1_TEMPLATE * tt)="\tjsr\t-1572(a6)";
#define ASN1_template_i2d(pval, out, tt) __ASN1_template_i2d(AmiSSLBase, (pval), (out), (tt))

void __ASN1_primitive_free(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1578(a6)";
#define ASN1_primitive_free(pval, it) __ASN1_primitive_free(AmiSSLBase, (pval), (it))

int __asn1_ex_i2c(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char * cont, __reg("a2") int * putype, __reg("a3") const ASN1_ITEM * it)="\tjsr\t-1584(a6)";
#define asn1_ex_i2c(pval, cont, putype, it) __asn1_ex_i2c(AmiSSLBase, (pval), (cont), (putype), (it))

int __asn1_ex_c2i(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char * cont, __reg("d0") LONG len, __reg("d1") LONG utype, __reg("a2") char * free_cont, __reg("a3") const ASN1_ITEM * it)="\tjsr\t-1590(a6)";
#define asn1_ex_c2i(pval, cont, len, utype, free_cont, it) __asn1_ex_c2i(AmiSSLBase, (pval), (cont), (len), (utype), (free_cont), (it))

int __asn1_get_choice_selector(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1596(a6)";
#define asn1_get_choice_selector(pval, it) __asn1_get_choice_selector(AmiSSLBase, (pval), (it))

int __asn1_set_choice_selector(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("d0") LONG value, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1602(a6)";
#define asn1_set_choice_selector(pval, value, it) __asn1_set_choice_selector(AmiSSLBase, (pval), (value), (it))

ASN1_VALUE ** __asn1_get_field_ptr(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_TEMPLATE * tt)="\tjsr\t-1608(a6)";
#define asn1_get_field_ptr(pval, tt) __asn1_get_field_ptr(AmiSSLBase, (pval), (tt))

const ASN1_TEMPLATE * __asn1_do_adb(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_TEMPLATE * tt, __reg("d0") LONG nullerr)="\tjsr\t-1614(a6)";
#define asn1_do_adb(pval, tt, nullerr) __asn1_do_adb(AmiSSLBase, (pval), (tt), (nullerr))

int __asn1_do_lock(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("d0") LONG op, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1620(a6)";
#define asn1_do_lock(pval, op, it) __asn1_do_lock(AmiSSLBase, (pval), (op), (it))

void __asn1_enc_init(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1626(a6)";
#define asn1_enc_init(pval, it) __asn1_enc_init(AmiSSLBase, (pval), (it))

void __asn1_enc_free(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") const ASN1_ITEM * it)="\tjsr\t-1632(a6)";
#define asn1_enc_free(pval, it) __asn1_enc_free(AmiSSLBase, (pval), (it))

int __asn1_enc_restore(__reg("a6") struct Library *, __reg("a0") int * len, __reg("a1") unsigned char ** out, __reg("a2") ASN1_VALUE ** pval, __reg("a3") const ASN1_ITEM * it)="\tjsr\t-1638(a6)";
#define asn1_enc_restore(len, out, pval, it) __asn1_enc_restore(AmiSSLBase, (len), (out), (pval), (it))

int __asn1_enc_save(__reg("a6") struct Library *, __reg("a0") ASN1_VALUE ** pval, __reg("a1") unsigned char * in, __reg("d0") LONG inlen, __reg("a2") const ASN1_ITEM * it)="\tjsr\t-1644(a6)";
#define asn1_enc_save(pval, in, inlen, it) __asn1_enc_save(AmiSSLBase, (pval), (in), (inlen), (it))

size_t __BIO_ctrl_pending(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1650(a6)";
#define BIO_ctrl_pending(b) __BIO_ctrl_pending(AmiSSLBase, (b))

size_t __BIO_ctrl_wpending(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1656(a6)";
#define BIO_ctrl_wpending(b) __BIO_ctrl_wpending(AmiSSLBase, (b))

size_t __BIO_ctrl_get_write_guarantee(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1662(a6)";
#define BIO_ctrl_get_write_guarantee(b) __BIO_ctrl_get_write_guarantee(AmiSSLBase, (b))

size_t __BIO_ctrl_get_read_request(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1668(a6)";
#define BIO_ctrl_get_read_request(b) __BIO_ctrl_get_read_request(AmiSSLBase, (b))

int __BIO_ctrl_reset_read_request(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1674(a6)";
#define BIO_ctrl_reset_read_request(b) __BIO_ctrl_reset_read_request(AmiSSLBase, (b))

int __BIO_set_ex_data(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("d0") LONG idx, __reg("a1") void * data)="\tjsr\t-1680(a6)";
#define BIO_set_ex_data(bio, idx, data) __BIO_set_ex_data(AmiSSLBase, (bio), (idx), (data))

void * __BIO_get_ex_data(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("d0") LONG idx)="\tjsr\t-1686(a6)";
#define BIO_get_ex_data(bio, idx) __BIO_get_ex_data(AmiSSLBase, (bio), (idx))

int __BIO_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-1692(a6)";
#define BIO_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __BIO_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

unsigned long __BIO_number_read(__reg("a6") struct Library *, __reg("a0") BIO * bio)="\tjsr\t-1698(a6)";
#define BIO_number_read(bio) __BIO_number_read(AmiSSLBase, (bio))

unsigned long __BIO_number_written(__reg("a6") struct Library *, __reg("a0") BIO * bio)="\tjsr\t-1704(a6)";
#define BIO_number_written(bio) __BIO_number_written(AmiSSLBase, (bio))

BIO_METHOD * __BIO_s_file(__reg("a6") struct Library *)="\tjsr\t-1710(a6)";
#define BIO_s_file() __BIO_s_file(AmiSSLBase)

BIO * __BIO_new_file(__reg("a6") struct Library *, __reg("a0") const char * filename, __reg("a1") const char * mode)="\tjsr\t-1716(a6)";
#define BIO_new_file(filename, mode) __BIO_new_file(AmiSSLBase, (filename), (mode))

BIO * __BIO_new_fp_amiga(__reg("a6") struct Library *, __reg("d0") BPTR stream, __reg("d1") LONG close_flag)="\tjsr\t-1722(a6)";
#define BIO_new_fp_amiga(stream, close_flag) __BIO_new_fp_amiga(AmiSSLBase, (stream), (close_flag))

BIO * __BIO_new(__reg("a6") struct Library *, __reg("a0") BIO_METHOD * type)="\tjsr\t-1728(a6)";
#define BIO_new(type) __BIO_new(AmiSSLBase, (type))

int __BIO_set(__reg("a6") struct Library *, __reg("a0") BIO * a, __reg("a1") BIO_METHOD * type)="\tjsr\t-1734(a6)";
#define BIO_set(a, type) __BIO_set(AmiSSLBase, (a), (type))

int __BIO_free(__reg("a6") struct Library *, __reg("a0") BIO * a)="\tjsr\t-1740(a6)";
#define BIO_free(a) __BIO_free(AmiSSLBase, (a))

void __BIO_vfree(__reg("a6") struct Library *, __reg("a0") BIO * a)="\tjsr\t-1746(a6)";
#define BIO_vfree(a) __BIO_vfree(AmiSSLBase, (a))

int __BIO_read(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") void * data, __reg("d0") LONG len)="\tjsr\t-1752(a6)";
#define BIO_read(b, data, len) __BIO_read(AmiSSLBase, (b), (data), (len))

int __BIO_gets(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") char * buf, __reg("d0") LONG size)="\tjsr\t-1758(a6)";
#define BIO_gets(bp, buf, size) __BIO_gets(AmiSSLBase, (bp), (buf), (size))

int __BIO_write(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") const void * data, __reg("d0") LONG len)="\tjsr\t-1764(a6)";
#define BIO_write(b, data, len) __BIO_write(AmiSSLBase, (b), (data), (len))

int __BIO_puts(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const char * buf)="\tjsr\t-1770(a6)";
#define BIO_puts(bp, buf) __BIO_puts(AmiSSLBase, (bp), (buf))

int __BIO_indent(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("d0") LONG indent, __reg("d1") LONG max)="\tjsr\t-1776(a6)";
#define BIO_indent(b, indent, max) __BIO_indent(AmiSSLBase, (b), (indent), (max))

long __BIO_ctrl(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("a1") void * parg)="\tjsr\t-1782(a6)";
#define BIO_ctrl(bp, cmd, larg, parg) __BIO_ctrl(AmiSSLBase, (bp), (cmd), (larg), (parg))

long __BIO_callback_ctrl(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("d0") LONG cmd, __reg("a1") void (*fp)(struct bio_st *, int, const char *, int, long, long))="\tjsr\t-1788(a6)";
#define BIO_callback_ctrl(b, cmd, fp) __BIO_callback_ctrl(AmiSSLBase, (b), (cmd), (fp))

char * __BIO_ptr_ctrl(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("d0") LONG cmd, __reg("d1") long larg)="\tjsr\t-1794(a6)";
#define BIO_ptr_ctrl(bp, cmd, larg) __BIO_ptr_ctrl(AmiSSLBase, (bp), (cmd), (larg))

long __BIO_int_ctrl(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("d2") LONG iarg)="\tjsr\t-1800(a6)";
#define BIO_int_ctrl(bp, cmd, larg, iarg) __BIO_int_ctrl(AmiSSLBase, (bp), (cmd), (larg), (iarg))

BIO * __BIO_push(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") BIO * append)="\tjsr\t-1806(a6)";
#define BIO_push(b, append) __BIO_push(AmiSSLBase, (b), (append))

BIO * __BIO_pop(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1812(a6)";
#define BIO_pop(b) __BIO_pop(AmiSSLBase, (b))

void __BIO_free_all(__reg("a6") struct Library *, __reg("a0") BIO * a)="\tjsr\t-1818(a6)";
#define BIO_free_all(a) __BIO_free_all(AmiSSLBase, (a))

BIO * __BIO_find_type(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("d0") LONG bio_type)="\tjsr\t-1824(a6)";
#define BIO_find_type(b, bio_type) __BIO_find_type(AmiSSLBase, (b), (bio_type))

BIO * __BIO_next(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-1830(a6)";
#define BIO_next(b) __BIO_next(AmiSSLBase, (b))

BIO * __BIO_get_retry_BIO(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") int * reason)="\tjsr\t-1836(a6)";
#define BIO_get_retry_BIO(bio, reason) __BIO_get_retry_BIO(AmiSSLBase, (bio), (reason))

int __BIO_get_retry_reason(__reg("a6") struct Library *, __reg("a0") BIO * bio)="\tjsr\t-1842(a6)";
#define BIO_get_retry_reason(bio) __BIO_get_retry_reason(AmiSSLBase, (bio))

BIO * __BIO_dup_chain(__reg("a6") struct Library *, __reg("a0") BIO * in)="\tjsr\t-1848(a6)";
#define BIO_dup_chain(in) __BIO_dup_chain(AmiSSLBase, (in))

int __BIO_nread0(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") char ** buf)="\tjsr\t-1854(a6)";
#define BIO_nread0(bio, buf) __BIO_nread0(AmiSSLBase, (bio), (buf))

int __BIO_nread(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") char ** buf, __reg("d0") LONG num)="\tjsr\t-1860(a6)";
#define BIO_nread(bio, buf, num) __BIO_nread(AmiSSLBase, (bio), (buf), (num))

int __BIO_nwrite0(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") char ** buf)="\tjsr\t-1866(a6)";
#define BIO_nwrite0(bio, buf) __BIO_nwrite0(AmiSSLBase, (bio), (buf))

int __BIO_nwrite(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") char ** buf, __reg("d0") LONG num)="\tjsr\t-1872(a6)";
#define BIO_nwrite(bio, buf, num) __BIO_nwrite(AmiSSLBase, (bio), (buf), (num))

long __BIO_debug_callback(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("d0") LONG cmd, __reg("a1") const char * argp, __reg("d1") LONG argi, __reg("d2") long argl, __reg("d3") long ret)="\tjsr\t-1878(a6)";
#define BIO_debug_callback(bio, cmd, argp, argi, argl, ret) __BIO_debug_callback(AmiSSLBase, (bio), (cmd), (argp), (argi), (argl), (ret))

BIO_METHOD * __BIO_s_mem(__reg("a6") struct Library *)="\tjsr\t-1884(a6)";
#define BIO_s_mem() __BIO_s_mem(AmiSSLBase)

BIO * __BIO_new_mem_buf(__reg("a6") struct Library *, __reg("a0") void * buf, __reg("d0") LONG len)="\tjsr\t-1890(a6)";
#define BIO_new_mem_buf(buf, len) __BIO_new_mem_buf(AmiSSLBase, (buf), (len))

BIO_METHOD * __BIO_s_socket(__reg("a6") struct Library *)="\tjsr\t-1896(a6)";
#define BIO_s_socket() __BIO_s_socket(AmiSSLBase)

BIO_METHOD * __BIO_s_connect(__reg("a6") struct Library *)="\tjsr\t-1902(a6)";
#define BIO_s_connect() __BIO_s_connect(AmiSSLBase)

BIO_METHOD * __BIO_s_accept(__reg("a6") struct Library *)="\tjsr\t-1908(a6)";
#define BIO_s_accept() __BIO_s_accept(AmiSSLBase)

BIO_METHOD * __BIO_s_fd(__reg("a6") struct Library *)="\tjsr\t-1914(a6)";
#define BIO_s_fd() __BIO_s_fd(AmiSSLBase)

BIO_METHOD * __BIO_s_log(__reg("a6") struct Library *)="\tjsr\t-1920(a6)";
#define BIO_s_log() __BIO_s_log(AmiSSLBase)

BIO_METHOD * __BIO_s_bio(__reg("a6") struct Library *)="\tjsr\t-1926(a6)";
#define BIO_s_bio() __BIO_s_bio(AmiSSLBase)

BIO_METHOD * __BIO_s_null(__reg("a6") struct Library *)="\tjsr\t-1932(a6)";
#define BIO_s_null() __BIO_s_null(AmiSSLBase)

BIO_METHOD * __BIO_f_null(__reg("a6") struct Library *)="\tjsr\t-1938(a6)";
#define BIO_f_null() __BIO_f_null(AmiSSLBase)

BIO_METHOD * __BIO_f_buffer(__reg("a6") struct Library *)="\tjsr\t-1944(a6)";
#define BIO_f_buffer() __BIO_f_buffer(AmiSSLBase)

BIO_METHOD * __BIO_f_nbio_test(__reg("a6") struct Library *)="\tjsr\t-1950(a6)";
#define BIO_f_nbio_test() __BIO_f_nbio_test(AmiSSLBase)

int __BIO_sock_should_retry(__reg("a6") struct Library *, __reg("d0") LONG i)="\tjsr\t-1956(a6)";
#define BIO_sock_should_retry(i) __BIO_sock_should_retry(AmiSSLBase, (i))

int __BIO_sock_non_fatal_error(__reg("a6") struct Library *, __reg("d0") LONG error)="\tjsr\t-1962(a6)";
#define BIO_sock_non_fatal_error(error) __BIO_sock_non_fatal_error(AmiSSLBase, (error))

int __BIO_fd_should_retry(__reg("a6") struct Library *, __reg("d0") LONG i)="\tjsr\t-1968(a6)";
#define BIO_fd_should_retry(i) __BIO_fd_should_retry(AmiSSLBase, (i))

int __BIO_fd_non_fatal_error(__reg("a6") struct Library *, __reg("d0") LONG error)="\tjsr\t-1974(a6)";
#define BIO_fd_non_fatal_error(error) __BIO_fd_non_fatal_error(AmiSSLBase, (error))

int __BIO_dump(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") const char * bytes, __reg("d0") LONG len)="\tjsr\t-1980(a6)";
#define BIO_dump(b, bytes, len) __BIO_dump(AmiSSLBase, (b), (bytes), (len))

int __BIO_dump_indent(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") const char * bytes, __reg("d0") LONG len, __reg("d1") LONG indent)="\tjsr\t-1986(a6)";
#define BIO_dump_indent(b, bytes, len, indent) __BIO_dump_indent(AmiSSLBase, (b), (bytes), (len), (indent))

struct hostent * __BIO_gethostbyname(__reg("a6") struct Library *, __reg("a0") const char * name)="\tjsr\t-1992(a6)";
#define BIO_gethostbyname(name) __BIO_gethostbyname(AmiSSLBase, (name))

int __BIO_sock_error(__reg("a6") struct Library *, __reg("d0") LONG sock)="\tjsr\t-1998(a6)";
#define BIO_sock_error(sock) __BIO_sock_error(AmiSSLBase, (sock))

int __BIO_socket_ioctl(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("d1") long type, __reg("a0") void * arg)="\tjsr\t-2004(a6)";
#define BIO_socket_ioctl(fd, type, arg) __BIO_socket_ioctl(AmiSSLBase, (fd), (type), (arg))

int __BIO_socket_nbio(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("d1") LONG mode)="\tjsr\t-2010(a6)";
#define BIO_socket_nbio(fd, mode) __BIO_socket_nbio(AmiSSLBase, (fd), (mode))

int __BIO_get_port(__reg("a6") struct Library *, __reg("a0") const char * str, __reg("a1") unsigned short * port_ptr)="\tjsr\t-2016(a6)";
#define BIO_get_port(str, port_ptr) __BIO_get_port(AmiSSLBase, (str), (port_ptr))

int __BIO_get_host_ip(__reg("a6") struct Library *, __reg("a0") const char * str, __reg("a1") unsigned char * ip)="\tjsr\t-2022(a6)";
#define BIO_get_host_ip(str, ip) __BIO_get_host_ip(AmiSSLBase, (str), (ip))

int __BIO_get_accept_socket(__reg("a6") struct Library *, __reg("a0") char * host_port, __reg("d0") LONG mode)="\tjsr\t-2028(a6)";
#define BIO_get_accept_socket(host_port, mode) __BIO_get_accept_socket(AmiSSLBase, (host_port), (mode))

int __BIO_accept(__reg("a6") struct Library *, __reg("d0") LONG sock, __reg("a0") char ** ip_port)="\tjsr\t-2034(a6)";
#define BIO_accept(sock, ip_port) __BIO_accept(AmiSSLBase, (sock), (ip_port))

int __BIO_sock_init(__reg("a6") struct Library *)="\tjsr\t-2040(a6)";
#define BIO_sock_init() __BIO_sock_init(AmiSSLBase)

void __BIO_sock_cleanup(__reg("a6") struct Library *)="\tjsr\t-2046(a6)";
#define BIO_sock_cleanup() __BIO_sock_cleanup(AmiSSLBase)

int __BIO_set_tcp_ndelay(__reg("a6") struct Library *, __reg("d0") LONG sock, __reg("d1") LONG turn_on)="\tjsr\t-2052(a6)";
#define BIO_set_tcp_ndelay(sock, turn_on) __BIO_set_tcp_ndelay(AmiSSLBase, (sock), (turn_on))

BIO * __BIO_new_socket(__reg("a6") struct Library *, __reg("d0") LONG sock, __reg("d1") LONG close_flag)="\tjsr\t-2058(a6)";
#define BIO_new_socket(sock, close_flag) __BIO_new_socket(AmiSSLBase, (sock), (close_flag))

BIO * __BIO_new_fd(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("d1") LONG close_flag)="\tjsr\t-2064(a6)";
#define BIO_new_fd(fd, close_flag) __BIO_new_fd(AmiSSLBase, (fd), (close_flag))

BIO * __BIO_new_connect(__reg("a6") struct Library *, __reg("a0") char * host_port)="\tjsr\t-2070(a6)";
#define BIO_new_connect(host_port) __BIO_new_connect(AmiSSLBase, (host_port))

BIO * __BIO_new_accept(__reg("a6") struct Library *, __reg("a0") char * host_port)="\tjsr\t-2076(a6)";
#define BIO_new_accept(host_port) __BIO_new_accept(AmiSSLBase, (host_port))

int __BIO_new_bio_pair(__reg("a6") struct Library *, __reg("a0") BIO ** bio1, __reg("d0") ULONG writebuf1, __reg("a1") BIO ** bio2, __reg("d1") ULONG writebuf2)="\tjsr\t-2082(a6)";
#define BIO_new_bio_pair(bio1, writebuf1, bio2, writebuf2) __BIO_new_bio_pair(AmiSSLBase, (bio1), (writebuf1), (bio2), (writebuf2))

void __BIO_copy_next_retry(__reg("a6") struct Library *, __reg("a0") BIO * b)="\tjsr\t-2088(a6)";
#define BIO_copy_next_retry(b) __BIO_copy_next_retry(AmiSSLBase, (b))

int __BIO_vprintf(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") const char * format, __reg("d0") long * args)="\tjsr\t-2094(a6)";
#define BIO_vprintf(bio, format, args) __BIO_vprintf(AmiSSLBase, (bio), (format), (args))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
int __BIO_printf(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") const char * format, long args, ...)="\tmove.l\td0,-(a7)\n\tmove.l\ta7,d0\n\taddq.l\t#4,d0\n\tjsr\t-2094(a6)\n\tmove.l\t(a7)+,d0";
#define BIO_printf(bio, format, ...) __BIO_printf(AmiSSLBase, (bio), (format), __VA_ARGS__)
#endif

int __BIO_vsnprintf(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") ULONG n, __reg("a1") const char * format, __reg("d1") long * args)="\tjsr\t-2100(a6)";
#define BIO_vsnprintf(buf, n, format, args) __BIO_vsnprintf(AmiSSLBase, (buf), (n), (format), (args))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
int __BIO_snprintf(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") ULONG n, __reg("a1") const char * format, long args, ...)="\tmove.l\td1,-(a7)\n\tmove.l\ta7,d1\n\taddq.l\t#4,d1\n\tjsr\t-2100(a6)\n\tmove.l\t(a7)+,d1";
#define BIO_snprintf(buf, n, format, ...) __BIO_snprintf(AmiSSLBase, (buf), (n), (format), __VA_ARGS__)
#endif

void __ERR_load_BIO_strings(__reg("a6") struct Library *)="\tjsr\t-2106(a6)";
#define ERR_load_BIO_strings() __ERR_load_BIO_strings(AmiSSLBase)

const BIGNUM * __BN_value_one(__reg("a6") struct Library *)="\tjsr\t-2112(a6)";
#define BN_value_one() __BN_value_one(AmiSSLBase)

char * __BN_options(__reg("a6") struct Library *)="\tjsr\t-2118(a6)";
#define BN_options() __BN_options(AmiSSLBase)

BN_CTX * __BN_CTX_new(__reg("a6") struct Library *)="\tjsr\t-2124(a6)";
#define BN_CTX_new() __BN_CTX_new(AmiSSLBase)

void __BN_CTX_init(__reg("a6") struct Library *, __reg("a0") BN_CTX * c)="\tjsr\t-2130(a6)";
#define BN_CTX_init(c) __BN_CTX_init(AmiSSLBase, (c))

void __BN_CTX_free(__reg("a6") struct Library *, __reg("a0") BN_CTX * c)="\tjsr\t-2136(a6)";
#define BN_CTX_free(c) __BN_CTX_free(AmiSSLBase, (c))

void __BN_CTX_start(__reg("a6") struct Library *, __reg("a0") BN_CTX * ctx)="\tjsr\t-2142(a6)";
#define BN_CTX_start(ctx) __BN_CTX_start(AmiSSLBase, (ctx))

BIGNUM * __BN_CTX_get(__reg("a6") struct Library *, __reg("a0") BN_CTX * ctx)="\tjsr\t-2148(a6)";
#define BN_CTX_get(ctx) __BN_CTX_get(AmiSSLBase, (ctx))

void __BN_CTX_end(__reg("a6") struct Library *, __reg("a0") BN_CTX * ctx)="\tjsr\t-2154(a6)";
#define BN_CTX_end(ctx) __BN_CTX_end(AmiSSLBase, (ctx))

int __BN_rand(__reg("a6") struct Library *, __reg("a0") BIGNUM * rnd, __reg("d0") LONG bits, __reg("d1") LONG top, __reg("d2") LONG bottom)="\tjsr\t-2160(a6)";
#define BN_rand(rnd, bits, top, bottom) __BN_rand(AmiSSLBase, (rnd), (bits), (top), (bottom))

int __BN_pseudo_rand(__reg("a6") struct Library *, __reg("a0") BIGNUM * rnd, __reg("d0") LONG bits, __reg("d1") LONG top, __reg("d2") LONG bottom)="\tjsr\t-2166(a6)";
#define BN_pseudo_rand(rnd, bits, top, bottom) __BN_pseudo_rand(AmiSSLBase, (rnd), (bits), (top), (bottom))

int __BN_rand_range(__reg("a6") struct Library *, __reg("a0") BIGNUM * rnd, __reg("a1") BIGNUM * range)="\tjsr\t-2172(a6)";
#define BN_rand_range(rnd, range) __BN_rand_range(AmiSSLBase, (rnd), (range))

int __BN_pseudo_rand_range(__reg("a6") struct Library *, __reg("a0") BIGNUM * rnd, __reg("a1") BIGNUM * range)="\tjsr\t-2178(a6)";
#define BN_pseudo_rand_range(rnd, range) __BN_pseudo_rand_range(AmiSSLBase, (rnd), (range))

int __BN_num_bits(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a)="\tjsr\t-2184(a6)";
#define BN_num_bits(a) __BN_num_bits(AmiSSLBase, (a))

int __BN_num_bits_word(__reg("a6") struct Library *, __reg("d0") BN_ULONG a)="\tjsr\t-2190(a6)";
#define BN_num_bits_word(a) __BN_num_bits_word(AmiSSLBase, (a))

BIGNUM * __BN_new(__reg("a6") struct Library *)="\tjsr\t-2196(a6)";
#define BN_new() __BN_new(AmiSSLBase)

void __BN_init(__reg("a6") struct Library *, __reg("a0") BIGNUM * a)="\tjsr\t-2202(a6)";
#define BN_init(a) __BN_init(AmiSSLBase, (a))

void __BN_clear_free(__reg("a6") struct Library *, __reg("a0") BIGNUM * a)="\tjsr\t-2208(a6)";
#define BN_clear_free(a) __BN_clear_free(AmiSSLBase, (a))

BIGNUM * __BN_copy(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("a1") const BIGNUM * b)="\tjsr\t-2214(a6)";
#define BN_copy(a, b) __BN_copy(AmiSSLBase, (a), (b))

void __BN_swap(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("a1") BIGNUM * b)="\tjsr\t-2220(a6)";
#define BN_swap(a, b) __BN_swap(AmiSSLBase, (a), (b))

BIGNUM * __BN_bin2bn(__reg("a6") struct Library *, __reg("a0") const unsigned char * s, __reg("d0") LONG len, __reg("a1") BIGNUM * ret)="\tjsr\t-2226(a6)";
#define BN_bin2bn(s, len, ret) __BN_bin2bn(AmiSSLBase, (s), (len), (ret))

int __BN_bn2bin(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("a1") unsigned char * to)="\tjsr\t-2232(a6)";
#define BN_bn2bin(a, to) __BN_bn2bin(AmiSSLBase, (a), (to))

BIGNUM * __BN_mpi2bn(__reg("a6") struct Library *, __reg("a0") const unsigned char * s, __reg("d0") LONG len, __reg("a1") BIGNUM * ret)="\tjsr\t-2238(a6)";
#define BN_mpi2bn(s, len, ret) __BN_mpi2bn(AmiSSLBase, (s), (len), (ret))

int __BN_bn2mpi(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("a1") unsigned char * to)="\tjsr\t-2244(a6)";
#define BN_bn2mpi(a, to) __BN_bn2mpi(AmiSSLBase, (a), (to))

int __BN_sub(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b)="\tjsr\t-2250(a6)";
#define BN_sub(r, a, b) __BN_sub(AmiSSLBase, (r), (a), (b))

int __BN_usub(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b)="\tjsr\t-2256(a6)";
#define BN_usub(r, a, b) __BN_usub(AmiSSLBase, (r), (a), (b))

int __BN_uadd(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b)="\tjsr\t-2262(a6)";
#define BN_uadd(r, a, b) __BN_uadd(AmiSSLBase, (r), (a), (b))

int __BN_add(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b)="\tjsr\t-2268(a6)";
#define BN_add(r, a, b) __BN_add(AmiSSLBase, (r), (a), (b))

int __BN_mul(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") BN_CTX * ctx)="\tjsr\t-2274(a6)";
#define BN_mul(r, a, b, ctx) __BN_mul(AmiSSLBase, (r), (a), (b), (ctx))

int __BN_sqr(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") BN_CTX * ctx)="\tjsr\t-2280(a6)";
#define BN_sqr(r, a, ctx) __BN_sqr(AmiSSLBase, (r), (a), (ctx))

int __BN_div(__reg("a6") struct Library *, __reg("a0") BIGNUM * dv, __reg("a1") BIGNUM * rem, __reg("a2") const BIGNUM * m, __reg("a3") const BIGNUM * d, __reg("d0") BN_CTX * ctx)="\tjsr\t-2286(a6)";
#define BN_div(dv, rem, m, d, ctx) __BN_div(AmiSSLBase, (dv), (rem), (m), (d), (ctx))

int __BN_nnmod(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * m, __reg("a2") const BIGNUM * d, __reg("a3") BN_CTX * ctx)="\tjsr\t-2292(a6)";
#define BN_nnmod(r, m, d, ctx) __BN_nnmod(AmiSSLBase, (r), (m), (d), (ctx))

int __BN_mod_add(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2298(a6)";
#define BN_mod_add(r, a, b, m, ctx) __BN_mod_add(AmiSSLBase, (r), (a), (b), (m), (ctx))

int __BN_mod_add_quick(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") const BIGNUM * m)="\tjsr\t-2304(a6)";
#define BN_mod_add_quick(r, a, b, m) __BN_mod_add_quick(AmiSSLBase, (r), (a), (b), (m))

int __BN_mod_sub(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2310(a6)";
#define BN_mod_sub(r, a, b, m, ctx) __BN_mod_sub(AmiSSLBase, (r), (a), (b), (m), (ctx))

int __BN_mod_sub_quick(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") const BIGNUM * m)="\tjsr\t-2316(a6)";
#define BN_mod_sub_quick(r, a, b, m) __BN_mod_sub_quick(AmiSSLBase, (r), (a), (b), (m))

int __BN_mod_mul(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2322(a6)";
#define BN_mod_mul(r, a, b, m, ctx) __BN_mod_mul(AmiSSLBase, (r), (a), (b), (m), (ctx))

int __BN_mod_sqr(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * m, __reg("a3") BN_CTX * ctx)="\tjsr\t-2328(a6)";
#define BN_mod_sqr(r, a, m, ctx) __BN_mod_sqr(AmiSSLBase, (r), (a), (m), (ctx))

int __BN_mod_lshift1(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * m, __reg("a3") BN_CTX * ctx)="\tjsr\t-2334(a6)";
#define BN_mod_lshift1(r, a, m, ctx) __BN_mod_lshift1(AmiSSLBase, (r), (a), (m), (ctx))

int __BN_mod_lshift1_quick(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * m)="\tjsr\t-2340(a6)";
#define BN_mod_lshift1_quick(r, a, m) __BN_mod_lshift1_quick(AmiSSLBase, (r), (a), (m))

int __BN_mod_lshift(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("d0") LONG n, __reg("a2") const BIGNUM * m, __reg("a3") BN_CTX * ctx)="\tjsr\t-2346(a6)";
#define BN_mod_lshift(r, a, n, m, ctx) __BN_mod_lshift(AmiSSLBase, (r), (a), (n), (m), (ctx))

int __BN_mod_lshift_quick(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("d0") LONG n, __reg("a2") const BIGNUM * m)="\tjsr\t-2352(a6)";
#define BN_mod_lshift_quick(r, a, n, m) __BN_mod_lshift_quick(AmiSSLBase, (r), (a), (n), (m))

BN_ULONG __BN_mod_word(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2358(a6)";
#define BN_mod_word(a, w) __BN_mod_word(AmiSSLBase, (a), (w))

BN_ULONG __BN_div_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2364(a6)";
#define BN_div_word(a, w) __BN_div_word(AmiSSLBase, (a), (w))

int __BN_mul_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2370(a6)";
#define BN_mul_word(a, w) __BN_mul_word(AmiSSLBase, (a), (w))

int __BN_add_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2376(a6)";
#define BN_add_word(a, w) __BN_add_word(AmiSSLBase, (a), (w))

int __BN_sub_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2382(a6)";
#define BN_sub_word(a, w) __BN_sub_word(AmiSSLBase, (a), (w))

int __BN_set_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") BN_ULONG w)="\tjsr\t-2388(a6)";
#define BN_set_word(a, w) __BN_set_word(AmiSSLBase, (a), (w))

BN_ULONG __BN_get_word(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a)="\tjsr\t-2394(a6)";
#define BN_get_word(a) __BN_get_word(AmiSSLBase, (a))

int __BN_cmp(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("a1") const BIGNUM * b)="\tjsr\t-2400(a6)";
#define BN_cmp(a, b) __BN_cmp(AmiSSLBase, (a), (b))

void __BN_free(__reg("a6") struct Library *, __reg("a0") BIGNUM * a)="\tjsr\t-2406(a6)";
#define BN_free(a) __BN_free(AmiSSLBase, (a))

int __BN_is_bit_set(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2412(a6)";
#define BN_is_bit_set(a, n) __BN_is_bit_set(AmiSSLBase, (a), (n))

int __BN_lshift(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2418(a6)";
#define BN_lshift(r, a, n) __BN_lshift(AmiSSLBase, (r), (a), (n))

int __BN_lshift1(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a)="\tjsr\t-2424(a6)";
#define BN_lshift1(r, a) __BN_lshift1(AmiSSLBase, (r), (a))

int __BN_exp(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * p, __reg("a3") BN_CTX * ctx)="\tjsr\t-2430(a6)";
#define BN_exp(r, a, p, ctx) __BN_exp(AmiSSLBase, (r), (a), (p), (ctx))

int __BN_mod_exp(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * p, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2436(a6)";
#define BN_mod_exp(r, a, p, m, ctx) __BN_mod_exp(AmiSSLBase, (r), (a), (p), (m), (ctx))

int __BN_mod_exp_mont(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * p, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx, __reg("d1") BN_MONT_CTX * m_ctx)="\tjsr\t-2442(a6)";
#define BN_mod_exp_mont(r, a, p, m, ctx, m_ctx) __BN_mod_exp_mont(AmiSSLBase, (r), (a), (p), (m), (ctx), (m_ctx))

int __BN_mod_exp_mont_word(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("d0") BN_ULONG a, __reg("a1") const BIGNUM * p, __reg("a2") const BIGNUM * m, __reg("a3") BN_CTX * ctx, __reg("d1") BN_MONT_CTX * m_ctx)="\tjsr\t-2448(a6)";
#define BN_mod_exp_mont_word(r, a, p, m, ctx, m_ctx) __BN_mod_exp_mont_word(AmiSSLBase, (r), (a), (p), (m), (ctx), (m_ctx))

int __BN_mod_exp2_mont(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a1, __reg("a2") const BIGNUM * p1, __reg("a3") const BIGNUM * a2, __reg("d0") const BIGNUM * p2, __reg("d1") const BIGNUM * m, __reg("d2") BN_CTX * ctx, __reg("d3") BN_MONT_CTX * m_ctx)="\tjsr\t-2454(a6)";
#define BN_mod_exp2_mont(r, a1, p1, a2, p2, m, ctx, m_ctx) __BN_mod_exp2_mont(AmiSSLBase, (r), (a1), (p1), (a2), (p2), (m), (ctx), (m_ctx))

int __BN_mod_exp_simple(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * p, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2460(a6)";
#define BN_mod_exp_simple(r, a, p, m, ctx) __BN_mod_exp_simple(AmiSSLBase, (r), (a), (p), (m), (ctx))

int __BN_mask_bits(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2466(a6)";
#define BN_mask_bits(a, n) __BN_mask_bits(AmiSSLBase, (a), (n))

int __BN_print(__reg("a6") struct Library *, __reg("a0") BIO * fp, __reg("a1") const BIGNUM * a)="\tjsr\t-2472(a6)";
#define BN_print(fp, a) __BN_print(AmiSSLBase, (fp), (a))

int __BN_reciprocal(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * m, __reg("d0") LONG len, __reg("a2") BN_CTX * ctx)="\tjsr\t-2478(a6)";
#define BN_reciprocal(r, m, len, ctx) __BN_reciprocal(AmiSSLBase, (r), (m), (len), (ctx))

int __BN_rshift(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2484(a6)";
#define BN_rshift(r, a, n) __BN_rshift(AmiSSLBase, (r), (a), (n))

int __BN_rshift1(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a)="\tjsr\t-2490(a6)";
#define BN_rshift1(r, a) __BN_rshift1(AmiSSLBase, (r), (a))

void __BN_clear(__reg("a6") struct Library *, __reg("a0") BIGNUM * a)="\tjsr\t-2496(a6)";
#define BN_clear(a) __BN_clear(AmiSSLBase, (a))

BIGNUM * __BN_dup(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a)="\tjsr\t-2502(a6)";
#define BN_dup(a) __BN_dup(AmiSSLBase, (a))

int __BN_ucmp(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("a1") const BIGNUM * b)="\tjsr\t-2508(a6)";
#define BN_ucmp(a, b) __BN_ucmp(AmiSSLBase, (a), (b))

int __BN_set_bit(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2514(a6)";
#define BN_set_bit(a, n) __BN_set_bit(AmiSSLBase, (a), (n))

int __BN_clear_bit(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") LONG n)="\tjsr\t-2520(a6)";
#define BN_clear_bit(a, n) __BN_clear_bit(AmiSSLBase, (a), (n))

char * __BN_bn2hex(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a)="\tjsr\t-2526(a6)";
#define BN_bn2hex(a) __BN_bn2hex(AmiSSLBase, (a))

char * __BN_bn2dec(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a)="\tjsr\t-2532(a6)";
#define BN_bn2dec(a) __BN_bn2dec(AmiSSLBase, (a))

int __BN_hex2bn(__reg("a6") struct Library *, __reg("a0") BIGNUM ** a, __reg("a1") const char * str)="\tjsr\t-2538(a6)";
#define BN_hex2bn(a, str) __BN_hex2bn(AmiSSLBase, (a), (str))

int __BN_dec2bn(__reg("a6") struct Library *, __reg("a0") BIGNUM ** a, __reg("a1") const char * str)="\tjsr\t-2544(a6)";
#define BN_dec2bn(a, str) __BN_dec2bn(AmiSSLBase, (a), (str))

int __BN_gcd(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") BN_CTX * ctx)="\tjsr\t-2550(a6)";
#define BN_gcd(r, a, b, ctx) __BN_gcd(AmiSSLBase, (r), (a), (b), (ctx))

int __BN_kronecker(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("a1") const BIGNUM * b, __reg("a2") BN_CTX * ctx)="\tjsr\t-2556(a6)";
#define BN_kronecker(a, b, ctx) __BN_kronecker(AmiSSLBase, (a), (b), (ctx))

BIGNUM * __BN_mod_inverse(__reg("a6") struct Library *, __reg("a0") BIGNUM * ret, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * n, __reg("a3") BN_CTX * ctx)="\tjsr\t-2562(a6)";
#define BN_mod_inverse(ret, a, n, ctx) __BN_mod_inverse(AmiSSLBase, (ret), (a), (n), (ctx))

BIGNUM * __BN_mod_sqrt(__reg("a6") struct Library *, __reg("a0") BIGNUM * ret, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * n, __reg("a3") BN_CTX * ctx)="\tjsr\t-2568(a6)";
#define BN_mod_sqrt(ret, a, n, ctx) __BN_mod_sqrt(AmiSSLBase, (ret), (a), (n), (ctx))

BIGNUM * __BN_generate_prime(__reg("a6") struct Library *, __reg("a0") BIGNUM * ret, __reg("d0") LONG bits, __reg("d1") LONG safe, __reg("a1") const BIGNUM * add, __reg("a2") const BIGNUM * rem, __reg("a3") void (*callback)(int, int, void *), __reg("d2") void * cb_arg)="\tjsr\t-2574(a6)";
#define BN_generate_prime(ret, bits, safe, add, rem, callback, cb_arg) __BN_generate_prime(AmiSSLBase, (ret), (bits), (safe), (add), (rem), (callback), (cb_arg))

int __BN_is_prime(__reg("a6") struct Library *, __reg("a0") const BIGNUM * p, __reg("d0") LONG nchecks, __reg("a1") void (*callback)(int, int, void *), __reg("a2") BN_CTX * ctx, __reg("a3") void * cb_arg)="\tjsr\t-2580(a6)";
#define BN_is_prime(p, nchecks, callback, ctx, cb_arg) __BN_is_prime(AmiSSLBase, (p), (nchecks), (callback), (ctx), (cb_arg))

int __BN_is_prime_fasttest(__reg("a6") struct Library *, __reg("a0") const BIGNUM * p, __reg("d0") LONG nchecks, __reg("a1") void (*callback)(int, int, void *), __reg("a2") BN_CTX * ctx, __reg("a3") void * cb_arg, __reg("d1") LONG do_trial_division)="\tjsr\t-2586(a6)";
#define BN_is_prime_fasttest(p, nchecks, callback, ctx, cb_arg, do_trial_division) __BN_is_prime_fasttest(AmiSSLBase, (p), (nchecks), (callback), (ctx), (cb_arg), (do_trial_division))

BN_MONT_CTX * __BN_MONT_CTX_new(__reg("a6") struct Library *)="\tjsr\t-2592(a6)";
#define BN_MONT_CTX_new() __BN_MONT_CTX_new(AmiSSLBase)

void __BN_MONT_CTX_init(__reg("a6") struct Library *, __reg("a0") BN_MONT_CTX * ctx)="\tjsr\t-2598(a6)";
#define BN_MONT_CTX_init(ctx) __BN_MONT_CTX_init(AmiSSLBase, (ctx))

int __BN_mod_mul_montgomery(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") BN_MONT_CTX * mont, __reg("d0") BN_CTX * ctx)="\tjsr\t-2604(a6)";
#define BN_mod_mul_montgomery(r, a, b, mont, ctx) __BN_mod_mul_montgomery(AmiSSLBase, (r), (a), (b), (mont), (ctx))

int __BN_from_montgomery(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") BN_MONT_CTX * mont, __reg("a3") BN_CTX * ctx)="\tjsr\t-2610(a6)";
#define BN_from_montgomery(r, a, mont, ctx) __BN_from_montgomery(AmiSSLBase, (r), (a), (mont), (ctx))

void __BN_MONT_CTX_free(__reg("a6") struct Library *, __reg("a0") BN_MONT_CTX * mont)="\tjsr\t-2616(a6)";
#define BN_MONT_CTX_free(mont) __BN_MONT_CTX_free(AmiSSLBase, (mont))

int __BN_MONT_CTX_set(__reg("a6") struct Library *, __reg("a0") BN_MONT_CTX * mont, __reg("a1") const BIGNUM * mod, __reg("a2") BN_CTX * ctx)="\tjsr\t-2622(a6)";
#define BN_MONT_CTX_set(mont, mod, ctx) __BN_MONT_CTX_set(AmiSSLBase, (mont), (mod), (ctx))

BN_MONT_CTX * __BN_MONT_CTX_copy(__reg("a6") struct Library *, __reg("a0") BN_MONT_CTX * to, __reg("a1") BN_MONT_CTX * from)="\tjsr\t-2628(a6)";
#define BN_MONT_CTX_copy(to, from) __BN_MONT_CTX_copy(AmiSSLBase, (to), (from))

BN_BLINDING * __BN_BLINDING_new(__reg("a6") struct Library *, __reg("a0") BIGNUM * A, __reg("a1") BIGNUM * Ai, __reg("a2") BIGNUM * mod)="\tjsr\t-2634(a6)";
#define BN_BLINDING_new(A, Ai, mod) __BN_BLINDING_new(AmiSSLBase, (A), (Ai), (mod))

void __BN_BLINDING_free(__reg("a6") struct Library *, __reg("a0") BN_BLINDING * b)="\tjsr\t-2640(a6)";
#define BN_BLINDING_free(b) __BN_BLINDING_free(AmiSSLBase, (b))

int __BN_BLINDING_update(__reg("a6") struct Library *, __reg("a0") BN_BLINDING * b, __reg("a1") BN_CTX * ctx)="\tjsr\t-2646(a6)";
#define BN_BLINDING_update(b, ctx) __BN_BLINDING_update(AmiSSLBase, (b), (ctx))

int __BN_BLINDING_convert(__reg("a6") struct Library *, __reg("a0") BIGNUM * n, __reg("a1") BN_BLINDING * r, __reg("a2") BN_CTX * ctx)="\tjsr\t-2652(a6)";
#define BN_BLINDING_convert(n, r, ctx) __BN_BLINDING_convert(AmiSSLBase, (n), (r), (ctx))

int __BN_BLINDING_invert(__reg("a6") struct Library *, __reg("a0") BIGNUM * n, __reg("a1") BN_BLINDING * b, __reg("a2") BN_CTX * ctx)="\tjsr\t-2658(a6)";
#define BN_BLINDING_invert(n, b, ctx) __BN_BLINDING_invert(AmiSSLBase, (n), (b), (ctx))

void __BN_set_params(__reg("a6") struct Library *, __reg("d0") LONG mul, __reg("d1") LONG high, __reg("d2") LONG low, __reg("d3") LONG mont)="\tjsr\t-2664(a6)";
#define BN_set_params(mul, high, low, mont) __BN_set_params(AmiSSLBase, (mul), (high), (low), (mont))

int __BN_get_params(__reg("a6") struct Library *, __reg("d0") LONG which)="\tjsr\t-2670(a6)";
#define BN_get_params(which) __BN_get_params(AmiSSLBase, (which))

void __BN_RECP_CTX_init(__reg("a6") struct Library *, __reg("a0") BN_RECP_CTX * recp)="\tjsr\t-2676(a6)";
#define BN_RECP_CTX_init(recp) __BN_RECP_CTX_init(AmiSSLBase, (recp))

BN_RECP_CTX * __BN_RECP_CTX_new(__reg("a6") struct Library *)="\tjsr\t-2682(a6)";
#define BN_RECP_CTX_new() __BN_RECP_CTX_new(AmiSSLBase)

void __BN_RECP_CTX_free(__reg("a6") struct Library *, __reg("a0") BN_RECP_CTX * recp)="\tjsr\t-2688(a6)";
#define BN_RECP_CTX_free(recp) __BN_RECP_CTX_free(AmiSSLBase, (recp))

int __BN_RECP_CTX_set(__reg("a6") struct Library *, __reg("a0") BN_RECP_CTX * recp, __reg("a1") const BIGNUM * rdiv, __reg("a2") BN_CTX * ctx)="\tjsr\t-2694(a6)";
#define BN_RECP_CTX_set(recp, rdiv, ctx) __BN_RECP_CTX_set(AmiSSLBase, (recp), (rdiv), (ctx))

int __BN_mod_mul_reciprocal(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * x, __reg("a2") const BIGNUM * y, __reg("a3") BN_RECP_CTX * recp, __reg("d0") BN_CTX * ctx)="\tjsr\t-2700(a6)";
#define BN_mod_mul_reciprocal(r, x, y, recp, ctx) __BN_mod_mul_reciprocal(AmiSSLBase, (r), (x), (y), (recp), (ctx))

int __BN_mod_exp_recp(__reg("a6") struct Library *, __reg("a0") BIGNUM * r, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * p, __reg("a3") const BIGNUM * m, __reg("d0") BN_CTX * ctx)="\tjsr\t-2706(a6)";
#define BN_mod_exp_recp(r, a, p, m, ctx) __BN_mod_exp_recp(AmiSSLBase, (r), (a), (p), (m), (ctx))

int __BN_div_recp(__reg("a6") struct Library *, __reg("a0") BIGNUM * dv, __reg("a1") BIGNUM * rem, __reg("a2") const BIGNUM * m, __reg("a3") BN_RECP_CTX * recp, __reg("d0") BN_CTX * ctx)="\tjsr\t-2712(a6)";
#define BN_div_recp(dv, rem, m, recp, ctx) __BN_div_recp(AmiSSLBase, (dv), (rem), (m), (recp), (ctx))

BIGNUM * __bn_expand2(__reg("a6") struct Library *, __reg("a0") BIGNUM * a, __reg("d0") LONG words)="\tjsr\t-2718(a6)";
#define bn_expand2(a, words) __bn_expand2(AmiSSLBase, (a), (words))

BIGNUM * __bn_dup_expand(__reg("a6") struct Library *, __reg("a0") const BIGNUM * a, __reg("d0") LONG words)="\tjsr\t-2724(a6)";
#define bn_dup_expand(a, words) __bn_dup_expand(AmiSSLBase, (a), (words))

BN_ULONG __bn_mul_add_words(__reg("a6") struct Library *, __reg("a0") BN_ULONG * rp, __reg("a1") const BN_ULONG * ap, __reg("d0") LONG num, __reg("d1") BN_ULONG w)="\tjsr\t-2730(a6)";
#define bn_mul_add_words(rp, ap, num, w) __bn_mul_add_words(AmiSSLBase, (rp), (ap), (num), (w))

BN_ULONG __bn_mul_words(__reg("a6") struct Library *, __reg("a0") BN_ULONG * rp, __reg("a1") const BN_ULONG * ap, __reg("d0") LONG num, __reg("d1") BN_ULONG w)="\tjsr\t-2736(a6)";
#define bn_mul_words(rp, ap, num, w) __bn_mul_words(AmiSSLBase, (rp), (ap), (num), (w))

void __bn_sqr_words(__reg("a6") struct Library *, __reg("a0") BN_ULONG * rp, __reg("a1") const BN_ULONG * ap, __reg("d0") LONG num)="\tjsr\t-2742(a6)";
#define bn_sqr_words(rp, ap, num) __bn_sqr_words(AmiSSLBase, (rp), (ap), (num))

BN_ULONG __bn_div_words(__reg("a6") struct Library *, __reg("d0") BN_ULONG h, __reg("d1") BN_ULONG l, __reg("d2") BN_ULONG d)="\tjsr\t-2748(a6)";
#define bn_div_words(h, l, d) __bn_div_words(AmiSSLBase, (h), (l), (d))

BN_ULONG __bn_add_words(__reg("a6") struct Library *, __reg("a0") BN_ULONG * rp, __reg("a1") const BN_ULONG * ap, __reg("a2") const BN_ULONG * bp, __reg("d0") LONG num)="\tjsr\t-2754(a6)";
#define bn_add_words(rp, ap, bp, num) __bn_add_words(AmiSSLBase, (rp), (ap), (bp), (num))

BN_ULONG __bn_sub_words(__reg("a6") struct Library *, __reg("a0") BN_ULONG * rp, __reg("a1") const BN_ULONG * ap, __reg("a2") const BN_ULONG * bp, __reg("d0") LONG num)="\tjsr\t-2760(a6)";
#define bn_sub_words(rp, ap, bp, num) __bn_sub_words(AmiSSLBase, (rp), (ap), (bp), (num))

int __BN_bntest_rand(__reg("a6") struct Library *, __reg("a0") BIGNUM * rnd, __reg("d0") LONG bits, __reg("d1") LONG top, __reg("d2") LONG bottom)="\tjsr\t-2766(a6)";
#define BN_bntest_rand(rnd, bits, top, bottom) __BN_bntest_rand(AmiSSLBase, (rnd), (bits), (top), (bottom))

void __ERR_load_BN_strings(__reg("a6") struct Library *)="\tjsr\t-2772(a6)";
#define ERR_load_BN_strings() __ERR_load_BN_strings(AmiSSLBase)

BUF_MEM * __BUF_MEM_new(__reg("a6") struct Library *)="\tjsr\t-2778(a6)";
#define BUF_MEM_new() __BUF_MEM_new(AmiSSLBase)

void __BUF_MEM_free(__reg("a6") struct Library *, __reg("a0") BUF_MEM * a)="\tjsr\t-2784(a6)";
#define BUF_MEM_free(a) __BUF_MEM_free(AmiSSLBase, (a))

int __BUF_MEM_grow(__reg("a6") struct Library *, __reg("a0") BUF_MEM * str, __reg("d0") LONG len)="\tjsr\t-2790(a6)";
#define BUF_MEM_grow(str, len) __BUF_MEM_grow(AmiSSLBase, (str), (len))

int __BUF_MEM_grow_clean(__reg("a6") struct Library *, __reg("a0") BUF_MEM * str, __reg("d0") LONG len)="\tjsr\t-2796(a6)";
#define BUF_MEM_grow_clean(str, len) __BUF_MEM_grow_clean(AmiSSLBase, (str), (len))

char * __BUF_strdup(__reg("a6") struct Library *, __reg("a0") const char * str)="\tjsr\t-2802(a6)";
#define BUF_strdup(str) __BUF_strdup(AmiSSLBase, (str))

size_t __BUF_strlcpy(__reg("a6") struct Library *, __reg("a0") char * dst, __reg("a1") const char * src, __reg("d0") ULONG siz)="\tjsr\t-2808(a6)";
#define BUF_strlcpy(dst, src, siz) __BUF_strlcpy(AmiSSLBase, (dst), (src), (siz))

size_t __BUF_strlcat(__reg("a6") struct Library *, __reg("a0") char * dst, __reg("a1") const char * src, __reg("d0") ULONG siz)="\tjsr\t-2814(a6)";
#define BUF_strlcat(dst, src, siz) __BUF_strlcat(AmiSSLBase, (dst), (src), (siz))

void __ERR_load_BUF_strings(__reg("a6") struct Library *)="\tjsr\t-2820(a6)";
#define ERR_load_BUF_strings() __ERR_load_BUF_strings(AmiSSLBase)

COMP_CTX * __COMP_CTX_new(__reg("a6") struct Library *, __reg("a0") COMP_METHOD * meth)="\tjsr\t-2826(a6)";
#define COMP_CTX_new(meth) __COMP_CTX_new(AmiSSLBase, (meth))

void __COMP_CTX_free(__reg("a6") struct Library *, __reg("a0") COMP_CTX * ctx)="\tjsr\t-2832(a6)";
#define COMP_CTX_free(ctx) __COMP_CTX_free(AmiSSLBase, (ctx))

int __COMP_compress_block(__reg("a6") struct Library *, __reg("a0") COMP_CTX * ctx, __reg("a1") unsigned char * out, __reg("d0") LONG olen, __reg("a2") unsigned char * in, __reg("d1") LONG ilen)="\tjsr\t-2838(a6)";
#define COMP_compress_block(ctx, out, olen, in, ilen) __COMP_compress_block(AmiSSLBase, (ctx), (out), (olen), (in), (ilen))

int __COMP_expand_block(__reg("a6") struct Library *, __reg("a0") COMP_CTX * ctx, __reg("a1") unsigned char * out, __reg("d0") LONG olen, __reg("a2") unsigned char * in, __reg("d1") LONG ilen)="\tjsr\t-2844(a6)";
#define COMP_expand_block(ctx, out, olen, in, ilen) __COMP_expand_block(AmiSSLBase, (ctx), (out), (olen), (in), (ilen))

COMP_METHOD * __COMP_rle(__reg("a6") struct Library *)="\tjsr\t-2850(a6)";
#define COMP_rle() __COMP_rle(AmiSSLBase)

COMP_METHOD * __COMP_zlib(__reg("a6") struct Library *)="\tjsr\t-2856(a6)";
#define COMP_zlib() __COMP_zlib(AmiSSLBase)

void __ERR_load_COMP_strings(__reg("a6") struct Library *)="\tjsr\t-2862(a6)";
#define ERR_load_COMP_strings() __ERR_load_COMP_strings(AmiSSLBase)

int __CONF_set_default_method(__reg("a6") struct Library *, __reg("a0") CONF_METHOD * meth)="\tjsr\t-2868(a6)";
#define CONF_set_default_method(meth) __CONF_set_default_method(AmiSSLBase, (meth))

void __CONF_set_nconf(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") LHASH * hash)="\tjsr\t-2874(a6)";
#define CONF_set_nconf(conf, hash) __CONF_set_nconf(AmiSSLBase, (conf), (hash))

LHASH * __CONF_load(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") const char * file, __reg("a2") long * eline)="\tjsr\t-2880(a6)";
#define CONF_load(conf, file, eline) __CONF_load(AmiSSLBase, (conf), (file), (eline))

LHASH * __CONF_load_bio(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") BIO * bp, __reg("a2") long * eline)="\tjsr\t-2886(a6)";
#define CONF_load_bio(conf, bp, eline) __CONF_load_bio(AmiSSLBase, (conf), (bp), (eline))

void * __CONF_get_section(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") const char * section)="\tjsr\t-2892(a6)";
#define CONF_get_section(conf, section) __CONF_get_section(AmiSSLBase, (conf), (section))

char * __CONF_get_string(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") const char * group, __reg("a2") const char * name)="\tjsr\t-2898(a6)";
#define CONF_get_string(conf, group, name) __CONF_get_string(AmiSSLBase, (conf), (group), (name))

long __CONF_get_number(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") const char * group, __reg("a2") const char * name)="\tjsr\t-2904(a6)";
#define CONF_get_number(conf, group, name) __CONF_get_number(AmiSSLBase, (conf), (group), (name))

void __CONF_free(__reg("a6") struct Library *, __reg("a0") LHASH * conf)="\tjsr\t-2910(a6)";
#define CONF_free(conf) __CONF_free(AmiSSLBase, (conf))

int __CONF_dump_bio(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") BIO * out)="\tjsr\t-2916(a6)";
#define CONF_dump_bio(conf, out) __CONF_dump_bio(AmiSSLBase, (conf), (out))

void __OPENSSL_config(__reg("a6") struct Library *, __reg("a0") const char * config_name)="\tjsr\t-2922(a6)";
#define OPENSSL_config(config_name) __OPENSSL_config(AmiSSLBase, (config_name))

void __OPENSSL_no_config(__reg("a6") struct Library *)="\tjsr\t-2928(a6)";
#define OPENSSL_no_config() __OPENSSL_no_config(AmiSSLBase)

CONF * __NCONF_new(__reg("a6") struct Library *, __reg("a0") CONF_METHOD * meth)="\tjsr\t-2934(a6)";
#define NCONF_new(meth) __NCONF_new(AmiSSLBase, (meth))

CONF_METHOD * __NCONF_default(__reg("a6") struct Library *)="\tjsr\t-2940(a6)";
#define NCONF_default() __NCONF_default(AmiSSLBase)

CONF_METHOD * __NCONF_WIN32(__reg("a6") struct Library *)="\tjsr\t-2946(a6)";
#define NCONF_WIN32() __NCONF_WIN32(AmiSSLBase)

void __NCONF_free(__reg("a6") struct Library *, __reg("a0") CONF * conf)="\tjsr\t-2952(a6)";
#define NCONF_free(conf) __NCONF_free(AmiSSLBase, (conf))

void __NCONF_free_data(__reg("a6") struct Library *, __reg("a0") CONF * conf)="\tjsr\t-2958(a6)";
#define NCONF_free_data(conf) __NCONF_free_data(AmiSSLBase, (conf))

int __NCONF_load(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") const char * file, __reg("a2") long * eline)="\tjsr\t-2964(a6)";
#define NCONF_load(conf, file, eline) __NCONF_load(AmiSSLBase, (conf), (file), (eline))

int __NCONF_load_bio(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") BIO * bp, __reg("a2") long * eline)="\tjsr\t-2970(a6)";
#define NCONF_load_bio(conf, bp, eline) __NCONF_load_bio(AmiSSLBase, (conf), (bp), (eline))

void * __NCONF_get_section(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * section)="\tjsr\t-2976(a6)";
#define NCONF_get_section(conf, section) __NCONF_get_section(AmiSSLBase, (conf), (section))

char * __NCONF_get_string(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * group, __reg("a2") const char * name)="\tjsr\t-2982(a6)";
#define NCONF_get_string(conf, group, name) __NCONF_get_string(AmiSSLBase, (conf), (group), (name))

int __NCONF_get_number_e(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * group, __reg("a2") const char * name, __reg("a3") long * result)="\tjsr\t-2988(a6)";
#define NCONF_get_number_e(conf, group, name, result) __NCONF_get_number_e(AmiSSLBase, (conf), (group), (name), (result))

int __NCONF_dump_bio(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") BIO * out)="\tjsr\t-2994(a6)";
#define NCONF_dump_bio(conf, out) __NCONF_dump_bio(AmiSSLBase, (conf), (out))

int __CONF_modules_load(__reg("a6") struct Library *, __reg("a0") const CONF * cnf, __reg("a1") const char * appname, __reg("d0") unsigned long flags)="\tjsr\t-3000(a6)";
#define CONF_modules_load(cnf, appname, flags) __CONF_modules_load(AmiSSLBase, (cnf), (appname), (flags))

int __CONF_modules_load_file(__reg("a6") struct Library *, __reg("a0") const char * filename, __reg("a1") const char * appname, __reg("d0") unsigned long flags)="\tjsr\t-3006(a6)";
#define CONF_modules_load_file(filename, appname, flags) __CONF_modules_load_file(AmiSSLBase, (filename), (appname), (flags))

void __CONF_modules_unload(__reg("a6") struct Library *, __reg("d0") LONG all)="\tjsr\t-3012(a6)";
#define CONF_modules_unload(all) __CONF_modules_unload(AmiSSLBase, (all))

void __CONF_modules_finish(__reg("a6") struct Library *)="\tjsr\t-3018(a6)";
#define CONF_modules_finish() __CONF_modules_finish(AmiSSLBase)

void __CONF_modules_free(__reg("a6") struct Library *)="\tjsr\t-3024(a6)";
#define CONF_modules_free() __CONF_modules_free(AmiSSLBase)

int __CONF_module_add(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("a1") conf_init_func * (*ifunc)(), __reg("a2") conf_finish_func * (*ffunc)())="\tjsr\t-3030(a6)";
#define CONF_module_add(name, ifunc, ffunc) __CONF_module_add(AmiSSLBase, (name), (ifunc), (ffunc))

const char * __CONF_imodule_get_name(__reg("a6") struct Library *, __reg("a0") const CONF_IMODULE * md)="\tjsr\t-3036(a6)";
#define CONF_imodule_get_name(md) __CONF_imodule_get_name(AmiSSLBase, (md))

const char * __CONF_imodule_get_value(__reg("a6") struct Library *, __reg("a0") const CONF_IMODULE * md)="\tjsr\t-3042(a6)";
#define CONF_imodule_get_value(md) __CONF_imodule_get_value(AmiSSLBase, (md))

void * __CONF_imodule_get_usr_data(__reg("a6") struct Library *, __reg("a0") const CONF_IMODULE * md)="\tjsr\t-3048(a6)";
#define CONF_imodule_get_usr_data(md) __CONF_imodule_get_usr_data(AmiSSLBase, (md))

void __CONF_imodule_set_usr_data(__reg("a6") struct Library *, __reg("a0") CONF_IMODULE * md, __reg("a1") void * usr_data)="\tjsr\t-3054(a6)";
#define CONF_imodule_set_usr_data(md, usr_data) __CONF_imodule_set_usr_data(AmiSSLBase, (md), (usr_data))

CONF_MODULE * __CONF_imodule_get_module(__reg("a6") struct Library *, __reg("a0") const CONF_IMODULE * md)="\tjsr\t-3060(a6)";
#define CONF_imodule_get_module(md) __CONF_imodule_get_module(AmiSSLBase, (md))

unsigned long __CONF_imodule_get_flags(__reg("a6") struct Library *, __reg("a0") const CONF_IMODULE * md)="\tjsr\t-3066(a6)";
#define CONF_imodule_get_flags(md) __CONF_imodule_get_flags(AmiSSLBase, (md))

void __CONF_imodule_set_flags(__reg("a6") struct Library *, __reg("a0") CONF_IMODULE * md, __reg("d0") unsigned long flags)="\tjsr\t-3072(a6)";
#define CONF_imodule_set_flags(md, flags) __CONF_imodule_set_flags(AmiSSLBase, (md), (flags))

void * __CONF_module_get_usr_data(__reg("a6") struct Library *, __reg("a0") CONF_MODULE * pmod)="\tjsr\t-3078(a6)";
#define CONF_module_get_usr_data(pmod) __CONF_module_get_usr_data(AmiSSLBase, (pmod))

void __CONF_module_set_usr_data(__reg("a6") struct Library *, __reg("a0") CONF_MODULE * pmod, __reg("a1") void * usr_data)="\tjsr\t-3084(a6)";
#define CONF_module_set_usr_data(pmod, usr_data) __CONF_module_set_usr_data(AmiSSLBase, (pmod), (usr_data))

char * __CONF_get1_default_config_file(__reg("a6") struct Library *)="\tjsr\t-3090(a6)";
#define CONF_get1_default_config_file() __CONF_get1_default_config_file(AmiSSLBase)

int __CONF_parse_list(__reg("a6") struct Library *, __reg("a0") const char * list, __reg("d0") LONG sep, __reg("d1") LONG nospc, __reg("a1") int (*list_cb)(const char *elem, int len, void *usr), __reg("a2") void * arg)="\tjsr\t-3096(a6)";
#define CONF_parse_list(list, sep, nospc, list_cb, arg) __CONF_parse_list(AmiSSLBase, (list), (sep), (nospc), (list_cb), (arg))

void __OPENSSL_load_builtin_modules(__reg("a6") struct Library *)="\tjsr\t-3102(a6)";
#define OPENSSL_load_builtin_modules() __OPENSSL_load_builtin_modules(AmiSSLBase)

void __ERR_load_CONF_strings(__reg("a6") struct Library *)="\tjsr\t-3108(a6)";
#define ERR_load_CONF_strings() __ERR_load_CONF_strings(AmiSSLBase)

CONF_VALUE * ___CONF_new_section(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") const char * section)="\tjsr\t-3114(a6)";
#define _CONF_new_section(conf, section) ___CONF_new_section(AmiSSLBase, (conf), (section))

CONF_VALUE * ___CONF_get_section(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * section)="\tjsr\t-3120(a6)";
#define _CONF_get_section(conf, section) ___CONF_get_section(AmiSSLBase, (conf), (section))

void * ___CONF_get_section_values(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * section)="\tjsr\t-3126(a6)";
#define _CONF_get_section_values(conf, section) ___CONF_get_section_values(AmiSSLBase, (conf), (section))

int ___CONF_add_string(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") CONF_VALUE * section, __reg("a2") CONF_VALUE * value)="\tjsr\t-3132(a6)";
#define _CONF_add_string(conf, section, value) ___CONF_add_string(AmiSSLBase, (conf), (section), (value))

char * ___CONF_get_string(__reg("a6") struct Library *, __reg("a0") const CONF * conf, __reg("a1") const char * section, __reg("a2") const char * name)="\tjsr\t-3138(a6)";
#define _CONF_get_string(conf, section, name) ___CONF_get_string(AmiSSLBase, (conf), (section), (name))

int ___CONF_new_data(__reg("a6") struct Library *, __reg("a0") CONF * conf)="\tjsr\t-3144(a6)";
#define _CONF_new_data(conf) ___CONF_new_data(AmiSSLBase, (conf))

void ___CONF_free_data(__reg("a6") struct Library *, __reg("a0") CONF * conf)="\tjsr\t-3150(a6)";
#define _CONF_free_data(conf) ___CONF_free_data(AmiSSLBase, (conf))

int __CRYPTO_mem_ctrl(__reg("a6") struct Library *, __reg("d0") LONG mode)="\tjsr\t-3156(a6)";
#define CRYPTO_mem_ctrl(mode) __CRYPTO_mem_ctrl(AmiSSLBase, (mode))

int __CRYPTO_is_mem_check_on(__reg("a6") struct Library *)="\tjsr\t-3162(a6)";
#define CRYPTO_is_mem_check_on() __CRYPTO_is_mem_check_on(AmiSSLBase)

const char * __SSLeay_version(__reg("a6") struct Library *, __reg("d0") LONG type)="\tjsr\t-3168(a6)";
#define SSLeay_version(type) __SSLeay_version(AmiSSLBase, (type))

unsigned long __SSLeay(__reg("a6") struct Library *)="\tjsr\t-3174(a6)";
#define SSLeay() __SSLeay(AmiSSLBase)

int __OPENSSL_issetugid(__reg("a6") struct Library *)="\tjsr\t-3180(a6)";
#define OPENSSL_issetugid() __OPENSSL_issetugid(AmiSSLBase)

const CRYPTO_EX_DATA_IMPL * __CRYPTO_get_ex_data_implementation(__reg("a6") struct Library *)="\tjsr\t-3186(a6)";
#define CRYPTO_get_ex_data_implementation() __CRYPTO_get_ex_data_implementation(AmiSSLBase)

int __CRYPTO_set_ex_data_implementation(__reg("a6") struct Library *, __reg("a0") const CRYPTO_EX_DATA_IMPL * i)="\tjsr\t-3192(a6)";
#define CRYPTO_set_ex_data_implementation(i) __CRYPTO_set_ex_data_implementation(AmiSSLBase, (i))

int __CRYPTO_ex_data_new_class(__reg("a6") struct Library *)="\tjsr\t-3198(a6)";
#define CRYPTO_ex_data_new_class() __CRYPTO_ex_data_new_class(AmiSSLBase)

int __CRYPTO_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") LONG class_index, __reg("d1") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-3204(a6)";
#define CRYPTO_get_ex_new_index(class_index, argl, argp, new_func, dup_func, free_func) __CRYPTO_get_ex_new_index(AmiSSLBase, (class_index), (argl), (argp), (new_func), (dup_func), (free_func))

int __CRYPTO_new_ex_data(__reg("a6") struct Library *, __reg("d0") LONG class_index, __reg("a0") void * obj, __reg("a1") CRYPTO_EX_DATA * ad)="\tjsr\t-3210(a6)";
#define CRYPTO_new_ex_data(class_index, obj, ad) __CRYPTO_new_ex_data(AmiSSLBase, (class_index), (obj), (ad))

int __CRYPTO_dup_ex_data(__reg("a6") struct Library *, __reg("d0") LONG class_index, __reg("a0") CRYPTO_EX_DATA * to, __reg("a1") CRYPTO_EX_DATA * from)="\tjsr\t-3216(a6)";
#define CRYPTO_dup_ex_data(class_index, to, from) __CRYPTO_dup_ex_data(AmiSSLBase, (class_index), (to), (from))

void __CRYPTO_free_ex_data(__reg("a6") struct Library *, __reg("d0") LONG class_index, __reg("a0") void * obj, __reg("a1") CRYPTO_EX_DATA * ad)="\tjsr\t-3222(a6)";
#define CRYPTO_free_ex_data(class_index, obj, ad) __CRYPTO_free_ex_data(AmiSSLBase, (class_index), (obj), (ad))

int __CRYPTO_set_ex_data(__reg("a6") struct Library *, __reg("a0") CRYPTO_EX_DATA * ad, __reg("d0") LONG idx, __reg("a1") void * val)="\tjsr\t-3228(a6)";
#define CRYPTO_set_ex_data(ad, idx, val) __CRYPTO_set_ex_data(AmiSSLBase, (ad), (idx), (val))

void * __CRYPTO_get_ex_data(__reg("a6") struct Library *, __reg("a0") const CRYPTO_EX_DATA * ad, __reg("d0") LONG idx)="\tjsr\t-3234(a6)";
#define CRYPTO_get_ex_data(ad, idx) __CRYPTO_get_ex_data(AmiSSLBase, (ad), (idx))

void __CRYPTO_cleanup_all_ex_data(__reg("a6") struct Library *)="\tjsr\t-3240(a6)";
#define CRYPTO_cleanup_all_ex_data() __CRYPTO_cleanup_all_ex_data(AmiSSLBase)

int __CRYPTO_get_new_lockid(__reg("a6") struct Library *, __reg("a0") char * name)="\tjsr\t-3246(a6)";
#define CRYPTO_get_new_lockid(name) __CRYPTO_get_new_lockid(AmiSSLBase, (name))

int __CRYPTO_num_locks(__reg("a6") struct Library *)="\tjsr\t-3252(a6)";
#define CRYPTO_num_locks() __CRYPTO_num_locks(AmiSSLBase)

void __CRYPTO_lock(__reg("a6") struct Library *, __reg("d0") LONG mode, __reg("d1") LONG type, __reg("a0") const char * file, __reg("d2") LONG line)="\tjsr\t-3258(a6)";
#define CRYPTO_lock(mode, type, file, line) __CRYPTO_lock(AmiSSLBase, (mode), (type), (file), (line))

void __CRYPTO_set_locking_callback(__reg("a6") struct Library *, __reg("a0") void (*func)(int mode, int type, const char *file, int line))="\tjsr\t-3264(a6)";
#define CRYPTO_set_locking_callback(func) __CRYPTO_set_locking_callback(AmiSSLBase, (func))

void * __CRYPTO_get_locking_callback(__reg("a6") struct Library *)="\tjsr\t-3270(a6)";
#define CRYPTO_get_locking_callback() __CRYPTO_get_locking_callback(AmiSSLBase)

void __CRYPTO_set_add_lock_callback(__reg("a6") struct Library *, __reg("a0") int (*func)(int *num, int mount, int type, const char *file, int line))="\tjsr\t-3276(a6)";
#define CRYPTO_set_add_lock_callback(func) __CRYPTO_set_add_lock_callback(AmiSSLBase, (func))

void * __CRYPTO_get_add_lock_callback(__reg("a6") struct Library *)="\tjsr\t-3282(a6)";
#define CRYPTO_get_add_lock_callback() __CRYPTO_get_add_lock_callback(AmiSSLBase)

void __CRYPTO_set_id_callback(__reg("a6") struct Library *, __reg("a0") unsigned long (*func)(void))="\tjsr\t-3288(a6)";
#define CRYPTO_set_id_callback(func) __CRYPTO_set_id_callback(AmiSSLBase, (func))

void * __CRYPTO_get_id_callback(__reg("a6") struct Library *)="\tjsr\t-3294(a6)";
#define CRYPTO_get_id_callback() __CRYPTO_get_id_callback(AmiSSLBase)

unsigned long __CRYPTO_thread_id(__reg("a6") struct Library *)="\tjsr\t-3300(a6)";
#define CRYPTO_thread_id() __CRYPTO_thread_id(AmiSSLBase)

const char * __CRYPTO_get_lock_name(__reg("a6") struct Library *, __reg("d0") LONG type)="\tjsr\t-3306(a6)";
#define CRYPTO_get_lock_name(type) __CRYPTO_get_lock_name(AmiSSLBase, (type))

int __CRYPTO_add_lock(__reg("a6") struct Library *, __reg("a0") int * pointer, __reg("d0") LONG amount, __reg("d1") LONG type, __reg("a1") const char * file, __reg("d2") LONG line)="\tjsr\t-3312(a6)";
#define CRYPTO_add_lock(pointer, amount, type, file, line) __CRYPTO_add_lock(AmiSSLBase, (pointer), (amount), (type), (file), (line))

int __CRYPTO_get_new_dynlockid(__reg("a6") struct Library *)="\tjsr\t-3318(a6)";
#define CRYPTO_get_new_dynlockid() __CRYPTO_get_new_dynlockid(AmiSSLBase)

void __CRYPTO_destroy_dynlockid(__reg("a6") struct Library *, __reg("d0") LONG i)="\tjsr\t-3324(a6)";
#define CRYPTO_destroy_dynlockid(i) __CRYPTO_destroy_dynlockid(AmiSSLBase, (i))

struct CRYPTO_dynlock_value * __CRYPTO_get_dynlock_value(__reg("a6") struct Library *, __reg("d0") LONG i)="\tjsr\t-3330(a6)";
#define CRYPTO_get_dynlock_value(i) __CRYPTO_get_dynlock_value(AmiSSLBase, (i))

void __CRYPTO_set_dynlock_create_callback(__reg("a6") struct Library *, __reg("a0") struct CRYPTO_dynlock_value * (*dyn_create_function)(const char *file, int line))="\tjsr\t-3336(a6)";
#define CRYPTO_set_dynlock_create_callback(dyn_create_function) __CRYPTO_set_dynlock_create_callback(AmiSSLBase, (dyn_create_function))

void __CRYPTO_set_dynlock_lock_callback(__reg("a6") struct Library *, __reg("a0") void (*dyn_lock_function)(int mode, struct CRYPTO_dynlock_value *l, const char *file, int line))="\tjsr\t-3342(a6)";
#define CRYPTO_set_dynlock_lock_callback(dyn_lock_function) __CRYPTO_set_dynlock_lock_callback(AmiSSLBase, (dyn_lock_function))

void __CRYPTO_set_dynlock_destroy_callback(__reg("a6") struct Library *, __reg("a0") void (*dyn_destroy_function)(struct CRYPTO_dynlock_value *l, const char *file, int line))="\tjsr\t-3348(a6)";
#define CRYPTO_set_dynlock_destroy_callback(dyn_destroy_function) __CRYPTO_set_dynlock_destroy_callback(AmiSSLBase, (dyn_destroy_function))

void * __CRYPTO_get_dynlock_create_callback(__reg("a6") struct Library *)="\tjsr\t-3354(a6)";
#define CRYPTO_get_dynlock_create_callback() __CRYPTO_get_dynlock_create_callback(AmiSSLBase)

void * __CRYPTO_get_dynlock_lock_callback(__reg("a6") struct Library *)="\tjsr\t-3360(a6)";
#define CRYPTO_get_dynlock_lock_callback() __CRYPTO_get_dynlock_lock_callback(AmiSSLBase)

void * __CRYPTO_get_dynlock_destroy_callback(__reg("a6") struct Library *)="\tjsr\t-3366(a6)";
#define CRYPTO_get_dynlock_destroy_callback() __CRYPTO_get_dynlock_destroy_callback(AmiSSLBase)

int __CRYPTO_set_mem_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t), __reg("a1") void * (*r)(void *, size_t), __reg("a2") void (*f)(void *))="\tjsr\t-3372(a6)";
#define CRYPTO_set_mem_functions(m, r, f) __CRYPTO_set_mem_functions(AmiSSLBase, (m), (r), (f))

int __CRYPTO_set_locked_mem_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t), __reg("a1") void (*free_func)(void *))="\tjsr\t-3378(a6)";
#define CRYPTO_set_locked_mem_functions(m, free_func) __CRYPTO_set_locked_mem_functions(AmiSSLBase, (m), (free_func))

int __CRYPTO_set_mem_ex_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t, const char *, int), __reg("a1") void * (*r)(void *, size_t, const char *, int), __reg("a2") void (*f)(void *))="\tjsr\t-3384(a6)";
#define CRYPTO_set_mem_ex_functions(m, r, f) __CRYPTO_set_mem_ex_functions(AmiSSLBase, (m), (r), (f))

int __CRYPTO_set_locked_mem_ex_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t, const char *, int), __reg("a1") void (*free_func)(void *))="\tjsr\t-3390(a6)";
#define CRYPTO_set_locked_mem_ex_functions(m, free_func) __CRYPTO_set_locked_mem_ex_functions(AmiSSLBase, (m), (free_func))

int __CRYPTO_set_mem_debug_functions(__reg("a6") struct Library *, __reg("a0") void (*m)(void *, int, const char *, int, int), __reg("a1") void (*r)(void *, void *, int, const char *, int, int), __reg("a2") void (*f)(void *, int), __reg("a3") void (*so)(long), __reg("d0") long (*go)(void))="\tjsr\t-3396(a6)";
#define CRYPTO_set_mem_debug_functions(m, r, f, so, go) __CRYPTO_set_mem_debug_functions(AmiSSLBase, (m), (r), (f), (so), (go))

void __CRYPTO_get_mem_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t), __reg("a1") void * (*r)(void *, size_t), __reg("a2") void (*f)(void *))="\tjsr\t-3402(a6)";
#define CRYPTO_get_mem_functions(m, r, f) __CRYPTO_get_mem_functions(AmiSSLBase, (m), (r), (f))

void __CRYPTO_get_locked_mem_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t), __reg("a1") void (*f)(void *))="\tjsr\t-3408(a6)";
#define CRYPTO_get_locked_mem_functions(m, f) __CRYPTO_get_locked_mem_functions(AmiSSLBase, (m), (f))

void __CRYPTO_get_mem_ex_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t, const char *, int), __reg("a1") void * (*r)(void *, size_t, const char *, int), __reg("a2") void (*f)(void *))="\tjsr\t-3414(a6)";
#define CRYPTO_get_mem_ex_functions(m, r, f) __CRYPTO_get_mem_ex_functions(AmiSSLBase, (m), (r), (f))

void __CRYPTO_get_locked_mem_ex_functions(__reg("a6") struct Library *, __reg("a0") void * (*m)(size_t, const char *, int), __reg("a1") void (*f)(void *))="\tjsr\t-3420(a6)";
#define CRYPTO_get_locked_mem_ex_functions(m, f) __CRYPTO_get_locked_mem_ex_functions(AmiSSLBase, (m), (f))

void __CRYPTO_get_mem_debug_functions(__reg("a6") struct Library *, __reg("a0") void (*m)(void *, int, const char *, int, int), __reg("a1") void (*r)(void *, void *, int, const char *, int, int), __reg("a2") void (*f)(void *, int), __reg("a3") void (*so)(long), __reg("d0") long (*go)(void))="\tjsr\t-3426(a6)";
#define CRYPTO_get_mem_debug_functions(m, r, f, so, go) __CRYPTO_get_mem_debug_functions(AmiSSLBase, (m), (r), (f), (so), (go))

void * __CRYPTO_malloc_locked(__reg("a6") struct Library *, __reg("d0") LONG num, __reg("a0") const char * file, __reg("d1") LONG line)="\tjsr\t-3432(a6)";
#define CRYPTO_malloc_locked(num, file, line) __CRYPTO_malloc_locked(AmiSSLBase, (num), (file), (line))

void __CRYPTO_free_locked(__reg("a6") struct Library *, __reg("a0") void * a)="\tjsr\t-3438(a6)";
#define CRYPTO_free_locked(a) __CRYPTO_free_locked(AmiSSLBase, (a))

void * __CRYPTO_malloc(__reg("a6") struct Library *, __reg("d0") LONG num, __reg("a0") const char * file, __reg("d1") LONG line)="\tjsr\t-3444(a6)";
#define CRYPTO_malloc(num, file, line) __CRYPTO_malloc(AmiSSLBase, (num), (file), (line))

void __CRYPTO_free(__reg("a6") struct Library *, __reg("a0") void * a)="\tjsr\t-3450(a6)";
#define CRYPTO_free(a) __CRYPTO_free(AmiSSLBase, (a))

void * __CRYPTO_realloc(__reg("a6") struct Library *, __reg("a0") void * addr, __reg("d0") LONG num, __reg("a1") const char * file, __reg("d1") LONG line)="\tjsr\t-3456(a6)";
#define CRYPTO_realloc(addr, num, file, line) __CRYPTO_realloc(AmiSSLBase, (addr), (num), (file), (line))

void * __CRYPTO_realloc_clean(__reg("a6") struct Library *, __reg("a0") void * addr, __reg("d0") LONG old_num, __reg("d1") LONG num, __reg("a1") const char * file, __reg("d2") LONG line)="\tjsr\t-3462(a6)";
#define CRYPTO_realloc_clean(addr, old_num, num, file, line) __CRYPTO_realloc_clean(AmiSSLBase, (addr), (old_num), (num), (file), (line))

void * __CRYPTO_remalloc(__reg("a6") struct Library *, __reg("a0") void * addr, __reg("d0") LONG num, __reg("a1") const char * file, __reg("d1") LONG line)="\tjsr\t-3468(a6)";
#define CRYPTO_remalloc(addr, num, file, line) __CRYPTO_remalloc(AmiSSLBase, (addr), (num), (file), (line))

void __OPENSSL_cleanse(__reg("a6") struct Library *, __reg("a0") void * ptr, __reg("d0") ULONG len)="\tjsr\t-3474(a6)";
#define OPENSSL_cleanse(ptr, len) __OPENSSL_cleanse(AmiSSLBase, (ptr), (len))

void __CRYPTO_set_mem_debug_options(__reg("a6") struct Library *, __reg("d0") long bits)="\tjsr\t-3480(a6)";
#define CRYPTO_set_mem_debug_options(bits) __CRYPTO_set_mem_debug_options(AmiSSLBase, (bits))

long __CRYPTO_get_mem_debug_options(__reg("a6") struct Library *)="\tjsr\t-3486(a6)";
#define CRYPTO_get_mem_debug_options() __CRYPTO_get_mem_debug_options(AmiSSLBase)

int __CRYPTO_push_info_(__reg("a6") struct Library *, __reg("a0") const char * info, __reg("a1") const char * file, __reg("d0") LONG line)="\tjsr\t-3492(a6)";
#define CRYPTO_push_info_(info, file, line) __CRYPTO_push_info_(AmiSSLBase, (info), (file), (line))

int __CRYPTO_pop_info(__reg("a6") struct Library *)="\tjsr\t-3498(a6)";
#define CRYPTO_pop_info() __CRYPTO_pop_info(AmiSSLBase)

int __CRYPTO_remove_all_info(__reg("a6") struct Library *)="\tjsr\t-3504(a6)";
#define CRYPTO_remove_all_info() __CRYPTO_remove_all_info(AmiSSLBase)

void __CRYPTO_dbg_malloc(__reg("a6") struct Library *, __reg("a0") void * addr, __reg("d0") LONG num, __reg("a1") const char * file, __reg("d1") LONG line, __reg("d2") LONG before_p)="\tjsr\t-3510(a6)";
#define CRYPTO_dbg_malloc(addr, num, file, line, before_p) __CRYPTO_dbg_malloc(AmiSSLBase, (addr), (num), (file), (line), (before_p))

void __CRYPTO_dbg_realloc(__reg("a6") struct Library *, __reg("a0") void * addr1, __reg("a1") void * addr2, __reg("d0") LONG num, __reg("a2") const char * file, __reg("d1") LONG line, __reg("d2") LONG before_p)="\tjsr\t-3516(a6)";
#define CRYPTO_dbg_realloc(addr1, addr2, num, file, line, before_p) __CRYPTO_dbg_realloc(AmiSSLBase, (addr1), (addr2), (num), (file), (line), (before_p))

void __CRYPTO_dbg_free(__reg("a6") struct Library *, __reg("a0") void * addr, __reg("d0") LONG before_p)="\tjsr\t-3522(a6)";
#define CRYPTO_dbg_free(addr, before_p) __CRYPTO_dbg_free(AmiSSLBase, (addr), (before_p))

void __CRYPTO_dbg_set_options(__reg("a6") struct Library *, __reg("d0") long bits)="\tjsr\t-3528(a6)";
#define CRYPTO_dbg_set_options(bits) __CRYPTO_dbg_set_options(AmiSSLBase, (bits))

long __CRYPTO_dbg_get_options(__reg("a6") struct Library *)="\tjsr\t-3534(a6)";
#define CRYPTO_dbg_get_options() __CRYPTO_dbg_get_options(AmiSSLBase)

void __CRYPTO_mem_leaks(__reg("a6") struct Library *, __reg("a0") struct bio_st * bio)="\tjsr\t-3540(a6)";
#define CRYPTO_mem_leaks(bio) __CRYPTO_mem_leaks(AmiSSLBase, (bio))

void __CRYPTO_mem_leaks_cb(__reg("a6") struct Library *, __reg("a0") CRYPTO_MEM_LEAK_CB * (*cb)(unsigned long, const char *, int, int, void *))="\tjsr\t-3546(a6)";
#define CRYPTO_mem_leaks_cb(cb) __CRYPTO_mem_leaks_cb(AmiSSLBase, (cb))

void __OpenSSLDie(__reg("a6") struct Library *, __reg("a0") const char * file, __reg("d0") LONG line, __reg("a1") const char * assertion)="\tjsr\t-3552(a6)";
#define OpenSSLDie(file, line, assertion) __OpenSSLDie(AmiSSLBase, (file), (line), (assertion))

void __ERR_load_CRYPTO_strings(__reg("a6") struct Library *)="\tjsr\t-3558(a6)";
#define ERR_load_CRYPTO_strings() __ERR_load_CRYPTO_strings(AmiSSLBase)

DSO * __DSO_new(__reg("a6") struct Library *)="\tjsr\t-3564(a6)";
#define DSO_new() __DSO_new(AmiSSLBase)

DSO * __DSO_new_method(__reg("a6") struct Library *, __reg("a0") DSO_METHOD * method)="\tjsr\t-3570(a6)";
#define DSO_new_method(method) __DSO_new_method(AmiSSLBase, (method))

int __DSO_free(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3576(a6)";
#define DSO_free(dso) __DSO_free(AmiSSLBase, (dso))

int __DSO_flags(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3582(a6)";
#define DSO_flags(dso) __DSO_flags(AmiSSLBase, (dso))

int __DSO_up_ref(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3588(a6)";
#define DSO_up_ref(dso) __DSO_up_ref(AmiSSLBase, (dso))

long __DSO_ctrl(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("a1") void * parg)="\tjsr\t-3594(a6)";
#define DSO_ctrl(dso, cmd, larg, parg) __DSO_ctrl(AmiSSLBase, (dso), (cmd), (larg), (parg))

int __DSO_set_name_converter(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("d0") LONG cb, __reg("a1") DSO_NAME_CONVERTER_FUNC * oldcb)="\tjsr\t-3600(a6)";
#define DSO_set_name_converter(dso, cb, oldcb) __DSO_set_name_converter(AmiSSLBase, (dso), (cb), (oldcb))

const char * __DSO_get_filename(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3606(a6)";
#define DSO_get_filename(dso) __DSO_get_filename(AmiSSLBase, (dso))

int __DSO_set_filename(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") const char * filename)="\tjsr\t-3612(a6)";
#define DSO_set_filename(dso, filename) __DSO_set_filename(AmiSSLBase, (dso), (filename))

char * __DSO_convert_filename(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") const char * filename)="\tjsr\t-3618(a6)";
#define DSO_convert_filename(dso, filename) __DSO_convert_filename(AmiSSLBase, (dso), (filename))

const char * __DSO_get_loaded_filename(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3624(a6)";
#define DSO_get_loaded_filename(dso) __DSO_get_loaded_filename(AmiSSLBase, (dso))

void __DSO_set_default_method(__reg("a6") struct Library *, __reg("a0") DSO_METHOD * meth)="\tjsr\t-3630(a6)";
#define DSO_set_default_method(meth) __DSO_set_default_method(AmiSSLBase, (meth))

DSO_METHOD * __DSO_get_default_method(__reg("a6") struct Library *)="\tjsr\t-3636(a6)";
#define DSO_get_default_method() __DSO_get_default_method(AmiSSLBase)

DSO_METHOD * __DSO_get_method(__reg("a6") struct Library *, __reg("a0") DSO * dso)="\tjsr\t-3642(a6)";
#define DSO_get_method(dso) __DSO_get_method(AmiSSLBase, (dso))

DSO_METHOD * __DSO_set_method(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") DSO_METHOD * meth)="\tjsr\t-3648(a6)";
#define DSO_set_method(dso, meth) __DSO_set_method(AmiSSLBase, (dso), (meth))

DSO * __DSO_load(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") const char * filename, __reg("a2") DSO_METHOD * meth, __reg("d0") LONG flags)="\tjsr\t-3654(a6)";
#define DSO_load(dso, filename, meth, flags) __DSO_load(AmiSSLBase, (dso), (filename), (meth), (flags))

void * __DSO_bind_var(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") const char * symname)="\tjsr\t-3660(a6)";
#define DSO_bind_var(dso, symname) __DSO_bind_var(AmiSSLBase, (dso), (symname))

DSO_FUNC_TYPE __DSO_bind_func(__reg("a6") struct Library *, __reg("a0") DSO * dso, __reg("a1") const char * symname)="\tjsr\t-3666(a6)";
#define DSO_bind_func(dso, symname) __DSO_bind_func(AmiSSLBase, (dso), (symname))

DSO_METHOD * __DSO_METHOD_openssl(__reg("a6") struct Library *)="\tjsr\t-3672(a6)";
#define DSO_METHOD_openssl() __DSO_METHOD_openssl(AmiSSLBase)

DSO_METHOD * __DSO_METHOD_null(__reg("a6") struct Library *)="\tjsr\t-3678(a6)";
#define DSO_METHOD_null() __DSO_METHOD_null(AmiSSLBase)

DSO_METHOD * __DSO_METHOD_dlfcn(__reg("a6") struct Library *)="\tjsr\t-3684(a6)";
#define DSO_METHOD_dlfcn() __DSO_METHOD_dlfcn(AmiSSLBase)

DSO_METHOD * __DSO_METHOD_dl(__reg("a6") struct Library *)="\tjsr\t-3690(a6)";
#define DSO_METHOD_dl() __DSO_METHOD_dl(AmiSSLBase)

DSO_METHOD * __DSO_METHOD_win32(__reg("a6") struct Library *)="\tjsr\t-3696(a6)";
#define DSO_METHOD_win32() __DSO_METHOD_win32(AmiSSLBase)

DSO_METHOD * __DSO_METHOD_vms(__reg("a6") struct Library *)="\tjsr\t-3702(a6)";
#define DSO_METHOD_vms() __DSO_METHOD_vms(AmiSSLBase)

void __ERR_load_DSO_strings(__reg("a6") struct Library *)="\tjsr\t-3708(a6)";
#define ERR_load_DSO_strings() __ERR_load_DSO_strings(AmiSSLBase)

const EC_METHOD * __EC_GFp_simple_method(__reg("a6") struct Library *)="\tjsr\t-3714(a6)";
#define EC_GFp_simple_method() __EC_GFp_simple_method(AmiSSLBase)

const EC_METHOD * __EC_GFp_mont_method(__reg("a6") struct Library *)="\tjsr\t-3720(a6)";
#define EC_GFp_mont_method() __EC_GFp_mont_method(AmiSSLBase)

EC_GROUP * __EC_GROUP_new(__reg("a6") struct Library *, __reg("a0") const EC_METHOD * a)="\tjsr\t-3726(a6)";
#define EC_GROUP_new(a) __EC_GROUP_new(AmiSSLBase, (a))

void __EC_GROUP_free(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a)="\tjsr\t-3732(a6)";
#define EC_GROUP_free(a) __EC_GROUP_free(AmiSSLBase, (a))

void __EC_GROUP_clear_free(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a)="\tjsr\t-3738(a6)";
#define EC_GROUP_clear_free(a) __EC_GROUP_clear_free(AmiSSLBase, (a))

int __EC_GROUP_copy(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a, __reg("a1") const EC_GROUP * b)="\tjsr\t-3744(a6)";
#define EC_GROUP_copy(a, b) __EC_GROUP_copy(AmiSSLBase, (a), (b))

const EC_METHOD * __EC_GROUP_method_of(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a)="\tjsr\t-3750(a6)";
#define EC_GROUP_method_of(a) __EC_GROUP_method_of(AmiSSLBase, (a))

int __EC_GROUP_set_curve_GFp(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a1, __reg("a1") const BIGNUM * p, __reg("a2") const BIGNUM * a, __reg("a3") const BIGNUM * b, __reg("d0") BN_CTX * a2)="\tjsr\t-3756(a6)";
#define EC_GROUP_set_curve_GFp(a1, p, a, b, a2) __EC_GROUP_set_curve_GFp(AmiSSLBase, (a1), (p), (a), (b), (a2))

int __EC_GROUP_get_curve_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") BIGNUM * p, __reg("a2") BIGNUM * a, __reg("a3") BIGNUM * b, __reg("d0") BN_CTX * a2)="\tjsr\t-3762(a6)";
#define EC_GROUP_get_curve_GFp(a1, p, a, b, a2) __EC_GROUP_get_curve_GFp(AmiSSLBase, (a1), (p), (a), (b), (a2))

EC_GROUP * __EC_GROUP_new_curve_GFp(__reg("a6") struct Library *, __reg("a0") const BIGNUM * p, __reg("a1") const BIGNUM * a, __reg("a2") const BIGNUM * b, __reg("a3") BN_CTX * a1)="\tjsr\t-3768(a6)";
#define EC_GROUP_new_curve_GFp(p, a, b, a1) __EC_GROUP_new_curve_GFp(AmiSSLBase, (p), (a), (b), (a1))

int __EC_GROUP_set_generator(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a1, __reg("a1") const EC_POINT * generator, __reg("a2") const BIGNUM * order, __reg("a3") const BIGNUM * cofactor)="\tjsr\t-3774(a6)";
#define EC_GROUP_set_generator(a1, generator, order, cofactor) __EC_GROUP_set_generator(AmiSSLBase, (a1), (generator), (order), (cofactor))

EC_POINT * __EC_GROUP_get0_generator(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1)="\tjsr\t-3780(a6)";
#define EC_GROUP_get0_generator(a1) __EC_GROUP_get0_generator(AmiSSLBase, (a1))

int __EC_GROUP_get_order(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") BIGNUM * order, __reg("a2") BN_CTX * a2)="\tjsr\t-3786(a6)";
#define EC_GROUP_get_order(a1, order, a2) __EC_GROUP_get_order(AmiSSLBase, (a1), (order), (a2))

int __EC_GROUP_get_cofactor(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") BIGNUM * cofactor, __reg("a2") BN_CTX * a2)="\tjsr\t-3792(a6)";
#define EC_GROUP_get_cofactor(a1, cofactor, a2) __EC_GROUP_get_cofactor(AmiSSLBase, (a1), (cofactor), (a2))

EC_POINT * __EC_POINT_new(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a)="\tjsr\t-3798(a6)";
#define EC_POINT_new(a) __EC_POINT_new(AmiSSLBase, (a))

void __EC_POINT_free(__reg("a6") struct Library *, __reg("a0") EC_POINT * a)="\tjsr\t-3804(a6)";
#define EC_POINT_free(a) __EC_POINT_free(AmiSSLBase, (a))

void __EC_POINT_clear_free(__reg("a6") struct Library *, __reg("a0") EC_POINT * a)="\tjsr\t-3810(a6)";
#define EC_POINT_clear_free(a) __EC_POINT_clear_free(AmiSSLBase, (a))

int __EC_POINT_copy(__reg("a6") struct Library *, __reg("a0") EC_POINT * a, __reg("a1") const EC_POINT * b)="\tjsr\t-3816(a6)";
#define EC_POINT_copy(a, b) __EC_POINT_copy(AmiSSLBase, (a), (b))

const EC_METHOD * __EC_POINT_method_of(__reg("a6") struct Library *, __reg("a0") const EC_POINT * a)="\tjsr\t-3822(a6)";
#define EC_POINT_method_of(a) __EC_POINT_method_of(AmiSSLBase, (a))

int __EC_POINT_set_to_infinity(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a, __reg("a1") EC_POINT * b)="\tjsr\t-3828(a6)";
#define EC_POINT_set_to_infinity(a, b) __EC_POINT_set_to_infinity(AmiSSLBase, (a), (b))

int __EC_POINT_set_Jprojective_coordinates_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * a2, __reg("a2") const BIGNUM * x, __reg("a3") const BIGNUM * y, __reg("d0") const BIGNUM * z, __reg("d1") BN_CTX * a3)="\tjsr\t-3834(a6)";
#define EC_POINT_set_Jprojective_coordinates_GFp(a1, a2, x, y, z, a3) __EC_POINT_set_Jprojective_coordinates_GFp(AmiSSLBase, (a1), (a2), (x), (y), (z), (a3))

int __EC_POINT_get_Jprojective_coordinates_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") const EC_POINT * a2, __reg("a2") BIGNUM * x, __reg("a3") BIGNUM * y, __reg("d0") BIGNUM * z, __reg("d1") BN_CTX * a3)="\tjsr\t-3840(a6)";
#define EC_POINT_get_Jprojective_coordinates_GFp(a1, a2, x, y, z, a3) __EC_POINT_get_Jprojective_coordinates_GFp(AmiSSLBase, (a1), (a2), (x), (y), (z), (a3))

int __EC_POINT_set_affine_coordinates_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * a2, __reg("a2") const BIGNUM * x, __reg("a3") const BIGNUM * y, __reg("d0") BN_CTX * a3)="\tjsr\t-3846(a6)";
#define EC_POINT_set_affine_coordinates_GFp(a1, a2, x, y, a3) __EC_POINT_set_affine_coordinates_GFp(AmiSSLBase, (a1), (a2), (x), (y), (a3))

int __EC_POINT_get_affine_coordinates_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") const EC_POINT * a2, __reg("a2") BIGNUM * x, __reg("a3") BIGNUM * y, __reg("d0") BN_CTX * a3)="\tjsr\t-3852(a6)";
#define EC_POINT_get_affine_coordinates_GFp(a1, a2, x, y, a3) __EC_POINT_get_affine_coordinates_GFp(AmiSSLBase, (a1), (a2), (x), (y), (a3))

int __EC_POINT_set_compressed_coordinates_GFp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * a2, __reg("a2") const BIGNUM * x, __reg("d0") LONG y_bit, __reg("a3") BN_CTX * a3)="\tjsr\t-3858(a6)";
#define EC_POINT_set_compressed_coordinates_GFp(a1, a2, x, y_bit, a3) __EC_POINT_set_compressed_coordinates_GFp(AmiSSLBase, (a1), (a2), (x), (y_bit), (a3))

size_t __EC_POINT_point2oct(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") const EC_POINT * a2, __reg("d0") LONG form, __reg("a2") unsigned char * buf, __reg("d1") ULONG len, __reg("a3") BN_CTX * a3)="\tjsr\t-3864(a6)";
#define EC_POINT_point2oct(a1, a2, form, buf, len, a3) __EC_POINT_point2oct(AmiSSLBase, (a1), (a2), (form), (buf), (len), (a3))

int __EC_POINT_oct2point(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * a2, __reg("a2") const unsigned char * buf, __reg("d0") ULONG len, __reg("a3") BN_CTX * a3)="\tjsr\t-3870(a6)";
#define EC_POINT_oct2point(a1, a2, buf, len, a3) __EC_POINT_oct2point(AmiSSLBase, (a1), (a2), (buf), (len), (a3))

int __EC_POINT_add(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * r, __reg("a2") const EC_POINT * a, __reg("a3") const EC_POINT * b, __reg("d0") BN_CTX * a2)="\tjsr\t-3876(a6)";
#define EC_POINT_add(a1, r, a, b, a2) __EC_POINT_add(AmiSSLBase, (a1), (r), (a), (b), (a2))

int __EC_POINT_dbl(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * r, __reg("a2") const EC_POINT * a, __reg("a3") BN_CTX * a2)="\tjsr\t-3882(a6)";
#define EC_POINT_dbl(a1, r, a, a2) __EC_POINT_dbl(AmiSSLBase, (a1), (r), (a), (a2))

int __EC_POINT_invert(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * a2, __reg("a2") BN_CTX * a3)="\tjsr\t-3888(a6)";
#define EC_POINT_invert(a1, a2, a3) __EC_POINT_invert(AmiSSLBase, (a1), (a2), (a3))

int __EC_POINT_is_at_infinity(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a, __reg("a1") const EC_POINT * b)="\tjsr\t-3894(a6)";
#define EC_POINT_is_at_infinity(a, b) __EC_POINT_is_at_infinity(AmiSSLBase, (a), (b))

int __EC_POINT_is_on_curve(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a, __reg("a1") const EC_POINT * b, __reg("a2") BN_CTX * c)="\tjsr\t-3900(a6)";
#define EC_POINT_is_on_curve(a, b, c) __EC_POINT_is_on_curve(AmiSSLBase, (a), (b), (c))

int __EC_POINT_cmp(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") const EC_POINT * a, __reg("a2") const EC_POINT * b, __reg("a3") BN_CTX * a2)="\tjsr\t-3906(a6)";
#define EC_POINT_cmp(a1, a, b, a2) __EC_POINT_cmp(AmiSSLBase, (a1), (a), (b), (a2))

int __EC_POINT_make_affine(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a, __reg("a1") EC_POINT * b, __reg("a2") BN_CTX * c)="\tjsr\t-3912(a6)";
#define EC_POINT_make_affine(a, b, c) __EC_POINT_make_affine(AmiSSLBase, (a), (b), (c))

int __EC_POINTs_make_affine(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("d0") ULONG num, __reg("a1") EC_POINT ** a2, __reg("a2") BN_CTX * a3)="\tjsr\t-3918(a6)";
#define EC_POINTs_make_affine(a1, num, a2, a3) __EC_POINTs_make_affine(AmiSSLBase, (a1), (num), (a2), (a3))

int __EC_POINTs_mul(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * r, __reg("a2") const BIGNUM * a2, __reg("d0") ULONG num, __reg("a3") const EC_POINT ** a3, __reg("d1") const BIGNUM ** a4, __reg("d2") BN_CTX * a5)="\tjsr\t-3924(a6)";
#define EC_POINTs_mul(a1, r, a2, num, a3, a4, a5) __EC_POINTs_mul(AmiSSLBase, (a1), (r), (a2), (num), (a3), (a4), (a5))

int __EC_POINT_mul(__reg("a6") struct Library *, __reg("a0") const EC_GROUP * a1, __reg("a1") EC_POINT * r, __reg("a2") const BIGNUM * a2, __reg("a3") const EC_POINT * a3, __reg("d0") const BIGNUM * a4, __reg("d1") BN_CTX * a5)="\tjsr\t-3930(a6)";
#define EC_POINT_mul(a1, r, a2, a3, a4, a5) __EC_POINT_mul(AmiSSLBase, (a1), (r), (a2), (a3), (a4), (a5))

int __EC_GROUP_precompute_mult(__reg("a6") struct Library *, __reg("a0") EC_GROUP * a, __reg("a1") BN_CTX * b)="\tjsr\t-3936(a6)";
#define EC_GROUP_precompute_mult(a, b) __EC_GROUP_precompute_mult(AmiSSLBase, (a), (b))

void __ERR_load_EC_strings(__reg("a6") struct Library *)="\tjsr\t-3942(a6)";
#define ERR_load_EC_strings() __ERR_load_EC_strings(AmiSSLBase)

void __ERR_put_error(__reg("a6") struct Library *, __reg("d0") LONG lib, __reg("d1") LONG func, __reg("d2") LONG reason, __reg("a0") const char * file, __reg("d3") LONG line)="\tjsr\t-3948(a6)";
#define ERR_put_error(lib, func, reason, file, line) __ERR_put_error(AmiSSLBase, (lib), (func), (reason), (file), (line))

void __ERR_set_error_data(__reg("a6") struct Library *, __reg("a0") char * data, __reg("d0") LONG flags)="\tjsr\t-3954(a6)";
#define ERR_set_error_data(data, flags) __ERR_set_error_data(AmiSSLBase, (data), (flags))

unsigned long __ERR_get_error(__reg("a6") struct Library *)="\tjsr\t-3960(a6)";
#define ERR_get_error() __ERR_get_error(AmiSSLBase)

unsigned long __ERR_get_error_line(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line)="\tjsr\t-3966(a6)";
#define ERR_get_error_line(file, line) __ERR_get_error_line(AmiSSLBase, (file), (line))

unsigned long __ERR_get_error_line_data(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line, __reg("a2") const char ** data, __reg("a3") int * flags)="\tjsr\t-3972(a6)";
#define ERR_get_error_line_data(file, line, data, flags) __ERR_get_error_line_data(AmiSSLBase, (file), (line), (data), (flags))

unsigned long __ERR_peek_error(__reg("a6") struct Library *)="\tjsr\t-3978(a6)";
#define ERR_peek_error() __ERR_peek_error(AmiSSLBase)

unsigned long __ERR_peek_error_line(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line)="\tjsr\t-3984(a6)";
#define ERR_peek_error_line(file, line) __ERR_peek_error_line(AmiSSLBase, (file), (line))

unsigned long __ERR_peek_error_line_data(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line, __reg("a2") const char ** data, __reg("a3") int * flags)="\tjsr\t-3990(a6)";
#define ERR_peek_error_line_data(file, line, data, flags) __ERR_peek_error_line_data(AmiSSLBase, (file), (line), (data), (flags))

unsigned long __ERR_peek_last_error(__reg("a6") struct Library *)="\tjsr\t-3996(a6)";
#define ERR_peek_last_error() __ERR_peek_last_error(AmiSSLBase)

unsigned long __ERR_peek_last_error_line(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line)="\tjsr\t-4002(a6)";
#define ERR_peek_last_error_line(file, line) __ERR_peek_last_error_line(AmiSSLBase, (file), (line))

unsigned long __ERR_peek_last_error_line_data(__reg("a6") struct Library *, __reg("a0") const char ** file, __reg("a1") int * line, __reg("a2") const char ** data, __reg("a3") int * flags)="\tjsr\t-4008(a6)";
#define ERR_peek_last_error_line_data(file, line, data, flags) __ERR_peek_last_error_line_data(AmiSSLBase, (file), (line), (data), (flags))

void __ERR_clear_error(__reg("a6") struct Library *)="\tjsr\t-4014(a6)";
#define ERR_clear_error() __ERR_clear_error(AmiSSLBase)

char * __ERR_error_string(__reg("a6") struct Library *, __reg("d0") unsigned long e, __reg("a0") char * buf)="\tjsr\t-4020(a6)";
#define ERR_error_string(e, buf) __ERR_error_string(AmiSSLBase, (e), (buf))

void __ERR_error_string_n(__reg("a6") struct Library *, __reg("d0") unsigned long e, __reg("a0") char * buf, __reg("d1") ULONG len)="\tjsr\t-4026(a6)";
#define ERR_error_string_n(e, buf, len) __ERR_error_string_n(AmiSSLBase, (e), (buf), (len))

const char * __ERR_lib_error_string(__reg("a6") struct Library *, __reg("d0") unsigned long e)="\tjsr\t-4032(a6)";
#define ERR_lib_error_string(e) __ERR_lib_error_string(AmiSSLBase, (e))

const char * __ERR_func_error_string(__reg("a6") struct Library *, __reg("d0") unsigned long e)="\tjsr\t-4038(a6)";
#define ERR_func_error_string(e) __ERR_func_error_string(AmiSSLBase, (e))

const char * __ERR_reason_error_string(__reg("a6") struct Library *, __reg("d0") unsigned long e)="\tjsr\t-4044(a6)";
#define ERR_reason_error_string(e) __ERR_reason_error_string(AmiSSLBase, (e))

void __ERR_print_errors_cb(__reg("a6") struct Library *, __reg("a0") int (*cb)(const char *str, size_t len, void *u), __reg("a1") void * u)="\tjsr\t-4050(a6)";
#define ERR_print_errors_cb(cb, u) __ERR_print_errors_cb(AmiSSLBase, (cb), (u))

void __ERR_print_errors(__reg("a6") struct Library *, __reg("a0") BIO * bp)="\tjsr\t-4056(a6)";
#define ERR_print_errors(bp) __ERR_print_errors(AmiSSLBase, (bp))

void __ERR_add_error_dataA(__reg("a6") struct Library *, __reg("d0") LONG num, __reg("d1") long * args)="\tjsr\t-4062(a6)";
#define ERR_add_error_dataA(num, args) __ERR_add_error_dataA(AmiSSLBase, (num), (args))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
void __ERR_add_error_data(__reg("a6") struct Library *, __reg("d0") LONG num, long args, ...)="\tmove.l\td1,-(a7)\n\tmove.l\ta7,d1\n\taddq.l\t#4,d1\n\tjsr\t-4062(a6)\n\tmove.l\t(a7)+,d1";
#define ERR_add_error_data(num, ...) __ERR_add_error_data(AmiSSLBase, (num), __VA_ARGS__)
#endif

void __ERR_load_strings(__reg("a6") struct Library *, __reg("d0") LONG lib, __reg("a0") ERR_STRING_DATA * str)="\tjsr\t-4068(a6)";
#define ERR_load_strings(lib, str) __ERR_load_strings(AmiSSLBase, (lib), (str))

void __ERR_unload_strings(__reg("a6") struct Library *, __reg("d0") LONG lib, __reg("a0") ERR_STRING_DATA * str)="\tjsr\t-4074(a6)";
#define ERR_unload_strings(lib, str) __ERR_unload_strings(AmiSSLBase, (lib), (str))

void __ERR_load_ERR_strings(__reg("a6") struct Library *)="\tjsr\t-4080(a6)";
#define ERR_load_ERR_strings() __ERR_load_ERR_strings(AmiSSLBase)

void __ERR_load_crypto_strings(__reg("a6") struct Library *)="\tjsr\t-4086(a6)";
#define ERR_load_crypto_strings() __ERR_load_crypto_strings(AmiSSLBase)

void __ERR_free_strings(__reg("a6") struct Library *)="\tjsr\t-4092(a6)";
#define ERR_free_strings() __ERR_free_strings(AmiSSLBase)

void __ERR_remove_state(__reg("a6") struct Library *, __reg("d0") unsigned long pid)="\tjsr\t-4098(a6)";
#define ERR_remove_state(pid) __ERR_remove_state(AmiSSLBase, (pid))

ERR_STATE * __ERR_get_state(__reg("a6") struct Library *)="\tjsr\t-4104(a6)";
#define ERR_get_state() __ERR_get_state(AmiSSLBase)

LHASH * __ERR_get_string_table(__reg("a6") struct Library *)="\tjsr\t-4110(a6)";
#define ERR_get_string_table() __ERR_get_string_table(AmiSSLBase)

LHASH * __ERR_get_err_state_table(__reg("a6") struct Library *)="\tjsr\t-4116(a6)";
#define ERR_get_err_state_table() __ERR_get_err_state_table(AmiSSLBase)

void __ERR_release_err_state_table(__reg("a6") struct Library *, __reg("a0") LHASH ** hash)="\tjsr\t-4122(a6)";
#define ERR_release_err_state_table(hash) __ERR_release_err_state_table(AmiSSLBase, (hash))

int __ERR_get_next_error_library(__reg("a6") struct Library *)="\tjsr\t-4128(a6)";
#define ERR_get_next_error_library() __ERR_get_next_error_library(AmiSSLBase)

const ERR_FNS * __ERR_get_implementation(__reg("a6") struct Library *)="\tjsr\t-4134(a6)";
#define ERR_get_implementation() __ERR_get_implementation(AmiSSLBase)

int __ERR_set_implementation(__reg("a6") struct Library *, __reg("a0") const ERR_FNS * fns)="\tjsr\t-4140(a6)";
#define ERR_set_implementation(fns) __ERR_set_implementation(AmiSSLBase, (fns))

void __EVP_MD_CTX_init(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx)="\tjsr\t-4146(a6)";
#define EVP_MD_CTX_init(ctx) __EVP_MD_CTX_init(AmiSSLBase, (ctx))

int __EVP_MD_CTX_cleanup(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx)="\tjsr\t-4152(a6)";
#define EVP_MD_CTX_cleanup(ctx) __EVP_MD_CTX_cleanup(AmiSSLBase, (ctx))

EVP_MD_CTX * __EVP_MD_CTX_create(__reg("a6") struct Library *)="\tjsr\t-4158(a6)";
#define EVP_MD_CTX_create() __EVP_MD_CTX_create(AmiSSLBase)

void __EVP_MD_CTX_destroy(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx)="\tjsr\t-4164(a6)";
#define EVP_MD_CTX_destroy(ctx) __EVP_MD_CTX_destroy(AmiSSLBase, (ctx))

int __EVP_MD_CTX_copy_ex(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * out, __reg("a1") const EVP_MD_CTX * in)="\tjsr\t-4170(a6)";
#define EVP_MD_CTX_copy_ex(out, in) __EVP_MD_CTX_copy_ex(AmiSSLBase, (out), (in))

int __EVP_DigestInit_ex(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") const EVP_MD * type, __reg("a2") ENGINE * impl)="\tjsr\t-4176(a6)";
#define EVP_DigestInit_ex(ctx, type, impl) __EVP_DigestInit_ex(AmiSSLBase, (ctx), (type), (impl))

int __EVP_DigestUpdate(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") const void * d, __reg("d0") ULONG cnt)="\tjsr\t-4182(a6)";
#define EVP_DigestUpdate(ctx, d, cnt) __EVP_DigestUpdate(AmiSSLBase, (ctx), (d), (cnt))

int __EVP_DigestFinal_ex(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * md, __reg("a2") unsigned int * s)="\tjsr\t-4188(a6)";
#define EVP_DigestFinal_ex(ctx, md, s) __EVP_DigestFinal_ex(AmiSSLBase, (ctx), (md), (s))

int __EVP_Digest(__reg("a6") struct Library *, __reg("a0") void * data, __reg("d0") ULONG count, __reg("a1") unsigned char * md, __reg("a2") unsigned int * size, __reg("a3") const EVP_MD * type, __reg("d1") ENGINE * impl)="\tjsr\t-4194(a6)";
#define EVP_Digest(data, count, md, size, type, impl) __EVP_Digest(AmiSSLBase, (data), (count), (md), (size), (type), (impl))

int __EVP_MD_CTX_copy(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * out, __reg("a1") const EVP_MD_CTX * in)="\tjsr\t-4200(a6)";
#define EVP_MD_CTX_copy(out, in) __EVP_MD_CTX_copy(AmiSSLBase, (out), (in))

int __EVP_DigestInit(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") const EVP_MD * type)="\tjsr\t-4206(a6)";
#define EVP_DigestInit(ctx, type) __EVP_DigestInit(AmiSSLBase, (ctx), (type))

int __EVP_DigestFinal(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * md, __reg("a2") unsigned int * s)="\tjsr\t-4212(a6)";
#define EVP_DigestFinal(ctx, md, s) __EVP_DigestFinal(AmiSSLBase, (ctx), (md), (s))

int __EVP_read_pw_string(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG length, __reg("a1") const char * prompt, __reg("d1") LONG verify)="\tjsr\t-4218(a6)";
#define EVP_read_pw_string(buf, length, prompt, verify) __EVP_read_pw_string(AmiSSLBase, (buf), (length), (prompt), (verify))

void __EVP_set_pw_prompt(__reg("a6") struct Library *, __reg("a0") char * prompt)="\tjsr\t-4224(a6)";
#define EVP_set_pw_prompt(prompt) __EVP_set_pw_prompt(AmiSSLBase, (prompt))

char * __EVP_get_pw_prompt(__reg("a6") struct Library *)="\tjsr\t-4230(a6)";
#define EVP_get_pw_prompt() __EVP_get_pw_prompt(AmiSSLBase)

int __EVP_BytesToKey(__reg("a6") struct Library *, __reg("a0") const EVP_CIPHER * type, __reg("a1") const EVP_MD * md, __reg("a2") const unsigned char * salt, __reg("a3") const unsigned char * data, __reg("d0") LONG datal, __reg("d1") LONG count, __reg("d2") unsigned char * key, __reg("d3") unsigned char * iv)="\tjsr\t-4236(a6)";
#define EVP_BytesToKey(type, md, salt, data, datal, count, key, iv) __EVP_BytesToKey(AmiSSLBase, (type), (md), (salt), (data), (datal), (count), (key), (iv))

int __EVP_EncryptInit(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") const unsigned char * key, __reg("a3") const unsigned char * iv)="\tjsr\t-4242(a6)";
#define EVP_EncryptInit(ctx, cipher, key, iv) __EVP_EncryptInit(AmiSSLBase, (ctx), (cipher), (key), (iv))

int __EVP_EncryptInit_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") ENGINE * impl, __reg("a3") const unsigned char * key, __reg("d0") const unsigned char * iv)="\tjsr\t-4248(a6)";
#define EVP_EncryptInit_ex(ctx, cipher, impl, key, iv) __EVP_EncryptInit_ex(AmiSSLBase, (ctx), (cipher), (impl), (key), (iv))

int __EVP_EncryptUpdate(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") const unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-4254(a6)";
#define EVP_EncryptUpdate(ctx, out, outl, in, inl) __EVP_EncryptUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

int __EVP_EncryptFinal_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4260(a6)";
#define EVP_EncryptFinal_ex(ctx, out, outl) __EVP_EncryptFinal_ex(AmiSSLBase, (ctx), (out), (outl))

int __EVP_EncryptFinal(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4266(a6)";
#define EVP_EncryptFinal(ctx, out, outl) __EVP_EncryptFinal(AmiSSLBase, (ctx), (out), (outl))

int __EVP_DecryptInit(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") const unsigned char * key, __reg("a3") const unsigned char * iv)="\tjsr\t-4272(a6)";
#define EVP_DecryptInit(ctx, cipher, key, iv) __EVP_DecryptInit(AmiSSLBase, (ctx), (cipher), (key), (iv))

int __EVP_DecryptInit_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") ENGINE * impl, __reg("a3") const unsigned char * key, __reg("d0") const unsigned char * iv)="\tjsr\t-4278(a6)";
#define EVP_DecryptInit_ex(ctx, cipher, impl, key, iv) __EVP_DecryptInit_ex(AmiSSLBase, (ctx), (cipher), (impl), (key), (iv))

int __EVP_DecryptUpdate(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") const unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-4284(a6)";
#define EVP_DecryptUpdate(ctx, out, outl, in, inl) __EVP_DecryptUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

int __EVP_DecryptFinal(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * outm, __reg("a2") int * outl)="\tjsr\t-4290(a6)";
#define EVP_DecryptFinal(ctx, outm, outl) __EVP_DecryptFinal(AmiSSLBase, (ctx), (outm), (outl))

int __EVP_DecryptFinal_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * outm, __reg("a2") int * outl)="\tjsr\t-4296(a6)";
#define EVP_DecryptFinal_ex(ctx, outm, outl) __EVP_DecryptFinal_ex(AmiSSLBase, (ctx), (outm), (outl))

int __EVP_CipherInit(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") const unsigned char * key, __reg("a3") const unsigned char * iv, __reg("d0") LONG enc)="\tjsr\t-4302(a6)";
#define EVP_CipherInit(ctx, cipher, key, iv, enc) __EVP_CipherInit(AmiSSLBase, (ctx), (cipher), (key), (iv), (enc))

int __EVP_CipherInit_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * cipher, __reg("a2") ENGINE * impl, __reg("a3") const unsigned char * key, __reg("d0") const unsigned char * iv, __reg("d1") LONG enc)="\tjsr\t-4308(a6)";
#define EVP_CipherInit_ex(ctx, cipher, impl, key, iv, enc) __EVP_CipherInit_ex(AmiSSLBase, (ctx), (cipher), (impl), (key), (iv), (enc))

int __EVP_CipherUpdate(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") const unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-4314(a6)";
#define EVP_CipherUpdate(ctx, out, outl, in, inl) __EVP_CipherUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

int __EVP_CipherFinal(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * outm, __reg("a2") int * outl)="\tjsr\t-4320(a6)";
#define EVP_CipherFinal(ctx, outm, outl) __EVP_CipherFinal(AmiSSLBase, (ctx), (outm), (outl))

int __EVP_CipherFinal_ex(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * outm, __reg("a2") int * outl)="\tjsr\t-4326(a6)";
#define EVP_CipherFinal_ex(ctx, outm, outl) __EVP_CipherFinal_ex(AmiSSLBase, (ctx), (outm), (outl))

int __EVP_SignFinal(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * md, __reg("a2") unsigned int * s, __reg("a3") EVP_PKEY * pkey)="\tjsr\t-4332(a6)";
#define EVP_SignFinal(ctx, md, s, pkey) __EVP_SignFinal(AmiSSLBase, (ctx), (md), (s), (pkey))

int __EVP_VerifyFinal(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * sigbuf, __reg("d0") ULONG siglen, __reg("a2") EVP_PKEY * pkey)="\tjsr\t-4338(a6)";
#define EVP_VerifyFinal(ctx, sigbuf, siglen, pkey) __EVP_VerifyFinal(AmiSSLBase, (ctx), (sigbuf), (siglen), (pkey))

int __EVP_OpenInit(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * type, __reg("a2") unsigned char * ek, __reg("d0") LONG ekl, __reg("a3") unsigned char * iv, __reg("d1") EVP_PKEY * priv)="\tjsr\t-4344(a6)";
#define EVP_OpenInit(ctx, type, ek, ekl, iv, priv) __EVP_OpenInit(AmiSSLBase, (ctx), (type), (ek), (ekl), (iv), (priv))

int __EVP_OpenFinal(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4350(a6)";
#define EVP_OpenFinal(ctx, out, outl) __EVP_OpenFinal(AmiSSLBase, (ctx), (out), (outl))

int __EVP_SealInit(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const EVP_CIPHER * type, __reg("a2") unsigned char ** ek, __reg("a3") int * ekl, __reg("d0") unsigned char * iv, __reg("d1") EVP_PKEY ** pubk, __reg("d2") LONG npubk)="\tjsr\t-4356(a6)";
#define EVP_SealInit(ctx, type, ek, ekl, iv, pubk, npubk) __EVP_SealInit(AmiSSLBase, (ctx), (type), (ek), (ekl), (iv), (pubk), (npubk))

int __EVP_SealFinal(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4362(a6)";
#define EVP_SealFinal(ctx, out, outl) __EVP_SealFinal(AmiSSLBase, (ctx), (out), (outl))

void __EVP_EncodeInit(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx)="\tjsr\t-4368(a6)";
#define EVP_EncodeInit(ctx) __EVP_EncodeInit(AmiSSLBase, (ctx))

void __EVP_EncodeUpdate(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-4374(a6)";
#define EVP_EncodeUpdate(ctx, out, outl, in, inl) __EVP_EncodeUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

void __EVP_EncodeFinal(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4380(a6)";
#define EVP_EncodeFinal(ctx, out, outl) __EVP_EncodeFinal(AmiSSLBase, (ctx), (out), (outl))

int __EVP_EncodeBlock(__reg("a6") struct Library *, __reg("a0") unsigned char * t, __reg("a1") const unsigned char * f, __reg("d0") LONG n)="\tjsr\t-4386(a6)";
#define EVP_EncodeBlock(t, f, n) __EVP_EncodeBlock(AmiSSLBase, (t), (f), (n))

void __EVP_DecodeInit(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx)="\tjsr\t-4392(a6)";
#define EVP_DecodeInit(ctx) __EVP_DecodeInit(AmiSSLBase, (ctx))

int __EVP_DecodeUpdate(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-4398(a6)";
#define EVP_DecodeUpdate(ctx, out, outl, in, inl) __EVP_DecodeUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

int __EVP_DecodeFinal(__reg("a6") struct Library *, __reg("a0") EVP_ENCODE_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl)="\tjsr\t-4404(a6)";
#define EVP_DecodeFinal(ctx, out, outl) __EVP_DecodeFinal(AmiSSLBase, (ctx), (out), (outl))

int __EVP_DecodeBlock(__reg("a6") struct Library *, __reg("a0") unsigned char * t, __reg("a1") const unsigned char * f, __reg("d0") LONG n)="\tjsr\t-4410(a6)";
#define EVP_DecodeBlock(t, f, n) __EVP_DecodeBlock(AmiSSLBase, (t), (f), (n))

void __EVP_CIPHER_CTX_init(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * a)="\tjsr\t-4416(a6)";
#define EVP_CIPHER_CTX_init(a) __EVP_CIPHER_CTX_init(AmiSSLBase, (a))

int __EVP_CIPHER_CTX_cleanup(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * a)="\tjsr\t-4422(a6)";
#define EVP_CIPHER_CTX_cleanup(a) __EVP_CIPHER_CTX_cleanup(AmiSSLBase, (a))

int __EVP_CIPHER_CTX_set_key_length(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * x, __reg("d0") LONG keylen)="\tjsr\t-4428(a6)";
#define EVP_CIPHER_CTX_set_key_length(x, keylen) __EVP_CIPHER_CTX_set_key_length(AmiSSLBase, (x), (keylen))

int __EVP_CIPHER_CTX_set_padding(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * c, __reg("d0") LONG pad)="\tjsr\t-4434(a6)";
#define EVP_CIPHER_CTX_set_padding(c, pad) __EVP_CIPHER_CTX_set_padding(AmiSSLBase, (c), (pad))

int __EVP_CIPHER_CTX_ctrl(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("d0") LONG type, __reg("d1") LONG arg, __reg("a1") void * ptr)="\tjsr\t-4440(a6)";
#define EVP_CIPHER_CTX_ctrl(ctx, type, arg, ptr) __EVP_CIPHER_CTX_ctrl(AmiSSLBase, (ctx), (type), (arg), (ptr))

BIO_METHOD * __BIO_f_md(__reg("a6") struct Library *)="\tjsr\t-4446(a6)";
#define BIO_f_md() __BIO_f_md(AmiSSLBase)

BIO_METHOD * __BIO_f_base64(__reg("a6") struct Library *)="\tjsr\t-4452(a6)";
#define BIO_f_base64() __BIO_f_base64(AmiSSLBase)

BIO_METHOD * __BIO_f_cipher(__reg("a6") struct Library *)="\tjsr\t-4458(a6)";
#define BIO_f_cipher() __BIO_f_cipher(AmiSSLBase)

BIO_METHOD * __BIO_f_reliable(__reg("a6") struct Library *)="\tjsr\t-4464(a6)";
#define BIO_f_reliable() __BIO_f_reliable(AmiSSLBase)

void __BIO_set_cipher(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") const EVP_CIPHER * c, __reg("a2") unsigned char * k, __reg("a3") unsigned char * i, __reg("d0") LONG enc)="\tjsr\t-4470(a6)";
#define BIO_set_cipher(b, c, k, i, enc) __BIO_set_cipher(AmiSSLBase, (b), (c), (k), (i), (enc))

const EVP_MD * __EVP_md_null(__reg("a6") struct Library *)="\tjsr\t-4476(a6)";
#define EVP_md_null() __EVP_md_null(AmiSSLBase)

const EVP_MD * __EVP_md2(__reg("a6") struct Library *)="\tjsr\t-4482(a6)";
#define EVP_md2() __EVP_md2(AmiSSLBase)

const EVP_MD * __EVP_md4(__reg("a6") struct Library *)="\tjsr\t-4488(a6)";
#define EVP_md4() __EVP_md4(AmiSSLBase)

const EVP_MD * __EVP_md5(__reg("a6") struct Library *)="\tjsr\t-4494(a6)";
#define EVP_md5() __EVP_md5(AmiSSLBase)

const EVP_MD * __EVP_sha(__reg("a6") struct Library *)="\tjsr\t-4500(a6)";
#define EVP_sha() __EVP_sha(AmiSSLBase)

const EVP_MD * __EVP_sha1(__reg("a6") struct Library *)="\tjsr\t-4506(a6)";
#define EVP_sha1() __EVP_sha1(AmiSSLBase)

const EVP_MD * __EVP_dss(__reg("a6") struct Library *)="\tjsr\t-4512(a6)";
#define EVP_dss() __EVP_dss(AmiSSLBase)

const EVP_MD * __EVP_dss1(__reg("a6") struct Library *)="\tjsr\t-4518(a6)";
#define EVP_dss1() __EVP_dss1(AmiSSLBase)

const EVP_MD * __EVP_mdc2(__reg("a6") struct Library *)="\tjsr\t-4524(a6)";
#define EVP_mdc2() __EVP_mdc2(AmiSSLBase)

const EVP_MD * __EVP_ripemd160(__reg("a6") struct Library *)="\tjsr\t-4530(a6)";
#define EVP_ripemd160() __EVP_ripemd160(AmiSSLBase)

const EVP_CIPHER * __EVP_enc_null(__reg("a6") struct Library *)="\tjsr\t-4536(a6)";
#define EVP_enc_null() __EVP_enc_null(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ecb(__reg("a6") struct Library *)="\tjsr\t-4542(a6)";
#define EVP_des_ecb() __EVP_des_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede(__reg("a6") struct Library *)="\tjsr\t-4548(a6)";
#define EVP_des_ede() __EVP_des_ede(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3(__reg("a6") struct Library *)="\tjsr\t-4554(a6)";
#define EVP_des_ede3() __EVP_des_ede3(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede_ecb(__reg("a6") struct Library *)="\tjsr\t-4560(a6)";
#define EVP_des_ede_ecb() __EVP_des_ede_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_ecb(__reg("a6") struct Library *)="\tjsr\t-4566(a6)";
#define EVP_des_ede3_ecb() __EVP_des_ede3_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_cfb64(__reg("a6") struct Library *)="\tjsr\t-4572(a6)";
#define EVP_des_cfb64() __EVP_des_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_des_cfb1(__reg("a6") struct Library *)="\tjsr\t-4578(a6)";
#define EVP_des_cfb1() __EVP_des_cfb1(AmiSSLBase)

const EVP_CIPHER * __EVP_des_cfb8(__reg("a6") struct Library *)="\tjsr\t-4584(a6)";
#define EVP_des_cfb8() __EVP_des_cfb8(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede_cfb64(__reg("a6") struct Library *)="\tjsr\t-4590(a6)";
#define EVP_des_ede_cfb64() __EVP_des_ede_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_cfb64(__reg("a6") struct Library *)="\tjsr\t-4596(a6)";
#define EVP_des_ede3_cfb64() __EVP_des_ede3_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_cfb1(__reg("a6") struct Library *)="\tjsr\t-4602(a6)";
#define EVP_des_ede3_cfb1() __EVP_des_ede3_cfb1(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_cfb8(__reg("a6") struct Library *)="\tjsr\t-4608(a6)";
#define EVP_des_ede3_cfb8() __EVP_des_ede3_cfb8(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ofb(__reg("a6") struct Library *)="\tjsr\t-4614(a6)";
#define EVP_des_ofb() __EVP_des_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede_ofb(__reg("a6") struct Library *)="\tjsr\t-4620(a6)";
#define EVP_des_ede_ofb() __EVP_des_ede_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_ofb(__reg("a6") struct Library *)="\tjsr\t-4626(a6)";
#define EVP_des_ede3_ofb() __EVP_des_ede3_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_des_cbc(__reg("a6") struct Library *)="\tjsr\t-4632(a6)";
#define EVP_des_cbc() __EVP_des_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede_cbc(__reg("a6") struct Library *)="\tjsr\t-4638(a6)";
#define EVP_des_ede_cbc() __EVP_des_ede_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_des_ede3_cbc(__reg("a6") struct Library *)="\tjsr\t-4644(a6)";
#define EVP_des_ede3_cbc() __EVP_des_ede3_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_desx_cbc(__reg("a6") struct Library *)="\tjsr\t-4650(a6)";
#define EVP_desx_cbc() __EVP_desx_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc4(__reg("a6") struct Library *)="\tjsr\t-4656(a6)";
#define EVP_rc4() __EVP_rc4(AmiSSLBase)

const EVP_CIPHER * __EVP_rc4_40(__reg("a6") struct Library *)="\tjsr\t-4662(a6)";
#define EVP_rc4_40() __EVP_rc4_40(AmiSSLBase)

const EVP_CIPHER * __EVP_idea_ecb(__reg("a6") struct Library *)="\tjsr\t-4668(a6)";
#define EVP_idea_ecb() __EVP_idea_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_idea_cfb64(__reg("a6") struct Library *)="\tjsr\t-4674(a6)";
#define EVP_idea_cfb64() __EVP_idea_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_idea_ofb(__reg("a6") struct Library *)="\tjsr\t-4680(a6)";
#define EVP_idea_ofb() __EVP_idea_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_idea_cbc(__reg("a6") struct Library *)="\tjsr\t-4686(a6)";
#define EVP_idea_cbc() __EVP_idea_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_ecb(__reg("a6") struct Library *)="\tjsr\t-4692(a6)";
#define EVP_rc2_ecb() __EVP_rc2_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_cbc(__reg("a6") struct Library *)="\tjsr\t-4698(a6)";
#define EVP_rc2_cbc() __EVP_rc2_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_40_cbc(__reg("a6") struct Library *)="\tjsr\t-4704(a6)";
#define EVP_rc2_40_cbc() __EVP_rc2_40_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_64_cbc(__reg("a6") struct Library *)="\tjsr\t-4710(a6)";
#define EVP_rc2_64_cbc() __EVP_rc2_64_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_cfb64(__reg("a6") struct Library *)="\tjsr\t-4716(a6)";
#define EVP_rc2_cfb64() __EVP_rc2_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_rc2_ofb(__reg("a6") struct Library *)="\tjsr\t-4722(a6)";
#define EVP_rc2_ofb() __EVP_rc2_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_bf_ecb(__reg("a6") struct Library *)="\tjsr\t-4728(a6)";
#define EVP_bf_ecb() __EVP_bf_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_bf_cbc(__reg("a6") struct Library *)="\tjsr\t-4734(a6)";
#define EVP_bf_cbc() __EVP_bf_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_bf_cfb64(__reg("a6") struct Library *)="\tjsr\t-4740(a6)";
#define EVP_bf_cfb64() __EVP_bf_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_bf_ofb(__reg("a6") struct Library *)="\tjsr\t-4746(a6)";
#define EVP_bf_ofb() __EVP_bf_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_cast5_ecb(__reg("a6") struct Library *)="\tjsr\t-4752(a6)";
#define EVP_cast5_ecb() __EVP_cast5_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_cast5_cbc(__reg("a6") struct Library *)="\tjsr\t-4758(a6)";
#define EVP_cast5_cbc() __EVP_cast5_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_cast5_cfb64(__reg("a6") struct Library *)="\tjsr\t-4764(a6)";
#define EVP_cast5_cfb64() __EVP_cast5_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_cast5_ofb(__reg("a6") struct Library *)="\tjsr\t-4770(a6)";
#define EVP_cast5_ofb() __EVP_cast5_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_rc5_32_12_16_cbc(__reg("a6") struct Library *)="\tjsr\t-4776(a6)";
#define EVP_rc5_32_12_16_cbc() __EVP_rc5_32_12_16_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_rc5_32_12_16_ecb(__reg("a6") struct Library *)="\tjsr\t-4782(a6)";
#define EVP_rc5_32_12_16_ecb() __EVP_rc5_32_12_16_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_rc5_32_12_16_cfb64(__reg("a6") struct Library *)="\tjsr\t-4788(a6)";
#define EVP_rc5_32_12_16_cfb64() __EVP_rc5_32_12_16_cfb64(AmiSSLBase)

const EVP_CIPHER * __EVP_rc5_32_12_16_ofb(__reg("a6") struct Library *)="\tjsr\t-4794(a6)";
#define EVP_rc5_32_12_16_ofb() __EVP_rc5_32_12_16_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_ecb(__reg("a6") struct Library *)="\tjsr\t-4800(a6)";
#define EVP_aes_128_ecb() __EVP_aes_128_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_cbc(__reg("a6") struct Library *)="\tjsr\t-4806(a6)";
#define EVP_aes_128_cbc() __EVP_aes_128_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_cfb1(__reg("a6") struct Library *)="\tjsr\t-4812(a6)";
#define EVP_aes_128_cfb1() __EVP_aes_128_cfb1(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_cfb8(__reg("a6") struct Library *)="\tjsr\t-4818(a6)";
#define EVP_aes_128_cfb8() __EVP_aes_128_cfb8(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_cfb128(__reg("a6") struct Library *)="\tjsr\t-4824(a6)";
#define EVP_aes_128_cfb128() __EVP_aes_128_cfb128(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_128_ofb(__reg("a6") struct Library *)="\tjsr\t-4830(a6)";
#define EVP_aes_128_ofb() __EVP_aes_128_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_ecb(__reg("a6") struct Library *)="\tjsr\t-4836(a6)";
#define EVP_aes_192_ecb() __EVP_aes_192_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_cbc(__reg("a6") struct Library *)="\tjsr\t-4842(a6)";
#define EVP_aes_192_cbc() __EVP_aes_192_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_cfb1(__reg("a6") struct Library *)="\tjsr\t-4848(a6)";
#define EVP_aes_192_cfb1() __EVP_aes_192_cfb1(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_cfb8(__reg("a6") struct Library *)="\tjsr\t-4854(a6)";
#define EVP_aes_192_cfb8() __EVP_aes_192_cfb8(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_cfb128(__reg("a6") struct Library *)="\tjsr\t-4860(a6)";
#define EVP_aes_192_cfb128() __EVP_aes_192_cfb128(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_192_ofb(__reg("a6") struct Library *)="\tjsr\t-4866(a6)";
#define EVP_aes_192_ofb() __EVP_aes_192_ofb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_ecb(__reg("a6") struct Library *)="\tjsr\t-4872(a6)";
#define EVP_aes_256_ecb() __EVP_aes_256_ecb(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_cbc(__reg("a6") struct Library *)="\tjsr\t-4878(a6)";
#define EVP_aes_256_cbc() __EVP_aes_256_cbc(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_cfb1(__reg("a6") struct Library *)="\tjsr\t-4884(a6)";
#define EVP_aes_256_cfb1() __EVP_aes_256_cfb1(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_cfb8(__reg("a6") struct Library *)="\tjsr\t-4890(a6)";
#define EVP_aes_256_cfb8() __EVP_aes_256_cfb8(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_cfb128(__reg("a6") struct Library *)="\tjsr\t-4896(a6)";
#define EVP_aes_256_cfb128() __EVP_aes_256_cfb128(AmiSSLBase)

const EVP_CIPHER * __EVP_aes_256_ofb(__reg("a6") struct Library *)="\tjsr\t-4902(a6)";
#define EVP_aes_256_ofb() __EVP_aes_256_ofb(AmiSSLBase)

void __OPENSSL_add_all_algorithms_noconf(__reg("a6") struct Library *)="\tjsr\t-4908(a6)";
#define OPENSSL_add_all_algorithms_noconf() __OPENSSL_add_all_algorithms_noconf(AmiSSLBase)

void __OPENSSL_add_all_algorithms_conf(__reg("a6") struct Library *)="\tjsr\t-4914(a6)";
#define OPENSSL_add_all_algorithms_conf() __OPENSSL_add_all_algorithms_conf(AmiSSLBase)

void __OpenSSL_add_all_ciphers(__reg("a6") struct Library *)="\tjsr\t-4920(a6)";
#define OpenSSL_add_all_ciphers() __OpenSSL_add_all_ciphers(AmiSSLBase)

void __OpenSSL_add_all_digests(__reg("a6") struct Library *)="\tjsr\t-4926(a6)";
#define OpenSSL_add_all_digests() __OpenSSL_add_all_digests(AmiSSLBase)

int __EVP_add_cipher(__reg("a6") struct Library *, __reg("a0") const EVP_CIPHER * cipher)="\tjsr\t-4932(a6)";
#define EVP_add_cipher(cipher) __EVP_add_cipher(AmiSSLBase, (cipher))

int __EVP_add_digest(__reg("a6") struct Library *, __reg("a0") const EVP_MD * digest)="\tjsr\t-4938(a6)";
#define EVP_add_digest(digest) __EVP_add_digest(AmiSSLBase, (digest))

const EVP_CIPHER * __EVP_get_cipherbyname(__reg("a6") struct Library *, __reg("a0") const char * name)="\tjsr\t-4944(a6)";
#define EVP_get_cipherbyname(name) __EVP_get_cipherbyname(AmiSSLBase, (name))

const EVP_MD * __EVP_get_digestbyname(__reg("a6") struct Library *, __reg("a0") const char * name)="\tjsr\t-4950(a6)";
#define EVP_get_digestbyname(name) __EVP_get_digestbyname(AmiSSLBase, (name))

void __EVP_cleanup(__reg("a6") struct Library *)="\tjsr\t-4956(a6)";
#define EVP_cleanup() __EVP_cleanup(AmiSSLBase)

int __EVP_PKEY_decrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * dec_key, __reg("a1") unsigned char * enc_key, __reg("d0") LONG enc_key_len, __reg("a2") EVP_PKEY * private_key)="\tjsr\t-4962(a6)";
#define EVP_PKEY_decrypt(dec_key, enc_key, enc_key_len, private_key) __EVP_PKEY_decrypt(AmiSSLBase, (dec_key), (enc_key), (enc_key_len), (private_key))

int __EVP_PKEY_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * enc_key, __reg("a1") unsigned char * key, __reg("d0") LONG key_len, __reg("a2") EVP_PKEY * pub_key)="\tjsr\t-4968(a6)";
#define EVP_PKEY_encrypt(enc_key, key, key_len, pub_key) __EVP_PKEY_encrypt(AmiSSLBase, (enc_key), (key), (key_len), (pub_key))

int __EVP_PKEY_type(__reg("a6") struct Library *, __reg("d0") LONG type)="\tjsr\t-4974(a6)";
#define EVP_PKEY_type(type) __EVP_PKEY_type(AmiSSLBase, (type))

int __EVP_PKEY_bits(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-4980(a6)";
#define EVP_PKEY_bits(pkey) __EVP_PKEY_bits(AmiSSLBase, (pkey))

int __EVP_PKEY_size(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-4986(a6)";
#define EVP_PKEY_size(pkey) __EVP_PKEY_size(AmiSSLBase, (pkey))

int __EVP_PKEY_assign(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("d0") LONG type, __reg("a1") char * key)="\tjsr\t-4992(a6)";
#define EVP_PKEY_assign(pkey, type, key) __EVP_PKEY_assign(AmiSSLBase, (pkey), (type), (key))

int __EVP_PKEY_set1_RSA(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("a1") struct rsa_st * key)="\tjsr\t-4998(a6)";
#define EVP_PKEY_set1_RSA(pkey, key) __EVP_PKEY_set1_RSA(AmiSSLBase, (pkey), (key))

struct rsa_st * __EVP_PKEY_get1_RSA(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-5004(a6)";
#define EVP_PKEY_get1_RSA(pkey) __EVP_PKEY_get1_RSA(AmiSSLBase, (pkey))

int __EVP_PKEY_set1_DSA(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("a1") struct dsa_st * key)="\tjsr\t-5010(a6)";
#define EVP_PKEY_set1_DSA(pkey, key) __EVP_PKEY_set1_DSA(AmiSSLBase, (pkey), (key))

struct dsa_st * __EVP_PKEY_get1_DSA(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-5016(a6)";
#define EVP_PKEY_get1_DSA(pkey) __EVP_PKEY_get1_DSA(AmiSSLBase, (pkey))

int __EVP_PKEY_set1_DH(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("a1") struct dh_st * key)="\tjsr\t-5022(a6)";
#define EVP_PKEY_set1_DH(pkey, key) __EVP_PKEY_set1_DH(AmiSSLBase, (pkey), (key))

struct dh_st * __EVP_PKEY_get1_DH(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-5028(a6)";
#define EVP_PKEY_get1_DH(pkey) __EVP_PKEY_get1_DH(AmiSSLBase, (pkey))

EVP_PKEY * __EVP_PKEY_new(__reg("a6") struct Library *)="\tjsr\t-5034(a6)";
#define EVP_PKEY_new() __EVP_PKEY_new(AmiSSLBase)

void __EVP_PKEY_free(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-5040(a6)";
#define EVP_PKEY_free(pkey) __EVP_PKEY_free(AmiSSLBase, (pkey))

EVP_PKEY * __d2i_PublicKey(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") EVP_PKEY ** a, __reg("a1") unsigned char ** pp, __reg("d1") long length)="\tjsr\t-5046(a6)";
#define d2i_PublicKey(type, a, pp, length) __d2i_PublicKey(AmiSSLBase, (type), (a), (pp), (length))

int __i2d_PublicKey(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * a, __reg("a1") unsigned char ** pp)="\tjsr\t-5052(a6)";
#define i2d_PublicKey(a, pp) __i2d_PublicKey(AmiSSLBase, (a), (pp))

EVP_PKEY * __d2i_PrivateKey(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") EVP_PKEY ** a, __reg("a1") unsigned char ** pp, __reg("d1") long length)="\tjsr\t-5058(a6)";
#define d2i_PrivateKey(type, a, pp, length) __d2i_PrivateKey(AmiSSLBase, (type), (a), (pp), (length))

EVP_PKEY * __d2i_AutoPrivateKey(__reg("a6") struct Library *, __reg("a0") EVP_PKEY ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-5064(a6)";
#define d2i_AutoPrivateKey(a, pp, length) __d2i_AutoPrivateKey(AmiSSLBase, (a), (pp), (length))

int __i2d_PrivateKey(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * a, __reg("a1") unsigned char ** pp)="\tjsr\t-5070(a6)";
#define i2d_PrivateKey(a, pp) __i2d_PrivateKey(AmiSSLBase, (a), (pp))

int __EVP_PKEY_copy_parameters(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * to, __reg("a1") EVP_PKEY * from)="\tjsr\t-5076(a6)";
#define EVP_PKEY_copy_parameters(to, from) __EVP_PKEY_copy_parameters(AmiSSLBase, (to), (from))

int __EVP_PKEY_missing_parameters(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-5082(a6)";
#define EVP_PKEY_missing_parameters(pkey) __EVP_PKEY_missing_parameters(AmiSSLBase, (pkey))

int __EVP_PKEY_save_parameters(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("d0") LONG mode)="\tjsr\t-5088(a6)";
#define EVP_PKEY_save_parameters(pkey, mode) __EVP_PKEY_save_parameters(AmiSSLBase, (pkey), (mode))

int __EVP_PKEY_cmp_parameters(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * a, __reg("a1") EVP_PKEY * b)="\tjsr\t-5094(a6)";
#define EVP_PKEY_cmp_parameters(a, b) __EVP_PKEY_cmp_parameters(AmiSSLBase, (a), (b))

int __EVP_CIPHER_type(__reg("a6") struct Library *, __reg("a0") const EVP_CIPHER * ctx)="\tjsr\t-5100(a6)";
#define EVP_CIPHER_type(ctx) __EVP_CIPHER_type(AmiSSLBase, (ctx))

int __EVP_CIPHER_param_to_asn1(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * c, __reg("a1") ASN1_TYPE * type)="\tjsr\t-5106(a6)";
#define EVP_CIPHER_param_to_asn1(c, type) __EVP_CIPHER_param_to_asn1(AmiSSLBase, (c), (type))

int __EVP_CIPHER_asn1_to_param(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * c, __reg("a1") ASN1_TYPE * type)="\tjsr\t-5112(a6)";
#define EVP_CIPHER_asn1_to_param(c, type) __EVP_CIPHER_asn1_to_param(AmiSSLBase, (c), (type))

int __EVP_CIPHER_set_asn1_iv(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * c, __reg("a1") ASN1_TYPE * type)="\tjsr\t-5118(a6)";
#define EVP_CIPHER_set_asn1_iv(c, type) __EVP_CIPHER_set_asn1_iv(AmiSSLBase, (c), (type))

int __EVP_CIPHER_get_asn1_iv(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * c, __reg("a1") ASN1_TYPE * type)="\tjsr\t-5124(a6)";
#define EVP_CIPHER_get_asn1_iv(c, type) __EVP_CIPHER_get_asn1_iv(AmiSSLBase, (c), (type))

int __PKCS5_PBE_keyivgen(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") ASN1_TYPE * param, __reg("a3") const EVP_CIPHER * cipher, __reg("d1") const EVP_MD * md, __reg("d2") LONG en_de)="\tjsr\t-5130(a6)";
#define PKCS5_PBE_keyivgen(ctx, pass, passlen, param, cipher, md, en_de) __PKCS5_PBE_keyivgen(AmiSSLBase, (ctx), (pass), (passlen), (param), (cipher), (md), (en_de))

int __PKCS5_PBKDF2_HMAC_SHA1(__reg("a6") struct Library *, __reg("a0") const char * pass, __reg("d0") LONG passlen, __reg("a1") unsigned char * salt, __reg("d1") LONG saltlen, __reg("d2") LONG iter, __reg("d3") LONG keylen, __reg("a2") unsigned char * out)="\tjsr\t-5136(a6)";
#define PKCS5_PBKDF2_HMAC_SHA1(pass, passlen, salt, saltlen, iter, keylen, out) __PKCS5_PBKDF2_HMAC_SHA1(AmiSSLBase, (pass), (passlen), (salt), (saltlen), (iter), (keylen), (out))

int __PKCS5_v2_PBE_keyivgen(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") ASN1_TYPE * param, __reg("a3") const EVP_CIPHER * cipher, __reg("d1") const EVP_MD * md, __reg("d2") LONG en_de)="\tjsr\t-5142(a6)";
#define PKCS5_v2_PBE_keyivgen(ctx, pass, passlen, param, cipher, md, en_de) __PKCS5_v2_PBE_keyivgen(AmiSSLBase, (ctx), (pass), (passlen), (param), (cipher), (md), (en_de))

void __PKCS5_PBE_add(__reg("a6") struct Library *)="\tjsr\t-5148(a6)";
#define PKCS5_PBE_add() __PKCS5_PBE_add(AmiSSLBase)

int __EVP_PBE_CipherInit(__reg("a6") struct Library *, __reg("a0") ASN1_OBJECT * pbe_obj, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") ASN1_TYPE * param, __reg("a3") EVP_CIPHER_CTX * ctx, __reg("d1") LONG en_de)="\tjsr\t-5154(a6)";
#define EVP_PBE_CipherInit(pbe_obj, pass, passlen, param, ctx, en_de) __EVP_PBE_CipherInit(AmiSSLBase, (pbe_obj), (pass), (passlen), (param), (ctx), (en_de))

int __EVP_PBE_alg_add(__reg("a6") struct Library *, __reg("d0") LONG nid, __reg("a0") const EVP_CIPHER * cipher, __reg("a1") const EVP_MD * md, __reg("a2") EVP_PBE_KEYGEN * (*keygen)(struct evp_cipher_ctx_st *ctx, const char *pass, int passlen, struct asn1_type_st *param, struct evp_cipher_st *cipher, struct env_md_st *md, int en_de))="\tjsr\t-5160(a6)";
#define EVP_PBE_alg_add(nid, cipher, md, keygen) __EVP_PBE_alg_add(AmiSSLBase, (nid), (cipher), (md), (keygen))

void __EVP_PBE_cleanup(__reg("a6") struct Library *)="\tjsr\t-5166(a6)";
#define EVP_PBE_cleanup() __EVP_PBE_cleanup(AmiSSLBase)

void __ERR_load_EVP_strings(__reg("a6") struct Library *)="\tjsr\t-5172(a6)";
#define ERR_load_EVP_strings() __ERR_load_EVP_strings(AmiSSLBase)

void __HMAC_CTX_init(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx)="\tjsr\t-5178(a6)";
#define HMAC_CTX_init(ctx) __HMAC_CTX_init(AmiSSLBase, (ctx))

void __HMAC_CTX_cleanup(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx)="\tjsr\t-5184(a6)";
#define HMAC_CTX_cleanup(ctx) __HMAC_CTX_cleanup(AmiSSLBase, (ctx))

void __HMAC_Init(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx, __reg("a1") const void * key, __reg("d0") LONG len, __reg("a2") const EVP_MD * md)="\tjsr\t-5190(a6)";
#define HMAC_Init(ctx, key, len, md) __HMAC_Init(AmiSSLBase, (ctx), (key), (len), (md))

void __HMAC_Init_ex(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx, __reg("a1") const void * key, __reg("d0") LONG len, __reg("a2") const EVP_MD * md, __reg("a3") ENGINE * impl)="\tjsr\t-5196(a6)";
#define HMAC_Init_ex(ctx, key, len, md, impl) __HMAC_Init_ex(AmiSSLBase, (ctx), (key), (len), (md), (impl))

void __HMAC_Update(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx, __reg("a1") const unsigned char * data, __reg("d0") LONG len)="\tjsr\t-5202(a6)";
#define HMAC_Update(ctx, data, len) __HMAC_Update(AmiSSLBase, (ctx), (data), (len))

void __HMAC_Final(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx, __reg("a1") unsigned char * md, __reg("a2") unsigned int * len)="\tjsr\t-5208(a6)";
#define HMAC_Final(ctx, md, len) __HMAC_Final(AmiSSLBase, (ctx), (md), (len))

unsigned char * __HMAC(__reg("a6") struct Library *, __reg("a0") const EVP_MD * evp_md, __reg("a1") const void * key, __reg("d0") LONG key_len, __reg("a2") const unsigned char * d, __reg("d1") LONG n, __reg("a3") unsigned char * md, __reg("d2") unsigned int * md_len)="\tjsr\t-5214(a6)";
#define HMAC(evp_md, key, key_len, d, n, md, md_len) __HMAC(AmiSSLBase, (evp_md), (key), (key_len), (d), (n), (md), (md_len))

KRB5_ENCDATA * __KRB5_ENCDATA_new(__reg("a6") struct Library *)="\tjsr\t-5220(a6)";
#define KRB5_ENCDATA_new() __KRB5_ENCDATA_new(AmiSSLBase)

void __KRB5_ENCDATA_free(__reg("a6") struct Library *, __reg("a0") KRB5_ENCDATA * a)="\tjsr\t-5226(a6)";
#define KRB5_ENCDATA_free(a) __KRB5_ENCDATA_free(AmiSSLBase, (a))

KRB5_ENCDATA * __d2i_KRB5_ENCDATA(__reg("a6") struct Library *, __reg("a0") KRB5_ENCDATA ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5232(a6)";
#define d2i_KRB5_ENCDATA(a, in, len) __d2i_KRB5_ENCDATA(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_ENCDATA(__reg("a6") struct Library *, __reg("a0") KRB5_ENCDATA * a, __reg("a1") unsigned char ** out)="\tjsr\t-5238(a6)";
#define i2d_KRB5_ENCDATA(a, out) __i2d_KRB5_ENCDATA(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_ENCDATA_it(__reg("a6") struct Library *)="\tjsr\t-5244(a6)";
#define KRB5_ENCDATA_it() __KRB5_ENCDATA_it(AmiSSLBase)

KRB5_PRINCNAME * __KRB5_PRINCNAME_new(__reg("a6") struct Library *)="\tjsr\t-5250(a6)";
#define KRB5_PRINCNAME_new() __KRB5_PRINCNAME_new(AmiSSLBase)

void __KRB5_PRINCNAME_free(__reg("a6") struct Library *, __reg("a0") KRB5_PRINCNAME * a)="\tjsr\t-5256(a6)";
#define KRB5_PRINCNAME_free(a) __KRB5_PRINCNAME_free(AmiSSLBase, (a))

KRB5_PRINCNAME * __d2i_KRB5_PRINCNAME(__reg("a6") struct Library *, __reg("a0") KRB5_PRINCNAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5262(a6)";
#define d2i_KRB5_PRINCNAME(a, in, len) __d2i_KRB5_PRINCNAME(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_PRINCNAME(__reg("a6") struct Library *, __reg("a0") KRB5_PRINCNAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-5268(a6)";
#define i2d_KRB5_PRINCNAME(a, out) __i2d_KRB5_PRINCNAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_PRINCNAME_it(__reg("a6") struct Library *)="\tjsr\t-5274(a6)";
#define KRB5_PRINCNAME_it() __KRB5_PRINCNAME_it(AmiSSLBase)

KRB5_TKTBODY * __KRB5_TKTBODY_new(__reg("a6") struct Library *)="\tjsr\t-5280(a6)";
#define KRB5_TKTBODY_new() __KRB5_TKTBODY_new(AmiSSLBase)

void __KRB5_TKTBODY_free(__reg("a6") struct Library *, __reg("a0") KRB5_TKTBODY * a)="\tjsr\t-5286(a6)";
#define KRB5_TKTBODY_free(a) __KRB5_TKTBODY_free(AmiSSLBase, (a))

KRB5_TKTBODY * __d2i_KRB5_TKTBODY(__reg("a6") struct Library *, __reg("a0") KRB5_TKTBODY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5292(a6)";
#define d2i_KRB5_TKTBODY(a, in, len) __d2i_KRB5_TKTBODY(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_TKTBODY(__reg("a6") struct Library *, __reg("a0") KRB5_TKTBODY * a, __reg("a1") unsigned char ** out)="\tjsr\t-5298(a6)";
#define i2d_KRB5_TKTBODY(a, out) __i2d_KRB5_TKTBODY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_TKTBODY_it(__reg("a6") struct Library *)="\tjsr\t-5304(a6)";
#define KRB5_TKTBODY_it() __KRB5_TKTBODY_it(AmiSSLBase)

KRB5_APREQBODY * __KRB5_APREQBODY_new(__reg("a6") struct Library *)="\tjsr\t-5310(a6)";
#define KRB5_APREQBODY_new() __KRB5_APREQBODY_new(AmiSSLBase)

void __KRB5_APREQBODY_free(__reg("a6") struct Library *, __reg("a0") KRB5_APREQBODY * a)="\tjsr\t-5316(a6)";
#define KRB5_APREQBODY_free(a) __KRB5_APREQBODY_free(AmiSSLBase, (a))

KRB5_APREQBODY * __d2i_KRB5_APREQBODY(__reg("a6") struct Library *, __reg("a0") KRB5_APREQBODY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5322(a6)";
#define d2i_KRB5_APREQBODY(a, in, len) __d2i_KRB5_APREQBODY(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_APREQBODY(__reg("a6") struct Library *, __reg("a0") KRB5_APREQBODY * a, __reg("a1") unsigned char ** out)="\tjsr\t-5328(a6)";
#define i2d_KRB5_APREQBODY(a, out) __i2d_KRB5_APREQBODY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_APREQBODY_it(__reg("a6") struct Library *)="\tjsr\t-5334(a6)";
#define KRB5_APREQBODY_it() __KRB5_APREQBODY_it(AmiSSLBase)

KRB5_TICKET * __KRB5_TICKET_new(__reg("a6") struct Library *)="\tjsr\t-5340(a6)";
#define KRB5_TICKET_new() __KRB5_TICKET_new(AmiSSLBase)

void __KRB5_TICKET_free(__reg("a6") struct Library *, __reg("a0") KRB5_TICKET * a)="\tjsr\t-5346(a6)";
#define KRB5_TICKET_free(a) __KRB5_TICKET_free(AmiSSLBase, (a))

KRB5_TICKET * __d2i_KRB5_TICKET(__reg("a6") struct Library *, __reg("a0") KRB5_TICKET ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5352(a6)";
#define d2i_KRB5_TICKET(a, in, len) __d2i_KRB5_TICKET(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_TICKET(__reg("a6") struct Library *, __reg("a0") KRB5_TICKET * a, __reg("a1") unsigned char ** out)="\tjsr\t-5358(a6)";
#define i2d_KRB5_TICKET(a, out) __i2d_KRB5_TICKET(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_TICKET_it(__reg("a6") struct Library *)="\tjsr\t-5364(a6)";
#define KRB5_TICKET_it() __KRB5_TICKET_it(AmiSSLBase)

KRB5_APREQ * __KRB5_APREQ_new(__reg("a6") struct Library *)="\tjsr\t-5370(a6)";
#define KRB5_APREQ_new() __KRB5_APREQ_new(AmiSSLBase)

void __KRB5_APREQ_free(__reg("a6") struct Library *, __reg("a0") KRB5_APREQ * a)="\tjsr\t-5376(a6)";
#define KRB5_APREQ_free(a) __KRB5_APREQ_free(AmiSSLBase, (a))

KRB5_APREQ * __d2i_KRB5_APREQ(__reg("a6") struct Library *, __reg("a0") KRB5_APREQ ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5382(a6)";
#define d2i_KRB5_APREQ(a, in, len) __d2i_KRB5_APREQ(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_APREQ(__reg("a6") struct Library *, __reg("a0") KRB5_APREQ * a, __reg("a1") unsigned char ** out)="\tjsr\t-5388(a6)";
#define i2d_KRB5_APREQ(a, out) __i2d_KRB5_APREQ(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_APREQ_it(__reg("a6") struct Library *)="\tjsr\t-5394(a6)";
#define KRB5_APREQ_it() __KRB5_APREQ_it(AmiSSLBase)

KRB5_CHECKSUM * __KRB5_CHECKSUM_new(__reg("a6") struct Library *)="\tjsr\t-5400(a6)";
#define KRB5_CHECKSUM_new() __KRB5_CHECKSUM_new(AmiSSLBase)

void __KRB5_CHECKSUM_free(__reg("a6") struct Library *, __reg("a0") KRB5_CHECKSUM * a)="\tjsr\t-5406(a6)";
#define KRB5_CHECKSUM_free(a) __KRB5_CHECKSUM_free(AmiSSLBase, (a))

KRB5_CHECKSUM * __d2i_KRB5_CHECKSUM(__reg("a6") struct Library *, __reg("a0") KRB5_CHECKSUM ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5412(a6)";
#define d2i_KRB5_CHECKSUM(a, in, len) __d2i_KRB5_CHECKSUM(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_CHECKSUM(__reg("a6") struct Library *, __reg("a0") KRB5_CHECKSUM * a, __reg("a1") unsigned char ** out)="\tjsr\t-5418(a6)";
#define i2d_KRB5_CHECKSUM(a, out) __i2d_KRB5_CHECKSUM(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_CHECKSUM_it(__reg("a6") struct Library *)="\tjsr\t-5424(a6)";
#define KRB5_CHECKSUM_it() __KRB5_CHECKSUM_it(AmiSSLBase)

KRB5_ENCKEY * __KRB5_ENCKEY_new(__reg("a6") struct Library *)="\tjsr\t-5430(a6)";
#define KRB5_ENCKEY_new() __KRB5_ENCKEY_new(AmiSSLBase)

void __KRB5_ENCKEY_free(__reg("a6") struct Library *, __reg("a0") KRB5_ENCKEY * a)="\tjsr\t-5436(a6)";
#define KRB5_ENCKEY_free(a) __KRB5_ENCKEY_free(AmiSSLBase, (a))

KRB5_ENCKEY * __d2i_KRB5_ENCKEY(__reg("a6") struct Library *, __reg("a0") KRB5_ENCKEY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5442(a6)";
#define d2i_KRB5_ENCKEY(a, in, len) __d2i_KRB5_ENCKEY(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_ENCKEY(__reg("a6") struct Library *, __reg("a0") KRB5_ENCKEY * a, __reg("a1") unsigned char ** out)="\tjsr\t-5448(a6)";
#define i2d_KRB5_ENCKEY(a, out) __i2d_KRB5_ENCKEY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_ENCKEY_it(__reg("a6") struct Library *)="\tjsr\t-5454(a6)";
#define KRB5_ENCKEY_it() __KRB5_ENCKEY_it(AmiSSLBase)

KRB5_AUTHDATA * __KRB5_AUTHDATA_new(__reg("a6") struct Library *)="\tjsr\t-5460(a6)";
#define KRB5_AUTHDATA_new() __KRB5_AUTHDATA_new(AmiSSLBase)

void __KRB5_AUTHDATA_free(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHDATA * a)="\tjsr\t-5466(a6)";
#define KRB5_AUTHDATA_free(a) __KRB5_AUTHDATA_free(AmiSSLBase, (a))

KRB5_AUTHDATA * __d2i_KRB5_AUTHDATA(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHDATA ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5472(a6)";
#define d2i_KRB5_AUTHDATA(a, in, len) __d2i_KRB5_AUTHDATA(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_AUTHDATA(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHDATA * a, __reg("a1") unsigned char ** out)="\tjsr\t-5478(a6)";
#define i2d_KRB5_AUTHDATA(a, out) __i2d_KRB5_AUTHDATA(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_AUTHDATA_it(__reg("a6") struct Library *)="\tjsr\t-5484(a6)";
#define KRB5_AUTHDATA_it() __KRB5_AUTHDATA_it(AmiSSLBase)

KRB5_AUTHENTBODY * __KRB5_AUTHENTBODY_new(__reg("a6") struct Library *)="\tjsr\t-5490(a6)";
#define KRB5_AUTHENTBODY_new() __KRB5_AUTHENTBODY_new(AmiSSLBase)

void __KRB5_AUTHENTBODY_free(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENTBODY * a)="\tjsr\t-5496(a6)";
#define KRB5_AUTHENTBODY_free(a) __KRB5_AUTHENTBODY_free(AmiSSLBase, (a))

KRB5_AUTHENTBODY * __d2i_KRB5_AUTHENTBODY(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENTBODY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5502(a6)";
#define d2i_KRB5_AUTHENTBODY(a, in, len) __d2i_KRB5_AUTHENTBODY(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_AUTHENTBODY(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENTBODY * a, __reg("a1") unsigned char ** out)="\tjsr\t-5508(a6)";
#define i2d_KRB5_AUTHENTBODY(a, out) __i2d_KRB5_AUTHENTBODY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_AUTHENTBODY_it(__reg("a6") struct Library *)="\tjsr\t-5514(a6)";
#define KRB5_AUTHENTBODY_it() __KRB5_AUTHENTBODY_it(AmiSSLBase)

KRB5_AUTHENT * __KRB5_AUTHENT_new(__reg("a6") struct Library *)="\tjsr\t-5520(a6)";
#define KRB5_AUTHENT_new() __KRB5_AUTHENT_new(AmiSSLBase)

void __KRB5_AUTHENT_free(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENT * a)="\tjsr\t-5526(a6)";
#define KRB5_AUTHENT_free(a) __KRB5_AUTHENT_free(AmiSSLBase, (a))

KRB5_AUTHENT * __d2i_KRB5_AUTHENT(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENT ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-5532(a6)";
#define d2i_KRB5_AUTHENT(a, in, len) __d2i_KRB5_AUTHENT(AmiSSLBase, (a), (in), (len))

int __i2d_KRB5_AUTHENT(__reg("a6") struct Library *, __reg("a0") KRB5_AUTHENT * a, __reg("a1") unsigned char ** out)="\tjsr\t-5538(a6)";
#define i2d_KRB5_AUTHENT(a, out) __i2d_KRB5_AUTHENT(AmiSSLBase, (a), (out))

const ASN1_ITEM * __KRB5_AUTHENT_it(__reg("a6") struct Library *)="\tjsr\t-5544(a6)";
#define KRB5_AUTHENT_it() __KRB5_AUTHENT_it(AmiSSLBase)

LHASH * __lh_new(__reg("a6") struct Library *, __reg("d0") LONG h, __reg("d1") LONG c)="\tjsr\t-5550(a6)";
#define lh_new(h, c) __lh_new(AmiSSLBase, (h), (c))

void __lh_free(__reg("a6") struct Library *, __reg("a0") LHASH * lh)="\tjsr\t-5556(a6)";
#define lh_free(lh) __lh_free(AmiSSLBase, (lh))

void * __lh_insert(__reg("a6") struct Library *, __reg("a0") LHASH * lh, __reg("a1") const void * data)="\tjsr\t-5562(a6)";
#define lh_insert(lh, data) __lh_insert(AmiSSLBase, (lh), (data))

void * __lh_delete(__reg("a6") struct Library *, __reg("a0") LHASH * lh, __reg("a1") const void * data)="\tjsr\t-5568(a6)";
#define lh_delete(lh, data) __lh_delete(AmiSSLBase, (lh), (data))

void * __lh_retrieve(__reg("a6") struct Library *, __reg("a0") LHASH * lh, __reg("a1") const void * data)="\tjsr\t-5574(a6)";
#define lh_retrieve(lh, data) __lh_retrieve(AmiSSLBase, (lh), (data))

void __lh_doall(__reg("a6") struct Library *, __reg("a0") LHASH * lh, __reg("d0") LONG func)="\tjsr\t-5580(a6)";
#define lh_doall(lh, func) __lh_doall(AmiSSLBase, (lh), (func))

void __lh_doall_arg(__reg("a6") struct Library *, __reg("a0") LHASH * lh, __reg("d0") LONG func, __reg("a1") void * arg)="\tjsr\t-5586(a6)";
#define lh_doall_arg(lh, func, arg) __lh_doall_arg(AmiSSLBase, (lh), (func), (arg))

unsigned long __lh_strhash(__reg("a6") struct Library *, __reg("a0") const char * c)="\tjsr\t-5592(a6)";
#define lh_strhash(c) __lh_strhash(AmiSSLBase, (c))

unsigned long __lh_num_items(__reg("a6") struct Library *, __reg("a0") const LHASH * lh)="\tjsr\t-5598(a6)";
#define lh_num_items(lh) __lh_num_items(AmiSSLBase, (lh))

void __lh_stats_bio(__reg("a6") struct Library *, __reg("a0") const LHASH * lh, __reg("a1") BIO * out)="\tjsr\t-5604(a6)";
#define lh_stats_bio(lh, out) __lh_stats_bio(AmiSSLBase, (lh), (out))

void __lh_node_stats_bio(__reg("a6") struct Library *, __reg("a0") const LHASH * lh, __reg("a1") BIO * out)="\tjsr\t-5610(a6)";
#define lh_node_stats_bio(lh, out) __lh_node_stats_bio(AmiSSLBase, (lh), (out))

void __lh_node_usage_stats_bio(__reg("a6") struct Library *, __reg("a0") const LHASH * lh, __reg("a1") BIO * out)="\tjsr\t-5616(a6)";
#define lh_node_usage_stats_bio(lh, out) __lh_node_usage_stats_bio(AmiSSLBase, (lh), (out))

int __OBJ_NAME_init(__reg("a6") struct Library *)="\tjsr\t-5622(a6)";
#define OBJ_NAME_init() __OBJ_NAME_init(AmiSSLBase)

int __OBJ_NAME_new_index(__reg("a6") struct Library *, __reg("a0") unsigned long (*hash_func)(const char *), __reg("a1") int (*cmp_func)(const char *, const char *), __reg("a2") void (*free_func)(const char *, int, const char *))="\tjsr\t-5628(a6)";
#define OBJ_NAME_new_index(hash_func, cmp_func, free_func) __OBJ_NAME_new_index(AmiSSLBase, (hash_func), (cmp_func), (free_func))

const char * __OBJ_NAME_get(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("d0") LONG type)="\tjsr\t-5634(a6)";
#define OBJ_NAME_get(name, type) __OBJ_NAME_get(AmiSSLBase, (name), (type))

int __OBJ_NAME_add(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("d0") LONG type, __reg("a1") const char * data)="\tjsr\t-5640(a6)";
#define OBJ_NAME_add(name, type, data) __OBJ_NAME_add(AmiSSLBase, (name), (type), (data))

int __OBJ_NAME_remove(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("d0") LONG type)="\tjsr\t-5646(a6)";
#define OBJ_NAME_remove(name, type) __OBJ_NAME_remove(AmiSSLBase, (name), (type))

void __OBJ_NAME_cleanup(__reg("a6") struct Library *, __reg("d0") LONG type)="\tjsr\t-5652(a6)";
#define OBJ_NAME_cleanup(type) __OBJ_NAME_cleanup(AmiSSLBase, (type))

void __OBJ_NAME_do_all(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") void (*fn)(const OBJ_NAME *, void *arg), __reg("a1") void * arg)="\tjsr\t-5658(a6)";
#define OBJ_NAME_do_all(type, fn, arg) __OBJ_NAME_do_all(AmiSSLBase, (type), (fn), (arg))

void __OBJ_NAME_do_all_sorted(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") void (*fn)(const OBJ_NAME *, void *arg), __reg("a1") void * arg)="\tjsr\t-5664(a6)";
#define OBJ_NAME_do_all_sorted(type, fn, arg) __OBJ_NAME_do_all_sorted(AmiSSLBase, (type), (fn), (arg))

ASN1_OBJECT * __OBJ_dup(__reg("a6") struct Library *, __reg("a0") const ASN1_OBJECT * o)="\tjsr\t-5670(a6)";
#define OBJ_dup(o) __OBJ_dup(AmiSSLBase, (o))

ASN1_OBJECT * __OBJ_nid2obj(__reg("a6") struct Library *, __reg("d0") LONG n)="\tjsr\t-5676(a6)";
#define OBJ_nid2obj(n) __OBJ_nid2obj(AmiSSLBase, (n))

const char * __OBJ_nid2ln(__reg("a6") struct Library *, __reg("d0") LONG n)="\tjsr\t-5682(a6)";
#define OBJ_nid2ln(n) __OBJ_nid2ln(AmiSSLBase, (n))

const char * __OBJ_nid2sn(__reg("a6") struct Library *, __reg("d0") LONG n)="\tjsr\t-5688(a6)";
#define OBJ_nid2sn(n) __OBJ_nid2sn(AmiSSLBase, (n))

int __OBJ_obj2nid(__reg("a6") struct Library *, __reg("a0") const ASN1_OBJECT * o)="\tjsr\t-5694(a6)";
#define OBJ_obj2nid(o) __OBJ_obj2nid(AmiSSLBase, (o))

ASN1_OBJECT * __OBJ_txt2obj(__reg("a6") struct Library *, __reg("a0") const char * s, __reg("d0") LONG no_name)="\tjsr\t-5700(a6)";
#define OBJ_txt2obj(s, no_name) __OBJ_txt2obj(AmiSSLBase, (s), (no_name))

int __OBJ_obj2txt(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG buf_len, __reg("a1") const ASN1_OBJECT * a, __reg("d1") LONG no_name)="\tjsr\t-5706(a6)";
#define OBJ_obj2txt(buf, buf_len, a, no_name) __OBJ_obj2txt(AmiSSLBase, (buf), (buf_len), (a), (no_name))

int __OBJ_txt2nid(__reg("a6") struct Library *, __reg("a0") const char * s)="\tjsr\t-5712(a6)";
#define OBJ_txt2nid(s) __OBJ_txt2nid(AmiSSLBase, (s))

int __OBJ_ln2nid(__reg("a6") struct Library *, __reg("a0") const char * s)="\tjsr\t-5718(a6)";
#define OBJ_ln2nid(s) __OBJ_ln2nid(AmiSSLBase, (s))

int __OBJ_sn2nid(__reg("a6") struct Library *, __reg("a0") const char * s)="\tjsr\t-5724(a6)";
#define OBJ_sn2nid(s) __OBJ_sn2nid(AmiSSLBase, (s))

int __OBJ_cmp(__reg("a6") struct Library *, __reg("a0") const ASN1_OBJECT * a, __reg("a1") const ASN1_OBJECT * b)="\tjsr\t-5730(a6)";
#define OBJ_cmp(a, b) __OBJ_cmp(AmiSSLBase, (a), (b))

const char * __OBJ_bsearch(__reg("a6") struct Library *, __reg("a0") const char * key, __reg("a1") const char * base, __reg("d0") LONG num, __reg("d1") LONG size, __reg("a2") int (*cmp)(const void *, const void *))="\tjsr\t-5736(a6)";
#define OBJ_bsearch(key, base, num, size, cmp) __OBJ_bsearch(AmiSSLBase, (key), (base), (num), (size), (cmp))

int __OBJ_new_nid(__reg("a6") struct Library *, __reg("d0") LONG num)="\tjsr\t-5742(a6)";
#define OBJ_new_nid(num) __OBJ_new_nid(AmiSSLBase, (num))

int __OBJ_add_object(__reg("a6") struct Library *, __reg("a0") const ASN1_OBJECT * obj)="\tjsr\t-5748(a6)";
#define OBJ_add_object(obj) __OBJ_add_object(AmiSSLBase, (obj))

int __OBJ_create(__reg("a6") struct Library *, __reg("a0") const char * oid, __reg("a1") const char * sn, __reg("a2") const char * ln)="\tjsr\t-5754(a6)";
#define OBJ_create(oid, sn, ln) __OBJ_create(AmiSSLBase, (oid), (sn), (ln))

void __OBJ_cleanup(__reg("a6") struct Library *)="\tjsr\t-5760(a6)";
#define OBJ_cleanup() __OBJ_cleanup(AmiSSLBase)

int __OBJ_create_objects(__reg("a6") struct Library *, __reg("a0") BIO * in)="\tjsr\t-5766(a6)";
#define OBJ_create_objects(in) __OBJ_create_objects(AmiSSLBase, (in))

void __ERR_load_OBJ_strings(__reg("a6") struct Library *)="\tjsr\t-5772(a6)";
#define ERR_load_OBJ_strings() __ERR_load_OBJ_strings(AmiSSLBase)

OCSP_RESPONSE * __OCSP_sendreq_bio(__reg("a6") struct Library *, __reg("a0") BIO * b, __reg("a1") char * path, __reg("a2") OCSP_REQUEST * req)="\tjsr\t-5778(a6)";
#define OCSP_sendreq_bio(b, path, req) __OCSP_sendreq_bio(AmiSSLBase, (b), (path), (req))

OCSP_CERTID * __OCSP_cert_to_id(__reg("a6") struct Library *, __reg("a0") const EVP_MD * dgst, __reg("a1") X509 * subject, __reg("a2") X509 * issuer)="\tjsr\t-5784(a6)";
#define OCSP_cert_to_id(dgst, subject, issuer) __OCSP_cert_to_id(AmiSSLBase, (dgst), (subject), (issuer))

OCSP_CERTID * __OCSP_cert_id_new(__reg("a6") struct Library *, __reg("a0") const EVP_MD * dgst, __reg("a1") X509_NAME * issuerName, __reg("a2") ASN1_BIT_STRING* issuerKey, __reg("a3") ASN1_INTEGER * serialNumber)="\tjsr\t-5790(a6)";
#define OCSP_cert_id_new(dgst, issuerName, issuerKey, serialNumber) __OCSP_cert_id_new(AmiSSLBase, (dgst), (issuerName), (issuerKey), (serialNumber))

OCSP_ONEREQ * __OCSP_request_add0_id(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") OCSP_CERTID * cid)="\tjsr\t-5796(a6)";
#define OCSP_request_add0_id(req, cid) __OCSP_request_add0_id(AmiSSLBase, (req), (cid))

int __OCSP_request_add1_nonce(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") unsigned char * val, __reg("d0") LONG len)="\tjsr\t-5802(a6)";
#define OCSP_request_add1_nonce(req, val, len) __OCSP_request_add1_nonce(AmiSSLBase, (req), (val), (len))

int __OCSP_basic_add1_nonce(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * resp, __reg("a1") unsigned char * val, __reg("d0") LONG len)="\tjsr\t-5808(a6)";
#define OCSP_basic_add1_nonce(resp, val, len) __OCSP_basic_add1_nonce(AmiSSLBase, (resp), (val), (len))

int __OCSP_check_nonce(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") OCSP_BASICRESP * bs)="\tjsr\t-5814(a6)";
#define OCSP_check_nonce(req, bs) __OCSP_check_nonce(AmiSSLBase, (req), (bs))

int __OCSP_copy_nonce(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * resp, __reg("a1") OCSP_REQUEST * req)="\tjsr\t-5820(a6)";
#define OCSP_copy_nonce(resp, req) __OCSP_copy_nonce(AmiSSLBase, (resp), (req))

int __OCSP_request_set1_name(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") X509_NAME * nm)="\tjsr\t-5826(a6)";
#define OCSP_request_set1_name(req, nm) __OCSP_request_set1_name(AmiSSLBase, (req), (nm))

int __OCSP_request_add1_cert(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") X509 * cert)="\tjsr\t-5832(a6)";
#define OCSP_request_add1_cert(req, cert) __OCSP_request_add1_cert(AmiSSLBase, (req), (cert))

int __OCSP_request_sign(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") X509 * signer, __reg("a2") EVP_PKEY * key, __reg("a3") const EVP_MD * dgst, __reg("d0") void * certs, __reg("d1") unsigned long flags)="\tjsr\t-5838(a6)";
#define OCSP_request_sign(req, signer, key, dgst, certs, flags) __OCSP_request_sign(AmiSSLBase, (req), (signer), (key), (dgst), (certs), (flags))

int __OCSP_response_status(__reg("a6") struct Library *, __reg("a0") OCSP_RESPONSE * resp)="\tjsr\t-5844(a6)";
#define OCSP_response_status(resp) __OCSP_response_status(AmiSSLBase, (resp))

OCSP_BASICRESP * __OCSP_response_get1_basic(__reg("a6") struct Library *, __reg("a0") OCSP_RESPONSE * resp)="\tjsr\t-5850(a6)";
#define OCSP_response_get1_basic(resp) __OCSP_response_get1_basic(AmiSSLBase, (resp))

int __OCSP_resp_count(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * bs)="\tjsr\t-5856(a6)";
#define OCSP_resp_count(bs) __OCSP_resp_count(AmiSSLBase, (bs))

OCSP_SINGLERESP * __OCSP_resp_get0(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * bs, __reg("d0") LONG idx)="\tjsr\t-5862(a6)";
#define OCSP_resp_get0(bs, idx) __OCSP_resp_get0(AmiSSLBase, (bs), (idx))

int __OCSP_resp_find(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * bs, __reg("a1") OCSP_CERTID * id, __reg("d0") LONG last)="\tjsr\t-5868(a6)";
#define OCSP_resp_find(bs, id, last) __OCSP_resp_find(AmiSSLBase, (bs), (id), (last))

int __OCSP_single_get0_status(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * single, __reg("a1") int * reason, __reg("a2") ASN1_GENERALIZEDTIME ** revtime, __reg("a3") ASN1_GENERALIZEDTIME ** thisupd, __reg("d0") ASN1_GENERALIZEDTIME ** nextupd)="\tjsr\t-5874(a6)";
#define OCSP_single_get0_status(single, reason, revtime, thisupd, nextupd) __OCSP_single_get0_status(AmiSSLBase, (single), (reason), (revtime), (thisupd), (nextupd))

int __OCSP_resp_find_status(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * bs, __reg("a1") OCSP_CERTID * id, __reg("a2") int * status, __reg("a3") int * reason, __reg("d0") ASN1_GENERALIZEDTIME ** revtime, __reg("d1") ASN1_GENERALIZEDTIME ** thisupd, __reg("d2") ASN1_GENERALIZEDTIME ** nextupd)="\tjsr\t-5880(a6)";
#define OCSP_resp_find_status(bs, id, status, reason, revtime, thisupd, nextupd) __OCSP_resp_find_status(AmiSSLBase, (bs), (id), (status), (reason), (revtime), (thisupd), (nextupd))

int __OCSP_check_validity(__reg("a6") struct Library *, __reg("a0") ASN1_GENERALIZEDTIME * thisupd, __reg("a1") ASN1_GENERALIZEDTIME * nextupd, __reg("d0") long sec, __reg("d1") long maxsec)="\tjsr\t-5886(a6)";
#define OCSP_check_validity(thisupd, nextupd, sec, maxsec) __OCSP_check_validity(AmiSSLBase, (thisupd), (nextupd), (sec), (maxsec))

int __OCSP_request_verify(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("a1") void * certs, __reg("a2") X509_STORE * store, __reg("d0") unsigned long flags)="\tjsr\t-5892(a6)";
#define OCSP_request_verify(req, certs, store, flags) __OCSP_request_verify(AmiSSLBase, (req), (certs), (store), (flags))

int __OCSP_parse_url(__reg("a6") struct Library *, __reg("a0") char * url, __reg("a1") char ** phost, __reg("a2") char ** pport, __reg("a3") char ** ppath, __reg("d0") int * pssl)="\tjsr\t-5898(a6)";
#define OCSP_parse_url(url, phost, pport, ppath, pssl) __OCSP_parse_url(AmiSSLBase, (url), (phost), (pport), (ppath), (pssl))

int __OCSP_id_issuer_cmp(__reg("a6") struct Library *, __reg("a0") OCSP_CERTID * a, __reg("a1") OCSP_CERTID * b)="\tjsr\t-5904(a6)";
#define OCSP_id_issuer_cmp(a, b) __OCSP_id_issuer_cmp(AmiSSLBase, (a), (b))

int __OCSP_id_cmp(__reg("a6") struct Library *, __reg("a0") OCSP_CERTID * a, __reg("a1") OCSP_CERTID * b)="\tjsr\t-5910(a6)";
#define OCSP_id_cmp(a, b) __OCSP_id_cmp(AmiSSLBase, (a), (b))

int __OCSP_request_onereq_count(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req)="\tjsr\t-5916(a6)";
#define OCSP_request_onereq_count(req) __OCSP_request_onereq_count(AmiSSLBase, (req))

OCSP_ONEREQ * __OCSP_request_onereq_get0(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req, __reg("d0") LONG i)="\tjsr\t-5922(a6)";
#define OCSP_request_onereq_get0(req, i) __OCSP_request_onereq_get0(AmiSSLBase, (req), (i))

OCSP_CERTID * __OCSP_onereq_get0_id(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * one)="\tjsr\t-5928(a6)";
#define OCSP_onereq_get0_id(one) __OCSP_onereq_get0_id(AmiSSLBase, (one))

int __OCSP_id_get0_info(__reg("a6") struct Library *, __reg("a0") ASN1_OCTET_STRING ** piNameHash, __reg("a1") ASN1_OBJECT ** pmd, __reg("a2") ASN1_OCTET_STRING ** pikeyHash, __reg("a3") ASN1_INTEGER ** pserial, __reg("d0") OCSP_CERTID * cid)="\tjsr\t-5934(a6)";
#define OCSP_id_get0_info(piNameHash, pmd, pikeyHash, pserial, cid) __OCSP_id_get0_info(AmiSSLBase, (piNameHash), (pmd), (pikeyHash), (pserial), (cid))

int __OCSP_request_is_signed(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * req)="\tjsr\t-5940(a6)";
#define OCSP_request_is_signed(req) __OCSP_request_is_signed(AmiSSLBase, (req))

OCSP_RESPONSE * __OCSP_response_create(__reg("a6") struct Library *, __reg("d0") LONG status, __reg("a0") OCSP_BASICRESP * bs)="\tjsr\t-5946(a6)";
#define OCSP_response_create(status, bs) __OCSP_response_create(AmiSSLBase, (status), (bs))

OCSP_SINGLERESP * __OCSP_basic_add1_status(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * rsp, __reg("a1") OCSP_CERTID * cid, __reg("d0") LONG status, __reg("d1") LONG reason, __reg("a2") ASN1_TIME * revtime, __reg("a3") ASN1_TIME * thisupd, __reg("d2") ASN1_TIME * nextupd)="\tjsr\t-5952(a6)";
#define OCSP_basic_add1_status(rsp, cid, status, reason, revtime, thisupd, nextupd) __OCSP_basic_add1_status(AmiSSLBase, (rsp), (cid), (status), (reason), (revtime), (thisupd), (nextupd))

int __OCSP_basic_add1_cert(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * resp, __reg("a1") X509 * cert)="\tjsr\t-5958(a6)";
#define OCSP_basic_add1_cert(resp, cert) __OCSP_basic_add1_cert(AmiSSLBase, (resp), (cert))

int __OCSP_basic_sign(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * brsp, __reg("a1") X509 * signer, __reg("a2") EVP_PKEY * key, __reg("a3") const EVP_MD * dgst, __reg("d0") void * certs, __reg("d1") unsigned long flags)="\tjsr\t-5964(a6)";
#define OCSP_basic_sign(brsp, signer, key, dgst, certs, flags) __OCSP_basic_sign(AmiSSLBase, (brsp), (signer), (key), (dgst), (certs), (flags))

ASN1_STRING * __ASN1_STRING_encode(__reg("a6") struct Library *, __reg("a0") ASN1_STRING * s, __reg("a1") int (*i2d)(), __reg("a2") char * data, __reg("a3") void * sk)="\tjsr\t-5970(a6)";
#define ASN1_STRING_encode(s, i2d, data, sk) __ASN1_STRING_encode(AmiSSLBase, (s), (i2d), (data), (sk))

X509_EXTENSION * __OCSP_crlID_new(__reg("a6") struct Library *, __reg("a0") char * url, __reg("a1") long * n, __reg("a2") char * tim)="\tjsr\t-5976(a6)";
#define OCSP_crlID_new(url, n, tim) __OCSP_crlID_new(AmiSSLBase, (url), (n), (tim))

X509_EXTENSION * __OCSP_accept_responses_new(__reg("a6") struct Library *, __reg("a0") char ** oids)="\tjsr\t-5982(a6)";
#define OCSP_accept_responses_new(oids) __OCSP_accept_responses_new(AmiSSLBase, (oids))

X509_EXTENSION * __OCSP_archive_cutoff_new(__reg("a6") struct Library *, __reg("a0") char* tim)="\tjsr\t-5988(a6)";
#define OCSP_archive_cutoff_new(tim) __OCSP_archive_cutoff_new(AmiSSLBase, (tim))

X509_EXTENSION * __OCSP_url_svcloc_new(__reg("a6") struct Library *, __reg("a0") X509_NAME* issuer, __reg("a1") char ** urls)="\tjsr\t-5994(a6)";
#define OCSP_url_svcloc_new(issuer, urls) __OCSP_url_svcloc_new(AmiSSLBase, (issuer), (urls))

int __OCSP_REQUEST_get_ext_count(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x)="\tjsr\t-6000(a6)";
#define OCSP_REQUEST_get_ext_count(x) __OCSP_REQUEST_get_ext_count(AmiSSLBase, (x))

int __OCSP_REQUEST_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-6006(a6)";
#define OCSP_REQUEST_get_ext_by_NID(x, nid, lastpos) __OCSP_REQUEST_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __OCSP_REQUEST_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-6012(a6)";
#define OCSP_REQUEST_get_ext_by_OBJ(x, obj, lastpos) __OCSP_REQUEST_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __OCSP_REQUEST_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-6018(a6)";
#define OCSP_REQUEST_get_ext_by_critical(x, crit, lastpos) __OCSP_REQUEST_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __OCSP_REQUEST_get_ext(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG loc)="\tjsr\t-6024(a6)";
#define OCSP_REQUEST_get_ext(x, loc) __OCSP_REQUEST_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __OCSP_REQUEST_delete_ext(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG loc)="\tjsr\t-6030(a6)";
#define OCSP_REQUEST_delete_ext(x, loc) __OCSP_REQUEST_delete_ext(AmiSSLBase, (x), (loc))

void * __OCSP_REQUEST_get1_ext_d2i(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-6036(a6)";
#define OCSP_REQUEST_get1_ext_d2i(x, nid, crit, idx) __OCSP_REQUEST_get1_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __OCSP_REQUEST_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-6042(a6)";
#define OCSP_REQUEST_add1_ext_i2d(x, nid, value, crit, flags) __OCSP_REQUEST_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __OCSP_REQUEST_add_ext(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-6048(a6)";
#define OCSP_REQUEST_add_ext(x, ex, loc) __OCSP_REQUEST_add_ext(AmiSSLBase, (x), (ex), (loc))

int __OCSP_ONEREQ_get_ext_count(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x)="\tjsr\t-6054(a6)";
#define OCSP_ONEREQ_get_ext_count(x) __OCSP_ONEREQ_get_ext_count(AmiSSLBase, (x))

int __OCSP_ONEREQ_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-6060(a6)";
#define OCSP_ONEREQ_get_ext_by_NID(x, nid, lastpos) __OCSP_ONEREQ_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __OCSP_ONEREQ_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-6066(a6)";
#define OCSP_ONEREQ_get_ext_by_OBJ(x, obj, lastpos) __OCSP_ONEREQ_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __OCSP_ONEREQ_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-6072(a6)";
#define OCSP_ONEREQ_get_ext_by_critical(x, crit, lastpos) __OCSP_ONEREQ_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __OCSP_ONEREQ_get_ext(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG loc)="\tjsr\t-6078(a6)";
#define OCSP_ONEREQ_get_ext(x, loc) __OCSP_ONEREQ_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __OCSP_ONEREQ_delete_ext(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG loc)="\tjsr\t-6084(a6)";
#define OCSP_ONEREQ_delete_ext(x, loc) __OCSP_ONEREQ_delete_ext(AmiSSLBase, (x), (loc))

void * __OCSP_ONEREQ_get1_ext_d2i(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-6090(a6)";
#define OCSP_ONEREQ_get1_ext_d2i(x, nid, crit, idx) __OCSP_ONEREQ_get1_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __OCSP_ONEREQ_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-6096(a6)";
#define OCSP_ONEREQ_add1_ext_i2d(x, nid, value, crit, flags) __OCSP_ONEREQ_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __OCSP_ONEREQ_add_ext(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-6102(a6)";
#define OCSP_ONEREQ_add_ext(x, ex, loc) __OCSP_ONEREQ_add_ext(AmiSSLBase, (x), (ex), (loc))

int __OCSP_BASICRESP_get_ext_count(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x)="\tjsr\t-6108(a6)";
#define OCSP_BASICRESP_get_ext_count(x) __OCSP_BASICRESP_get_ext_count(AmiSSLBase, (x))

int __OCSP_BASICRESP_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-6114(a6)";
#define OCSP_BASICRESP_get_ext_by_NID(x, nid, lastpos) __OCSP_BASICRESP_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __OCSP_BASICRESP_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-6120(a6)";
#define OCSP_BASICRESP_get_ext_by_OBJ(x, obj, lastpos) __OCSP_BASICRESP_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __OCSP_BASICRESP_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-6126(a6)";
#define OCSP_BASICRESP_get_ext_by_critical(x, crit, lastpos) __OCSP_BASICRESP_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __OCSP_BASICRESP_get_ext(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG loc)="\tjsr\t-6132(a6)";
#define OCSP_BASICRESP_get_ext(x, loc) __OCSP_BASICRESP_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __OCSP_BASICRESP_delete_ext(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG loc)="\tjsr\t-6138(a6)";
#define OCSP_BASICRESP_delete_ext(x, loc) __OCSP_BASICRESP_delete_ext(AmiSSLBase, (x), (loc))

void * __OCSP_BASICRESP_get1_ext_d2i(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-6144(a6)";
#define OCSP_BASICRESP_get1_ext_d2i(x, nid, crit, idx) __OCSP_BASICRESP_get1_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __OCSP_BASICRESP_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-6150(a6)";
#define OCSP_BASICRESP_add1_ext_i2d(x, nid, value, crit, flags) __OCSP_BASICRESP_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __OCSP_BASICRESP_add_ext(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-6156(a6)";
#define OCSP_BASICRESP_add_ext(x, ex, loc) __OCSP_BASICRESP_add_ext(AmiSSLBase, (x), (ex), (loc))

int __OCSP_SINGLERESP_get_ext_count(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x)="\tjsr\t-6162(a6)";
#define OCSP_SINGLERESP_get_ext_count(x) __OCSP_SINGLERESP_get_ext_count(AmiSSLBase, (x))

int __OCSP_SINGLERESP_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-6168(a6)";
#define OCSP_SINGLERESP_get_ext_by_NID(x, nid, lastpos) __OCSP_SINGLERESP_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __OCSP_SINGLERESP_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-6174(a6)";
#define OCSP_SINGLERESP_get_ext_by_OBJ(x, obj, lastpos) __OCSP_SINGLERESP_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __OCSP_SINGLERESP_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-6180(a6)";
#define OCSP_SINGLERESP_get_ext_by_critical(x, crit, lastpos) __OCSP_SINGLERESP_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __OCSP_SINGLERESP_get_ext(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG loc)="\tjsr\t-6186(a6)";
#define OCSP_SINGLERESP_get_ext(x, loc) __OCSP_SINGLERESP_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __OCSP_SINGLERESP_delete_ext(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG loc)="\tjsr\t-6192(a6)";
#define OCSP_SINGLERESP_delete_ext(x, loc) __OCSP_SINGLERESP_delete_ext(AmiSSLBase, (x), (loc))

void * __OCSP_SINGLERESP_get1_ext_d2i(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-6198(a6)";
#define OCSP_SINGLERESP_get1_ext_d2i(x, nid, crit, idx) __OCSP_SINGLERESP_get1_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __OCSP_SINGLERESP_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-6204(a6)";
#define OCSP_SINGLERESP_add1_ext_i2d(x, nid, value, crit, flags) __OCSP_SINGLERESP_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __OCSP_SINGLERESP_add_ext(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-6210(a6)";
#define OCSP_SINGLERESP_add_ext(x, ex, loc) __OCSP_SINGLERESP_add_ext(AmiSSLBase, (x), (ex), (loc))

OCSP_SINGLERESP * __OCSP_SINGLERESP_new(__reg("a6") struct Library *)="\tjsr\t-6216(a6)";
#define OCSP_SINGLERESP_new() __OCSP_SINGLERESP_new(AmiSSLBase)

void __OCSP_SINGLERESP_free(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * a)="\tjsr\t-6222(a6)";
#define OCSP_SINGLERESP_free(a) __OCSP_SINGLERESP_free(AmiSSLBase, (a))

OCSP_SINGLERESP * __d2i_OCSP_SINGLERESP(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6228(a6)";
#define d2i_OCSP_SINGLERESP(a, in, len) __d2i_OCSP_SINGLERESP(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_SINGLERESP(__reg("a6") struct Library *, __reg("a0") OCSP_SINGLERESP * a, __reg("a1") unsigned char ** out)="\tjsr\t-6234(a6)";
#define i2d_OCSP_SINGLERESP(a, out) __i2d_OCSP_SINGLERESP(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_SINGLERESP_it(__reg("a6") struct Library *)="\tjsr\t-6240(a6)";
#define OCSP_SINGLERESP_it() __OCSP_SINGLERESP_it(AmiSSLBase)

OCSP_CERTSTATUS * __OCSP_CERTSTATUS_new(__reg("a6") struct Library *)="\tjsr\t-6246(a6)";
#define OCSP_CERTSTATUS_new() __OCSP_CERTSTATUS_new(AmiSSLBase)

void __OCSP_CERTSTATUS_free(__reg("a6") struct Library *, __reg("a0") OCSP_CERTSTATUS * a)="\tjsr\t-6252(a6)";
#define OCSP_CERTSTATUS_free(a) __OCSP_CERTSTATUS_free(AmiSSLBase, (a))

OCSP_CERTSTATUS * __d2i_OCSP_CERTSTATUS(__reg("a6") struct Library *, __reg("a0") OCSP_CERTSTATUS ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6258(a6)";
#define d2i_OCSP_CERTSTATUS(a, in, len) __d2i_OCSP_CERTSTATUS(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_CERTSTATUS(__reg("a6") struct Library *, __reg("a0") OCSP_CERTSTATUS * a, __reg("a1") unsigned char ** out)="\tjsr\t-6264(a6)";
#define i2d_OCSP_CERTSTATUS(a, out) __i2d_OCSP_CERTSTATUS(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_CERTSTATUS_it(__reg("a6") struct Library *)="\tjsr\t-6270(a6)";
#define OCSP_CERTSTATUS_it() __OCSP_CERTSTATUS_it(AmiSSLBase)

OCSP_REVOKEDINFO * __OCSP_REVOKEDINFO_new(__reg("a6") struct Library *)="\tjsr\t-6276(a6)";
#define OCSP_REVOKEDINFO_new() __OCSP_REVOKEDINFO_new(AmiSSLBase)

void __OCSP_REVOKEDINFO_free(__reg("a6") struct Library *, __reg("a0") OCSP_REVOKEDINFO * a)="\tjsr\t-6282(a6)";
#define OCSP_REVOKEDINFO_free(a) __OCSP_REVOKEDINFO_free(AmiSSLBase, (a))

OCSP_REVOKEDINFO * __d2i_OCSP_REVOKEDINFO(__reg("a6") struct Library *, __reg("a0") OCSP_REVOKEDINFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6288(a6)";
#define d2i_OCSP_REVOKEDINFO(a, in, len) __d2i_OCSP_REVOKEDINFO(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_REVOKEDINFO(__reg("a6") struct Library *, __reg("a0") OCSP_REVOKEDINFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-6294(a6)";
#define i2d_OCSP_REVOKEDINFO(a, out) __i2d_OCSP_REVOKEDINFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_REVOKEDINFO_it(__reg("a6") struct Library *)="\tjsr\t-6300(a6)";
#define OCSP_REVOKEDINFO_it() __OCSP_REVOKEDINFO_it(AmiSSLBase)

OCSP_BASICRESP * __OCSP_BASICRESP_new(__reg("a6") struct Library *)="\tjsr\t-6306(a6)";
#define OCSP_BASICRESP_new() __OCSP_BASICRESP_new(AmiSSLBase)

void __OCSP_BASICRESP_free(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * a)="\tjsr\t-6312(a6)";
#define OCSP_BASICRESP_free(a) __OCSP_BASICRESP_free(AmiSSLBase, (a))

OCSP_BASICRESP * __d2i_OCSP_BASICRESP(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6318(a6)";
#define d2i_OCSP_BASICRESP(a, in, len) __d2i_OCSP_BASICRESP(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_BASICRESP(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * a, __reg("a1") unsigned char ** out)="\tjsr\t-6324(a6)";
#define i2d_OCSP_BASICRESP(a, out) __i2d_OCSP_BASICRESP(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_BASICRESP_it(__reg("a6") struct Library *)="\tjsr\t-6330(a6)";
#define OCSP_BASICRESP_it() __OCSP_BASICRESP_it(AmiSSLBase)

OCSP_RESPDATA * __OCSP_RESPDATA_new(__reg("a6") struct Library *)="\tjsr\t-6336(a6)";
#define OCSP_RESPDATA_new() __OCSP_RESPDATA_new(AmiSSLBase)

void __OCSP_RESPDATA_free(__reg("a6") struct Library *, __reg("a0") OCSP_RESPDATA * a)="\tjsr\t-6342(a6)";
#define OCSP_RESPDATA_free(a) __OCSP_RESPDATA_free(AmiSSLBase, (a))

OCSP_RESPDATA * __d2i_OCSP_RESPDATA(__reg("a6") struct Library *, __reg("a0") OCSP_RESPDATA ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6348(a6)";
#define d2i_OCSP_RESPDATA(a, in, len) __d2i_OCSP_RESPDATA(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_RESPDATA(__reg("a6") struct Library *, __reg("a0") OCSP_RESPDATA * a, __reg("a1") unsigned char ** out)="\tjsr\t-6354(a6)";
#define i2d_OCSP_RESPDATA(a, out) __i2d_OCSP_RESPDATA(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_RESPDATA_it(__reg("a6") struct Library *)="\tjsr\t-6360(a6)";
#define OCSP_RESPDATA_it() __OCSP_RESPDATA_it(AmiSSLBase)

OCSP_RESPID * __OCSP_RESPID_new(__reg("a6") struct Library *)="\tjsr\t-6366(a6)";
#define OCSP_RESPID_new() __OCSP_RESPID_new(AmiSSLBase)

void __OCSP_RESPID_free(__reg("a6") struct Library *, __reg("a0") OCSP_RESPID * a)="\tjsr\t-6372(a6)";
#define OCSP_RESPID_free(a) __OCSP_RESPID_free(AmiSSLBase, (a))

OCSP_RESPID * __d2i_OCSP_RESPID(__reg("a6") struct Library *, __reg("a0") OCSP_RESPID ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6378(a6)";
#define d2i_OCSP_RESPID(a, in, len) __d2i_OCSP_RESPID(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_RESPID(__reg("a6") struct Library *, __reg("a0") OCSP_RESPID * a, __reg("a1") unsigned char ** out)="\tjsr\t-6384(a6)";
#define i2d_OCSP_RESPID(a, out) __i2d_OCSP_RESPID(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_RESPID_it(__reg("a6") struct Library *)="\tjsr\t-6390(a6)";
#define OCSP_RESPID_it() __OCSP_RESPID_it(AmiSSLBase)

OCSP_RESPONSE * __OCSP_RESPONSE_new(__reg("a6") struct Library *)="\tjsr\t-6396(a6)";
#define OCSP_RESPONSE_new() __OCSP_RESPONSE_new(AmiSSLBase)

void __OCSP_RESPONSE_free(__reg("a6") struct Library *, __reg("a0") OCSP_RESPONSE * a)="\tjsr\t-6402(a6)";
#define OCSP_RESPONSE_free(a) __OCSP_RESPONSE_free(AmiSSLBase, (a))

OCSP_RESPONSE * __d2i_OCSP_RESPONSE(__reg("a6") struct Library *, __reg("a0") OCSP_RESPONSE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6408(a6)";
#define d2i_OCSP_RESPONSE(a, in, len) __d2i_OCSP_RESPONSE(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_RESPONSE(__reg("a6") struct Library *, __reg("a0") OCSP_RESPONSE * a, __reg("a1") unsigned char ** out)="\tjsr\t-6414(a6)";
#define i2d_OCSP_RESPONSE(a, out) __i2d_OCSP_RESPONSE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_RESPONSE_it(__reg("a6") struct Library *)="\tjsr\t-6420(a6)";
#define OCSP_RESPONSE_it() __OCSP_RESPONSE_it(AmiSSLBase)

OCSP_RESPBYTES * __OCSP_RESPBYTES_new(__reg("a6") struct Library *)="\tjsr\t-6426(a6)";
#define OCSP_RESPBYTES_new() __OCSP_RESPBYTES_new(AmiSSLBase)

void __OCSP_RESPBYTES_free(__reg("a6") struct Library *, __reg("a0") OCSP_RESPBYTES * a)="\tjsr\t-6432(a6)";
#define OCSP_RESPBYTES_free(a) __OCSP_RESPBYTES_free(AmiSSLBase, (a))

OCSP_RESPBYTES * __d2i_OCSP_RESPBYTES(__reg("a6") struct Library *, __reg("a0") OCSP_RESPBYTES ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6438(a6)";
#define d2i_OCSP_RESPBYTES(a, in, len) __d2i_OCSP_RESPBYTES(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_RESPBYTES(__reg("a6") struct Library *, __reg("a0") OCSP_RESPBYTES * a, __reg("a1") unsigned char ** out)="\tjsr\t-6444(a6)";
#define i2d_OCSP_RESPBYTES(a, out) __i2d_OCSP_RESPBYTES(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_RESPBYTES_it(__reg("a6") struct Library *)="\tjsr\t-6450(a6)";
#define OCSP_RESPBYTES_it() __OCSP_RESPBYTES_it(AmiSSLBase)

OCSP_ONEREQ * __OCSP_ONEREQ_new(__reg("a6") struct Library *)="\tjsr\t-6456(a6)";
#define OCSP_ONEREQ_new() __OCSP_ONEREQ_new(AmiSSLBase)

void __OCSP_ONEREQ_free(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * a)="\tjsr\t-6462(a6)";
#define OCSP_ONEREQ_free(a) __OCSP_ONEREQ_free(AmiSSLBase, (a))

OCSP_ONEREQ * __d2i_OCSP_ONEREQ(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6468(a6)";
#define d2i_OCSP_ONEREQ(a, in, len) __d2i_OCSP_ONEREQ(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_ONEREQ(__reg("a6") struct Library *, __reg("a0") OCSP_ONEREQ * a, __reg("a1") unsigned char ** out)="\tjsr\t-6474(a6)";
#define i2d_OCSP_ONEREQ(a, out) __i2d_OCSP_ONEREQ(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_ONEREQ_it(__reg("a6") struct Library *)="\tjsr\t-6480(a6)";
#define OCSP_ONEREQ_it() __OCSP_ONEREQ_it(AmiSSLBase)

OCSP_CERTID * __OCSP_CERTID_new(__reg("a6") struct Library *)="\tjsr\t-6486(a6)";
#define OCSP_CERTID_new() __OCSP_CERTID_new(AmiSSLBase)

void __OCSP_CERTID_free(__reg("a6") struct Library *, __reg("a0") OCSP_CERTID * a)="\tjsr\t-6492(a6)";
#define OCSP_CERTID_free(a) __OCSP_CERTID_free(AmiSSLBase, (a))

OCSP_CERTID * __d2i_OCSP_CERTID(__reg("a6") struct Library *, __reg("a0") OCSP_CERTID ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6498(a6)";
#define d2i_OCSP_CERTID(a, in, len) __d2i_OCSP_CERTID(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_CERTID(__reg("a6") struct Library *, __reg("a0") OCSP_CERTID * a, __reg("a1") unsigned char ** out)="\tjsr\t-6504(a6)";
#define i2d_OCSP_CERTID(a, out) __i2d_OCSP_CERTID(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_CERTID_it(__reg("a6") struct Library *)="\tjsr\t-6510(a6)";
#define OCSP_CERTID_it() __OCSP_CERTID_it(AmiSSLBase)

OCSP_REQUEST * __OCSP_REQUEST_new(__reg("a6") struct Library *)="\tjsr\t-6516(a6)";
#define OCSP_REQUEST_new() __OCSP_REQUEST_new(AmiSSLBase)

void __OCSP_REQUEST_free(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * a)="\tjsr\t-6522(a6)";
#define OCSP_REQUEST_free(a) __OCSP_REQUEST_free(AmiSSLBase, (a))

OCSP_REQUEST * __d2i_OCSP_REQUEST(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6528(a6)";
#define d2i_OCSP_REQUEST(a, in, len) __d2i_OCSP_REQUEST(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_REQUEST(__reg("a6") struct Library *, __reg("a0") OCSP_REQUEST * a, __reg("a1") unsigned char ** out)="\tjsr\t-6534(a6)";
#define i2d_OCSP_REQUEST(a, out) __i2d_OCSP_REQUEST(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_REQUEST_it(__reg("a6") struct Library *)="\tjsr\t-6540(a6)";
#define OCSP_REQUEST_it() __OCSP_REQUEST_it(AmiSSLBase)

OCSP_SIGNATURE * __OCSP_SIGNATURE_new(__reg("a6") struct Library *)="\tjsr\t-6546(a6)";
#define OCSP_SIGNATURE_new() __OCSP_SIGNATURE_new(AmiSSLBase)

void __OCSP_SIGNATURE_free(__reg("a6") struct Library *, __reg("a0") OCSP_SIGNATURE * a)="\tjsr\t-6552(a6)";
#define OCSP_SIGNATURE_free(a) __OCSP_SIGNATURE_free(AmiSSLBase, (a))

OCSP_SIGNATURE * __d2i_OCSP_SIGNATURE(__reg("a6") struct Library *, __reg("a0") OCSP_SIGNATURE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6558(a6)";
#define d2i_OCSP_SIGNATURE(a, in, len) __d2i_OCSP_SIGNATURE(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_SIGNATURE(__reg("a6") struct Library *, __reg("a0") OCSP_SIGNATURE * a, __reg("a1") unsigned char ** out)="\tjsr\t-6564(a6)";
#define i2d_OCSP_SIGNATURE(a, out) __i2d_OCSP_SIGNATURE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_SIGNATURE_it(__reg("a6") struct Library *)="\tjsr\t-6570(a6)";
#define OCSP_SIGNATURE_it() __OCSP_SIGNATURE_it(AmiSSLBase)

OCSP_REQINFO * __OCSP_REQINFO_new(__reg("a6") struct Library *)="\tjsr\t-6576(a6)";
#define OCSP_REQINFO_new() __OCSP_REQINFO_new(AmiSSLBase)

void __OCSP_REQINFO_free(__reg("a6") struct Library *, __reg("a0") OCSP_REQINFO * a)="\tjsr\t-6582(a6)";
#define OCSP_REQINFO_free(a) __OCSP_REQINFO_free(AmiSSLBase, (a))

OCSP_REQINFO * __d2i_OCSP_REQINFO(__reg("a6") struct Library *, __reg("a0") OCSP_REQINFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6588(a6)";
#define d2i_OCSP_REQINFO(a, in, len) __d2i_OCSP_REQINFO(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_REQINFO(__reg("a6") struct Library *, __reg("a0") OCSP_REQINFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-6594(a6)";
#define i2d_OCSP_REQINFO(a, out) __i2d_OCSP_REQINFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_REQINFO_it(__reg("a6") struct Library *)="\tjsr\t-6600(a6)";
#define OCSP_REQINFO_it() __OCSP_REQINFO_it(AmiSSLBase)

OCSP_CRLID * __OCSP_CRLID_new(__reg("a6") struct Library *)="\tjsr\t-6606(a6)";
#define OCSP_CRLID_new() __OCSP_CRLID_new(AmiSSLBase)

void __OCSP_CRLID_free(__reg("a6") struct Library *, __reg("a0") OCSP_CRLID * a)="\tjsr\t-6612(a6)";
#define OCSP_CRLID_free(a) __OCSP_CRLID_free(AmiSSLBase, (a))

OCSP_CRLID * __d2i_OCSP_CRLID(__reg("a6") struct Library *, __reg("a0") OCSP_CRLID ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6618(a6)";
#define d2i_OCSP_CRLID(a, in, len) __d2i_OCSP_CRLID(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_CRLID(__reg("a6") struct Library *, __reg("a0") OCSP_CRLID * a, __reg("a1") unsigned char ** out)="\tjsr\t-6624(a6)";
#define i2d_OCSP_CRLID(a, out) __i2d_OCSP_CRLID(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_CRLID_it(__reg("a6") struct Library *)="\tjsr\t-6630(a6)";
#define OCSP_CRLID_it() __OCSP_CRLID_it(AmiSSLBase)

OCSP_SERVICELOC * __OCSP_SERVICELOC_new(__reg("a6") struct Library *)="\tjsr\t-6636(a6)";
#define OCSP_SERVICELOC_new() __OCSP_SERVICELOC_new(AmiSSLBase)

void __OCSP_SERVICELOC_free(__reg("a6") struct Library *, __reg("a0") OCSP_SERVICELOC * a)="\tjsr\t-6642(a6)";
#define OCSP_SERVICELOC_free(a) __OCSP_SERVICELOC_free(AmiSSLBase, (a))

OCSP_SERVICELOC * __d2i_OCSP_SERVICELOC(__reg("a6") struct Library *, __reg("a0") OCSP_SERVICELOC ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-6648(a6)";
#define d2i_OCSP_SERVICELOC(a, in, len) __d2i_OCSP_SERVICELOC(AmiSSLBase, (a), (in), (len))

int __i2d_OCSP_SERVICELOC(__reg("a6") struct Library *, __reg("a0") OCSP_SERVICELOC * a, __reg("a1") unsigned char ** out)="\tjsr\t-6654(a6)";
#define i2d_OCSP_SERVICELOC(a, out) __i2d_OCSP_SERVICELOC(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OCSP_SERVICELOC_it(__reg("a6") struct Library *)="\tjsr\t-6660(a6)";
#define OCSP_SERVICELOC_it() __OCSP_SERVICELOC_it(AmiSSLBase)

char * __OCSP_response_status_str(__reg("a6") struct Library *, __reg("d0") long s)="\tjsr\t-6666(a6)";
#define OCSP_response_status_str(s) __OCSP_response_status_str(AmiSSLBase, (s))

char * __OCSP_cert_status_str(__reg("a6") struct Library *, __reg("d0") long s)="\tjsr\t-6672(a6)";
#define OCSP_cert_status_str(s) __OCSP_cert_status_str(AmiSSLBase, (s))

char * __OCSP_crl_reason_str(__reg("a6") struct Library *, __reg("d0") long s)="\tjsr\t-6678(a6)";
#define OCSP_crl_reason_str(s) __OCSP_crl_reason_str(AmiSSLBase, (s))

int __OCSP_REQUEST_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") OCSP_REQUEST* a, __reg("d0") unsigned long flags)="\tjsr\t-6684(a6)";
#define OCSP_REQUEST_print(bp, a, flags) __OCSP_REQUEST_print(AmiSSLBase, (bp), (a), (flags))

int __OCSP_RESPONSE_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") OCSP_RESPONSE* o, __reg("d0") unsigned long flags)="\tjsr\t-6690(a6)";
#define OCSP_RESPONSE_print(bp, o, flags) __OCSP_RESPONSE_print(AmiSSLBase, (bp), (o), (flags))

int __OCSP_basic_verify(__reg("a6") struct Library *, __reg("a0") OCSP_BASICRESP * bs, __reg("a1") void * certs, __reg("a2") X509_STORE * st, __reg("d0") unsigned long flags)="\tjsr\t-6696(a6)";
#define OCSP_basic_verify(bs, certs, st, flags) __OCSP_basic_verify(AmiSSLBase, (bs), (certs), (st), (flags))

void __ERR_load_OCSP_strings(__reg("a6") struct Library *)="\tjsr\t-6702(a6)";
#define ERR_load_OCSP_strings() __ERR_load_OCSP_strings(AmiSSLBase)

int __PEM_get_EVP_CIPHER_INFO(__reg("a6") struct Library *, __reg("a0") char * header, __reg("a1") EVP_CIPHER_INFO * cipher)="\tjsr\t-6708(a6)";
#define PEM_get_EVP_CIPHER_INFO(header, cipher) __PEM_get_EVP_CIPHER_INFO(AmiSSLBase, (header), (cipher))

int __PEM_do_header(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_INFO * cipher, __reg("a1") unsigned char * data, __reg("a2") long * len, __reg("a3") pem_password_cb * (*callback)(char *buf, int size, int rwflag, void *userdata), __reg("d0") void * u)="\tjsr\t-6714(a6)";
#define PEM_do_header(cipher, data, len, callback, u) __PEM_do_header(AmiSSLBase, (cipher), (data), (len), (callback), (u))

int __PEM_read_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") char ** name, __reg("a2") char ** header, __reg("a3") unsigned char ** data, __reg("d0") long * len)="\tjsr\t-6720(a6)";
#define PEM_read_bio(bp, name, header, data, len) __PEM_read_bio(AmiSSLBase, (bp), (name), (header), (data), (len))

int __PEM_write_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const char * name, __reg("a2") char * hdr, __reg("a3") unsigned char * data, __reg("d0") long len)="\tjsr\t-6726(a6)";
#define PEM_write_bio(bp, name, hdr, data, len) __PEM_write_bio(AmiSSLBase, (bp), (name), (hdr), (data), (len))

int __PEM_bytes_read_bio(__reg("a6") struct Library *, __reg("a0") unsigned char ** pdata, __reg("a1") long * plen, __reg("a2") char ** pnm, __reg("a3") const char * name, __reg("d0") BIO * bp, __reg("d1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-6732(a6)";
#define PEM_bytes_read_bio(pdata, plen, pnm, name, bp, cb, u) __PEM_bytes_read_bio(AmiSSLBase, (pdata), (plen), (pnm), (name), (bp), (cb), (u))

char * __PEM_ASN1_read_bio(__reg("a6") struct Library *, __reg("a0") char * (*d2i)(), __reg("a1") const char * name, __reg("a2") BIO * bp, __reg("a3") char ** x, __reg("d0") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d1") void * u)="\tjsr\t-6738(a6)";
#define PEM_ASN1_read_bio(d2i, name, bp, x, cb, u) __PEM_ASN1_read_bio(AmiSSLBase, (d2i), (name), (bp), (x), (cb), (u))

int __PEM_ASN1_write_bio(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") const char * name, __reg("a2") BIO * bp, __reg("a3") char * x, __reg("d0") const EVP_CIPHER * enc, __reg("d1") unsigned char * kstr, __reg("d2") LONG klen, __reg("d3") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d4") void * u)="\tjsr\t-6744(a6)";
#define PEM_ASN1_write_bio(i2d, name, bp, x, enc, kstr, klen, cb, u) __PEM_ASN1_write_bio(AmiSSLBase, (i2d), (name), (bp), (x), (enc), (kstr), (klen), (cb), (u))

void * __PEM_X509_INFO_read_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") void * sk, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6750(a6)";
#define PEM_X509_INFO_read_bio(bp, sk, cb, u) __PEM_X509_INFO_read_bio(AmiSSLBase, (bp), (sk), (cb), (u))

int __PEM_X509_INFO_write_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_INFO * xi, __reg("a2") EVP_CIPHER * enc, __reg("a3") unsigned char * kstr, __reg("d0") LONG klen, __reg("d1") pem_password_cb * (*cd)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-6756(a6)";
#define PEM_X509_INFO_write_bio(bp, xi, enc, kstr, klen, cd, u) __PEM_X509_INFO_write_bio(AmiSSLBase, (bp), (xi), (enc), (kstr), (klen), (cd), (u))

int __PEM_SealInit(__reg("a6") struct Library *, __reg("a0") PEM_ENCODE_SEAL_CTX * ctx, __reg("a1") EVP_CIPHER * type, __reg("a2") EVP_MD * md_type, __reg("a3") unsigned char ** ek, __reg("d0") int * ekl, __reg("d1") unsigned char * iv, __reg("d2") EVP_PKEY ** pubk, __reg("d3") LONG npubk)="\tjsr\t-6762(a6)";
#define PEM_SealInit(ctx, type, md_type, ek, ekl, iv, pubk, npubk) __PEM_SealInit(AmiSSLBase, (ctx), (type), (md_type), (ek), (ekl), (iv), (pubk), (npubk))

void __PEM_SealUpdate(__reg("a6") struct Library *, __reg("a0") PEM_ENCODE_SEAL_CTX * ctx, __reg("a1") unsigned char * out, __reg("a2") int * outl, __reg("a3") unsigned char * in, __reg("d0") LONG inl)="\tjsr\t-6768(a6)";
#define PEM_SealUpdate(ctx, out, outl, in, inl) __PEM_SealUpdate(AmiSSLBase, (ctx), (out), (outl), (in), (inl))

int __PEM_SealFinal(__reg("a6") struct Library *, __reg("a0") PEM_ENCODE_SEAL_CTX * ctx, __reg("a1") unsigned char * sig, __reg("a2") int * sigl, __reg("a3") unsigned char * out, __reg("d0") int * outl, __reg("d1") EVP_PKEY * priv)="\tjsr\t-6774(a6)";
#define PEM_SealFinal(ctx, sig, sigl, out, outl, priv) __PEM_SealFinal(AmiSSLBase, (ctx), (sig), (sigl), (out), (outl), (priv))

void __PEM_SignInit(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") EVP_MD * type)="\tjsr\t-6780(a6)";
#define PEM_SignInit(ctx, type) __PEM_SignInit(AmiSSLBase, (ctx), (type))

void __PEM_SignUpdate(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * d, __reg("d0") ULONG cnt)="\tjsr\t-6786(a6)";
#define PEM_SignUpdate(ctx, d, cnt) __PEM_SignUpdate(AmiSSLBase, (ctx), (d), (cnt))

int __PEM_SignFinal(__reg("a6") struct Library *, __reg("a0") EVP_MD_CTX * ctx, __reg("a1") unsigned char * sigret, __reg("a2") unsigned int * siglen, __reg("a3") EVP_PKEY * pkey)="\tjsr\t-6792(a6)";
#define PEM_SignFinal(ctx, sigret, siglen, pkey) __PEM_SignFinal(AmiSSLBase, (ctx), (sigret), (siglen), (pkey))

int __PEM_def_callback(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG num, __reg("d1") LONG w, __reg("a1") void * key)="\tjsr\t-6798(a6)";
#define PEM_def_callback(buf, num, w, key) __PEM_def_callback(AmiSSLBase, (buf), (num), (w), (key))

void __PEM_proc_type(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG type)="\tjsr\t-6804(a6)";
#define PEM_proc_type(buf, type) __PEM_proc_type(AmiSSLBase, (buf), (type))

void __PEM_dek_info(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("a1") const char * type, __reg("d0") LONG len, __reg("a2") char * str)="\tjsr\t-6810(a6)";
#define PEM_dek_info(buf, type, len, str) __PEM_dek_info(AmiSSLBase, (buf), (type), (len), (str))

X509 * __PEM_read_bio_X509(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6816(a6)";
#define PEM_read_bio_X509(bp, x, cb, u) __PEM_read_bio_X509(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_X509(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x)="\tjsr\t-6822(a6)";
#define PEM_write_bio_X509(bp, x) __PEM_write_bio_X509(AmiSSLBase, (bp), (x))

X509 * __PEM_read_bio_X509_AUX(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6828(a6)";
#define PEM_read_bio_X509_AUX(bp, x, cb, u) __PEM_read_bio_X509_AUX(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_X509_AUX(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x)="\tjsr\t-6834(a6)";
#define PEM_write_bio_X509_AUX(bp, x) __PEM_write_bio_X509_AUX(AmiSSLBase, (bp), (x))

X509_REQ * __PEM_read_bio_X509_REQ(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6840(a6)";
#define PEM_read_bio_X509_REQ(bp, x, cb, u) __PEM_read_bio_X509_REQ(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_X509_REQ(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ * x)="\tjsr\t-6846(a6)";
#define PEM_write_bio_X509_REQ(bp, x) __PEM_write_bio_X509_REQ(AmiSSLBase, (bp), (x))

int __PEM_write_bio_X509_REQ_NEW(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ * x)="\tjsr\t-6852(a6)";
#define PEM_write_bio_X509_REQ_NEW(bp, x) __PEM_write_bio_X509_REQ_NEW(AmiSSLBase, (bp), (x))

X509_CRL * __PEM_read_bio_X509_CRL(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CRL ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6858(a6)";
#define PEM_read_bio_X509_CRL(bp, x, cb, u) __PEM_read_bio_X509_CRL(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_X509_CRL(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CRL * x)="\tjsr\t-6864(a6)";
#define PEM_write_bio_X509_CRL(bp, x) __PEM_write_bio_X509_CRL(AmiSSLBase, (bp), (x))

PKCS7 * __PEM_read_bio_PKCS7(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS7 ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6870(a6)";
#define PEM_read_bio_PKCS7(bp, x, cb, u) __PEM_read_bio_PKCS7(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_PKCS7(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS7 * x)="\tjsr\t-6876(a6)";
#define PEM_write_bio_PKCS7(bp, x) __PEM_write_bio_PKCS7(AmiSSLBase, (bp), (x))

NETSCAPE_CERT_SEQUENCE * __PEM_read_bio_NETSCAPE_CERT_SEQUENCE(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") NETSCAPE_CERT_SEQUENCE ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6882(a6)";
#define PEM_read_bio_NETSCAPE_CERT_SEQUENCE(bp, x, cb, u) __PEM_read_bio_NETSCAPE_CERT_SEQUENCE(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_NETSCAPE_CERT_SEQUENCE(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") NETSCAPE_CERT_SEQUENCE * x)="\tjsr\t-6888(a6)";
#define PEM_write_bio_NETSCAPE_CERT_SEQUENCE(bp, x) __PEM_write_bio_NETSCAPE_CERT_SEQUENCE(AmiSSLBase, (bp), (x))

X509_SIG * __PEM_read_bio_PKCS8(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_SIG ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6894(a6)";
#define PEM_read_bio_PKCS8(bp, x, cb, u) __PEM_read_bio_PKCS8(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_PKCS8(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_SIG * x)="\tjsr\t-6900(a6)";
#define PEM_write_bio_PKCS8(bp, x) __PEM_write_bio_PKCS8(AmiSSLBase, (bp), (x))

PKCS8_PRIV_KEY_INFO * __PEM_read_bio_PKCS8_PRIV_KEY_INFO(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS8_PRIV_KEY_INFO ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6906(a6)";
#define PEM_read_bio_PKCS8_PRIV_KEY_INFO(bp, x, cb, u) __PEM_read_bio_PKCS8_PRIV_KEY_INFO(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_PKCS8_PRIV_KEY_INFO(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS8_PRIV_KEY_INFO * x)="\tjsr\t-6912(a6)";
#define PEM_write_bio_PKCS8_PRIV_KEY_INFO(bp, x) __PEM_write_bio_PKCS8_PRIV_KEY_INFO(AmiSSLBase, (bp), (x))

RSA * __PEM_read_bio_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6918(a6)";
#define PEM_read_bio_RSAPrivateKey(bp, x, cb, u) __PEM_read_bio_RSAPrivateKey(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * x, __reg("a2") const EVP_CIPHER * enc, __reg("a3") unsigned char * kstr, __reg("d0") LONG klen, __reg("d1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-6924(a6)";
#define PEM_write_bio_RSAPrivateKey(bp, x, enc, kstr, klen, cb, u) __PEM_write_bio_RSAPrivateKey(AmiSSLBase, (bp), (x), (enc), (kstr), (klen), (cb), (u))

RSA * __PEM_read_bio_RSAPublicKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6930(a6)";
#define PEM_read_bio_RSAPublicKey(bp, x, cb, u) __PEM_read_bio_RSAPublicKey(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_RSAPublicKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * x)="\tjsr\t-6936(a6)";
#define PEM_write_bio_RSAPublicKey(bp, x) __PEM_write_bio_RSAPublicKey(AmiSSLBase, (bp), (x))

RSA * __PEM_read_bio_RSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6942(a6)";
#define PEM_read_bio_RSA_PUBKEY(bp, x, cb, u) __PEM_read_bio_RSA_PUBKEY(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_RSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * x)="\tjsr\t-6948(a6)";
#define PEM_write_bio_RSA_PUBKEY(bp, x) __PEM_write_bio_RSA_PUBKEY(AmiSSLBase, (bp), (x))

DSA * __PEM_read_bio_DSAPrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6954(a6)";
#define PEM_read_bio_DSAPrivateKey(bp, x, cb, u) __PEM_read_bio_DSAPrivateKey(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_DSAPrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA * x, __reg("a2") const EVP_CIPHER * enc, __reg("a3") unsigned char * kstr, __reg("d0") LONG klen, __reg("d1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-6960(a6)";
#define PEM_write_bio_DSAPrivateKey(bp, x, enc, kstr, klen, cb, u) __PEM_write_bio_DSAPrivateKey(AmiSSLBase, (bp), (x), (enc), (kstr), (klen), (cb), (u))

DSA * __PEM_read_bio_DSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6966(a6)";
#define PEM_read_bio_DSA_PUBKEY(bp, x, cb, u) __PEM_read_bio_DSA_PUBKEY(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_DSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA * x)="\tjsr\t-6972(a6)";
#define PEM_write_bio_DSA_PUBKEY(bp, x) __PEM_write_bio_DSA_PUBKEY(AmiSSLBase, (bp), (x))

DSA * __PEM_read_bio_DSAparams(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6978(a6)";
#define PEM_read_bio_DSAparams(bp, x, cb, u) __PEM_read_bio_DSAparams(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_DSAparams(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA * x)="\tjsr\t-6984(a6)";
#define PEM_write_bio_DSAparams(bp, x) __PEM_write_bio_DSAparams(AmiSSLBase, (bp), (x))

DH * __PEM_read_bio_DHparams(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DH ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-6990(a6)";
#define PEM_read_bio_DHparams(bp, x, cb, u) __PEM_read_bio_DHparams(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_DHparams(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DH * x)="\tjsr\t-6996(a6)";
#define PEM_write_bio_DHparams(bp, x) __PEM_write_bio_DHparams(AmiSSLBase, (bp), (x))

EVP_PKEY * __PEM_read_bio_PrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-7002(a6)";
#define PEM_read_bio_PrivateKey(bp, x, cb, u) __PEM_read_bio_PrivateKey(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_PrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * x, __reg("a2") const EVP_CIPHER * enc, __reg("a3") unsigned char * kstr, __reg("d0") LONG klen, __reg("d1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-7008(a6)";
#define PEM_write_bio_PrivateKey(bp, x, enc, kstr, klen, cb, u) __PEM_write_bio_PrivateKey(AmiSSLBase, (bp), (x), (enc), (kstr), (klen), (cb), (u))

EVP_PKEY * __PEM_read_bio_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-7014(a6)";
#define PEM_read_bio_PUBKEY(bp, x, cb, u) __PEM_read_bio_PUBKEY(AmiSSLBase, (bp), (x), (cb), (u))

int __PEM_write_bio_PUBKEY(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * x)="\tjsr\t-7020(a6)";
#define PEM_write_bio_PUBKEY(bp, x) __PEM_write_bio_PUBKEY(AmiSSLBase, (bp), (x))

int __PEM_write_bio_PKCS8PrivateKey_nid(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * x, __reg("d0") LONG nid, __reg("a2") char * kstr, __reg("d1") LONG klen, __reg("a3") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-7026(a6)";
#define PEM_write_bio_PKCS8PrivateKey_nid(bp, x, nid, kstr, klen, cb, u) __PEM_write_bio_PKCS8PrivateKey_nid(AmiSSLBase, (bp), (x), (nid), (kstr), (klen), (cb), (u))

int __PEM_write_bio_PKCS8PrivateKey(__reg("a6") struct Library *, __reg("a0") BIO * a, __reg("a1") EVP_PKEY * b, __reg("a2") const EVP_CIPHER * c, __reg("a3") char * d, __reg("d0") LONG a1, __reg("d1") pem_password_cb * (*e)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * f)="\tjsr\t-7032(a6)";
#define PEM_write_bio_PKCS8PrivateKey(a, b, c, d, a1, e, f) __PEM_write_bio_PKCS8PrivateKey(AmiSSLBase, (a), (b), (c), (d), (a1), (e), (f))

int __i2d_PKCS8PrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * x, __reg("a2") const EVP_CIPHER * enc, __reg("a3") char * kstr, __reg("d0") LONG klen, __reg("d1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-7038(a6)";
#define i2d_PKCS8PrivateKey_bio(bp, x, enc, kstr, klen, cb, u) __i2d_PKCS8PrivateKey_bio(AmiSSLBase, (bp), (x), (enc), (kstr), (klen), (cb), (u))

int __i2d_PKCS8PrivateKey_nid_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * x, __reg("d0") LONG nid, __reg("a2") char * kstr, __reg("d1") LONG klen, __reg("a3") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("d2") void * u)="\tjsr\t-7044(a6)";
#define i2d_PKCS8PrivateKey_nid_bio(bp, x, nid, kstr, klen, cb, u) __i2d_PKCS8PrivateKey_nid_bio(AmiSSLBase, (bp), (x), (nid), (kstr), (klen), (cb), (u))

EVP_PKEY * __d2i_PKCS8PrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY ** x, __reg("a2") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata), __reg("a3") void * u)="\tjsr\t-7050(a6)";
#define d2i_PKCS8PrivateKey_bio(bp, x, cb, u) __d2i_PKCS8PrivateKey_bio(AmiSSLBase, (bp), (x), (cb), (u))

void __ERR_load_PEM_strings(__reg("a6") struct Library *)="\tjsr\t-7056(a6)";
#define ERR_load_PEM_strings() __ERR_load_PEM_strings(AmiSSLBase)

PKCS12_SAFEBAG * __PKCS12_x5092certbag(__reg("a6") struct Library *, __reg("a0") X509 * x509)="\tjsr\t-7062(a6)";
#define PKCS12_x5092certbag(x509) __PKCS12_x5092certbag(AmiSSLBase, (x509))

PKCS12_SAFEBAG * __PKCS12_x509crl2certbag(__reg("a6") struct Library *, __reg("a0") X509_CRL * crl)="\tjsr\t-7068(a6)";
#define PKCS12_x509crl2certbag(crl) __PKCS12_x509crl2certbag(AmiSSLBase, (crl))

X509 * __PKCS12_certbag2x509(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag)="\tjsr\t-7074(a6)";
#define PKCS12_certbag2x509(bag) __PKCS12_certbag2x509(AmiSSLBase, (bag))

X509_CRL * __PKCS12_certbag2x509crl(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag)="\tjsr\t-7080(a6)";
#define PKCS12_certbag2x509crl(bag) __PKCS12_certbag2x509crl(AmiSSLBase, (bag))

PKCS12_SAFEBAG * __PKCS12_item_pack_safebag(__reg("a6") struct Library *, __reg("a0") void * obj, __reg("a1") const ASN1_ITEM * it, __reg("d0") LONG nid1, __reg("d1") LONG nid2)="\tjsr\t-7086(a6)";
#define PKCS12_item_pack_safebag(obj, it, nid1, nid2) __PKCS12_item_pack_safebag(AmiSSLBase, (obj), (it), (nid1), (nid2))

PKCS12_SAFEBAG * __PKCS12_MAKE_KEYBAG(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * p8)="\tjsr\t-7092(a6)";
#define PKCS12_MAKE_KEYBAG(p8) __PKCS12_MAKE_KEYBAG(AmiSSLBase, (p8))

PKCS8_PRIV_KEY_INFO * __PKCS8_decrypt(__reg("a6") struct Library *, __reg("a0") X509_SIG * p8, __reg("a1") const char * pass, __reg("d0") LONG passlen)="\tjsr\t-7098(a6)";
#define PKCS8_decrypt(p8, pass, passlen) __PKCS8_decrypt(AmiSSLBase, (p8), (pass), (passlen))

PKCS8_PRIV_KEY_INFO * __PKCS12_decrypt_skey(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag, __reg("a1") const char * pass, __reg("d0") LONG passlen)="\tjsr\t-7104(a6)";
#define PKCS12_decrypt_skey(bag, pass, passlen) __PKCS12_decrypt_skey(AmiSSLBase, (bag), (pass), (passlen))

X509_SIG * __PKCS8_encrypt(__reg("a6") struct Library *, __reg("d0") LONG pbe_nid, __reg("a0") const EVP_CIPHER * cipher, __reg("a1") const char * pass, __reg("d1") LONG passlen, __reg("a2") unsigned char * salt, __reg("d2") LONG saltlen, __reg("d3") LONG iter, __reg("a3") PKCS8_PRIV_KEY_INFO * p8)="\tjsr\t-7110(a6)";
#define PKCS8_encrypt(pbe_nid, cipher, pass, passlen, salt, saltlen, iter, p8) __PKCS8_encrypt(AmiSSLBase, (pbe_nid), (cipher), (pass), (passlen), (salt), (saltlen), (iter), (p8))

PKCS12_SAFEBAG * __PKCS12_MAKE_SHKEYBAG(__reg("a6") struct Library *, __reg("d0") LONG pbe_nid, __reg("a0") const char * pass, __reg("d1") LONG passlen, __reg("a1") unsigned char * salt, __reg("d2") LONG saltlen, __reg("d3") LONG iter, __reg("a2") PKCS8_PRIV_KEY_INFO * p8)="\tjsr\t-7116(a6)";
#define PKCS12_MAKE_SHKEYBAG(pbe_nid, pass, passlen, salt, saltlen, iter, p8) __PKCS12_MAKE_SHKEYBAG(AmiSSLBase, (pbe_nid), (pass), (passlen), (salt), (saltlen), (iter), (p8))

PKCS7 * __PKCS12_pack_p7data(__reg("a6") struct Library *, __reg("a0") void * sk)="\tjsr\t-7122(a6)";
#define PKCS12_pack_p7data(sk) __PKCS12_pack_p7data(AmiSSLBase, (sk))

void * __PKCS12_unpack_p7data(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7)="\tjsr\t-7128(a6)";
#define PKCS12_unpack_p7data(p7) __PKCS12_unpack_p7data(AmiSSLBase, (p7))

PKCS7 * __PKCS12_pack_p7encdata(__reg("a6") struct Library *, __reg("d0") LONG pbe_nid, __reg("a0") const char * pass, __reg("d1") LONG passlen, __reg("a1") unsigned char * salt, __reg("d2") LONG saltlen, __reg("d3") LONG iter, __reg("a2") void * bags)="\tjsr\t-7134(a6)";
#define PKCS12_pack_p7encdata(pbe_nid, pass, passlen, salt, saltlen, iter, bags) __PKCS12_pack_p7encdata(AmiSSLBase, (pbe_nid), (pass), (passlen), (salt), (saltlen), (iter), (bags))

void * __PKCS12_unpack_p7encdata(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") const char * pass, __reg("d0") LONG passlen)="\tjsr\t-7140(a6)";
#define PKCS12_unpack_p7encdata(p7, pass, passlen) __PKCS12_unpack_p7encdata(AmiSSLBase, (p7), (pass), (passlen))

int __PKCS12_pack_authsafes(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") void * safes)="\tjsr\t-7146(a6)";
#define PKCS12_pack_authsafes(p12, safes) __PKCS12_pack_authsafes(AmiSSLBase, (p12), (safes))

void * __PKCS12_unpack_authsafes(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12)="\tjsr\t-7152(a6)";
#define PKCS12_unpack_authsafes(p12) __PKCS12_unpack_authsafes(AmiSSLBase, (p12))

int __PKCS12_add_localkeyid(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag, __reg("a1") unsigned char * name, __reg("d0") LONG namelen)="\tjsr\t-7158(a6)";
#define PKCS12_add_localkeyid(bag, name, namelen) __PKCS12_add_localkeyid(AmiSSLBase, (bag), (name), (namelen))

int __PKCS12_add_friendlyname_asc(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag, __reg("a1") const char * name, __reg("d0") LONG namelen)="\tjsr\t-7164(a6)";
#define PKCS12_add_friendlyname_asc(bag, name, namelen) __PKCS12_add_friendlyname_asc(AmiSSLBase, (bag), (name), (namelen))

int __PKCS12_add_CSPName_asc(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag, __reg("a1") const char * name, __reg("d0") LONG namelen)="\tjsr\t-7170(a6)";
#define PKCS12_add_CSPName_asc(bag, name, namelen) __PKCS12_add_CSPName_asc(AmiSSLBase, (bag), (name), (namelen))

int __PKCS12_add_friendlyname_uni(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag, __reg("a1") const unsigned char * name, __reg("d0") LONG namelen)="\tjsr\t-7176(a6)";
#define PKCS12_add_friendlyname_uni(bag, name, namelen) __PKCS12_add_friendlyname_uni(AmiSSLBase, (bag), (name), (namelen))

int __PKCS8_add_keyusage(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * p8, __reg("d0") LONG usage)="\tjsr\t-7182(a6)";
#define PKCS8_add_keyusage(p8, usage) __PKCS8_add_keyusage(AmiSSLBase, (p8), (usage))

ASN1_TYPE * __PKCS12_get_attr_gen(__reg("a6") struct Library *, __reg("a0") void * attrs, __reg("d0") LONG attr_nid)="\tjsr\t-7188(a6)";
#define PKCS12_get_attr_gen(attrs, attr_nid) __PKCS12_get_attr_gen(AmiSSLBase, (attrs), (attr_nid))

char * __PKCS12_get_friendlyname(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * bag)="\tjsr\t-7194(a6)";
#define PKCS12_get_friendlyname(bag) __PKCS12_get_friendlyname(AmiSSLBase, (bag))

unsigned char * __PKCS12_pbe_crypt(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * algor, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") unsigned char * in, __reg("d1") LONG inlen, __reg("a3") unsigned char ** data, __reg("d2") int * datalen, __reg("d3") LONG en_de)="\tjsr\t-7200(a6)";
#define PKCS12_pbe_crypt(algor, pass, passlen, in, inlen, data, datalen, en_de) __PKCS12_pbe_crypt(AmiSSLBase, (algor), (pass), (passlen), (in), (inlen), (data), (datalen), (en_de))

void * __PKCS12_item_decrypt_d2i(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * algor, __reg("a1") const ASN1_ITEM * it, __reg("a2") const char * pass, __reg("d0") LONG passlen, __reg("a3") ASN1_OCTET_STRING * oct, __reg("d1") LONG zbuf)="\tjsr\t-7206(a6)";
#define PKCS12_item_decrypt_d2i(algor, it, pass, passlen, oct, zbuf) __PKCS12_item_decrypt_d2i(AmiSSLBase, (algor), (it), (pass), (passlen), (oct), (zbuf))

ASN1_OCTET_STRING * __PKCS12_item_i2d_encrypt(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * algor, __reg("a1") const ASN1_ITEM * it, __reg("a2") const char * pass, __reg("d0") LONG passlen, __reg("a3") void * obj, __reg("d1") LONG zbuf)="\tjsr\t-7212(a6)";
#define PKCS12_item_i2d_encrypt(algor, it, pass, passlen, obj, zbuf) __PKCS12_item_i2d_encrypt(AmiSSLBase, (algor), (it), (pass), (passlen), (obj), (zbuf))

PKCS12 * __PKCS12_init(__reg("a6") struct Library *, __reg("d0") LONG mode)="\tjsr\t-7218(a6)";
#define PKCS12_init(mode) __PKCS12_init(AmiSSLBase, (mode))

int __PKCS12_key_gen_asc(__reg("a6") struct Library *, __reg("a0") const char * pass, __reg("d0") LONG passlen, __reg("a1") unsigned char * salt, __reg("d1") LONG saltlen, __reg("d2") LONG id, __reg("d3") LONG iter, __reg("d4") LONG n, __reg("a2") unsigned char * out, __reg("a3") const EVP_MD * md_type)="\tjsr\t-7224(a6)";
#define PKCS12_key_gen_asc(pass, passlen, salt, saltlen, id, iter, n, out, md_type) __PKCS12_key_gen_asc(AmiSSLBase, (pass), (passlen), (salt), (saltlen), (id), (iter), (n), (out), (md_type))

int __PKCS12_key_gen_uni(__reg("a6") struct Library *, __reg("a0") unsigned char * pass, __reg("d0") LONG passlen, __reg("a1") unsigned char * salt, __reg("d1") LONG saltlen, __reg("d2") LONG id, __reg("d3") LONG iter, __reg("d4") LONG n, __reg("a2") unsigned char * out, __reg("a3") const EVP_MD * md_type)="\tjsr\t-7230(a6)";
#define PKCS12_key_gen_uni(pass, passlen, salt, saltlen, id, iter, n, out, md_type) __PKCS12_key_gen_uni(AmiSSLBase, (pass), (passlen), (salt), (saltlen), (id), (iter), (n), (out), (md_type))

int __PKCS12_PBE_keyivgen(__reg("a6") struct Library *, __reg("a0") EVP_CIPHER_CTX * ctx, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") ASN1_TYPE * param, __reg("a3") const EVP_CIPHER * cipher, __reg("d1") const EVP_MD * md_type, __reg("d2") LONG en_de)="\tjsr\t-7236(a6)";
#define PKCS12_PBE_keyivgen(ctx, pass, passlen, param, cipher, md_type, en_de) __PKCS12_PBE_keyivgen(AmiSSLBase, (ctx), (pass), (passlen), (param), (cipher), (md_type), (en_de))

int __PKCS12_gen_mac(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") unsigned char * mac, __reg("a3") unsigned int * maclen)="\tjsr\t-7242(a6)";
#define PKCS12_gen_mac(p12, pass, passlen, mac, maclen) __PKCS12_gen_mac(AmiSSLBase, (p12), (pass), (passlen), (mac), (maclen))

int __PKCS12_verify_mac(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") const char * pass, __reg("d0") LONG passlen)="\tjsr\t-7248(a6)";
#define PKCS12_verify_mac(p12, pass, passlen) __PKCS12_verify_mac(AmiSSLBase, (p12), (pass), (passlen))

int __PKCS12_set_mac(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") const char * pass, __reg("d0") LONG passlen, __reg("a2") unsigned char * salt, __reg("d1") LONG saltlen, __reg("d2") LONG iter, __reg("a3") const EVP_MD * md_type)="\tjsr\t-7254(a6)";
#define PKCS12_set_mac(p12, pass, passlen, salt, saltlen, iter, md_type) __PKCS12_set_mac(AmiSSLBase, (p12), (pass), (passlen), (salt), (saltlen), (iter), (md_type))

int __PKCS12_setup_mac(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("d0") LONG iter, __reg("a1") unsigned char * salt, __reg("d1") LONG saltlen, __reg("a2") const EVP_MD * md_type)="\tjsr\t-7260(a6)";
#define PKCS12_setup_mac(p12, iter, salt, saltlen, md_type) __PKCS12_setup_mac(AmiSSLBase, (p12), (iter), (salt), (saltlen), (md_type))

unsigned char * __asc2uni(__reg("a6") struct Library *, __reg("a0") const char * asc, __reg("d0") LONG asclen, __reg("a1") unsigned char ** uni, __reg("a2") int * unilen)="\tjsr\t-7266(a6)";
#define asc2uni(asc, asclen, uni, unilen) __asc2uni(AmiSSLBase, (asc), (asclen), (uni), (unilen))

char * __uni2asc(__reg("a6") struct Library *, __reg("a0") unsigned char * uni, __reg("d0") LONG unilen)="\tjsr\t-7272(a6)";
#define uni2asc(uni, unilen) __uni2asc(AmiSSLBase, (uni), (unilen))

PKCS12 * __PKCS12_new(__reg("a6") struct Library *)="\tjsr\t-7278(a6)";
#define PKCS12_new() __PKCS12_new(AmiSSLBase)

void __PKCS12_free(__reg("a6") struct Library *, __reg("a0") PKCS12 * a)="\tjsr\t-7284(a6)";
#define PKCS12_free(a) __PKCS12_free(AmiSSLBase, (a))

PKCS12 * __d2i_PKCS12(__reg("a6") struct Library *, __reg("a0") PKCS12 ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7290(a6)";
#define d2i_PKCS12(a, in, len) __d2i_PKCS12(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS12(__reg("a6") struct Library *, __reg("a0") PKCS12 * a, __reg("a1") unsigned char ** out)="\tjsr\t-7296(a6)";
#define i2d_PKCS12(a, out) __i2d_PKCS12(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS12_it(__reg("a6") struct Library *)="\tjsr\t-7302(a6)";
#define PKCS12_it() __PKCS12_it(AmiSSLBase)

PKCS12_MAC_DATA * __PKCS12_MAC_DATA_new(__reg("a6") struct Library *)="\tjsr\t-7308(a6)";
#define PKCS12_MAC_DATA_new() __PKCS12_MAC_DATA_new(AmiSSLBase)

void __PKCS12_MAC_DATA_free(__reg("a6") struct Library *, __reg("a0") PKCS12_MAC_DATA * a)="\tjsr\t-7314(a6)";
#define PKCS12_MAC_DATA_free(a) __PKCS12_MAC_DATA_free(AmiSSLBase, (a))

PKCS12_MAC_DATA * __d2i_PKCS12_MAC_DATA(__reg("a6") struct Library *, __reg("a0") PKCS12_MAC_DATA ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7320(a6)";
#define d2i_PKCS12_MAC_DATA(a, in, len) __d2i_PKCS12_MAC_DATA(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS12_MAC_DATA(__reg("a6") struct Library *, __reg("a0") PKCS12_MAC_DATA * a, __reg("a1") unsigned char ** out)="\tjsr\t-7326(a6)";
#define i2d_PKCS12_MAC_DATA(a, out) __i2d_PKCS12_MAC_DATA(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS12_MAC_DATA_it(__reg("a6") struct Library *)="\tjsr\t-7332(a6)";
#define PKCS12_MAC_DATA_it() __PKCS12_MAC_DATA_it(AmiSSLBase)

PKCS12_SAFEBAG * __PKCS12_SAFEBAG_new(__reg("a6") struct Library *)="\tjsr\t-7338(a6)";
#define PKCS12_SAFEBAG_new() __PKCS12_SAFEBAG_new(AmiSSLBase)

void __PKCS12_SAFEBAG_free(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * a)="\tjsr\t-7344(a6)";
#define PKCS12_SAFEBAG_free(a) __PKCS12_SAFEBAG_free(AmiSSLBase, (a))

PKCS12_SAFEBAG * __d2i_PKCS12_SAFEBAG(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7350(a6)";
#define d2i_PKCS12_SAFEBAG(a, in, len) __d2i_PKCS12_SAFEBAG(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS12_SAFEBAG(__reg("a6") struct Library *, __reg("a0") PKCS12_SAFEBAG * a, __reg("a1") unsigned char ** out)="\tjsr\t-7356(a6)";
#define i2d_PKCS12_SAFEBAG(a, out) __i2d_PKCS12_SAFEBAG(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS12_SAFEBAG_it(__reg("a6") struct Library *)="\tjsr\t-7362(a6)";
#define PKCS12_SAFEBAG_it() __PKCS12_SAFEBAG_it(AmiSSLBase)

PKCS12_BAGS * __PKCS12_BAGS_new(__reg("a6") struct Library *)="\tjsr\t-7368(a6)";
#define PKCS12_BAGS_new() __PKCS12_BAGS_new(AmiSSLBase)

void __PKCS12_BAGS_free(__reg("a6") struct Library *, __reg("a0") PKCS12_BAGS * a)="\tjsr\t-7374(a6)";
#define PKCS12_BAGS_free(a) __PKCS12_BAGS_free(AmiSSLBase, (a))

PKCS12_BAGS * __d2i_PKCS12_BAGS(__reg("a6") struct Library *, __reg("a0") PKCS12_BAGS ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7380(a6)";
#define d2i_PKCS12_BAGS(a, in, len) __d2i_PKCS12_BAGS(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS12_BAGS(__reg("a6") struct Library *, __reg("a0") PKCS12_BAGS * a, __reg("a1") unsigned char ** out)="\tjsr\t-7386(a6)";
#define i2d_PKCS12_BAGS(a, out) __i2d_PKCS12_BAGS(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS12_BAGS_it(__reg("a6") struct Library *)="\tjsr\t-7392(a6)";
#define PKCS12_BAGS_it() __PKCS12_BAGS_it(AmiSSLBase)

const ASN1_ITEM * __PKCS12_SAFEBAGS_it(__reg("a6") struct Library *)="\tjsr\t-7398(a6)";
#define PKCS12_SAFEBAGS_it() __PKCS12_SAFEBAGS_it(AmiSSLBase)

const ASN1_ITEM * __PKCS12_AUTHSAFES_it(__reg("a6") struct Library *)="\tjsr\t-7404(a6)";
#define PKCS12_AUTHSAFES_it() __PKCS12_AUTHSAFES_it(AmiSSLBase)

void __PKCS12_PBE_add(__reg("a6") struct Library *)="\tjsr\t-7410(a6)";
#define PKCS12_PBE_add() __PKCS12_PBE_add(AmiSSLBase)

int __PKCS12_parse(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") const char * pass, __reg("a2") EVP_PKEY ** pkey, __reg("a3") X509 ** cert, __reg("d0") void ** ca)="\tjsr\t-7416(a6)";
#define PKCS12_parse(p12, pass, pkey, cert, ca) __PKCS12_parse(AmiSSLBase, (p12), (pass), (pkey), (cert), (ca))

PKCS12 * __PKCS12_create(__reg("a6") struct Library *, __reg("a0") char * pass, __reg("a1") char * name, __reg("a2") EVP_PKEY * pkey, __reg("a3") X509 * cert, __reg("d0") void * ca, __reg("d1") LONG nid_key, __reg("d2") LONG nid_cert, __reg("d3") LONG iter, __reg("d4") LONG mac_iter, __reg("d5") LONG keytype)="\tjsr\t-7422(a6)";
#define PKCS12_create(pass, name, pkey, cert, ca, nid_key, nid_cert, iter, mac_iter, keytype) __PKCS12_create(AmiSSLBase, (pass), (name), (pkey), (cert), (ca), (nid_key), (nid_cert), (iter), (mac_iter), (keytype))

int __i2d_PKCS12_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS12 * p12)="\tjsr\t-7428(a6)";
#define i2d_PKCS12_bio(bp, p12) __i2d_PKCS12_bio(AmiSSLBase, (bp), (p12))

PKCS12 * __d2i_PKCS12_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS12 ** p12)="\tjsr\t-7434(a6)";
#define d2i_PKCS12_bio(bp, p12) __d2i_PKCS12_bio(AmiSSLBase, (bp), (p12))

int __PKCS12_newpass(__reg("a6") struct Library *, __reg("a0") PKCS12 * p12, __reg("a1") char * oldpass, __reg("a2") char * newpass)="\tjsr\t-7440(a6)";
#define PKCS12_newpass(p12, oldpass, newpass) __PKCS12_newpass(AmiSSLBase, (p12), (oldpass), (newpass))

void __ERR_load_PKCS12_strings(__reg("a6") struct Library *)="\tjsr\t-7446(a6)";
#define ERR_load_PKCS12_strings() __ERR_load_PKCS12_strings(AmiSSLBase)

int __PKCS7_ISSUER_AND_SERIAL_digest(__reg("a6") struct Library *, __reg("a0") PKCS7_ISSUER_AND_SERIAL * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-7452(a6)";
#define PKCS7_ISSUER_AND_SERIAL_digest(data, type, md, len) __PKCS7_ISSUER_AND_SERIAL_digest(AmiSSLBase, (data), (type), (md), (len))

PKCS7 * __PKCS7_dup(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7)="\tjsr\t-7458(a6)";
#define PKCS7_dup(p7) __PKCS7_dup(AmiSSLBase, (p7))

PKCS7 * __d2i_PKCS7_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS7 ** p7)="\tjsr\t-7464(a6)";
#define d2i_PKCS7_bio(bp, p7) __d2i_PKCS7_bio(AmiSSLBase, (bp), (p7))

int __i2d_PKCS7_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS7 * p7)="\tjsr\t-7470(a6)";
#define i2d_PKCS7_bio(bp, p7) __i2d_PKCS7_bio(AmiSSLBase, (bp), (p7))

PKCS7_ISSUER_AND_SERIAL * __PKCS7_ISSUER_AND_SERIAL_new(__reg("a6") struct Library *)="\tjsr\t-7476(a6)";
#define PKCS7_ISSUER_AND_SERIAL_new() __PKCS7_ISSUER_AND_SERIAL_new(AmiSSLBase)

void __PKCS7_ISSUER_AND_SERIAL_free(__reg("a6") struct Library *, __reg("a0") PKCS7_ISSUER_AND_SERIAL * a)="\tjsr\t-7482(a6)";
#define PKCS7_ISSUER_AND_SERIAL_free(a) __PKCS7_ISSUER_AND_SERIAL_free(AmiSSLBase, (a))

PKCS7_ISSUER_AND_SERIAL * __d2i_PKCS7_ISSUER_AND_SERIAL(__reg("a6") struct Library *, __reg("a0") PKCS7_ISSUER_AND_SERIAL ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7488(a6)";
#define d2i_PKCS7_ISSUER_AND_SERIAL(a, in, len) __d2i_PKCS7_ISSUER_AND_SERIAL(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_ISSUER_AND_SERIAL(__reg("a6") struct Library *, __reg("a0") PKCS7_ISSUER_AND_SERIAL * a, __reg("a1") unsigned char ** out)="\tjsr\t-7494(a6)";
#define i2d_PKCS7_ISSUER_AND_SERIAL(a, out) __i2d_PKCS7_ISSUER_AND_SERIAL(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_ISSUER_AND_SERIAL_it(__reg("a6") struct Library *)="\tjsr\t-7500(a6)";
#define PKCS7_ISSUER_AND_SERIAL_it() __PKCS7_ISSUER_AND_SERIAL_it(AmiSSLBase)

PKCS7_SIGNER_INFO * __PKCS7_SIGNER_INFO_new(__reg("a6") struct Library *)="\tjsr\t-7506(a6)";
#define PKCS7_SIGNER_INFO_new() __PKCS7_SIGNER_INFO_new(AmiSSLBase)

void __PKCS7_SIGNER_INFO_free(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * a)="\tjsr\t-7512(a6)";
#define PKCS7_SIGNER_INFO_free(a) __PKCS7_SIGNER_INFO_free(AmiSSLBase, (a))

PKCS7_SIGNER_INFO * __d2i_PKCS7_SIGNER_INFO(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7518(a6)";
#define d2i_PKCS7_SIGNER_INFO(a, in, len) __d2i_PKCS7_SIGNER_INFO(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_SIGNER_INFO(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-7524(a6)";
#define i2d_PKCS7_SIGNER_INFO(a, out) __i2d_PKCS7_SIGNER_INFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_SIGNER_INFO_it(__reg("a6") struct Library *)="\tjsr\t-7530(a6)";
#define PKCS7_SIGNER_INFO_it() __PKCS7_SIGNER_INFO_it(AmiSSLBase)

PKCS7_RECIP_INFO * __PKCS7_RECIP_INFO_new(__reg("a6") struct Library *)="\tjsr\t-7536(a6)";
#define PKCS7_RECIP_INFO_new() __PKCS7_RECIP_INFO_new(AmiSSLBase)

void __PKCS7_RECIP_INFO_free(__reg("a6") struct Library *, __reg("a0") PKCS7_RECIP_INFO * a)="\tjsr\t-7542(a6)";
#define PKCS7_RECIP_INFO_free(a) __PKCS7_RECIP_INFO_free(AmiSSLBase, (a))

PKCS7_RECIP_INFO * __d2i_PKCS7_RECIP_INFO(__reg("a6") struct Library *, __reg("a0") PKCS7_RECIP_INFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7548(a6)";
#define d2i_PKCS7_RECIP_INFO(a, in, len) __d2i_PKCS7_RECIP_INFO(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_RECIP_INFO(__reg("a6") struct Library *, __reg("a0") PKCS7_RECIP_INFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-7554(a6)";
#define i2d_PKCS7_RECIP_INFO(a, out) __i2d_PKCS7_RECIP_INFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_RECIP_INFO_it(__reg("a6") struct Library *)="\tjsr\t-7560(a6)";
#define PKCS7_RECIP_INFO_it() __PKCS7_RECIP_INFO_it(AmiSSLBase)

PKCS7_SIGNED * __PKCS7_SIGNED_new(__reg("a6") struct Library *)="\tjsr\t-7566(a6)";
#define PKCS7_SIGNED_new() __PKCS7_SIGNED_new(AmiSSLBase)

void __PKCS7_SIGNED_free(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNED * a)="\tjsr\t-7572(a6)";
#define PKCS7_SIGNED_free(a) __PKCS7_SIGNED_free(AmiSSLBase, (a))

PKCS7_SIGNED * __d2i_PKCS7_SIGNED(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNED ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7578(a6)";
#define d2i_PKCS7_SIGNED(a, in, len) __d2i_PKCS7_SIGNED(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_SIGNED(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNED * a, __reg("a1") unsigned char ** out)="\tjsr\t-7584(a6)";
#define i2d_PKCS7_SIGNED(a, out) __i2d_PKCS7_SIGNED(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_SIGNED_it(__reg("a6") struct Library *)="\tjsr\t-7590(a6)";
#define PKCS7_SIGNED_it() __PKCS7_SIGNED_it(AmiSSLBase)

PKCS7_ENC_CONTENT * __PKCS7_ENC_CONTENT_new(__reg("a6") struct Library *)="\tjsr\t-7596(a6)";
#define PKCS7_ENC_CONTENT_new() __PKCS7_ENC_CONTENT_new(AmiSSLBase)

void __PKCS7_ENC_CONTENT_free(__reg("a6") struct Library *, __reg("a0") PKCS7_ENC_CONTENT * a)="\tjsr\t-7602(a6)";
#define PKCS7_ENC_CONTENT_free(a) __PKCS7_ENC_CONTENT_free(AmiSSLBase, (a))

PKCS7_ENC_CONTENT * __d2i_PKCS7_ENC_CONTENT(__reg("a6") struct Library *, __reg("a0") PKCS7_ENC_CONTENT ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7608(a6)";
#define d2i_PKCS7_ENC_CONTENT(a, in, len) __d2i_PKCS7_ENC_CONTENT(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_ENC_CONTENT(__reg("a6") struct Library *, __reg("a0") PKCS7_ENC_CONTENT * a, __reg("a1") unsigned char ** out)="\tjsr\t-7614(a6)";
#define i2d_PKCS7_ENC_CONTENT(a, out) __i2d_PKCS7_ENC_CONTENT(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_ENC_CONTENT_it(__reg("a6") struct Library *)="\tjsr\t-7620(a6)";
#define PKCS7_ENC_CONTENT_it() __PKCS7_ENC_CONTENT_it(AmiSSLBase)

PKCS7_ENVELOPE * __PKCS7_ENVELOPE_new(__reg("a6") struct Library *)="\tjsr\t-7626(a6)";
#define PKCS7_ENVELOPE_new() __PKCS7_ENVELOPE_new(AmiSSLBase)

void __PKCS7_ENVELOPE_free(__reg("a6") struct Library *, __reg("a0") PKCS7_ENVELOPE * a)="\tjsr\t-7632(a6)";
#define PKCS7_ENVELOPE_free(a) __PKCS7_ENVELOPE_free(AmiSSLBase, (a))

PKCS7_ENVELOPE * __d2i_PKCS7_ENVELOPE(__reg("a6") struct Library *, __reg("a0") PKCS7_ENVELOPE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7638(a6)";
#define d2i_PKCS7_ENVELOPE(a, in, len) __d2i_PKCS7_ENVELOPE(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_ENVELOPE(__reg("a6") struct Library *, __reg("a0") PKCS7_ENVELOPE * a, __reg("a1") unsigned char ** out)="\tjsr\t-7644(a6)";
#define i2d_PKCS7_ENVELOPE(a, out) __i2d_PKCS7_ENVELOPE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_ENVELOPE_it(__reg("a6") struct Library *)="\tjsr\t-7650(a6)";
#define PKCS7_ENVELOPE_it() __PKCS7_ENVELOPE_it(AmiSSLBase)

PKCS7_SIGN_ENVELOPE * __PKCS7_SIGN_ENVELOPE_new(__reg("a6") struct Library *)="\tjsr\t-7656(a6)";
#define PKCS7_SIGN_ENVELOPE_new() __PKCS7_SIGN_ENVELOPE_new(AmiSSLBase)

void __PKCS7_SIGN_ENVELOPE_free(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGN_ENVELOPE * a)="\tjsr\t-7662(a6)";
#define PKCS7_SIGN_ENVELOPE_free(a) __PKCS7_SIGN_ENVELOPE_free(AmiSSLBase, (a))

PKCS7_SIGN_ENVELOPE * __d2i_PKCS7_SIGN_ENVELOPE(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGN_ENVELOPE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7668(a6)";
#define d2i_PKCS7_SIGN_ENVELOPE(a, in, len) __d2i_PKCS7_SIGN_ENVELOPE(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_SIGN_ENVELOPE(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGN_ENVELOPE * a, __reg("a1") unsigned char ** out)="\tjsr\t-7674(a6)";
#define i2d_PKCS7_SIGN_ENVELOPE(a, out) __i2d_PKCS7_SIGN_ENVELOPE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_SIGN_ENVELOPE_it(__reg("a6") struct Library *)="\tjsr\t-7680(a6)";
#define PKCS7_SIGN_ENVELOPE_it() __PKCS7_SIGN_ENVELOPE_it(AmiSSLBase)

PKCS7_DIGEST * __PKCS7_DIGEST_new(__reg("a6") struct Library *)="\tjsr\t-7686(a6)";
#define PKCS7_DIGEST_new() __PKCS7_DIGEST_new(AmiSSLBase)

void __PKCS7_DIGEST_free(__reg("a6") struct Library *, __reg("a0") PKCS7_DIGEST * a)="\tjsr\t-7692(a6)";
#define PKCS7_DIGEST_free(a) __PKCS7_DIGEST_free(AmiSSLBase, (a))

PKCS7_DIGEST * __d2i_PKCS7_DIGEST(__reg("a6") struct Library *, __reg("a0") PKCS7_DIGEST ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7698(a6)";
#define d2i_PKCS7_DIGEST(a, in, len) __d2i_PKCS7_DIGEST(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_DIGEST(__reg("a6") struct Library *, __reg("a0") PKCS7_DIGEST * a, __reg("a1") unsigned char ** out)="\tjsr\t-7704(a6)";
#define i2d_PKCS7_DIGEST(a, out) __i2d_PKCS7_DIGEST(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_DIGEST_it(__reg("a6") struct Library *)="\tjsr\t-7710(a6)";
#define PKCS7_DIGEST_it() __PKCS7_DIGEST_it(AmiSSLBase)

PKCS7_ENCRYPT * __PKCS7_ENCRYPT_new(__reg("a6") struct Library *)="\tjsr\t-7716(a6)";
#define PKCS7_ENCRYPT_new() __PKCS7_ENCRYPT_new(AmiSSLBase)

void __PKCS7_ENCRYPT_free(__reg("a6") struct Library *, __reg("a0") PKCS7_ENCRYPT * a)="\tjsr\t-7722(a6)";
#define PKCS7_ENCRYPT_free(a) __PKCS7_ENCRYPT_free(AmiSSLBase, (a))

PKCS7_ENCRYPT * __d2i_PKCS7_ENCRYPT(__reg("a6") struct Library *, __reg("a0") PKCS7_ENCRYPT ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7728(a6)";
#define d2i_PKCS7_ENCRYPT(a, in, len) __d2i_PKCS7_ENCRYPT(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7_ENCRYPT(__reg("a6") struct Library *, __reg("a0") PKCS7_ENCRYPT * a, __reg("a1") unsigned char ** out)="\tjsr\t-7734(a6)";
#define i2d_PKCS7_ENCRYPT(a, out) __i2d_PKCS7_ENCRYPT(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_ENCRYPT_it(__reg("a6") struct Library *)="\tjsr\t-7740(a6)";
#define PKCS7_ENCRYPT_it() __PKCS7_ENCRYPT_it(AmiSSLBase)

PKCS7 * __PKCS7_new(__reg("a6") struct Library *)="\tjsr\t-7746(a6)";
#define PKCS7_new() __PKCS7_new(AmiSSLBase)

void __PKCS7_free(__reg("a6") struct Library *, __reg("a0") PKCS7 * a)="\tjsr\t-7752(a6)";
#define PKCS7_free(a) __PKCS7_free(AmiSSLBase, (a))

PKCS7 * __d2i_PKCS7(__reg("a6") struct Library *, __reg("a0") PKCS7 ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-7758(a6)";
#define d2i_PKCS7(a, in, len) __d2i_PKCS7(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS7(__reg("a6") struct Library *, __reg("a0") PKCS7 * a, __reg("a1") unsigned char ** out)="\tjsr\t-7764(a6)";
#define i2d_PKCS7(a, out) __i2d_PKCS7(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS7_it(__reg("a6") struct Library *)="\tjsr\t-7770(a6)";
#define PKCS7_it() __PKCS7_it(AmiSSLBase)

const ASN1_ITEM * __PKCS7_ATTR_SIGN_it(__reg("a6") struct Library *)="\tjsr\t-7776(a6)";
#define PKCS7_ATTR_SIGN_it() __PKCS7_ATTR_SIGN_it(AmiSSLBase)

const ASN1_ITEM * __PKCS7_ATTR_VERIFY_it(__reg("a6") struct Library *)="\tjsr\t-7782(a6)";
#define PKCS7_ATTR_VERIFY_it() __PKCS7_ATTR_VERIFY_it(AmiSSLBase)

long __PKCS7_ctrl(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("a1") char * parg)="\tjsr\t-7788(a6)";
#define PKCS7_ctrl(p7, cmd, larg, parg) __PKCS7_ctrl(AmiSSLBase, (p7), (cmd), (larg), (parg))

int __PKCS7_set_type(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("d0") LONG type)="\tjsr\t-7794(a6)";
#define PKCS7_set_type(p7, type) __PKCS7_set_type(AmiSSLBase, (p7), (type))

int __PKCS7_set_content(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") PKCS7 * p7_data)="\tjsr\t-7800(a6)";
#define PKCS7_set_content(p7, p7_data) __PKCS7_set_content(AmiSSLBase, (p7), (p7_data))

int __PKCS7_SIGNER_INFO_set(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * p7i, __reg("a1") X509 * x509, __reg("a2") EVP_PKEY * pkey, __reg("a3") const EVP_MD * dgst)="\tjsr\t-7806(a6)";
#define PKCS7_SIGNER_INFO_set(p7i, x509, pkey, dgst) __PKCS7_SIGNER_INFO_set(AmiSSLBase, (p7i), (x509), (pkey), (dgst))

int __PKCS7_add_signer(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") PKCS7_SIGNER_INFO * p7i)="\tjsr\t-7812(a6)";
#define PKCS7_add_signer(p7, p7i) __PKCS7_add_signer(AmiSSLBase, (p7), (p7i))

int __PKCS7_add_certificate(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") X509 * x509)="\tjsr\t-7818(a6)";
#define PKCS7_add_certificate(p7, x509) __PKCS7_add_certificate(AmiSSLBase, (p7), (x509))

int __PKCS7_add_crl(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") X509_CRL * x509)="\tjsr\t-7824(a6)";
#define PKCS7_add_crl(p7, x509) __PKCS7_add_crl(AmiSSLBase, (p7), (x509))

int __PKCS7_content_new(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("d0") LONG nid)="\tjsr\t-7830(a6)";
#define PKCS7_content_new(p7, nid) __PKCS7_content_new(AmiSSLBase, (p7), (nid))

int __PKCS7_dataVerify(__reg("a6") struct Library *, __reg("a0") X509_STORE * cert_store, __reg("a1") X509_STORE_CTX * ctx, __reg("a2") BIO * bio, __reg("a3") PKCS7 * p7, __reg("d0") PKCS7_SIGNER_INFO * si)="\tjsr\t-7836(a6)";
#define PKCS7_dataVerify(cert_store, ctx, bio, p7, si) __PKCS7_dataVerify(AmiSSLBase, (cert_store), (ctx), (bio), (p7), (si))

int __PKCS7_signatureVerify(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") PKCS7 * p7, __reg("a2") PKCS7_SIGNER_INFO * si, __reg("a3") X509 * x509)="\tjsr\t-7842(a6)";
#define PKCS7_signatureVerify(bio, p7, si, x509) __PKCS7_signatureVerify(AmiSSLBase, (bio), (p7), (si), (x509))

BIO * __PKCS7_dataInit(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") BIO * bio)="\tjsr\t-7848(a6)";
#define PKCS7_dataInit(p7, bio) __PKCS7_dataInit(AmiSSLBase, (p7), (bio))

int __PKCS7_dataFinal(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") BIO * bio)="\tjsr\t-7854(a6)";
#define PKCS7_dataFinal(p7, bio) __PKCS7_dataFinal(AmiSSLBase, (p7), (bio))

BIO * __PKCS7_dataDecode(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") EVP_PKEY * pkey, __reg("a2") BIO * in_bio, __reg("a3") X509 * pcert)="\tjsr\t-7860(a6)";
#define PKCS7_dataDecode(p7, pkey, in_bio, pcert) __PKCS7_dataDecode(AmiSSLBase, (p7), (pkey), (in_bio), (pcert))

PKCS7_SIGNER_INFO * __PKCS7_add_signature(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") X509 * x509, __reg("a2") EVP_PKEY * pkey, __reg("a3") const EVP_MD * dgst)="\tjsr\t-7866(a6)";
#define PKCS7_add_signature(p7, x509, pkey, dgst) __PKCS7_add_signature(AmiSSLBase, (p7), (x509), (pkey), (dgst))

X509 * __PKCS7_cert_from_signer_info(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") PKCS7_SIGNER_INFO * si)="\tjsr\t-7872(a6)";
#define PKCS7_cert_from_signer_info(p7, si) __PKCS7_cert_from_signer_info(AmiSSLBase, (p7), (si))

void * __PKCS7_get_signer_info(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7)="\tjsr\t-7878(a6)";
#define PKCS7_get_signer_info(p7) __PKCS7_get_signer_info(AmiSSLBase, (p7))

PKCS7_RECIP_INFO * __PKCS7_add_recipient(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") X509 * x509)="\tjsr\t-7884(a6)";
#define PKCS7_add_recipient(p7, x509) __PKCS7_add_recipient(AmiSSLBase, (p7), (x509))

int __PKCS7_add_recipient_info(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") PKCS7_RECIP_INFO * ri)="\tjsr\t-7890(a6)";
#define PKCS7_add_recipient_info(p7, ri) __PKCS7_add_recipient_info(AmiSSLBase, (p7), (ri))

int __PKCS7_RECIP_INFO_set(__reg("a6") struct Library *, __reg("a0") PKCS7_RECIP_INFO * p7i, __reg("a1") X509 * x509)="\tjsr\t-7896(a6)";
#define PKCS7_RECIP_INFO_set(p7i, x509) __PKCS7_RECIP_INFO_set(AmiSSLBase, (p7i), (x509))

int __PKCS7_set_cipher(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") const EVP_CIPHER * cipher)="\tjsr\t-7902(a6)";
#define PKCS7_set_cipher(p7, cipher) __PKCS7_set_cipher(AmiSSLBase, (p7), (cipher))

PKCS7_ISSUER_AND_SERIAL * __PKCS7_get_issuer_and_serial(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("d0") LONG idx)="\tjsr\t-7908(a6)";
#define PKCS7_get_issuer_and_serial(p7, idx) __PKCS7_get_issuer_and_serial(AmiSSLBase, (p7), (idx))

ASN1_OCTET_STRING * __PKCS7_digest_from_attributes(__reg("a6") struct Library *, __reg("a0") void * sk)="\tjsr\t-7914(a6)";
#define PKCS7_digest_from_attributes(sk) __PKCS7_digest_from_attributes(AmiSSLBase, (sk))

int __PKCS7_add_signed_attribute(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * p7si, __reg("d0") LONG nid, __reg("d1") LONG type, __reg("a1") void * data)="\tjsr\t-7920(a6)";
#define PKCS7_add_signed_attribute(p7si, nid, type, data) __PKCS7_add_signed_attribute(AmiSSLBase, (p7si), (nid), (type), (data))

int __PKCS7_add_attribute(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * p7si, __reg("d0") LONG nid, __reg("d1") LONG atrtype, __reg("a1") void * value)="\tjsr\t-7926(a6)";
#define PKCS7_add_attribute(p7si, nid, atrtype, value) __PKCS7_add_attribute(AmiSSLBase, (p7si), (nid), (atrtype), (value))

ASN1_TYPE * __PKCS7_get_attribute(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * si, __reg("d0") LONG nid)="\tjsr\t-7932(a6)";
#define PKCS7_get_attribute(si, nid) __PKCS7_get_attribute(AmiSSLBase, (si), (nid))

ASN1_TYPE * __PKCS7_get_signed_attribute(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * si, __reg("d0") LONG nid)="\tjsr\t-7938(a6)";
#define PKCS7_get_signed_attribute(si, nid) __PKCS7_get_signed_attribute(AmiSSLBase, (si), (nid))

int __PKCS7_set_signed_attributes(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * p7si, __reg("a1") void * sk)="\tjsr\t-7944(a6)";
#define PKCS7_set_signed_attributes(p7si, sk) __PKCS7_set_signed_attributes(AmiSSLBase, (p7si), (sk))

int __PKCS7_set_attributes(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * p7si, __reg("a1") void * sk)="\tjsr\t-7950(a6)";
#define PKCS7_set_attributes(p7si, sk) __PKCS7_set_attributes(AmiSSLBase, (p7si), (sk))

PKCS7 * __PKCS7_sign(__reg("a6") struct Library *, __reg("a0") X509 * signcert, __reg("a1") EVP_PKEY * pkey, __reg("a2") void * certs, __reg("a3") BIO * data, __reg("d0") LONG flags)="\tjsr\t-7956(a6)";
#define PKCS7_sign(signcert, pkey, certs, data, flags) __PKCS7_sign(AmiSSLBase, (signcert), (pkey), (certs), (data), (flags))

int __PKCS7_verify(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") void * certs, __reg("a2") X509_STORE * store, __reg("a3") BIO * indata, __reg("d0") BIO * out, __reg("d1") LONG flags)="\tjsr\t-7962(a6)";
#define PKCS7_verify(p7, certs, store, indata, out, flags) __PKCS7_verify(AmiSSLBase, (p7), (certs), (store), (indata), (out), (flags))

void * __PKCS7_get0_signers(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") void * certs, __reg("d0") LONG flags)="\tjsr\t-7968(a6)";
#define PKCS7_get0_signers(p7, certs, flags) __PKCS7_get0_signers(AmiSSLBase, (p7), (certs), (flags))

PKCS7 * __PKCS7_encrypt(__reg("a6") struct Library *, __reg("a0") void * certs, __reg("a1") BIO * in, __reg("a2") const EVP_CIPHER * cipher, __reg("d0") LONG flags)="\tjsr\t-7974(a6)";
#define PKCS7_encrypt(certs, in, cipher, flags) __PKCS7_encrypt(AmiSSLBase, (certs), (in), (cipher), (flags))

int __PKCS7_decrypt(__reg("a6") struct Library *, __reg("a0") PKCS7 * p7, __reg("a1") EVP_PKEY * pkey, __reg("a2") X509 * cert, __reg("a3") BIO * data, __reg("d0") LONG flags)="\tjsr\t-7980(a6)";
#define PKCS7_decrypt(p7, pkey, cert, data, flags) __PKCS7_decrypt(AmiSSLBase, (p7), (pkey), (cert), (data), (flags))

int __PKCS7_add_attrib_smimecap(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * si, __reg("a1") void * cap)="\tjsr\t-7986(a6)";
#define PKCS7_add_attrib_smimecap(si, cap) __PKCS7_add_attrib_smimecap(AmiSSLBase, (si), (cap))

void * __PKCS7_get_smimecap(__reg("a6") struct Library *, __reg("a0") PKCS7_SIGNER_INFO * si)="\tjsr\t-7992(a6)";
#define PKCS7_get_smimecap(si) __PKCS7_get_smimecap(AmiSSLBase, (si))

int __PKCS7_simple_smimecap(__reg("a6") struct Library *, __reg("a0") void * sk, __reg("d0") LONG nid, __reg("d1") LONG arg)="\tjsr\t-7998(a6)";
#define PKCS7_simple_smimecap(sk, nid, arg) __PKCS7_simple_smimecap(AmiSSLBase, (sk), (nid), (arg))

int __SMIME_write_PKCS7(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") PKCS7 * p7, __reg("a2") BIO * data, __reg("d0") LONG flags)="\tjsr\t-8004(a6)";
#define SMIME_write_PKCS7(bio, p7, data, flags) __SMIME_write_PKCS7(AmiSSLBase, (bio), (p7), (data), (flags))

PKCS7 * __SMIME_read_PKCS7(__reg("a6") struct Library *, __reg("a0") BIO * bio, __reg("a1") BIO ** bcont)="\tjsr\t-8010(a6)";
#define SMIME_read_PKCS7(bio, bcont) __SMIME_read_PKCS7(AmiSSLBase, (bio), (bcont))

int __SMIME_crlf_copy(__reg("a6") struct Library *, __reg("a0") BIO * in, __reg("a1") BIO * out, __reg("d0") LONG flags)="\tjsr\t-8016(a6)";
#define SMIME_crlf_copy(in, out, flags) __SMIME_crlf_copy(AmiSSLBase, (in), (out), (flags))

int __SMIME_text(__reg("a6") struct Library *, __reg("a0") BIO * in, __reg("a1") BIO * out)="\tjsr\t-8022(a6)";
#define SMIME_text(in, out) __SMIME_text(AmiSSLBase, (in), (out))

void __ERR_load_PKCS7_strings(__reg("a6") struct Library *)="\tjsr\t-8028(a6)";
#define ERR_load_PKCS7_strings() __ERR_load_PKCS7_strings(AmiSSLBase)

int __RAND_set_rand_method(__reg("a6") struct Library *, __reg("a0") const RAND_METHOD * meth)="\tjsr\t-8034(a6)";
#define RAND_set_rand_method(meth) __RAND_set_rand_method(AmiSSLBase, (meth))

const RAND_METHOD * __RAND_get_rand_method(__reg("a6") struct Library *)="\tjsr\t-8040(a6)";
#define RAND_get_rand_method() __RAND_get_rand_method(AmiSSLBase)

RAND_METHOD * __RAND_SSLeay(__reg("a6") struct Library *)="\tjsr\t-8046(a6)";
#define RAND_SSLeay() __RAND_SSLeay(AmiSSLBase)

void __RAND_cleanup(__reg("a6") struct Library *)="\tjsr\t-8052(a6)";
#define RAND_cleanup() __RAND_cleanup(AmiSSLBase)

int __RAND_bytes(__reg("a6") struct Library *, __reg("a0") unsigned char * buf, __reg("d0") LONG num)="\tjsr\t-8058(a6)";
#define RAND_bytes(buf, num) __RAND_bytes(AmiSSLBase, (buf), (num))

int __RAND_pseudo_bytes(__reg("a6") struct Library *, __reg("a0") unsigned char * buf, __reg("d0") LONG num)="\tjsr\t-8064(a6)";
#define RAND_pseudo_bytes(buf, num) __RAND_pseudo_bytes(AmiSSLBase, (buf), (num))

void __RAND_seed(__reg("a6") struct Library *, __reg("a0") const void * buf, __reg("d0") LONG num)="\tjsr\t-8070(a6)";
#define RAND_seed(buf, num) __RAND_seed(AmiSSLBase, (buf), (num))

void __RAND_add(__reg("a6") struct Library *, __reg("a0") const void * buf, __reg("d0") LONG num, __reg("d1") float entropy)="\tjsr\t-8076(a6)";
#define RAND_add(buf, num, entropy) __RAND_add(AmiSSLBase, (buf), (num), (entropy))

int __RAND_load_file(__reg("a6") struct Library *, __reg("a0") const char * file, __reg("d0") long max_bytes)="\tjsr\t-8082(a6)";
#define RAND_load_file(file, max_bytes) __RAND_load_file(AmiSSLBase, (file), (max_bytes))

int __RAND_write_file(__reg("a6") struct Library *, __reg("a0") const char * file)="\tjsr\t-8088(a6)";
#define RAND_write_file(file) __RAND_write_file(AmiSSLBase, (file))

const char * __RAND_file_name(__reg("a6") struct Library *, __reg("a0") char * file, __reg("d0") ULONG num)="\tjsr\t-8094(a6)";
#define RAND_file_name(file, num) __RAND_file_name(AmiSSLBase, (file), (num))

int __RAND_status(__reg("a6") struct Library *)="\tjsr\t-8100(a6)";
#define RAND_status() __RAND_status(AmiSSLBase)

int __RAND_query_egd_bytes(__reg("a6") struct Library *, __reg("a0") const char * path, __reg("a1") unsigned char * buf, __reg("d0") LONG bytes)="\tjsr\t-8106(a6)";
#define RAND_query_egd_bytes(path, buf, bytes) __RAND_query_egd_bytes(AmiSSLBase, (path), (buf), (bytes))

int __RAND_egd(__reg("a6") struct Library *, __reg("a0") const char * path)="\tjsr\t-8112(a6)";
#define RAND_egd(path) __RAND_egd(AmiSSLBase, (path))

int __RAND_egd_bytes(__reg("a6") struct Library *, __reg("a0") const char * path, __reg("d0") LONG bytes)="\tjsr\t-8118(a6)";
#define RAND_egd_bytes(path, bytes) __RAND_egd_bytes(AmiSSLBase, (path), (bytes))

int __RAND_poll(__reg("a6") struct Library *)="\tjsr\t-8124(a6)";
#define RAND_poll() __RAND_poll(AmiSSLBase)

void __ERR_load_RAND_strings(__reg("a6") struct Library *)="\tjsr\t-8130(a6)";
#define ERR_load_RAND_strings() __ERR_load_RAND_strings(AmiSSLBase)

void __SSL_CTX_set_msg_callback(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") void (*cb)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))="\tjsr\t-8136(a6)";
#define SSL_CTX_set_msg_callback(ctx, cb) __SSL_CTX_set_msg_callback(AmiSSLBase, (ctx), (cb))

void __SSL_set_msg_callback(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") void (*cb)(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg))="\tjsr\t-8142(a6)";
#define SSL_set_msg_callback(ssl, cb) __SSL_set_msg_callback(AmiSSLBase, (ssl), (cb))

struct lhash_st * __SSL_CTX_sessions(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx)="\tjsr\t-8148(a6)";
#define SSL_CTX_sessions(ctx) __SSL_CTX_sessions(AmiSSLBase, (ctx))

size_t __SSL_get_finished(__reg("a6") struct Library *, __reg("a0") const SSL * s, __reg("a1") void * buf, __reg("d0") ULONG count)="\tjsr\t-8154(a6)";
#define SSL_get_finished(s, buf, count) __SSL_get_finished(AmiSSLBase, (s), (buf), (count))

size_t __SSL_get_peer_finished(__reg("a6") struct Library *, __reg("a0") const SSL * s, __reg("a1") void * buf, __reg("d0") ULONG count)="\tjsr\t-8160(a6)";
#define SSL_get_peer_finished(s, buf, count) __SSL_get_peer_finished(AmiSSLBase, (s), (buf), (count))

BIO_METHOD * __BIO_f_ssl(__reg("a6") struct Library *)="\tjsr\t-8166(a6)";
#define BIO_f_ssl() __BIO_f_ssl(AmiSSLBase)

BIO * __BIO_new_ssl(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG client)="\tjsr\t-8172(a6)";
#define BIO_new_ssl(ctx, client) __BIO_new_ssl(AmiSSLBase, (ctx), (client))

BIO * __BIO_new_ssl_connect(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx)="\tjsr\t-8178(a6)";
#define BIO_new_ssl_connect(ctx) __BIO_new_ssl_connect(AmiSSLBase, (ctx))

BIO * __BIO_new_buffer_ssl_connect(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx)="\tjsr\t-8184(a6)";
#define BIO_new_buffer_ssl_connect(ctx) __BIO_new_buffer_ssl_connect(AmiSSLBase, (ctx))

int __BIO_ssl_copy_session_id(__reg("a6") struct Library *, __reg("a0") BIO * to, __reg("a1") BIO * from)="\tjsr\t-8190(a6)";
#define BIO_ssl_copy_session_id(to, from) __BIO_ssl_copy_session_id(AmiSSLBase, (to), (from))

void __BIO_ssl_shutdown(__reg("a6") struct Library *, __reg("a0") BIO * ssl_bio)="\tjsr\t-8196(a6)";
#define BIO_ssl_shutdown(ssl_bio) __BIO_ssl_shutdown(AmiSSLBase, (ssl_bio))

int __SSL_CTX_set_cipher_list(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a1, __reg("a1") const char * str)="\tjsr\t-8202(a6)";
#define SSL_CTX_set_cipher_list(a1, str) __SSL_CTX_set_cipher_list(AmiSSLBase, (a1), (str))

SSL_CTX * __SSL_CTX_new(__reg("a6") struct Library *, __reg("a0") SSL_METHOD * meth)="\tjsr\t-8208(a6)";
#define SSL_CTX_new(meth) __SSL_CTX_new(AmiSSLBase, (meth))

void __SSL_CTX_free(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a)="\tjsr\t-8214(a6)";
#define SSL_CTX_free(a) __SSL_CTX_free(AmiSSLBase, (a))

long __SSL_CTX_set_timeout(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") long t)="\tjsr\t-8220(a6)";
#define SSL_CTX_set_timeout(ctx, t) __SSL_CTX_set_timeout(AmiSSLBase, (ctx), (t))

long __SSL_CTX_get_timeout(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-8226(a6)";
#define SSL_CTX_get_timeout(ctx) __SSL_CTX_get_timeout(AmiSSLBase, (ctx))

X509_STORE * __SSL_CTX_get_cert_store(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * a)="\tjsr\t-8232(a6)";
#define SSL_CTX_get_cert_store(a) __SSL_CTX_get_cert_store(AmiSSLBase, (a))

void __SSL_CTX_set_cert_store(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a, __reg("a1") X509_STORE * b)="\tjsr\t-8238(a6)";
#define SSL_CTX_set_cert_store(a, b) __SSL_CTX_set_cert_store(AmiSSLBase, (a), (b))

int __SSL_want(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8244(a6)";
#define SSL_want(s) __SSL_want(AmiSSLBase, (s))

int __SSL_clear(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-8250(a6)";
#define SSL_clear(s) __SSL_clear(AmiSSLBase, (s))

void __SSL_CTX_flush_sessions(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") long tm)="\tjsr\t-8256(a6)";
#define SSL_CTX_flush_sessions(ctx, tm) __SSL_CTX_flush_sessions(AmiSSLBase, (ctx), (tm))

SSL_CIPHER * __SSL_get_current_cipher(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8262(a6)";
#define SSL_get_current_cipher(s) __SSL_get_current_cipher(AmiSSLBase, (s))

int __SSL_CIPHER_get_bits(__reg("a6") struct Library *, __reg("a0") const SSL_CIPHER * c, __reg("a1") int * alg_bits)="\tjsr\t-8268(a6)";
#define SSL_CIPHER_get_bits(c, alg_bits) __SSL_CIPHER_get_bits(AmiSSLBase, (c), (alg_bits))

char * __SSL_CIPHER_get_version(__reg("a6") struct Library *, __reg("a0") const SSL_CIPHER * c)="\tjsr\t-8274(a6)";
#define SSL_CIPHER_get_version(c) __SSL_CIPHER_get_version(AmiSSLBase, (c))

const char * __SSL_CIPHER_get_name(__reg("a6") struct Library *, __reg("a0") const SSL_CIPHER * c)="\tjsr\t-8280(a6)";
#define SSL_CIPHER_get_name(c) __SSL_CIPHER_get_name(AmiSSLBase, (c))

const char * __SSL_CIPHER_get_mac(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * cipher)="\tjsr\t-8286(a6)";
#define SSL_CIPHER_get_mac(cipher) __SSL_CIPHER_get_mac(AmiSSLBase, (cipher))

const char * __SSL_CIPHER_get_encryption(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * cipher)="\tjsr\t-8292(a6)";
#define SSL_CIPHER_get_encryption(cipher) __SSL_CIPHER_get_encryption(AmiSSLBase, (cipher))

const char * __SSL_CIPHER_get_authentication(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * cipher)="\tjsr\t-8298(a6)";
#define SSL_CIPHER_get_authentication(cipher) __SSL_CIPHER_get_authentication(AmiSSLBase, (cipher))

const char * __SSL_CIPHER_get_key_exchange(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * cipher)="\tjsr\t-8304(a6)";
#define SSL_CIPHER_get_key_exchange(cipher) __SSL_CIPHER_get_key_exchange(AmiSSLBase, (cipher))

const char * __SSL_CIPHER_get_export(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * cipher)="\tjsr\t-8310(a6)";
#define SSL_CIPHER_get_export(cipher) __SSL_CIPHER_get_export(AmiSSLBase, (cipher))

int __SSL_get_fd(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8316(a6)";
#define SSL_get_fd(s) __SSL_get_fd(AmiSSLBase, (s))

int __SSL_get_rfd(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8322(a6)";
#define SSL_get_rfd(s) __SSL_get_rfd(AmiSSLBase, (s))

int __SSL_get_wfd(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8328(a6)";
#define SSL_get_wfd(s) __SSL_get_wfd(AmiSSLBase, (s))

const char * __SSL_get_cipher_list(__reg("a6") struct Library *, __reg("a0") const SSL * s, __reg("d0") LONG n)="\tjsr\t-8334(a6)";
#define SSL_get_cipher_list(s, n) __SSL_get_cipher_list(AmiSSLBase, (s), (n))

char * __SSL_get_shared_ciphers(__reg("a6") struct Library *, __reg("a0") const SSL * s, __reg("a1") char * buf, __reg("d0") LONG len)="\tjsr\t-8340(a6)";
#define SSL_get_shared_ciphers(s, buf, len) __SSL_get_shared_ciphers(AmiSSLBase, (s), (buf), (len))

int __SSL_get_read_ahead(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8346(a6)";
#define SSL_get_read_ahead(s) __SSL_get_read_ahead(AmiSSLBase, (s))

int __SSL_pending(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8352(a6)";
#define SSL_pending(s) __SSL_pending(AmiSSLBase, (s))

int __SSL_set_fd(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG fd)="\tjsr\t-8358(a6)";
#define SSL_set_fd(s, fd) __SSL_set_fd(AmiSSLBase, (s), (fd))

int __SSL_set_rfd(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG fd)="\tjsr\t-8364(a6)";
#define SSL_set_rfd(s, fd) __SSL_set_rfd(AmiSSLBase, (s), (fd))

int __SSL_set_wfd(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG fd)="\tjsr\t-8370(a6)";
#define SSL_set_wfd(s, fd) __SSL_set_wfd(AmiSSLBase, (s), (fd))

void __SSL_set_bio(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("a1") BIO * rbio, __reg("a2") BIO * wbio)="\tjsr\t-8376(a6)";
#define SSL_set_bio(s, rbio, wbio) __SSL_set_bio(AmiSSLBase, (s), (rbio), (wbio))

BIO * __SSL_get_rbio(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8382(a6)";
#define SSL_get_rbio(s) __SSL_get_rbio(AmiSSLBase, (s))

BIO * __SSL_get_wbio(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8388(a6)";
#define SSL_get_wbio(s) __SSL_get_wbio(AmiSSLBase, (s))

int __SSL_set_cipher_list(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("a1") const char * str)="\tjsr\t-8394(a6)";
#define SSL_set_cipher_list(s, str) __SSL_set_cipher_list(AmiSSLBase, (s), (str))

void __SSL_set_read_ahead(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG yes)="\tjsr\t-8400(a6)";
#define SSL_set_read_ahead(s, yes) __SSL_set_read_ahead(AmiSSLBase, (s), (yes))

int __SSL_get_verify_mode(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8406(a6)";
#define SSL_get_verify_mode(s) __SSL_get_verify_mode(AmiSSLBase, (s))

int __SSL_get_verify_depth(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8412(a6)";
#define SSL_get_verify_depth(s) __SSL_get_verify_depth(AmiSSLBase, (s))

void * __SSL_get_verify_callback(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8418(a6)";
#define SSL_get_verify_callback(s) __SSL_get_verify_callback(AmiSSLBase, (s))

void __SSL_set_verify(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG mode, __reg("a1") int (*callback)(int ok, X509_STORE_CTX *ctx))="\tjsr\t-8424(a6)";
#define SSL_set_verify(s, mode, callback) __SSL_set_verify(AmiSSLBase, (s), (mode), (callback))

void __SSL_set_verify_depth(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG depth)="\tjsr\t-8430(a6)";
#define SSL_set_verify_depth(s, depth) __SSL_set_verify_depth(AmiSSLBase, (s), (depth))

int __SSL_use_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") RSA * rsa)="\tjsr\t-8436(a6)";
#define SSL_use_RSAPrivateKey(ssl, rsa) __SSL_use_RSAPrivateKey(AmiSSLBase, (ssl), (rsa))

int __SSL_use_RSAPrivateKey_ASN1(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") unsigned char * d, __reg("d0") long len)="\tjsr\t-8442(a6)";
#define SSL_use_RSAPrivateKey_ASN1(ssl, d, len) __SSL_use_RSAPrivateKey_ASN1(AmiSSLBase, (ssl), (d), (len))

int __SSL_use_PrivateKey(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-8448(a6)";
#define SSL_use_PrivateKey(ssl, pkey) __SSL_use_PrivateKey(AmiSSLBase, (ssl), (pkey))

int __SSL_use_PrivateKey_ASN1(__reg("a6") struct Library *, __reg("d0") LONG pk, __reg("a0") SSL * ssl, __reg("a1") unsigned char * d, __reg("d1") long len)="\tjsr\t-8454(a6)";
#define SSL_use_PrivateKey_ASN1(pk, ssl, d, len) __SSL_use_PrivateKey_ASN1(AmiSSLBase, (pk), (ssl), (d), (len))

int __SSL_use_certificate(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") X509 * x)="\tjsr\t-8460(a6)";
#define SSL_use_certificate(ssl, x) __SSL_use_certificate(AmiSSLBase, (ssl), (x))

int __SSL_use_certificate_ASN1(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") unsigned char * d, __reg("d0") LONG len)="\tjsr\t-8466(a6)";
#define SSL_use_certificate_ASN1(ssl, d, len) __SSL_use_certificate_ASN1(AmiSSLBase, (ssl), (d), (len))

int __SSL_use_RSAPrivateKey_file(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8472(a6)";
#define SSL_use_RSAPrivateKey_file(ssl, file, type) __SSL_use_RSAPrivateKey_file(AmiSSLBase, (ssl), (file), (type))

int __SSL_use_PrivateKey_file(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8478(a6)";
#define SSL_use_PrivateKey_file(ssl, file, type) __SSL_use_PrivateKey_file(AmiSSLBase, (ssl), (file), (type))

int __SSL_use_certificate_file(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8484(a6)";
#define SSL_use_certificate_file(ssl, file, type) __SSL_use_certificate_file(AmiSSLBase, (ssl), (file), (type))

int __SSL_CTX_use_RSAPrivateKey_file(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8490(a6)";
#define SSL_CTX_use_RSAPrivateKey_file(ctx, file, type) __SSL_CTX_use_RSAPrivateKey_file(AmiSSLBase, (ctx), (file), (type))

int __SSL_CTX_use_PrivateKey_file(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8496(a6)";
#define SSL_CTX_use_PrivateKey_file(ctx, file, type) __SSL_CTX_use_PrivateKey_file(AmiSSLBase, (ctx), (file), (type))

int __SSL_CTX_use_certificate_file(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-8502(a6)";
#define SSL_CTX_use_certificate_file(ctx, file, type) __SSL_CTX_use_certificate_file(AmiSSLBase, (ctx), (file), (type))

int __SSL_CTX_use_certificate_chain_file(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const char * file)="\tjsr\t-8508(a6)";
#define SSL_CTX_use_certificate_chain_file(ctx, file) __SSL_CTX_use_certificate_chain_file(AmiSSLBase, (ctx), (file))

void * __SSL_load_client_CA_file(__reg("a6") struct Library *, __reg("a0") const char * file)="\tjsr\t-8514(a6)";
#define SSL_load_client_CA_file(file) __SSL_load_client_CA_file(AmiSSLBase, (file))

int __SSL_add_file_cert_subjects_to_stack(__reg("a6") struct Library *, __reg("a0") void * stackCAs, __reg("a1") const char * file)="\tjsr\t-8520(a6)";
#define SSL_add_file_cert_subjects_to_stack(stackCAs, file) __SSL_add_file_cert_subjects_to_stack(AmiSSLBase, (stackCAs), (file))

int __SSL_add_dir_cert_subjects_to_stack(__reg("a6") struct Library *, __reg("a0") void * stackCAs, __reg("a1") const char * dir)="\tjsr\t-8526(a6)";
#define SSL_add_dir_cert_subjects_to_stack(stackCAs, dir) __SSL_add_dir_cert_subjects_to_stack(AmiSSLBase, (stackCAs), (dir))

void __SSL_load_error_strings(__reg("a6") struct Library *)="\tjsr\t-8532(a6)";
#define SSL_load_error_strings() __SSL_load_error_strings(AmiSSLBase)

const char * __SSL_state_string(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8538(a6)";
#define SSL_state_string(s) __SSL_state_string(AmiSSLBase, (s))

const char * __SSL_rstate_string(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8544(a6)";
#define SSL_rstate_string(s) __SSL_rstate_string(AmiSSLBase, (s))

const char * __SSL_state_string_long(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8550(a6)";
#define SSL_state_string_long(s) __SSL_state_string_long(AmiSSLBase, (s))

const char * __SSL_rstate_string_long(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8556(a6)";
#define SSL_rstate_string_long(s) __SSL_rstate_string_long(AmiSSLBase, (s))

long __SSL_SESSION_get_time(__reg("a6") struct Library *, __reg("a0") const SSL_SESSION * s)="\tjsr\t-8562(a6)";
#define SSL_SESSION_get_time(s) __SSL_SESSION_get_time(AmiSSLBase, (s))

long __SSL_SESSION_set_time(__reg("a6") struct Library *, __reg("a0") SSL_SESSION * s, __reg("d0") long t)="\tjsr\t-8568(a6)";
#define SSL_SESSION_set_time(s, t) __SSL_SESSION_set_time(AmiSSLBase, (s), (t))

long __SSL_SESSION_get_timeout(__reg("a6") struct Library *, __reg("a0") const SSL_SESSION * s)="\tjsr\t-8574(a6)";
#define SSL_SESSION_get_timeout(s) __SSL_SESSION_get_timeout(AmiSSLBase, (s))

long __SSL_SESSION_set_timeout(__reg("a6") struct Library *, __reg("a0") SSL_SESSION * s, __reg("d0") long t)="\tjsr\t-8580(a6)";
#define SSL_SESSION_set_timeout(s, t) __SSL_SESSION_set_timeout(AmiSSLBase, (s), (t))

void __SSL_copy_session_id(__reg("a6") struct Library *, __reg("a0") SSL * to, __reg("a1") const SSL * from)="\tjsr\t-8586(a6)";
#define SSL_copy_session_id(to, from) __SSL_copy_session_id(AmiSSLBase, (to), (from))

SSL_SESSION * __SSL_SESSION_new(__reg("a6") struct Library *)="\tjsr\t-8592(a6)";
#define SSL_SESSION_new() __SSL_SESSION_new(AmiSSLBase)

unsigned long __SSL_SESSION_hash(__reg("a6") struct Library *, __reg("a0") const SSL_SESSION * a)="\tjsr\t-8598(a6)";
#define SSL_SESSION_hash(a) __SSL_SESSION_hash(AmiSSLBase, (a))

int __SSL_SESSION_cmp(__reg("a6") struct Library *, __reg("a0") const SSL_SESSION * a, __reg("a1") const SSL_SESSION * b)="\tjsr\t-8604(a6)";
#define SSL_SESSION_cmp(a, b) __SSL_SESSION_cmp(AmiSSLBase, (a), (b))

int __SSL_SESSION_print(__reg("a6") struct Library *, __reg("a0") BIO * fp, __reg("a1") const SSL_SESSION * ses)="\tjsr\t-8610(a6)";
#define SSL_SESSION_print(fp, ses) __SSL_SESSION_print(AmiSSLBase, (fp), (ses))

void __SSL_SESSION_free(__reg("a6") struct Library *, __reg("a0") SSL_SESSION * ses)="\tjsr\t-8616(a6)";
#define SSL_SESSION_free(ses) __SSL_SESSION_free(AmiSSLBase, (ses))

int __i2d_SSL_SESSION(__reg("a6") struct Library *, __reg("a0") SSL_SESSION * in, __reg("a1") unsigned char ** pp)="\tjsr\t-8622(a6)";
#define i2d_SSL_SESSION(in, pp) __i2d_SSL_SESSION(AmiSSLBase, (in), (pp))

int __SSL_set_session(__reg("a6") struct Library *, __reg("a0") SSL * to, __reg("a1") SSL_SESSION * session)="\tjsr\t-8628(a6)";
#define SSL_set_session(to, session) __SSL_set_session(AmiSSLBase, (to), (session))

int __SSL_CTX_add_session(__reg("a6") struct Library *, __reg("a0") SSL_CTX * s, __reg("a1") SSL_SESSION * c)="\tjsr\t-8634(a6)";
#define SSL_CTX_add_session(s, c) __SSL_CTX_add_session(AmiSSLBase, (s), (c))

int __SSL_CTX_remove_session(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a1, __reg("a1") SSL_SESSION * c)="\tjsr\t-8640(a6)";
#define SSL_CTX_remove_session(a1, c) __SSL_CTX_remove_session(AmiSSLBase, (a1), (c))

int __SSL_CTX_set_generate_session_id(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a, __reg("d0") LONG b)="\tjsr\t-8646(a6)";
#define SSL_CTX_set_generate_session_id(a, b) __SSL_CTX_set_generate_session_id(AmiSSLBase, (a), (b))

int __SSL_set_generate_session_id(__reg("a6") struct Library *, __reg("a0") SSL * a, __reg("d0") LONG b)="\tjsr\t-8652(a6)";
#define SSL_set_generate_session_id(a, b) __SSL_set_generate_session_id(AmiSSLBase, (a), (b))

int __SSL_has_matching_session_id(__reg("a6") struct Library *, __reg("a0") const SSL * ssl, __reg("a1") const unsigned char * id, __reg("d0") ULONG id_len)="\tjsr\t-8658(a6)";
#define SSL_has_matching_session_id(ssl, id, id_len) __SSL_has_matching_session_id(AmiSSLBase, (ssl), (id), (id_len))

SSL_SESSION * __d2i_SSL_SESSION(__reg("a6") struct Library *, __reg("a0") SSL_SESSION ** a, __reg("a1") const unsigned char *const * pp, __reg("d0") long length)="\tjsr\t-8664(a6)";
#define d2i_SSL_SESSION(a, pp, length) __d2i_SSL_SESSION(AmiSSLBase, (a), (pp), (length))

X509 * __SSL_get_peer_certificate(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8670(a6)";
#define SSL_get_peer_certificate(s) __SSL_get_peer_certificate(AmiSSLBase, (s))

void * __SSL_get_peer_cert_chain(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8676(a6)";
#define SSL_get_peer_cert_chain(s) __SSL_get_peer_cert_chain(AmiSSLBase, (s))

int __SSL_CTX_get_verify_mode(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-8682(a6)";
#define SSL_CTX_get_verify_mode(ctx) __SSL_CTX_get_verify_mode(AmiSSLBase, (ctx))

int __SSL_CTX_get_verify_depth(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-8688(a6)";
#define SSL_CTX_get_verify_depth(ctx) __SSL_CTX_get_verify_depth(AmiSSLBase, (ctx))

void * __SSL_CTX_get_verify_callback(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-8694(a6)";
#define SSL_CTX_get_verify_callback(ctx) __SSL_CTX_get_verify_callback(AmiSSLBase, (ctx))

void __SSL_CTX_set_verify(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG mode, __reg("a1") int (*callback)(int, X509_STORE_CTX *))="\tjsr\t-8700(a6)";
#define SSL_CTX_set_verify(ctx, mode, callback) __SSL_CTX_set_verify(AmiSSLBase, (ctx), (mode), (callback))

void __SSL_CTX_set_verify_depth(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG depth)="\tjsr\t-8706(a6)";
#define SSL_CTX_set_verify_depth(ctx, depth) __SSL_CTX_set_verify_depth(AmiSSLBase, (ctx), (depth))

void __SSL_CTX_set_cert_verify_callback(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") int (*cb)(X509_STORE_CTX *, void *), __reg("a2") void * arg)="\tjsr\t-8712(a6)";
#define SSL_CTX_set_cert_verify_callback(ctx, cb, arg) __SSL_CTX_set_cert_verify_callback(AmiSSLBase, (ctx), (cb), (arg))

int __SSL_CTX_use_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") RSA * rsa)="\tjsr\t-8718(a6)";
#define SSL_CTX_use_RSAPrivateKey(ctx, rsa) __SSL_CTX_use_RSAPrivateKey(AmiSSLBase, (ctx), (rsa))

int __SSL_CTX_use_RSAPrivateKey_ASN1(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") unsigned char * d, __reg("d0") long len)="\tjsr\t-8724(a6)";
#define SSL_CTX_use_RSAPrivateKey_ASN1(ctx, d, len) __SSL_CTX_use_RSAPrivateKey_ASN1(AmiSSLBase, (ctx), (d), (len))

int __SSL_CTX_use_PrivateKey(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-8730(a6)";
#define SSL_CTX_use_PrivateKey(ctx, pkey) __SSL_CTX_use_PrivateKey(AmiSSLBase, (ctx), (pkey))

int __SSL_CTX_use_PrivateKey_ASN1(__reg("a6") struct Library *, __reg("d0") LONG pk, __reg("a0") SSL_CTX * ctx, __reg("a1") unsigned char * d, __reg("d1") long len)="\tjsr\t-8736(a6)";
#define SSL_CTX_use_PrivateKey_ASN1(pk, ctx, d, len) __SSL_CTX_use_PrivateKey_ASN1(AmiSSLBase, (pk), (ctx), (d), (len))

int __SSL_CTX_use_certificate(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") X509 * x)="\tjsr\t-8742(a6)";
#define SSL_CTX_use_certificate(ctx, x) __SSL_CTX_use_certificate(AmiSSLBase, (ctx), (x))

int __SSL_CTX_use_certificate_ASN1(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG len, __reg("a1") unsigned char * d)="\tjsr\t-8748(a6)";
#define SSL_CTX_use_certificate_ASN1(ctx, len, d) __SSL_CTX_use_certificate_ASN1(AmiSSLBase, (ctx), (len), (d))

void __SSL_CTX_set_default_passwd_cb(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") pem_password_cb * (*cb)(char *buf, int size, int rwflag, void *userdata))="\tjsr\t-8754(a6)";
#define SSL_CTX_set_default_passwd_cb(ctx, cb) __SSL_CTX_set_default_passwd_cb(AmiSSLBase, (ctx), (cb))

void __SSL_CTX_set_default_passwd_cb_userdata(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") void * u)="\tjsr\t-8760(a6)";
#define SSL_CTX_set_default_passwd_cb_userdata(ctx, u) __SSL_CTX_set_default_passwd_cb_userdata(AmiSSLBase, (ctx), (u))

int __SSL_CTX_check_private_key(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-8766(a6)";
#define SSL_CTX_check_private_key(ctx) __SSL_CTX_check_private_key(AmiSSLBase, (ctx))

int __SSL_check_private_key(__reg("a6") struct Library *, __reg("a0") const SSL * ctx)="\tjsr\t-8772(a6)";
#define SSL_check_private_key(ctx) __SSL_check_private_key(AmiSSLBase, (ctx))

int __SSL_CTX_set_session_id_context(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const unsigned char * sid_ctx, __reg("d0") ULONG sid_ctx_len)="\tjsr\t-8778(a6)";
#define SSL_CTX_set_session_id_context(ctx, sid_ctx, sid_ctx_len) __SSL_CTX_set_session_id_context(AmiSSLBase, (ctx), (sid_ctx), (sid_ctx_len))

SSL * __SSL_new(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx)="\tjsr\t-8784(a6)";
#define SSL_new(ctx) __SSL_new(AmiSSLBase, (ctx))

int __SSL_set_session_id_context(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") const unsigned char * sid_ctx, __reg("d0") ULONG sid_ctx_len)="\tjsr\t-8790(a6)";
#define SSL_set_session_id_context(ssl, sid_ctx, sid_ctx_len) __SSL_set_session_id_context(AmiSSLBase, (ssl), (sid_ctx), (sid_ctx_len))

int __SSL_CTX_set_purpose(__reg("a6") struct Library *, __reg("a0") SSL_CTX * s, __reg("d0") LONG purpose)="\tjsr\t-8796(a6)";
#define SSL_CTX_set_purpose(s, purpose) __SSL_CTX_set_purpose(AmiSSLBase, (s), (purpose))

int __SSL_set_purpose(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG purpose)="\tjsr\t-8802(a6)";
#define SSL_set_purpose(s, purpose) __SSL_set_purpose(AmiSSLBase, (s), (purpose))

int __SSL_CTX_set_trust(__reg("a6") struct Library *, __reg("a0") SSL_CTX * s, __reg("d0") LONG trust)="\tjsr\t-8808(a6)";
#define SSL_CTX_set_trust(s, trust) __SSL_CTX_set_trust(AmiSSLBase, (s), (trust))

int __SSL_set_trust(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("d0") LONG trust)="\tjsr\t-8814(a6)";
#define SSL_set_trust(s, trust) __SSL_set_trust(AmiSSLBase, (s), (trust))

void __SSL_free(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-8820(a6)";
#define SSL_free(ssl) __SSL_free(AmiSSLBase, (ssl))

int __SSL_accept(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-8826(a6)";
#define SSL_accept(ssl) __SSL_accept(AmiSSLBase, (ssl))

int __SSL_connect(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-8832(a6)";
#define SSL_connect(ssl) __SSL_connect(AmiSSLBase, (ssl))

int __SSL_read(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") void * buf, __reg("d0") LONG num)="\tjsr\t-8838(a6)";
#define SSL_read(ssl, buf, num) __SSL_read(AmiSSLBase, (ssl), (buf), (num))

int __SSL_peek(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") void * buf, __reg("d0") LONG num)="\tjsr\t-8844(a6)";
#define SSL_peek(ssl, buf, num) __SSL_peek(AmiSSLBase, (ssl), (buf), (num))

int __SSL_write(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") const void * buf, __reg("d0") LONG num)="\tjsr\t-8850(a6)";
#define SSL_write(ssl, buf, num) __SSL_write(AmiSSLBase, (ssl), (buf), (num))

long __SSL_ctrl(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("a1") void * parg)="\tjsr\t-8856(a6)";
#define SSL_ctrl(ssl, cmd, larg, parg) __SSL_ctrl(AmiSSLBase, (ssl), (cmd), (larg), (parg))

long __SSL_callback_ctrl(__reg("a6") struct Library *, __reg("a0") SSL * a, __reg("d0") LONG a1, __reg("a1") void (*b)())="\tjsr\t-8862(a6)";
#define SSL_callback_ctrl(a, a1, b) __SSL_callback_ctrl(AmiSSLBase, (a), (a1), (b))

long __SSL_CTX_ctrl(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG cmd, __reg("d1") long larg, __reg("a1") void * parg)="\tjsr\t-8868(a6)";
#define SSL_CTX_ctrl(ctx, cmd, larg, parg) __SSL_CTX_ctrl(AmiSSLBase, (ctx), (cmd), (larg), (parg))

long __SSL_CTX_callback_ctrl(__reg("a6") struct Library *, __reg("a0") SSL_CTX * a1, __reg("d0") LONG a2, __reg("a1") void (*a3)())="\tjsr\t-8874(a6)";
#define SSL_CTX_callback_ctrl(a1, a2, a3) __SSL_CTX_callback_ctrl(AmiSSLBase, (a1), (a2), (a3))

int __SSL_get_error(__reg("a6") struct Library *, __reg("a0") const SSL * s, __reg("d0") LONG ret_code)="\tjsr\t-8880(a6)";
#define SSL_get_error(s, ret_code) __SSL_get_error(AmiSSLBase, (s), (ret_code))

const char * __SSL_get_version(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8886(a6)";
#define SSL_get_version(s) __SSL_get_version(AmiSSLBase, (s))

int __SSL_CTX_set_ssl_version(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") SSL_METHOD * meth)="\tjsr\t-8892(a6)";
#define SSL_CTX_set_ssl_version(ctx, meth) __SSL_CTX_set_ssl_version(AmiSSLBase, (ctx), (meth))

SSL_METHOD * __SSLv2_method(__reg("a6") struct Library *)="\tjsr\t-8898(a6)";
#define SSLv2_method() __SSLv2_method(AmiSSLBase)

SSL_METHOD * __SSLv2_server_method(__reg("a6") struct Library *)="\tjsr\t-8904(a6)";
#define SSLv2_server_method() __SSLv2_server_method(AmiSSLBase)

SSL_METHOD * __SSLv2_client_method(__reg("a6") struct Library *)="\tjsr\t-8910(a6)";
#define SSLv2_client_method() __SSLv2_client_method(AmiSSLBase)

SSL_METHOD * __SSLv3_method(__reg("a6") struct Library *)="\tjsr\t-8916(a6)";
#define SSLv3_method() __SSLv3_method(AmiSSLBase)

SSL_METHOD * __SSLv3_server_method(__reg("a6") struct Library *)="\tjsr\t-8922(a6)";
#define SSLv3_server_method() __SSLv3_server_method(AmiSSLBase)

SSL_METHOD * __SSLv3_client_method(__reg("a6") struct Library *)="\tjsr\t-8928(a6)";
#define SSLv3_client_method() __SSLv3_client_method(AmiSSLBase)

SSL_METHOD * __SSLv23_method(__reg("a6") struct Library *)="\tjsr\t-8934(a6)";
#define SSLv23_method() __SSLv23_method(AmiSSLBase)

SSL_METHOD * __SSLv23_server_method(__reg("a6") struct Library *)="\tjsr\t-8940(a6)";
#define SSLv23_server_method() __SSLv23_server_method(AmiSSLBase)

SSL_METHOD * __SSLv23_client_method(__reg("a6") struct Library *)="\tjsr\t-8946(a6)";
#define SSLv23_client_method() __SSLv23_client_method(AmiSSLBase)

SSL_METHOD * __TLSv1_method(__reg("a6") struct Library *)="\tjsr\t-8952(a6)";
#define TLSv1_method() __TLSv1_method(AmiSSLBase)

SSL_METHOD * __TLSv1_server_method(__reg("a6") struct Library *)="\tjsr\t-8958(a6)";
#define TLSv1_server_method() __TLSv1_server_method(AmiSSLBase)

SSL_METHOD * __TLSv1_client_method(__reg("a6") struct Library *)="\tjsr\t-8964(a6)";
#define TLSv1_client_method() __TLSv1_client_method(AmiSSLBase)

void * __SSL_get_ciphers(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-8970(a6)";
#define SSL_get_ciphers(s) __SSL_get_ciphers(AmiSSLBase, (s))

int __SSL_do_handshake(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-8976(a6)";
#define SSL_do_handshake(s) __SSL_do_handshake(AmiSSLBase, (s))

int __SSL_renegotiate(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-8982(a6)";
#define SSL_renegotiate(s) __SSL_renegotiate(AmiSSLBase, (s))

int __SSL_renegotiate_pending(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-8988(a6)";
#define SSL_renegotiate_pending(s) __SSL_renegotiate_pending(AmiSSLBase, (s))

int __SSL_shutdown(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-8994(a6)";
#define SSL_shutdown(s) __SSL_shutdown(AmiSSLBase, (s))

SSL_METHOD * __SSL_get_ssl_method(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-9000(a6)";
#define SSL_get_ssl_method(s) __SSL_get_ssl_method(AmiSSLBase, (s))

int __SSL_set_ssl_method(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("a1") SSL_METHOD * method)="\tjsr\t-9006(a6)";
#define SSL_set_ssl_method(s, method) __SSL_set_ssl_method(AmiSSLBase, (s), (method))

const char * __SSL_alert_type_string_long(__reg("a6") struct Library *, __reg("d0") LONG value)="\tjsr\t-9012(a6)";
#define SSL_alert_type_string_long(value) __SSL_alert_type_string_long(AmiSSLBase, (value))

const char * __SSL_alert_type_string(__reg("a6") struct Library *, __reg("d0") LONG value)="\tjsr\t-9018(a6)";
#define SSL_alert_type_string(value) __SSL_alert_type_string(AmiSSLBase, (value))

const char * __SSL_alert_desc_string_long(__reg("a6") struct Library *, __reg("d0") LONG value)="\tjsr\t-9024(a6)";
#define SSL_alert_desc_string_long(value) __SSL_alert_desc_string_long(AmiSSLBase, (value))

const char * __SSL_alert_desc_string(__reg("a6") struct Library *, __reg("d0") LONG value)="\tjsr\t-9030(a6)";
#define SSL_alert_desc_string(value) __SSL_alert_desc_string(AmiSSLBase, (value))

void __SSL_set_client_CA_list(__reg("a6") struct Library *, __reg("a0") SSL * s, __reg("a1") void * name_list)="\tjsr\t-9036(a6)";
#define SSL_set_client_CA_list(s, name_list) __SSL_set_client_CA_list(AmiSSLBase, (s), (name_list))

void __SSL_CTX_set_client_CA_list(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") void * name_list)="\tjsr\t-9042(a6)";
#define SSL_CTX_set_client_CA_list(ctx, name_list) __SSL_CTX_set_client_CA_list(AmiSSLBase, (ctx), (name_list))

void * __SSL_get_client_CA_list(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-9048(a6)";
#define SSL_get_client_CA_list(s) __SSL_get_client_CA_list(AmiSSLBase, (s))

void * __SSL_CTX_get_client_CA_list(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * s)="\tjsr\t-9054(a6)";
#define SSL_CTX_get_client_CA_list(s) __SSL_CTX_get_client_CA_list(AmiSSLBase, (s))

int __SSL_add_client_CA(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") X509 * x)="\tjsr\t-9060(a6)";
#define SSL_add_client_CA(ssl, x) __SSL_add_client_CA(AmiSSLBase, (ssl), (x))

int __SSL_CTX_add_client_CA(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") X509 * x)="\tjsr\t-9066(a6)";
#define SSL_CTX_add_client_CA(ctx, x) __SSL_CTX_add_client_CA(AmiSSLBase, (ctx), (x))

void __SSL_set_connect_state(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-9072(a6)";
#define SSL_set_connect_state(s) __SSL_set_connect_state(AmiSSLBase, (s))

void __SSL_set_accept_state(__reg("a6") struct Library *, __reg("a0") SSL * s)="\tjsr\t-9078(a6)";
#define SSL_set_accept_state(s) __SSL_set_accept_state(AmiSSLBase, (s))

long __SSL_get_default_timeout(__reg("a6") struct Library *, __reg("a0") const SSL * s)="\tjsr\t-9084(a6)";
#define SSL_get_default_timeout(s) __SSL_get_default_timeout(AmiSSLBase, (s))

int __SSL_library_init(__reg("a6") struct Library *)="\tjsr\t-9090(a6)";
#define SSL_library_init() __SSL_library_init(AmiSSLBase)

char * __SSL_CIPHER_description(__reg("a6") struct Library *, __reg("a0") SSL_CIPHER * a1, __reg("a1") char * buf, __reg("d0") LONG size)="\tjsr\t-9096(a6)";
#define SSL_CIPHER_description(a1, buf, size) __SSL_CIPHER_description(AmiSSLBase, (a1), (buf), (size))

void * __SSL_dup_CA_list(__reg("a6") struct Library *, __reg("a0") void * sk)="\tjsr\t-9102(a6)";
#define SSL_dup_CA_list(sk) __SSL_dup_CA_list(AmiSSLBase, (sk))

SSL * __SSL_dup(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-9108(a6)";
#define SSL_dup(ssl) __SSL_dup(AmiSSLBase, (ssl))

X509 * __SSL_get_certificate(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9114(a6)";
#define SSL_get_certificate(ssl) __SSL_get_certificate(AmiSSLBase, (ssl))

struct evp_pkey_st * __SSL_get_privatekey(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-9120(a6)";
#define SSL_get_privatekey(ssl) __SSL_get_privatekey(AmiSSLBase, (ssl))

void __SSL_CTX_set_quiet_shutdown(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("d0") LONG mode)="\tjsr\t-9126(a6)";
#define SSL_CTX_set_quiet_shutdown(ctx, mode) __SSL_CTX_set_quiet_shutdown(AmiSSLBase, (ctx), (mode))

int __SSL_CTX_get_quiet_shutdown(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ctx)="\tjsr\t-9132(a6)";
#define SSL_CTX_get_quiet_shutdown(ctx) __SSL_CTX_get_quiet_shutdown(AmiSSLBase, (ctx))

void __SSL_set_quiet_shutdown(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("d0") LONG mode)="\tjsr\t-9138(a6)";
#define SSL_set_quiet_shutdown(ssl, mode) __SSL_set_quiet_shutdown(AmiSSLBase, (ssl), (mode))

int __SSL_get_quiet_shutdown(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9144(a6)";
#define SSL_get_quiet_shutdown(ssl) __SSL_get_quiet_shutdown(AmiSSLBase, (ssl))

void __SSL_set_shutdown(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("d0") LONG mode)="\tjsr\t-9150(a6)";
#define SSL_set_shutdown(ssl, mode) __SSL_set_shutdown(AmiSSLBase, (ssl), (mode))

int __SSL_get_shutdown(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9156(a6)";
#define SSL_get_shutdown(ssl) __SSL_get_shutdown(AmiSSLBase, (ssl))

int __SSL_version(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9162(a6)";
#define SSL_version(ssl) __SSL_version(AmiSSLBase, (ssl))

int __SSL_CTX_set_default_verify_paths(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx)="\tjsr\t-9168(a6)";
#define SSL_CTX_set_default_verify_paths(ctx) __SSL_CTX_set_default_verify_paths(AmiSSLBase, (ctx))

int __SSL_CTX_load_verify_locations(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") const char * CAfile, __reg("a2") const char * CApath)="\tjsr\t-9174(a6)";
#define SSL_CTX_load_verify_locations(ctx, CAfile, CApath) __SSL_CTX_load_verify_locations(AmiSSLBase, (ctx), (CAfile), (CApath))

SSL_SESSION * __SSL_get_session(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9180(a6)";
#define SSL_get_session(ssl) __SSL_get_session(AmiSSLBase, (ssl))

SSL_SESSION * __SSL_get1_session(__reg("a6") struct Library *, __reg("a0") SSL * ssl)="\tjsr\t-9186(a6)";
#define SSL_get1_session(ssl) __SSL_get1_session(AmiSSLBase, (ssl))

SSL_CTX * __SSL_get_SSL_CTX(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9192(a6)";
#define SSL_get_SSL_CTX(ssl) __SSL_get_SSL_CTX(AmiSSLBase, (ssl))

void __SSL_set_info_callback(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") void (*cb)(const SSL *ssl, int type, int val))="\tjsr\t-9198(a6)";
#define SSL_set_info_callback(ssl, cb) __SSL_set_info_callback(AmiSSLBase, (ssl), (cb))

void * __SSL_get_info_callback(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9204(a6)";
#define SSL_get_info_callback(ssl) __SSL_get_info_callback(AmiSSLBase, (ssl))

int __SSL_state(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9210(a6)";
#define SSL_state(ssl) __SSL_state(AmiSSLBase, (ssl))

void __SSL_set_verify_result(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("d0") long v)="\tjsr\t-9216(a6)";
#define SSL_set_verify_result(ssl, v) __SSL_set_verify_result(AmiSSLBase, (ssl), (v))

long __SSL_get_verify_result(__reg("a6") struct Library *, __reg("a0") const SSL * ssl)="\tjsr\t-9222(a6)";
#define SSL_get_verify_result(ssl) __SSL_get_verify_result(AmiSSLBase, (ssl))

int __SSL_set_ex_data(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("d0") LONG idx, __reg("a1") void * data)="\tjsr\t-9228(a6)";
#define SSL_set_ex_data(ssl, idx, data) __SSL_set_ex_data(AmiSSLBase, (ssl), (idx), (data))

void * __SSL_get_ex_data(__reg("a6") struct Library *, __reg("a0") const SSL * ssl, __reg("d0") LONG idx)="\tjsr\t-9234(a6)";
#define SSL_get_ex_data(ssl, idx) __SSL_get_ex_data(AmiSSLBase, (ssl), (idx))

int __SSL_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-9240(a6)";
#define SSL_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __SSL_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __SSL_SESSION_set_ex_data(__reg("a6") struct Library *, __reg("a0") SSL_SESSION * ss, __reg("d0") LONG idx, __reg("a1") void * data)="\tjsr\t-9246(a6)";
#define SSL_SESSION_set_ex_data(ss, idx, data) __SSL_SESSION_set_ex_data(AmiSSLBase, (ss), (idx), (data))

void * __SSL_SESSION_get_ex_data(__reg("a6") struct Library *, __reg("a0") const SSL_SESSION * ss, __reg("d0") LONG idx)="\tjsr\t-9252(a6)";
#define SSL_SESSION_get_ex_data(ss, idx) __SSL_SESSION_get_ex_data(AmiSSLBase, (ss), (idx))

int __SSL_SESSION_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-9258(a6)";
#define SSL_SESSION_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __SSL_SESSION_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __SSL_CTX_set_ex_data(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ssl, __reg("d0") LONG idx, __reg("a1") void * data)="\tjsr\t-9264(a6)";
#define SSL_CTX_set_ex_data(ssl, idx, data) __SSL_CTX_set_ex_data(AmiSSLBase, (ssl), (idx), (data))

void * __SSL_CTX_get_ex_data(__reg("a6") struct Library *, __reg("a0") const SSL_CTX * ssl, __reg("d0") LONG idx)="\tjsr\t-9270(a6)";
#define SSL_CTX_get_ex_data(ssl, idx) __SSL_CTX_get_ex_data(AmiSSLBase, (ssl), (idx))

int __SSL_CTX_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-9276(a6)";
#define SSL_CTX_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __SSL_CTX_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __SSL_get_ex_data_X509_STORE_CTX_idx(__reg("a6") struct Library *)="\tjsr\t-9282(a6)";
#define SSL_get_ex_data_X509_STORE_CTX_idx() __SSL_get_ex_data_X509_STORE_CTX_idx(AmiSSLBase)

void __SSL_CTX_set_tmp_rsa_callback(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") RSA * (*cb)(SSL *ssl, int is_export, int keylength))="\tjsr\t-9288(a6)";
#define SSL_CTX_set_tmp_rsa_callback(ctx, cb) __SSL_CTX_set_tmp_rsa_callback(AmiSSLBase, (ctx), (cb))

void __SSL_set_tmp_rsa_callback(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") RSA * (*cb)(SSL *ssl, int is_export, int keylength))="\tjsr\t-9294(a6)";
#define SSL_set_tmp_rsa_callback(ssl, cb) __SSL_set_tmp_rsa_callback(AmiSSLBase, (ssl), (cb))

void __SSL_CTX_set_tmp_dh_callback(__reg("a6") struct Library *, __reg("a0") SSL_CTX * ctx, __reg("a1") DH * (*dh)(SSL *ssl, int is_export, int keylength))="\tjsr\t-9300(a6)";
#define SSL_CTX_set_tmp_dh_callback(ctx, dh) __SSL_CTX_set_tmp_dh_callback(AmiSSLBase, (ctx), (dh))

void __SSL_set_tmp_dh_callback(__reg("a6") struct Library *, __reg("a0") SSL * ssl, __reg("a1") DH * (*dh)(SSL *ssl, int is_export, int keylength))="\tjsr\t-9306(a6)";
#define SSL_set_tmp_dh_callback(ssl, dh) __SSL_set_tmp_dh_callback(AmiSSLBase, (ssl), (dh))

int __SSL_COMP_add_compression_method(__reg("a6") struct Library *, __reg("d0") LONG id, __reg("a0") COMP_METHOD * cm)="\tjsr\t-9312(a6)";
#define SSL_COMP_add_compression_method(id, cm) __SSL_COMP_add_compression_method(AmiSSLBase, (id), (cm))

void __ERR_load_SSL_strings(__reg("a6") struct Library *)="\tjsr\t-9318(a6)";
#define ERR_load_SSL_strings() __ERR_load_SSL_strings(AmiSSLBase)

int __sk_num(__reg("a6") struct Library *, __reg("a0") const STACK * a)="\tjsr\t-9324(a6)";
#define sk_num(a) __sk_num(AmiSSLBase, (a))

char * __sk_value(__reg("a6") struct Library *, __reg("a0") const STACK * a, __reg("d0") LONG b)="\tjsr\t-9330(a6)";
#define sk_value(a, b) __sk_value(AmiSSLBase, (a), (b))

char * __sk_set(__reg("a6") struct Library *, __reg("a0") STACK * a, __reg("d0") LONG b, __reg("a1") char * c)="\tjsr\t-9336(a6)";
#define sk_set(a, b, c) __sk_set(AmiSSLBase, (a), (b), (c))

STACK * __sk_new(__reg("a6") struct Library *, __reg("a0") int (*cmp)(const char *const *, const char *const *))="\tjsr\t-9342(a6)";
#define sk_new(cmp) __sk_new(AmiSSLBase, (cmp))

STACK * __sk_new_null(__reg("a6") struct Library *)="\tjsr\t-9348(a6)";
#define sk_new_null() __sk_new_null(AmiSSLBase)

void __sk_free(__reg("a6") struct Library *, __reg("a0") STACK * a)="\tjsr\t-9354(a6)";
#define sk_free(a) __sk_free(AmiSSLBase, (a))

void __sk_pop_free(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("a1") void (*func)(void *))="\tjsr\t-9360(a6)";
#define sk_pop_free(st, func) __sk_pop_free(AmiSSLBase, (st), (func))

int __sk_insert(__reg("a6") struct Library *, __reg("a0") STACK * sk, __reg("a1") char * data, __reg("d0") LONG where)="\tjsr\t-9366(a6)";
#define sk_insert(sk, data, where) __sk_insert(AmiSSLBase, (sk), (data), (where))

char * __sk_delete(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("d0") LONG loc)="\tjsr\t-9372(a6)";
#define sk_delete(st, loc) __sk_delete(AmiSSLBase, (st), (loc))

char * __sk_delete_ptr(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("a1") char * p)="\tjsr\t-9378(a6)";
#define sk_delete_ptr(st, p) __sk_delete_ptr(AmiSSLBase, (st), (p))

int __sk_find(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("a1") char * data)="\tjsr\t-9384(a6)";
#define sk_find(st, data) __sk_find(AmiSSLBase, (st), (data))

int __sk_push(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("a1") char * data)="\tjsr\t-9390(a6)";
#define sk_push(st, data) __sk_push(AmiSSLBase, (st), (data))

int __sk_unshift(__reg("a6") struct Library *, __reg("a0") STACK * st, __reg("a1") char * data)="\tjsr\t-9396(a6)";
#define sk_unshift(st, data) __sk_unshift(AmiSSLBase, (st), (data))

char * __sk_shift(__reg("a6") struct Library *, __reg("a0") STACK * st)="\tjsr\t-9402(a6)";
#define sk_shift(st) __sk_shift(AmiSSLBase, (st))

char * __sk_pop(__reg("a6") struct Library *, __reg("a0") STACK * st)="\tjsr\t-9408(a6)";
#define sk_pop(st) __sk_pop(AmiSSLBase, (st))

void __sk_zero(__reg("a6") struct Library *, __reg("a0") STACK * st)="\tjsr\t-9414(a6)";
#define sk_zero(st) __sk_zero(AmiSSLBase, (st))

void * __sk_set_cmp_func(__reg("a6") struct Library *, __reg("a0") STACK * sk, __reg("a1") int (*c)(const char *const *, const char *const *))="\tjsr\t-9420(a6)";
#define sk_set_cmp_func(sk, c) __sk_set_cmp_func(AmiSSLBase, (sk), (c))

STACK * __sk_dup(__reg("a6") struct Library *, __reg("a0") STACK * st)="\tjsr\t-9426(a6)";
#define sk_dup(st) __sk_dup(AmiSSLBase, (st))

void __sk_sort(__reg("a6") struct Library *, __reg("a0") STACK * st)="\tjsr\t-9432(a6)";
#define sk_sort(st) __sk_sort(AmiSSLBase, (st))

int __sk_is_sorted(__reg("a6") struct Library *, __reg("a0") const STACK * st)="\tjsr\t-9438(a6)";
#define sk_is_sorted(st) __sk_is_sorted(AmiSSLBase, (st))

char * __ms_time_new(__reg("a6") struct Library *)="\tjsr\t-9444(a6)";
#define ms_time_new() __ms_time_new(AmiSSLBase)

void __ms_time_free(__reg("a6") struct Library *, __reg("a0") char * a)="\tjsr\t-9450(a6)";
#define ms_time_free(a) __ms_time_free(AmiSSLBase, (a))

void __ms_time_get(__reg("a6") struct Library *, __reg("a0") char * a)="\tjsr\t-9456(a6)";
#define ms_time_get(a) __ms_time_get(AmiSSLBase, (a))

double __ms_time_diff(__reg("a6") struct Library *, __reg("a0") char * start, __reg("a1") char * end)="\tjsr\t-9462(a6)";
#define ms_time_diff(start, end) __ms_time_diff(AmiSSLBase, (start), (end))

int __ms_time_cmp(__reg("a6") struct Library *, __reg("a0") char * ap, __reg("a1") char * bp)="\tjsr\t-9468(a6)";
#define ms_time_cmp(ap, bp) __ms_time_cmp(AmiSSLBase, (ap), (bp))

TXT_DB * __TXT_DB_read(__reg("a6") struct Library *, __reg("a0") BIO * in, __reg("d0") LONG num)="\tjsr\t-9474(a6)";
#define TXT_DB_read(in, num) __TXT_DB_read(AmiSSLBase, (in), (num))

long __TXT_DB_write(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") TXT_DB * db)="\tjsr\t-9480(a6)";
#define TXT_DB_write(out, db) __TXT_DB_write(AmiSSLBase, (out), (db))

int __TXT_DB_create_index(__reg("a6") struct Library *, __reg("a0") TXT_DB * db, __reg("d0") LONG field, __reg("a1") int (*qual)(), __reg("d1") LONG hash, __reg("d2") LONG cmp)="\tjsr\t-9486(a6)";
#define TXT_DB_create_index(db, field, qual, hash, cmp) __TXT_DB_create_index(AmiSSLBase, (db), (field), (qual), (hash), (cmp))

void __TXT_DB_free(__reg("a6") struct Library *, __reg("a0") TXT_DB * db)="\tjsr\t-9492(a6)";
#define TXT_DB_free(db) __TXT_DB_free(AmiSSLBase, (db))

char ** __TXT_DB_get_by_index(__reg("a6") struct Library *, __reg("a0") TXT_DB * db, __reg("d0") LONG idx, __reg("a1") char ** value)="\tjsr\t-9498(a6)";
#define TXT_DB_get_by_index(db, idx, value) __TXT_DB_get_by_index(AmiSSLBase, (db), (idx), (value))

int __TXT_DB_insert(__reg("a6") struct Library *, __reg("a0") TXT_DB * db, __reg("a1") char ** value)="\tjsr\t-9504(a6)";
#define TXT_DB_insert(db, value) __TXT_DB_insert(AmiSSLBase, (db), (value))

UI * __UI_new(__reg("a6") struct Library *)="\tjsr\t-9510(a6)";
#define UI_new() __UI_new(AmiSSLBase)

UI * __UI_new_method(__reg("a6") struct Library *, __reg("a0") const UI_METHOD * method)="\tjsr\t-9516(a6)";
#define UI_new_method(method) __UI_new_method(AmiSSLBase, (method))

void __UI_free(__reg("a6") struct Library *, __reg("a0") UI * ui)="\tjsr\t-9522(a6)";
#define UI_free(ui) __UI_free(AmiSSLBase, (ui))

int __UI_add_input_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("d0") LONG flags, __reg("a2") char * result_buf, __reg("d1") LONG minsize, __reg("d2") LONG maxsize)="\tjsr\t-9528(a6)";
#define UI_add_input_string(ui, prompt, flags, result_buf, minsize, maxsize) __UI_add_input_string(AmiSSLBase, (ui), (prompt), (flags), (result_buf), (minsize), (maxsize))

int __UI_dup_input_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("d0") LONG flags, __reg("a2") char * result_buf, __reg("d1") LONG minsize, __reg("d2") LONG maxsize)="\tjsr\t-9534(a6)";
#define UI_dup_input_string(ui, prompt, flags, result_buf, minsize, maxsize) __UI_dup_input_string(AmiSSLBase, (ui), (prompt), (flags), (result_buf), (minsize), (maxsize))

int __UI_add_verify_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("d0") LONG flags, __reg("a2") char * result_buf, __reg("d1") LONG minsize, __reg("d2") LONG maxsize, __reg("a3") const char * test_buf)="\tjsr\t-9540(a6)";
#define UI_add_verify_string(ui, prompt, flags, result_buf, minsize, maxsize, test_buf) __UI_add_verify_string(AmiSSLBase, (ui), (prompt), (flags), (result_buf), (minsize), (maxsize), (test_buf))

int __UI_dup_verify_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("d0") LONG flags, __reg("a2") char * result_buf, __reg("d1") LONG minsize, __reg("d2") LONG maxsize, __reg("a3") const char * test_buf)="\tjsr\t-9546(a6)";
#define UI_dup_verify_string(ui, prompt, flags, result_buf, minsize, maxsize, test_buf) __UI_dup_verify_string(AmiSSLBase, (ui), (prompt), (flags), (result_buf), (minsize), (maxsize), (test_buf))

int __UI_add_input_boolean(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("a2") const char * action_desc, __reg("a3") const char * ok_chars, __reg("d0") const char * cancel_chars, __reg("d1") LONG flags, __reg("d2") char * result_buf)="\tjsr\t-9552(a6)";
#define UI_add_input_boolean(ui, prompt, action_desc, ok_chars, cancel_chars, flags, result_buf) __UI_add_input_boolean(AmiSSLBase, (ui), (prompt), (action_desc), (ok_chars), (cancel_chars), (flags), (result_buf))

int __UI_dup_input_boolean(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * prompt, __reg("a2") const char * action_desc, __reg("a3") const char * ok_chars, __reg("d0") const char * cancel_chars, __reg("d1") LONG flags, __reg("d2") char * result_buf)="\tjsr\t-9558(a6)";
#define UI_dup_input_boolean(ui, prompt, action_desc, ok_chars, cancel_chars, flags, result_buf) __UI_dup_input_boolean(AmiSSLBase, (ui), (prompt), (action_desc), (ok_chars), (cancel_chars), (flags), (result_buf))

int __UI_add_info_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * text)="\tjsr\t-9564(a6)";
#define UI_add_info_string(ui, text) __UI_add_info_string(AmiSSLBase, (ui), (text))

int __UI_dup_info_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * text)="\tjsr\t-9570(a6)";
#define UI_dup_info_string(ui, text) __UI_dup_info_string(AmiSSLBase, (ui), (text))

int __UI_add_error_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * text)="\tjsr\t-9576(a6)";
#define UI_add_error_string(ui, text) __UI_add_error_string(AmiSSLBase, (ui), (text))

int __UI_dup_error_string(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const char * text)="\tjsr\t-9582(a6)";
#define UI_dup_error_string(ui, text) __UI_dup_error_string(AmiSSLBase, (ui), (text))

char * __UI_construct_prompt(__reg("a6") struct Library *, __reg("a0") UI * ui_method, __reg("a1") const char * object_desc, __reg("a2") const char * object_name)="\tjsr\t-9588(a6)";
#define UI_construct_prompt(ui_method, object_desc, object_name) __UI_construct_prompt(AmiSSLBase, (ui_method), (object_desc), (object_name))

void * __UI_add_user_data(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") void * user_data)="\tjsr\t-9594(a6)";
#define UI_add_user_data(ui, user_data) __UI_add_user_data(AmiSSLBase, (ui), (user_data))

void * __UI_get0_user_data(__reg("a6") struct Library *, __reg("a0") UI * ui)="\tjsr\t-9600(a6)";
#define UI_get0_user_data(ui) __UI_get0_user_data(AmiSSLBase, (ui))

const char * __UI_get0_result(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("d0") LONG i)="\tjsr\t-9606(a6)";
#define UI_get0_result(ui, i) __UI_get0_result(AmiSSLBase, (ui), (i))

int __UI_process(__reg("a6") struct Library *, __reg("a0") UI * ui)="\tjsr\t-9612(a6)";
#define UI_process(ui) __UI_process(AmiSSLBase, (ui))

int __UI_ctrl(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("d0") LONG cmd, __reg("d1") long i, __reg("a1") void * p, __reg("a2") void (*f)())="\tjsr\t-9618(a6)";
#define UI_ctrl(ui, cmd, i, p, f) __UI_ctrl(AmiSSLBase, (ui), (cmd), (i), (p), (f))

int __UI_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-9624(a6)";
#define UI_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __UI_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __UI_set_ex_data(__reg("a6") struct Library *, __reg("a0") UI * r, __reg("d0") LONG idx, __reg("a1") void * arg)="\tjsr\t-9630(a6)";
#define UI_set_ex_data(r, idx, arg) __UI_set_ex_data(AmiSSLBase, (r), (idx), (arg))

void * __UI_get_ex_data(__reg("a6") struct Library *, __reg("a0") UI * r, __reg("d0") LONG idx)="\tjsr\t-9636(a6)";
#define UI_get_ex_data(r, idx) __UI_get_ex_data(AmiSSLBase, (r), (idx))

void __UI_set_default_method(__reg("a6") struct Library *, __reg("a0") const UI_METHOD * meth)="\tjsr\t-9642(a6)";
#define UI_set_default_method(meth) __UI_set_default_method(AmiSSLBase, (meth))

const UI_METHOD * __UI_get_default_method(__reg("a6") struct Library *)="\tjsr\t-9648(a6)";
#define UI_get_default_method() __UI_get_default_method(AmiSSLBase)

const UI_METHOD * __UI_get_method(__reg("a6") struct Library *, __reg("a0") UI * ui)="\tjsr\t-9654(a6)";
#define UI_get_method(ui) __UI_get_method(AmiSSLBase, (ui))

const UI_METHOD * __UI_set_method(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") const UI_METHOD * meth)="\tjsr\t-9660(a6)";
#define UI_set_method(ui, meth) __UI_set_method(AmiSSLBase, (ui), (meth))

UI_METHOD * __UI_OpenSSL(__reg("a6") struct Library *)="\tjsr\t-9666(a6)";
#define UI_OpenSSL() __UI_OpenSSL(AmiSSLBase)

UI_METHOD * __UI_create_method(__reg("a6") struct Library *, __reg("a0") char * name)="\tjsr\t-9672(a6)";
#define UI_create_method(name) __UI_create_method(AmiSSLBase, (name))

void __UI_destroy_method(__reg("a6") struct Library *, __reg("a0") UI_METHOD * ui_method)="\tjsr\t-9678(a6)";
#define UI_destroy_method(ui_method) __UI_destroy_method(AmiSSLBase, (ui_method))

int __UI_method_set_opener(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method, __reg("a1") int (*opener)(UI *ui))="\tjsr\t-9684(a6)";
#define UI_method_set_opener(method, opener) __UI_method_set_opener(AmiSSLBase, (method), (opener))

int __UI_method_set_writer(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method, __reg("a1") int (*writer)(UI *ui, UI_STRING *uis))="\tjsr\t-9690(a6)";
#define UI_method_set_writer(method, writer) __UI_method_set_writer(AmiSSLBase, (method), (writer))

int __UI_method_set_flusher(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method, __reg("a1") int (*flusher)(UI *ui))="\tjsr\t-9696(a6)";
#define UI_method_set_flusher(method, flusher) __UI_method_set_flusher(AmiSSLBase, (method), (flusher))

int __UI_method_set_reader(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method, __reg("a1") int (*reader)(UI *ui, UI_STRING *uis))="\tjsr\t-9702(a6)";
#define UI_method_set_reader(method, reader) __UI_method_set_reader(AmiSSLBase, (method), (reader))

int __UI_method_set_closer(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method, __reg("a1") int (*closer)(UI *ui))="\tjsr\t-9708(a6)";
#define UI_method_set_closer(method, closer) __UI_method_set_closer(AmiSSLBase, (method), (closer))

void * __UI_method_get_opener(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method)="\tjsr\t-9714(a6)";
#define UI_method_get_opener(method) __UI_method_get_opener(AmiSSLBase, (method))

void * __UI_method_get_writer(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method)="\tjsr\t-9720(a6)";
#define UI_method_get_writer(method) __UI_method_get_writer(AmiSSLBase, (method))

void * __UI_method_get_flusher(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method)="\tjsr\t-9726(a6)";
#define UI_method_get_flusher(method) __UI_method_get_flusher(AmiSSLBase, (method))

void * __UI_method_get_reader(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method)="\tjsr\t-9732(a6)";
#define UI_method_get_reader(method) __UI_method_get_reader(AmiSSLBase, (method))

void * __UI_method_get_closer(__reg("a6") struct Library *, __reg("a0") UI_METHOD * method)="\tjsr\t-9738(a6)";
#define UI_method_get_closer(method) __UI_method_get_closer(AmiSSLBase, (method))

enum UI_string_types __UI_get_string_type(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9744(a6)";
#define UI_get_string_type(uis) __UI_get_string_type(AmiSSLBase, (uis))

int __UI_get_input_flags(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9750(a6)";
#define UI_get_input_flags(uis) __UI_get_input_flags(AmiSSLBase, (uis))

const char * __UI_get0_output_string(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9756(a6)";
#define UI_get0_output_string(uis) __UI_get0_output_string(AmiSSLBase, (uis))

const char * __UI_get0_action_string(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9762(a6)";
#define UI_get0_action_string(uis) __UI_get0_action_string(AmiSSLBase, (uis))

const char * __UI_get0_result_string(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9768(a6)";
#define UI_get0_result_string(uis) __UI_get0_result_string(AmiSSLBase, (uis))

const char * __UI_get0_test_string(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9774(a6)";
#define UI_get0_test_string(uis) __UI_get0_test_string(AmiSSLBase, (uis))

int __UI_get_result_minsize(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9780(a6)";
#define UI_get_result_minsize(uis) __UI_get_result_minsize(AmiSSLBase, (uis))

int __UI_get_result_maxsize(__reg("a6") struct Library *, __reg("a0") UI_STRING * uis)="\tjsr\t-9786(a6)";
#define UI_get_result_maxsize(uis) __UI_get_result_maxsize(AmiSSLBase, (uis))

int __UI_set_result(__reg("a6") struct Library *, __reg("a0") UI * ui, __reg("a1") UI_STRING * uis, __reg("a2") const char * result)="\tjsr\t-9792(a6)";
#define UI_set_result(ui, uis, result) __UI_set_result(AmiSSLBase, (ui), (uis), (result))

int __UI_UTIL_read_pw_string(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG length, __reg("a1") const char * prompt, __reg("d1") LONG verify)="\tjsr\t-9798(a6)";
#define UI_UTIL_read_pw_string(buf, length, prompt, verify) __UI_UTIL_read_pw_string(AmiSSLBase, (buf), (length), (prompt), (verify))

int __UI_UTIL_read_pw(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("a1") char * buff, __reg("d0") LONG size, __reg("a2") const char * prompt, __reg("d1") LONG verify)="\tjsr\t-9804(a6)";
#define UI_UTIL_read_pw(buf, buff, size, prompt, verify) __UI_UTIL_read_pw(AmiSSLBase, (buf), (buff), (size), (prompt), (verify))

void __ERR_load_UI_strings(__reg("a6") struct Library *)="\tjsr\t-9810(a6)";
#define ERR_load_UI_strings() __ERR_load_UI_strings(AmiSSLBase)

int ___ossl_old_des_read_pw_string(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("d0") LONG length, __reg("a1") const char * prompt, __reg("d1") LONG verify)="\tjsr\t-9816(a6)";
#define _ossl_old_des_read_pw_string(buf, length, prompt, verify) ___ossl_old_des_read_pw_string(AmiSSLBase, (buf), (length), (prompt), (verify))

int ___ossl_old_des_read_pw(__reg("a6") struct Library *, __reg("a0") char * buf, __reg("a1") char * buff, __reg("d0") LONG size, __reg("a2") const char * prompt, __reg("d1") LONG verify)="\tjsr\t-9822(a6)";
#define _ossl_old_des_read_pw(buf, buff, size, prompt, verify) ___ossl_old_des_read_pw(AmiSSLBase, (buf), (buff), (size), (prompt), (verify))

const char * __X509_verify_cert_error_string(__reg("a6") struct Library *, __reg("d0") long n)="\tjsr\t-9828(a6)";
#define X509_verify_cert_error_string(n) __X509_verify_cert_error_string(AmiSSLBase, (n))

int __X509_verify(__reg("a6") struct Library *, __reg("a0") X509 * a, __reg("a1") EVP_PKEY * r)="\tjsr\t-9834(a6)";
#define X509_verify(a, r) __X509_verify(AmiSSLBase, (a), (r))

int __X509_REQ_verify(__reg("a6") struct Library *, __reg("a0") X509_REQ * a, __reg("a1") EVP_PKEY * r)="\tjsr\t-9840(a6)";
#define X509_REQ_verify(a, r) __X509_REQ_verify(AmiSSLBase, (a), (r))

int __X509_CRL_verify(__reg("a6") struct Library *, __reg("a0") X509_CRL * a, __reg("a1") EVP_PKEY * r)="\tjsr\t-9846(a6)";
#define X509_CRL_verify(a, r) __X509_CRL_verify(AmiSSLBase, (a), (r))

int __NETSCAPE_SPKI_verify(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * a, __reg("a1") EVP_PKEY * r)="\tjsr\t-9852(a6)";
#define NETSCAPE_SPKI_verify(a, r) __NETSCAPE_SPKI_verify(AmiSSLBase, (a), (r))

NETSCAPE_SPKI * __NETSCAPE_SPKI_b64_decode(__reg("a6") struct Library *, __reg("a0") const char * str, __reg("d0") LONG len)="\tjsr\t-9858(a6)";
#define NETSCAPE_SPKI_b64_decode(str, len) __NETSCAPE_SPKI_b64_decode(AmiSSLBase, (str), (len))

char * __NETSCAPE_SPKI_b64_encode(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * x)="\tjsr\t-9864(a6)";
#define NETSCAPE_SPKI_b64_encode(x) __NETSCAPE_SPKI_b64_encode(AmiSSLBase, (x))

EVP_PKEY * __NETSCAPE_SPKI_get_pubkey(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * x)="\tjsr\t-9870(a6)";
#define NETSCAPE_SPKI_get_pubkey(x) __NETSCAPE_SPKI_get_pubkey(AmiSSLBase, (x))

int __NETSCAPE_SPKI_set_pubkey(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * x, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-9876(a6)";
#define NETSCAPE_SPKI_set_pubkey(x, pkey) __NETSCAPE_SPKI_set_pubkey(AmiSSLBase, (x), (pkey))

int __NETSCAPE_SPKI_print(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") NETSCAPE_SPKI * spki)="\tjsr\t-9882(a6)";
#define NETSCAPE_SPKI_print(out, spki) __NETSCAPE_SPKI_print(AmiSSLBase, (out), (spki))

int __X509_signature_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_ALGOR * alg, __reg("a2") ASN1_STRING * sig)="\tjsr\t-9888(a6)";
#define X509_signature_print(bp, alg, sig) __X509_signature_print(AmiSSLBase, (bp), (alg), (sig))

int __X509_sign(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") EVP_PKEY * pkey, __reg("a2") const EVP_MD * md)="\tjsr\t-9894(a6)";
#define X509_sign(x, pkey, md) __X509_sign(AmiSSLBase, (x), (pkey), (md))

int __X509_REQ_sign(__reg("a6") struct Library *, __reg("a0") X509_REQ * x, __reg("a1") EVP_PKEY * pkey, __reg("a2") const EVP_MD * md)="\tjsr\t-9900(a6)";
#define X509_REQ_sign(x, pkey, md) __X509_REQ_sign(AmiSSLBase, (x), (pkey), (md))

int __X509_CRL_sign(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") EVP_PKEY * pkey, __reg("a2") const EVP_MD * md)="\tjsr\t-9906(a6)";
#define X509_CRL_sign(x, pkey, md) __X509_CRL_sign(AmiSSLBase, (x), (pkey), (md))

int __NETSCAPE_SPKI_sign(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * x, __reg("a1") EVP_PKEY * pkey, __reg("a2") const EVP_MD * md)="\tjsr\t-9912(a6)";
#define NETSCAPE_SPKI_sign(x, pkey, md) __NETSCAPE_SPKI_sign(AmiSSLBase, (x), (pkey), (md))

int __X509_pubkey_digest(__reg("a6") struct Library *, __reg("a0") const X509 * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-9918(a6)";
#define X509_pubkey_digest(data, type, md, len) __X509_pubkey_digest(AmiSSLBase, (data), (type), (md), (len))

int __X509_digest(__reg("a6") struct Library *, __reg("a0") const X509 * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-9924(a6)";
#define X509_digest(data, type, md, len) __X509_digest(AmiSSLBase, (data), (type), (md), (len))

int __X509_CRL_digest(__reg("a6") struct Library *, __reg("a0") const X509_CRL * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-9930(a6)";
#define X509_CRL_digest(data, type, md, len) __X509_CRL_digest(AmiSSLBase, (data), (type), (md), (len))

int __X509_REQ_digest(__reg("a6") struct Library *, __reg("a0") const X509_REQ * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-9936(a6)";
#define X509_REQ_digest(data, type, md, len) __X509_REQ_digest(AmiSSLBase, (data), (type), (md), (len))

int __X509_NAME_digest(__reg("a6") struct Library *, __reg("a0") const X509_NAME * data, __reg("a1") const EVP_MD * type, __reg("a2") unsigned char * md, __reg("a3") unsigned int * len)="\tjsr\t-9942(a6)";
#define X509_NAME_digest(data, type, md, len) __X509_NAME_digest(AmiSSLBase, (data), (type), (md), (len))

X509 * __d2i_X509_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 ** x509)="\tjsr\t-9948(a6)";
#define d2i_X509_bio(bp, x509) __d2i_X509_bio(AmiSSLBase, (bp), (x509))

int __i2d_X509_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x509)="\tjsr\t-9954(a6)";
#define i2d_X509_bio(bp, x509) __i2d_X509_bio(AmiSSLBase, (bp), (x509))

X509_CRL * __d2i_X509_CRL_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CRL ** crl)="\tjsr\t-9960(a6)";
#define d2i_X509_CRL_bio(bp, crl) __d2i_X509_CRL_bio(AmiSSLBase, (bp), (crl))

int __i2d_X509_CRL_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CRL * crl)="\tjsr\t-9966(a6)";
#define i2d_X509_CRL_bio(bp, crl) __i2d_X509_CRL_bio(AmiSSLBase, (bp), (crl))

X509_REQ * __d2i_X509_REQ_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ ** req)="\tjsr\t-9972(a6)";
#define d2i_X509_REQ_bio(bp, req) __d2i_X509_REQ_bio(AmiSSLBase, (bp), (req))

int __i2d_X509_REQ_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ * req)="\tjsr\t-9978(a6)";
#define i2d_X509_REQ_bio(bp, req) __i2d_X509_REQ_bio(AmiSSLBase, (bp), (req))

RSA * __d2i_RSAPrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** rsa)="\tjsr\t-9984(a6)";
#define d2i_RSAPrivateKey_bio(bp, rsa) __d2i_RSAPrivateKey_bio(AmiSSLBase, (bp), (rsa))

int __i2d_RSAPrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * rsa)="\tjsr\t-9990(a6)";
#define i2d_RSAPrivateKey_bio(bp, rsa) __i2d_RSAPrivateKey_bio(AmiSSLBase, (bp), (rsa))

RSA * __d2i_RSAPublicKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** rsa)="\tjsr\t-9996(a6)";
#define d2i_RSAPublicKey_bio(bp, rsa) __d2i_RSAPublicKey_bio(AmiSSLBase, (bp), (rsa))

int __i2d_RSAPublicKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * rsa)="\tjsr\t-10002(a6)";
#define i2d_RSAPublicKey_bio(bp, rsa) __i2d_RSAPublicKey_bio(AmiSSLBase, (bp), (rsa))

RSA * __d2i_RSA_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA ** rsa)="\tjsr\t-10008(a6)";
#define d2i_RSA_PUBKEY_bio(bp, rsa) __d2i_RSA_PUBKEY_bio(AmiSSLBase, (bp), (rsa))

int __i2d_RSA_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") RSA * rsa)="\tjsr\t-10014(a6)";
#define i2d_RSA_PUBKEY_bio(bp, rsa) __i2d_RSA_PUBKEY_bio(AmiSSLBase, (bp), (rsa))

DSA * __d2i_DSA_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA ** dsa)="\tjsr\t-10020(a6)";
#define d2i_DSA_PUBKEY_bio(bp, dsa) __d2i_DSA_PUBKEY_bio(AmiSSLBase, (bp), (dsa))

int __i2d_DSA_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA * dsa)="\tjsr\t-10026(a6)";
#define i2d_DSA_PUBKEY_bio(bp, dsa) __i2d_DSA_PUBKEY_bio(AmiSSLBase, (bp), (dsa))

DSA * __d2i_DSAPrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA ** dsa)="\tjsr\t-10032(a6)";
#define d2i_DSAPrivateKey_bio(bp, dsa) __d2i_DSAPrivateKey_bio(AmiSSLBase, (bp), (dsa))

int __i2d_DSAPrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") DSA * dsa)="\tjsr\t-10038(a6)";
#define i2d_DSAPrivateKey_bio(bp, dsa) __i2d_DSAPrivateKey_bio(AmiSSLBase, (bp), (dsa))

X509_SIG * __d2i_PKCS8_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_SIG ** p8)="\tjsr\t-10044(a6)";
#define d2i_PKCS8_bio(bp, p8) __d2i_PKCS8_bio(AmiSSLBase, (bp), (p8))

int __i2d_PKCS8_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_SIG * p8)="\tjsr\t-10050(a6)";
#define i2d_PKCS8_bio(bp, p8) __i2d_PKCS8_bio(AmiSSLBase, (bp), (p8))

PKCS8_PRIV_KEY_INFO * __d2i_PKCS8_PRIV_KEY_INFO_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS8_PRIV_KEY_INFO ** p8inf)="\tjsr\t-10056(a6)";
#define d2i_PKCS8_PRIV_KEY_INFO_bio(bp, p8inf) __d2i_PKCS8_PRIV_KEY_INFO_bio(AmiSSLBase, (bp), (p8inf))

int __i2d_PKCS8_PRIV_KEY_INFO_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") PKCS8_PRIV_KEY_INFO * p8inf)="\tjsr\t-10062(a6)";
#define i2d_PKCS8_PRIV_KEY_INFO_bio(bp, p8inf) __i2d_PKCS8_PRIV_KEY_INFO_bio(AmiSSLBase, (bp), (p8inf))

int __i2d_PKCS8PrivateKeyInfo_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * key)="\tjsr\t-10068(a6)";
#define i2d_PKCS8PrivateKeyInfo_bio(bp, key) __i2d_PKCS8PrivateKeyInfo_bio(AmiSSLBase, (bp), (key))

int __i2d_PrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-10074(a6)";
#define i2d_PrivateKey_bio(bp, pkey) __i2d_PrivateKey_bio(AmiSSLBase, (bp), (pkey))

EVP_PKEY * __d2i_PrivateKey_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY ** a)="\tjsr\t-10080(a6)";
#define d2i_PrivateKey_bio(bp, a) __d2i_PrivateKey_bio(AmiSSLBase, (bp), (a))

int __i2d_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-10086(a6)";
#define i2d_PUBKEY_bio(bp, pkey) __i2d_PUBKEY_bio(AmiSSLBase, (bp), (pkey))

EVP_PKEY * __d2i_PUBKEY_bio(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") EVP_PKEY ** a)="\tjsr\t-10092(a6)";
#define d2i_PUBKEY_bio(bp, a) __d2i_PUBKEY_bio(AmiSSLBase, (bp), (a))

X509 * __X509_dup(__reg("a6") struct Library *, __reg("a0") X509 * x509)="\tjsr\t-10098(a6)";
#define X509_dup(x509) __X509_dup(AmiSSLBase, (x509))

X509_ATTRIBUTE * __X509_ATTRIBUTE_dup(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * xa)="\tjsr\t-10104(a6)";
#define X509_ATTRIBUTE_dup(xa) __X509_ATTRIBUTE_dup(AmiSSLBase, (xa))

X509_EXTENSION * __X509_EXTENSION_dup(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex)="\tjsr\t-10110(a6)";
#define X509_EXTENSION_dup(ex) __X509_EXTENSION_dup(AmiSSLBase, (ex))

X509_CRL * __X509_CRL_dup(__reg("a6") struct Library *, __reg("a0") X509_CRL * crl)="\tjsr\t-10116(a6)";
#define X509_CRL_dup(crl) __X509_CRL_dup(AmiSSLBase, (crl))

X509_REQ * __X509_REQ_dup(__reg("a6") struct Library *, __reg("a0") X509_REQ * req)="\tjsr\t-10122(a6)";
#define X509_REQ_dup(req) __X509_REQ_dup(AmiSSLBase, (req))

X509_ALGOR * __X509_ALGOR_dup(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * xn)="\tjsr\t-10128(a6)";
#define X509_ALGOR_dup(xn) __X509_ALGOR_dup(AmiSSLBase, (xn))

X509_NAME * __X509_NAME_dup(__reg("a6") struct Library *, __reg("a0") X509_NAME * xn)="\tjsr\t-10134(a6)";
#define X509_NAME_dup(xn) __X509_NAME_dup(AmiSSLBase, (xn))

X509_NAME_ENTRY * __X509_NAME_ENTRY_dup(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * ne)="\tjsr\t-10140(a6)";
#define X509_NAME_ENTRY_dup(ne) __X509_NAME_ENTRY_dup(AmiSSLBase, (ne))

int __X509_cmp_time(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * s, __reg("a1") time_t * t)="\tjsr\t-10146(a6)";
#define X509_cmp_time(s, t) __X509_cmp_time(AmiSSLBase, (s), (t))

int __X509_cmp_current_time(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * s)="\tjsr\t-10152(a6)";
#define X509_cmp_current_time(s) __X509_cmp_current_time(AmiSSLBase, (s))

ASN1_TIME * __X509_time_adj(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * s, __reg("d0") long adj, __reg("a1") time_t * t)="\tjsr\t-10158(a6)";
#define X509_time_adj(s, adj, t) __X509_time_adj(AmiSSLBase, (s), (adj), (t))

ASN1_TIME * __X509_gmtime_adj(__reg("a6") struct Library *, __reg("a0") ASN1_TIME * s, __reg("d0") long adj)="\tjsr\t-10164(a6)";
#define X509_gmtime_adj(s, adj) __X509_gmtime_adj(AmiSSLBase, (s), (adj))

const char * __X509_get_default_cert_area(__reg("a6") struct Library *)="\tjsr\t-10170(a6)";
#define X509_get_default_cert_area() __X509_get_default_cert_area(AmiSSLBase)

const char * __X509_get_default_cert_dir(__reg("a6") struct Library *)="\tjsr\t-10176(a6)";
#define X509_get_default_cert_dir() __X509_get_default_cert_dir(AmiSSLBase)

const char * __X509_get_default_cert_file(__reg("a6") struct Library *)="\tjsr\t-10182(a6)";
#define X509_get_default_cert_file() __X509_get_default_cert_file(AmiSSLBase)

const char * __X509_get_default_cert_dir_env(__reg("a6") struct Library *)="\tjsr\t-10188(a6)";
#define X509_get_default_cert_dir_env() __X509_get_default_cert_dir_env(AmiSSLBase)

const char * __X509_get_default_cert_file_env(__reg("a6") struct Library *)="\tjsr\t-10194(a6)";
#define X509_get_default_cert_file_env() __X509_get_default_cert_file_env(AmiSSLBase)

const char * __X509_get_default_private_dir(__reg("a6") struct Library *)="\tjsr\t-10200(a6)";
#define X509_get_default_private_dir() __X509_get_default_private_dir(AmiSSLBase)

X509_REQ * __X509_to_X509_REQ(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") EVP_PKEY * pkey, __reg("a2") const EVP_MD * md)="\tjsr\t-10206(a6)";
#define X509_to_X509_REQ(x, pkey, md) __X509_to_X509_REQ(AmiSSLBase, (x), (pkey), (md))

X509 * __X509_REQ_to_X509(__reg("a6") struct Library *, __reg("a0") X509_REQ * r, __reg("d0") LONG days, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-10212(a6)";
#define X509_REQ_to_X509(r, days, pkey) __X509_REQ_to_X509(AmiSSLBase, (r), (days), (pkey))

X509_ALGOR * __X509_ALGOR_new(__reg("a6") struct Library *)="\tjsr\t-10218(a6)";
#define X509_ALGOR_new() __X509_ALGOR_new(AmiSSLBase)

void __X509_ALGOR_free(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * a)="\tjsr\t-10224(a6)";
#define X509_ALGOR_free(a) __X509_ALGOR_free(AmiSSLBase, (a))

X509_ALGOR * __d2i_X509_ALGOR(__reg("a6") struct Library *, __reg("a0") X509_ALGOR ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10230(a6)";
#define d2i_X509_ALGOR(a, in, len) __d2i_X509_ALGOR(AmiSSLBase, (a), (in), (len))

int __i2d_X509_ALGOR(__reg("a6") struct Library *, __reg("a0") X509_ALGOR * a, __reg("a1") unsigned char ** out)="\tjsr\t-10236(a6)";
#define i2d_X509_ALGOR(a, out) __i2d_X509_ALGOR(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_ALGOR_it(__reg("a6") struct Library *)="\tjsr\t-10242(a6)";
#define X509_ALGOR_it() __X509_ALGOR_it(AmiSSLBase)

X509_VAL * __X509_VAL_new(__reg("a6") struct Library *)="\tjsr\t-10248(a6)";
#define X509_VAL_new() __X509_VAL_new(AmiSSLBase)

void __X509_VAL_free(__reg("a6") struct Library *, __reg("a0") X509_VAL * a)="\tjsr\t-10254(a6)";
#define X509_VAL_free(a) __X509_VAL_free(AmiSSLBase, (a))

X509_VAL * __d2i_X509_VAL(__reg("a6") struct Library *, __reg("a0") X509_VAL ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10260(a6)";
#define d2i_X509_VAL(a, in, len) __d2i_X509_VAL(AmiSSLBase, (a), (in), (len))

int __i2d_X509_VAL(__reg("a6") struct Library *, __reg("a0") X509_VAL * a, __reg("a1") unsigned char ** out)="\tjsr\t-10266(a6)";
#define i2d_X509_VAL(a, out) __i2d_X509_VAL(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_VAL_it(__reg("a6") struct Library *)="\tjsr\t-10272(a6)";
#define X509_VAL_it() __X509_VAL_it(AmiSSLBase)

X509_PUBKEY * __X509_PUBKEY_new(__reg("a6") struct Library *)="\tjsr\t-10278(a6)";
#define X509_PUBKEY_new() __X509_PUBKEY_new(AmiSSLBase)

void __X509_PUBKEY_free(__reg("a6") struct Library *, __reg("a0") X509_PUBKEY * a)="\tjsr\t-10284(a6)";
#define X509_PUBKEY_free(a) __X509_PUBKEY_free(AmiSSLBase, (a))

X509_PUBKEY * __d2i_X509_PUBKEY(__reg("a6") struct Library *, __reg("a0") X509_PUBKEY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10290(a6)";
#define d2i_X509_PUBKEY(a, in, len) __d2i_X509_PUBKEY(AmiSSLBase, (a), (in), (len))

int __i2d_X509_PUBKEY(__reg("a6") struct Library *, __reg("a0") X509_PUBKEY * a, __reg("a1") unsigned char ** out)="\tjsr\t-10296(a6)";
#define i2d_X509_PUBKEY(a, out) __i2d_X509_PUBKEY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_PUBKEY_it(__reg("a6") struct Library *)="\tjsr\t-10302(a6)";
#define X509_PUBKEY_it() __X509_PUBKEY_it(AmiSSLBase)

int __X509_PUBKEY_set(__reg("a6") struct Library *, __reg("a0") X509_PUBKEY ** x, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-10308(a6)";
#define X509_PUBKEY_set(x, pkey) __X509_PUBKEY_set(AmiSSLBase, (x), (pkey))

EVP_PKEY * __X509_PUBKEY_get(__reg("a6") struct Library *, __reg("a0") X509_PUBKEY * key)="\tjsr\t-10314(a6)";
#define X509_PUBKEY_get(key) __X509_PUBKEY_get(AmiSSLBase, (key))

int __X509_get_pubkey_parameters(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("a1") void * chain)="\tjsr\t-10320(a6)";
#define X509_get_pubkey_parameters(pkey, chain) __X509_get_pubkey_parameters(AmiSSLBase, (pkey), (chain))

int __i2d_PUBKEY(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * a, __reg("a1") unsigned char ** pp)="\tjsr\t-10326(a6)";
#define i2d_PUBKEY(a, pp) __i2d_PUBKEY(AmiSSLBase, (a), (pp))

EVP_PKEY * __d2i_PUBKEY(__reg("a6") struct Library *, __reg("a0") EVP_PKEY ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-10332(a6)";
#define d2i_PUBKEY(a, pp, length) __d2i_PUBKEY(AmiSSLBase, (a), (pp), (length))

int __i2d_RSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") RSA * a, __reg("a1") unsigned char ** pp)="\tjsr\t-10338(a6)";
#define i2d_RSA_PUBKEY(a, pp) __i2d_RSA_PUBKEY(AmiSSLBase, (a), (pp))

RSA * __d2i_RSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") RSA ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-10344(a6)";
#define d2i_RSA_PUBKEY(a, pp, length) __d2i_RSA_PUBKEY(AmiSSLBase, (a), (pp), (length))

int __i2d_DSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") DSA * a, __reg("a1") unsigned char ** pp)="\tjsr\t-10350(a6)";
#define i2d_DSA_PUBKEY(a, pp) __i2d_DSA_PUBKEY(AmiSSLBase, (a), (pp))

DSA * __d2i_DSA_PUBKEY(__reg("a6") struct Library *, __reg("a0") DSA ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-10356(a6)";
#define d2i_DSA_PUBKEY(a, pp, length) __d2i_DSA_PUBKEY(AmiSSLBase, (a), (pp), (length))

X509_SIG * __X509_SIG_new(__reg("a6") struct Library *)="\tjsr\t-10362(a6)";
#define X509_SIG_new() __X509_SIG_new(AmiSSLBase)

void __X509_SIG_free(__reg("a6") struct Library *, __reg("a0") X509_SIG * a)="\tjsr\t-10368(a6)";
#define X509_SIG_free(a) __X509_SIG_free(AmiSSLBase, (a))

X509_SIG * __d2i_X509_SIG(__reg("a6") struct Library *, __reg("a0") X509_SIG ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10374(a6)";
#define d2i_X509_SIG(a, in, len) __d2i_X509_SIG(AmiSSLBase, (a), (in), (len))

int __i2d_X509_SIG(__reg("a6") struct Library *, __reg("a0") X509_SIG * a, __reg("a1") unsigned char ** out)="\tjsr\t-10380(a6)";
#define i2d_X509_SIG(a, out) __i2d_X509_SIG(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_SIG_it(__reg("a6") struct Library *)="\tjsr\t-10386(a6)";
#define X509_SIG_it() __X509_SIG_it(AmiSSLBase)

X509_REQ_INFO * __X509_REQ_INFO_new(__reg("a6") struct Library *)="\tjsr\t-10392(a6)";
#define X509_REQ_INFO_new() __X509_REQ_INFO_new(AmiSSLBase)

void __X509_REQ_INFO_free(__reg("a6") struct Library *, __reg("a0") X509_REQ_INFO * a)="\tjsr\t-10398(a6)";
#define X509_REQ_INFO_free(a) __X509_REQ_INFO_free(AmiSSLBase, (a))

X509_REQ_INFO * __d2i_X509_REQ_INFO(__reg("a6") struct Library *, __reg("a0") X509_REQ_INFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10404(a6)";
#define d2i_X509_REQ_INFO(a, in, len) __d2i_X509_REQ_INFO(AmiSSLBase, (a), (in), (len))

int __i2d_X509_REQ_INFO(__reg("a6") struct Library *, __reg("a0") X509_REQ_INFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-10410(a6)";
#define i2d_X509_REQ_INFO(a, out) __i2d_X509_REQ_INFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_REQ_INFO_it(__reg("a6") struct Library *)="\tjsr\t-10416(a6)";
#define X509_REQ_INFO_it() __X509_REQ_INFO_it(AmiSSLBase)

X509_REQ * __X509_REQ_new(__reg("a6") struct Library *)="\tjsr\t-10422(a6)";
#define X509_REQ_new() __X509_REQ_new(AmiSSLBase)

void __X509_REQ_free(__reg("a6") struct Library *, __reg("a0") X509_REQ * a)="\tjsr\t-10428(a6)";
#define X509_REQ_free(a) __X509_REQ_free(AmiSSLBase, (a))

X509_REQ * __d2i_X509_REQ(__reg("a6") struct Library *, __reg("a0") X509_REQ ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10434(a6)";
#define d2i_X509_REQ(a, in, len) __d2i_X509_REQ(AmiSSLBase, (a), (in), (len))

int __i2d_X509_REQ(__reg("a6") struct Library *, __reg("a0") X509_REQ * a, __reg("a1") unsigned char ** out)="\tjsr\t-10440(a6)";
#define i2d_X509_REQ(a, out) __i2d_X509_REQ(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_REQ_it(__reg("a6") struct Library *)="\tjsr\t-10446(a6)";
#define X509_REQ_it() __X509_REQ_it(AmiSSLBase)

X509_ATTRIBUTE * __X509_ATTRIBUTE_new(__reg("a6") struct Library *)="\tjsr\t-10452(a6)";
#define X509_ATTRIBUTE_new() __X509_ATTRIBUTE_new(AmiSSLBase)

void __X509_ATTRIBUTE_free(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * a)="\tjsr\t-10458(a6)";
#define X509_ATTRIBUTE_free(a) __X509_ATTRIBUTE_free(AmiSSLBase, (a))

X509_ATTRIBUTE * __d2i_X509_ATTRIBUTE(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10464(a6)";
#define d2i_X509_ATTRIBUTE(a, in, len) __d2i_X509_ATTRIBUTE(AmiSSLBase, (a), (in), (len))

int __i2d_X509_ATTRIBUTE(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * a, __reg("a1") unsigned char ** out)="\tjsr\t-10470(a6)";
#define i2d_X509_ATTRIBUTE(a, out) __i2d_X509_ATTRIBUTE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_ATTRIBUTE_it(__reg("a6") struct Library *)="\tjsr\t-10476(a6)";
#define X509_ATTRIBUTE_it() __X509_ATTRIBUTE_it(AmiSSLBase)

X509_ATTRIBUTE * __X509_ATTRIBUTE_create(__reg("a6") struct Library *, __reg("d0") LONG nid, __reg("d1") LONG atrtype, __reg("a0") void * value)="\tjsr\t-10482(a6)";
#define X509_ATTRIBUTE_create(nid, atrtype, value) __X509_ATTRIBUTE_create(AmiSSLBase, (nid), (atrtype), (value))

X509_EXTENSION * __X509_EXTENSION_new(__reg("a6") struct Library *)="\tjsr\t-10488(a6)";
#define X509_EXTENSION_new() __X509_EXTENSION_new(AmiSSLBase)

void __X509_EXTENSION_free(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * a)="\tjsr\t-10494(a6)";
#define X509_EXTENSION_free(a) __X509_EXTENSION_free(AmiSSLBase, (a))

X509_EXTENSION * __d2i_X509_EXTENSION(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10500(a6)";
#define d2i_X509_EXTENSION(a, in, len) __d2i_X509_EXTENSION(AmiSSLBase, (a), (in), (len))

int __i2d_X509_EXTENSION(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * a, __reg("a1") unsigned char ** out)="\tjsr\t-10506(a6)";
#define i2d_X509_EXTENSION(a, out) __i2d_X509_EXTENSION(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_EXTENSION_it(__reg("a6") struct Library *)="\tjsr\t-10512(a6)";
#define X509_EXTENSION_it() __X509_EXTENSION_it(AmiSSLBase)

X509_NAME_ENTRY * __X509_NAME_ENTRY_new(__reg("a6") struct Library *)="\tjsr\t-10518(a6)";
#define X509_NAME_ENTRY_new() __X509_NAME_ENTRY_new(AmiSSLBase)

void __X509_NAME_ENTRY_free(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * a)="\tjsr\t-10524(a6)";
#define X509_NAME_ENTRY_free(a) __X509_NAME_ENTRY_free(AmiSSLBase, (a))

X509_NAME_ENTRY * __d2i_X509_NAME_ENTRY(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10530(a6)";
#define d2i_X509_NAME_ENTRY(a, in, len) __d2i_X509_NAME_ENTRY(AmiSSLBase, (a), (in), (len))

int __i2d_X509_NAME_ENTRY(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * a, __reg("a1") unsigned char ** out)="\tjsr\t-10536(a6)";
#define i2d_X509_NAME_ENTRY(a, out) __i2d_X509_NAME_ENTRY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_NAME_ENTRY_it(__reg("a6") struct Library *)="\tjsr\t-10542(a6)";
#define X509_NAME_ENTRY_it() __X509_NAME_ENTRY_it(AmiSSLBase)

X509_NAME * __X509_NAME_new(__reg("a6") struct Library *)="\tjsr\t-10548(a6)";
#define X509_NAME_new() __X509_NAME_new(AmiSSLBase)

void __X509_NAME_free(__reg("a6") struct Library *, __reg("a0") X509_NAME * a)="\tjsr\t-10554(a6)";
#define X509_NAME_free(a) __X509_NAME_free(AmiSSLBase, (a))

X509_NAME * __d2i_X509_NAME(__reg("a6") struct Library *, __reg("a0") X509_NAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10560(a6)";
#define d2i_X509_NAME(a, in, len) __d2i_X509_NAME(AmiSSLBase, (a), (in), (len))

int __i2d_X509_NAME(__reg("a6") struct Library *, __reg("a0") X509_NAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-10566(a6)";
#define i2d_X509_NAME(a, out) __i2d_X509_NAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_NAME_it(__reg("a6") struct Library *)="\tjsr\t-10572(a6)";
#define X509_NAME_it() __X509_NAME_it(AmiSSLBase)

int __X509_NAME_set(__reg("a6") struct Library *, __reg("a0") X509_NAME ** xn, __reg("a1") X509_NAME * name)="\tjsr\t-10578(a6)";
#define X509_NAME_set(xn, name) __X509_NAME_set(AmiSSLBase, (xn), (name))

X509_CINF * __X509_CINF_new(__reg("a6") struct Library *)="\tjsr\t-10584(a6)";
#define X509_CINF_new() __X509_CINF_new(AmiSSLBase)

void __X509_CINF_free(__reg("a6") struct Library *, __reg("a0") X509_CINF * a)="\tjsr\t-10590(a6)";
#define X509_CINF_free(a) __X509_CINF_free(AmiSSLBase, (a))

X509_CINF * __d2i_X509_CINF(__reg("a6") struct Library *, __reg("a0") X509_CINF ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10596(a6)";
#define d2i_X509_CINF(a, in, len) __d2i_X509_CINF(AmiSSLBase, (a), (in), (len))

int __i2d_X509_CINF(__reg("a6") struct Library *, __reg("a0") X509_CINF * a, __reg("a1") unsigned char ** out)="\tjsr\t-10602(a6)";
#define i2d_X509_CINF(a, out) __i2d_X509_CINF(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_CINF_it(__reg("a6") struct Library *)="\tjsr\t-10608(a6)";
#define X509_CINF_it() __X509_CINF_it(AmiSSLBase)

X509 * __X509_new(__reg("a6") struct Library *)="\tjsr\t-10614(a6)";
#define X509_new() __X509_new(AmiSSLBase)

void __X509_free(__reg("a6") struct Library *, __reg("a0") X509 * a)="\tjsr\t-10620(a6)";
#define X509_free(a) __X509_free(AmiSSLBase, (a))

X509 * __d2i_X509(__reg("a6") struct Library *, __reg("a0") X509 ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10626(a6)";
#define d2i_X509(a, in, len) __d2i_X509(AmiSSLBase, (a), (in), (len))

int __i2d_X509(__reg("a6") struct Library *, __reg("a0") X509 * a, __reg("a1") unsigned char ** out)="\tjsr\t-10632(a6)";
#define i2d_X509(a, out) __i2d_X509(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_it(__reg("a6") struct Library *)="\tjsr\t-10638(a6)";
#define X509_it() __X509_it(AmiSSLBase)

X509_CERT_AUX * __X509_CERT_AUX_new(__reg("a6") struct Library *)="\tjsr\t-10644(a6)";
#define X509_CERT_AUX_new() __X509_CERT_AUX_new(AmiSSLBase)

void __X509_CERT_AUX_free(__reg("a6") struct Library *, __reg("a0") X509_CERT_AUX * a)="\tjsr\t-10650(a6)";
#define X509_CERT_AUX_free(a) __X509_CERT_AUX_free(AmiSSLBase, (a))

X509_CERT_AUX * __d2i_X509_CERT_AUX(__reg("a6") struct Library *, __reg("a0") X509_CERT_AUX ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10656(a6)";
#define d2i_X509_CERT_AUX(a, in, len) __d2i_X509_CERT_AUX(AmiSSLBase, (a), (in), (len))

int __i2d_X509_CERT_AUX(__reg("a6") struct Library *, __reg("a0") X509_CERT_AUX * a, __reg("a1") unsigned char ** out)="\tjsr\t-10662(a6)";
#define i2d_X509_CERT_AUX(a, out) __i2d_X509_CERT_AUX(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_CERT_AUX_it(__reg("a6") struct Library *)="\tjsr\t-10668(a6)";
#define X509_CERT_AUX_it() __X509_CERT_AUX_it(AmiSSLBase)

int __X509_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-10674(a6)";
#define X509_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __X509_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __X509_set_ex_data(__reg("a6") struct Library *, __reg("a0") X509 * r, __reg("d0") LONG idx, __reg("a1") void * arg)="\tjsr\t-10680(a6)";
#define X509_set_ex_data(r, idx, arg) __X509_set_ex_data(AmiSSLBase, (r), (idx), (arg))

void * __X509_get_ex_data(__reg("a6") struct Library *, __reg("a0") X509 * r, __reg("d0") LONG idx)="\tjsr\t-10686(a6)";
#define X509_get_ex_data(r, idx) __X509_get_ex_data(AmiSSLBase, (r), (idx))

int __i2d_X509_AUX(__reg("a6") struct Library *, __reg("a0") X509 * a, __reg("a1") unsigned char ** pp)="\tjsr\t-10692(a6)";
#define i2d_X509_AUX(a, pp) __i2d_X509_AUX(AmiSSLBase, (a), (pp))

X509 * __d2i_X509_AUX(__reg("a6") struct Library *, __reg("a0") X509 ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-10698(a6)";
#define d2i_X509_AUX(a, pp, length) __d2i_X509_AUX(AmiSSLBase, (a), (pp), (length))

int __X509_alias_set1(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") unsigned char * name, __reg("d0") LONG len)="\tjsr\t-10704(a6)";
#define X509_alias_set1(x, name, len) __X509_alias_set1(AmiSSLBase, (x), (name), (len))

int __X509_keyid_set1(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") unsigned char * id, __reg("d0") LONG len)="\tjsr\t-10710(a6)";
#define X509_keyid_set1(x, id, len) __X509_keyid_set1(AmiSSLBase, (x), (id), (len))

unsigned char * __X509_alias_get0(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") int * len)="\tjsr\t-10716(a6)";
#define X509_alias_get0(x, len) __X509_alias_get0(AmiSSLBase, (x), (len))

void * __X509_TRUST_set_default(__reg("a6") struct Library *, __reg("a0") int (*trust)(int, X509 *, int))="\tjsr\t-10722(a6)";
#define X509_TRUST_set_default(trust) __X509_TRUST_set_default(AmiSSLBase, (trust))

int __X509_TRUST_set(__reg("a6") struct Library *, __reg("a0") int * t, __reg("d0") LONG trust)="\tjsr\t-10728(a6)";
#define X509_TRUST_set(t, trust) __X509_TRUST_set(AmiSSLBase, (t), (trust))

int __X509_add1_trust_object(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_OBJECT * obj)="\tjsr\t-10734(a6)";
#define X509_add1_trust_object(x, obj) __X509_add1_trust_object(AmiSSLBase, (x), (obj))

int __X509_add1_reject_object(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_OBJECT * obj)="\tjsr\t-10740(a6)";
#define X509_add1_reject_object(x, obj) __X509_add1_reject_object(AmiSSLBase, (x), (obj))

void __X509_trust_clear(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-10746(a6)";
#define X509_trust_clear(x) __X509_trust_clear(AmiSSLBase, (x))

void __X509_reject_clear(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-10752(a6)";
#define X509_reject_clear(x) __X509_reject_clear(AmiSSLBase, (x))

X509_REVOKED * __X509_REVOKED_new(__reg("a6") struct Library *)="\tjsr\t-10758(a6)";
#define X509_REVOKED_new() __X509_REVOKED_new(AmiSSLBase)

void __X509_REVOKED_free(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * a)="\tjsr\t-10764(a6)";
#define X509_REVOKED_free(a) __X509_REVOKED_free(AmiSSLBase, (a))

X509_REVOKED * __d2i_X509_REVOKED(__reg("a6") struct Library *, __reg("a0") X509_REVOKED ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10770(a6)";
#define d2i_X509_REVOKED(a, in, len) __d2i_X509_REVOKED(AmiSSLBase, (a), (in), (len))

int __i2d_X509_REVOKED(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * a, __reg("a1") unsigned char ** out)="\tjsr\t-10776(a6)";
#define i2d_X509_REVOKED(a, out) __i2d_X509_REVOKED(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_REVOKED_it(__reg("a6") struct Library *)="\tjsr\t-10782(a6)";
#define X509_REVOKED_it() __X509_REVOKED_it(AmiSSLBase)

X509_CRL_INFO * __X509_CRL_INFO_new(__reg("a6") struct Library *)="\tjsr\t-10788(a6)";
#define X509_CRL_INFO_new() __X509_CRL_INFO_new(AmiSSLBase)

void __X509_CRL_INFO_free(__reg("a6") struct Library *, __reg("a0") X509_CRL_INFO * a)="\tjsr\t-10794(a6)";
#define X509_CRL_INFO_free(a) __X509_CRL_INFO_free(AmiSSLBase, (a))

X509_CRL_INFO * __d2i_X509_CRL_INFO(__reg("a6") struct Library *, __reg("a0") X509_CRL_INFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10800(a6)";
#define d2i_X509_CRL_INFO(a, in, len) __d2i_X509_CRL_INFO(AmiSSLBase, (a), (in), (len))

int __i2d_X509_CRL_INFO(__reg("a6") struct Library *, __reg("a0") X509_CRL_INFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-10806(a6)";
#define i2d_X509_CRL_INFO(a, out) __i2d_X509_CRL_INFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_CRL_INFO_it(__reg("a6") struct Library *)="\tjsr\t-10812(a6)";
#define X509_CRL_INFO_it() __X509_CRL_INFO_it(AmiSSLBase)

X509_CRL * __X509_CRL_new(__reg("a6") struct Library *)="\tjsr\t-10818(a6)";
#define X509_CRL_new() __X509_CRL_new(AmiSSLBase)

void __X509_CRL_free(__reg("a6") struct Library *, __reg("a0") X509_CRL * a)="\tjsr\t-10824(a6)";
#define X509_CRL_free(a) __X509_CRL_free(AmiSSLBase, (a))

X509_CRL * __d2i_X509_CRL(__reg("a6") struct Library *, __reg("a0") X509_CRL ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10830(a6)";
#define d2i_X509_CRL(a, in, len) __d2i_X509_CRL(AmiSSLBase, (a), (in), (len))

int __i2d_X509_CRL(__reg("a6") struct Library *, __reg("a0") X509_CRL * a, __reg("a1") unsigned char ** out)="\tjsr\t-10836(a6)";
#define i2d_X509_CRL(a, out) __i2d_X509_CRL(AmiSSLBase, (a), (out))

const ASN1_ITEM * __X509_CRL_it(__reg("a6") struct Library *)="\tjsr\t-10842(a6)";
#define X509_CRL_it() __X509_CRL_it(AmiSSLBase)

int __X509_CRL_add0_revoked(__reg("a6") struct Library *, __reg("a0") X509_CRL * crl, __reg("a1") X509_REVOKED * rev)="\tjsr\t-10848(a6)";
#define X509_CRL_add0_revoked(crl, rev) __X509_CRL_add0_revoked(AmiSSLBase, (crl), (rev))

X509_PKEY * __X509_PKEY_new(__reg("a6") struct Library *)="\tjsr\t-10854(a6)";
#define X509_PKEY_new() __X509_PKEY_new(AmiSSLBase)

void __X509_PKEY_free(__reg("a6") struct Library *, __reg("a0") X509_PKEY * a)="\tjsr\t-10860(a6)";
#define X509_PKEY_free(a) __X509_PKEY_free(AmiSSLBase, (a))

int __i2d_X509_PKEY(__reg("a6") struct Library *, __reg("a0") X509_PKEY * a, __reg("a1") unsigned char ** pp)="\tjsr\t-10866(a6)";
#define i2d_X509_PKEY(a, pp) __i2d_X509_PKEY(AmiSSLBase, (a), (pp))

X509_PKEY * __d2i_X509_PKEY(__reg("a6") struct Library *, __reg("a0") X509_PKEY ** a, __reg("a1") unsigned char ** pp, __reg("d0") long length)="\tjsr\t-10872(a6)";
#define d2i_X509_PKEY(a, pp, length) __d2i_X509_PKEY(AmiSSLBase, (a), (pp), (length))

NETSCAPE_SPKI * __NETSCAPE_SPKI_new(__reg("a6") struct Library *)="\tjsr\t-10878(a6)";
#define NETSCAPE_SPKI_new() __NETSCAPE_SPKI_new(AmiSSLBase)

void __NETSCAPE_SPKI_free(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * a)="\tjsr\t-10884(a6)";
#define NETSCAPE_SPKI_free(a) __NETSCAPE_SPKI_free(AmiSSLBase, (a))

NETSCAPE_SPKI * __d2i_NETSCAPE_SPKI(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10890(a6)";
#define d2i_NETSCAPE_SPKI(a, in, len) __d2i_NETSCAPE_SPKI(AmiSSLBase, (a), (in), (len))

int __i2d_NETSCAPE_SPKI(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKI * a, __reg("a1") unsigned char ** out)="\tjsr\t-10896(a6)";
#define i2d_NETSCAPE_SPKI(a, out) __i2d_NETSCAPE_SPKI(AmiSSLBase, (a), (out))

const ASN1_ITEM * __NETSCAPE_SPKI_it(__reg("a6") struct Library *)="\tjsr\t-10902(a6)";
#define NETSCAPE_SPKI_it() __NETSCAPE_SPKI_it(AmiSSLBase)

NETSCAPE_SPKAC * __NETSCAPE_SPKAC_new(__reg("a6") struct Library *)="\tjsr\t-10908(a6)";
#define NETSCAPE_SPKAC_new() __NETSCAPE_SPKAC_new(AmiSSLBase)

void __NETSCAPE_SPKAC_free(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKAC * a)="\tjsr\t-10914(a6)";
#define NETSCAPE_SPKAC_free(a) __NETSCAPE_SPKAC_free(AmiSSLBase, (a))

NETSCAPE_SPKAC * __d2i_NETSCAPE_SPKAC(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKAC ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10920(a6)";
#define d2i_NETSCAPE_SPKAC(a, in, len) __d2i_NETSCAPE_SPKAC(AmiSSLBase, (a), (in), (len))

int __i2d_NETSCAPE_SPKAC(__reg("a6") struct Library *, __reg("a0") NETSCAPE_SPKAC * a, __reg("a1") unsigned char ** out)="\tjsr\t-10926(a6)";
#define i2d_NETSCAPE_SPKAC(a, out) __i2d_NETSCAPE_SPKAC(AmiSSLBase, (a), (out))

const ASN1_ITEM * __NETSCAPE_SPKAC_it(__reg("a6") struct Library *)="\tjsr\t-10932(a6)";
#define NETSCAPE_SPKAC_it() __NETSCAPE_SPKAC_it(AmiSSLBase)

NETSCAPE_CERT_SEQUENCE * __NETSCAPE_CERT_SEQUENCE_new(__reg("a6") struct Library *)="\tjsr\t-10938(a6)";
#define NETSCAPE_CERT_SEQUENCE_new() __NETSCAPE_CERT_SEQUENCE_new(AmiSSLBase)

void __NETSCAPE_CERT_SEQUENCE_free(__reg("a6") struct Library *, __reg("a0") NETSCAPE_CERT_SEQUENCE * a)="\tjsr\t-10944(a6)";
#define NETSCAPE_CERT_SEQUENCE_free(a) __NETSCAPE_CERT_SEQUENCE_free(AmiSSLBase, (a))

NETSCAPE_CERT_SEQUENCE * __d2i_NETSCAPE_CERT_SEQUENCE(__reg("a6") struct Library *, __reg("a0") NETSCAPE_CERT_SEQUENCE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-10950(a6)";
#define d2i_NETSCAPE_CERT_SEQUENCE(a, in, len) __d2i_NETSCAPE_CERT_SEQUENCE(AmiSSLBase, (a), (in), (len))

int __i2d_NETSCAPE_CERT_SEQUENCE(__reg("a6") struct Library *, __reg("a0") NETSCAPE_CERT_SEQUENCE * a, __reg("a1") unsigned char ** out)="\tjsr\t-10956(a6)";
#define i2d_NETSCAPE_CERT_SEQUENCE(a, out) __i2d_NETSCAPE_CERT_SEQUENCE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __NETSCAPE_CERT_SEQUENCE_it(__reg("a6") struct Library *)="\tjsr\t-10962(a6)";
#define NETSCAPE_CERT_SEQUENCE_it() __NETSCAPE_CERT_SEQUENCE_it(AmiSSLBase)

X509_INFO * __X509_INFO_new(__reg("a6") struct Library *)="\tjsr\t-10968(a6)";
#define X509_INFO_new() __X509_INFO_new(AmiSSLBase)

void __X509_INFO_free(__reg("a6") struct Library *, __reg("a0") X509_INFO * a)="\tjsr\t-10974(a6)";
#define X509_INFO_free(a) __X509_INFO_free(AmiSSLBase, (a))

char * __X509_NAME_oneline(__reg("a6") struct Library *, __reg("a0") X509_NAME * a, __reg("a1") char * buf, __reg("d0") LONG size)="\tjsr\t-10980(a6)";
#define X509_NAME_oneline(a, buf, size) __X509_NAME_oneline(AmiSSLBase, (a), (buf), (size))

int __ASN1_verify(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") X509_ALGOR * algor1, __reg("a2") ASN1_BIT_STRING * signature, __reg("a3") char * data, __reg("d0") EVP_PKEY * pkey)="\tjsr\t-10986(a6)";
#define ASN1_verify(i2d, algor1, signature, data, pkey) __ASN1_verify(AmiSSLBase, (i2d), (algor1), (signature), (data), (pkey))

int __ASN1_digest(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") const EVP_MD * type, __reg("a2") char * data, __reg("a3") unsigned char * md, __reg("d0") unsigned int * len)="\tjsr\t-10992(a6)";
#define ASN1_digest(i2d, type, data, md, len) __ASN1_digest(AmiSSLBase, (i2d), (type), (data), (md), (len))

int __ASN1_sign(__reg("a6") struct Library *, __reg("a0") int (*i2d)(), __reg("a1") X509_ALGOR * algor1, __reg("a2") X509_ALGOR * algor2, __reg("a3") ASN1_BIT_STRING * signature, __reg("d0") char * data, __reg("d1") EVP_PKEY * pkey, __reg("d2") const EVP_MD * type)="\tjsr\t-10998(a6)";
#define ASN1_sign(i2d, algor1, algor2, signature, data, pkey, type) __ASN1_sign(AmiSSLBase, (i2d), (algor1), (algor2), (signature), (data), (pkey), (type))

int __ASN1_item_digest(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") const EVP_MD * type, __reg("a2") void * data, __reg("a3") unsigned char * md, __reg("d0") unsigned int * len)="\tjsr\t-11004(a6)";
#define ASN1_item_digest(it, type, data, md, len) __ASN1_item_digest(AmiSSLBase, (it), (type), (data), (md), (len))

int __ASN1_item_verify(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") X509_ALGOR * algor1, __reg("a2") ASN1_BIT_STRING * signature, __reg("a3") void * data, __reg("d0") EVP_PKEY * pkey)="\tjsr\t-11010(a6)";
#define ASN1_item_verify(it, algor1, signature, data, pkey) __ASN1_item_verify(AmiSSLBase, (it), (algor1), (signature), (data), (pkey))

int __ASN1_item_sign(__reg("a6") struct Library *, __reg("a0") const ASN1_ITEM * it, __reg("a1") X509_ALGOR * algor1, __reg("a2") X509_ALGOR * algor2, __reg("a3") ASN1_BIT_STRING * signature, __reg("d0") void * data, __reg("d1") EVP_PKEY * pkey, __reg("d2") const EVP_MD * type)="\tjsr\t-11016(a6)";
#define ASN1_item_sign(it, algor1, algor2, signature, data, pkey, type) __ASN1_item_sign(AmiSSLBase, (it), (algor1), (algor2), (signature), (data), (pkey), (type))

int __X509_set_version(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") long version)="\tjsr\t-11022(a6)";
#define X509_set_version(x, version) __X509_set_version(AmiSSLBase, (x), (version))

int __X509_set_serialNumber(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_INTEGER * serial)="\tjsr\t-11028(a6)";
#define X509_set_serialNumber(x, serial) __X509_set_serialNumber(AmiSSLBase, (x), (serial))

ASN1_INTEGER * __X509_get_serialNumber(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-11034(a6)";
#define X509_get_serialNumber(x) __X509_get_serialNumber(AmiSSLBase, (x))

int __X509_set_issuer_name(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") X509_NAME * name)="\tjsr\t-11040(a6)";
#define X509_set_issuer_name(x, name) __X509_set_issuer_name(AmiSSLBase, (x), (name))

X509_NAME * __X509_get_issuer_name(__reg("a6") struct Library *, __reg("a0") X509 * a)="\tjsr\t-11046(a6)";
#define X509_get_issuer_name(a) __X509_get_issuer_name(AmiSSLBase, (a))

int __X509_set_subject_name(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") X509_NAME * name)="\tjsr\t-11052(a6)";
#define X509_set_subject_name(x, name) __X509_set_subject_name(AmiSSLBase, (x), (name))

X509_NAME * __X509_get_subject_name(__reg("a6") struct Library *, __reg("a0") X509 * a)="\tjsr\t-11058(a6)";
#define X509_get_subject_name(a) __X509_get_subject_name(AmiSSLBase, (a))

int __X509_set_notBefore(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_TIME * tm)="\tjsr\t-11064(a6)";
#define X509_set_notBefore(x, tm) __X509_set_notBefore(AmiSSLBase, (x), (tm))

int __X509_set_notAfter(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_TIME * tm)="\tjsr\t-11070(a6)";
#define X509_set_notAfter(x, tm) __X509_set_notAfter(AmiSSLBase, (x), (tm))

int __X509_set_pubkey(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-11076(a6)";
#define X509_set_pubkey(x, pkey) __X509_set_pubkey(AmiSSLBase, (x), (pkey))

EVP_PKEY * __X509_get_pubkey(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-11082(a6)";
#define X509_get_pubkey(x) __X509_get_pubkey(AmiSSLBase, (x))

ASN1_BIT_STRING * __X509_get0_pubkey_bitstr(__reg("a6") struct Library *, __reg("a0") const X509 * x)="\tjsr\t-11088(a6)";
#define X509_get0_pubkey_bitstr(x) __X509_get0_pubkey_bitstr(AmiSSLBase, (x))

int __X509_certificate_type(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") EVP_PKEY * pubkey)="\tjsr\t-11094(a6)";
#define X509_certificate_type(x, pubkey) __X509_certificate_type(AmiSSLBase, (x), (pubkey))

int __X509_REQ_set_version(__reg("a6") struct Library *, __reg("a0") X509_REQ * x, __reg("d0") long version)="\tjsr\t-11100(a6)";
#define X509_REQ_set_version(x, version) __X509_REQ_set_version(AmiSSLBase, (x), (version))

int __X509_REQ_set_subject_name(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") X509_NAME * name)="\tjsr\t-11106(a6)";
#define X509_REQ_set_subject_name(req, name) __X509_REQ_set_subject_name(AmiSSLBase, (req), (name))

int __X509_REQ_set_pubkey(__reg("a6") struct Library *, __reg("a0") X509_REQ * x, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-11112(a6)";
#define X509_REQ_set_pubkey(x, pkey) __X509_REQ_set_pubkey(AmiSSLBase, (x), (pkey))

EVP_PKEY * __X509_REQ_get_pubkey(__reg("a6") struct Library *, __reg("a0") X509_REQ * req)="\tjsr\t-11118(a6)";
#define X509_REQ_get_pubkey(req) __X509_REQ_get_pubkey(AmiSSLBase, (req))

int __X509_REQ_extension_nid(__reg("a6") struct Library *, __reg("d0") LONG nid)="\tjsr\t-11124(a6)";
#define X509_REQ_extension_nid(nid) __X509_REQ_extension_nid(AmiSSLBase, (nid))

int * __X509_REQ_get_extension_nids(__reg("a6") struct Library *)="\tjsr\t-11130(a6)";
#define X509_REQ_get_extension_nids() __X509_REQ_get_extension_nids(AmiSSLBase)

void __X509_REQ_set_extension_nids(__reg("a6") struct Library *, __reg("a0") int * nids)="\tjsr\t-11136(a6)";
#define X509_REQ_set_extension_nids(nids) __X509_REQ_set_extension_nids(AmiSSLBase, (nids))

void * __X509_REQ_get_extensions(__reg("a6") struct Library *, __reg("a0") X509_REQ * req)="\tjsr\t-11142(a6)";
#define X509_REQ_get_extensions(req) __X509_REQ_get_extensions(AmiSSLBase, (req))

int __X509_REQ_add_extensions_nid(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") void * exts, __reg("d0") LONG nid)="\tjsr\t-11148(a6)";
#define X509_REQ_add_extensions_nid(req, exts, nid) __X509_REQ_add_extensions_nid(AmiSSLBase, (req), (exts), (nid))

int __X509_REQ_add_extensions(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") void * exts)="\tjsr\t-11154(a6)";
#define X509_REQ_add_extensions(req, exts) __X509_REQ_add_extensions(AmiSSLBase, (req), (exts))

int __X509_REQ_get_attr_count(__reg("a6") struct Library *, __reg("a0") const X509_REQ * req)="\tjsr\t-11160(a6)";
#define X509_REQ_get_attr_count(req) __X509_REQ_get_attr_count(AmiSSLBase, (req))

int __X509_REQ_get_attr_by_NID(__reg("a6") struct Library *, __reg("a0") const X509_REQ * req, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11166(a6)";
#define X509_REQ_get_attr_by_NID(req, nid, lastpos) __X509_REQ_get_attr_by_NID(AmiSSLBase, (req), (nid), (lastpos))

int __X509_REQ_get_attr_by_OBJ(__reg("a6") struct Library *, __reg("a0") const X509_REQ * req, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11172(a6)";
#define X509_REQ_get_attr_by_OBJ(req, obj, lastpos) __X509_REQ_get_attr_by_OBJ(AmiSSLBase, (req), (obj), (lastpos))

X509_ATTRIBUTE * __X509_REQ_get_attr(__reg("a6") struct Library *, __reg("a0") const X509_REQ * req, __reg("d0") LONG loc)="\tjsr\t-11178(a6)";
#define X509_REQ_get_attr(req, loc) __X509_REQ_get_attr(AmiSSLBase, (req), (loc))

X509_ATTRIBUTE * __X509_REQ_delete_attr(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("d0") LONG loc)="\tjsr\t-11184(a6)";
#define X509_REQ_delete_attr(req, loc) __X509_REQ_delete_attr(AmiSSLBase, (req), (loc))

int __X509_REQ_add1_attr(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") X509_ATTRIBUTE * attr)="\tjsr\t-11190(a6)";
#define X509_REQ_add1_attr(req, attr) __X509_REQ_add1_attr(AmiSSLBase, (req), (attr))

int __X509_REQ_add1_attr_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") const ASN1_OBJECT * obj, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11196(a6)";
#define X509_REQ_add1_attr_by_OBJ(req, obj, type, bytes, len) __X509_REQ_add1_attr_by_OBJ(AmiSSLBase, (req), (obj), (type), (bytes), (len))

int __X509_REQ_add1_attr_by_NID(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("d0") LONG nid, __reg("d1") LONG type, __reg("a1") const unsigned char * bytes, __reg("d2") LONG len)="\tjsr\t-11202(a6)";
#define X509_REQ_add1_attr_by_NID(req, nid, type, bytes, len) __X509_REQ_add1_attr_by_NID(AmiSSLBase, (req), (nid), (type), (bytes), (len))

int __X509_REQ_add1_attr_by_txt(__reg("a6") struct Library *, __reg("a0") X509_REQ * req, __reg("a1") const char * attrname, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11208(a6)";
#define X509_REQ_add1_attr_by_txt(req, attrname, type, bytes, len) __X509_REQ_add1_attr_by_txt(AmiSSLBase, (req), (attrname), (type), (bytes), (len))

int __X509_CRL_set_version(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") long version)="\tjsr\t-11214(a6)";
#define X509_CRL_set_version(x, version) __X509_CRL_set_version(AmiSSLBase, (x), (version))

int __X509_CRL_set_issuer_name(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") X509_NAME * name)="\tjsr\t-11220(a6)";
#define X509_CRL_set_issuer_name(x, name) __X509_CRL_set_issuer_name(AmiSSLBase, (x), (name))

int __X509_CRL_set_lastUpdate(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") ASN1_TIME * tm)="\tjsr\t-11226(a6)";
#define X509_CRL_set_lastUpdate(x, tm) __X509_CRL_set_lastUpdate(AmiSSLBase, (x), (tm))

int __X509_CRL_set_nextUpdate(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") ASN1_TIME * tm)="\tjsr\t-11232(a6)";
#define X509_CRL_set_nextUpdate(x, tm) __X509_CRL_set_nextUpdate(AmiSSLBase, (x), (tm))

int __X509_CRL_sort(__reg("a6") struct Library *, __reg("a0") X509_CRL * crl)="\tjsr\t-11238(a6)";
#define X509_CRL_sort(crl) __X509_CRL_sort(AmiSSLBase, (crl))

int __X509_REVOKED_set_serialNumber(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("a1") ASN1_INTEGER * serial)="\tjsr\t-11244(a6)";
#define X509_REVOKED_set_serialNumber(x, serial) __X509_REVOKED_set_serialNumber(AmiSSLBase, (x), (serial))

int __X509_REVOKED_set_revocationDate(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * r, __reg("a1") ASN1_TIME * tm)="\tjsr\t-11250(a6)";
#define X509_REVOKED_set_revocationDate(r, tm) __X509_REVOKED_set_revocationDate(AmiSSLBase, (r), (tm))

int __X509_check_private_key(__reg("a6") struct Library *, __reg("a0") X509 * x509, __reg("a1") EVP_PKEY * pkey)="\tjsr\t-11256(a6)";
#define X509_check_private_key(x509, pkey) __X509_check_private_key(AmiSSLBase, (x509), (pkey))

int __X509_issuer_and_serial_cmp(__reg("a6") struct Library *, __reg("a0") const X509 * a, __reg("a1") const X509 * b)="\tjsr\t-11262(a6)";
#define X509_issuer_and_serial_cmp(a, b) __X509_issuer_and_serial_cmp(AmiSSLBase, (a), (b))

unsigned long __X509_issuer_and_serial_hash(__reg("a6") struct Library *, __reg("a0") X509 * a)="\tjsr\t-11268(a6)";
#define X509_issuer_and_serial_hash(a) __X509_issuer_and_serial_hash(AmiSSLBase, (a))

int __X509_issuer_name_cmp(__reg("a6") struct Library *, __reg("a0") const X509 * a, __reg("a1") const X509 * b)="\tjsr\t-11274(a6)";
#define X509_issuer_name_cmp(a, b) __X509_issuer_name_cmp(AmiSSLBase, (a), (b))

unsigned long __X509_issuer_name_hash(__reg("a6") struct Library *, __reg("a0") X509 * a)="\tjsr\t-11280(a6)";
#define X509_issuer_name_hash(a) __X509_issuer_name_hash(AmiSSLBase, (a))

int __X509_subject_name_cmp(__reg("a6") struct Library *, __reg("a0") const X509 * a, __reg("a1") const X509 * b)="\tjsr\t-11286(a6)";
#define X509_subject_name_cmp(a, b) __X509_subject_name_cmp(AmiSSLBase, (a), (b))

unsigned long __X509_subject_name_hash(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-11292(a6)";
#define X509_subject_name_hash(x) __X509_subject_name_hash(AmiSSLBase, (x))

int __X509_cmp(__reg("a6") struct Library *, __reg("a0") const X509 * a, __reg("a1") const X509 * b)="\tjsr\t-11298(a6)";
#define X509_cmp(a, b) __X509_cmp(AmiSSLBase, (a), (b))

int __X509_NAME_cmp(__reg("a6") struct Library *, __reg("a0") const X509_NAME * a, __reg("a1") const X509_NAME * b)="\tjsr\t-11304(a6)";
#define X509_NAME_cmp(a, b) __X509_NAME_cmp(AmiSSLBase, (a), (b))

unsigned long __X509_NAME_hash(__reg("a6") struct Library *, __reg("a0") X509_NAME * x)="\tjsr\t-11310(a6)";
#define X509_NAME_hash(x) __X509_NAME_hash(AmiSSLBase, (x))

int __X509_CRL_cmp(__reg("a6") struct Library *, __reg("a0") const X509_CRL * a, __reg("a1") const X509_CRL * b)="\tjsr\t-11316(a6)";
#define X509_CRL_cmp(a, b) __X509_CRL_cmp(AmiSSLBase, (a), (b))

int __X509_NAME_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_NAME * name, __reg("d0") LONG obase)="\tjsr\t-11322(a6)";
#define X509_NAME_print(bp, name, obase) __X509_NAME_print(AmiSSLBase, (bp), (name), (obase))

int __X509_NAME_print_ex(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") X509_NAME * nm, __reg("d0") LONG indent, __reg("d1") unsigned long flags)="\tjsr\t-11328(a6)";
#define X509_NAME_print_ex(out, nm, indent, flags) __X509_NAME_print_ex(AmiSSLBase, (out), (nm), (indent), (flags))

int __X509_print_ex(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x, __reg("d0") unsigned long nmflag, __reg("d1") unsigned long cflag)="\tjsr\t-11334(a6)";
#define X509_print_ex(bp, x, nmflag, cflag) __X509_print_ex(AmiSSLBase, (bp), (x), (nmflag), (cflag))

int __X509_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x)="\tjsr\t-11340(a6)";
#define X509_print(bp, x) __X509_print(AmiSSLBase, (bp), (x))

int __X509_ocspid_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509 * x)="\tjsr\t-11346(a6)";
#define X509_ocspid_print(bp, x) __X509_ocspid_print(AmiSSLBase, (bp), (x))

int __X509_CERT_AUX_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CERT_AUX * x, __reg("d0") LONG indent)="\tjsr\t-11352(a6)";
#define X509_CERT_AUX_print(bp, x, indent) __X509_CERT_AUX_print(AmiSSLBase, (bp), (x), (indent))

int __X509_CRL_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_CRL * x)="\tjsr\t-11358(a6)";
#define X509_CRL_print(bp, x) __X509_CRL_print(AmiSSLBase, (bp), (x))

int __X509_REQ_print_ex(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ * x, __reg("d0") unsigned long nmflag, __reg("d1") unsigned long cflag)="\tjsr\t-11364(a6)";
#define X509_REQ_print_ex(bp, x, nmflag, cflag) __X509_REQ_print_ex(AmiSSLBase, (bp), (x), (nmflag), (cflag))

int __X509_REQ_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") X509_REQ * req)="\tjsr\t-11370(a6)";
#define X509_REQ_print(bp, req) __X509_REQ_print(AmiSSLBase, (bp), (req))

int __X509_NAME_entry_count(__reg("a6") struct Library *, __reg("a0") X509_NAME * name)="\tjsr\t-11376(a6)";
#define X509_NAME_entry_count(name) __X509_NAME_entry_count(AmiSSLBase, (name))

int __X509_NAME_get_text_by_NID(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("d0") LONG nid, __reg("a1") char * buf, __reg("d1") LONG len)="\tjsr\t-11382(a6)";
#define X509_NAME_get_text_by_NID(name, nid, buf, len) __X509_NAME_get_text_by_NID(AmiSSLBase, (name), (nid), (buf), (len))

int __X509_NAME_get_text_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("a1") ASN1_OBJECT * obj, __reg("a2") char * buf, __reg("d0") LONG len)="\tjsr\t-11388(a6)";
#define X509_NAME_get_text_by_OBJ(name, obj, buf, len) __X509_NAME_get_text_by_OBJ(AmiSSLBase, (name), (obj), (buf), (len))

int __X509_NAME_get_index_by_NID(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11394(a6)";
#define X509_NAME_get_index_by_NID(name, nid, lastpos) __X509_NAME_get_index_by_NID(AmiSSLBase, (name), (nid), (lastpos))

int __X509_NAME_get_index_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11400(a6)";
#define X509_NAME_get_index_by_OBJ(name, obj, lastpos) __X509_NAME_get_index_by_OBJ(AmiSSLBase, (name), (obj), (lastpos))

X509_NAME_ENTRY * __X509_NAME_get_entry(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("d0") LONG loc)="\tjsr\t-11406(a6)";
#define X509_NAME_get_entry(name, loc) __X509_NAME_get_entry(AmiSSLBase, (name), (loc))

X509_NAME_ENTRY * __X509_NAME_delete_entry(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("d0") LONG loc)="\tjsr\t-11412(a6)";
#define X509_NAME_delete_entry(name, loc) __X509_NAME_delete_entry(AmiSSLBase, (name), (loc))

int __X509_NAME_add_entry(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("a1") X509_NAME_ENTRY * ne, __reg("d0") LONG loc, __reg("d1") LONG set)="\tjsr\t-11418(a6)";
#define X509_NAME_add_entry(name, ne, loc, set) __X509_NAME_add_entry(AmiSSLBase, (name), (ne), (loc), (set))

int __X509_NAME_add_entry_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG type, __reg("a2") unsigned char * bytes, __reg("d1") LONG len, __reg("d2") LONG loc, __reg("d3") LONG set)="\tjsr\t-11424(a6)";
#define X509_NAME_add_entry_by_OBJ(name, obj, type, bytes, len, loc, set) __X509_NAME_add_entry_by_OBJ(AmiSSLBase, (name), (obj), (type), (bytes), (len), (loc), (set))

int __X509_NAME_add_entry_by_NID(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("d0") LONG nid, __reg("d1") LONG type, __reg("a1") unsigned char * bytes, __reg("d2") LONG len, __reg("d3") LONG loc, __reg("d4") LONG set)="\tjsr\t-11430(a6)";
#define X509_NAME_add_entry_by_NID(name, nid, type, bytes, len, loc, set) __X509_NAME_add_entry_by_NID(AmiSSLBase, (name), (nid), (type), (bytes), (len), (loc), (set))

X509_NAME_ENTRY * __X509_NAME_ENTRY_create_by_txt(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY ** ne, __reg("a1") const char * field, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11436(a6)";
#define X509_NAME_ENTRY_create_by_txt(ne, field, type, bytes, len) __X509_NAME_ENTRY_create_by_txt(AmiSSLBase, (ne), (field), (type), (bytes), (len))

X509_NAME_ENTRY * __X509_NAME_ENTRY_create_by_NID(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY ** ne, __reg("d0") LONG nid, __reg("d1") LONG type, __reg("a1") unsigned char * bytes, __reg("d2") LONG len)="\tjsr\t-11442(a6)";
#define X509_NAME_ENTRY_create_by_NID(ne, nid, type, bytes, len) __X509_NAME_ENTRY_create_by_NID(AmiSSLBase, (ne), (nid), (type), (bytes), (len))

int __X509_NAME_add_entry_by_txt(__reg("a6") struct Library *, __reg("a0") X509_NAME * name, __reg("a1") const char * field, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len, __reg("d2") LONG loc, __reg("d3") LONG set)="\tjsr\t-11448(a6)";
#define X509_NAME_add_entry_by_txt(name, field, type, bytes, len, loc, set) __X509_NAME_add_entry_by_txt(AmiSSLBase, (name), (field), (type), (bytes), (len), (loc), (set))

X509_NAME_ENTRY * __X509_NAME_ENTRY_create_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY ** ne, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11454(a6)";
#define X509_NAME_ENTRY_create_by_OBJ(ne, obj, type, bytes, len) __X509_NAME_ENTRY_create_by_OBJ(AmiSSLBase, (ne), (obj), (type), (bytes), (len))

int __X509_NAME_ENTRY_set_object(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * ne, __reg("a1") ASN1_OBJECT * obj)="\tjsr\t-11460(a6)";
#define X509_NAME_ENTRY_set_object(ne, obj) __X509_NAME_ENTRY_set_object(AmiSSLBase, (ne), (obj))

int __X509_NAME_ENTRY_set_data(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * ne, __reg("d0") LONG type, __reg("a1") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11466(a6)";
#define X509_NAME_ENTRY_set_data(ne, type, bytes, len) __X509_NAME_ENTRY_set_data(AmiSSLBase, (ne), (type), (bytes), (len))

ASN1_OBJECT * __X509_NAME_ENTRY_get_object(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * ne)="\tjsr\t-11472(a6)";
#define X509_NAME_ENTRY_get_object(ne) __X509_NAME_ENTRY_get_object(AmiSSLBase, (ne))

ASN1_STRING * __X509_NAME_ENTRY_get_data(__reg("a6") struct Library *, __reg("a0") X509_NAME_ENTRY * ne)="\tjsr\t-11478(a6)";
#define X509_NAME_ENTRY_get_data(ne) __X509_NAME_ENTRY_get_data(AmiSSLBase, (ne))

int __X509v3_get_ext_count(__reg("a6") struct Library *, __reg("a0") const void * x)="\tjsr\t-11484(a6)";
#define X509v3_get_ext_count(x) __X509v3_get_ext_count(AmiSSLBase, (x))

int __X509v3_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11490(a6)";
#define X509v3_get_ext_by_NID(x, nid, lastpos) __X509v3_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __X509v3_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11496(a6)";
#define X509v3_get_ext_by_OBJ(x, obj, lastpos) __X509v3_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __X509v3_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-11502(a6)";
#define X509v3_get_ext_by_critical(x, crit, lastpos) __X509v3_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __X509v3_get_ext(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("d0") LONG loc)="\tjsr\t-11508(a6)";
#define X509v3_get_ext(x, loc) __X509v3_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __X509v3_delete_ext(__reg("a6") struct Library *, __reg("a0") void * x, __reg("d0") LONG loc)="\tjsr\t-11514(a6)";
#define X509v3_delete_ext(x, loc) __X509v3_delete_ext(AmiSSLBase, (x), (loc))

void * __X509v3_add_ext(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-11520(a6)";
#define X509v3_add_ext(x, ex, loc) __X509v3_add_ext(AmiSSLBase, (x), (ex), (loc))

int __X509_get_ext_count(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-11526(a6)";
#define X509_get_ext_count(x) __X509_get_ext_count(AmiSSLBase, (x))

int __X509_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11532(a6)";
#define X509_get_ext_by_NID(x, nid, lastpos) __X509_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __X509_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11538(a6)";
#define X509_get_ext_by_OBJ(x, obj, lastpos) __X509_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __X509_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-11544(a6)";
#define X509_get_ext_by_critical(x, crit, lastpos) __X509_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __X509_get_ext(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG loc)="\tjsr\t-11550(a6)";
#define X509_get_ext(x, loc) __X509_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __X509_delete_ext(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG loc)="\tjsr\t-11556(a6)";
#define X509_delete_ext(x, loc) __X509_delete_ext(AmiSSLBase, (x), (loc))

int __X509_add_ext(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-11562(a6)";
#define X509_add_ext(x, ex, loc) __X509_add_ext(AmiSSLBase, (x), (ex), (loc))

void * __X509_get_ext_d2i(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-11568(a6)";
#define X509_get_ext_d2i(x, nid, crit, idx) __X509_get_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __X509_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-11574(a6)";
#define X509_add1_ext_i2d(x, nid, value, crit, flags) __X509_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __X509_CRL_get_ext_count(__reg("a6") struct Library *, __reg("a0") X509_CRL * x)="\tjsr\t-11580(a6)";
#define X509_CRL_get_ext_count(x) __X509_CRL_get_ext_count(AmiSSLBase, (x))

int __X509_CRL_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11586(a6)";
#define X509_CRL_get_ext_by_NID(x, nid, lastpos) __X509_CRL_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __X509_CRL_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11592(a6)";
#define X509_CRL_get_ext_by_OBJ(x, obj, lastpos) __X509_CRL_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __X509_CRL_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-11598(a6)";
#define X509_CRL_get_ext_by_critical(x, crit, lastpos) __X509_CRL_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __X509_CRL_get_ext(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG loc)="\tjsr\t-11604(a6)";
#define X509_CRL_get_ext(x, loc) __X509_CRL_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __X509_CRL_delete_ext(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG loc)="\tjsr\t-11610(a6)";
#define X509_CRL_delete_ext(x, loc) __X509_CRL_delete_ext(AmiSSLBase, (x), (loc))

int __X509_CRL_add_ext(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-11616(a6)";
#define X509_CRL_add_ext(x, ex, loc) __X509_CRL_add_ext(AmiSSLBase, (x), (ex), (loc))

void * __X509_CRL_get_ext_d2i(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-11622(a6)";
#define X509_CRL_get_ext_d2i(x, nid, crit, idx) __X509_CRL_get_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __X509_CRL_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") X509_CRL * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-11628(a6)";
#define X509_CRL_add1_ext_i2d(x, nid, value, crit, flags) __X509_CRL_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

int __X509_REVOKED_get_ext_count(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x)="\tjsr\t-11634(a6)";
#define X509_REVOKED_get_ext_count(x) __X509_REVOKED_get_ext_count(AmiSSLBase, (x))

int __X509_REVOKED_get_ext_by_NID(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11640(a6)";
#define X509_REVOKED_get_ext_by_NID(x, nid, lastpos) __X509_REVOKED_get_ext_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __X509_REVOKED_get_ext_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11646(a6)";
#define X509_REVOKED_get_ext_by_OBJ(x, obj, lastpos) __X509_REVOKED_get_ext_by_OBJ(AmiSSLBase, (x), (obj), (lastpos))

int __X509_REVOKED_get_ext_by_critical(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG crit, __reg("d1") LONG lastpos)="\tjsr\t-11652(a6)";
#define X509_REVOKED_get_ext_by_critical(x, crit, lastpos) __X509_REVOKED_get_ext_by_critical(AmiSSLBase, (x), (crit), (lastpos))

X509_EXTENSION * __X509_REVOKED_get_ext(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG loc)="\tjsr\t-11658(a6)";
#define X509_REVOKED_get_ext(x, loc) __X509_REVOKED_get_ext(AmiSSLBase, (x), (loc))

X509_EXTENSION * __X509_REVOKED_delete_ext(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG loc)="\tjsr\t-11664(a6)";
#define X509_REVOKED_delete_ext(x, loc) __X509_REVOKED_delete_ext(AmiSSLBase, (x), (loc))

int __X509_REVOKED_add_ext(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("a1") X509_EXTENSION * ex, __reg("d0") LONG loc)="\tjsr\t-11670(a6)";
#define X509_REVOKED_add_ext(x, ex, loc) __X509_REVOKED_add_ext(AmiSSLBase, (x), (ex), (loc))

void * __X509_REVOKED_get_ext_d2i(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-11676(a6)";
#define X509_REVOKED_get_ext_d2i(x, nid, crit, idx) __X509_REVOKED_get_ext_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

int __X509_REVOKED_add1_ext_i2d(__reg("a6") struct Library *, __reg("a0") X509_REVOKED * x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-11682(a6)";
#define X509_REVOKED_add1_ext_i2d(x, nid, value, crit, flags) __X509_REVOKED_add1_ext_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

X509_EXTENSION * __X509_EXTENSION_create_by_NID(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION ** ex, __reg("d0") LONG nid, __reg("d1") LONG crit, __reg("a1") ASN1_OCTET_STRING * data)="\tjsr\t-11688(a6)";
#define X509_EXTENSION_create_by_NID(ex, nid, crit, data) __X509_EXTENSION_create_by_NID(AmiSSLBase, (ex), (nid), (crit), (data))

X509_EXTENSION * __X509_EXTENSION_create_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION ** ex, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG crit, __reg("a2") ASN1_OCTET_STRING * data)="\tjsr\t-11694(a6)";
#define X509_EXTENSION_create_by_OBJ(ex, obj, crit, data) __X509_EXTENSION_create_by_OBJ(AmiSSLBase, (ex), (obj), (crit), (data))

int __X509_EXTENSION_set_object(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex, __reg("a1") ASN1_OBJECT * obj)="\tjsr\t-11700(a6)";
#define X509_EXTENSION_set_object(ex, obj) __X509_EXTENSION_set_object(AmiSSLBase, (ex), (obj))

int __X509_EXTENSION_set_critical(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex, __reg("d0") LONG crit)="\tjsr\t-11706(a6)";
#define X509_EXTENSION_set_critical(ex, crit) __X509_EXTENSION_set_critical(AmiSSLBase, (ex), (crit))

int __X509_EXTENSION_set_data(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex, __reg("a1") ASN1_OCTET_STRING * data)="\tjsr\t-11712(a6)";
#define X509_EXTENSION_set_data(ex, data) __X509_EXTENSION_set_data(AmiSSLBase, (ex), (data))

ASN1_OBJECT * __X509_EXTENSION_get_object(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex)="\tjsr\t-11718(a6)";
#define X509_EXTENSION_get_object(ex) __X509_EXTENSION_get_object(AmiSSLBase, (ex))

ASN1_OCTET_STRING * __X509_EXTENSION_get_data(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ne)="\tjsr\t-11724(a6)";
#define X509_EXTENSION_get_data(ne) __X509_EXTENSION_get_data(AmiSSLBase, (ne))

int __X509_EXTENSION_get_critical(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex)="\tjsr\t-11730(a6)";
#define X509_EXTENSION_get_critical(ex) __X509_EXTENSION_get_critical(AmiSSLBase, (ex))

int __X509at_get_attr_count(__reg("a6") struct Library *, __reg("a0") const void * x)="\tjsr\t-11736(a6)";
#define X509at_get_attr_count(x) __X509at_get_attr_count(AmiSSLBase, (x))

int __X509at_get_attr_by_NID(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("d0") LONG nid, __reg("d1") LONG lastpos)="\tjsr\t-11742(a6)";
#define X509at_get_attr_by_NID(x, nid, lastpos) __X509at_get_attr_by_NID(AmiSSLBase, (x), (nid), (lastpos))

int __X509at_get_attr_by_OBJ(__reg("a6") struct Library *, __reg("a0") const void * sk, __reg("a1") ASN1_OBJECT * obj, __reg("d0") LONG lastpos)="\tjsr\t-11748(a6)";
#define X509at_get_attr_by_OBJ(sk, obj, lastpos) __X509at_get_attr_by_OBJ(AmiSSLBase, (sk), (obj), (lastpos))

X509_ATTRIBUTE * __X509at_get_attr(__reg("a6") struct Library *, __reg("a0") const void * x, __reg("d0") LONG loc)="\tjsr\t-11754(a6)";
#define X509at_get_attr(x, loc) __X509at_get_attr(AmiSSLBase, (x), (loc))

X509_ATTRIBUTE * __X509at_delete_attr(__reg("a6") struct Library *, __reg("a0") void * x, __reg("d0") LONG loc)="\tjsr\t-11760(a6)";
#define X509at_delete_attr(x, loc) __X509at_delete_attr(AmiSSLBase, (x), (loc))

void * __X509at_add1_attr(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("a1") X509_ATTRIBUTE * attr)="\tjsr\t-11766(a6)";
#define X509at_add1_attr(x, attr) __X509at_add1_attr(AmiSSLBase, (x), (attr))

void * __X509at_add1_attr_by_OBJ(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("a1") const ASN1_OBJECT * obj, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11772(a6)";
#define X509at_add1_attr_by_OBJ(x, obj, type, bytes, len) __X509at_add1_attr_by_OBJ(AmiSSLBase, (x), (obj), (type), (bytes), (len))

void * __X509at_add1_attr_by_NID(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("d0") LONG nid, __reg("d1") LONG type, __reg("a1") const unsigned char * bytes, __reg("d2") LONG len)="\tjsr\t-11778(a6)";
#define X509at_add1_attr_by_NID(x, nid, type, bytes, len) __X509at_add1_attr_by_NID(AmiSSLBase, (x), (nid), (type), (bytes), (len))

void * __X509at_add1_attr_by_txt(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("a1") const char * attrname, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11784(a6)";
#define X509at_add1_attr_by_txt(x, attrname, type, bytes, len) __X509at_add1_attr_by_txt(AmiSSLBase, (x), (attrname), (type), (bytes), (len))

X509_ATTRIBUTE * __X509_ATTRIBUTE_create_by_NID(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE ** attr, __reg("d0") LONG nid, __reg("d1") LONG atrtype, __reg("a1") const void * data, __reg("d2") LONG len)="\tjsr\t-11790(a6)";
#define X509_ATTRIBUTE_create_by_NID(attr, nid, atrtype, data, len) __X509_ATTRIBUTE_create_by_NID(AmiSSLBase, (attr), (nid), (atrtype), (data), (len))

X509_ATTRIBUTE * __X509_ATTRIBUTE_create_by_OBJ(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE ** attr, __reg("a1") const ASN1_OBJECT * obj, __reg("d0") LONG atrtype, __reg("a2") const void * data, __reg("d1") LONG len)="\tjsr\t-11796(a6)";
#define X509_ATTRIBUTE_create_by_OBJ(attr, obj, atrtype, data, len) __X509_ATTRIBUTE_create_by_OBJ(AmiSSLBase, (attr), (obj), (atrtype), (data), (len))

X509_ATTRIBUTE * __X509_ATTRIBUTE_create_by_txt(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE ** attr, __reg("a1") const char * atrname, __reg("d0") LONG type, __reg("a2") const unsigned char * bytes, __reg("d1") LONG len)="\tjsr\t-11802(a6)";
#define X509_ATTRIBUTE_create_by_txt(attr, atrname, type, bytes, len) __X509_ATTRIBUTE_create_by_txt(AmiSSLBase, (attr), (atrname), (type), (bytes), (len))

int __X509_ATTRIBUTE_set1_object(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr, __reg("a1") const ASN1_OBJECT * obj)="\tjsr\t-11808(a6)";
#define X509_ATTRIBUTE_set1_object(attr, obj) __X509_ATTRIBUTE_set1_object(AmiSSLBase, (attr), (obj))

int __X509_ATTRIBUTE_set1_data(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr, __reg("d0") LONG attrtype, __reg("a1") const void * data, __reg("d1") LONG len)="\tjsr\t-11814(a6)";
#define X509_ATTRIBUTE_set1_data(attr, attrtype, data, len) __X509_ATTRIBUTE_set1_data(AmiSSLBase, (attr), (attrtype), (data), (len))

void * __X509_ATTRIBUTE_get0_data(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr, __reg("d0") LONG idx, __reg("d1") LONG atrtype, __reg("a1") void * data)="\tjsr\t-11820(a6)";
#define X509_ATTRIBUTE_get0_data(attr, idx, atrtype, data) __X509_ATTRIBUTE_get0_data(AmiSSLBase, (attr), (idx), (atrtype), (data))

int __X509_ATTRIBUTE_count(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr)="\tjsr\t-11826(a6)";
#define X509_ATTRIBUTE_count(attr) __X509_ATTRIBUTE_count(AmiSSLBase, (attr))

ASN1_OBJECT * __X509_ATTRIBUTE_get0_object(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr)="\tjsr\t-11832(a6)";
#define X509_ATTRIBUTE_get0_object(attr) __X509_ATTRIBUTE_get0_object(AmiSSLBase, (attr))

ASN1_TYPE * __X509_ATTRIBUTE_get0_type(__reg("a6") struct Library *, __reg("a0") X509_ATTRIBUTE * attr, __reg("d0") LONG idx)="\tjsr\t-11838(a6)";
#define X509_ATTRIBUTE_get0_type(attr, idx) __X509_ATTRIBUTE_get0_type(AmiSSLBase, (attr), (idx))

int __X509_verify_cert(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-11844(a6)";
#define X509_verify_cert(ctx) __X509_verify_cert(AmiSSLBase, (ctx))

X509 * __X509_find_by_issuer_and_serial(__reg("a6") struct Library *, __reg("a0") void * sk, __reg("a1") X509_NAME * name, __reg("a2") ASN1_INTEGER * serial)="\tjsr\t-11850(a6)";
#define X509_find_by_issuer_and_serial(sk, name, serial) __X509_find_by_issuer_and_serial(AmiSSLBase, (sk), (name), (serial))

X509 * __X509_find_by_subject(__reg("a6") struct Library *, __reg("a0") void * sk, __reg("a1") X509_NAME * name)="\tjsr\t-11856(a6)";
#define X509_find_by_subject(sk, name) __X509_find_by_subject(AmiSSLBase, (sk), (name))

PBEPARAM * __PBEPARAM_new(__reg("a6") struct Library *)="\tjsr\t-11862(a6)";
#define PBEPARAM_new() __PBEPARAM_new(AmiSSLBase)

void __PBEPARAM_free(__reg("a6") struct Library *, __reg("a0") PBEPARAM * a)="\tjsr\t-11868(a6)";
#define PBEPARAM_free(a) __PBEPARAM_free(AmiSSLBase, (a))

PBEPARAM * __d2i_PBEPARAM(__reg("a6") struct Library *, __reg("a0") PBEPARAM ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-11874(a6)";
#define d2i_PBEPARAM(a, in, len) __d2i_PBEPARAM(AmiSSLBase, (a), (in), (len))

int __i2d_PBEPARAM(__reg("a6") struct Library *, __reg("a0") PBEPARAM * a, __reg("a1") unsigned char ** out)="\tjsr\t-11880(a6)";
#define i2d_PBEPARAM(a, out) __i2d_PBEPARAM(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PBEPARAM_it(__reg("a6") struct Library *)="\tjsr\t-11886(a6)";
#define PBEPARAM_it() __PBEPARAM_it(AmiSSLBase)

PBE2PARAM * __PBE2PARAM_new(__reg("a6") struct Library *)="\tjsr\t-11892(a6)";
#define PBE2PARAM_new() __PBE2PARAM_new(AmiSSLBase)

void __PBE2PARAM_free(__reg("a6") struct Library *, __reg("a0") PBE2PARAM * a)="\tjsr\t-11898(a6)";
#define PBE2PARAM_free(a) __PBE2PARAM_free(AmiSSLBase, (a))

PBE2PARAM * __d2i_PBE2PARAM(__reg("a6") struct Library *, __reg("a0") PBE2PARAM ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-11904(a6)";
#define d2i_PBE2PARAM(a, in, len) __d2i_PBE2PARAM(AmiSSLBase, (a), (in), (len))

int __i2d_PBE2PARAM(__reg("a6") struct Library *, __reg("a0") PBE2PARAM * a, __reg("a1") unsigned char ** out)="\tjsr\t-11910(a6)";
#define i2d_PBE2PARAM(a, out) __i2d_PBE2PARAM(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PBE2PARAM_it(__reg("a6") struct Library *)="\tjsr\t-11916(a6)";
#define PBE2PARAM_it() __PBE2PARAM_it(AmiSSLBase)

PBKDF2PARAM * __PBKDF2PARAM_new(__reg("a6") struct Library *)="\tjsr\t-11922(a6)";
#define PBKDF2PARAM_new() __PBKDF2PARAM_new(AmiSSLBase)

void __PBKDF2PARAM_free(__reg("a6") struct Library *, __reg("a0") PBKDF2PARAM * a)="\tjsr\t-11928(a6)";
#define PBKDF2PARAM_free(a) __PBKDF2PARAM_free(AmiSSLBase, (a))

PBKDF2PARAM * __d2i_PBKDF2PARAM(__reg("a6") struct Library *, __reg("a0") PBKDF2PARAM ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-11934(a6)";
#define d2i_PBKDF2PARAM(a, in, len) __d2i_PBKDF2PARAM(AmiSSLBase, (a), (in), (len))

int __i2d_PBKDF2PARAM(__reg("a6") struct Library *, __reg("a0") PBKDF2PARAM * a, __reg("a1") unsigned char ** out)="\tjsr\t-11940(a6)";
#define i2d_PBKDF2PARAM(a, out) __i2d_PBKDF2PARAM(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PBKDF2PARAM_it(__reg("a6") struct Library *)="\tjsr\t-11946(a6)";
#define PBKDF2PARAM_it() __PBKDF2PARAM_it(AmiSSLBase)

X509_ALGOR * __PKCS5_pbe_set(__reg("a6") struct Library *, __reg("d0") LONG alg, __reg("d1") LONG iter, __reg("a0") unsigned char * salt, __reg("d2") LONG saltlen)="\tjsr\t-11952(a6)";
#define PKCS5_pbe_set(alg, iter, salt, saltlen) __PKCS5_pbe_set(AmiSSLBase, (alg), (iter), (salt), (saltlen))

X509_ALGOR * __PKCS5_pbe2_set(__reg("a6") struct Library *, __reg("a0") const EVP_CIPHER * cipher, __reg("d0") LONG iter, __reg("a1") unsigned char * salt, __reg("d1") LONG saltlen)="\tjsr\t-11958(a6)";
#define PKCS5_pbe2_set(cipher, iter, salt, saltlen) __PKCS5_pbe2_set(AmiSSLBase, (cipher), (iter), (salt), (saltlen))

PKCS8_PRIV_KEY_INFO * __PKCS8_PRIV_KEY_INFO_new(__reg("a6") struct Library *)="\tjsr\t-11964(a6)";
#define PKCS8_PRIV_KEY_INFO_new() __PKCS8_PRIV_KEY_INFO_new(AmiSSLBase)

void __PKCS8_PRIV_KEY_INFO_free(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * a)="\tjsr\t-11970(a6)";
#define PKCS8_PRIV_KEY_INFO_free(a) __PKCS8_PRIV_KEY_INFO_free(AmiSSLBase, (a))

PKCS8_PRIV_KEY_INFO * __d2i_PKCS8_PRIV_KEY_INFO(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-11976(a6)";
#define d2i_PKCS8_PRIV_KEY_INFO(a, in, len) __d2i_PKCS8_PRIV_KEY_INFO(AmiSSLBase, (a), (in), (len))

int __i2d_PKCS8_PRIV_KEY_INFO(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-11982(a6)";
#define i2d_PKCS8_PRIV_KEY_INFO(a, out) __i2d_PKCS8_PRIV_KEY_INFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKCS8_PRIV_KEY_INFO_it(__reg("a6") struct Library *)="\tjsr\t-11988(a6)";
#define PKCS8_PRIV_KEY_INFO_it() __PKCS8_PRIV_KEY_INFO_it(AmiSSLBase)

EVP_PKEY * __EVP_PKCS82PKEY(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * p8)="\tjsr\t-11994(a6)";
#define EVP_PKCS82PKEY(p8) __EVP_PKCS82PKEY(AmiSSLBase, (p8))

PKCS8_PRIV_KEY_INFO * __EVP_PKEY2PKCS8(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey)="\tjsr\t-12000(a6)";
#define EVP_PKEY2PKCS8(pkey) __EVP_PKEY2PKCS8(AmiSSLBase, (pkey))

PKCS8_PRIV_KEY_INFO * __EVP_PKEY2PKCS8_broken(__reg("a6") struct Library *, __reg("a0") EVP_PKEY * pkey, __reg("d0") LONG broken)="\tjsr\t-12006(a6)";
#define EVP_PKEY2PKCS8_broken(pkey, broken) __EVP_PKEY2PKCS8_broken(AmiSSLBase, (pkey), (broken))

PKCS8_PRIV_KEY_INFO * __PKCS8_set_broken(__reg("a6") struct Library *, __reg("a0") PKCS8_PRIV_KEY_INFO * p8, __reg("d0") LONG broken)="\tjsr\t-12012(a6)";
#define PKCS8_set_broken(p8, broken) __PKCS8_set_broken(AmiSSLBase, (p8), (broken))

int __X509_check_trust(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG id, __reg("d1") LONG flags)="\tjsr\t-12018(a6)";
#define X509_check_trust(x, id, flags) __X509_check_trust(AmiSSLBase, (x), (id), (flags))

int __X509_TRUST_get_count(__reg("a6") struct Library *)="\tjsr\t-12024(a6)";
#define X509_TRUST_get_count() __X509_TRUST_get_count(AmiSSLBase)

X509_TRUST * __X509_TRUST_get0(__reg("a6") struct Library *, __reg("d0") LONG idx)="\tjsr\t-12030(a6)";
#define X509_TRUST_get0(idx) __X509_TRUST_get0(AmiSSLBase, (idx))

int __X509_TRUST_get_by_id(__reg("a6") struct Library *, __reg("d0") LONG id)="\tjsr\t-12036(a6)";
#define X509_TRUST_get_by_id(id) __X509_TRUST_get_by_id(AmiSSLBase, (id))

int __X509_TRUST_add(__reg("a6") struct Library *, __reg("d0") LONG id, __reg("d1") LONG flags, __reg("a0") int (*ck)(X509_TRUST *, X509 *, int), __reg("a1") char * name, __reg("d2") LONG arg1, __reg("a2") void * arg2)="\tjsr\t-12042(a6)";
#define X509_TRUST_add(id, flags, ck, name, arg1, arg2) __X509_TRUST_add(AmiSSLBase, (id), (flags), (ck), (name), (arg1), (arg2))

void __X509_TRUST_cleanup(__reg("a6") struct Library *)="\tjsr\t-12048(a6)";
#define X509_TRUST_cleanup() __X509_TRUST_cleanup(AmiSSLBase)

int __X509_TRUST_get_flags(__reg("a6") struct Library *, __reg("a0") X509_TRUST * xp)="\tjsr\t-12054(a6)";
#define X509_TRUST_get_flags(xp) __X509_TRUST_get_flags(AmiSSLBase, (xp))

char * __X509_TRUST_get0_name(__reg("a6") struct Library *, __reg("a0") X509_TRUST * xp)="\tjsr\t-12060(a6)";
#define X509_TRUST_get0_name(xp) __X509_TRUST_get0_name(AmiSSLBase, (xp))

int __X509_TRUST_get_trust(__reg("a6") struct Library *, __reg("a0") X509_TRUST * xp)="\tjsr\t-12066(a6)";
#define X509_TRUST_get_trust(xp) __X509_TRUST_get_trust(AmiSSLBase, (xp))

void __ERR_load_X509_strings(__reg("a6") struct Library *)="\tjsr\t-12072(a6)";
#define ERR_load_X509_strings() __ERR_load_X509_strings(AmiSSLBase)

int __X509_OBJECT_idx_by_subject(__reg("a6") struct Library *, __reg("a0") void * h, __reg("d0") LONG type, __reg("a1") X509_NAME * name)="\tjsr\t-12078(a6)";
#define X509_OBJECT_idx_by_subject(h, type, name) __X509_OBJECT_idx_by_subject(AmiSSLBase, (h), (type), (name))

X509_OBJECT * __X509_OBJECT_retrieve_by_subject(__reg("a6") struct Library *, __reg("a0") void * h, __reg("d0") LONG type, __reg("a1") X509_NAME * name)="\tjsr\t-12084(a6)";
#define X509_OBJECT_retrieve_by_subject(h, type, name) __X509_OBJECT_retrieve_by_subject(AmiSSLBase, (h), (type), (name))

X509_OBJECT * __X509_OBJECT_retrieve_match(__reg("a6") struct Library *, __reg("a0") void * h, __reg("a1") X509_OBJECT * x)="\tjsr\t-12090(a6)";
#define X509_OBJECT_retrieve_match(h, x) __X509_OBJECT_retrieve_match(AmiSSLBase, (h), (x))

void __X509_OBJECT_up_ref_count(__reg("a6") struct Library *, __reg("a0") X509_OBJECT * a)="\tjsr\t-12096(a6)";
#define X509_OBJECT_up_ref_count(a) __X509_OBJECT_up_ref_count(AmiSSLBase, (a))

void __X509_OBJECT_free_contents(__reg("a6") struct Library *, __reg("a0") X509_OBJECT * a)="\tjsr\t-12102(a6)";
#define X509_OBJECT_free_contents(a) __X509_OBJECT_free_contents(AmiSSLBase, (a))

X509_STORE * __X509_STORE_new(__reg("a6") struct Library *)="\tjsr\t-12108(a6)";
#define X509_STORE_new() __X509_STORE_new(AmiSSLBase)

void __X509_STORE_free(__reg("a6") struct Library *, __reg("a0") X509_STORE * v)="\tjsr\t-12114(a6)";
#define X509_STORE_free(v) __X509_STORE_free(AmiSSLBase, (v))

void __X509_STORE_set_flags(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("d0") long flags)="\tjsr\t-12120(a6)";
#define X509_STORE_set_flags(ctx, flags) __X509_STORE_set_flags(AmiSSLBase, (ctx), (flags))

int __X509_STORE_set_purpose(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("d0") LONG purpose)="\tjsr\t-12126(a6)";
#define X509_STORE_set_purpose(ctx, purpose) __X509_STORE_set_purpose(AmiSSLBase, (ctx), (purpose))

int __X509_STORE_set_trust(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("d0") LONG trust)="\tjsr\t-12132(a6)";
#define X509_STORE_set_trust(ctx, trust) __X509_STORE_set_trust(AmiSSLBase, (ctx), (trust))

X509_STORE_CTX * __X509_STORE_CTX_new(__reg("a6") struct Library *)="\tjsr\t-12138(a6)";
#define X509_STORE_CTX_new() __X509_STORE_CTX_new(AmiSSLBase)

int __X509_STORE_CTX_get1_issuer(__reg("a6") struct Library *, __reg("a0") X509 ** issuer, __reg("a1") X509_STORE_CTX * ctx, __reg("a2") X509 * x)="\tjsr\t-12144(a6)";
#define X509_STORE_CTX_get1_issuer(issuer, ctx, x) __X509_STORE_CTX_get1_issuer(AmiSSLBase, (issuer), (ctx), (x))

void __X509_STORE_CTX_free(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12150(a6)";
#define X509_STORE_CTX_free(ctx) __X509_STORE_CTX_free(AmiSSLBase, (ctx))

int __X509_STORE_CTX_init(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("a1") X509_STORE * store, __reg("a2") X509 * x509, __reg("a3") void * chain)="\tjsr\t-12156(a6)";
#define X509_STORE_CTX_init(ctx, store, x509, chain) __X509_STORE_CTX_init(AmiSSLBase, (ctx), (store), (x509), (chain))

void __X509_STORE_CTX_trusted_stack(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("a1") void * sk)="\tjsr\t-12162(a6)";
#define X509_STORE_CTX_trusted_stack(ctx, sk) __X509_STORE_CTX_trusted_stack(AmiSSLBase, (ctx), (sk))

void __X509_STORE_CTX_cleanup(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12168(a6)";
#define X509_STORE_CTX_cleanup(ctx) __X509_STORE_CTX_cleanup(AmiSSLBase, (ctx))

X509_LOOKUP * __X509_STORE_add_lookup(__reg("a6") struct Library *, __reg("a0") X509_STORE * v, __reg("a1") X509_LOOKUP_METHOD * m)="\tjsr\t-12174(a6)";
#define X509_STORE_add_lookup(v, m) __X509_STORE_add_lookup(AmiSSLBase, (v), (m))

X509_LOOKUP_METHOD * __X509_LOOKUP_hash_dir(__reg("a6") struct Library *)="\tjsr\t-12180(a6)";
#define X509_LOOKUP_hash_dir() __X509_LOOKUP_hash_dir(AmiSSLBase)

X509_LOOKUP_METHOD * __X509_LOOKUP_file(__reg("a6") struct Library *)="\tjsr\t-12186(a6)";
#define X509_LOOKUP_file() __X509_LOOKUP_file(AmiSSLBase)

int __X509_STORE_add_cert(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("a1") X509 * x)="\tjsr\t-12192(a6)";
#define X509_STORE_add_cert(ctx, x) __X509_STORE_add_cert(AmiSSLBase, (ctx), (x))

int __X509_STORE_add_crl(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("a1") X509_CRL * x)="\tjsr\t-12198(a6)";
#define X509_STORE_add_crl(ctx, x) __X509_STORE_add_crl(AmiSSLBase, (ctx), (x))

int __X509_STORE_get_by_subject(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * vs, __reg("d0") LONG type, __reg("a1") X509_NAME * name, __reg("a2") X509_OBJECT * ret)="\tjsr\t-12204(a6)";
#define X509_STORE_get_by_subject(vs, type, name, ret) __X509_STORE_get_by_subject(AmiSSLBase, (vs), (type), (name), (ret))

int __X509_LOOKUP_ctrl(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("d0") LONG cmd, __reg("a1") const char * argc, __reg("d1") long argl, __reg("a2") char ** ret)="\tjsr\t-12210(a6)";
#define X509_LOOKUP_ctrl(ctx, cmd, argc, argl, ret) __X509_LOOKUP_ctrl(AmiSSLBase, (ctx), (cmd), (argc), (argl), (ret))

int __X509_load_cert_file(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-12216(a6)";
#define X509_load_cert_file(ctx, file, type) __X509_load_cert_file(AmiSSLBase, (ctx), (file), (type))

int __X509_load_crl_file(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-12222(a6)";
#define X509_load_crl_file(ctx, file, type) __X509_load_crl_file(AmiSSLBase, (ctx), (file), (type))

int __X509_load_cert_crl_file(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("a1") const char * file, __reg("d0") LONG type)="\tjsr\t-12228(a6)";
#define X509_load_cert_crl_file(ctx, file, type) __X509_load_cert_crl_file(AmiSSLBase, (ctx), (file), (type))

X509_LOOKUP * __X509_LOOKUP_new(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP_METHOD * method)="\tjsr\t-12234(a6)";
#define X509_LOOKUP_new(method) __X509_LOOKUP_new(AmiSSLBase, (method))

void __X509_LOOKUP_free(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx)="\tjsr\t-12240(a6)";
#define X509_LOOKUP_free(ctx) __X509_LOOKUP_free(AmiSSLBase, (ctx))

int __X509_LOOKUP_init(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx)="\tjsr\t-12246(a6)";
#define X509_LOOKUP_init(ctx) __X509_LOOKUP_init(AmiSSLBase, (ctx))

int __X509_LOOKUP_by_subject(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("d0") LONG type, __reg("a1") X509_NAME * name, __reg("a2") X509_OBJECT * ret)="\tjsr\t-12252(a6)";
#define X509_LOOKUP_by_subject(ctx, type, name, ret) __X509_LOOKUP_by_subject(AmiSSLBase, (ctx), (type), (name), (ret))

int __X509_LOOKUP_by_issuer_serial(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("d0") LONG type, __reg("a1") X509_NAME * name, __reg("a2") ASN1_INTEGER * serial, __reg("a3") X509_OBJECT * ret)="\tjsr\t-12258(a6)";
#define X509_LOOKUP_by_issuer_serial(ctx, type, name, serial, ret) __X509_LOOKUP_by_issuer_serial(AmiSSLBase, (ctx), (type), (name), (serial), (ret))

int __X509_LOOKUP_by_fingerprint(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("d0") LONG type, __reg("a1") unsigned char * bytes, __reg("d1") LONG len, __reg("a2") X509_OBJECT * ret)="\tjsr\t-12264(a6)";
#define X509_LOOKUP_by_fingerprint(ctx, type, bytes, len, ret) __X509_LOOKUP_by_fingerprint(AmiSSLBase, (ctx), (type), (bytes), (len), (ret))

int __X509_LOOKUP_by_alias(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx, __reg("d0") LONG type, __reg("a1") char * str, __reg("d1") LONG len, __reg("a2") X509_OBJECT * ret)="\tjsr\t-12270(a6)";
#define X509_LOOKUP_by_alias(ctx, type, str, len, ret) __X509_LOOKUP_by_alias(AmiSSLBase, (ctx), (type), (str), (len), (ret))

int __X509_LOOKUP_shutdown(__reg("a6") struct Library *, __reg("a0") X509_LOOKUP * ctx)="\tjsr\t-12276(a6)";
#define X509_LOOKUP_shutdown(ctx) __X509_LOOKUP_shutdown(AmiSSLBase, (ctx))

int __X509_STORE_load_locations(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx, __reg("a1") const char * file, __reg("a2") const char * dir)="\tjsr\t-12282(a6)";
#define X509_STORE_load_locations(ctx, file, dir) __X509_STORE_load_locations(AmiSSLBase, (ctx), (file), (dir))

int __X509_STORE_set_default_paths(__reg("a6") struct Library *, __reg("a0") X509_STORE * ctx)="\tjsr\t-12288(a6)";
#define X509_STORE_set_default_paths(ctx) __X509_STORE_set_default_paths(AmiSSLBase, (ctx))

int __X509_STORE_CTX_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-12294(a6)";
#define X509_STORE_CTX_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __X509_STORE_CTX_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __X509_STORE_CTX_set_ex_data(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG idx, __reg("a1") void * data)="\tjsr\t-12300(a6)";
#define X509_STORE_CTX_set_ex_data(ctx, idx, data) __X509_STORE_CTX_set_ex_data(AmiSSLBase, (ctx), (idx), (data))

void * __X509_STORE_CTX_get_ex_data(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG idx)="\tjsr\t-12306(a6)";
#define X509_STORE_CTX_get_ex_data(ctx, idx) __X509_STORE_CTX_get_ex_data(AmiSSLBase, (ctx), (idx))

int __X509_STORE_CTX_get_error(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12312(a6)";
#define X509_STORE_CTX_get_error(ctx) __X509_STORE_CTX_get_error(AmiSSLBase, (ctx))

void __X509_STORE_CTX_set_error(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG s)="\tjsr\t-12318(a6)";
#define X509_STORE_CTX_set_error(ctx, s) __X509_STORE_CTX_set_error(AmiSSLBase, (ctx), (s))

int __X509_STORE_CTX_get_error_depth(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12324(a6)";
#define X509_STORE_CTX_get_error_depth(ctx) __X509_STORE_CTX_get_error_depth(AmiSSLBase, (ctx))

X509 * __X509_STORE_CTX_get_current_cert(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12330(a6)";
#define X509_STORE_CTX_get_current_cert(ctx) __X509_STORE_CTX_get_current_cert(AmiSSLBase, (ctx))

void * __X509_STORE_CTX_get_chain(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12336(a6)";
#define X509_STORE_CTX_get_chain(ctx) __X509_STORE_CTX_get_chain(AmiSSLBase, (ctx))

void * __X509_STORE_CTX_get1_chain(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx)="\tjsr\t-12342(a6)";
#define X509_STORE_CTX_get1_chain(ctx) __X509_STORE_CTX_get1_chain(AmiSSLBase, (ctx))

void __X509_STORE_CTX_set_cert(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * c, __reg("a1") X509 * x)="\tjsr\t-12348(a6)";
#define X509_STORE_CTX_set_cert(c, x) __X509_STORE_CTX_set_cert(AmiSSLBase, (c), (x))

void __X509_STORE_CTX_set_chain(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * c, __reg("a1") void * sk)="\tjsr\t-12354(a6)";
#define X509_STORE_CTX_set_chain(c, sk) __X509_STORE_CTX_set_chain(AmiSSLBase, (c), (sk))

int __X509_STORE_CTX_set_purpose(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG purpose)="\tjsr\t-12360(a6)";
#define X509_STORE_CTX_set_purpose(ctx, purpose) __X509_STORE_CTX_set_purpose(AmiSSLBase, (ctx), (purpose))

int __X509_STORE_CTX_set_trust(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG trust)="\tjsr\t-12366(a6)";
#define X509_STORE_CTX_set_trust(ctx, trust) __X509_STORE_CTX_set_trust(AmiSSLBase, (ctx), (trust))

int __X509_STORE_CTX_purpose_inherit(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") LONG def_purpose, __reg("d1") LONG purpose, __reg("d2") LONG trust)="\tjsr\t-12372(a6)";
#define X509_STORE_CTX_purpose_inherit(ctx, def_purpose, purpose, trust) __X509_STORE_CTX_purpose_inherit(AmiSSLBase, (ctx), (def_purpose), (purpose), (trust))

void __X509_STORE_CTX_set_flags(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") long flags)="\tjsr\t-12378(a6)";
#define X509_STORE_CTX_set_flags(ctx, flags) __X509_STORE_CTX_set_flags(AmiSSLBase, (ctx), (flags))

void __X509_STORE_CTX_set_time(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("d0") long flags, __reg("d1") time_t t)="\tjsr\t-12384(a6)";
#define X509_STORE_CTX_set_time(ctx, flags, t) __X509_STORE_CTX_set_time(AmiSSLBase, (ctx), (flags), (t))

void __X509_STORE_CTX_set_verify_cb(__reg("a6") struct Library *, __reg("a0") X509_STORE_CTX * ctx, __reg("a1") int (*verify_cb)(int, X509_STORE_CTX *))="\tjsr\t-12390(a6)";
#define X509_STORE_CTX_set_verify_cb(ctx, verify_cb) __X509_STORE_CTX_set_verify_cb(AmiSSLBase, (ctx), (verify_cb))

BASIC_CONSTRAINTS * __BASIC_CONSTRAINTS_new(__reg("a6") struct Library *)="\tjsr\t-12396(a6)";
#define BASIC_CONSTRAINTS_new() __BASIC_CONSTRAINTS_new(AmiSSLBase)

void __BASIC_CONSTRAINTS_free(__reg("a6") struct Library *, __reg("a0") BASIC_CONSTRAINTS * a)="\tjsr\t-12402(a6)";
#define BASIC_CONSTRAINTS_free(a) __BASIC_CONSTRAINTS_free(AmiSSLBase, (a))

BASIC_CONSTRAINTS * __d2i_BASIC_CONSTRAINTS(__reg("a6") struct Library *, __reg("a0") BASIC_CONSTRAINTS ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12408(a6)";
#define d2i_BASIC_CONSTRAINTS(a, in, len) __d2i_BASIC_CONSTRAINTS(AmiSSLBase, (a), (in), (len))

int __i2d_BASIC_CONSTRAINTS(__reg("a6") struct Library *, __reg("a0") BASIC_CONSTRAINTS * a, __reg("a1") unsigned char ** out)="\tjsr\t-12414(a6)";
#define i2d_BASIC_CONSTRAINTS(a, out) __i2d_BASIC_CONSTRAINTS(AmiSSLBase, (a), (out))

const ASN1_ITEM * __BASIC_CONSTRAINTS_it(__reg("a6") struct Library *)="\tjsr\t-12420(a6)";
#define BASIC_CONSTRAINTS_it() __BASIC_CONSTRAINTS_it(AmiSSLBase)

SXNET * __SXNET_new(__reg("a6") struct Library *)="\tjsr\t-12426(a6)";
#define SXNET_new() __SXNET_new(AmiSSLBase)

void __SXNET_free(__reg("a6") struct Library *, __reg("a0") SXNET * a)="\tjsr\t-12432(a6)";
#define SXNET_free(a) __SXNET_free(AmiSSLBase, (a))

SXNET * __d2i_SXNET(__reg("a6") struct Library *, __reg("a0") SXNET ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12438(a6)";
#define d2i_SXNET(a, in, len) __d2i_SXNET(AmiSSLBase, (a), (in), (len))

int __i2d_SXNET(__reg("a6") struct Library *, __reg("a0") SXNET * a, __reg("a1") unsigned char ** out)="\tjsr\t-12444(a6)";
#define i2d_SXNET(a, out) __i2d_SXNET(AmiSSLBase, (a), (out))

const ASN1_ITEM * __SXNET_it(__reg("a6") struct Library *)="\tjsr\t-12450(a6)";
#define SXNET_it() __SXNET_it(AmiSSLBase)

SXNETID * __SXNETID_new(__reg("a6") struct Library *)="\tjsr\t-12456(a6)";
#define SXNETID_new() __SXNETID_new(AmiSSLBase)

void __SXNETID_free(__reg("a6") struct Library *, __reg("a0") SXNETID * a)="\tjsr\t-12462(a6)";
#define SXNETID_free(a) __SXNETID_free(AmiSSLBase, (a))

SXNETID * __d2i_SXNETID(__reg("a6") struct Library *, __reg("a0") SXNETID ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12468(a6)";
#define d2i_SXNETID(a, in, len) __d2i_SXNETID(AmiSSLBase, (a), (in), (len))

int __i2d_SXNETID(__reg("a6") struct Library *, __reg("a0") SXNETID * a, __reg("a1") unsigned char ** out)="\tjsr\t-12474(a6)";
#define i2d_SXNETID(a, out) __i2d_SXNETID(AmiSSLBase, (a), (out))

const ASN1_ITEM * __SXNETID_it(__reg("a6") struct Library *)="\tjsr\t-12480(a6)";
#define SXNETID_it() __SXNETID_it(AmiSSLBase)

int __SXNET_add_id_asc(__reg("a6") struct Library *, __reg("a0") SXNET ** psx, __reg("a1") char * zone, __reg("a2") char * user, __reg("d0") LONG userlen)="\tjsr\t-12486(a6)";
#define SXNET_add_id_asc(psx, zone, user, userlen) __SXNET_add_id_asc(AmiSSLBase, (psx), (zone), (user), (userlen))

int __SXNET_add_id_ulong(__reg("a6") struct Library *, __reg("a0") SXNET ** psx, __reg("d0") unsigned long lzone, __reg("a1") char * user, __reg("d1") LONG userlen)="\tjsr\t-12492(a6)";
#define SXNET_add_id_ulong(psx, lzone, user, userlen) __SXNET_add_id_ulong(AmiSSLBase, (psx), (lzone), (user), (userlen))

int __SXNET_add_id_INTEGER(__reg("a6") struct Library *, __reg("a0") SXNET ** psx, __reg("a1") ASN1_INTEGER * izone, __reg("a2") char * user, __reg("d0") LONG userlen)="\tjsr\t-12498(a6)";
#define SXNET_add_id_INTEGER(psx, izone, user, userlen) __SXNET_add_id_INTEGER(AmiSSLBase, (psx), (izone), (user), (userlen))

ASN1_OCTET_STRING * __SXNET_get_id_asc(__reg("a6") struct Library *, __reg("a0") SXNET * sx, __reg("a1") char * zone)="\tjsr\t-12504(a6)";
#define SXNET_get_id_asc(sx, zone) __SXNET_get_id_asc(AmiSSLBase, (sx), (zone))

ASN1_OCTET_STRING * __SXNET_get_id_ulong(__reg("a6") struct Library *, __reg("a0") SXNET * sx, __reg("d0") unsigned long lzone)="\tjsr\t-12510(a6)";
#define SXNET_get_id_ulong(sx, lzone) __SXNET_get_id_ulong(AmiSSLBase, (sx), (lzone))

ASN1_OCTET_STRING * __SXNET_get_id_INTEGER(__reg("a6") struct Library *, __reg("a0") SXNET * sx, __reg("a1") ASN1_INTEGER * zone)="\tjsr\t-12516(a6)";
#define SXNET_get_id_INTEGER(sx, zone) __SXNET_get_id_INTEGER(AmiSSLBase, (sx), (zone))

AUTHORITY_KEYID * __AUTHORITY_KEYID_new(__reg("a6") struct Library *)="\tjsr\t-12522(a6)";
#define AUTHORITY_KEYID_new() __AUTHORITY_KEYID_new(AmiSSLBase)

void __AUTHORITY_KEYID_free(__reg("a6") struct Library *, __reg("a0") AUTHORITY_KEYID * a)="\tjsr\t-12528(a6)";
#define AUTHORITY_KEYID_free(a) __AUTHORITY_KEYID_free(AmiSSLBase, (a))

AUTHORITY_KEYID * __d2i_AUTHORITY_KEYID(__reg("a6") struct Library *, __reg("a0") AUTHORITY_KEYID ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12534(a6)";
#define d2i_AUTHORITY_KEYID(a, in, len) __d2i_AUTHORITY_KEYID(AmiSSLBase, (a), (in), (len))

int __i2d_AUTHORITY_KEYID(__reg("a6") struct Library *, __reg("a0") AUTHORITY_KEYID * a, __reg("a1") unsigned char ** out)="\tjsr\t-12540(a6)";
#define i2d_AUTHORITY_KEYID(a, out) __i2d_AUTHORITY_KEYID(AmiSSLBase, (a), (out))

const ASN1_ITEM * __AUTHORITY_KEYID_it(__reg("a6") struct Library *)="\tjsr\t-12546(a6)";
#define AUTHORITY_KEYID_it() __AUTHORITY_KEYID_it(AmiSSLBase)

PKEY_USAGE_PERIOD * __PKEY_USAGE_PERIOD_new(__reg("a6") struct Library *)="\tjsr\t-12552(a6)";
#define PKEY_USAGE_PERIOD_new() __PKEY_USAGE_PERIOD_new(AmiSSLBase)

void __PKEY_USAGE_PERIOD_free(__reg("a6") struct Library *, __reg("a0") PKEY_USAGE_PERIOD * a)="\tjsr\t-12558(a6)";
#define PKEY_USAGE_PERIOD_free(a) __PKEY_USAGE_PERIOD_free(AmiSSLBase, (a))

PKEY_USAGE_PERIOD * __d2i_PKEY_USAGE_PERIOD(__reg("a6") struct Library *, __reg("a0") PKEY_USAGE_PERIOD ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12564(a6)";
#define d2i_PKEY_USAGE_PERIOD(a, in, len) __d2i_PKEY_USAGE_PERIOD(AmiSSLBase, (a), (in), (len))

int __i2d_PKEY_USAGE_PERIOD(__reg("a6") struct Library *, __reg("a0") PKEY_USAGE_PERIOD * a, __reg("a1") unsigned char ** out)="\tjsr\t-12570(a6)";
#define i2d_PKEY_USAGE_PERIOD(a, out) __i2d_PKEY_USAGE_PERIOD(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PKEY_USAGE_PERIOD_it(__reg("a6") struct Library *)="\tjsr\t-12576(a6)";
#define PKEY_USAGE_PERIOD_it() __PKEY_USAGE_PERIOD_it(AmiSSLBase)

GENERAL_NAME * __GENERAL_NAME_new(__reg("a6") struct Library *)="\tjsr\t-12582(a6)";
#define GENERAL_NAME_new() __GENERAL_NAME_new(AmiSSLBase)

void __GENERAL_NAME_free(__reg("a6") struct Library *, __reg("a0") GENERAL_NAME * a)="\tjsr\t-12588(a6)";
#define GENERAL_NAME_free(a) __GENERAL_NAME_free(AmiSSLBase, (a))

GENERAL_NAME * __d2i_GENERAL_NAME(__reg("a6") struct Library *, __reg("a0") GENERAL_NAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12594(a6)";
#define d2i_GENERAL_NAME(a, in, len) __d2i_GENERAL_NAME(AmiSSLBase, (a), (in), (len))

int __i2d_GENERAL_NAME(__reg("a6") struct Library *, __reg("a0") GENERAL_NAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-12600(a6)";
#define i2d_GENERAL_NAME(a, out) __i2d_GENERAL_NAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __GENERAL_NAME_it(__reg("a6") struct Library *)="\tjsr\t-12606(a6)";
#define GENERAL_NAME_it() __GENERAL_NAME_it(AmiSSLBase)

void * __i2v_GENERAL_NAME(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") GENERAL_NAME * gen, __reg("a2") void * ret)="\tjsr\t-12612(a6)";
#define i2v_GENERAL_NAME(method, gen, ret) __i2v_GENERAL_NAME(AmiSSLBase, (method), (gen), (ret))

int __GENERAL_NAME_print(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") GENERAL_NAME * gen)="\tjsr\t-12618(a6)";
#define GENERAL_NAME_print(out, gen) __GENERAL_NAME_print(AmiSSLBase, (out), (gen))

GENERAL_NAMES * __GENERAL_NAMES_new(__reg("a6") struct Library *)="\tjsr\t-12624(a6)";
#define GENERAL_NAMES_new() __GENERAL_NAMES_new(AmiSSLBase)

void __GENERAL_NAMES_free(__reg("a6") struct Library *, __reg("a0") GENERAL_NAMES * a)="\tjsr\t-12630(a6)";
#define GENERAL_NAMES_free(a) __GENERAL_NAMES_free(AmiSSLBase, (a))

GENERAL_NAMES * __d2i_GENERAL_NAMES(__reg("a6") struct Library *, __reg("a0") GENERAL_NAMES ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12636(a6)";
#define d2i_GENERAL_NAMES(a, in, len) __d2i_GENERAL_NAMES(AmiSSLBase, (a), (in), (len))

int __i2d_GENERAL_NAMES(__reg("a6") struct Library *, __reg("a0") GENERAL_NAMES * a, __reg("a1") unsigned char ** out)="\tjsr\t-12642(a6)";
#define i2d_GENERAL_NAMES(a, out) __i2d_GENERAL_NAMES(AmiSSLBase, (a), (out))

const ASN1_ITEM * __GENERAL_NAMES_it(__reg("a6") struct Library *)="\tjsr\t-12648(a6)";
#define GENERAL_NAMES_it() __GENERAL_NAMES_it(AmiSSLBase)

void * __i2v_GENERAL_NAMES(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") GENERAL_NAMES * gen, __reg("a2") void * extlist)="\tjsr\t-12654(a6)";
#define i2v_GENERAL_NAMES(method, gen, extlist) __i2v_GENERAL_NAMES(AmiSSLBase, (method), (gen), (extlist))

GENERAL_NAMES * __v2i_GENERAL_NAMES(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") X509V3_CTX * ctx, __reg("a2") void * nval)="\tjsr\t-12660(a6)";
#define v2i_GENERAL_NAMES(method, ctx, nval) __v2i_GENERAL_NAMES(AmiSSLBase, (method), (ctx), (nval))

OTHERNAME * __OTHERNAME_new(__reg("a6") struct Library *)="\tjsr\t-12666(a6)";
#define OTHERNAME_new() __OTHERNAME_new(AmiSSLBase)

void __OTHERNAME_free(__reg("a6") struct Library *, __reg("a0") OTHERNAME * a)="\tjsr\t-12672(a6)";
#define OTHERNAME_free(a) __OTHERNAME_free(AmiSSLBase, (a))

OTHERNAME * __d2i_OTHERNAME(__reg("a6") struct Library *, __reg("a0") OTHERNAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12678(a6)";
#define d2i_OTHERNAME(a, in, len) __d2i_OTHERNAME(AmiSSLBase, (a), (in), (len))

int __i2d_OTHERNAME(__reg("a6") struct Library *, __reg("a0") OTHERNAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-12684(a6)";
#define i2d_OTHERNAME(a, out) __i2d_OTHERNAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __OTHERNAME_it(__reg("a6") struct Library *)="\tjsr\t-12690(a6)";
#define OTHERNAME_it() __OTHERNAME_it(AmiSSLBase)

EDIPARTYNAME * __EDIPARTYNAME_new(__reg("a6") struct Library *)="\tjsr\t-12696(a6)";
#define EDIPARTYNAME_new() __EDIPARTYNAME_new(AmiSSLBase)

void __EDIPARTYNAME_free(__reg("a6") struct Library *, __reg("a0") EDIPARTYNAME * a)="\tjsr\t-12702(a6)";
#define EDIPARTYNAME_free(a) __EDIPARTYNAME_free(AmiSSLBase, (a))

EDIPARTYNAME * __d2i_EDIPARTYNAME(__reg("a6") struct Library *, __reg("a0") EDIPARTYNAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12708(a6)";
#define d2i_EDIPARTYNAME(a, in, len) __d2i_EDIPARTYNAME(AmiSSLBase, (a), (in), (len))

int __i2d_EDIPARTYNAME(__reg("a6") struct Library *, __reg("a0") EDIPARTYNAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-12714(a6)";
#define i2d_EDIPARTYNAME(a, out) __i2d_EDIPARTYNAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __EDIPARTYNAME_it(__reg("a6") struct Library *)="\tjsr\t-12720(a6)";
#define EDIPARTYNAME_it() __EDIPARTYNAME_it(AmiSSLBase)

char * __i2s_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") ASN1_OCTET_STRING * ia5)="\tjsr\t-12726(a6)";
#define i2s_ASN1_OCTET_STRING(method, ia5) __i2s_ASN1_OCTET_STRING(AmiSSLBase, (method), (ia5))

ASN1_OCTET_STRING * __s2i_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * str)="\tjsr\t-12732(a6)";
#define s2i_ASN1_OCTET_STRING(method, ctx, str) __s2i_ASN1_OCTET_STRING(AmiSSLBase, (method), (ctx), (str))

EXTENDED_KEY_USAGE * __EXTENDED_KEY_USAGE_new(__reg("a6") struct Library *)="\tjsr\t-12738(a6)";
#define EXTENDED_KEY_USAGE_new() __EXTENDED_KEY_USAGE_new(AmiSSLBase)

void __EXTENDED_KEY_USAGE_free(__reg("a6") struct Library *, __reg("a0") EXTENDED_KEY_USAGE * a)="\tjsr\t-12744(a6)";
#define EXTENDED_KEY_USAGE_free(a) __EXTENDED_KEY_USAGE_free(AmiSSLBase, (a))

EXTENDED_KEY_USAGE * __d2i_EXTENDED_KEY_USAGE(__reg("a6") struct Library *, __reg("a0") EXTENDED_KEY_USAGE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12750(a6)";
#define d2i_EXTENDED_KEY_USAGE(a, in, len) __d2i_EXTENDED_KEY_USAGE(AmiSSLBase, (a), (in), (len))

int __i2d_EXTENDED_KEY_USAGE(__reg("a6") struct Library *, __reg("a0") EXTENDED_KEY_USAGE * a, __reg("a1") unsigned char ** out)="\tjsr\t-12756(a6)";
#define i2d_EXTENDED_KEY_USAGE(a, out) __i2d_EXTENDED_KEY_USAGE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __EXTENDED_KEY_USAGE_it(__reg("a6") struct Library *)="\tjsr\t-12762(a6)";
#define EXTENDED_KEY_USAGE_it() __EXTENDED_KEY_USAGE_it(AmiSSLBase)

int __i2a_ACCESS_DESCRIPTION(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") ACCESS_DESCRIPTION* a)="\tjsr\t-12768(a6)";
#define i2a_ACCESS_DESCRIPTION(bp, a) __i2a_ACCESS_DESCRIPTION(AmiSSLBase, (bp), (a))

CERTIFICATEPOLICIES * __CERTIFICATEPOLICIES_new(__reg("a6") struct Library *)="\tjsr\t-12774(a6)";
#define CERTIFICATEPOLICIES_new() __CERTIFICATEPOLICIES_new(AmiSSLBase)

void __CERTIFICATEPOLICIES_free(__reg("a6") struct Library *, __reg("a0") CERTIFICATEPOLICIES * a)="\tjsr\t-12780(a6)";
#define CERTIFICATEPOLICIES_free(a) __CERTIFICATEPOLICIES_free(AmiSSLBase, (a))

CERTIFICATEPOLICIES * __d2i_CERTIFICATEPOLICIES(__reg("a6") struct Library *, __reg("a0") CERTIFICATEPOLICIES ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12786(a6)";
#define d2i_CERTIFICATEPOLICIES(a, in, len) __d2i_CERTIFICATEPOLICIES(AmiSSLBase, (a), (in), (len))

int __i2d_CERTIFICATEPOLICIES(__reg("a6") struct Library *, __reg("a0") CERTIFICATEPOLICIES * a, __reg("a1") unsigned char ** out)="\tjsr\t-12792(a6)";
#define i2d_CERTIFICATEPOLICIES(a, out) __i2d_CERTIFICATEPOLICIES(AmiSSLBase, (a), (out))

const ASN1_ITEM * __CERTIFICATEPOLICIES_it(__reg("a6") struct Library *)="\tjsr\t-12798(a6)";
#define CERTIFICATEPOLICIES_it() __CERTIFICATEPOLICIES_it(AmiSSLBase)

POLICYINFO * __POLICYINFO_new(__reg("a6") struct Library *)="\tjsr\t-12804(a6)";
#define POLICYINFO_new() __POLICYINFO_new(AmiSSLBase)

void __POLICYINFO_free(__reg("a6") struct Library *, __reg("a0") POLICYINFO * a)="\tjsr\t-12810(a6)";
#define POLICYINFO_free(a) __POLICYINFO_free(AmiSSLBase, (a))

POLICYINFO * __d2i_POLICYINFO(__reg("a6") struct Library *, __reg("a0") POLICYINFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12816(a6)";
#define d2i_POLICYINFO(a, in, len) __d2i_POLICYINFO(AmiSSLBase, (a), (in), (len))

int __i2d_POLICYINFO(__reg("a6") struct Library *, __reg("a0") POLICYINFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-12822(a6)";
#define i2d_POLICYINFO(a, out) __i2d_POLICYINFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __POLICYINFO_it(__reg("a6") struct Library *)="\tjsr\t-12828(a6)";
#define POLICYINFO_it() __POLICYINFO_it(AmiSSLBase)

POLICYQUALINFO * __POLICYQUALINFO_new(__reg("a6") struct Library *)="\tjsr\t-12834(a6)";
#define POLICYQUALINFO_new() __POLICYQUALINFO_new(AmiSSLBase)

void __POLICYQUALINFO_free(__reg("a6") struct Library *, __reg("a0") POLICYQUALINFO * a)="\tjsr\t-12840(a6)";
#define POLICYQUALINFO_free(a) __POLICYQUALINFO_free(AmiSSLBase, (a))

POLICYQUALINFO * __d2i_POLICYQUALINFO(__reg("a6") struct Library *, __reg("a0") POLICYQUALINFO ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12846(a6)";
#define d2i_POLICYQUALINFO(a, in, len) __d2i_POLICYQUALINFO(AmiSSLBase, (a), (in), (len))

int __i2d_POLICYQUALINFO(__reg("a6") struct Library *, __reg("a0") POLICYQUALINFO * a, __reg("a1") unsigned char ** out)="\tjsr\t-12852(a6)";
#define i2d_POLICYQUALINFO(a, out) __i2d_POLICYQUALINFO(AmiSSLBase, (a), (out))

const ASN1_ITEM * __POLICYQUALINFO_it(__reg("a6") struct Library *)="\tjsr\t-12858(a6)";
#define POLICYQUALINFO_it() __POLICYQUALINFO_it(AmiSSLBase)

USERNOTICE * __USERNOTICE_new(__reg("a6") struct Library *)="\tjsr\t-12864(a6)";
#define USERNOTICE_new() __USERNOTICE_new(AmiSSLBase)

void __USERNOTICE_free(__reg("a6") struct Library *, __reg("a0") USERNOTICE * a)="\tjsr\t-12870(a6)";
#define USERNOTICE_free(a) __USERNOTICE_free(AmiSSLBase, (a))

USERNOTICE * __d2i_USERNOTICE(__reg("a6") struct Library *, __reg("a0") USERNOTICE ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12876(a6)";
#define d2i_USERNOTICE(a, in, len) __d2i_USERNOTICE(AmiSSLBase, (a), (in), (len))

int __i2d_USERNOTICE(__reg("a6") struct Library *, __reg("a0") USERNOTICE * a, __reg("a1") unsigned char ** out)="\tjsr\t-12882(a6)";
#define i2d_USERNOTICE(a, out) __i2d_USERNOTICE(AmiSSLBase, (a), (out))

const ASN1_ITEM * __USERNOTICE_it(__reg("a6") struct Library *)="\tjsr\t-12888(a6)";
#define USERNOTICE_it() __USERNOTICE_it(AmiSSLBase)

NOTICEREF * __NOTICEREF_new(__reg("a6") struct Library *)="\tjsr\t-12894(a6)";
#define NOTICEREF_new() __NOTICEREF_new(AmiSSLBase)

void __NOTICEREF_free(__reg("a6") struct Library *, __reg("a0") NOTICEREF * a)="\tjsr\t-12900(a6)";
#define NOTICEREF_free(a) __NOTICEREF_free(AmiSSLBase, (a))

NOTICEREF * __d2i_NOTICEREF(__reg("a6") struct Library *, __reg("a0") NOTICEREF ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12906(a6)";
#define d2i_NOTICEREF(a, in, len) __d2i_NOTICEREF(AmiSSLBase, (a), (in), (len))

int __i2d_NOTICEREF(__reg("a6") struct Library *, __reg("a0") NOTICEREF * a, __reg("a1") unsigned char ** out)="\tjsr\t-12912(a6)";
#define i2d_NOTICEREF(a, out) __i2d_NOTICEREF(AmiSSLBase, (a), (out))

const ASN1_ITEM * __NOTICEREF_it(__reg("a6") struct Library *)="\tjsr\t-12918(a6)";
#define NOTICEREF_it() __NOTICEREF_it(AmiSSLBase)

CRL_DIST_POINTS * __CRL_DIST_POINTS_new(__reg("a6") struct Library *)="\tjsr\t-12924(a6)";
#define CRL_DIST_POINTS_new() __CRL_DIST_POINTS_new(AmiSSLBase)

void __CRL_DIST_POINTS_free(__reg("a6") struct Library *, __reg("a0") CRL_DIST_POINTS * a)="\tjsr\t-12930(a6)";
#define CRL_DIST_POINTS_free(a) __CRL_DIST_POINTS_free(AmiSSLBase, (a))

CRL_DIST_POINTS * __d2i_CRL_DIST_POINTS(__reg("a6") struct Library *, __reg("a0") CRL_DIST_POINTS ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12936(a6)";
#define d2i_CRL_DIST_POINTS(a, in, len) __d2i_CRL_DIST_POINTS(AmiSSLBase, (a), (in), (len))

int __i2d_CRL_DIST_POINTS(__reg("a6") struct Library *, __reg("a0") CRL_DIST_POINTS * a, __reg("a1") unsigned char ** out)="\tjsr\t-12942(a6)";
#define i2d_CRL_DIST_POINTS(a, out) __i2d_CRL_DIST_POINTS(AmiSSLBase, (a), (out))

const ASN1_ITEM * __CRL_DIST_POINTS_it(__reg("a6") struct Library *)="\tjsr\t-12948(a6)";
#define CRL_DIST_POINTS_it() __CRL_DIST_POINTS_it(AmiSSLBase)

DIST_POINT * __DIST_POINT_new(__reg("a6") struct Library *)="\tjsr\t-12954(a6)";
#define DIST_POINT_new() __DIST_POINT_new(AmiSSLBase)

void __DIST_POINT_free(__reg("a6") struct Library *, __reg("a0") DIST_POINT * a)="\tjsr\t-12960(a6)";
#define DIST_POINT_free(a) __DIST_POINT_free(AmiSSLBase, (a))

DIST_POINT * __d2i_DIST_POINT(__reg("a6") struct Library *, __reg("a0") DIST_POINT ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12966(a6)";
#define d2i_DIST_POINT(a, in, len) __d2i_DIST_POINT(AmiSSLBase, (a), (in), (len))

int __i2d_DIST_POINT(__reg("a6") struct Library *, __reg("a0") DIST_POINT * a, __reg("a1") unsigned char ** out)="\tjsr\t-12972(a6)";
#define i2d_DIST_POINT(a, out) __i2d_DIST_POINT(AmiSSLBase, (a), (out))

const ASN1_ITEM * __DIST_POINT_it(__reg("a6") struct Library *)="\tjsr\t-12978(a6)";
#define DIST_POINT_it() __DIST_POINT_it(AmiSSLBase)

DIST_POINT_NAME * __DIST_POINT_NAME_new(__reg("a6") struct Library *)="\tjsr\t-12984(a6)";
#define DIST_POINT_NAME_new() __DIST_POINT_NAME_new(AmiSSLBase)

void __DIST_POINT_NAME_free(__reg("a6") struct Library *, __reg("a0") DIST_POINT_NAME * a)="\tjsr\t-12990(a6)";
#define DIST_POINT_NAME_free(a) __DIST_POINT_NAME_free(AmiSSLBase, (a))

DIST_POINT_NAME * __d2i_DIST_POINT_NAME(__reg("a6") struct Library *, __reg("a0") DIST_POINT_NAME ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-12996(a6)";
#define d2i_DIST_POINT_NAME(a, in, len) __d2i_DIST_POINT_NAME(AmiSSLBase, (a), (in), (len))

int __i2d_DIST_POINT_NAME(__reg("a6") struct Library *, __reg("a0") DIST_POINT_NAME * a, __reg("a1") unsigned char ** out)="\tjsr\t-13002(a6)";
#define i2d_DIST_POINT_NAME(a, out) __i2d_DIST_POINT_NAME(AmiSSLBase, (a), (out))

const ASN1_ITEM * __DIST_POINT_NAME_it(__reg("a6") struct Library *)="\tjsr\t-13008(a6)";
#define DIST_POINT_NAME_it() __DIST_POINT_NAME_it(AmiSSLBase)

ACCESS_DESCRIPTION * __ACCESS_DESCRIPTION_new(__reg("a6") struct Library *)="\tjsr\t-13014(a6)";
#define ACCESS_DESCRIPTION_new() __ACCESS_DESCRIPTION_new(AmiSSLBase)

void __ACCESS_DESCRIPTION_free(__reg("a6") struct Library *, __reg("a0") ACCESS_DESCRIPTION * a)="\tjsr\t-13020(a6)";
#define ACCESS_DESCRIPTION_free(a) __ACCESS_DESCRIPTION_free(AmiSSLBase, (a))

ACCESS_DESCRIPTION * __d2i_ACCESS_DESCRIPTION(__reg("a6") struct Library *, __reg("a0") ACCESS_DESCRIPTION ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-13026(a6)";
#define d2i_ACCESS_DESCRIPTION(a, in, len) __d2i_ACCESS_DESCRIPTION(AmiSSLBase, (a), (in), (len))

int __i2d_ACCESS_DESCRIPTION(__reg("a6") struct Library *, __reg("a0") ACCESS_DESCRIPTION * a, __reg("a1") unsigned char ** out)="\tjsr\t-13032(a6)";
#define i2d_ACCESS_DESCRIPTION(a, out) __i2d_ACCESS_DESCRIPTION(AmiSSLBase, (a), (out))

const ASN1_ITEM * __ACCESS_DESCRIPTION_it(__reg("a6") struct Library *)="\tjsr\t-13038(a6)";
#define ACCESS_DESCRIPTION_it() __ACCESS_DESCRIPTION_it(AmiSSLBase)

AUTHORITY_INFO_ACCESS * __AUTHORITY_INFO_ACCESS_new(__reg("a6") struct Library *)="\tjsr\t-13044(a6)";
#define AUTHORITY_INFO_ACCESS_new() __AUTHORITY_INFO_ACCESS_new(AmiSSLBase)

void __AUTHORITY_INFO_ACCESS_free(__reg("a6") struct Library *, __reg("a0") AUTHORITY_INFO_ACCESS * a)="\tjsr\t-13050(a6)";
#define AUTHORITY_INFO_ACCESS_free(a) __AUTHORITY_INFO_ACCESS_free(AmiSSLBase, (a))

AUTHORITY_INFO_ACCESS * __d2i_AUTHORITY_INFO_ACCESS(__reg("a6") struct Library *, __reg("a0") AUTHORITY_INFO_ACCESS ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-13056(a6)";
#define d2i_AUTHORITY_INFO_ACCESS(a, in, len) __d2i_AUTHORITY_INFO_ACCESS(AmiSSLBase, (a), (in), (len))

int __i2d_AUTHORITY_INFO_ACCESS(__reg("a6") struct Library *, __reg("a0") AUTHORITY_INFO_ACCESS * a, __reg("a1") unsigned char ** out)="\tjsr\t-13062(a6)";
#define i2d_AUTHORITY_INFO_ACCESS(a, out) __i2d_AUTHORITY_INFO_ACCESS(AmiSSLBase, (a), (out))

const ASN1_ITEM * __AUTHORITY_INFO_ACCESS_it(__reg("a6") struct Library *)="\tjsr\t-13068(a6)";
#define AUTHORITY_INFO_ACCESS_it() __AUTHORITY_INFO_ACCESS_it(AmiSSLBase)

GENERAL_NAME * __v2i_GENERAL_NAME(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * method, __reg("a1") X509V3_CTX * ctx, __reg("a2") CONF_VALUE * cnf)="\tjsr\t-13074(a6)";
#define v2i_GENERAL_NAME(method, ctx, cnf) __v2i_GENERAL_NAME(AmiSSLBase, (method), (ctx), (cnf))

void __X509V3_conf_free(__reg("a6") struct Library *, __reg("a0") CONF_VALUE * val)="\tjsr\t-13080(a6)";
#define X509V3_conf_free(val) __X509V3_conf_free(AmiSSLBase, (val))

X509_EXTENSION * __X509V3_EXT_nconf_nid(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("d0") LONG ext_nid, __reg("a2") char * value)="\tjsr\t-13086(a6)";
#define X509V3_EXT_nconf_nid(conf, ctx, ext_nid, value) __X509V3_EXT_nconf_nid(AmiSSLBase, (conf), (ctx), (ext_nid), (value))

X509_EXTENSION * __X509V3_EXT_nconf(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * name, __reg("a3") char * value)="\tjsr\t-13092(a6)";
#define X509V3_EXT_nconf(conf, ctx, name, value) __X509V3_EXT_nconf(AmiSSLBase, (conf), (ctx), (name), (value))

int __X509V3_EXT_add_nconf_sk(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") void ** sk)="\tjsr\t-13098(a6)";
#define X509V3_EXT_add_nconf_sk(conf, ctx, section, sk) __X509V3_EXT_add_nconf_sk(AmiSSLBase, (conf), (ctx), (section), (sk))

int __X509V3_EXT_add_nconf(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509 * cert)="\tjsr\t-13104(a6)";
#define X509V3_EXT_add_nconf(conf, ctx, section, cert) __X509V3_EXT_add_nconf(AmiSSLBase, (conf), (ctx), (section), (cert))

int __X509V3_EXT_REQ_add_nconf(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509_REQ * req)="\tjsr\t-13110(a6)";
#define X509V3_EXT_REQ_add_nconf(conf, ctx, section, req) __X509V3_EXT_REQ_add_nconf(AmiSSLBase, (conf), (ctx), (section), (req))

int __X509V3_EXT_CRL_add_nconf(__reg("a6") struct Library *, __reg("a0") CONF * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509_CRL * crl)="\tjsr\t-13116(a6)";
#define X509V3_EXT_CRL_add_nconf(conf, ctx, section, crl) __X509V3_EXT_CRL_add_nconf(AmiSSLBase, (conf), (ctx), (section), (crl))

X509_EXTENSION * __X509V3_EXT_conf_nid(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") X509V3_CTX * ctx, __reg("d0") LONG ext_nid, __reg("a2") char * value)="\tjsr\t-13122(a6)";
#define X509V3_EXT_conf_nid(conf, ctx, ext_nid, value) __X509V3_EXT_conf_nid(AmiSSLBase, (conf), (ctx), (ext_nid), (value))

X509_EXTENSION * __X509V3_EXT_conf(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * name, __reg("a3") char * value)="\tjsr\t-13128(a6)";
#define X509V3_EXT_conf(conf, ctx, name, value) __X509V3_EXT_conf(AmiSSLBase, (conf), (ctx), (name), (value))

int __X509V3_EXT_add_conf(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509 * cert)="\tjsr\t-13134(a6)";
#define X509V3_EXT_add_conf(conf, ctx, section, cert) __X509V3_EXT_add_conf(AmiSSLBase, (conf), (ctx), (section), (cert))

int __X509V3_EXT_REQ_add_conf(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509_REQ * req)="\tjsr\t-13140(a6)";
#define X509V3_EXT_REQ_add_conf(conf, ctx, section, req) __X509V3_EXT_REQ_add_conf(AmiSSLBase, (conf), (ctx), (section), (req))

int __X509V3_EXT_CRL_add_conf(__reg("a6") struct Library *, __reg("a0") LHASH * conf, __reg("a1") X509V3_CTX * ctx, __reg("a2") char * section, __reg("a3") X509_CRL * crl)="\tjsr\t-13146(a6)";
#define X509V3_EXT_CRL_add_conf(conf, ctx, section, crl) __X509V3_EXT_CRL_add_conf(AmiSSLBase, (conf), (ctx), (section), (crl))

int __X509V3_add_value_bool_nf(__reg("a6") struct Library *, __reg("a0") char * name, __reg("d0") LONG asn1_bool, __reg("a1") void ** extlist)="\tjsr\t-13152(a6)";
#define X509V3_add_value_bool_nf(name, asn1_bool, extlist) __X509V3_add_value_bool_nf(AmiSSLBase, (name), (asn1_bool), (extlist))

int __X509V3_get_value_bool(__reg("a6") struct Library *, __reg("a0") CONF_VALUE * value, __reg("a1") int * asn1_bool)="\tjsr\t-13158(a6)";
#define X509V3_get_value_bool(value, asn1_bool) __X509V3_get_value_bool(AmiSSLBase, (value), (asn1_bool))

int __X509V3_get_value_int(__reg("a6") struct Library *, __reg("a0") CONF_VALUE * value, __reg("a1") ASN1_INTEGER ** aint)="\tjsr\t-13164(a6)";
#define X509V3_get_value_int(value, aint) __X509V3_get_value_int(AmiSSLBase, (value), (aint))

void __X509V3_set_nconf(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") CONF * conf)="\tjsr\t-13170(a6)";
#define X509V3_set_nconf(ctx, conf) __X509V3_set_nconf(AmiSSLBase, (ctx), (conf))

void __X509V3_set_conf_lhash(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") LHASH * lhash)="\tjsr\t-13176(a6)";
#define X509V3_set_conf_lhash(ctx, lhash) __X509V3_set_conf_lhash(AmiSSLBase, (ctx), (lhash))

char * __X509V3_get_string(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") char * name, __reg("a2") char * section)="\tjsr\t-13182(a6)";
#define X509V3_get_string(ctx, name, section) __X509V3_get_string(AmiSSLBase, (ctx), (name), (section))

void * __X509V3_get_section(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") char * section)="\tjsr\t-13188(a6)";
#define X509V3_get_section(ctx, section) __X509V3_get_section(AmiSSLBase, (ctx), (section))

void __X509V3_string_free(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") char * str)="\tjsr\t-13194(a6)";
#define X509V3_string_free(ctx, str) __X509V3_string_free(AmiSSLBase, (ctx), (str))

void __X509V3_section_free(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") void * section)="\tjsr\t-13200(a6)";
#define X509V3_section_free(ctx, section) __X509V3_section_free(AmiSSLBase, (ctx), (section))

void __X509V3_set_ctx(__reg("a6") struct Library *, __reg("a0") X509V3_CTX * ctx, __reg("a1") X509 * issuer, __reg("a2") X509 * subject, __reg("a3") X509_REQ * req, __reg("d0") X509_CRL * crl, __reg("d1") LONG flags)="\tjsr\t-13206(a6)";
#define X509V3_set_ctx(ctx, issuer, subject, req, crl, flags) __X509V3_set_ctx(AmiSSLBase, (ctx), (issuer), (subject), (req), (crl), (flags))

int __X509V3_add_value(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("a1") const char * value, __reg("a2") void ** extlist)="\tjsr\t-13212(a6)";
#define X509V3_add_value(name, value, extlist) __X509V3_add_value(AmiSSLBase, (name), (value), (extlist))

int __X509V3_add_value_uchar(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("a1") const unsigned char * value, __reg("a2") void ** extlist)="\tjsr\t-13218(a6)";
#define X509V3_add_value_uchar(name, value, extlist) __X509V3_add_value_uchar(AmiSSLBase, (name), (value), (extlist))

int __X509V3_add_value_bool(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("d0") LONG asn1_bool, __reg("a1") void ** extlist)="\tjsr\t-13224(a6)";
#define X509V3_add_value_bool(name, asn1_bool, extlist) __X509V3_add_value_bool(AmiSSLBase, (name), (asn1_bool), (extlist))

int __X509V3_add_value_int(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("a1") ASN1_INTEGER * aint, __reg("a2") void ** extlist)="\tjsr\t-13230(a6)";
#define X509V3_add_value_int(name, aint, extlist) __X509V3_add_value_int(AmiSSLBase, (name), (aint), (extlist))

char * __i2s_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * meth, __reg("a1") ASN1_INTEGER * aint)="\tjsr\t-13236(a6)";
#define i2s_ASN1_INTEGER(meth, aint) __i2s_ASN1_INTEGER(AmiSSLBase, (meth), (aint))

ASN1_INTEGER * __s2i_ASN1_INTEGER(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * meth, __reg("a1") char * value)="\tjsr\t-13242(a6)";
#define s2i_ASN1_INTEGER(meth, value) __s2i_ASN1_INTEGER(AmiSSLBase, (meth), (value))

char * __i2s_ASN1_ENUMERATED(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * meth, __reg("a1") ASN1_ENUMERATED * aint)="\tjsr\t-13248(a6)";
#define i2s_ASN1_ENUMERATED(meth, aint) __i2s_ASN1_ENUMERATED(AmiSSLBase, (meth), (aint))

char * __i2s_ASN1_ENUMERATED_TABLE(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * meth, __reg("a1") ASN1_ENUMERATED * aint)="\tjsr\t-13254(a6)";
#define i2s_ASN1_ENUMERATED_TABLE(meth, aint) __i2s_ASN1_ENUMERATED_TABLE(AmiSSLBase, (meth), (aint))

int __X509V3_EXT_add(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * ext)="\tjsr\t-13260(a6)";
#define X509V3_EXT_add(ext) __X509V3_EXT_add(AmiSSLBase, (ext))

int __X509V3_EXT_add_list(__reg("a6") struct Library *, __reg("a0") X509V3_EXT_METHOD * extlist)="\tjsr\t-13266(a6)";
#define X509V3_EXT_add_list(extlist) __X509V3_EXT_add_list(AmiSSLBase, (extlist))

int __X509V3_EXT_add_alias(__reg("a6") struct Library *, __reg("d0") LONG nid_to, __reg("d1") LONG nid_from)="\tjsr\t-13272(a6)";
#define X509V3_EXT_add_alias(nid_to, nid_from) __X509V3_EXT_add_alias(AmiSSLBase, (nid_to), (nid_from))

void __X509V3_EXT_cleanup(__reg("a6") struct Library *)="\tjsr\t-13278(a6)";
#define X509V3_EXT_cleanup() __X509V3_EXT_cleanup(AmiSSLBase)

X509V3_EXT_METHOD * __X509V3_EXT_get(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ext)="\tjsr\t-13284(a6)";
#define X509V3_EXT_get(ext) __X509V3_EXT_get(AmiSSLBase, (ext))

X509V3_EXT_METHOD * __X509V3_EXT_get_nid(__reg("a6") struct Library *, __reg("d0") LONG nid)="\tjsr\t-13290(a6)";
#define X509V3_EXT_get_nid(nid) __X509V3_EXT_get_nid(AmiSSLBase, (nid))

int __X509V3_add_standard_extensions(__reg("a6") struct Library *)="\tjsr\t-13296(a6)";
#define X509V3_add_standard_extensions() __X509V3_add_standard_extensions(AmiSSLBase)

void * __X509V3_parse_list(__reg("a6") struct Library *, __reg("a0") const char * line)="\tjsr\t-13302(a6)";
#define X509V3_parse_list(line) __X509V3_parse_list(AmiSSLBase, (line))

void * __X509V3_EXT_d2i(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ext)="\tjsr\t-13308(a6)";
#define X509V3_EXT_d2i(ext) __X509V3_EXT_d2i(AmiSSLBase, (ext))

void * __X509V3_get_d2i(__reg("a6") struct Library *, __reg("a0") void * x, __reg("d0") LONG nid, __reg("a1") int * crit, __reg("a2") int * idx)="\tjsr\t-13314(a6)";
#define X509V3_get_d2i(x, nid, crit, idx) __X509V3_get_d2i(AmiSSLBase, (x), (nid), (crit), (idx))

X509_EXTENSION * __X509V3_EXT_i2d(__reg("a6") struct Library *, __reg("d0") LONG ext_nid, __reg("d1") LONG crit, __reg("a0") void * ext_struc)="\tjsr\t-13320(a6)";
#define X509V3_EXT_i2d(ext_nid, crit, ext_struc) __X509V3_EXT_i2d(AmiSSLBase, (ext_nid), (crit), (ext_struc))

int __X509V3_add1_i2d(__reg("a6") struct Library *, __reg("a0") void ** x, __reg("d0") LONG nid, __reg("a1") void * value, __reg("d1") LONG crit, __reg("d2") unsigned long flags)="\tjsr\t-13326(a6)";
#define X509V3_add1_i2d(x, nid, value, crit, flags) __X509V3_add1_i2d(AmiSSLBase, (x), (nid), (value), (crit), (flags))

char * __hex_to_string(__reg("a6") struct Library *, __reg("a0") unsigned char * buffer, __reg("d0") long len)="\tjsr\t-13332(a6)";
#define hex_to_string(buffer, len) __hex_to_string(AmiSSLBase, (buffer), (len))

unsigned char * __string_to_hex(__reg("a6") struct Library *, __reg("a0") char * str, __reg("a1") long * len)="\tjsr\t-13338(a6)";
#define string_to_hex(str, len) __string_to_hex(AmiSSLBase, (str), (len))

int __name_cmp(__reg("a6") struct Library *, __reg("a0") const char * name, __reg("a1") const char * cmp)="\tjsr\t-13344(a6)";
#define name_cmp(name, cmp) __name_cmp(AmiSSLBase, (name), (cmp))

void __X509V3_EXT_val_prn(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") void * val, __reg("d0") LONG indent, __reg("d1") LONG ml)="\tjsr\t-13350(a6)";
#define X509V3_EXT_val_prn(out, val, indent, ml) __X509V3_EXT_val_prn(AmiSSLBase, (out), (val), (indent), (ml))

int __X509V3_EXT_print(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") X509_EXTENSION * ext, __reg("d0") unsigned long flag, __reg("d1") LONG indent)="\tjsr\t-13356(a6)";
#define X509V3_EXT_print(out, ext, flag, indent) __X509V3_EXT_print(AmiSSLBase, (out), (ext), (flag), (indent))

int __X509V3_extensions_print(__reg("a6") struct Library *, __reg("a0") BIO * out, __reg("a1") char * title, __reg("a2") void * exts, __reg("d0") unsigned long flag, __reg("d1") LONG indent)="\tjsr\t-13362(a6)";
#define X509V3_extensions_print(out, title, exts, flag, indent) __X509V3_extensions_print(AmiSSLBase, (out), (title), (exts), (flag), (indent))

int __X509_check_purpose(__reg("a6") struct Library *, __reg("a0") X509 * x, __reg("d0") LONG id, __reg("d1") LONG ca)="\tjsr\t-13368(a6)";
#define X509_check_purpose(x, id, ca) __X509_check_purpose(AmiSSLBase, (x), (id), (ca))

int __X509_supported_extension(__reg("a6") struct Library *, __reg("a0") X509_EXTENSION * ex)="\tjsr\t-13374(a6)";
#define X509_supported_extension(ex) __X509_supported_extension(AmiSSLBase, (ex))

int __X509_PURPOSE_set(__reg("a6") struct Library *, __reg("a0") int * p, __reg("d0") LONG purpose)="\tjsr\t-13380(a6)";
#define X509_PURPOSE_set(p, purpose) __X509_PURPOSE_set(AmiSSLBase, (p), (purpose))

int __X509_check_issued(__reg("a6") struct Library *, __reg("a0") X509 * issuer, __reg("a1") X509 * subject)="\tjsr\t-13386(a6)";
#define X509_check_issued(issuer, subject) __X509_check_issued(AmiSSLBase, (issuer), (subject))

int __X509_PURPOSE_get_count(__reg("a6") struct Library *)="\tjsr\t-13392(a6)";
#define X509_PURPOSE_get_count() __X509_PURPOSE_get_count(AmiSSLBase)

X509_PURPOSE * __X509_PURPOSE_get0(__reg("a6") struct Library *, __reg("d0") LONG idx)="\tjsr\t-13398(a6)";
#define X509_PURPOSE_get0(idx) __X509_PURPOSE_get0(AmiSSLBase, (idx))

int __X509_PURPOSE_get_by_sname(__reg("a6") struct Library *, __reg("a0") char * sname)="\tjsr\t-13404(a6)";
#define X509_PURPOSE_get_by_sname(sname) __X509_PURPOSE_get_by_sname(AmiSSLBase, (sname))

int __X509_PURPOSE_get_by_id(__reg("a6") struct Library *, __reg("d0") LONG id)="\tjsr\t-13410(a6)";
#define X509_PURPOSE_get_by_id(id) __X509_PURPOSE_get_by_id(AmiSSLBase, (id))

int __X509_PURPOSE_add(__reg("a6") struct Library *, __reg("d0") LONG id, __reg("d1") LONG trust, __reg("d2") LONG flags, __reg("a0") int (*ck)(const X509_PURPOSE *, const X509 *, int), __reg("a1") char * name, __reg("a2") char * sname, __reg("a3") void * arg)="\tjsr\t-13416(a6)";
#define X509_PURPOSE_add(id, trust, flags, ck, name, sname, arg) __X509_PURPOSE_add(AmiSSLBase, (id), (trust), (flags), (ck), (name), (sname), (arg))

char * __X509_PURPOSE_get0_name(__reg("a6") struct Library *, __reg("a0") X509_PURPOSE * xp)="\tjsr\t-13422(a6)";
#define X509_PURPOSE_get0_name(xp) __X509_PURPOSE_get0_name(AmiSSLBase, (xp))

char * __X509_PURPOSE_get0_sname(__reg("a6") struct Library *, __reg("a0") X509_PURPOSE * xp)="\tjsr\t-13428(a6)";
#define X509_PURPOSE_get0_sname(xp) __X509_PURPOSE_get0_sname(AmiSSLBase, (xp))

int __X509_PURPOSE_get_trust(__reg("a6") struct Library *, __reg("a0") X509_PURPOSE * xp)="\tjsr\t-13434(a6)";
#define X509_PURPOSE_get_trust(xp) __X509_PURPOSE_get_trust(AmiSSLBase, (xp))

void __X509_PURPOSE_cleanup(__reg("a6") struct Library *)="\tjsr\t-13440(a6)";
#define X509_PURPOSE_cleanup() __X509_PURPOSE_cleanup(AmiSSLBase)

int __X509_PURPOSE_get_id(__reg("a6") struct Library *, __reg("a0") X509_PURPOSE * a)="\tjsr\t-13446(a6)";
#define X509_PURPOSE_get_id(a) __X509_PURPOSE_get_id(AmiSSLBase, (a))

STACK * __X509_get1_email(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-13452(a6)";
#define X509_get1_email(x) __X509_get1_email(AmiSSLBase, (x))

STACK * __X509_REQ_get1_email(__reg("a6") struct Library *, __reg("a0") X509_REQ * x)="\tjsr\t-13458(a6)";
#define X509_REQ_get1_email(x) __X509_REQ_get1_email(AmiSSLBase, (x))

void __X509_email_free(__reg("a6") struct Library *, __reg("a0") STACK * sk)="\tjsr\t-13464(a6)";
#define X509_email_free(sk) __X509_email_free(AmiSSLBase, (sk))

void __ERR_load_X509V3_strings(__reg("a6") struct Library *)="\tjsr\t-13470(a6)";
#define ERR_load_X509V3_strings() __ERR_load_X509V3_strings(AmiSSLBase)

const char * __AES_options(__reg("a6") struct Library *)="\tjsr\t-13476(a6)";
#define AES_options() __AES_options(AmiSSLBase)

int __AES_set_encrypt_key(__reg("a6") struct Library *, __reg("a0") const unsigned char * userKey, __reg("d0") LONG bits, __reg("a1") AES_KEY * key)="\tjsr\t-13482(a6)";
#define AES_set_encrypt_key(userKey, bits, key) __AES_set_encrypt_key(AmiSSLBase, (userKey), (bits), (key))

int __AES_set_decrypt_key(__reg("a6") struct Library *, __reg("a0") const unsigned char * userKey, __reg("d0") LONG bits, __reg("a1") AES_KEY * key)="\tjsr\t-13488(a6)";
#define AES_set_decrypt_key(userKey, bits, key) __AES_set_decrypt_key(AmiSSLBase, (userKey), (bits), (key))

void __AES_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") const AES_KEY * key)="\tjsr\t-13494(a6)";
#define AES_encrypt(in, out, key) __AES_encrypt(AmiSSLBase, (in), (out), (key))

void __AES_decrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") const AES_KEY * key)="\tjsr\t-13500(a6)";
#define AES_decrypt(in, out, key) __AES_decrypt(AmiSSLBase, (in), (out), (key))

void __AES_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") const AES_KEY * key, __reg("d0") LONG enc)="\tjsr\t-13506(a6)";
#define AES_ecb_encrypt(in, out, key, enc) __AES_ecb_encrypt(AmiSSLBase, (in), (out), (key), (enc))

void __AES_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") LONG enc)="\tjsr\t-13512(a6)";
#define AES_cbc_encrypt(in, out, length, key, ivec, enc) __AES_cbc_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (enc))

void __AES_cfb128_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13518(a6)";
#define AES_cfb128_encrypt(in, out, length, key, ivec, num, enc) __AES_cfb128_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (num), (enc))

void __AES_cfb1_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13524(a6)";
#define AES_cfb1_encrypt(in, out, length, key, ivec, num, enc) __AES_cfb1_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (num), (enc))

void __AES_cfb8_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13530(a6)";
#define AES_cfb8_encrypt(in, out, length, key, ivec, num, enc) __AES_cfb8_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (num), (enc))

void __AES_cfbr_encrypt_block(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG nbits, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") LONG enc)="\tjsr\t-13536(a6)";
#define AES_cfbr_encrypt_block(in, out, nbits, key, ivec, enc) __AES_cfbr_encrypt_block(AmiSSLBase, (in), (out), (nbits), (key), (ivec), (enc))

void __AES_ofb128_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("a3") unsigned char * ivec, __reg("d1") int * num)="\tjsr\t-13542(a6)";
#define AES_ofb128_encrypt(in, out, length, key, ivec, num) __AES_ofb128_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (num))

void __AES_ctr128_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") const unsigned long length, __reg("a2") const AES_KEY * key, __reg("d1") ULONG ivec, __reg("d2") ULONG ecount_buf, __reg("a3") unsigned int * num)="\tjsr\t-13548(a6)";
#define AES_ctr128_encrypt(in, out, length, key, ivec, ecount_buf, num) __AES_ctr128_encrypt(AmiSSLBase, (in), (out), (length), (key), (ivec), (ecount_buf), (num))

void __BF_set_key(__reg("a6") struct Library *, __reg("a0") BF_KEY * key, __reg("d0") LONG len, __reg("a1") const unsigned char * data)="\tjsr\t-13554(a6)";
#define BF_set_key(key, len, data) __BF_set_key(AmiSSLBase, (key), (len), (data))

void __BF_encrypt(__reg("a6") struct Library *, __reg("a0") BF_LONG * data, __reg("a1") const BF_KEY * key)="\tjsr\t-13560(a6)";
#define BF_encrypt(data, key) __BF_encrypt(AmiSSLBase, (data), (key))

void __BF_decrypt(__reg("a6") struct Library *, __reg("a0") BF_LONG * data, __reg("a1") const BF_KEY * key)="\tjsr\t-13566(a6)";
#define BF_decrypt(data, key) __BF_decrypt(AmiSSLBase, (data), (key))

void __BF_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") const BF_KEY * key, __reg("d0") LONG enc)="\tjsr\t-13572(a6)";
#define BF_ecb_encrypt(in, out, key, enc) __BF_ecb_encrypt(AmiSSLBase, (in), (out), (key), (enc))

void __BF_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") const BF_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") LONG enc)="\tjsr\t-13578(a6)";
#define BF_cbc_encrypt(in, out, length, schedule, ivec, enc) __BF_cbc_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (enc))

void __BF_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") const BF_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13584(a6)";
#define BF_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) __BF_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void __BF_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") const BF_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num)="\tjsr\t-13590(a6)";
#define BF_ofb64_encrypt(in, out, length, schedule, ivec, num) __BF_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

const char * __BF_options(__reg("a6") struct Library *)="\tjsr\t-13596(a6)";
#define BF_options() __BF_options(AmiSSLBase)

void __CAST_set_key(__reg("a6") struct Library *, __reg("a0") CAST_KEY * key, __reg("d0") LONG len, __reg("a1") const unsigned char * data)="\tjsr\t-13602(a6)";
#define CAST_set_key(key, len, data) __CAST_set_key(AmiSSLBase, (key), (len), (data))

void __CAST_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") CAST_KEY * key, __reg("d0") LONG enc)="\tjsr\t-13608(a6)";
#define CAST_ecb_encrypt(in, out, key, enc) __CAST_ecb_encrypt(AmiSSLBase, (in), (out), (key), (enc))

void __CAST_encrypt(__reg("a6") struct Library *, __reg("a0") CAST_LONG * data, __reg("a1") CAST_KEY * key)="\tjsr\t-13614(a6)";
#define CAST_encrypt(data, key) __CAST_encrypt(AmiSSLBase, (data), (key))

void __CAST_decrypt(__reg("a6") struct Library *, __reg("a0") CAST_LONG * data, __reg("a1") CAST_KEY * key)="\tjsr\t-13620(a6)";
#define CAST_decrypt(data, key) __CAST_decrypt(AmiSSLBase, (data), (key))

void __CAST_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") CAST_KEY * ks, __reg("a3") unsigned char * iv, __reg("d1") LONG enc)="\tjsr\t-13626(a6)";
#define CAST_cbc_encrypt(in, out, length, ks, iv, enc) __CAST_cbc_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (enc))

void __CAST_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") CAST_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13632(a6)";
#define CAST_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) __CAST_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void __CAST_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") CAST_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num)="\tjsr\t-13638(a6)";
#define CAST_ofb64_encrypt(in, out, length, schedule, ivec, num) __CAST_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

int * ___shadow_DES_check_key(__reg("a6") struct Library *)="\tjsr\t-13644(a6)";
#define _shadow_DES_check_key() ___shadow_DES_check_key(AmiSSLBase)

int * ___shadow_DES_rw_mode(__reg("a6") struct Library *)="\tjsr\t-13650(a6)";
#define _shadow_DES_rw_mode() ___shadow_DES_rw_mode(AmiSSLBase)

const char * __DES_options(__reg("a6") struct Library *)="\tjsr\t-13656(a6)";
#define DES_options() __DES_options(AmiSSLBase)

void __DES_ecb3_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d0") DES_key_schedule * ks3, __reg("d1") LONG enc)="\tjsr\t-13662(a6)";
#define DES_ecb3_encrypt(input, output, ks1, ks2, ks3, enc) __DES_ecb3_encrypt(AmiSSLBase, (input), (output), (ks1), (ks2), (ks3), (enc))

DES_LONG __DES_cbc_cksum(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") DES_cblock * output, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") const DES_cblock * ivec)="\tjsr\t-13668(a6)";
#define DES_cbc_cksum(input, output, length, schedule, ivec) __DES_cbc_cksum(AmiSSLBase, (input), (output), (length), (schedule), (ivec))

void __DES_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") LONG enc)="\tjsr\t-13674(a6)";
#define DES_cbc_encrypt(input, output, length, schedule, ivec, enc) __DES_cbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

void __DES_ncbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") LONG enc)="\tjsr\t-13680(a6)";
#define DES_ncbc_encrypt(input, output, length, schedule, ivec, enc) __DES_ncbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

void __DES_xcbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") const DES_cblock * inw, __reg("d2") const DES_cblock * outw, __reg("d3") LONG enc)="\tjsr\t-13686(a6)";
#define DES_xcbc_encrypt(input, output, length, schedule, ivec, inw, outw, enc) __DES_xcbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (inw), (outw), (enc))

void __DES_cfb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG numbits, __reg("d1") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d2") LONG enc)="\tjsr\t-13692(a6)";
#define DES_cfb_encrypt(in, out, numbits, length, schedule, ivec, enc) __DES_cfb_encrypt(AmiSSLBase, (in), (out), (numbits), (length), (schedule), (ivec), (enc))

void __DES_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const DES_cblock * input, __reg("a1") DES_cblock * output, __reg("a2") DES_key_schedule * ks, __reg("d0") LONG enc)="\tjsr\t-13698(a6)";
#define DES_ecb_encrypt(input, output, ks, enc) __DES_ecb_encrypt(AmiSSLBase, (input), (output), (ks), (enc))

void __DES_encrypt1(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("a1") DES_key_schedule * ks, __reg("d0") LONG enc)="\tjsr\t-13704(a6)";
#define DES_encrypt1(data, ks, enc) __DES_encrypt1(AmiSSLBase, (data), (ks), (enc))

void __DES_encrypt2(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("a1") DES_key_schedule * ks, __reg("d0") LONG enc)="\tjsr\t-13710(a6)";
#define DES_encrypt2(data, ks, enc) __DES_encrypt2(AmiSSLBase, (data), (ks), (enc))

void __DES_encrypt3(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("a1") DES_key_schedule * ks1, __reg("a2") DES_key_schedule * ks2, __reg("a3") DES_key_schedule * ks3)="\tjsr\t-13716(a6)";
#define DES_encrypt3(data, ks1, ks2, ks3) __DES_encrypt3(AmiSSLBase, (data), (ks1), (ks2), (ks3))

void __DES_decrypt3(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("a1") DES_key_schedule * ks1, __reg("a2") DES_key_schedule * ks2, __reg("a3") DES_key_schedule * ks3)="\tjsr\t-13722(a6)";
#define DES_decrypt3(data, ks1, ks2, ks3) __DES_decrypt3(AmiSSLBase, (data), (ks1), (ks2), (ks3))

void __DES_ede3_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("d0") long length, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d1") DES_key_schedule * ks3, __reg("d2") DES_cblock * ivec, __reg("d3") LONG enc)="\tjsr\t-13728(a6)";
#define DES_ede3_cbc_encrypt(input, output, length, ks1, ks2, ks3, ivec, enc) __DES_ede3_cbc_encrypt(AmiSSLBase, (input), (output), (length), (ks1), (ks2), (ks3), (ivec), (enc))

void __DES_ede3_cbcm_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d1") DES_key_schedule * ks3, __reg("d2") DES_cblock * ivec1, __reg("d3") DES_cblock * ivec2, __reg("d4") LONG enc)="\tjsr\t-13734(a6)";
#define DES_ede3_cbcm_encrypt(in, out, length, ks1, ks2, ks3, ivec1, ivec2, enc) __DES_ede3_cbcm_encrypt(AmiSSLBase, (in), (out), (length), (ks1), (ks2), (ks3), (ivec1), (ivec2), (enc))

void __DES_ede3_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d1") DES_key_schedule * ks3, __reg("d2") DES_cblock * ivec, __reg("d3") int * num, __reg("d4") LONG enc)="\tjsr\t-13740(a6)";
#define DES_ede3_cfb64_encrypt(in, out, length, ks1, ks2, ks3, ivec, num, enc) __DES_ede3_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (ks1), (ks2), (ks3), (ivec), (num), (enc))

void __DES_ede3_cfb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG numbits, __reg("d1") long length, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d2") DES_key_schedule * ks3, __reg("d3") DES_cblock * ivec, __reg("d4") LONG enc)="\tjsr\t-13746(a6)";
#define DES_ede3_cfb_encrypt(in, out, numbits, length, ks1, ks2, ks3, ivec, enc) __DES_ede3_cfb_encrypt(AmiSSLBase, (in), (out), (numbits), (length), (ks1), (ks2), (ks3), (ivec), (enc))

void __DES_ede3_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") DES_key_schedule * ks1, __reg("a3") DES_key_schedule * ks2, __reg("d1") DES_key_schedule * ks3, __reg("d2") DES_cblock * ivec, __reg("d3") int * num)="\tjsr\t-13752(a6)";
#define DES_ede3_ofb64_encrypt(in, out, length, ks1, ks2, ks3, ivec, num) __DES_ede3_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (ks1), (ks2), (ks3), (ivec), (num))

void __DES_xwhite_in2out(__reg("a6") struct Library *, __reg("a0") const DES_cblock * DES_key, __reg("a1") const DES_cblock * in_white, __reg("a2") DES_cblock * out_white)="\tjsr\t-13758(a6)";
#define DES_xwhite_in2out(DES_key, in_white, out_white) __DES_xwhite_in2out(AmiSSLBase, (DES_key), (in_white), (out_white))

int __DES_enc_read(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("a0") void * buf, __reg("d1") LONG len, __reg("a1") DES_key_schedule * sched, __reg("a2") DES_cblock * iv)="\tjsr\t-13764(a6)";
#define DES_enc_read(fd, buf, len, sched, iv) __DES_enc_read(AmiSSLBase, (fd), (buf), (len), (sched), (iv))

int __DES_enc_write(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("a0") const void * buf, __reg("d1") LONG len, __reg("a1") DES_key_schedule * sched, __reg("a2") DES_cblock * iv)="\tjsr\t-13770(a6)";
#define DES_enc_write(fd, buf, len, sched, iv) __DES_enc_write(AmiSSLBase, (fd), (buf), (len), (sched), (iv))

char * __DES_fcrypt(__reg("a6") struct Library *, __reg("a0") const char * buf, __reg("a1") const char * salt, __reg("a2") char * ret)="\tjsr\t-13776(a6)";
#define DES_fcrypt(buf, salt, ret) __DES_fcrypt(AmiSSLBase, (buf), (salt), (ret))

char * __DES_crypt(__reg("a6") struct Library *, __reg("a0") const char * buf, __reg("a1") const char * salt)="\tjsr\t-13782(a6)";
#define DES_crypt(buf, salt) __DES_crypt(AmiSSLBase, (buf), (salt))

void __DES_ofb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG numbits, __reg("d1") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec)="\tjsr\t-13788(a6)";
#define DES_ofb_encrypt(in, out, numbits, length, schedule, ivec) __DES_ofb_encrypt(AmiSSLBase, (in), (out), (numbits), (length), (schedule), (ivec))

void __DES_pcbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") unsigned char * output, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") LONG enc)="\tjsr\t-13794(a6)";
#define DES_pcbc_encrypt(input, output, length, schedule, ivec, enc) __DES_pcbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

DES_LONG __DES_quad_cksum(__reg("a6") struct Library *, __reg("a0") const unsigned char * input, __reg("a1") DES_cblock * output, __reg("d0") long length, __reg("d1") LONG out_count, __reg("a2") DES_cblock * seed)="\tjsr\t-13800(a6)";
#define DES_quad_cksum(input, output, length, out_count, seed) __DES_quad_cksum(AmiSSLBase, (input), (output), (length), (out_count), (seed))

int __DES_random_key(__reg("a6") struct Library *, __reg("a0") DES_cblock * ret)="\tjsr\t-13806(a6)";
#define DES_random_key(ret) __DES_random_key(AmiSSLBase, (ret))

void __DES_set_odd_parity(__reg("a6") struct Library *, __reg("a0") DES_cblock * key)="\tjsr\t-13812(a6)";
#define DES_set_odd_parity(key) __DES_set_odd_parity(AmiSSLBase, (key))

int __DES_check_key_parity(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key)="\tjsr\t-13818(a6)";
#define DES_check_key_parity(key) __DES_check_key_parity(AmiSSLBase, (key))

int __DES_is_weak_key(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key)="\tjsr\t-13824(a6)";
#define DES_is_weak_key(key) __DES_is_weak_key(AmiSSLBase, (key))

int __DES_set_key(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key, __reg("a1") DES_key_schedule * schedule)="\tjsr\t-13830(a6)";
#define DES_set_key(key, schedule) __DES_set_key(AmiSSLBase, (key), (schedule))

int __DES_key_sched(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key, __reg("a1") DES_key_schedule * schedule)="\tjsr\t-13836(a6)";
#define DES_key_sched(key, schedule) __DES_key_sched(AmiSSLBase, (key), (schedule))

int __DES_set_key_checked(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key, __reg("a1") DES_key_schedule * schedule)="\tjsr\t-13842(a6)";
#define DES_set_key_checked(key, schedule) __DES_set_key_checked(AmiSSLBase, (key), (schedule))

void __DES_set_key_unchecked(__reg("a6") struct Library *, __reg("a0") const DES_cblock * key, __reg("a1") DES_key_schedule * schedule)="\tjsr\t-13848(a6)";
#define DES_set_key_unchecked(key, schedule) __DES_set_key_unchecked(AmiSSLBase, (key), (schedule))

void __DES_string_to_key(__reg("a6") struct Library *, __reg("a0") const char * str, __reg("a1") DES_cblock * key)="\tjsr\t-13854(a6)";
#define DES_string_to_key(str, key) __DES_string_to_key(AmiSSLBase, (str), (key))

void __DES_string_to_2keys(__reg("a6") struct Library *, __reg("a0") const char * str, __reg("a1") DES_cblock * key1, __reg("a2") DES_cblock * key2)="\tjsr\t-13860(a6)";
#define DES_string_to_2keys(str, key1, key2) __DES_string_to_2keys(AmiSSLBase, (str), (key1), (key2))

void __DES_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-13866(a6)";
#define DES_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) __DES_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void __DES_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") DES_key_schedule * schedule, __reg("a3") DES_cblock * ivec, __reg("d1") int * num)="\tjsr\t-13872(a6)";
#define DES_ofb64_encrypt(in, out, length, schedule, ivec, num) __DES_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

int __DES_read_password(__reg("a6") struct Library *, __reg("a0") DES_cblock * key, __reg("a1") const char * prompt, __reg("d0") LONG verify)="\tjsr\t-13878(a6)";
#define DES_read_password(key, prompt, verify) __DES_read_password(AmiSSLBase, (key), (prompt), (verify))

int __DES_read_2passwords(__reg("a6") struct Library *, __reg("a0") DES_cblock * key1, __reg("a1") DES_cblock * key2, __reg("a2") const char * prompt, __reg("d0") LONG verify)="\tjsr\t-13884(a6)";
#define DES_read_2passwords(key1, key2, prompt, verify) __DES_read_2passwords(AmiSSLBase, (key1), (key2), (prompt), (verify))

const char * ___ossl_old_des_options(__reg("a6") struct Library *)="\tjsr\t-13890(a6)";
#define _ossl_old_des_options() ___ossl_old_des_options(AmiSSLBase)

void ___ossl_old_des_ecb3_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") LONG ks1, __reg("d1") LONG ks2, __reg("d2") LONG ks3, __reg("d3") LONG enc)="\tjsr\t-13896(a6)";
#define _ossl_old_des_ecb3_encrypt(input, output, ks1, ks2, ks3, enc) ___ossl_old_des_ecb3_encrypt(AmiSSLBase, (input), (output), (ks1), (ks2), (ks3), (enc))

DES_LONG ___ossl_old_des_cbc_cksum(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec)="\tjsr\t-13902(a6)";
#define _ossl_old_des_cbc_cksum(input, output, length, schedule, ivec) ___ossl_old_des_cbc_cksum(AmiSSLBase, (input), (output), (length), (schedule), (ivec))

void ___ossl_old_des_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("d2") LONG enc)="\tjsr\t-13908(a6)";
#define _ossl_old_des_cbc_encrypt(input, output, length, schedule, ivec, enc) ___ossl_old_des_cbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

void ___ossl_old_des_ncbc_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("d2") LONG enc)="\tjsr\t-13914(a6)";
#define _ossl_old_des_ncbc_encrypt(input, output, length, schedule, ivec, enc) ___ossl_old_des_ncbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

void ___ossl_old_des_xcbc_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("a3") _ossl_old_des_cblock * inw, __reg("d2") _ossl_old_des_cblock * outw, __reg("d3") LONG enc)="\tjsr\t-13920(a6)";
#define _ossl_old_des_xcbc_encrypt(input, output, length, schedule, ivec, inw, outw, enc) ___ossl_old_des_xcbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (inw), (outw), (enc))

void ___ossl_old_des_cfb_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG numbits, __reg("d1") long length, __reg("d2") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("d3") LONG enc)="\tjsr\t-13926(a6)";
#define _ossl_old_des_cfb_encrypt(in, out, numbits, length, schedule, ivec, enc) ___ossl_old_des_cfb_encrypt(AmiSSLBase, (in), (out), (numbits), (length), (schedule), (ivec), (enc))

void ___ossl_old_des_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") LONG ks, __reg("d1") LONG enc)="\tjsr\t-13932(a6)";
#define _ossl_old_des_ecb_encrypt(input, output, ks, enc) ___ossl_old_des_ecb_encrypt(AmiSSLBase, (input), (output), (ks), (enc))

void ___ossl_old_des_encrypt(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("d0") LONG ks, __reg("d1") LONG enc)="\tjsr\t-13938(a6)";
#define _ossl_old_des_encrypt(data, ks, enc) ___ossl_old_des_encrypt(AmiSSLBase, (data), (ks), (enc))

void ___ossl_old_des_encrypt2(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("d0") LONG ks, __reg("d1") LONG enc)="\tjsr\t-13944(a6)";
#define _ossl_old_des_encrypt2(data, ks, enc) ___ossl_old_des_encrypt2(AmiSSLBase, (data), (ks), (enc))

void ___ossl_old_des_encrypt3(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("d0") LONG ks1, __reg("d1") LONG ks2, __reg("d2") LONG ks3)="\tjsr\t-13950(a6)";
#define _ossl_old_des_encrypt3(data, ks1, ks2, ks3) ___ossl_old_des_encrypt3(AmiSSLBase, (data), (ks1), (ks2), (ks3))

void ___ossl_old_des_decrypt3(__reg("a6") struct Library *, __reg("a0") DES_LONG * data, __reg("d0") LONG ks1, __reg("d1") LONG ks2, __reg("d2") LONG ks3)="\tjsr\t-13956(a6)";
#define _ossl_old_des_decrypt3(data, ks1, ks2, ks3) ___ossl_old_des_decrypt3(AmiSSLBase, (data), (ks1), (ks2), (ks3))

void ___ossl_old_des_ede3_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG ks1, __reg("d2") LONG ks2, __reg("d3") LONG ks3, __reg("a2") _ossl_old_des_cblock * ivec, __reg("d4") LONG enc)="\tjsr\t-13962(a6)";
#define _ossl_old_des_ede3_cbc_encrypt(input, output, length, ks1, ks2, ks3, ivec, enc) ___ossl_old_des_ede3_cbc_encrypt(AmiSSLBase, (input), (output), (length), (ks1), (ks2), (ks3), (ivec), (enc))

void ___ossl_old_des_ede3_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("d1") LONG ks1, __reg("d2") LONG ks2, __reg("d3") LONG ks3, __reg("a2") _ossl_old_des_cblock * ivec, __reg("a3") int * num, __reg("d4") LONG enc)="\tjsr\t-13968(a6)";
#define _ossl_old_des_ede3_cfb64_encrypt(in, out, length, ks1, ks2, ks3, ivec, num, enc) ___ossl_old_des_ede3_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (ks1), (ks2), (ks3), (ivec), (num), (enc))

void ___ossl_old_des_ede3_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("d1") LONG ks1, __reg("d2") LONG ks2, __reg("d3") LONG ks3, __reg("a2") _ossl_old_des_cblock * ivec, __reg("a3") int * num)="\tjsr\t-13974(a6)";
#define _ossl_old_des_ede3_ofb64_encrypt(in, out, length, ks1, ks2, ks3, ivec, num) ___ossl_old_des_ede3_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (ks1), (ks2), (ks3), (ivec), (num))

void ___ossl_old_des_xwhite_in2out(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * des_key, __reg("a1") _ossl_old_des_cblock * in_white, __reg("a2") _ossl_old_des_cblock * out_white)="\tjsr\t-13980(a6)";
#define _ossl_old_des_xwhite_in2out(des_key, in_white, out_white) ___ossl_old_des_xwhite_in2out(AmiSSLBase, (des_key), (in_white), (out_white))

int ___ossl_old_des_enc_read(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("a0") char * buf, __reg("d1") LONG len, __reg("d2") LONG sched, __reg("a1") _ossl_old_des_cblock * iv)="\tjsr\t-13986(a6)";
#define _ossl_old_des_enc_read(fd, buf, len, sched, iv) ___ossl_old_des_enc_read(AmiSSLBase, (fd), (buf), (len), (sched), (iv))

int ___ossl_old_des_enc_write(__reg("a6") struct Library *, __reg("d0") LONG fd, __reg("a0") char * buf, __reg("d1") LONG len, __reg("d2") LONG sched, __reg("a1") _ossl_old_des_cblock * iv)="\tjsr\t-13992(a6)";
#define _ossl_old_des_enc_write(fd, buf, len, sched, iv) ___ossl_old_des_enc_write(AmiSSLBase, (fd), (buf), (len), (sched), (iv))

char * ___ossl_old_des_fcrypt(__reg("a6") struct Library *, __reg("a0") const char * buf, __reg("a1") const char * salt, __reg("a2") char * ret)="\tjsr\t-13998(a6)";
#define _ossl_old_des_fcrypt(buf, salt, ret) ___ossl_old_des_fcrypt(AmiSSLBase, (buf), (salt), (ret))

char * ___ossl_old_des_crypt(__reg("a6") struct Library *, __reg("a0") const char * buf, __reg("a1") const char * salt)="\tjsr\t-14004(a6)";
#define _ossl_old_des_crypt(buf, salt) ___ossl_old_des_crypt(AmiSSLBase, (buf), (salt))

char * ___ossl_old_crypt(__reg("a6") struct Library *, __reg("a0") const char * buf, __reg("a1") const char * salt)="\tjsr\t-14010(a6)";
#define _ossl_old_crypt(buf, salt) ___ossl_old_crypt(AmiSSLBase, (buf), (salt))

void ___ossl_old_des_ofb_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") LONG numbits, __reg("d1") long length, __reg("d2") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec)="\tjsr\t-14016(a6)";
#define _ossl_old_des_ofb_encrypt(in, out, numbits, length, schedule, ivec) ___ossl_old_des_ofb_encrypt(AmiSSLBase, (in), (out), (numbits), (length), (schedule), (ivec))

void ___ossl_old_des_pcbc_encrypt(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("d2") LONG enc)="\tjsr\t-14022(a6)";
#define _ossl_old_des_pcbc_encrypt(input, output, length, schedule, ivec, enc) ___ossl_old_des_pcbc_encrypt(AmiSSLBase, (input), (output), (length), (schedule), (ivec), (enc))

DES_LONG ___ossl_old_des_quad_cksum(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * input, __reg("a1") _ossl_old_des_cblock * output, __reg("d0") long length, __reg("d1") LONG out_count, __reg("a2") _ossl_old_des_cblock * seed)="\tjsr\t-14028(a6)";
#define _ossl_old_des_quad_cksum(input, output, length, out_count, seed) ___ossl_old_des_quad_cksum(AmiSSLBase, (input), (output), (length), (out_count), (seed))

void ___ossl_old_des_random_seed(__reg("a6") struct Library *, __reg("d0") LONG key)="\tjsr\t-14034(a6)";
#define _ossl_old_des_random_seed(key) ___ossl_old_des_random_seed(AmiSSLBase, (key))

void ___ossl_old_des_random_key(__reg("a6") struct Library *, __reg("d0") LONG ret)="\tjsr\t-14040(a6)";
#define _ossl_old_des_random_key(ret) ___ossl_old_des_random_key(AmiSSLBase, (ret))

int ___ossl_old_des_read_password(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key, __reg("a1") const char * prompt, __reg("d0") LONG verify)="\tjsr\t-14046(a6)";
#define _ossl_old_des_read_password(key, prompt, verify) ___ossl_old_des_read_password(AmiSSLBase, (key), (prompt), (verify))

int ___ossl_old_des_read_2passwords(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key1, __reg("a1") _ossl_old_des_cblock * key2, __reg("a2") const char * prompt, __reg("d0") LONG verify)="\tjsr\t-14052(a6)";
#define _ossl_old_des_read_2passwords(key1, key2, prompt, verify) ___ossl_old_des_read_2passwords(AmiSSLBase, (key1), (key2), (prompt), (verify))

void ___ossl_old_des_set_odd_parity(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key)="\tjsr\t-14058(a6)";
#define _ossl_old_des_set_odd_parity(key) ___ossl_old_des_set_odd_parity(AmiSSLBase, (key))

int ___ossl_old_des_is_weak_key(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key)="\tjsr\t-14064(a6)";
#define _ossl_old_des_is_weak_key(key) ___ossl_old_des_is_weak_key(AmiSSLBase, (key))

int ___ossl_old_des_set_key(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key, __reg("d0") LONG schedule)="\tjsr\t-14070(a6)";
#define _ossl_old_des_set_key(key, schedule) ___ossl_old_des_set_key(AmiSSLBase, (key), (schedule))

int ___ossl_old_des_key_sched(__reg("a6") struct Library *, __reg("a0") _ossl_old_des_cblock * key, __reg("d0") LONG schedule)="\tjsr\t-14076(a6)";
#define _ossl_old_des_key_sched(key, schedule) ___ossl_old_des_key_sched(AmiSSLBase, (key), (schedule))

void ___ossl_old_des_string_to_key(__reg("a6") struct Library *, __reg("a0") char * str, __reg("a1") _ossl_old_des_cblock * key)="\tjsr\t-14082(a6)";
#define _ossl_old_des_string_to_key(str, key) ___ossl_old_des_string_to_key(AmiSSLBase, (str), (key))

void ___ossl_old_des_string_to_2keys(__reg("a6") struct Library *, __reg("a0") char * str, __reg("a1") _ossl_old_des_cblock * key1, __reg("a2") _ossl_old_des_cblock * key2)="\tjsr\t-14088(a6)";
#define _ossl_old_des_string_to_2keys(str, key1, key2) ___ossl_old_des_string_to_2keys(AmiSSLBase, (str), (key1), (key2))

void ___ossl_old_des_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("a3") int * num, __reg("d2") LONG enc)="\tjsr\t-14094(a6)";
#define _ossl_old_des_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) ___ossl_old_des_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void ___ossl_old_des_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("d1") LONG schedule, __reg("a2") _ossl_old_des_cblock * ivec, __reg("a3") int * num)="\tjsr\t-14100(a6)";
#define _ossl_old_des_ofb64_encrypt(in, out, length, schedule, ivec, num) ___ossl_old_des_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

void ___ossl_096_des_random_seed(__reg("a6") struct Library *, __reg("a0") des_cblock * key)="\tjsr\t-14106(a6)";
#define _ossl_096_des_random_seed(key) ___ossl_096_des_random_seed(AmiSSLBase, (key))

const DH_METHOD * __DH_OpenSSL(__reg("a6") struct Library *)="\tjsr\t-14112(a6)";
#define DH_OpenSSL() __DH_OpenSSL(AmiSSLBase)

void __DH_set_default_method(__reg("a6") struct Library *, __reg("a0") const DH_METHOD * meth)="\tjsr\t-14118(a6)";
#define DH_set_default_method(meth) __DH_set_default_method(AmiSSLBase, (meth))

const DH_METHOD * __DH_get_default_method(__reg("a6") struct Library *)="\tjsr\t-14124(a6)";
#define DH_get_default_method() __DH_get_default_method(AmiSSLBase)

int __DH_set_method(__reg("a6") struct Library *, __reg("a0") DH * dh, __reg("a1") const DH_METHOD * meth)="\tjsr\t-14130(a6)";
#define DH_set_method(dh, meth) __DH_set_method(AmiSSLBase, (dh), (meth))

DH * __DH_new_method(__reg("a6") struct Library *, __reg("a0") ENGINE * engine)="\tjsr\t-14136(a6)";
#define DH_new_method(engine) __DH_new_method(AmiSSLBase, (engine))

DH * __DH_new(__reg("a6") struct Library *)="\tjsr\t-14142(a6)";
#define DH_new() __DH_new(AmiSSLBase)

void __DH_free(__reg("a6") struct Library *, __reg("a0") DH * dh)="\tjsr\t-14148(a6)";
#define DH_free(dh) __DH_free(AmiSSLBase, (dh))

int __DH_up_ref(__reg("a6") struct Library *, __reg("a0") DH * dh)="\tjsr\t-14154(a6)";
#define DH_up_ref(dh) __DH_up_ref(AmiSSLBase, (dh))

int __DH_size(__reg("a6") struct Library *, __reg("a0") const DH * dh)="\tjsr\t-14160(a6)";
#define DH_size(dh) __DH_size(AmiSSLBase, (dh))

int __DH_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-14166(a6)";
#define DH_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __DH_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __DH_set_ex_data(__reg("a6") struct Library *, __reg("a0") DH * d, __reg("d0") LONG idx, __reg("a1") void * arg)="\tjsr\t-14172(a6)";
#define DH_set_ex_data(d, idx, arg) __DH_set_ex_data(AmiSSLBase, (d), (idx), (arg))

void * __DH_get_ex_data(__reg("a6") struct Library *, __reg("a0") DH * d, __reg("d0") LONG idx)="\tjsr\t-14178(a6)";
#define DH_get_ex_data(d, idx) __DH_get_ex_data(AmiSSLBase, (d), (idx))

DH * __DH_generate_parameters(__reg("a6") struct Library *, __reg("d0") LONG prime_len, __reg("d1") LONG generator, __reg("a0") void (*callback)(int, int, void *), __reg("a1") void * cb_arg)="\tjsr\t-14184(a6)";
#define DH_generate_parameters(prime_len, generator, callback, cb_arg) __DH_generate_parameters(AmiSSLBase, (prime_len), (generator), (callback), (cb_arg))

int __DH_check(__reg("a6") struct Library *, __reg("a0") const DH * dh, __reg("a1") int * codes)="\tjsr\t-14190(a6)";
#define DH_check(dh, codes) __DH_check(AmiSSLBase, (dh), (codes))

int __DH_generate_key(__reg("a6") struct Library *, __reg("a0") DH * dh)="\tjsr\t-14196(a6)";
#define DH_generate_key(dh) __DH_generate_key(AmiSSLBase, (dh))

int __DH_compute_key(__reg("a6") struct Library *, __reg("a0") unsigned char * key, __reg("a1") const BIGNUM * pub_key, __reg("a2") DH * dh)="\tjsr\t-14202(a6)";
#define DH_compute_key(key, pub_key, dh) __DH_compute_key(AmiSSLBase, (key), (pub_key), (dh))

DH * __d2i_DHparams(__reg("a6") struct Library *, __reg("a0") DH ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length)="\tjsr\t-14208(a6)";
#define d2i_DHparams(a, pp, length) __d2i_DHparams(AmiSSLBase, (a), (pp), (length))

int __i2d_DHparams(__reg("a6") struct Library *, __reg("a0") const DH * a, __reg("a1") unsigned char ** pp)="\tjsr\t-14214(a6)";
#define i2d_DHparams(a, pp) __i2d_DHparams(AmiSSLBase, (a), (pp))

int __DHparams_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const DH * x)="\tjsr\t-14220(a6)";
#define DHparams_print(bp, x) __DHparams_print(AmiSSLBase, (bp), (x))

void __ERR_load_DH_strings(__reg("a6") struct Library *)="\tjsr\t-14226(a6)";
#define ERR_load_DH_strings() __ERR_load_DH_strings(AmiSSLBase)

DSA_SIG * __DSA_SIG_new(__reg("a6") struct Library *)="\tjsr\t-14232(a6)";
#define DSA_SIG_new() __DSA_SIG_new(AmiSSLBase)

void __DSA_SIG_free(__reg("a6") struct Library *, __reg("a0") DSA_SIG * a)="\tjsr\t-14238(a6)";
#define DSA_SIG_free(a) __DSA_SIG_free(AmiSSLBase, (a))

int __i2d_DSA_SIG(__reg("a6") struct Library *, __reg("a0") const DSA_SIG * a, __reg("a1") unsigned char ** pp)="\tjsr\t-14244(a6)";
#define i2d_DSA_SIG(a, pp) __i2d_DSA_SIG(AmiSSLBase, (a), (pp))

DSA_SIG * __d2i_DSA_SIG(__reg("a6") struct Library *, __reg("a0") DSA_SIG ** v, __reg("a1") const unsigned char ** pp, __reg("d0") long length)="\tjsr\t-14250(a6)";
#define d2i_DSA_SIG(v, pp, length) __d2i_DSA_SIG(AmiSSLBase, (v), (pp), (length))

DSA_SIG * __DSA_do_sign(__reg("a6") struct Library *, __reg("a0") const unsigned char * dgst, __reg("d0") LONG dlen, __reg("a1") DSA * dsa)="\tjsr\t-14256(a6)";
#define DSA_do_sign(dgst, dlen, dsa) __DSA_do_sign(AmiSSLBase, (dgst), (dlen), (dsa))

int __DSA_do_verify(__reg("a6") struct Library *, __reg("a0") const unsigned char * dgst, __reg("d0") LONG dgst_len, __reg("a1") DSA_SIG * sig, __reg("a2") DSA * dsa)="\tjsr\t-14262(a6)";
#define DSA_do_verify(dgst, dgst_len, sig, dsa) __DSA_do_verify(AmiSSLBase, (dgst), (dgst_len), (sig), (dsa))

const DSA_METHOD * __DSA_OpenSSL(__reg("a6") struct Library *)="\tjsr\t-14268(a6)";
#define DSA_OpenSSL() __DSA_OpenSSL(AmiSSLBase)

void __DSA_set_default_method(__reg("a6") struct Library *, __reg("a0") const DSA_METHOD * a)="\tjsr\t-14274(a6)";
#define DSA_set_default_method(a) __DSA_set_default_method(AmiSSLBase, (a))

const DSA_METHOD * __DSA_get_default_method(__reg("a6") struct Library *)="\tjsr\t-14280(a6)";
#define DSA_get_default_method() __DSA_get_default_method(AmiSSLBase)

int __DSA_set_method(__reg("a6") struct Library *, __reg("a0") DSA * dsa, __reg("a1") const DSA_METHOD * a)="\tjsr\t-14286(a6)";
#define DSA_set_method(dsa, a) __DSA_set_method(AmiSSLBase, (dsa), (a))

DSA * __DSA_new(__reg("a6") struct Library *)="\tjsr\t-14292(a6)";
#define DSA_new() __DSA_new(AmiSSLBase)

DSA * __DSA_new_method(__reg("a6") struct Library *, __reg("a0") ENGINE * engine)="\tjsr\t-14298(a6)";
#define DSA_new_method(engine) __DSA_new_method(AmiSSLBase, (engine))

void __DSA_free(__reg("a6") struct Library *, __reg("a0") DSA * r)="\tjsr\t-14304(a6)";
#define DSA_free(r) __DSA_free(AmiSSLBase, (r))

int __DSA_up_ref(__reg("a6") struct Library *, __reg("a0") DSA * r)="\tjsr\t-14310(a6)";
#define DSA_up_ref(r) __DSA_up_ref(AmiSSLBase, (r))

int __DSA_size(__reg("a6") struct Library *, __reg("a0") const DSA * a)="\tjsr\t-14316(a6)";
#define DSA_size(a) __DSA_size(AmiSSLBase, (a))

int __DSA_sign_setup(__reg("a6") struct Library *, __reg("a0") DSA * dsa, __reg("a1") BN_CTX * ctx_in, __reg("a2") BIGNUM ** kinvp, __reg("a3") BIGNUM ** rp)="\tjsr\t-14322(a6)";
#define DSA_sign_setup(dsa, ctx_in, kinvp, rp) __DSA_sign_setup(AmiSSLBase, (dsa), (ctx_in), (kinvp), (rp))

int __DSA_sign(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * dgst, __reg("d1") LONG dlen, __reg("a1") unsigned char * sig, __reg("a2") unsigned int * siglen, __reg("a3") DSA * dsa)="\tjsr\t-14328(a6)";
#define DSA_sign(type, dgst, dlen, sig, siglen, dsa) __DSA_sign(AmiSSLBase, (type), (dgst), (dlen), (sig), (siglen), (dsa))

int __DSA_verify(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * dgst, __reg("d1") LONG dgst_len, __reg("a1") const unsigned char * sigbuf, __reg("d2") LONG siglen, __reg("a2") DSA * dsa)="\tjsr\t-14334(a6)";
#define DSA_verify(type, dgst, dgst_len, sigbuf, siglen, dsa) __DSA_verify(AmiSSLBase, (type), (dgst), (dgst_len), (sigbuf), (siglen), (dsa))

int __DSA_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-14340(a6)";
#define DSA_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __DSA_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __DSA_set_ex_data(__reg("a6") struct Library *, __reg("a0") DSA * d, __reg("d0") LONG idx, __reg("a1") void * arg)="\tjsr\t-14346(a6)";
#define DSA_set_ex_data(d, idx, arg) __DSA_set_ex_data(AmiSSLBase, (d), (idx), (arg))

void * __DSA_get_ex_data(__reg("a6") struct Library *, __reg("a0") DSA * d, __reg("d0") LONG idx)="\tjsr\t-14352(a6)";
#define DSA_get_ex_data(d, idx) __DSA_get_ex_data(AmiSSLBase, (d), (idx))

DSA * __d2i_DSAPublicKey(__reg("a6") struct Library *, __reg("a0") DSA ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length)="\tjsr\t-14358(a6)";
#define d2i_DSAPublicKey(a, pp, length) __d2i_DSAPublicKey(AmiSSLBase, (a), (pp), (length))

DSA * __d2i_DSAPrivateKey(__reg("a6") struct Library *, __reg("a0") DSA ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length)="\tjsr\t-14364(a6)";
#define d2i_DSAPrivateKey(a, pp, length) __d2i_DSAPrivateKey(AmiSSLBase, (a), (pp), (length))

DSA * __d2i_DSAparams(__reg("a6") struct Library *, __reg("a0") DSA ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length)="\tjsr\t-14370(a6)";
#define d2i_DSAparams(a, pp, length) __d2i_DSAparams(AmiSSLBase, (a), (pp), (length))

DSA * __DSA_generate_parameters(__reg("a6") struct Library *, __reg("d0") LONG bits, __reg("a0") unsigned char * seed, __reg("d1") LONG seed_len, __reg("a1") int * counter_ret, __reg("a2") unsigned long * h_ret, __reg("a3") void (*callback)(int, int, void *), __reg("d2") void * cb_arg)="\tjsr\t-14376(a6)";
#define DSA_generate_parameters(bits, seed, seed_len, counter_ret, h_ret, callback, cb_arg) __DSA_generate_parameters(AmiSSLBase, (bits), (seed), (seed_len), (counter_ret), (h_ret), (callback), (cb_arg))

int __DSA_generate_key(__reg("a6") struct Library *, __reg("a0") DSA * a)="\tjsr\t-14382(a6)";
#define DSA_generate_key(a) __DSA_generate_key(AmiSSLBase, (a))

int __i2d_DSAPublicKey(__reg("a6") struct Library *, __reg("a0") const DSA * a, __reg("a1") unsigned char ** pp)="\tjsr\t-14388(a6)";
#define i2d_DSAPublicKey(a, pp) __i2d_DSAPublicKey(AmiSSLBase, (a), (pp))

int __i2d_DSAPrivateKey(__reg("a6") struct Library *, __reg("a0") const DSA * a, __reg("a1") unsigned char ** pp)="\tjsr\t-14394(a6)";
#define i2d_DSAPrivateKey(a, pp) __i2d_DSAPrivateKey(AmiSSLBase, (a), (pp))

int __i2d_DSAparams(__reg("a6") struct Library *, __reg("a0") const DSA * a, __reg("a1") unsigned char ** pp)="\tjsr\t-14400(a6)";
#define i2d_DSAparams(a, pp) __i2d_DSAparams(AmiSSLBase, (a), (pp))

int __DSAparams_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const DSA * x)="\tjsr\t-14406(a6)";
#define DSAparams_print(bp, x) __DSAparams_print(AmiSSLBase, (bp), (x))

int __DSA_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const DSA * x, __reg("d0") LONG off)="\tjsr\t-14412(a6)";
#define DSA_print(bp, x, off) __DSA_print(AmiSSLBase, (bp), (x), (off))

DH * __DSA_dup_DH(__reg("a6") struct Library *, __reg("a0") const DSA * r)="\tjsr\t-14418(a6)";
#define DSA_dup_DH(r) __DSA_dup_DH(AmiSSLBase, (r))

void __ERR_load_DSA_strings(__reg("a6") struct Library *)="\tjsr\t-14424(a6)";
#define ERR_load_DSA_strings() __ERR_load_DSA_strings(AmiSSLBase)

const char * __idea_options(__reg("a6") struct Library *)="\tjsr\t-14430(a6)";
#define idea_options() __idea_options(AmiSSLBase)

void __idea_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") IDEA_KEY_SCHEDULE * ks)="\tjsr\t-14436(a6)";
#define idea_ecb_encrypt(in, out, ks) __idea_ecb_encrypt(AmiSSLBase, (in), (out), (ks))

void __idea_set_encrypt_key(__reg("a6") struct Library *, __reg("a0") const unsigned char * key, __reg("a1") IDEA_KEY_SCHEDULE * ks)="\tjsr\t-14442(a6)";
#define idea_set_encrypt_key(key, ks) __idea_set_encrypt_key(AmiSSLBase, (key), (ks))

void __idea_set_decrypt_key(__reg("a6") struct Library *, __reg("a0") IDEA_KEY_SCHEDULE * ek, __reg("a1") IDEA_KEY_SCHEDULE * dk)="\tjsr\t-14448(a6)";
#define idea_set_decrypt_key(ek, dk) __idea_set_decrypt_key(AmiSSLBase, (ek), (dk))

void __idea_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") IDEA_KEY_SCHEDULE * ks, __reg("a3") unsigned char * iv, __reg("d1") LONG enc)="\tjsr\t-14454(a6)";
#define idea_cbc_encrypt(in, out, length, ks, iv, enc) __idea_cbc_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (enc))

void __idea_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") IDEA_KEY_SCHEDULE * ks, __reg("a3") unsigned char * iv, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-14460(a6)";
#define idea_cfb64_encrypt(in, out, length, ks, iv, num, enc) __idea_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (num), (enc))

void __idea_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") IDEA_KEY_SCHEDULE * ks, __reg("a3") unsigned char * iv, __reg("d1") int * num)="\tjsr\t-14466(a6)";
#define idea_ofb64_encrypt(in, out, length, ks, iv, num) __idea_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (num))

void __idea_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned long * in, __reg("a1") IDEA_KEY_SCHEDULE * ks)="\tjsr\t-14472(a6)";
#define idea_encrypt(in, ks) __idea_encrypt(AmiSSLBase, (in), (ks))

const char * __MD2_options(__reg("a6") struct Library *)="\tjsr\t-14478(a6)";
#define MD2_options() __MD2_options(AmiSSLBase)

int __MD2_Init(__reg("a6") struct Library *, __reg("a0") MD2_CTX * c)="\tjsr\t-14484(a6)";
#define MD2_Init(c) __MD2_Init(AmiSSLBase, (c))

int __MD2_Update(__reg("a6") struct Library *, __reg("a0") MD2_CTX * c, __reg("a1") const unsigned char * data, __reg("d0") unsigned long len)="\tjsr\t-14490(a6)";
#define MD2_Update(c, data, len) __MD2_Update(AmiSSLBase, (c), (data), (len))

int __MD2_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") MD2_CTX * c)="\tjsr\t-14496(a6)";
#define MD2_Final(md, c) __MD2_Final(AmiSSLBase, (md), (c))

unsigned char * __MD2(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-14502(a6)";
#define MD2(d, n, md) __MD2(AmiSSLBase, (d), (n), (md))

int __MD4_Init(__reg("a6") struct Library *, __reg("a0") MD4_CTX * c)="\tjsr\t-14508(a6)";
#define MD4_Init(c) __MD4_Init(AmiSSLBase, (c))

int __MD4_Update(__reg("a6") struct Library *, __reg("a0") MD4_CTX * c, __reg("a1") const void * data, __reg("d0") unsigned long len)="\tjsr\t-14514(a6)";
#define MD4_Update(c, data, len) __MD4_Update(AmiSSLBase, (c), (data), (len))

int __MD4_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") MD4_CTX * c)="\tjsr\t-14520(a6)";
#define MD4_Final(md, c) __MD4_Final(AmiSSLBase, (md), (c))

unsigned char * __MD4(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-14526(a6)";
#define MD4(d, n, md) __MD4(AmiSSLBase, (d), (n), (md))

void __MD4_Transform(__reg("a6") struct Library *, __reg("a0") MD4_CTX * c, __reg("a1") const unsigned char * b)="\tjsr\t-14532(a6)";
#define MD4_Transform(c, b) __MD4_Transform(AmiSSLBase, (c), (b))

int __MD5_Init(__reg("a6") struct Library *, __reg("a0") MD5_CTX * c)="\tjsr\t-14538(a6)";
#define MD5_Init(c) __MD5_Init(AmiSSLBase, (c))

int __MD5_Update(__reg("a6") struct Library *, __reg("a0") MD5_CTX * c, __reg("a1") const void * data, __reg("d0") unsigned long len)="\tjsr\t-14544(a6)";
#define MD5_Update(c, data, len) __MD5_Update(AmiSSLBase, (c), (data), (len))

int __MD5_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") MD5_CTX * c)="\tjsr\t-14550(a6)";
#define MD5_Final(md, c) __MD5_Final(AmiSSLBase, (md), (c))

unsigned char * __MD5(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-14556(a6)";
#define MD5(d, n, md) __MD5(AmiSSLBase, (d), (n), (md))

void __MD5_Transform(__reg("a6") struct Library *, __reg("a0") MD5_CTX * c, __reg("a1") const unsigned char * b)="\tjsr\t-14562(a6)";
#define MD5_Transform(c, b) __MD5_Transform(AmiSSLBase, (c), (b))

int __MDC2_Init(__reg("a6") struct Library *, __reg("a0") MDC2_CTX * c)="\tjsr\t-14568(a6)";
#define MDC2_Init(c) __MDC2_Init(AmiSSLBase, (c))

int __MDC2_Update(__reg("a6") struct Library *, __reg("a0") MDC2_CTX * c, __reg("a1") const unsigned char * data, __reg("d0") unsigned long len)="\tjsr\t-14574(a6)";
#define MDC2_Update(c, data, len) __MDC2_Update(AmiSSLBase, (c), (data), (len))

int __MDC2_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") MDC2_CTX * c)="\tjsr\t-14580(a6)";
#define MDC2_Final(md, c) __MDC2_Final(AmiSSLBase, (md), (c))

unsigned char * __MDC2(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-14586(a6)";
#define MDC2(d, n, md) __MDC2(AmiSSLBase, (d), (n), (md))

void __RC2_set_key(__reg("a6") struct Library *, __reg("a0") RC2_KEY * key, __reg("d0") LONG len, __reg("a1") const unsigned char * data, __reg("d1") LONG bits)="\tjsr\t-14592(a6)";
#define RC2_set_key(key, len, data, bits) __RC2_set_key(AmiSSLBase, (key), (len), (data), (bits))

void __RC2_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") RC2_KEY * key, __reg("d0") LONG enc)="\tjsr\t-14598(a6)";
#define RC2_ecb_encrypt(in, out, key, enc) __RC2_ecb_encrypt(AmiSSLBase, (in), (out), (key), (enc))

void __RC2_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned long * data, __reg("a1") RC2_KEY * key)="\tjsr\t-14604(a6)";
#define RC2_encrypt(data, key) __RC2_encrypt(AmiSSLBase, (data), (key))

void __RC2_decrypt(__reg("a6") struct Library *, __reg("a0") unsigned long * data, __reg("a1") RC2_KEY * key)="\tjsr\t-14610(a6)";
#define RC2_decrypt(data, key) __RC2_decrypt(AmiSSLBase, (data), (key))

void __RC2_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC2_KEY * ks, __reg("a3") unsigned char * iv, __reg("d1") LONG enc)="\tjsr\t-14616(a6)";
#define RC2_cbc_encrypt(in, out, length, ks, iv, enc) __RC2_cbc_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (enc))

void __RC2_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC2_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-14622(a6)";
#define RC2_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) __RC2_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void __RC2_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC2_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num)="\tjsr\t-14628(a6)";
#define RC2_ofb64_encrypt(in, out, length, schedule, ivec, num) __RC2_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

const char * __RC4_options(__reg("a6") struct Library *)="\tjsr\t-14634(a6)";
#define RC4_options() __RC4_options(AmiSSLBase)

void __RC4_set_key(__reg("a6") struct Library *, __reg("a0") RC4_KEY * key, __reg("d0") LONG len, __reg("a1") const unsigned char * data)="\tjsr\t-14640(a6)";
#define RC4_set_key(key, len, data) __RC4_set_key(AmiSSLBase, (key), (len), (data))

void __RC4(__reg("a6") struct Library *, __reg("a0") RC4_KEY * key, __reg("d0") unsigned long len, __reg("a1") const unsigned char * indata, __reg("a2") unsigned char * outdata)="\tjsr\t-14646(a6)";
#define RC4(key, len, indata, outdata) __RC4(AmiSSLBase, (key), (len), (indata), (outdata))

void __RC5_32_set_key(__reg("a6") struct Library *, __reg("a0") RC5_32_KEY * key, __reg("d0") LONG len, __reg("a1") const unsigned char * data, __reg("d1") LONG rounds)="\tjsr\t-14652(a6)";
#define RC5_32_set_key(key, len, data, rounds) __RC5_32_set_key(AmiSSLBase, (key), (len), (data), (rounds))

void __RC5_32_ecb_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("a2") RC5_32_KEY * key, __reg("d0") LONG enc)="\tjsr\t-14658(a6)";
#define RC5_32_ecb_encrypt(in, out, key, enc) __RC5_32_ecb_encrypt(AmiSSLBase, (in), (out), (key), (enc))

void __RC5_32_encrypt(__reg("a6") struct Library *, __reg("a0") unsigned long * data, __reg("a1") RC5_32_KEY * key)="\tjsr\t-14664(a6)";
#define RC5_32_encrypt(data, key) __RC5_32_encrypt(AmiSSLBase, (data), (key))

void __RC5_32_decrypt(__reg("a6") struct Library *, __reg("a0") unsigned long * data, __reg("a1") RC5_32_KEY * key)="\tjsr\t-14670(a6)";
#define RC5_32_decrypt(data, key) __RC5_32_decrypt(AmiSSLBase, (data), (key))

void __RC5_32_cbc_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC5_32_KEY * ks, __reg("a3") unsigned char * iv, __reg("d1") LONG enc)="\tjsr\t-14676(a6)";
#define RC5_32_cbc_encrypt(in, out, length, ks, iv, enc) __RC5_32_cbc_encrypt(AmiSSLBase, (in), (out), (length), (ks), (iv), (enc))

void __RC5_32_cfb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC5_32_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num, __reg("d2") LONG enc)="\tjsr\t-14682(a6)";
#define RC5_32_cfb64_encrypt(in, out, length, schedule, ivec, num, enc) __RC5_32_cfb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num), (enc))

void __RC5_32_ofb64_encrypt(__reg("a6") struct Library *, __reg("a0") const unsigned char * in, __reg("a1") unsigned char * out, __reg("d0") long length, __reg("a2") RC5_32_KEY * schedule, __reg("a3") unsigned char * ivec, __reg("d1") int * num)="\tjsr\t-14688(a6)";
#define RC5_32_ofb64_encrypt(in, out, length, schedule, ivec, num) __RC5_32_ofb64_encrypt(AmiSSLBase, (in), (out), (length), (schedule), (ivec), (num))

int __RIPEMD160_Init(__reg("a6") struct Library *, __reg("a0") RIPEMD160_CTX * c)="\tjsr\t-14694(a6)";
#define RIPEMD160_Init(c) __RIPEMD160_Init(AmiSSLBase, (c))

int __RIPEMD160_Update(__reg("a6") struct Library *, __reg("a0") RIPEMD160_CTX * c, __reg("a1") const void * data, __reg("d0") unsigned long len)="\tjsr\t-14700(a6)";
#define RIPEMD160_Update(c, data, len) __RIPEMD160_Update(AmiSSLBase, (c), (data), (len))

int __RIPEMD160_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") RIPEMD160_CTX * c)="\tjsr\t-14706(a6)";
#define RIPEMD160_Final(md, c) __RIPEMD160_Final(AmiSSLBase, (md), (c))

unsigned char * __RIPEMD160(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-14712(a6)";
#define RIPEMD160(d, n, md) __RIPEMD160(AmiSSLBase, (d), (n), (md))

void __RIPEMD160_Transform(__reg("a6") struct Library *, __reg("a0") RIPEMD160_CTX * c, __reg("a1") const unsigned char * b)="\tjsr\t-14718(a6)";
#define RIPEMD160_Transform(c, b) __RIPEMD160_Transform(AmiSSLBase, (c), (b))

RSA * __RSA_new(__reg("a6") struct Library *)="\tjsr\t-14724(a6)";
#define RSA_new() __RSA_new(AmiSSLBase)

RSA * __RSA_new_method(__reg("a6") struct Library *, __reg("a0") ENGINE * engine)="\tjsr\t-14730(a6)";
#define RSA_new_method(engine) __RSA_new_method(AmiSSLBase, (engine))

int __RSA_size(__reg("a6") struct Library *, __reg("a0") const RSA * a)="\tjsr\t-14736(a6)";
#define RSA_size(a) __RSA_size(AmiSSLBase, (a))

RSA * __RSA_generate_key(__reg("a6") struct Library *, __reg("d0") LONG bits, __reg("d1") unsigned long e, __reg("a0") void (*callback)(int, int, void *), __reg("a1") void * cb_arg)="\tjsr\t-14742(a6)";
#define RSA_generate_key(bits, e, callback, cb_arg) __RSA_generate_key(AmiSSLBase, (bits), (e), (callback), (cb_arg))

int __RSA_check_key(__reg("a6") struct Library *, __reg("a0") const RSA * a)="\tjsr\t-14748(a6)";
#define RSA_check_key(a) __RSA_check_key(AmiSSLBase, (a))

int __RSA_public_encrypt(__reg("a6") struct Library *, __reg("d0") LONG flen, __reg("a0") const unsigned char * from, __reg("a1") unsigned char * to, __reg("a2") RSA * rsa, __reg("d1") LONG padding)="\tjsr\t-14754(a6)";
#define RSA_public_encrypt(flen, from, to, rsa, padding) __RSA_public_encrypt(AmiSSLBase, (flen), (from), (to), (rsa), (padding))

int __RSA_private_encrypt(__reg("a6") struct Library *, __reg("d0") LONG flen, __reg("a0") const unsigned char * from, __reg("a1") unsigned char * to, __reg("a2") RSA * rsa, __reg("d1") LONG padding)="\tjsr\t-14760(a6)";
#define RSA_private_encrypt(flen, from, to, rsa, padding) __RSA_private_encrypt(AmiSSLBase, (flen), (from), (to), (rsa), (padding))

int __RSA_public_decrypt(__reg("a6") struct Library *, __reg("d0") LONG flen, __reg("a0") const unsigned char * from, __reg("a1") unsigned char * to, __reg("a2") RSA * rsa, __reg("d1") LONG padding)="\tjsr\t-14766(a6)";
#define RSA_public_decrypt(flen, from, to, rsa, padding) __RSA_public_decrypt(AmiSSLBase, (flen), (from), (to), (rsa), (padding))

int __RSA_private_decrypt(__reg("a6") struct Library *, __reg("d0") LONG flen, __reg("a0") const unsigned char * from, __reg("a1") unsigned char * to, __reg("a2") RSA * rsa, __reg("d1") LONG padding)="\tjsr\t-14772(a6)";
#define RSA_private_decrypt(flen, from, to, rsa, padding) __RSA_private_decrypt(AmiSSLBase, (flen), (from), (to), (rsa), (padding))

void __RSA_free(__reg("a6") struct Library *, __reg("a0") RSA * r)="\tjsr\t-14778(a6)";
#define RSA_free(r) __RSA_free(AmiSSLBase, (r))

int __RSA_up_ref(__reg("a6") struct Library *, __reg("a0") RSA * r)="\tjsr\t-14784(a6)";
#define RSA_up_ref(r) __RSA_up_ref(AmiSSLBase, (r))

int __RSA_flags(__reg("a6") struct Library *, __reg("a0") const RSA * r)="\tjsr\t-14790(a6)";
#define RSA_flags(r) __RSA_flags(AmiSSLBase, (r))

void __RSA_set_default_method(__reg("a6") struct Library *, __reg("a0") const RSA_METHOD * meth)="\tjsr\t-14796(a6)";
#define RSA_set_default_method(meth) __RSA_set_default_method(AmiSSLBase, (meth))

const RSA_METHOD * __RSA_get_default_method(__reg("a6") struct Library *)="\tjsr\t-14802(a6)";
#define RSA_get_default_method() __RSA_get_default_method(AmiSSLBase)

const RSA_METHOD * __RSA_get_method(__reg("a6") struct Library *, __reg("a0") const RSA * rsa)="\tjsr\t-14808(a6)";
#define RSA_get_method(rsa) __RSA_get_method(AmiSSLBase, (rsa))

int __RSA_set_method(__reg("a6") struct Library *, __reg("a0") RSA * rsa, __reg("a1") const RSA_METHOD * meth)="\tjsr\t-14814(a6)";
#define RSA_set_method(rsa, meth) __RSA_set_method(AmiSSLBase, (rsa), (meth))

int __RSA_memory_lock(__reg("a6") struct Library *, __reg("a0") RSA * r)="\tjsr\t-14820(a6)";
#define RSA_memory_lock(r) __RSA_memory_lock(AmiSSLBase, (r))

const RSA_METHOD * __RSA_PKCS1_SSLeay(__reg("a6") struct Library *)="\tjsr\t-14826(a6)";
#define RSA_PKCS1_SSLeay() __RSA_PKCS1_SSLeay(AmiSSLBase)

const RSA_METHOD * __RSA_null_method(__reg("a6") struct Library *)="\tjsr\t-14832(a6)";
#define RSA_null_method() __RSA_null_method(AmiSSLBase)

RSA * __d2i_RSAPublicKey(__reg("a6") struct Library *, __reg("a0") RSA ** a, __reg("a1") const unsigned char ** in, __reg("d0") long len)="\tjsr\t-14838(a6)";
#define d2i_RSAPublicKey(a, in, len) __d2i_RSAPublicKey(AmiSSLBase, (a), (in), (len))

int __i2d_RSAPublicKey(__reg("a6") struct Library *, __reg("a0") const RSA * a, __reg("a1") unsigned char ** out)="\tjsr\t-14844(a6)";
#define i2d_RSAPublicKey(a, out) __i2d_RSAPublicKey(AmiSSLBase, (a), (out))

const ASN1_ITEM * __RSAPublicKey_it(__reg("a6") struct Library *)="\tjsr\t-14850(a6)";
#define RSAPublicKey_it() __RSAPublicKey_it(AmiSSLBase)

RSA * __d2i_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") RSA ** a, __reg("a1") const unsigned char ** in, __reg("d0") long len)="\tjsr\t-14856(a6)";
#define d2i_RSAPrivateKey(a, in, len) __d2i_RSAPrivateKey(AmiSSLBase, (a), (in), (len))

int __i2d_RSAPrivateKey(__reg("a6") struct Library *, __reg("a0") const RSA * a, __reg("a1") unsigned char ** out)="\tjsr\t-14862(a6)";
#define i2d_RSAPrivateKey(a, out) __i2d_RSAPrivateKey(AmiSSLBase, (a), (out))

const ASN1_ITEM * __RSAPrivateKey_it(__reg("a6") struct Library *)="\tjsr\t-14868(a6)";
#define RSAPrivateKey_it() __RSAPrivateKey_it(AmiSSLBase)

int __RSA_print(__reg("a6") struct Library *, __reg("a0") BIO * bp, __reg("a1") const RSA * r, __reg("d0") LONG offset)="\tjsr\t-14874(a6)";
#define RSA_print(bp, r, offset) __RSA_print(AmiSSLBase, (bp), (r), (offset))

int __i2d_RSA_NET(__reg("a6") struct Library *, __reg("a0") const RSA * a, __reg("a1") unsigned char ** pp, __reg("a2") int (*cb)(), __reg("d0") LONG sgckey)="\tjsr\t-14880(a6)";
#define i2d_RSA_NET(a, pp, cb, sgckey) __i2d_RSA_NET(AmiSSLBase, (a), (pp), (cb), (sgckey))

RSA * __d2i_RSA_NET(__reg("a6") struct Library *, __reg("a0") RSA ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length, __reg("a2") int (*cb)(), __reg("d1") LONG sgckey)="\tjsr\t-14886(a6)";
#define d2i_RSA_NET(a, pp, length, cb, sgckey) __d2i_RSA_NET(AmiSSLBase, (a), (pp), (length), (cb), (sgckey))

int __i2d_Netscape_RSA(__reg("a6") struct Library *, __reg("a0") const RSA * a, __reg("a1") unsigned char ** pp, __reg("a2") int (*cb)())="\tjsr\t-14892(a6)";
#define i2d_Netscape_RSA(a, pp, cb) __i2d_Netscape_RSA(AmiSSLBase, (a), (pp), (cb))

RSA * __d2i_Netscape_RSA(__reg("a6") struct Library *, __reg("a0") RSA ** a, __reg("a1") const unsigned char ** pp, __reg("d0") long length, __reg("a2") int (*cb)())="\tjsr\t-14898(a6)";
#define d2i_Netscape_RSA(a, pp, length, cb) __d2i_Netscape_RSA(AmiSSLBase, (a), (pp), (length), (cb))

int __RSA_sign(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * m, __reg("d1") ULONG m_length, __reg("a1") unsigned char * sigret, __reg("a2") unsigned int * siglen, __reg("a3") RSA * rsa)="\tjsr\t-14904(a6)";
#define RSA_sign(type, m, m_length, sigret, siglen, rsa) __RSA_sign(AmiSSLBase, (type), (m), (m_length), (sigret), (siglen), (rsa))

int __RSA_verify(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * m, __reg("d1") ULONG m_length, __reg("a1") unsigned char * sigbuf, __reg("d2") ULONG siglen, __reg("a2") RSA * rsa)="\tjsr\t-14910(a6)";
#define RSA_verify(type, m, m_length, sigbuf, siglen, rsa) __RSA_verify(AmiSSLBase, (type), (m), (m_length), (sigbuf), (siglen), (rsa))

int __RSA_sign_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * m, __reg("d1") ULONG m_length, __reg("a1") unsigned char * sigret, __reg("a2") unsigned int * siglen, __reg("a3") RSA * rsa)="\tjsr\t-14916(a6)";
#define RSA_sign_ASN1_OCTET_STRING(type, m, m_length, sigret, siglen, rsa) __RSA_sign_ASN1_OCTET_STRING(AmiSSLBase, (type), (m), (m_length), (sigret), (siglen), (rsa))

int __RSA_verify_ASN1_OCTET_STRING(__reg("a6") struct Library *, __reg("d0") LONG type, __reg("a0") const unsigned char * m, __reg("d1") ULONG m_length, __reg("a1") unsigned char * sigbuf, __reg("d2") ULONG siglen, __reg("a2") RSA * rsa)="\tjsr\t-14922(a6)";
#define RSA_verify_ASN1_OCTET_STRING(type, m, m_length, sigbuf, siglen, rsa) __RSA_verify_ASN1_OCTET_STRING(AmiSSLBase, (type), (m), (m_length), (sigbuf), (siglen), (rsa))

int __RSA_blinding_on(__reg("a6") struct Library *, __reg("a0") RSA * rsa, __reg("a1") BN_CTX * ctx)="\tjsr\t-14928(a6)";
#define RSA_blinding_on(rsa, ctx) __RSA_blinding_on(AmiSSLBase, (rsa), (ctx))

void __RSA_blinding_off(__reg("a6") struct Library *, __reg("a0") RSA * rsa)="\tjsr\t-14934(a6)";
#define RSA_blinding_off(rsa) __RSA_blinding_off(AmiSSLBase, (rsa))

int __RSA_padding_add_PKCS1_type_1(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl)="\tjsr\t-14940(a6)";
#define RSA_padding_add_PKCS1_type_1(to, tlen, f, fl) __RSA_padding_add_PKCS1_type_1(AmiSSLBase, (to), (tlen), (f), (fl))

int __RSA_padding_check_PKCS1_type_1(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("d2") LONG rsa_len)="\tjsr\t-14946(a6)";
#define RSA_padding_check_PKCS1_type_1(to, tlen, f, fl, rsa_len) __RSA_padding_check_PKCS1_type_1(AmiSSLBase, (to), (tlen), (f), (fl), (rsa_len))

int __RSA_padding_add_PKCS1_type_2(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl)="\tjsr\t-14952(a6)";
#define RSA_padding_add_PKCS1_type_2(to, tlen, f, fl) __RSA_padding_add_PKCS1_type_2(AmiSSLBase, (to), (tlen), (f), (fl))

int __RSA_padding_check_PKCS1_type_2(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("d2") LONG rsa_len)="\tjsr\t-14958(a6)";
#define RSA_padding_check_PKCS1_type_2(to, tlen, f, fl, rsa_len) __RSA_padding_check_PKCS1_type_2(AmiSSLBase, (to), (tlen), (f), (fl), (rsa_len))

int __RSA_padding_add_PKCS1_OAEP(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("a2") const unsigned char * p, __reg("d2") LONG pl)="\tjsr\t-14964(a6)";
#define RSA_padding_add_PKCS1_OAEP(to, tlen, f, fl, p, pl) __RSA_padding_add_PKCS1_OAEP(AmiSSLBase, (to), (tlen), (f), (fl), (p), (pl))

int __RSA_padding_check_PKCS1_OAEP(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("d2") LONG rsa_len, __reg("a2") const unsigned char * p, __reg("d3") LONG pl)="\tjsr\t-14970(a6)";
#define RSA_padding_check_PKCS1_OAEP(to, tlen, f, fl, rsa_len, p, pl) __RSA_padding_check_PKCS1_OAEP(AmiSSLBase, (to), (tlen), (f), (fl), (rsa_len), (p), (pl))

int __RSA_padding_add_SSLv23(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl)="\tjsr\t-14976(a6)";
#define RSA_padding_add_SSLv23(to, tlen, f, fl) __RSA_padding_add_SSLv23(AmiSSLBase, (to), (tlen), (f), (fl))

int __RSA_padding_check_SSLv23(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("d2") LONG rsa_len)="\tjsr\t-14982(a6)";
#define RSA_padding_check_SSLv23(to, tlen, f, fl, rsa_len) __RSA_padding_check_SSLv23(AmiSSLBase, (to), (tlen), (f), (fl), (rsa_len))

int __RSA_padding_add_none(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl)="\tjsr\t-14988(a6)";
#define RSA_padding_add_none(to, tlen, f, fl) __RSA_padding_add_none(AmiSSLBase, (to), (tlen), (f), (fl))

int __RSA_padding_check_none(__reg("a6") struct Library *, __reg("a0") unsigned char * to, __reg("d0") LONG tlen, __reg("a1") const unsigned char * f, __reg("d1") LONG fl, __reg("d2") LONG rsa_len)="\tjsr\t-14994(a6)";
#define RSA_padding_check_none(to, tlen, f, fl, rsa_len) __RSA_padding_check_none(AmiSSLBase, (to), (tlen), (f), (fl), (rsa_len))

int __RSA_get_ex_new_index(__reg("a6") struct Library *, __reg("d0") long argl, __reg("a0") void * argp, __reg("a1") CRYPTO_EX_new * (*new_func)(), __reg("a2") CRYPTO_EX_dup * (*dup_func)(), __reg("a3") CRYPTO_EX_free * (*free_func)())="\tjsr\t-15000(a6)";
#define RSA_get_ex_new_index(argl, argp, new_func, dup_func, free_func) __RSA_get_ex_new_index(AmiSSLBase, (argl), (argp), (new_func), (dup_func), (free_func))

int __RSA_set_ex_data(__reg("a6") struct Library *, __reg("a0") RSA * r, __reg("d0") LONG idx, __reg("a1") void * arg)="\tjsr\t-15006(a6)";
#define RSA_set_ex_data(r, idx, arg) __RSA_set_ex_data(AmiSSLBase, (r), (idx), (arg))

void * __RSA_get_ex_data(__reg("a6") struct Library *, __reg("a0") const RSA * r, __reg("d0") LONG idx)="\tjsr\t-15012(a6)";
#define RSA_get_ex_data(r, idx) __RSA_get_ex_data(AmiSSLBase, (r), (idx))

RSA * __RSAPublicKey_dup(__reg("a6") struct Library *, __reg("a0") RSA * rsa)="\tjsr\t-15018(a6)";
#define RSAPublicKey_dup(rsa) __RSAPublicKey_dup(AmiSSLBase, (rsa))

RSA * __RSAPrivateKey_dup(__reg("a6") struct Library *, __reg("a0") RSA * rsa)="\tjsr\t-15024(a6)";
#define RSAPrivateKey_dup(rsa) __RSAPrivateKey_dup(AmiSSLBase, (rsa))

void __ERR_load_RSA_strings(__reg("a6") struct Library *)="\tjsr\t-15030(a6)";
#define ERR_load_RSA_strings() __ERR_load_RSA_strings(AmiSSLBase)

int __SHA_Init(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c)="\tjsr\t-15036(a6)";
#define SHA_Init(c) __SHA_Init(AmiSSLBase, (c))

int __SHA_Update(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c, __reg("a1") const void * data, __reg("d0") unsigned long len)="\tjsr\t-15042(a6)";
#define SHA_Update(c, data, len) __SHA_Update(AmiSSLBase, (c), (data), (len))

int __SHA_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") SHA_CTX * c)="\tjsr\t-15048(a6)";
#define SHA_Final(md, c) __SHA_Final(AmiSSLBase, (md), (c))

unsigned char * __SHA(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-15054(a6)";
#define SHA(d, n, md) __SHA(AmiSSLBase, (d), (n), (md))

void __SHA_Transform(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c, __reg("a1") const unsigned char * data)="\tjsr\t-15060(a6)";
#define SHA_Transform(c, data) __SHA_Transform(AmiSSLBase, (c), (data))

int __SHA1_Init(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c)="\tjsr\t-15066(a6)";
#define SHA1_Init(c) __SHA1_Init(AmiSSLBase, (c))

int __SHA1_Update(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c, __reg("a1") const void * data, __reg("d0") unsigned long len)="\tjsr\t-15072(a6)";
#define SHA1_Update(c, data, len) __SHA1_Update(AmiSSLBase, (c), (data), (len))

int __SHA1_Final(__reg("a6") struct Library *, __reg("a0") unsigned char * md, __reg("a1") SHA_CTX * c)="\tjsr\t-15078(a6)";
#define SHA1_Final(md, c) __SHA1_Final(AmiSSLBase, (md), (c))

unsigned char * __SHA1(__reg("a6") struct Library *, __reg("a0") const unsigned char * d, __reg("d0") unsigned long n, __reg("a1") unsigned char * md)="\tjsr\t-15084(a6)";
#define SHA1(d, n, md) __SHA1(AmiSSLBase, (d), (n), (md))

void __SHA1_Transform(__reg("a6") struct Library *, __reg("a0") SHA_CTX * c, __reg("a1") const unsigned char * data)="\tjsr\t-15090(a6)";
#define SHA1_Transform(c, data) __SHA1_Transform(AmiSSLBase, (c), (data))

void __HMAC_CTX_set_flags(__reg("a6") struct Library *, __reg("a0") HMAC_CTX * ctx, __reg("d0") unsigned long flags)="\tjsr\t-15108(a6)";
#define HMAC_CTX_set_flags(ctx, flags) __HMAC_CTX_set_flags(AmiSSLBase, (ctx), (flags))

int __X509_check_ca(__reg("a6") struct Library *, __reg("a0") X509 * x)="\tjsr\t-15114(a6)";
#define X509_check_ca(x) __X509_check_ca(AmiSSLBase, (x))

PROXY_POLICY * __PROXY_POLICY_new(__reg("a6") struct Library *)="\tjsr\t-15120(a6)";
#define PROXY_POLICY_new() __PROXY_POLICY_new(AmiSSLBase)

void __PROXY_POLICY_free(__reg("a6") struct Library *, __reg("a0") PROXY_POLICY * a)="\tjsr\t-15126(a6)";
#define PROXY_POLICY_free(a) __PROXY_POLICY_free(AmiSSLBase, (a))

PROXY_POLICY * __d2i_PROXY_POLICY(__reg("a6") struct Library *, __reg("a0") PROXY_POLICY ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-15132(a6)";
#define d2i_PROXY_POLICY(a, in, len) __d2i_PROXY_POLICY(AmiSSLBase, (a), (in), (len))

int __i2d_PROXY_POLICY(__reg("a6") struct Library *, __reg("a0") PROXY_POLICY * a, __reg("a1") unsigned char ** out)="\tjsr\t-15138(a6)";
#define i2d_PROXY_POLICY(a, out) __i2d_PROXY_POLICY(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PROXY_POLICY_it(__reg("a6") struct Library *)="\tjsr\t-15144(a6)";
#define PROXY_POLICY_it() __PROXY_POLICY_it(AmiSSLBase)

PROXY_CERT_INFO_EXTENSION * __PROXY_CERT_INFO_EXTENSION_new(__reg("a6") struct Library *)="\tjsr\t-15150(a6)";
#define PROXY_CERT_INFO_EXTENSION_new() __PROXY_CERT_INFO_EXTENSION_new(AmiSSLBase)

void __PROXY_CERT_INFO_EXTENSION_free(__reg("a6") struct Library *, __reg("a0") PROXY_CERT_INFO_EXTENSION * a)="\tjsr\t-15156(a6)";
#define PROXY_CERT_INFO_EXTENSION_free(a) __PROXY_CERT_INFO_EXTENSION_free(AmiSSLBase, (a))

PROXY_CERT_INFO_EXTENSION * __d2i_PROXY_CERT_INFO_EXTENSION(__reg("a6") struct Library *, __reg("a0") PROXY_CERT_INFO_EXTENSION ** a, __reg("a1") unsigned char ** in, __reg("d0") long len)="\tjsr\t-15162(a6)";
#define d2i_PROXY_CERT_INFO_EXTENSION(a, in, len) __d2i_PROXY_CERT_INFO_EXTENSION(AmiSSLBase, (a), (in), (len))

int __i2d_PROXY_CERT_INFO_EXTENSION(__reg("a6") struct Library *, __reg("a0") PROXY_CERT_INFO_EXTENSION * a, __reg("a1") unsigned char ** out)="\tjsr\t-15168(a6)";
#define i2d_PROXY_CERT_INFO_EXTENSION(a, out) __i2d_PROXY_CERT_INFO_EXTENSION(AmiSSLBase, (a), (out))

const ASN1_ITEM * __PROXY_CERT_INFO_EXTENSION_it(__reg("a6") struct Library *)="\tjsr\t-15174(a6)";
#define PROXY_CERT_INFO_EXTENSION_it() __PROXY_CERT_INFO_EXTENSION_it(AmiSSLBase)

#endif /*  _VBCCINLINE_AMISSL_H  */
