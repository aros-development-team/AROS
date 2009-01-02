/* Prototypes for functions defined in radix.c
 */

struct radix_node * rn_search(register caddr_t v,
                              struct radix_node * head);

struct radix_node * rn_search_m(register caddr_t v,
                                struct radix_node * head,
                                register caddr_t m);

struct radix_node * rn_match(caddr_t v,
                             struct radix_node * head);

struct radix_node * rn_newpair(caddr_t v,
                               int b,
                               struct radix_node * nodes);

struct radix_node * rn_insert(caddr_t v,
                              struct radix_node * head,
                              int * dupentry,
                              struct radix_node * nodes);

struct radix_node * rn_addmask(caddr_t netmask,
                               int search,
                               int skip);

struct radix_node * rn_addroute(caddr_t v,
                                caddr_t netmask,
                                struct radix_node * head,
                                struct radix_node * treenodes);

struct radix_node * rn_delete(caddr_t v,
                              caddr_t netmask,
                              struct radix_node * head);

int rn_inithead(struct radix_node_head ** head,
                int off,
                int af);

