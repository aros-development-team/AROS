/*
** $PROJECT: amigaguide.datatype
**
** $VER: file.c 50.1 (10.09.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include "classbase.h"

/* ------------------------------- defines -------------------------------- */

struct AmigaGuideFile *AllocAGFile(Class *cl, Object *obj)
{
   CLASSBASE;
   INSTDATA;
   struct AmigaGuideFile *agf;

   agf = AllocAGMem(cl, obj, sizeof(struct AmigaGuideFile));
   if(agf != NULL)
   {
      NewList(&agf->agf_Nodes);
      NewList(&agf->agf_Macros);

      /* default tabulator width is 8 spaces */
      agf->agf_TabWidth = 8;
      /* standard "terminal dimension */
      agf->agf_Width    = 80;
      agf->agf_Height   = 40;
      /* default word delimiters for this guide */
      agf->agf_WordDelim = "\t *-,<>()[]{};.\"";

      AddTail(&data->ag_Files, &agf->agf_Node);
   }
   return agf;
}

void AddMacro(Class *cl, Object *obj, struct AmigaGuideFile *agf,
	      STRPTR name, ULONG namelen,
	      STRPTR macro, ULONG macrolen)
{
   CLASSBASE;

   struct AmigaGuideMacro *mac;
   struct Node *n;
   STRPTR macrostring;

   /* allocate and copy macro string */
   if((macrostring = AllocAGVec(cl, obj, macrolen + 1)) != NULL)
      strncpy(macrostring,macro,macrolen);

   /* try to find given macro name and if a macro of that name is already
      there replace the macro string. */
   for(n = agf->agf_Macros.lh_Head ; n->ln_Succ != NULL ; n = n->ln_Succ)
      if(!Strnicmp(n->ln_Name,name,namelen) && strlen(n->ln_Name) == namelen)
      {
	 mac = (struct AmigaGuideMacro *) n;
	 FreeAGVec(cl, obj, mac->agm_Macro);
	 mac->agm_Macro = macrostring;
	 return;
      }

   /* now allocate memory for the macro node and the macro name */
   if((mac = AllocAGVec(cl, obj, sizeof(struct AmigaGuideMacro) + namelen + 1)) != NULL)
   {
      /* copy the macro name. */
      mac->agm_Node.ln_Name = (STRPTR) (mac + 1);
      strncpy(mac->agm_Node.ln_Name, name, namelen);
      mac->agm_Macro = macrostring;

      /* link into macro list */
      AddHead(&agf->agf_Macros, &mac->agm_Node);

      DB(("macro name:   \"%s\"\n",mac->agm_Node.ln_Name));
      DB(("macro string: \"%s\"\n",mac->agm_Macro));
   }
}

static
struct AmigaGuideNode *
ParseNodeLine(Class *cl, Object *obj, struct AmigaGuideFile *agf,
	      STRPTR args, LONG linelen)
{
   CLASSBASE;
   struct AmigaGuideNode *agn;
   ULONG size = sizeof(struct AmigaGuideNode);
   STRPTR name;
   STRPTR ptr=args;

   if(*ptr == '"')
   {
      name = ++ptr;
      while(*ptr != '"' && *ptr != '\n')
	 ptr++;
   } else
   {
      name = ptr;
      while(*ptr != ' ' && *ptr != '\t' && *ptr != '\n')
	 ptr++;
   }
   *ptr++ = '\0';
   size += ptr - name;

   if((agn = AllocAGVec(cl, obj, size)) != NULL)
   {
      agn->agn_Node.ln_Name = (STRPTR) (agn + 1);
      strcpy(agn->agn_Node.ln_Name, name);

      /* CHECK: possible Flush(fh) ? */
      agn->agn_Pos = Seek(agf->agf_Handle, 0, OFFSET_CURRENT) - linelen;
      agn->agn_File = agf;
      ++agf->agf_NodeCount;
      AddTail(&agf->agf_Nodes, &agn->agn_Node);

#if 0
      DB(("node found : %s\n",name));
      DB(("pos : %ld\n",agn->agn_Pos));
#endif
   }
   return agn;
}


static
void ParseMacroLine(Class *cl, Object *obj,
                    struct AmigaGuideFile *agf, STRPTR args)
{
   STRPTR ptr  = args;
   STRPTR name = ptr;
   ULONG namelen;
   STRPTR macro;

   while(*ptr != ' ' && *ptr != '\t' && *ptr != '\n')
      ptr++;
   namelen = ptr - name;
   ptr = eatws(ptr);

   if(*ptr == '"')
      ptr++;
   macro = ptr;

   while(*ptr != '\n' && *ptr != '"')
   {
      if(*ptr == '\\')
	 if(*(++ptr) == '\n')
	    break;
      ptr++;
   }

   if(*ptr == '"' && ptr[1] != '\n' && ptr[1] != '\0')
   {
      ptr = eatws(++ptr);
      if(*ptr != '\n' && *ptr != '\0')
      {
	 while(*ptr != '\0' && *ptr != '\n')
	    ptr++;
	 *ptr = '\0';

	 DB(("Macro string wrong: \"%s\"!\n",macro));
	 macro = "";
	 ptr   = macro;
      }
   }

   if(macro != NULL)
      AddMacro(cl, obj, agf, name, namelen, macro, ptr-macro);
}

/****** amigaguide.datatype/--commands-- ***********************************

    NAME
	AmigaGuide commands

    DESCRIPTION
        The following commands can be used within the global section of an
        AmigaGuide database.

    GLOBAL COMMANDS

        @$VER: <string> -- AmigaOS version string

        @(C) <string> -- Copyright notice for the AmigaGuide database

        @AMIGAGUIDE <file> -- file name of this AmigaGuide database.

        @AUTHOR <string> -- string describing the author of this AmigaGuide
            database.
        @DATABASE <file> -- file name of this AmigaGuide database.

        @DNODE -- obsolete

        @FONT <name> <size> -- specifies the font to be used for the whole
            AmigaGuide database.

        @HEIGHT <lines> -- specifies the nominal height of the document. This
            height is used to open an appropriate sized window.

        @HELP <file/node> -- specifies the help file or node for this
            AmigaGuide database.

        @INDEX <file/node> -- specifies the index file or node for this
            AmigaGuide database.

        @MACRO <name> <macro> -- defines a macro for this AmigaGuide database.
            This macro can be used as a normal AmigaGuide attribute. It
            supports arguments which can be accessed through "$1", "$2", etc.
            Maximal number of supported arguments are 10.

        @MASTER <file> -- specifies the name of the source for the AmigaGuide
            database.

        @NODE <name> [<title>] -- defines a new node named <name> within the
            AmigaGuide database. The title is optional.

        @ONCLOSE <file>-- specifies an ARexx script which is executed when the
            AmigaGuide database is closed.

        @ONOPEN <file> -- specifies an ARexx script which is executed when the
            AmigaGuide database is opened. If the script returns an error
            code the database will not be opened.

        @REM <string> -- remark line completely ignored.

        @REMARK <string> -- remark line completely ignored.

        @SMARTWRAP -- turns smart-wrapping for all nodes in the database on.

        @TAB <num> -- specifies the number of space to use for a regular tab
            stop

        @TOC <file/node> -- specifies the contents file or node for this
            AmigaGuide database.

        @WIDTH <chars> -- specifies the nominal width of the document. This
            width is used to open an appropriate sized window.

        @WORDWRAP -- turns word-wrapping for all nodes in the database on.

        @WORDDELIM -- specified word delimiter charaters for this database.
            Default is "\t *-,<>()[]{};.\"". New for V50.

    NODE COMMANDS

        @ENDNODE -- indicates the end of a node.

        @FONT <name> <size> -- specifies the font to be used for this node.

        @HELP <file/node> -- specifies the help file or node for this node.

        @INDEX <file/node> -- specifies the index file or node for this node.

        @KEYWORDS <keywords> -- whitespace or comma separated list of keywords
            which are used to find this node during a DTM_GOTO method.

        @NEXT <file/node> -- specifies the next logical file or node for this
            node.

        @ONCLOSE <file>-- specifies an ARexx script which is executed when
            this node is closed.

        @ONOPEN <file> -- specifies an ARexx script which is executed when the
            node is opened. If the script returns an error code the node
            will not be opened.

        @PREV <file/node> -- specifies the previous logical file or node for
            this node.

        @REM <string> -- remark line completely ignored.

        @REMARK <string> -- remark line completely ignored.

        @SMARTWRAP -- turns smart-wrapping for this node on.

        @TAB <num> -- specifies the number of space to use for a regular tab
            stop.

        @TITLE <title> -- specifies the title for this node.

        @TOC <file/node> -- specifies the contents file or node for this node.

        @WORDWRAP -- turns word-wrapping for all nodes in the database on.

    SEE ALSO
        --attribites--

******************************************************************************
*
*/

void ScanFile(Class *cl, Object *obj, struct AmigaGuideFile *agf)
{
   CLASSBASE;
   STRPTR buf;

   if((buf = AllocAGMem(cl, obj, AG_BUFSIZE_LINE)) != NULL)
   {
      struct AmigaGuideNode *agn = NULL;
      BOOL global = TRUE;
      BPTR fh = agf->agf_Handle;
      STRPTR ptr;

      while((ptr = FGets(fh, buf, AG_BUFSIZE_LINE)) != NULL)
      {
	 if(*ptr == '@' && ptr[1] != '{')
	 {
	    ptr++;

	    if(global)
	    {
	       STRPTR args = NULL;
	       LONG val = 0;

	       switch(GetGlobalCommand(cb, ptr, &args))
	       {
	       case CMD_NODE:
		  global = FALSE; /* first node terminates global section */
		  agn = ParseNodeLine(cl, obj, agf, args, strlen(buf));
		  break;
	       case CMD_MACRO:
		  ParseMacroLine(cl, obj, agf, args);
		  break;
	       case CMD_HELP:
		  agf->agf_Help = CopyAGString(cl, obj, args);
		  break;
	       case CMD_INDEX:
		  agf->agf_Index = CopyAGString(cl, obj, args);
		  break;
	       case CMD_TOC:
		  agf->agf_TOC = CopyAGString(cl, obj, args);
		  break;
	       case CMD_COPYRIGHT:
		  agf->agf_Copyright = CopyAGString(cl, obj, args);
		  break;
	       case CMD_VERSION:
		  agf->agf_Version = CopyAGString(cl, obj, args);
		  break;
	       case CMD_AUTHOR:
		  agf->agf_Author = CopyAGString(cl, obj, args);
		  break;
	       case CMD_DATABASE:
		  agf->agf_Name = CopyAGString(cl, obj, args);
		  break;
	       case CMD_FONT:
                  agf->agf_Font = CopyAGString(cl, obj, args);
                  DB(("global font: %s\n", agf->agf_Font));
		  break;
               case CMD_WORDDELIM:
                  agf->agf_WordDelim = CopyAGString(cl, obj, args);
                  break;
	       case CMD_MASTER:
		  break;
	       case CMD_ONCLOSE:
                  agf->agf_OnClose = CopyAGString(cl, obj, args);
		  break;
	       case CMD_ONOPEN:
                  agf->agf_OnOpen = CopyAGString(cl, obj, args);
		  break;
               case CMD_WIDTH:
		  if(StrToLong(args, &val) > 0)
		     agf->agf_Width = val;
                  break;
               case CMD_HEIGHT:
		  if(StrToLong(args, &val) > 0)
		     agf->agf_Height = val;
                  break;
	       case CMD_REMARK:
		  break;
	       case CMD_WORDWRAP:
		  DB(("set wordwrap !\n"));
		  agf->agf_Flags.WordWrap  = TRUE;
	       case CMD_SMARTWRAP:
		  DB(("set smartwrap !\n"));
		  agf->agf_Flags.SmartWrap = TRUE;
		  break;
	       case CMD_TAB:
		  if(StrToLong(args, &val) > 0)
		     agf->agf_TabWidth = val;
                  DB(("gloval tabwidth=%ld\n", agf->agf_TabWidth));
		  break;
	       default:
		  DB(("\"%s\" unknown command!\n",ptr));
	       }
	    } else if(!Strnicmp(ptr, "node", 4))
	    {
	       ptr += 4;
	       ptr = eatws(ptr);

	       agn = ParseNodeLine(cl, obj, agf, ptr, strlen(buf));
	    } else if(agn != NULL)
	    {
	       STRPTR args = NULL;

	       switch(GetNodeCommand(cb, ptr, &args))
	       {
	       case CMD_ENDNODE:
		  /* CHECK: possible Flush(fh); ? */
		  /* calculate length of node */
		  agn->agn_Length = Seek(fh, 0, OFFSET_CURRENT) - agn->agn_Pos - strlen(buf) - 1;
		  agn = NULL;
		  break;
	       case CMD_KEYWORDS:
		  agn->agn_Keywords = CopyAGString(cl, obj, args);
		  break;
               case CMD_FONT:
                  agn->agn_Font = CopyAGString(cl, obj, args);
                  DB(("node font: %s\n", agn->agn_Font));
                  break;
	       }
	    }
	 }
      }

      FreeAGMem(cl, obj, buf, AG_BUFSIZE_LINE);
   }
}

