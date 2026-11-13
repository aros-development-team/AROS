/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef _INLINE_EXPAT_H
#define _INLINE_EXPAT_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef EXPAT_BASE_NAME
#define EXPAT_BASE_NAME ExpatBase
#endif /* !EXPAT_BASE_NAME */

#define XML_ParserCreate(___encodingName) \
	AROS_LC1(XML_Parser, XML_ParserCreate, \
	AROS_LCA(const XML_Char *, (___encodingName), A0), \
	struct Library *, EXPAT_BASE_NAME, 5, Expat)

#define XML_ParserCreateNS(___encodingName, ___nsSep) \
	AROS_LC2(XML_Parser, XML_ParserCreateNS, \
	AROS_LCA(const XML_Char *, (___encodingName), A0), \
	AROS_LCA(XML_Char, (___nsSep), D0), \
	struct Library *, EXPAT_BASE_NAME, 6, Expat)

#define XML_ParserCreate_MM(___encodingName, ___memsuite, ___nameSep) \
	AROS_LC3(XML_Parser, XML_ParserCreate_MM, \
	AROS_LCA(const XML_Char *, (___encodingName), A0), \
	AROS_LCA(const XML_Memory_Handling_Suite *, (___memsuite), A1), \
	AROS_LCA(const XML_Char *, (___nameSep), A2), \
	struct Library *, EXPAT_BASE_NAME, 7, Expat)

#define XML_ExternalEntityParserCreate(___oldParser, ___context, ___encodingName) \
	AROS_LC3(XML_Parser, XML_ExternalEntityParserCreate, \
	AROS_LCA(XML_Parser, (___oldParser), A0), \
	AROS_LCA(const XML_Char *, (___context), A1), \
	AROS_LCA(const XML_Char *, (___encodingName), A2), \
	struct Library *, EXPAT_BASE_NAME, 8, Expat)

#define XML_ParserFree(___parser) \
	AROS_LC1(void, XML_ParserFree, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 9, Expat)

#define XML_Parse(___parser, ___s, ___len, ___isFinal) \
	AROS_LC4(int, XML_Parse, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(const char *, (___s), A1), \
	AROS_LCA(int, (___len), D0), \
	AROS_LCA(int, (___isFinal), D1), \
	struct Library *, EXPAT_BASE_NAME, 10, Expat)

#define XML_ParseBuffer(___parser, ___len, ___isFinal) \
	AROS_LC3(int, XML_ParseBuffer, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(int, (___len), D0), \
	AROS_LCA(int, (___isFinal), D1), \
	struct Library *, EXPAT_BASE_NAME, 11, Expat)

#define XML_GetBuffer(___parser, ___len) \
	AROS_LC2(void *, XML_GetBuffer, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(int, (___len), D0), \
	struct Library *, EXPAT_BASE_NAME, 12, Expat)

#define XML_SetStartElementHandler(___parser, ___start) \
	AROS_LC2(void, XML_SetStartElementHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartElementHandler, (___start), A1), \
	struct Library *, EXPAT_BASE_NAME, 13, Expat)

#define XML_SetEndElementHandler(___parser, ___end) \
	AROS_LC2(void, XML_SetEndElementHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_EndElementHandler, (___end), A1), \
	struct Library *, EXPAT_BASE_NAME, 14, Expat)

#define XML_SetElementHandler(___parser, ___start, ___end) \
	AROS_LC3(void, XML_SetElementHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartElementHandler, (___start), A1), \
	AROS_LCA(XML_EndElementHandler, (___end), A2), \
	struct Library *, EXPAT_BASE_NAME, 15, Expat)

#define XML_SetCharacterDataHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetCharacterDataHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_CharacterDataHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 16, Expat)

#define XML_SetProcessingInstructionHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetProcessingInstructionHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_ProcessingInstructionHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 17, Expat)

#define XML_SetCommentHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetCommentHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_CommentHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 18, Expat)

#define XML_SetStartCdataSectionHandler(___parser, ___start) \
	AROS_LC2(void, XML_SetStartCdataSectionHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartCdataSectionHandler, (___start), A1), \
	struct Library *, EXPAT_BASE_NAME, 19, Expat)

#define XML_SetEndCdataSectionHandler(___parser, ___end) \
	AROS_LC2(void, XML_SetEndCdataSectionHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_EndCdataSectionHandler, (___end), A1), \
	struct Library *, EXPAT_BASE_NAME, 20, Expat)

#define XML_SetCdataSectionHandler(___parser, ___start, ___end) \
	AROS_LC3(void, XML_SetCdataSectionHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartCdataSectionHandler, (___start), A1), \
	AROS_LCA(XML_EndCdataSectionHandler, (___end), A2), \
	struct Library *, EXPAT_BASE_NAME, 21, Expat)

#define XML_SetDefaultHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetDefaultHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_DefaultHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 22, Expat)

#define XML_SetDefaultHandlerExpand(___parser, ___handler) \
	AROS_LC2(void, XML_SetDefaultHandlerExpand, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_DefaultHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 23, Expat)

#define XML_SetExternalEntityRefHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetExternalEntityRefHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_ExternalEntityRefHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 24, Expat)

#define XML_SetExternalEntityRefHandlerArg(___parser, ___arg) \
	AROS_LC2(void, XML_SetExternalEntityRefHandlerArg, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(void *, (___arg), A1), \
	struct Library *, EXPAT_BASE_NAME, 25, Expat)

#define XML_SetUnknownEncodingHandler(___parser, ___handler, ___data) \
	AROS_LC3(void, XML_SetUnknownEncodingHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_UnknownEncodingHandler, (___handler), A1), \
	AROS_LCA(void *, (___data), A2), \
	struct Library *, EXPAT_BASE_NAME, 26, Expat)

#define XML_SetStartNamespaceDeclHandler(___parser, ___start) \
	AROS_LC2(void, XML_SetStartNamespaceDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartNamespaceDeclHandler, (___start), A1), \
	struct Library *, EXPAT_BASE_NAME, 27, Expat)

#define XML_SetEndNamespaceDeclHandler(___parser, ___end) \
	AROS_LC2(void, XML_SetEndNamespaceDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_EndNamespaceDeclHandler, (___end), A1), \
	struct Library *, EXPAT_BASE_NAME, 28, Expat)

#define XML_SetNamespaceDeclHandler(___parser, ___start, ___end) \
	AROS_LC3(void, XML_SetNamespaceDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartNamespaceDeclHandler, (___start), A1), \
	AROS_LCA(XML_EndNamespaceDeclHandler, (___end), A2), \
	struct Library *, EXPAT_BASE_NAME, 29, Expat)

#define XML_SetXmlDeclHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetXmlDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_XmlDeclHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 30, Expat)

#define XML_SetStartDoctypeDeclHandler(___parser, ___start) \
	AROS_LC2(void, XML_SetStartDoctypeDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartDoctypeDeclHandler, (___start), A1), \
	struct Library *, EXPAT_BASE_NAME, 31, Expat)

#define XML_SetEndDoctypeDeclHandler(___parser, ___end) \
	AROS_LC2(void, XML_SetEndDoctypeDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_EndDoctypeDeclHandler, (___end), A1), \
	struct Library *, EXPAT_BASE_NAME, 32, Expat)

#define XML_SetDoctypeDeclHandler(___parser, ___start, ___end) \
	AROS_LC3(void, XML_SetDoctypeDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_StartDoctypeDeclHandler, (___start), A1), \
	AROS_LCA(XML_EndDoctypeDeclHandler, (___end), A2), \
	struct Library *, EXPAT_BASE_NAME, 33, Expat)

#define XML_SetElementDeclHandler(___parser, ___eldecl) \
	AROS_LC2(void, XML_SetElementDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_ElementDeclHandler, (___eldecl), A1), \
	struct Library *, EXPAT_BASE_NAME, 34, Expat)

#define XML_SetAttlistDeclHandler(___parser, ___attdecl) \
	AROS_LC2(void, XML_SetAttlistDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_AttlistDeclHandler, (___attdecl), A1), \
	struct Library *, EXPAT_BASE_NAME, 35, Expat)

#define XML_SetEntityDeclHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetEntityDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_EntityDeclHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 36, Expat)

#define XML_SetUnparsedEntityDeclHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetUnparsedEntityDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_UnparsedEntityDeclHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 37, Expat)

#define XML_SetNotationDeclHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetNotationDeclHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_NotationDeclHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 38, Expat)

#define XML_SetNotStandaloneHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetNotStandaloneHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_NotStandaloneHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 39, Expat)

#define XML_GetErrorCode(___parser) \
	AROS_LC1(int, XML_GetErrorCode, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 40, Expat)

#define XML_ErrorString(___code) \
	AROS_LC1(const XML_LChar *, XML_ErrorString, \
	AROS_LCA(int, (___code), D0), \
	struct Library *, EXPAT_BASE_NAME, 41, Expat)

#define XML_GetCurrentByteIndex(___parser) \
	AROS_LC1(long, XML_GetCurrentByteIndex, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 42, Expat)

#define XML_GetCurrentLineNumber(___parser) \
	AROS_LC1(int, XML_GetCurrentLineNumber, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 43, Expat)

#define XML_GetCurrentColumnNumber(___parser) \
	AROS_LC1(int, XML_GetCurrentColumnNumber, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 44, Expat)

#define XML_GetCurrentByteCount(___parser) \
	AROS_LC1(int, XML_GetCurrentByteCount, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 45, Expat)

#define XML_GetInputContext(___parser, ___offset, ___size) \
	AROS_LC3(const char *, XML_GetInputContext, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(int *, (___offset), A1), \
	AROS_LCA(int *, (___size), A2), \
	struct Library *, EXPAT_BASE_NAME, 46, Expat)

#define XML_SetUserData(___parser, ___p) \
	AROS_LC2(void, XML_SetUserData, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(void *, (___p), A1), \
	struct Library *, EXPAT_BASE_NAME, 47, Expat)

#define XML_DefaultCurrent(___parser) \
	AROS_LC1(void, XML_DefaultCurrent, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 48, Expat)

#define XML_UseParserAsHandlerArg(___parser) \
	AROS_LC1(void, XML_UseParserAsHandlerArg, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 49, Expat)

#define XML_SetBase(___parser, ___p) \
	AROS_LC2(int, XML_SetBase, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(const XML_Char *, (___p), A1), \
	struct Library *, EXPAT_BASE_NAME, 50, Expat)

#define XML_GetBase(___parser) \
	AROS_LC1(const XML_Char *, XML_GetBase, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 51, Expat)

#define XML_GetSpecifiedAttributeCount(___parser) \
	AROS_LC1(int, XML_GetSpecifiedAttributeCount, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 52, Expat)

#define XML_GetIdAttributeIndex(___parser) \
	AROS_LC1(int, XML_GetIdAttributeIndex, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 53, Expat)

#define XML_SetEncoding(___parser, ___encodingName) \
	AROS_LC2(int, XML_SetEncoding, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(const XML_Char *, (___encodingName), A1), \
	struct Library *, EXPAT_BASE_NAME, 54, Expat)

#define XML_SetParamEntityParsing(___parser, ___peParsing) \
	AROS_LC2(int, XML_SetParamEntityParsing, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(int, (___peParsing), D0), \
	struct Library *, EXPAT_BASE_NAME, 55, Expat)

#define XML_SetReturnNSTriplet(___parser, ___do_nst) \
	AROS_LC2(void, XML_SetReturnNSTriplet, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(int, (___do_nst), D0), \
	struct Library *, EXPAT_BASE_NAME, 56, Expat)

#define XML_ExpatVersion() \
	AROS_LC0(const XML_LChar *, XML_ExpatVersion, \
	struct Library *, EXPAT_BASE_NAME, 57, Expat)

#define XML_ExpatVersionInfo() \
	AROS_LC0(XML_Expat_Version, XML_ExpatVersionInfo, \
	struct Library *, EXPAT_BASE_NAME, 58, Expat)

#define XML_ParserReset(___parser, ___encodingName) \
	AROS_LC2(XML_Bool, XML_ParserReset, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(const XML_Char *, (___encodingName), A1), \
	struct Library *, EXPAT_BASE_NAME, 59, Expat)

#define XML_SetSkippedEntityHandler(___parser, ___handler) \
	AROS_LC2(void, XML_SetSkippedEntityHandler, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_SkippedEntityHandler, (___handler), A1), \
	struct Library *, EXPAT_BASE_NAME, 60, Expat)

#define XML_UseForeignDTD(___parser, ___useDTD) \
	AROS_LC2(int, XML_UseForeignDTD, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_Bool, (___useDTD), D0), \
	struct Library *, EXPAT_BASE_NAME, 61, Expat)

#define XML_GetFeatureList() \
	AROS_LC0(const XML_Feature *, XML_GetFeatureList, \
	struct Library *, EXPAT_BASE_NAME, 62, Expat)

#define XML_StopParser(___parser, ___resumable) \
	AROS_LC2(int, XML_StopParser, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_Bool, (___resumable), D0), \
	struct Library *, EXPAT_BASE_NAME, 63, Expat)

#define XML_ResumeParser(___parser) \
	AROS_LC1(int, XML_ResumeParser, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	struct Library *, EXPAT_BASE_NAME, 64, Expat)

#define XML_GetParsingStatus(___parser, ___status) \
	AROS_LC2(void, XML_GetParsingStatus, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_ParsingStatus *, (___status), A1), \
	struct Library *, EXPAT_BASE_NAME, 65, Expat)

#define XML_FreeContentModel(___parser, ___model) \
	AROS_LC2(void, XML_FreeContentModel, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(XML_Content *, (___model), A1), \
	struct Library *, EXPAT_BASE_NAME, 66, Expat)

#define XML_MemMalloc(___parser, ___size) \
	AROS_LC2(void *, XML_MemMalloc, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(size_t, (___size), D0), \
	struct Library *, EXPAT_BASE_NAME, 67, Expat)

#define XML_MemRealloc(___parser, ___ptr, ___size) \
	AROS_LC3(void *, XML_MemRealloc, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(void *, (___ptr), A1), \
	AROS_LCA(size_t, (___size), D0), \
	struct Library *, EXPAT_BASE_NAME, 68, Expat)

#define XML_MemFree(___parser, ___ptr) \
	AROS_LC2(void, XML_MemFree, \
	AROS_LCA(XML_Parser, (___parser), A0), \
	AROS_LCA(void *, (___ptr), A1), \
	struct Library *, EXPAT_BASE_NAME, 69, Expat)

#endif /* !_INLINE_EXPAT_H */
