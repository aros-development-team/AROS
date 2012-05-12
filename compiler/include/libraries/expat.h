#ifndef LIBRARIES_EXPAT_H
#define LIBRARIES_EXPAT_H

#ifndef STRING_H
#include <string.h>
#endif

typedef void *XML_Parser;
typedef char XML_Char;
typedef char XML_LChar;
typedef unsigned char XML_Bool;

#define XML_TRUE   ((XML_Bool) 1)
#define XML_FALSE  ((XML_Bool) 0)

enum XML_Error
{
    XML_ERROR_NONE,
    XML_ERROR_NO_MEMORY,
    XML_ERROR_SYNTAX,
    XML_ERROR_NO_ELEMENTS,
    XML_ERROR_INVALID_TOKEN,
    XML_ERROR_UNCLOSED_TOKEN,
    XML_ERROR_PARTIAL_CHAR,
    XML_ERROR_TAG_MISMATCH,
    XML_ERROR_DUPLICATE_ATTRIBUTE,
    XML_ERROR_JUNK_AFTER_DOC_ELEMENT,
    XML_ERROR_PARAM_ENTITY_REF,
    XML_ERROR_UNDEFINED_ENTITY,
    XML_ERROR_RECURSIVE_ENTITY_REF,
    XML_ERROR_ASYNC_ENTITY,
    XML_ERROR_BAD_CHAR_REF,
    XML_ERROR_BINARY_ENTITY_REF,
    XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF,
    XML_ERROR_MISPLACED_XML_PI,
    XML_ERROR_UNKNOWN_ENCODING,
    XML_ERROR_INCORRECT_ENCODING,
    XML_ERROR_UNCLOSED_CDATA_SECTION,
    XML_ERROR_EXTERNAL_ENTITY_HANDLING,
    XML_ERROR_NOT_STANDALONE,
    XML_ERROR_UNEXPECTED_STATE,
    XML_ERROR_ENTITY_DECLARED_IN_PE,
    XML_ERROR_FEATURE_REQUIRES_XML_DTD,
    XML_ERROR_CANT_CHANGE_FEATURE_ONCE_PARSING,
    XML_ERROR_UNBOUND_PREFIX,
    XML_ERROR_UNDECLARING_PREFIX,
    XML_ERROR_INCOMPLETE_PE,
    XML_ERROR_XML_DECL,
    XML_ERROR_TEXT_DECL,
    XML_ERROR_PUBLICID,
    XML_ERROR_SUSPENDED,
    XML_ERROR_NOT_SUSPENDED,
    XML_ERROR_ABORTED,
    XML_ERROR_FINISHED,
    XML_ERROR_SUSPEND_PE
};

enum XML_Content_Type
{
    XML_CTYPE_EMPTY = 1,
    XML_CTYPE_ANY,
    XML_CTYPE_MIXED,
    XML_CTYPE_NAME,
    XML_CTYPE_CHOICE,
    XML_CTYPE_SEQ
};

enum XML_Content_Quant
{
    XML_CQUANT_NONE,
    XML_CQUANT_OPT,
    XML_CQUANT_REP,
    XML_CQUANT_PLUS
};

typedef struct XML_cp XML_Content;

struct XML_cp
{
    enum XML_Content_Type     type;
    enum XML_Content_Quant    quant;
    XML_Char 		      *name;
    unsigned int              numchildren;
    XML_Content 	      *children;
};


enum XML_Status
{
    XML_STATUS_ERROR = 0,
    XML_STATUS_OK = 1
};

enum XML_ParamEntityParsing
{
    XML_PARAM_ENTITY_PARSING_NEVER,
    XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE,
    XML_PARAM_ENTITY_PARSING_ALWAYS
};

typedef struct
{
    int major;
    int minor;
    int micro;
} XML_Expat_Version;

enum XML_FeatureEnum
{
    XML_FEATURE_END = 0,
    XML_FEATURE_UNICODE,
    XML_FEATURE_UNICODE_WCHAR_T,
    XML_FEATURE_DTD,
    XML_FEATURE_CONTEXT_BYTES,
    XML_FEATURE_MIN_SIZE,
    XML_FEATURE_SIZEOF_XML_CHAR,
    XML_FEATURE_SIZEOF_XML_LCHAR
};

typedef struct
{
    enum XML_FeatureEnum feature;
    XML_LChar            *name;
    long int             value;
} XML_Feature;

enum XML_Parsing
{
    XML_INITIALIZED,
    XML_PARSING,
    XML_FINISHED,
    XML_SUSPENDED
};

typedef struct
{
    enum XML_Parsing parsing;
    XML_Bool 	     finalBuffer;
} XML_ParsingStatus;


typedef struct
{
    void *(*malloc_fcn)(size_t size);
    void *(*realloc_fcn)(void *ptr, size_t size);
    void (*free_fcn)(void *ptr);
} XML_Memory_Handling_Suite;

typedef struct
{
    int  map[256];
    void *data;
    int  (*convert)(void *data, const char *s);
    void (*release)(void *data);
} XML_Encoding;

#define XML_MAJOR_VERSION 1
#define XML_MINOR_VERSION 95
#define XML_MICRO_VERSION 5

#define XML_GetUserData(parser) (*(void **)(parser))

/* Handlers  */

typedef void (*XML_ElementDeclHandler) (void 	       *userData,
                                        const XML_Char *name,
                                        XML_Content    *model);

typedef void (*XML_AttlistDeclHandler) (void	       *userData,
                                        const XML_Char *elname,
                                        const XML_Char *attname,
                                        const XML_Char *att_type,
                                        const XML_Char *dflt,
                                        int            isrequired);

typedef void (*XML_XmlDeclHandler) (void            *userData,
                                    const XML_Char  *version,
                                    const XML_Char  *encoding,
                                    int             standalone);


typedef void (*XML_StartElementHandler)(void 	       *userData,
                                        const XML_Char *name,
                                        const XML_Char **atts);

typedef void (*XML_EndElementHandler)(void 	     *userData,
                                      const XML_Char *name);

typedef void (*XML_CharacterDataHandler)(void 		*userData,
                                         const XML_Char *s,
                                         int 		len);

typedef void (*XML_ProcessingInstructionHandler)(void 		*userData,
                                                 const XML_Char *target,
                                                 const XML_Char *data);

typedef void (*XML_CommentHandler)(void 	  *userData,
				   const XML_Char *data);

typedef void (*XML_StartCdataSectionHandler)(void *userData);

typedef void (*XML_EndCdataSectionHandler)(void *userData);

typedef void (*XML_DefaultHandler)(void 	  *userData,
                                   const XML_Char *s,
                                   int 		  len);

typedef void (*XML_StartDoctypeDeclHandler)(void           *userData,
                                            const XML_Char *doctypeName,
                                            const XML_Char *sysid,
                                            const XML_Char *pubid,
                                            int 	   has_internal_subset);

typedef void (*XML_EndDoctypeDeclHandler)(void *userData);

typedef void (*XML_EntityDeclHandler) (void 	      *userData,
                                       const XML_Char *entityName,
                                       int 	      is_parameter_entity,
                                       const XML_Char *value,
                                       int 	      value_length,
                                       const XML_Char *base,
                                       const XML_Char *systemId,
                                       const XML_Char *publicId,
                                       const XML_Char *notationName);

typedef void (*XML_UnparsedEntityDeclHandler)(void 	     *userData,
                                              const XML_Char *entityName,
                                              const XML_Char *base,
                                              const XML_Char *systemId,
                                              const XML_Char *publicId,
                                              const XML_Char *notationName);

typedef void (*XML_NotationDeclHandler)(void 	       *userData,
                                        const XML_Char *notationName,
                                        const XML_Char *base,
                                        const XML_Char *systemId,
                                        const XML_Char *publicId);

typedef void (*XML_StartNamespaceDeclHandler)(void 	     *userData,
                                              const XML_Char *prefix,
                                              const XML_Char *uri);

typedef void (*XML_EndNamespaceDeclHandler)(void 	   *userData,
                                            const XML_Char *prefix);

typedef int (*XML_NotStandaloneHandler)(void *userData);

typedef int (*XML_ExternalEntityRefHandler)(XML_Parser 	   parser,
                                            const XML_Char *context,
                                            const XML_Char *base,
                                            const XML_Char *systemId,
                                            const XML_Char *publicId);

typedef void (*XML_SkippedEntityHandler)(void 		*userData,
                                         const XML_Char *entityName,
                                         int 		is_parameter_entity);

typedef int (*XML_UnknownEncodingHandler)(void 		 *encodingHandlerData,
                                          const XML_Char *name,
                                          XML_Encoding 	 *info);

#endif /* LIBRARIES_EXPAT_H */
