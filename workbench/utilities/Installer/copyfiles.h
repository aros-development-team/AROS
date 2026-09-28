/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef _COPYFILES_H
#define _COPYFILES_H

extern int do_copyfiles(struct ParameterList *pl);
extern int do_copylib(struct ParameterList *pl);
extern int scan_version(const char *file, ULONG *ver, ULONG *rev);
extern ULONG resident_version(const char *name);
extern LONG file_checksum(const char *file);
extern ULONG apply_protect_string(ULONG bits, const char *str);
extern int dir_matches(const char *dir, const char *pattern, char ***names, LONG **types);
extern char **tooltypes_clone(char **tt);
extern void tooltypes_free(char **tt);
extern char **tooltypes_set(char **tt, const char *name, const char *value);

#endif /* _COPYFILES_H */
