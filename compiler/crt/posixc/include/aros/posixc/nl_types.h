#ifndef _NL_TYPES_H
#define _NL_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type used for langinfo constants and message catalogs */
typedef int nl_item;

/* Constants for message catalogs (used with catgets, not langinfo) */
#define NL_SETD     1   /* Default set number */

/* Placeholder for catalog descriptor type */
typedef void *nl_catd;

/* Functions for message catalog (POSIX, not used by langinfo directly) */
nl_catd catopen(const char *name, int oflag);
char *catgets(nl_catd catalog, int set_number, int message_number, const char *default_string);
int catclose(nl_catd catalog);

#ifdef __cplusplus
}
#endif

#endif /* _NL_TYPES_H */
