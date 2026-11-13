/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include <exec/exec.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <expat.h>
#include "support.h"
#include <SDI_compiler.h>
#include "expat.library_rev.h"

#define LIBNAME "expat.library"
const char USED_VAR verstag[] = VERSTAG;

#ifndef __AROS__
#define IPTR ULONG
#endif

struct ExecBase *SysBase;

struct ExpatBase {
	struct Library libNode;
	UWORD pad;
	BPTR seglist;
};

int malloc_init(void);
void malloc_exit(void);

#ifdef __AROS__
static AROS_UFP3(struct ExpatBase *, LibInit,
	AROS_UFPA(struct ExpatBase *, libBase, D0),
	AROS_UFPA(BPTR, seglist, A0),
	AROS_UFPA(struct ExecBase *, exec_base, A6)
);
static AROS_LD1(struct ExpatBase *, LibOpen,
	AROS_LPA(ULONG, version, D0),
	struct ExpatBase *, libBase, 1, Expat
);
static AROS_LD0(BPTR, LibClose,
	struct ExpatBase *, libBase, 2, Expat
);
static AROS_LD0(BPTR, LibExpunge,
	struct ExpatBase *, libBase, 3, Expat
);
static AROS_LD0(APTR, LibReserved,
	struct ExpatBase *, libBase, 4, Expat
);
static AROS_LD1(XML_Parser, ParserCreate,
	AROS_LPA(const XML_Char *, encodingName, A0),
	struct ExpatBase *, libBase, 5, Expat
);
static AROS_LD2(XML_Parser, ParserCreateNS,
	AROS_LPA(const XML_Char *, encodingName, A0),
	AROS_LPA(XML_Char, nsSep, D0),
	struct ExpatBase *, libBase, 6, Expat
);
static AROS_LD3(XML_Parser, ParserCreate_MM,
	AROS_LPA(const XML_Char *, encodingName, A0),
	AROS_LPA(const XML_Memory_Handling_Suite *, memsuite, A1),
	AROS_LPA(const XML_Char *, nameSep, A2),
	struct ExpatBase *, libBase, 7, Expat
);
static AROS_LD3(XML_Parser, ExternalEntityParserCreate,
	AROS_LPA(XML_Parser, oldParser, A0),
	AROS_LPA(const XML_Char *, context, A1),
	AROS_LPA(const XML_Char *, encodingName, A2),
	struct ExpatBase *, libBase, 8, Expat
);
static AROS_LD1(void, ParserFree,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 9, Expat
);
static AROS_LD4(int, Parse,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(const char *, s, A1),
	AROS_LPA(int, len, D0),
	AROS_LPA(int, isFinal, D1),
	struct ExpatBase *, libBase, 10, Expat
);
static AROS_LD3(int, ParseBuffer,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(int, len, D0),
	AROS_LPA(int, isFinal, D1),
	struct ExpatBase *, libBase, 11, Expat
);
static AROS_LD2(void *, GetBuffer,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(int, len, D0),
	struct ExpatBase *, libBase, 12, Expat
);
static AROS_LD2(void, SetStartElementHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartElementHandler, start, A1),
	struct ExpatBase *, libBase, 13, Expat
);
static AROS_LD2(void, SetEndElementHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_EndElementHandler, end, A1),
	struct ExpatBase *, libBase, 14, Expat
);
static AROS_LD3(void, SetElementHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartElementHandler, start, A1),
	AROS_LPA(XML_EndElementHandler, end, A2),
	struct ExpatBase *, libBase, 15, Expat
);
static AROS_LD2(void, SetCharacterDataHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_CharacterDataHandler, handler, A1),
	struct ExpatBase *, libBase, 16, Expat
);
static AROS_LD2(void, SetProcessingInstructionHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_ProcessingInstructionHandler, handler, A1),
	struct ExpatBase *, libBase, 17, Expat
);
static AROS_LD2(void, SetCommentHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_CommentHandler, handler, A1),
	struct ExpatBase *, libBase, 18, Expat
);
static AROS_LD2(void, SetStartCdataSectionHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartCdataSectionHandler, start, A1),
	struct ExpatBase *, libBase, 19, Expat
);
static AROS_LD2(void, SetEndCdataSectionHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_EndCdataSectionHandler, end, A1),
	struct ExpatBase *, libBase, 20, Expat
);
static AROS_LD3(void, SetCdataSectionHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartCdataSectionHandler, start, A1),
	AROS_LPA(XML_EndCdataSectionHandler, end, A2),
	struct ExpatBase *, libBase, 21, Expat
);
static AROS_LD2(void, SetDefaultHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_DefaultHandler, handler, A1),
	struct ExpatBase *, libBase, 22, Expat
);
static AROS_LD2(void, SetDefaultHandlerExpand,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_DefaultHandler, handler, A1),
	struct ExpatBase *, libBase, 23, Expat
);
static AROS_LD2(void, SetExternalEntityRefHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_ExternalEntityRefHandler, handler, A1),
	struct ExpatBase *, libBase, 24, Expat
);
static AROS_LD2(void, SetExternalEntityRefHandlerArg,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(void *, arg, A1),
	struct ExpatBase *, libBase, 25, Expat
);
static AROS_LD3(void, SetUnknownEncodingHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_UnknownEncodingHandler, handler, A1),
	AROS_LPA(void *, data, A2),
	struct ExpatBase *, libBase, 26, Expat
);
static AROS_LD2(void, SetStartNamespaceDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartNamespaceDeclHandler, start, A1),
	struct ExpatBase *, libBase, 27, Expat
);
static AROS_LD2(void, SetEndNamespaceDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_EndNamespaceDeclHandler, end, A1),
	struct ExpatBase *, libBase, 28, Expat
);
static AROS_LD3(void, SetNamespaceDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartNamespaceDeclHandler, start, A1),
	AROS_LPA(XML_EndNamespaceDeclHandler, end, A2),
	struct ExpatBase *, libBase, 29, Expat
);
static AROS_LD2(void, SetXmlDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_XmlDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 30, Expat
);
static AROS_LD2(void, SetStartDoctypeDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartDoctypeDeclHandler, start, A1),
	struct ExpatBase *, libBase, 31, Expat
);
static AROS_LD2(void, SetEndDoctypeDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_EndDoctypeDeclHandler, end, A1),
	struct ExpatBase *, libBase, 32, Expat
);
static AROS_LD3(void, SetDoctypeDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_StartDoctypeDeclHandler, start, A1),
	AROS_LPA(XML_EndDoctypeDeclHandler, end, A2),
	struct ExpatBase *, libBase, 33, Expat
);
static AROS_LD2(void, SetElementDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_ElementDeclHandler, eldecl, A1),
	struct ExpatBase *, libBase, 34, Expat
);
static AROS_LD2(void, SetAttlistDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_AttlistDeclHandler, attdecl, A1),
	struct ExpatBase *, libBase, 35, Expat
);
static AROS_LD2(void, SetEntityDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_EntityDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 36, Expat
);
static AROS_LD2(void, SetUnparsedEntityDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_UnparsedEntityDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 37, Expat
);
static AROS_LD2(void, SetNotationDeclHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_NotationDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 38, Expat
);
static AROS_LD2(void, SetNotStandaloneHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_NotStandaloneHandler, handler, A1),
	struct ExpatBase *, libBase, 39, Expat
);
static AROS_LD1(int, GetErrorCode,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 40, Expat
);
static AROS_LD1(const XML_LChar *, ErrorString,
	AROS_LPA(int, code, D0),
	struct ExpatBase *, libBase, 41, Expat
);
static AROS_LD1(long, GetCurrentByteIndex,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 42, Expat
);
static AROS_LD1(int, GetCurrentLineNumber,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 43, Expat
);
static AROS_LD1(int, GetCurrentColumnNumber,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 44, Expat
);
static AROS_LD1(int, GetCurrentByteCount,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 45, Expat
);
static AROS_LD3(const char *, GetInputContext,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(int *, offset, A1),
	AROS_LPA(int *, size, A2),
	struct ExpatBase *, libBase, 46, Expat
);
static AROS_LD2(void, SetUserData,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(void *, p, A1),
	struct ExpatBase *, libBase, 47, Expat
);
static AROS_LD1(void, DefaultCurrent,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 48, Expat
);
static AROS_LD1(void, UseParserAsHandlerArg,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 49, Expat
);
static AROS_LD2(int, SetBase,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(const XML_Char *, p, A1),
	struct ExpatBase *, libBase, 50, Expat
);
static AROS_LD1(const XML_Char *, GetBase,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 51, Expat
);
static AROS_LD1(int, GetSpecifiedAttributeCount,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 52, Expat
);
static AROS_LD1(int, GetIdAttributeIndex,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 53, Expat
);
static AROS_LD2(int, SetEncoding,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(const XML_Char *, encodingName, A1),
	struct ExpatBase *, libBase, 54, Expat
);
static AROS_LD2(int, SetParamEntityParsing,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(int, peParsing, D0),
	struct ExpatBase *, libBase, 55, Expat
);
static AROS_LD2(void, SetReturnNSTriplet,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(int, do_nst, D0),
	struct ExpatBase *, libBase, 56, Expat
);
static AROS_LD0(const XML_LChar *, ExpatVersion,
	struct ExpatBase *, libBase, 57, Expat
);
static AROS_LD0(XML_Expat_Version, ExpatVersionInfo,
	struct ExpatBase *, libBase, 58, Expat
);
static AROS_LD2(XML_Bool, ParserReset,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(const XML_Char *, encodingName, A1),
	struct ExpatBase *, libBase, 59, Expat
);
static AROS_LD2(void, SetSkippedEntityHandler,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_SkippedEntityHandler, handler, A1),
	struct ExpatBase *, libBase, 60, Expat
);
static AROS_LD2(int, UseForeignDTD,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_Bool, useDTD, D0),
	struct ExpatBase *, libBase, 61, Expat
);
static AROS_LD0(const XML_Feature *, GetFeatureList,
	struct ExpatBase *, libBase, 62, Expat
);
static AROS_LD2(int, StopParser,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_Bool, resumable, D0),
	struct ExpatBase *, libBase, 63, Expat
);
static AROS_LD1(int, ResumeParser,
	AROS_LPA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 64, Expat
);
static AROS_LD2(void, GetParsingStatus,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_ParsingStatus *, status, A1),
	struct ExpatBase *, libBase, 65, Expat
);
static AROS_LD2(void, FreeContentModel,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(XML_Content *, model, A1),
	struct ExpatBase *, libBase, 66, Expat
);
static AROS_LD2(void *, MemMalloc,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(size_t, size, D0),
	struct ExpatBase *, libBase, 67, Expat
);
static AROS_LD3(void *, MemRealloc,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(void *, ptr, A1),
	AROS_LPA(size_t, size, D0),
	struct ExpatBase *, libBase, 68, Expat
);
static AROS_LD2(void, MemFree,
	AROS_LPA(XML_Parser, parser, A0),
	AROS_LPA(void *, ptr, A1),
	struct ExpatBase *, libBase, 69, Expat
);
#else
static struct ExpatBase *LibInit (REG(d0, struct ExpatBase *libBase),
	REG(a0, BPTR seglist), REG(a6, struct ExecBase *exec_base));
static struct ExpatBase *Expat_LibOpen (REG(a6, struct ExpatBase *libBase),
	REG(d0, ULONG version));
static BPTR Expat_LibClose (REG(a6, struct ExpatBase *libBase));
static BPTR Expat_LibExpunge (REG(a6, struct ExpatBase *libBase));
static APTR Expat_LibReserved (REG(a6, struct ExpatBase *libBase));
static XML_Parser Expat_ParserCreate(REG(a0, const XML_Char * encodingName));
static XML_Parser Expat_ParserCreateNS(REG(a0, const XML_Char * encodingName),
	REG(d0, XML_Char nsSep));
static XML_Parser Expat_ParserCreate_MM(REG(a0, const XML_Char * encodingName),
	REG(a1, const XML_Memory_Handling_Suite * memsuite),
	REG(a2, const XML_Char * nameSep));
static XML_Parser Expat_ExternalEntityParserCreate(REG(a0, XML_Parser oldParser),
	REG(a1, const XML_Char * context), REG(a2, const XML_Char * encodingName));
static void Expat_ParserFree(REG(a0, XML_Parser parser));
static int Expat_Parse(REG(a0, XML_Parser parser), REG(a1, const char * s),
	REG(d0, int len), REG(d1, int isFinal));
static int Expat_ParseBuffer(REG(a0, XML_Parser parser), REG(d0, int len),
	REG(d1, int isFinal));
static void * Expat_GetBuffer(REG(a0, XML_Parser parser), REG(d0, int len));
static void Expat_SetStartElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartElementHandler start));
static void Expat_SetEndElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndElementHandler end));
static void Expat_SetElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartElementHandler start),
	REG(a2, XML_EndElementHandler end));
static void Expat_SetCharacterDataHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_CharacterDataHandler handler));
static void Expat_SetProcessingInstructionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ProcessingInstructionHandler handler));
static void Expat_SetCommentHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_CommentHandler handler));
static void Expat_SetStartCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartCdataSectionHandler start));
static void Expat_SetEndCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndCdataSectionHandler end));
static void Expat_SetCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartCdataSectionHandler start),
	REG(a2, XML_EndCdataSectionHandler end));
static void Expat_SetDefaultHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_DefaultHandler handler));
static void Expat_SetDefaultHandlerExpand(REG(a0, XML_Parser parser),
	REG(a1, XML_DefaultHandler handler));
static void Expat_SetExternalEntityRefHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ExternalEntityRefHandler handler));
static void Expat_SetExternalEntityRefHandlerArg(REG(a0, XML_Parser parser),
	REG(a1, void * arg));
static void Expat_SetUnknownEncodingHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_UnknownEncodingHandler handler), REG(a2, void * data));
static void Expat_SetStartNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartNamespaceDeclHandler start));
static void Expat_SetEndNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndNamespaceDeclHandler end));
static void Expat_SetNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartNamespaceDeclHandler start),
	REG(a2, XML_EndNamespaceDeclHandler end));
static void Expat_SetXmlDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_XmlDeclHandler handler));
static void Expat_SetStartDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartDoctypeDeclHandler start));
static void Expat_SetEndDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndDoctypeDeclHandler end));
static void Expat_SetDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartDoctypeDeclHandler start),
	REG(a2, XML_EndDoctypeDeclHandler end));
static void Expat_SetElementDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ElementDeclHandler eldecl));
static void Expat_SetAttlistDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_AttlistDeclHandler attdecl));
static void Expat_SetEntityDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EntityDeclHandler handler));
static void Expat_SetUnparsedEntityDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_UnparsedEntityDeclHandler handler));
static void Expat_SetNotationDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_NotationDeclHandler handler));
static void Expat_SetNotStandaloneHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_NotStandaloneHandler handler));
static int Expat_GetErrorCode(REG(a0, XML_Parser parser));
static const XML_LChar * Expat_ErrorString(REG(d0, int code));
static long Expat_GetCurrentByteIndex(REG(a0, XML_Parser parser));
static int Expat_GetCurrentLineNumber(REG(a0, XML_Parser parser));
static int Expat_GetCurrentColumnNumber(REG(a0, XML_Parser parser));
static int Expat_GetCurrentByteCount(REG(a0, XML_Parser parser));
static const char * Expat_GetInputContext(REG(a0, XML_Parser parser),
	REG(a1, int * offset), REG(a2, int * size));
static void Expat_SetUserData(REG(a0, XML_Parser parser), REG(a1, void * p));
static void Expat_DefaultCurrent(REG(a0, XML_Parser parser));
static void Expat_UseParserAsHandlerArg(REG(a0, XML_Parser parser));
static int Expat_SetBase(REG(a0, XML_Parser parser), REG(a1, const XML_Char * p));
static const XML_Char * Expat_GetBase(REG(a0, XML_Parser parser));
static int Expat_GetSpecifiedAttributeCount(REG(a0, XML_Parser parser));
static int Expat_GetIdAttributeIndex(REG(a0, XML_Parser parser));
static int Expat_SetEncoding(REG(a0, XML_Parser parser),
	REG(a1, const XML_Char * encodingName));
static int Expat_SetParamEntityParsing(REG(a0, XML_Parser parser),
	REG(d0, int peParsing));
static void Expat_SetReturnNSTriplet(REG(a0, XML_Parser parser), REG(d0, int do_nst));
static const XML_LChar * Expat_ExpatVersion();
static XML_Expat_Version Expat_ExpatVersionInfo();
static XML_Bool Expat_ParserReset(REG(a0, XML_Parser parser),
	REG(a1, const XML_Char * encodingName));
static void Expat_SetSkippedEntityHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_SkippedEntityHandler handler));
static int Expat_UseForeignDTD(REG(a0, XML_Parser parser), REG(d0, XML_Bool useDTD));
static const XML_Feature * Expat_GetFeatureList();
static int Expat_StopParser(REG(a0, XML_Parser parser), REG(d0, XML_Bool resumable));
static int Expat_ResumeParser(REG(a0, XML_Parser parser));
static void Expat_GetParsingStatus(REG(a0, XML_Parser parser),
	REG(a1, XML_ParsingStatus * status));
static void Expat_FreeContentModel(REG(a0, XML_Parser parser),
	REG(a1, XML_Content * model));
static void * Expat_MemMalloc(REG(a0, XML_Parser parser), REG(d0, size_t size));
static void * Expat_MemRealloc(REG(a0, XML_Parser parser), REG(a1, void * ptr),
	REG(d0, size_t size));
static void Expat_MemFree(REG(a0, XML_Parser parser), REG(a1, void * ptr));
#endif

#ifdef __AROS__
#ifdef ABIV1
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, Expat, b)
#else
#define LIB_ENTRY(a,b) AROS_SLIB_ENTRY(a, Expat)
#endif
#else
#define LIB_ENTRY(a,b) Expat_##a
#endif

CONST_APTR LibVectors[] = {
	(APTR)LIB_ENTRY(LibOpen, 1),
	(APTR)LIB_ENTRY(LibClose, 2),
	(APTR)LIB_ENTRY(LibExpunge, 3),
	(APTR)LIB_ENTRY(LibReserved, 4),
	(APTR)LIB_ENTRY(ParserCreate, 5),
	(APTR)LIB_ENTRY(ParserCreateNS, 6),
	(APTR)LIB_ENTRY(ParserCreate_MM, 7),
	(APTR)LIB_ENTRY(ExternalEntityParserCreate, 8),
	(APTR)LIB_ENTRY(ParserFree, 9),
	(APTR)LIB_ENTRY(Parse, 10),
	(APTR)LIB_ENTRY(ParseBuffer, 11),
	(APTR)LIB_ENTRY(GetBuffer, 12),
	(APTR)LIB_ENTRY(SetStartElementHandler, 13),
	(APTR)LIB_ENTRY(SetEndElementHandler, 14),
	(APTR)LIB_ENTRY(SetElementHandler, 15),
	(APTR)LIB_ENTRY(SetCharacterDataHandler, 16),
	(APTR)LIB_ENTRY(SetProcessingInstructionHandler, 17),
	(APTR)LIB_ENTRY(SetCommentHandler, 18),
	(APTR)LIB_ENTRY(SetStartCdataSectionHandler, 19),
	(APTR)LIB_ENTRY(SetEndCdataSectionHandler, 20),
	(APTR)LIB_ENTRY(SetCdataSectionHandler, 21),
	(APTR)LIB_ENTRY(SetDefaultHandler, 22),
	(APTR)LIB_ENTRY(SetDefaultHandlerExpand, 23),
	(APTR)LIB_ENTRY(SetExternalEntityRefHandler, 24),
	(APTR)LIB_ENTRY(SetExternalEntityRefHandlerArg, 25),
	(APTR)LIB_ENTRY(SetUnknownEncodingHandler, 26),
	(APTR)LIB_ENTRY(SetStartNamespaceDeclHandler, 27),
	(APTR)LIB_ENTRY(SetEndNamespaceDeclHandler, 28),
	(APTR)LIB_ENTRY(SetNamespaceDeclHandler, 29),
	(APTR)LIB_ENTRY(SetXmlDeclHandler, 30),
	(APTR)LIB_ENTRY(SetStartDoctypeDeclHandler, 31),
	(APTR)LIB_ENTRY(SetEndDoctypeDeclHandler, 32),
	(APTR)LIB_ENTRY(SetDoctypeDeclHandler, 33),
	(APTR)LIB_ENTRY(SetElementDeclHandler, 34),
	(APTR)LIB_ENTRY(SetAttlistDeclHandler, 35),
	(APTR)LIB_ENTRY(SetEntityDeclHandler, 36),
	(APTR)LIB_ENTRY(SetUnparsedEntityDeclHandler, 37),
	(APTR)LIB_ENTRY(SetNotationDeclHandler, 38),
	(APTR)LIB_ENTRY(SetNotStandaloneHandler, 39),
	(APTR)LIB_ENTRY(GetErrorCode, 40),
	(APTR)LIB_ENTRY(ErrorString, 41),
	(APTR)LIB_ENTRY(GetCurrentByteIndex, 42),
	(APTR)LIB_ENTRY(GetCurrentLineNumber, 43),
	(APTR)LIB_ENTRY(GetCurrentColumnNumber, 44),
	(APTR)LIB_ENTRY(GetCurrentByteCount, 45),
	(APTR)LIB_ENTRY(GetInputContext, 46),
	(APTR)LIB_ENTRY(SetUserData, 47),
	(APTR)LIB_ENTRY(DefaultCurrent, 48),
	(APTR)LIB_ENTRY(UseParserAsHandlerArg, 49),
	(APTR)LIB_ENTRY(SetBase, 50),
	(APTR)LIB_ENTRY(GetBase, 51),
	(APTR)LIB_ENTRY(GetSpecifiedAttributeCount, 52),
	(APTR)LIB_ENTRY(GetIdAttributeIndex, 53),
	(APTR)LIB_ENTRY(SetEncoding, 54),
	(APTR)LIB_ENTRY(SetParamEntityParsing, 55),
	(APTR)LIB_ENTRY(SetReturnNSTriplet, 56),
	(APTR)LIB_ENTRY(ExpatVersion, 57),
	(APTR)LIB_ENTRY(ExpatVersionInfo, 58),
	(APTR)LIB_ENTRY(ParserReset, 59),
	(APTR)LIB_ENTRY(SetSkippedEntityHandler, 60),
	(APTR)LIB_ENTRY(UseForeignDTD, 61),
	(APTR)LIB_ENTRY(GetFeatureList, 62),
	(APTR)LIB_ENTRY(StopParser, 63),
	(APTR)LIB_ENTRY(ResumeParser, 64),
	(APTR)LIB_ENTRY(GetParsingStatus, 65),
	(APTR)LIB_ENTRY(FreeContentModel, 66),
	(APTR)LIB_ENTRY(MemMalloc, 67),
	(APTR)LIB_ENTRY(MemRealloc, 68),
	(APTR)LIB_ENTRY(MemFree, 69),
	(APTR)-1
};

const IPTR LibInitTab[] = {
	sizeof(struct ExpatBase),
	(IPTR)LibVectors,
	(IPTR)NULL,
	(IPTR)LibInit
};

const struct Resident USED_VAR ROMTag = {
	RTC_MATCHWORD,
	(struct Resident *)&ROMTag,
	(APTR)(&ROMTag + 1),
	RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	(STRPTR)LIBNAME,
	(STRPTR)VSTRING,
	(APTR)LibInitTab
};

#ifdef __AROS__
static AROS_UFH3(struct ExpatBase *, LibInit,
	AROS_UFHA(struct ExpatBase *, libBase, D0),
	AROS_UFHA(BPTR, seglist, A0),
	AROS_UFHA(struct ExecBase *, exec_base, A6)
)
{
	AROS_USERFUNC_INIT
#else
static struct ExpatBase *LibInit (REG(d0, struct ExpatBase *libBase), REG(a0, BPTR seglist),
	REG(a6, struct ExecBase *exec_base))
{
#endif
	libBase->libNode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libNode.lib_Node.ln_Pri  = 0;
	libBase->libNode.lib_Node.ln_Name = LIBNAME;
	libBase->libNode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libNode.lib_Version      = VERSION;
	libBase->libNode.lib_Revision     = REVISION;
	libBase->libNode.lib_IdString     = VSTRING;

	SysBase = exec_base;
	libBase->seglist = seglist;
	
	if (malloc_init()) {
		return libBase;
	}

	DeleteLibrary((struct Library *)libBase);

	return NULL;
#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(struct ExpatBase *, LibOpen,
	AROS_LHA(ULONG, version, D0),
	struct ExpatBase *, libBase, 1, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static struct ExpatBase *Expat_LibOpen (REG(a6, struct ExpatBase *libBase), REG(d0, ULONG version)) {
#endif
	libBase->libNode.lib_OpenCnt++;
	libBase->libNode.lib_Flags &= ~LIBF_DELEXP;
	return libBase;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(BPTR, LibClose,
	struct ExpatBase *, libBase, 2, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR Expat_LibClose (REG(a6, struct ExpatBase *libBase)) {
#endif
	libBase->libNode.lib_OpenCnt--;
	return 0;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(BPTR, LibExpunge,
	struct ExpatBase *, libBase, 3, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static BPTR Expat_LibExpunge (REG(a6, struct ExpatBase *libBase)) {
#endif
	BPTR result = 0;

	if (libBase->libNode.lib_OpenCnt > 0) {
		libBase->libNode.lib_Flags |= LIBF_DELEXP;
		return 0;
	}

	Remove(&libBase->libNode.lib_Node);

	result = libBase->seglist;
	
	malloc_exit();

	DeleteLibrary((struct Library *)libBase);

	return result;
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(APTR, LibReserved,
	struct ExpatBase *, libBase, 4, Expat
)
{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
}
#else
static APTR Expat_LibReserved (REG(a6, struct ExpatBase *libBase)) {
	return NULL;
}
#endif

#ifdef __AROS__
static AROS_LH1(XML_Parser, ParserCreate,
	AROS_LHA(const XML_Char *, encodingName, A0),
	struct ExpatBase *, libBase, 5, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Parser Expat_ParserCreate(REG(a0, const XML_Char * encodingName))
{
#endif
	return XML_ParserCreate(encodingName);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(XML_Parser, ParserCreateNS,
	AROS_LHA(const XML_Char *, encodingName, A0),
	AROS_LHA(XML_Char, nsSep, D0),
	struct ExpatBase *, libBase, 6, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Parser Expat_ParserCreateNS(REG(a0, const XML_Char * encodingName),
	REG(d0, XML_Char nsSep))
{
#endif
	return XML_ParserCreateNS(encodingName, nsSep);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(XML_Parser, ParserCreate_MM,
	AROS_LHA(const XML_Char *, encodingName, A0),
	AROS_LHA(const XML_Memory_Handling_Suite *, memsuite, A1),
	AROS_LHA(const XML_Char *, nameSep, A2),
	struct ExpatBase *, libBase, 7, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Parser Expat_ParserCreate_MM(REG(a0, const XML_Char * encodingName),
	REG(a1, const XML_Memory_Handling_Suite * memsuite),
	REG(a2, const XML_Char * nameSep))
{
#endif
	return XML_ParserCreate_MM(encodingName, memsuite, nameSep);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(XML_Parser, ExternalEntityParserCreate,
	AROS_LHA(XML_Parser, oldParser, A0),
	AROS_LHA(const XML_Char *, context, A1),
	AROS_LHA(const XML_Char *, encodingName, A2),
	struct ExpatBase *, libBase, 8, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Parser Expat_ExternalEntityParserCreate(REG(a0, XML_Parser oldParser),
	REG(a1, const XML_Char * context), REG(a2, const XML_Char * encodingName))
{
#endif
	return XML_ExternalEntityParserCreate(oldParser, context, encodingName);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(void, ParserFree,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 9, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_ParserFree(REG(a0, XML_Parser parser))
{
#endif
	XML_ParserFree(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH4(int, Parse,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(const char *, s, A1),
	AROS_LHA(int, len, D0),
	AROS_LHA(int, isFinal, D1),
	struct ExpatBase *, libBase, 10, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_Parse(REG(a0, XML_Parser parser), REG(a1, const char * s),
	REG(d0, int len), REG(d1, int isFinal))
{
#endif
	return XML_Parse(parser, s, len, isFinal);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(int, ParseBuffer,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(int, len, D0),
	AROS_LHA(int, isFinal, D1),
	struct ExpatBase *, libBase, 11, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_ParseBuffer(REG(a0, XML_Parser parser), REG(d0, int len),
	REG(d1, int isFinal))
{
#endif
	return XML_ParseBuffer(parser, len, isFinal);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void *, GetBuffer,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(int, len, D0),
	struct ExpatBase *, libBase, 12, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void * Expat_GetBuffer(REG(a0, XML_Parser parser), REG(d0, int len))
{
#endif
	return XML_GetBuffer(parser, len);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetStartElementHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartElementHandler, start, A1),
	struct ExpatBase *, libBase, 13, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetStartElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartElementHandler start))
{
#endif
	XML_SetStartElementHandler(parser, start);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetEndElementHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_EndElementHandler, end, A1),
	struct ExpatBase *, libBase, 14, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetEndElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndElementHandler end))
{
#endif
	XML_SetEndElementHandler(parser, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void, SetElementHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartElementHandler, start, A1),
	AROS_LHA(XML_EndElementHandler, end, A2),
	struct ExpatBase *, libBase, 15, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetElementHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartElementHandler start),
	REG(a2, XML_EndElementHandler end))
{
#endif
	XML_SetElementHandler(parser, start, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetCharacterDataHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_CharacterDataHandler, handler, A1),
	struct ExpatBase *, libBase, 16, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetCharacterDataHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_CharacterDataHandler handler))
{
#endif
	XML_SetCharacterDataHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetProcessingInstructionHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_ProcessingInstructionHandler, handler, A1),
	struct ExpatBase *, libBase, 17, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetProcessingInstructionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ProcessingInstructionHandler handler))
{
#endif
	XML_SetProcessingInstructionHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetCommentHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_CommentHandler, handler, A1),
	struct ExpatBase *, libBase, 18, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetCommentHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_CommentHandler handler))
{
#endif
	XML_SetCommentHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetStartCdataSectionHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartCdataSectionHandler, start, A1),
	struct ExpatBase *, libBase, 19, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetStartCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartCdataSectionHandler start))
{
#endif
	XML_SetStartCdataSectionHandler(parser, start);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetEndCdataSectionHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_EndCdataSectionHandler, end, A1),
	struct ExpatBase *, libBase, 20, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetEndCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndCdataSectionHandler end))
{
#endif
	XML_SetEndCdataSectionHandler(parser, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void, SetCdataSectionHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartCdataSectionHandler, start, A1),
	AROS_LHA(XML_EndCdataSectionHandler, end, A2),
	struct ExpatBase *, libBase, 21, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetCdataSectionHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartCdataSectionHandler start),
	REG(a2, XML_EndCdataSectionHandler end))
{
#endif
	XML_SetCdataSectionHandler(parser, start, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetDefaultHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_DefaultHandler, handler, A1),
	struct ExpatBase *, libBase, 22, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetDefaultHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_DefaultHandler handler))
{
#endif
	XML_SetDefaultHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetDefaultHandlerExpand,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_DefaultHandler, handler, A1),
	struct ExpatBase *, libBase, 23, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetDefaultHandlerExpand(REG(a0, XML_Parser parser),
	REG(a1, XML_DefaultHandler handler))
{
#endif
	XML_SetDefaultHandlerExpand(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetExternalEntityRefHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_ExternalEntityRefHandler, handler, A1),
	struct ExpatBase *, libBase, 24, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetExternalEntityRefHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ExternalEntityRefHandler handler))
{
#endif
	XML_SetExternalEntityRefHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetExternalEntityRefHandlerArg,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(void *, arg, A1),
	struct ExpatBase *, libBase, 25, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetExternalEntityRefHandlerArg(REG(a0, XML_Parser parser),
	REG(a1, void * arg))
{
#endif
	XML_SetExternalEntityRefHandlerArg(parser, arg);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void, SetUnknownEncodingHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_UnknownEncodingHandler, handler, A1),
	AROS_LHA(void *, data, A2),
	struct ExpatBase *, libBase, 26, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetUnknownEncodingHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_UnknownEncodingHandler handler), REG(a2, void * data))
{
#endif
	XML_SetUnknownEncodingHandler(parser, handler, data);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetStartNamespaceDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartNamespaceDeclHandler, start, A1),
	struct ExpatBase *, libBase, 27, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetStartNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartNamespaceDeclHandler start))
{
#endif
	XML_SetStartNamespaceDeclHandler(parser, start);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetEndNamespaceDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_EndNamespaceDeclHandler, end, A1),
	struct ExpatBase *, libBase, 28, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetEndNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndNamespaceDeclHandler end))
{
#endif
	XML_SetEndNamespaceDeclHandler(parser, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void, SetNamespaceDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartNamespaceDeclHandler, start, A1),
	AROS_LHA(XML_EndNamespaceDeclHandler, end, A2),
	struct ExpatBase *, libBase, 29, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetNamespaceDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartNamespaceDeclHandler start),
	REG(a2, XML_EndNamespaceDeclHandler end))
{
#endif
	XML_SetNamespaceDeclHandler(parser, start, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetXmlDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_XmlDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 30, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetXmlDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_XmlDeclHandler handler))
{
#endif
	XML_SetXmlDeclHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetStartDoctypeDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartDoctypeDeclHandler, start, A1),
	struct ExpatBase *, libBase, 31, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetStartDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartDoctypeDeclHandler start))
{
#endif
	XML_SetStartDoctypeDeclHandler(parser, start);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetEndDoctypeDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_EndDoctypeDeclHandler, end, A1),
	struct ExpatBase *, libBase, 32, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetEndDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EndDoctypeDeclHandler end))
{
#endif
	XML_SetEndDoctypeDeclHandler(parser, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void, SetDoctypeDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_StartDoctypeDeclHandler, start, A1),
	AROS_LHA(XML_EndDoctypeDeclHandler, end, A2),
	struct ExpatBase *, libBase, 33, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetDoctypeDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_StartDoctypeDeclHandler start),
	REG(a2, XML_EndDoctypeDeclHandler end))
{
#endif
	XML_SetDoctypeDeclHandler(parser, start, end);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetElementDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_ElementDeclHandler, eldecl, A1),
	struct ExpatBase *, libBase, 34, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetElementDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_ElementDeclHandler eldecl))
{
#endif
	XML_SetElementDeclHandler(parser, eldecl);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetAttlistDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_AttlistDeclHandler, attdecl, A1),
	struct ExpatBase *, libBase, 35, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetAttlistDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_AttlistDeclHandler attdecl))
{
#endif
	XML_SetAttlistDeclHandler(parser, attdecl);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetEntityDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_EntityDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 36, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetEntityDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_EntityDeclHandler handler))
{
#endif
	XML_SetEntityDeclHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetUnparsedEntityDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_UnparsedEntityDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 37, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetUnparsedEntityDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_UnparsedEntityDeclHandler handler))
{
#endif
	XML_SetUnparsedEntityDeclHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetNotationDeclHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_NotationDeclHandler, handler, A1),
	struct ExpatBase *, libBase, 38, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetNotationDeclHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_NotationDeclHandler handler))
{
#endif
	XML_SetNotationDeclHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetNotStandaloneHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_NotStandaloneHandler, handler, A1),
	struct ExpatBase *, libBase, 39, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetNotStandaloneHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_NotStandaloneHandler handler))
{
#endif
	XML_SetNotStandaloneHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetErrorCode,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 40, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetErrorCode(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetErrorCode(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(const XML_LChar *, ErrorString,
	AROS_LHA(int, code, D0),
	struct ExpatBase *, libBase, 41, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static const XML_LChar * Expat_ErrorString(REG(d0, int code))
{
#endif
	return XML_ErrorString(code);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(long, GetCurrentByteIndex,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 42, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static long Expat_GetCurrentByteIndex(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetCurrentByteIndex(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetCurrentLineNumber,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 43, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetCurrentLineNumber(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetCurrentLineNumber(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetCurrentColumnNumber,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 44, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetCurrentColumnNumber(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetCurrentColumnNumber(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetCurrentByteCount,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 45, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetCurrentByteCount(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetCurrentByteCount(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(const char *, GetInputContext,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(int *, offset, A1),
	AROS_LHA(int *, size, A2),
	struct ExpatBase *, libBase, 46, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static const char * Expat_GetInputContext(REG(a0, XML_Parser parser),
	REG(a1, int * offset), REG(a2, int * size))
{
#endif
	return XML_GetInputContext(parser, offset, size);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetUserData,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(void *, p, A1),
	struct ExpatBase *, libBase, 47, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetUserData(REG(a0, XML_Parser parser), REG(a1, void * p))
{
#endif
	XML_SetUserData(parser, p);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(void, DefaultCurrent,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 48, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_DefaultCurrent(REG(a0, XML_Parser parser))
{
#endif
	XML_DefaultCurrent(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(void, UseParserAsHandlerArg,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 49, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_UseParserAsHandlerArg(REG(a0, XML_Parser parser))
{
#endif
	XML_UseParserAsHandlerArg(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(int, SetBase,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(const XML_Char *, p, A1),
	struct ExpatBase *, libBase, 50, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_SetBase(REG(a0, XML_Parser parser), REG(a1, const XML_Char * p))
{
#endif
	return XML_SetBase(parser, p);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(const XML_Char *, GetBase,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 51, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static const XML_Char * Expat_GetBase(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetBase(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetSpecifiedAttributeCount,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 52, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetSpecifiedAttributeCount(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetSpecifiedAttributeCount(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, GetIdAttributeIndex,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 53, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_GetIdAttributeIndex(REG(a0, XML_Parser parser))
{
#endif
	return XML_GetIdAttributeIndex(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(int, SetEncoding,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(const XML_Char *, encodingName, A1),
	struct ExpatBase *, libBase, 54, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_SetEncoding(REG(a0, XML_Parser parser),
	REG(a1, const XML_Char * encodingName))
{
#endif
	return XML_SetEncoding(parser, encodingName);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(int, SetParamEntityParsing,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(int, peParsing, D0),
	struct ExpatBase *, libBase, 55, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_SetParamEntityParsing(REG(a0, XML_Parser parser),
	REG(d0, int peParsing))
{
#endif
	return XML_SetParamEntityParsing(parser, peParsing);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetReturnNSTriplet,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(int, do_nst, D0),
	struct ExpatBase *, libBase, 56, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetReturnNSTriplet(REG(a0, XML_Parser parser), REG(d0, int do_nst))
{
#endif
	XML_SetReturnNSTriplet(parser, do_nst);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(const XML_LChar *, ExpatVersion,
	struct ExpatBase *, libBase, 57, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static const XML_LChar * Expat_ExpatVersion()
{
#endif
	return XML_ExpatVersion();
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(XML_Expat_Version, ExpatVersionInfo,
	struct ExpatBase *, libBase, 58, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Expat_Version Expat_ExpatVersionInfo()
{
#endif
	return XML_ExpatVersionInfo();
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(XML_Bool, ParserReset,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(const XML_Char *, encodingName, A1),
	struct ExpatBase *, libBase, 59, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static XML_Bool Expat_ParserReset(REG(a0, XML_Parser parser),
	REG(a1, const XML_Char * encodingName))
{
#endif
	return XML_ParserReset(parser, encodingName);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, SetSkippedEntityHandler,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_SkippedEntityHandler, handler, A1),
	struct ExpatBase *, libBase, 60, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_SetSkippedEntityHandler(REG(a0, XML_Parser parser),
	REG(a1, XML_SkippedEntityHandler handler))
{
#endif
	XML_SetSkippedEntityHandler(parser, handler);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(int, UseForeignDTD,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_Bool, useDTD, D0),
	struct ExpatBase *, libBase, 61, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_UseForeignDTD(REG(a0, XML_Parser parser), REG(d0, XML_Bool useDTD))
{
#endif
	return XML_UseForeignDTD(parser, useDTD);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH0(const XML_Feature *, GetFeatureList,
	struct ExpatBase *, libBase, 62, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static const XML_Feature * Expat_GetFeatureList()
{
#endif
	return XML_GetFeatureList();
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(int, StopParser,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_Bool, resumable, D0),
	struct ExpatBase *, libBase, 63, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_StopParser(REG(a0, XML_Parser parser), REG(d0, XML_Bool resumable))
{
#endif
	return XML_StopParser(parser, resumable);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH1(int, ResumeParser,
	AROS_LHA(XML_Parser, parser, A0),
	struct ExpatBase *, libBase, 64, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static int Expat_ResumeParser(REG(a0, XML_Parser parser))
{
#endif
	return XML_ResumeParser(parser);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, GetParsingStatus,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_ParsingStatus *, status, A1),
	struct ExpatBase *, libBase, 65, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_GetParsingStatus(REG(a0, XML_Parser parser),
	REG(a1, XML_ParsingStatus * status))
{
#endif
	XML_GetParsingStatus(parser, status);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, FreeContentModel,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(XML_Content *, model, A1),
	struct ExpatBase *, libBase, 66, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_FreeContentModel(REG(a0, XML_Parser parser),
	REG(a1, XML_Content * model))
{
#endif
	XML_FreeContentModel(parser, model);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void *, MemMalloc,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(size_t, size, D0),
	struct ExpatBase *, libBase, 67, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void * Expat_MemMalloc(REG(a0, XML_Parser parser), REG(d0, size_t size))
{
#endif
	return XML_MemMalloc(parser, size);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH3(void *, MemRealloc,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(void *, ptr, A1),
	AROS_LHA(size_t, size, D0),
	struct ExpatBase *, libBase, 68, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void * Expat_MemRealloc(REG(a0, XML_Parser parser), REG(a1, void * ptr),
	REG(d0, size_t size))
{
#endif
	return XML_MemRealloc(parser, ptr, size);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
static AROS_LH2(void, MemFree,
	AROS_LHA(XML_Parser, parser, A0),
	AROS_LHA(void *, ptr, A1),
	struct ExpatBase *, libBase, 69, Expat
)
{
	AROS_LIBFUNC_INIT
#else
static void Expat_MemFree(REG(a0, XML_Parser parser), REG(a1, void * ptr))
{
#endif
	return XML_MemFree(parser, ptr);
#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}
