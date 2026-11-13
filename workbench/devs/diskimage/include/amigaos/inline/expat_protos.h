#ifndef _VBCCINLINE_EXPAT_H
#define _VBCCINLINE_EXPAT_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

XML_Parser __XML_ParserCreate(__reg("a6") struct Library *, __reg("a0") void * encodingName)="\tjsr\t-30(a6)";
#define XML_ParserCreate(encodingName) __XML_ParserCreate(ExpatBase, (void *)(encodingName))

XML_Parser __XML_ParserCreateNS(__reg("a6") struct Library *, __reg("a0") void * encodingName, __reg("d0") LONG nsSep)="\tjsr\t-36(a6)";
#define XML_ParserCreateNS(encodingName, nsSep) __XML_ParserCreateNS(ExpatBase, (void *)(encodingName), (nsSep))

XML_Parser __XML_ParserCreate_MM(__reg("a6") struct Library *, __reg("a0") void * encodingName, __reg("a1") void * memsuite, __reg("a2") void * nameSep)="\tjsr\t-42(a6)";
#define XML_ParserCreate_MM(encodingName, memsuite, nameSep) __XML_ParserCreate_MM(ExpatBase, (void *)(encodingName), (void *)(memsuite), (void *)(nameSep))

XML_Parser __XML_ExternalEntityParserCreate(__reg("a6") struct Library *, __reg("a0") void * oldParser, __reg("a1") void * context, __reg("a2") void * encodingName)="\tjsr\t-48(a6)";
#define XML_ExternalEntityParserCreate(oldParser, context, encodingName) __XML_ExternalEntityParserCreate(ExpatBase, (void *)(oldParser), (void *)(context), (void *)(encodingName))

void __XML_ParserFree(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-54(a6)";
#define XML_ParserFree(parser) __XML_ParserFree(ExpatBase, (void *)(parser))

int __XML_Parse(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") const char * s, __reg("d0") LONG len, __reg("d1") LONG isFinal)="\tjsr\t-60(a6)";
#define XML_Parse(parser, s, len, isFinal) __XML_Parse(ExpatBase, (void *)(parser), (s), (len), (isFinal))

int __XML_ParseBuffer(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG len, __reg("d1") LONG isFinal)="\tjsr\t-66(a6)";
#define XML_ParseBuffer(parser, len, isFinal) __XML_ParseBuffer(ExpatBase, (void *)(parser), (len), (isFinal))

void * __XML_GetBuffer(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG len)="\tjsr\t-72(a6)";
#define XML_GetBuffer(parser, len) __XML_GetBuffer(ExpatBase, (void *)(parser), (len))

void __XML_SetStartElementHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start)="\tjsr\t-78(a6)";
#define XML_SetStartElementHandler(parser, start) __XML_SetStartElementHandler(ExpatBase, (void *)(parser), (void *)(start))

void __XML_SetEndElementHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * end)="\tjsr\t-84(a6)";
#define XML_SetEndElementHandler(parser, end) __XML_SetEndElementHandler(ExpatBase, (void *)(parser), (void *)(end))

void __XML_SetElementHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start, __reg("a2") void * end)="\tjsr\t-90(a6)";
#define XML_SetElementHandler(parser, start, end) __XML_SetElementHandler(ExpatBase, (void *)(parser), (void *)(start), (void *)(end))

void __XML_SetCharacterDataHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-96(a6)";
#define XML_SetCharacterDataHandler(parser, handler) __XML_SetCharacterDataHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetProcessingInstructionHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-102(a6)";
#define XML_SetProcessingInstructionHandler(parser, handler) __XML_SetProcessingInstructionHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetCommentHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-108(a6)";
#define XML_SetCommentHandler(parser, handler) __XML_SetCommentHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetStartCdataSectionHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start)="\tjsr\t-114(a6)";
#define XML_SetStartCdataSectionHandler(parser, start) __XML_SetStartCdataSectionHandler(ExpatBase, (void *)(parser), (void *)(start))

void __XML_SetEndCdataSectionHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * end)="\tjsr\t-120(a6)";
#define XML_SetEndCdataSectionHandler(parser, end) __XML_SetEndCdataSectionHandler(ExpatBase, (void *)(parser), (void *)(end))

void __XML_SetCdataSectionHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start, __reg("a2") void * end)="\tjsr\t-126(a6)";
#define XML_SetCdataSectionHandler(parser, start, end) __XML_SetCdataSectionHandler(ExpatBase, (void *)(parser), (void *)(start), (void *)(end))

void __XML_SetDefaultHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-132(a6)";
#define XML_SetDefaultHandler(parser, handler) __XML_SetDefaultHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetDefaultHandlerExpand(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-138(a6)";
#define XML_SetDefaultHandlerExpand(parser, handler) __XML_SetDefaultHandlerExpand(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetExternalEntityRefHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-144(a6)";
#define XML_SetExternalEntityRefHandler(parser, handler) __XML_SetExternalEntityRefHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetExternalEntityRefHandlerArg(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * arg)="\tjsr\t-150(a6)";
#define XML_SetExternalEntityRefHandlerArg(parser, arg) __XML_SetExternalEntityRefHandlerArg(ExpatBase, (void *)(parser), (arg))

void __XML_SetUnknownEncodingHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler, __reg("a2") void * data)="\tjsr\t-156(a6)";
#define XML_SetUnknownEncodingHandler(parser, handler, data) __XML_SetUnknownEncodingHandler(ExpatBase, (void *)(parser), (void *)(handler), (data))

void __XML_SetStartNamespaceDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start)="\tjsr\t-162(a6)";
#define XML_SetStartNamespaceDeclHandler(parser, start) __XML_SetStartNamespaceDeclHandler(ExpatBase, (void *)(parser), (void *)(start))

void __XML_SetEndNamespaceDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * end)="\tjsr\t-168(a6)";
#define XML_SetEndNamespaceDeclHandler(parser, end) __XML_SetEndNamespaceDeclHandler(ExpatBase, (void *)(parser), (void *)(end))

void __XML_SetNamespaceDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start, __reg("a2") void * end)="\tjsr\t-174(a6)";
#define XML_SetNamespaceDeclHandler(parser, start, end) __XML_SetNamespaceDeclHandler(ExpatBase, (void *)(parser), (void *)(start), (void *)(end))

void __XML_SetXmlDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-180(a6)";
#define XML_SetXmlDeclHandler(parser, handler) __XML_SetXmlDeclHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetStartDoctypeDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start)="\tjsr\t-186(a6)";
#define XML_SetStartDoctypeDeclHandler(parser, start) __XML_SetStartDoctypeDeclHandler(ExpatBase, (void *)(parser), (void *)(start))

void __XML_SetEndDoctypeDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * end)="\tjsr\t-192(a6)";
#define XML_SetEndDoctypeDeclHandler(parser, end) __XML_SetEndDoctypeDeclHandler(ExpatBase, (void *)(parser), (void *)(end))

void __XML_SetDoctypeDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * start, __reg("a2") void * end)="\tjsr\t-198(a6)";
#define XML_SetDoctypeDeclHandler(parser, start, end) __XML_SetDoctypeDeclHandler(ExpatBase, (void *)(parser), (void *)(start), (void *)(end))

void __XML_SetElementDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * eldecl)="\tjsr\t-204(a6)";
#define XML_SetElementDeclHandler(parser, eldecl) __XML_SetElementDeclHandler(ExpatBase, (void *)(parser), (void *)(eldecl))

void __XML_SetAttlistDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * attdecl)="\tjsr\t-210(a6)";
#define XML_SetAttlistDeclHandler(parser, attdecl) __XML_SetAttlistDeclHandler(ExpatBase, (void *)(parser), (void *)(attdecl))

void __XML_SetEntityDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-216(a6)";
#define XML_SetEntityDeclHandler(parser, handler) __XML_SetEntityDeclHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetUnparsedEntityDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-222(a6)";
#define XML_SetUnparsedEntityDeclHandler(parser, handler) __XML_SetUnparsedEntityDeclHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetNotationDeclHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-228(a6)";
#define XML_SetNotationDeclHandler(parser, handler) __XML_SetNotationDeclHandler(ExpatBase, (void *)(parser), (void *)(handler))

void __XML_SetNotStandaloneHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-234(a6)";
#define XML_SetNotStandaloneHandler(parser, handler) __XML_SetNotStandaloneHandler(ExpatBase, (void *)(parser), (void *)(handler))

int __XML_GetErrorCode(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-240(a6)";
#define XML_GetErrorCode(parser) __XML_GetErrorCode(ExpatBase, (void *)(parser))

const XML_LChar * __XML_ErrorString(__reg("a6") struct Library *, __reg("d0") LONG code)="\tjsr\t-246(a6)";
#define XML_ErrorString(code) __XML_ErrorString(ExpatBase, (code))

long __XML_GetCurrentByteIndex(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-252(a6)";
#define XML_GetCurrentByteIndex(parser) __XML_GetCurrentByteIndex(ExpatBase, (void *)(parser))

int __XML_GetCurrentLineNumber(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-258(a6)";
#define XML_GetCurrentLineNumber(parser) __XML_GetCurrentLineNumber(ExpatBase, (void *)(parser))

int __XML_GetCurrentColumnNumber(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-264(a6)";
#define XML_GetCurrentColumnNumber(parser) __XML_GetCurrentColumnNumber(ExpatBase, (void *)(parser))

int __XML_GetCurrentByteCount(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-270(a6)";
#define XML_GetCurrentByteCount(parser) __XML_GetCurrentByteCount(ExpatBase, (void *)(parser))

const char * __XML_GetInputContext(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") int * offset, __reg("a2") int * size)="\tjsr\t-276(a6)";
#define XML_GetInputContext(parser, offset, size) __XML_GetInputContext(ExpatBase, (void *)(parser), (offset), (size))

void __XML_SetUserData(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * p)="\tjsr\t-282(a6)";
#define XML_SetUserData(parser, p) __XML_SetUserData(ExpatBase, (void *)(parser), (p))

void __XML_DefaultCurrent(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-288(a6)";
#define XML_DefaultCurrent(parser) __XML_DefaultCurrent(ExpatBase, (void *)(parser))

void __XML_UseParserAsHandlerArg(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-294(a6)";
#define XML_UseParserAsHandlerArg(parser) __XML_UseParserAsHandlerArg(ExpatBase, (void *)(parser))

int __XML_SetBase(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * p)="\tjsr\t-300(a6)";
#define XML_SetBase(parser, p) __XML_SetBase(ExpatBase, (void *)(parser), (void *)(p))

const XML_Char * __XML_GetBase(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-306(a6)";
#define XML_GetBase(parser) __XML_GetBase(ExpatBase, (void *)(parser))

int __XML_GetSpecifiedAttributeCount(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-312(a6)";
#define XML_GetSpecifiedAttributeCount(parser) __XML_GetSpecifiedAttributeCount(ExpatBase, (void *)(parser))

int __XML_GetIdAttributeIndex(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-318(a6)";
#define XML_GetIdAttributeIndex(parser) __XML_GetIdAttributeIndex(ExpatBase, (void *)(parser))

int __XML_SetEncoding(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * encodingName)="\tjsr\t-324(a6)";
#define XML_SetEncoding(parser, encodingName) __XML_SetEncoding(ExpatBase, (void *)(parser), (void *)(encodingName))

int __XML_SetParamEntityParsing(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG peParsing)="\tjsr\t-330(a6)";
#define XML_SetParamEntityParsing(parser, peParsing) __XML_SetParamEntityParsing(ExpatBase, (void *)(parser), (peParsing))

void __XML_SetReturnNSTriplet(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG do_nst)="\tjsr\t-336(a6)";
#define XML_SetReturnNSTriplet(parser, do_nst) __XML_SetReturnNSTriplet(ExpatBase, (void *)(parser), (do_nst))

const XML_LChar * __XML_ExpatVersion(__reg("a6") struct Library *)="\tjsr\t-342(a6)";
#define XML_ExpatVersion() __XML_ExpatVersion(ExpatBase)

XML_Expat_Version __XML_ExpatVersionInfo(__reg("a6") struct Library *)="\tjsr\t-348(a6)";
#define XML_ExpatVersionInfo() __XML_ExpatVersionInfo(ExpatBase)

XML_Bool __XML_ParserReset(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * encodingName)="\tjsr\t-354(a6)";
#define XML_ParserReset(parser, encodingName) __XML_ParserReset(ExpatBase, (void *)(parser), (void *)(encodingName))

void __XML_SetSkippedEntityHandler(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * handler)="\tjsr\t-360(a6)";
#define XML_SetSkippedEntityHandler(parser, handler) __XML_SetSkippedEntityHandler(ExpatBase, (void *)(parser), (void *)(handler))

int __XML_UseForeignDTD(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG useDTD)="\tjsr\t-366(a6)";
#define XML_UseForeignDTD(parser, useDTD) __XML_UseForeignDTD(ExpatBase, (void *)(parser), (useDTD))

const XML_Feature * __XML_GetFeatureList(__reg("a6") struct Library *)="\tjsr\t-372(a6)";
#define XML_GetFeatureList() __XML_GetFeatureList(ExpatBase)

int __XML_StopParser(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") LONG resumable)="\tjsr\t-378(a6)";
#define XML_StopParser(parser, resumable) __XML_StopParser(ExpatBase, (void *)(parser), (resumable))

int __XML_ResumeParser(__reg("a6") struct Library *, __reg("a0") void * parser)="\tjsr\t-384(a6)";
#define XML_ResumeParser(parser) __XML_ResumeParser(ExpatBase, (void *)(parser))

void __XML_GetParsingStatus(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") XML_ParsingStatus * status)="\tjsr\t-390(a6)";
#define XML_GetParsingStatus(parser, status) __XML_GetParsingStatus(ExpatBase, (void *)(parser), (status))

void __XML_FreeContentModel(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") XML_Content * model)="\tjsr\t-396(a6)";
#define XML_FreeContentModel(parser, model) __XML_FreeContentModel(ExpatBase, (void *)(parser), (model))

void * __XML_MemMalloc(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("d0") ULONG size)="\tjsr\t-402(a6)";
#define XML_MemMalloc(parser, size) __XML_MemMalloc(ExpatBase, (void *)(parser), (size))

void * __XML_MemRealloc(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * ptr, __reg("d0") ULONG size)="\tjsr\t-408(a6)";
#define XML_MemRealloc(parser, ptr, size) __XML_MemRealloc(ExpatBase, (void *)(parser), (ptr), (size))

void __XML_MemFree(__reg("a6") struct Library *, __reg("a0") void * parser, __reg("a1") void * ptr)="\tjsr\t-414(a6)";
#define XML_MemFree(parser, ptr) __XML_MemFree(ExpatBase, (void *)(parser), (ptr))

#endif /*  _VBCCINLINE_EXPAT_H  */
