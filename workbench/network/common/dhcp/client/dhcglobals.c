/* dhcglobals.c

   Global variables shared between the client code and link libraries. */

#include "dhcpd.h"

/* dhcpd.c */
int ddns_update_style;
enum dhcp_shutdown_state shutdown_state;

/* class.c */
struct class unknown_class;
struct class known_class;
/* default_collection and collections are defined in common/parse.c */
struct executable_statement *default_classification_rules;

/* resolv.c */
struct name_server *name_servers;
struct domain_search_list *domains;
