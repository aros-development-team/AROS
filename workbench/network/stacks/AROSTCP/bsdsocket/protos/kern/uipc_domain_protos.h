/* Prototypes for functions defined in uipc_domain.c
 */

BOOL domaininit(void);

struct protosw * pffindtype(int family,
                            int type);

struct protosw * pffindproto(int family,
                             int protocol,
                             int type);

void pfctlinput(int cmd,
                struct sockaddr * sa);

void pfslowtimo(void);

void pffasttimo(void);

