#ifndef CLIB_EXPAT_PROTOS_H
#define CLIB_EXPAT_PROTOS_H


/*
**	$VER: expat_protos.h 1.0 (14.04.2010)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2010 
**	All Rights Reserved
*/

#ifndef  LIBRARIES_EXPAT_H
#include <libraries/expat.h>
#endif

XML_Parser XML_ParserCreate(const encodingName);
XML_Parser XML_ParserCreateNS(const encodingName, LONG nsSep);
XML_Parser XML_ParserCreate_MM(const encodingName, const memsuite, const nameSep);
XML_Parser XML_ExternalEntityParserCreate(LONG oldParser, const context, const encodingName);
void XML_ParserFree(LONG parser);
int XML_Parse(LONG parser, const char * s, LONG len, LONG isFinal);
int XML_ParseBuffer(LONG parser, LONG len, LONG isFinal);
void * XML_GetBuffer(LONG parser, LONG len);
void XML_SetStartElementHandler(LONG parser, LONG start);
void XML_SetEndElementHandler(LONG parser, LONG end);
void XML_SetElementHandler(LONG parser, LONG start, LONG end);
void XML_SetCharacterDataHandler(LONG parser, LONG handler);
void XML_SetProcessingInstructionHandler(LONG parser, LONG handler);
void XML_SetCommentHandler(LONG parser, LONG handler);
void XML_SetStartCdataSectionHandler(LONG parser, LONG start);
void XML_SetEndCdataSectionHandler(LONG parser, LONG end);
void XML_SetCdataSectionHandler(LONG parser, LONG start, LONG end);
void XML_SetDefaultHandler(LONG parser, LONG handler);
void XML_SetDefaultHandlerExpand(LONG parser, LONG handler);
void XML_SetExternalEntityRefHandler(LONG parser, LONG handler);
void XML_SetExternalEntityRefHandlerArg(LONG parser, void * arg);
void XML_SetUnknownEncodingHandler(LONG parser, LONG handler, void * data);
void XML_SetStartNamespaceDeclHandler(LONG parser, LONG start);
void XML_SetEndNamespaceDeclHandler(LONG parser, LONG end);
void XML_SetNamespaceDeclHandler(LONG parser, LONG start, LONG end);
void XML_SetXmlDeclHandler(LONG parser, LONG handler);
void XML_SetStartDoctypeDeclHandler(LONG parser, LONG start);
void XML_SetEndDoctypeDeclHandler(LONG parser, LONG end);
void XML_SetDoctypeDeclHandler(LONG parser, LONG start, LONG end);
void XML_SetElementDeclHandler(LONG parser, LONG eldecl);
void XML_SetAttlistDeclHandler(LONG parser, LONG attdecl);
void XML_SetEntityDeclHandler(LONG parser, LONG handler);
void XML_SetUnparsedEntityDeclHandler(LONG parser, LONG handler);
void XML_SetNotationDeclHandler(LONG parser, LONG handler);
void XML_SetNotStandaloneHandler(LONG parser, LONG handler);
int XML_GetErrorCode(LONG parser);
const XML_LChar * XML_ErrorString(LONG code);
long XML_GetCurrentByteIndex(LONG parser);
int XML_GetCurrentLineNumber(LONG parser);
int XML_GetCurrentColumnNumber(LONG parser);
int XML_GetCurrentByteCount(LONG parser);
const char * XML_GetInputContext(LONG parser, int * offset, int * size);
void XML_SetUserData(LONG parser, void * p);
void XML_DefaultCurrent(LONG parser);
void XML_UseParserAsHandlerArg(LONG parser);
int XML_SetBase(LONG parser, const p);
const XML_Char * XML_GetBase(LONG parser);
int XML_GetSpecifiedAttributeCount(LONG parser);
int XML_GetIdAttributeIndex(LONG parser);
int XML_SetEncoding(LONG parser, const encodingName);
int XML_SetParamEntityParsing(LONG parser, LONG peParsing);
void XML_SetReturnNSTriplet(LONG parser, LONG do_nst);
const XML_LChar * XML_ExpatVersion(void);
XML_Expat_Version XML_ExpatVersionInfo(void);
XML_Bool XML_ParserReset(LONG parser, const encodingName);
void XML_SetSkippedEntityHandler(LONG parser, LONG handler);
int XML_UseForeignDTD(LONG parser, LONG useDTD);
const XML_Feature * XML_GetFeatureList(void);
int XML_StopParser(LONG parser, LONG resumable);
int XML_ResumeParser(LONG parser);
void XML_GetParsingStatus(LONG parser, XML_ParsingStatus * status);
void XML_FreeContentModel(LONG parser, XML_Content * model);
void * XML_MemMalloc(LONG parser, ULONG size);
void * XML_MemRealloc(LONG parser, void * ptr, ULONG size);
void XML_MemFree(LONG parser, void * ptr);

#endif	/*  CLIB_EXPAT_PROTOS_H  */
