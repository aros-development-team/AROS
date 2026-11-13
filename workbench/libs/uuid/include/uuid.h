#ifndef LIBRARIES_UUID_H
#define LIBRARIES_UUID_H

#include <inttypes.h>

typedef uint64_t uuid_time_t;

typedef struct
{
    uint8_t  nodeID[6]; 
} uuid_node_t;
 
typedef struct
{
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version; 
    uint8_t  clock_seq_hi_and_reserved;
    uint8_t  clock_seq_low;
    uint8_t  node[6];
} uuid_t;

#define MAKE_UUID(a, b, c, d, e)	\
{					\
    (a), (b), (c),			\
    ((d) >> 8) & 0xFF, (d) & 0xFF,	\
    {					\
         (UQUAD)(e) >> 40,		\
        ((UQUAD)(e) >> 32) & 0xFF,	\
               ((e) >> 24) & 0xFF,	\
               ((e) >> 16) & 0xFF,	\
               ((e) >>  8) & 0xFF,	\
                (e) 	   & 0xFF	\
    }					\
}

typedef enum
{
    UUID_NAMESPACE_DNS = 1,
    UUID_NAMESPACE_URL,
    UUID_NAMESPACE_OID,
    UUID_NAMESPACE_X500
} uuid_namespace_t;

typedef enum
{
    UUID_TYPE_DCE_TIME = 1,
    UUID_TYPE_DCE_RANDOM = 4
} uuid_type_t;

typedef enum
{
    UUID_VARIANT_NCS = 0,
    UUID_VARIANT_DCE = 1,
    UUID_VARIANT_MICROSOFT = 2,
    UUID_VARIANT_OTHER = 3
} uuid_variant_t;

#define UUID_STRLEN     36

#endif /*LIBRARIES_UUID_H*/
