/*
** $PROJECT: amigaguide.datatype
**
** $VER: nodeclass.c 50.1 (08.09.03)
**
** $AUTHOR: Stefan Ruppert <stefan@ruppert-it.de>
**
*/

/* ------------------------------- includes ------------------------------- */

#include "classbase.h"

#include <datatypes/textclass.h>

/* -------------------------- private structures -------------------------- */

#define LNF_JMASK    (0x0300)
#define LNF_JLEFT    (0x0000)
#define LNF_JRIGHT   (0x0100)
#define LNF_JCENTER  (0x0200)

#define MAX_MACRO_ARGS 10

struct NodeData
{
   APTR n_Pool;
   APTR n_LinePool;

   STRPTR n_Buffer;
   ULONG n_BufferLen;

   Object *n_RootObject;
   struct AmigaGuideFile *n_AGFile;


   STRPTR n_WordDelim;

   struct
   {
      ULONG SmartWrap : 1;
      ULONG WordWrap  : 1;
   } n_Flags;

   UWORD n_TabWidth;

   STRPTR n_Title;
   STRPTR n_TOC;
   STRPTR n_Index;
   STRPTR n_Help;
   STRPTR n_Prev;
   STRPTR n_Next;
};

#define MAX_TABS 20

struct ParseNodeData
{
   /* ParseNode info vars */
   struct List *p_List;
   struct TextFont *p_Font;
   struct IBox *p_Domain;
   struct DrawInfo *p_DrawInfo;

   STRPTR p_End;

   LONG p_SpcWidth;
   LONG p_TabWidth;
   LONG p_Width;

   /* paragraph indention state info */
   LONG p_BodyLineIndent;
   LONG p_FirstLineIndent;

   /* custom tabulator support */
   UWORD *p_CTabs;         /* if != NULL use custom tabs */
   UWORD p_CustomTab;      /* current custom tab in line */
   UWORD p_CustomTabMax;   /* number of valid tab stops in array */
   UWORD p_CustomTabArray[MAX_TABS];

   /* vars storing current parsing state */
   struct Line *p_LastLine;
   LONG p_CurWidth;
   UWORD p_FgPen;
   UWORD p_BgPen;
   UWORD p_Style;
   UWORD p_Justification;
   LONG p_LineIndent;
   LONG p_Lines;
   LONG p_Links;
   LONG p_Newline;
   LONG p_MaxWidth;

   struct
   {
      ULONG WordWrap : 1;
      ULONG SmartWrap : 1;
   } p_Flags;

   /* tempory rastport for calculating text length */
   struct RastPort p_RPort;
};

/****** amigaguide.datatype/--attributes-- ***********************************

    NAME
	AmigaGuide attributes

    DESCRIPTION
        The following attributes can be applied to the text of each AmigaGuide
        node.

        @{AMIGAGUIDE} -- displays the word AmigaGuide® in bold font.

        @{APEN <pen num>} -- sets the foreground color to a specific pen
            number.

        @{B} -- turns bold font style on.

        @{BG <color>} -- change the background text color. One of the
            following aliases can be used:
                Text
                Shine
                Shadow
                Fill
                FillText
                Background
                Highlight

        @{BODY} -- indicate that the following text is the body of the
            document. Word and/or smart wrap will be turned back on if it's
            the default.

        @{BPEN <pen num>} -- sets the background color to a specific pen
            number.

        @{CLEARTABS} -- clears any custom tab stops previously set with
            @{SETTABS ...}.

        @{CODE} -- indicate that the following text should not be word- and/or
            smart-wrapped.

        @{FG <color>} -- change the foreground text color. One of the
            following aliases can be used:
                Text
                Shine
                Shadow
                Fill
                FillText
                Background
                Highlight

        @{I} -- turns italic font style on.

        @{JCENTER} -- turns centering of text on.

        @{JLEFT} -- turns left justification on.

        @{JRIGHT} -- turns right justification on.

        @{LINDENT <num spaces>} -- sets the number of spaces to indent the
            body of paragraph.

        @{LINE} -- force a line feed without starting a new paragraph.

        @{PAR} -- terminates a paragraph. Same as two sequential line feeds
            in the node text.

        @{PARD} -- restore the default settings for a paragraph:
            Text pen to 1, background pen to 0, normal font, and no
            indentation.

        @{PARI <num spaces>} -- set the number of spaces to indent the first
            line of a paragraph relative to the normal paragraph indentation.
            The value may be a negative number.

        @{PLAIN} -- resets font style to normal style.
            Same as "@{UB}@{UI}@{UU}"

        @{SETTABS <tab1> ... <tabN>} -- defines custom tab stops. Currently
            the maximum number of supported tab stops is 20.

        @{TAB} -- same as the ASCII character 9 in the source node text.

        @{U} -- turn underlining font style on.

        @{UB} -- turn bold font style off.

        @{UI} -- turn italic font style off.

        @{UU} -- tund underlining font style off.

    SEE ALSO
        --commands--

******************************************************************************
*
*/

static
const struct AmigaGuideAttr amigaguide_attrs[] =
{
   {"amigaguide", ATTR_AMIGAGUIDE},
   {"apen",    ATTR_APEN},
   {"b",       ATTR_B},
   {"bg",      ATTR_BG},
   {"body",    ATTR_BODY},
   {"bpen",    ATTR_BPEN},
   {"cleartabs", ATTR_CLEARTABS},
   {"code",    ATTR_CODE},
   {"fg",      ATTR_FG},
   {"i",       ATTR_I},
   {"jcenter", ATTR_JCENTER},
   {"jleft",   ATTR_JLEFT},
   {"jright",  ATTR_JRIGHT},
   {"lindent", ATTR_LINDENT},
   {"line",    ATTR_LINE},
   {"par",     ATTR_PAR},
   {"pard",    ATTR_PARD},
   {"pari",    ATTR_PARI},
   {"plain",   ATTR_PLAIN},
   {"settabs", ATTR_SETTABS},
   {"tab",     ATTR_TAB},
   {"u",       ATTR_U},
   {"ub",      ATTR_UB},
   {"ui",      ATTR_UI},
   {"uu",      ATTR_UU},
   {"",        ATTR_MAX}
};

#undef INSTDATA
#define INSTDATA struct NodeData *data = INST_DATA(cl, obj)

/* ------------------------------ prototypes ------------------------------ */

static ULONG om_new(Class *cl, Object *obj, struct opSet *msg);
static ULONG om_dispose(Class *cl, Object *obj, Msg msg);
static ULONG om_set(Class *cl, Object *obj, struct opSet *msg);

static ULONG dtm_asynclayout(Class *cl, Object *obj, struct gpLayout *msg);

/* -------------------------- private functions --------------------------- */

static
APTR AllocNodeMem(Class *cl, Object *obj, ULONG size)
{
   CLASSBASE;
   INSTDATA;
   APTR mem;

   mem = AllocPooled(data->n_Pool,size);
   return mem;
}
static
void FreeNodeMem(Class *cl, Object *obj, APTR mem, ULONG size)
{
   CLASSBASE;
   INSTDATA;
   FreePooled(data->n_Pool, mem, size);
}

static
APTR AllocNodeVec(Class *cl, Object *obj, ULONG size)
{
   ULONG *mem;
   size += sizeof(ULONG);
   if((mem = AllocNodeMem(cl, obj, size)) != NULL)
      *mem++= size;

   return mem;
}
static
void FreeNodeVec(Class *cl, Object *obj, APTR mem)
{
   ULONG *m = ((ULONG *) mem) - 1;
   FreeNodeMem(cl, obj, m, *m);
}

/* -------------------------- parsing functions --------------------------- */

static
LONG GetAmigaGuideAttr(Class *cl, Object *obj, STRPTR attr, LONG attr_len)
{
   CLASSBASE;
   const struct AmigaGuideAttr *attrs = amigaguide_attrs;

   while(attrs->aga_Id != ATTR_MAX)
   {
      LONG len = strlen(attrs->aga_Name);
      if(len == attr_len && !Strnicmp(attrs->aga_Name, attr, len))
         return attrs->aga_Id;
      ++attrs;
   }
   return ATTR_MAX;
}

static
LONG GetPenColor(Class *cl, Object *obj, struct ParseNodeData *pd,
                 STRPTR arg, LONG arg_len, LONG *val)
{
   CLASSBASE;
   const struct DrawInfoPen
   {
      STRPTR Name;
      LONG Index;
   } pens[] =
   {
      {"Text",     TEXTPEN},
      {"Shine",    SHINEPEN},
      {"Shadow",   SHADOWPEN},
      {"Fill",     FILLPEN},
      {"FillText", FILLTEXTPEN},
      {"Background", BACKGROUNDPEN},
      {"Back",       BACKGROUNDPEN},
      {"Highlight", HIGHLIGHTTEXTPEN},
      {"", -1}
   };
   LONG i;

   for(i=0; pens[i].Index != -1; ++i)
   {
      LONG len = strlen(pens[i].Name);
      if(len == arg_len && !Strnicmp(arg, pens[i].Name, len))
      {
         *val = pd->p_DrawInfo->dri_Pens[pens[i].Index];
         return 1;
      }
   }
   return 0;
}

static
STRPTR ParseNodeString(Class *cl, Object *obj, STRPTR args, STRPTR end, LONG *len)
{
   STRPTR ptr = args;
   STRPTR help = ptr;

   ptr = eatws(ptr);

   if(ptr < end && *ptr == '"')
      help = ++ptr;

   while(ptr < end && *ptr != '\n' && *ptr != '\0' && *ptr != '"')
   {
      if(*ptr == '\\')
	 ++ptr;
      ++ptr;
   }

   if(ptr == end)
      return NULL;
   *len = ptr-help;
   return help;
}

static
STRPTR CopyNodeString(Class *cl, Object *obj, STRPTR args, LONG len, STRPTR old)
{
   STRPTR str;
   STRPTR ptr = args;
   STRPTR help = ptr;
   STRPTR end = ptr + ((len == -1) ? strlen(ptr) : len);

   while(ptr < end && *ptr != '\n' && *ptr != '\0')
      ptr++;

   if((str = AllocNodeVec(cl, obj, ptr-help+1)) != NULL)
   {
      strncpy(str, help, ptr-help);
      if(old != NULL)
	 FreeNodeVec(cl, obj, old);
   } else
   {
      str = old;
   }

   return str;
}

static
STRPTR CopyLineString(Class *cl, Object *obj,
                      STRPTR str, LONG len)
{
   CLASSBASE;
   INSTDATA;
   STRPTR cpy;
   STRPTR end = str + ((len == -1) ? strlen(str) : len);

   if((cpy = AllocPooled(data->n_LinePool, end-str+1)) != NULL)
   {
      strncpy(cpy, str, len);
   }
   return cpy;
}

static
struct Line *AllocLine(Class *cl, Object *obj,
                       struct ParseNodeData *pd,
                       STRPTR text, LONG len)
{
   CLASSBASE;
   INSTDATA;
   struct Line *line;

   /* if last character is a new line don't output it */
   if(text[len-1]=='\n')
      --len;

   if((line = AllocPooled(data->n_LinePool, sizeof(struct Line))) != NULL)
   {
      pd->p_LastLine = line;
      if(pd->p_CurWidth == 0)
         pd->p_CurWidth = pd->p_LineIndent * pd->p_SpcWidth;
      line->ln_FgPen = pd->p_FgPen;
      line->ln_BgPen = pd->p_BgPen;
      line->ln_Style = pd->p_Style;
      line->ln_Text  = text;
      line->ln_TextLen = len;
      line->ln_Width = TextLength(&pd->p_RPort, text, len);
      line->ln_Height = pd->p_Font->tf_YSize;
      line->ln_XOffset = pd->p_CurWidth;
      line->ln_YOffset = 0;
      line->ln_Flags = pd->p_Justification;

      pd->p_CurWidth += line->ln_Width;
      if(pd->p_MaxWidth < pd->p_CurWidth)
         pd->p_MaxWidth = pd->p_CurWidth;

#if 0
      {
      UBYTE chr;
      chr = text[len];
      text[len] = '\0';
      DB(("init line %s (len=%ld)\n", text, len));
      text[len] = chr;
      }
#endif
      AddTail(pd->p_List, (struct Node *) line);
   }
   return line;
}

static
void LineFeed(struct ParseNodeData *pd, struct Line *line)
{
   line->ln_Flags |= LNF_LF;
   /* reset current width */
   pd->p_CurWidth = 0;
   /* reset custom tab counter */
   pd->p_CustomTab = 0;
   /* increment newline counter */
   ++pd->p_Newline;
   /* increment overall line counter */
   ++pd->p_Lines;
}

static
struct Line *AllocTextLine(Class *cl, Object *obj,
                           struct ParseNodeData *pd,
                           STRPTR text, LONG len)
{
   CLASSBASE;
   INSTDATA;
   struct Line *line = NULL;

   if(len < 0)
      return NULL;

   if(pd->p_Flags.WordWrap)
   {
      STRPTR delim = data->n_WordDelim;
      LONG width;
      LONG tmp, last;
      BOOL failed;
      BOOL wrapped;

      do
      {
         failed = FALSE;
         last = tmp = len;
         width = TextLength(&pd->p_RPort, text, len);
         wrapped = FALSE;
         while(pd->p_CurWidth + width > pd->p_Width)
         {
            wrapped = TRUE;
            --tmp;
            while(tmp >= 0 && strchr(delim, text[tmp]) == NULL)
               --tmp;
            if(tmp <= 0)
            {
               failed = TRUE;
               break;
            }
            width = TextLength(&pd->p_RPort, text, tmp+1);
            last = tmp+1;
         }

         if(failed && pd->p_LastLine != NULL && !(pd->p_LastLine->ln_Flags & LNF_LF))
         {
            LineFeed(pd, pd->p_LastLine);
            DB(("wordwrapped line!\n"));
            continue;
         }

         tmp = (failed) ? last : (wrapped) ? tmp+1 : tmp;
         line = AllocLine(cl, obj, pd, text, tmp);
         if(line != NULL && (wrapped || failed))
         {
            LineFeed(pd, line);
            DB(("wordwrapped line!\n"));
         }
         text += tmp;
         len -= tmp;
      } while(len > 0);
   } else
   {
      line = AllocLine(cl, obj, pd, text, len);
   }
   return line;
}

static
struct Line *AllocLFLine(Class *cl, Object *obj,
                         struct ParseNodeData *pd,
                         STRPTR text, STRPTR ptr)
{
   struct Line *line;

   if(text < ptr)
   {
      line = AllocTextLine(cl, obj, pd, text, ptr-text);
   } else
   {
      line = AllocLine(cl, obj, pd, "", 0);
   }
   if(line != NULL)
   {
      LineFeed(pd, line);
   }
   return line;
}

static
struct Line *AllocTabLine(Class *cl, Object *obj,
                          struct ParseNodeData *pd,
                          STRPTR text, STRPTR ptr)
{
   struct Line *line;

   if(text < ptr)
   {
      AllocTextLine(cl, obj, pd, text, ptr-text);
   }

   line = AllocLine(cl, obj, pd, "", 0);
   if(line != NULL)
   {
      BOOL done = FALSE;

      if(pd->p_CTabs != NULL && pd->p_CustomTab < pd->p_CustomTabMax)
      {
         LONG curwidth = pd->p_CurWidth / pd->p_SpcWidth;
         while(pd->p_CustomTab < pd->p_CustomTabMax && pd->p_CTabs[pd->p_CustomTab] < curwidth)
            ++pd->p_CustomTab;
         if(pd->p_CustomTab == pd->p_CustomTabMax)
            done = FALSE;
         else
         {
            done = TRUE;
            curwidth = pd->p_CTabs[pd->p_CustomTab] * pd->p_SpcWidth;
            line->ln_Width = curwidth-pd->p_CurWidth;
            pd->p_CurWidth += line->ln_Width;
         }
      }

      if(!done)
      {
         LONG curwidth = pd->p_CurWidth / pd->p_TabWidth;
         ++curwidth;
         curwidth *= pd->p_TabWidth;

         line->ln_Width = curwidth-pd->p_CurWidth;
         pd->p_CurWidth += line->ln_Width;
      }
   }
   return line;
}

static
BOOL DoNodeAttr(Class *cl, Object *obj, struct ParseNodeData *pd,
                LONG id, STRPTR attrargs, LONG attrargs_len,
                STRPTR *ptr_ptr, BOOL *newline)
{
   CLASSBASE;
   INSTDATA;
   struct Line *line;
   STRPTR ptr = *ptr_ptr;

   LONG val;
   switch(id)
   {
   /* font style attributes */
   case ATTR_B:
      pd->p_Style |= FSF_BOLD;
      break;
   case ATTR_UB:
      pd->p_Style &= ~FSF_BOLD;
      break;
   case ATTR_I:
      pd->p_Style |= FSF_ITALIC;
      break;
   case ATTR_UI:
      pd->p_Style &= ~FSF_ITALIC;
      break;
   case ATTR_U:
      pd->p_Style |= FSF_UNDERLINED;
      break;
   case ATTR_UU:
      pd->p_Style &= ~FSF_UNDERLINED;
      break;
   case ATTR_PLAIN:
      pd->p_Style = FS_NORMAL;
      break;
   /* foreground and background color attributes */
   case ATTR_APEN:
      if(StrToLong(attrargs, &val) > 0)
         pd->p_FgPen = val;
      break;
   case ATTR_BPEN:
      if(StrToLong(attrargs, &val) > 0)
         pd->p_BgPen = val;
      break;
   case ATTR_FG:
      if(GetPenColor(cl, obj, pd, attrargs, attrargs_len, &val))
         pd->p_FgPen = val;
      break;
   case ATTR_BG:
      if(GetPenColor(cl, obj, pd, attrargs, attrargs_len, &val))
         pd->p_BgPen = val;
      break;
   /* tabulator attributes */
   case ATTR_TAB:
      AllocTabLine(cl, obj, pd, ptr, ptr);
      break;
   case ATTR_SETTABS:
      {
         LONG i=0;
         LONG chrs;
         LONG tab;
         while(i < MAX_TABS && *attrargs != '}')
         {
            attrargs = eatws(attrargs);
            if((chrs = StrToLong(attrargs, &tab)) > 0)
            {
               pd->p_CustomTabArray[i] = tab;
               ++i;
               attrargs += chrs;
            } else
            {
               ++attrargs;
            }
         }
         if(i > 0)
         {
            pd->p_CTabs = pd->p_CustomTabArray;
            pd->p_CustomTab = 0;
            pd->p_CustomTabMax = i;
         }
      }
      break;
   case ATTR_CLEARTABS:
      /* clear custom tabs */
      pd->p_CTabs = NULL;
      break;
   /* paragraph attributes */
   case ATTR_LINE:
      AllocLFLine(cl, obj, pd, ptr, ptr);
      *newline = TRUE;
      break;
   case ATTR_PAR:
      pd->p_LineIndent = 0;
      AllocLFLine(cl, obj, pd, ptr, ptr);
      AllocLFLine(cl, obj, pd, ptr, ptr);
      /* skip possible new line directly after par attribute */
      if(ptr < pd->p_End && *ptr == '\n')
         ++ptr;
      else
         *newline = TRUE;
      pd->p_LineIndent = pd->p_BodyLineIndent + pd->p_FirstLineIndent;
      break;
   case ATTR_PARD:
      pd->p_FgPen = pd->p_DrawInfo->dri_Pens[TEXTPEN];
      pd->p_BgPen = pd->p_DrawInfo->dri_Pens[BACKGROUNDPEN];
      pd->p_Style = FS_NORMAL;
      pd->p_BodyLineIndent = 0;
      pd->p_FirstLineIndent = 0;
      break;
   case ATTR_LINDENT:
      if(StrToLong(attrargs, &val) > 0)
         pd->p_BodyLineIndent = val;
      pd->p_LineIndent = pd->p_BodyLineIndent + pd->p_FirstLineIndent;
      DB(("lindent %ld, indent %ld\n", pd->p_BodyLineIndent, pd->p_LineIndent));
      break;
   case ATTR_PARI:
      if(StrToLong(attrargs, &val) > 0)
         pd->p_FirstLineIndent = val;
      pd->p_LineIndent = pd->p_BodyLineIndent + pd->p_FirstLineIndent;
      break;
   /* misc attributes */
   case ATTR_AMIGAGUIDE:
      if((line = AllocLine(cl, obj, pd, "AmigaGuide®", 11)) != NULL)
      {
         line->ln_Style |= FSF_BOLD;
      }
      break;
   /* justification attributes */
   case ATTR_JCENTER:
      pd->p_Justification &= ~LNF_JMASK;
      pd->p_Justification |= LNF_JCENTER;
      break;
   case ATTR_JLEFT:
      pd->p_Justification &= ~LNF_JMASK;
      pd->p_Justification |= LNF_JLEFT;
      break;
   case ATTR_JRIGHT:
      pd->p_Justification &= ~LNF_JMASK;
      pd->p_Justification |= LNF_JRIGHT;
      break;
   case ATTR_CODE:
      pd->p_Flags.WordWrap = FALSE;
      break;
   case ATTR_BODY:
      pd->p_Flags.WordWrap = data->n_Flags.WordWrap;
      if(data->n_Flags.SmartWrap && *ptr == '\n' && ptr < pd->p_End)
         ++ptr;
      break;
   default:
      return FALSE;
   }

   *ptr_ptr = ptr;
   return TRUE;
}


static
void FindAndExpandMacro(Class *cl, Object *obj, struct ParseNodeData *pd,
                        STRPTR attr, LONG attr_len, STRPTR args, LONG args_len,
                        STRPTR *ptr_ptr, BOOL *newline)
{
   CLASSBASE;
   INSTDATA;
   struct AmigaGuideMacro *agm = NULL;
   struct Node *n = data->n_AGFile->agf_Macros.lh_Head;

   while(n->ln_Succ != NULL)
   {
      LONG len = strlen(n->ln_Name);
      if(len == attr_len && !Strnicmp(attr, n->ln_Name, len))
      {
         agm = (struct AmigaGuideMacro *) n;
         break;
      }
      n = n->ln_Succ;
   }

   if(agm == NULL)
   {
      UBYTE chr = attr[attr_len];
      attr[attr_len] = '\0';
      DB(("macro \"%s\" not found\n", attr));
      attr[attr_len] = chr;
   } else
   {
      STRPTR mac = agm->agm_Macro;
      STRPTR argsend = args + args_len;
      STRPTR mac_args[MAX_MACRO_ARGS];
      STRPTR text;
      LONG mac_args_len[MAX_MACRO_ARGS];
      LONG mac_args_num = 0;

      /* setup macro arguments */
      while(args < argsend)
      {
         BOOL quote = FALSE;
         args = eatws(args);
         if(*args == '"')
         {
            ++args;
            quote = TRUE;
         }
         mac_args[mac_args_num] = args;

         if(quote)
         {
            while(args < argsend && *args != '"')
               ++args;
            ++args;
         } else
         {
            while(args < argsend && *args != ' ' && *args != '\t')
               ++args;
            ++args;
         }
         mac_args_len[mac_args_num] = args - mac_args[mac_args_num] - 1;
         ++mac_args_num;
      }

      /* now parse macro */
      text = mac;
      while(*mac != '\0')
      {
         if(*mac == '@' && *(mac+1) == '{')
         {
            /* attribute found */
            STRPTR attrargs;
            STRPTR ptr = mac + 2;
            STRPTR attrname;
            STRPTR attrend;
            LONG id;
            LONG attrargs_len;

            if(text < mac)
               AllocTextLine(cl, obj, pd, text, mac-text);

            attrname = ptr;
            while(*ptr != '\0' && *ptr != ' ' && *ptr != '\t' && *ptr != '}')
               ++ptr;
            attrend = ptr;

            ptr = eatws(ptr);

            attrargs = ptr;
            while(*ptr != '\0' && *ptr != '}')
               ++ptr;
            attrargs_len = ptr-attrargs;
            ++ptr;

            id = GetAmigaGuideAttr(cl, obj, attrname, attrend-attrname);
            if(!DoNodeAttr(cl, obj, pd, id, attrargs, attrargs_len,
                           ptr_ptr, newline))
            {
               FindAndExpandMacro(cl, obj, pd,
                                  attrname, attrend-attrname,
                                  attrargs, attrargs_len,
                                  ptr_ptr, newline);
            }
            mac = ptr;
            text = mac;
         } else if(*mac == '$')
         {
            LONG argnum = 0;
            LONG chrs;
            if((chrs = StrToLong(mac+1, &argnum)) > 0)
            {
               if(text < mac)
                  AllocTextLine(cl, obj, pd, text, mac-text);
               if(argnum > 0 && argnum <= MAX_MACRO_ARGS)
               {
                  --argnum;
                  AllocTextLine(cl, obj, pd, mac_args[argnum], mac_args_len[argnum]);
               }
               mac += chrs;
            }
            text = ++mac;
         } else
         {
            ++mac;
         }
      }
   }
}

static
LONG ParseNode(Class *cl, Object *obj, struct ParseNodeData *pd)
{
   CLASSBASE;
   INSTDATA;
   struct Line *line;

   if(pd != NULL &&
      GetDTAttrs(obj,
 	         TDTA_LineList, (ULONG) &pd->p_List,
                 DTA_TextFont,  (ULONG) &pd->p_Font,
 	         DTA_Domain,    (ULONG) &pd->p_Domain,
 	         TAG_DONE) == 3 &&
                 pd->p_List != NULL && pd->p_Domain != NULL && pd->p_Font != NULL)
   {
      STRPTR ptr = data->n_Buffer;
      STRPTR end = ptr + data->n_BufferLen;
      STRPTR text = ptr;

      InitRastPort(&pd->p_RPort);
      SetFont(&pd->p_RPort, pd->p_Font);

      pd->p_SpcWidth = TextLength(&pd->p_RPort, " ", 1);
      pd->p_TabWidth = pd->p_SpcWidth * data->n_TabWidth;
      pd->p_Width = pd->p_Domain->Width;
      pd->p_End = end;

      pd->p_FgPen = pd->p_DrawInfo->dri_Pens[TEXTPEN];
      pd->p_BgPen = pd->p_DrawInfo->dri_Pens[BACKGROUNDPEN];
      pd->p_Newline = 1;
      pd->p_Flags.WordWrap = data->n_Flags.WordWrap;

      while(ptr < end)
      {
         if(*ptr == '@')
         {
            BOOL newline = FALSE;
            if(*(ptr+1) != '{')
            {
               /* skip AmigaGuide commands. */
               if(pd->p_Newline > 0)
               {
                  while(ptr < end && *ptr != '\n')
                     ++ptr;
                  ++ptr;
                  text = ptr;
                  newline = TRUE;
               } else
               {
                  ++ptr;
               }
            } else
            {
               STRPTR attrname;
               if(text < ptr)
               {
                  STRPTR end = ptr;
                  if((line = AllocTextLine(cl, obj, pd, text, end-text)) != NULL)
                  {
                  }
               }
               ptr += 2;
               attrname = ptr;
               if(*ptr == '"')
               {
                  STRPTR linktext;
                  STRPTR linktextend;
                  STRPTR link;
                  STRPTR linkend;
                  ptr += 1;
                  linktext = ptr;
                  while(ptr < end && *ptr != '"')
                    ++ptr;
                  linktextend = ptr++;

                  ptr = eatws(ptr);
                  link = ptr;
                  while(ptr < end && *ptr != ' ' && *ptr != '\t')
                    ++ptr;

                  while(ptr < end && *ptr != '}')
                    ++ptr;
                  linkend = ptr;
                  ++ptr;
                  if((line = AllocLine(cl, obj, pd, linktext, linktextend-linktext)) != NULL)
                  {
                     line->ln_Flags |= LNF_LINK;
                     line->ln_FgPen = pd->p_DrawInfo->dri_Pens[TEXTPEN];
                     line->ln_BgPen = pd->p_DrawInfo->dri_Pens[BACKGROUNDPEN];
                     line->ln_Data  = CopyLineString(cl, obj, link, linkend-link);

                     /* adjust the width and height to make more room for the link text. */
                     line->ln_Width += 2;
                     line->ln_Height+= 1;
                     pd->p_CurWidth += 2;
                     ++pd->p_Links;
                  }
               } else
               {
                  STRPTR attrend;
                  STRPTR attrargs;
                  LONG id;
                  LONG attrargs_len;
                  while(ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '}')
                     ++ptr;
                  attrend = ptr;

                  ptr = eatws(ptr);

                  attrargs = ptr;
                  while(ptr < end && *ptr != '}')
                     ++ptr;
                  attrargs_len = ptr-attrargs;
                  ++ptr;


                  id = GetAmigaGuideAttr(cl, obj, attrname, attrend-attrname);
                  if(!DoNodeAttr(cl, obj, pd, id, attrargs, attrargs_len,
                                 &ptr, &newline))
                  {
                     FindAndExpandMacro(cl, obj, pd,
                                        attrname, attrend-attrname,
                                        attrargs, attrargs_len,
                                        &ptr, &newline);
                  }
               }
               text = ptr;
            }
            pd->p_Newline = (newline) ? 1 : 0;
         } else if(*ptr == '\n')
         {
            STRPTR tmp=ptr;
            while(tmp < end && *tmp == '\n')
               ++tmp;
            switch(tmp-ptr)
            {
            case 1:
               AllocLFLine(cl, obj, pd, text, ptr);
               pd->p_LineIndent = pd->p_BodyLineIndent;
               ++ptr;
               break;
            default:
               while(ptr < tmp)
               {
                  AllocLFLine(cl, obj, pd, text, ptr);
                  pd->p_LineIndent = 0;
                  text = ++ptr;
               }
               pd->p_LineIndent = pd->p_BodyLineIndent + pd->p_FirstLineIndent;
               break;
            }
            text=ptr;
         } else if(*ptr == '\t')
         {
            AllocTabLine(cl, obj, pd, text, ptr);
            ++ptr;
            text = ptr;
            pd->p_Newline = 0;
         } else if(*ptr == '\\')
         {
            if(text < ptr)
               AllocTextLine(cl, obj, pd, text, ptr-text);
            ++ptr;
            text = ptr;
            ++ptr;
            pd->p_Newline = 0;
         } else
         {
            ++ptr;
            pd->p_Newline = 0;
         }
      }
      if(text < ptr)
      {
         AllocLFLine(cl, obj, pd, text, ptr);
      }
      AllocLFLine(cl, obj, pd, text, text);
   }

   return pd->p_Lines;
}

static
void PrepareSmartWrap(Class *cl, Object *obj, struct ParseNodeData *pd)
{
   INSTDATA;
   STRPTR ptr = data->n_Buffer;
   STRPTR end = ptr + data->n_BufferLen;
   BOOL wordwrap = TRUE;

   while(ptr < end)
   {
      if(*ptr == '@')
      {
         if(*(ptr+1) != '{')
         {
            while(ptr < end && *ptr != '\n')
               ++ptr;
            ++ptr;
         } else
         {
            STRPTR attrname;
            ptr += 2;
            attrname = ptr;

            if(*ptr == '"')
            {
               ptr += 1;
               while(ptr < end && *ptr != '"')
                 ++ptr;
               ptr = eatws(ptr);
               while(ptr < end && *ptr != ' ' && *ptr != '\t')
                 ++ptr;

               while(ptr < end && *ptr != '}')
                 ++ptr;
               ++ptr;
            } else
            {
               LONG id;
               STRPTR attrend;
               while(ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '}')
                  ++ptr;
               attrend = ptr;
               ptr = eatws(ptr);
               while(ptr < end && *ptr != '}')
                  ++ptr;
               ++ptr;

               switch((id = GetAmigaGuideAttr(cl, obj, attrname, attrend-attrname)))
               {
               case ATTR_CODE:
                  wordwrap = FALSE;
                  break;
               case ATTR_BODY:
                  wordwrap = TRUE;
                  if(ptr < end && *ptr == '\n')
                     ++ptr;
                  break;
               default:
                  break;
               }
            }
         }
      } else if(*ptr == '\n')
      {
         /* convert the first newline to a space if wordwrap is ON. */
         if(wordwrap)
            *ptr++ = ' ';
         while(ptr < end && *ptr == '\n')
            ++ptr;
      } else
      {
         ++ptr;
      }
   }
}

/* ------------------------- scan node functions -------------------------- */

static
void ScanNode(Class *cl, Object *obj)
{
   CLASSBASE;
   INSTDATA;
   STRPTR begin = data->n_Buffer;
   STRPTR end   = data->n_Buffer+data->n_BufferLen;
   STRPTR ptr   = begin;

   while(ptr < end)
   {
      if(*ptr == '@')
      {
	 ++ptr;
	 if(ptr < end && *ptr != '{')
	 {
	    STRPTR args;
	    STRPTR str;
	    LONG len;
	    LONG cmd;
            LONG val;

	    cmd = GetNodeCommand(cb, ptr, &args);
	    DB(("command %ld found\n", cmd));
	    switch(cmd)
	    {
	    case CMD_TITLE:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_Title = CopyNodeString(cl, obj, str, len, data->n_Title);
	       break;
	    case CMD_PREV:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_Prev = CopyNodeString(cl, obj, str, len, data->n_Prev);
	       break;
	    case CMD_NEXT:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_Next = CopyNodeString(cl, obj, str, len, data->n_Next);
	       break;
            case CMD_TOC:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_TOC = CopyNodeString(cl, obj, str, len, data->n_TOC);
               break;
            case CMD_INDEX:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_Index = CopyNodeString(cl, obj, str, len, data->n_Index);
               break;
            case CMD_HELP:
	       if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		  data->n_Help = CopyNodeString(cl, obj, str, len, data->n_Help);
               break;
	    case CMD_NODE:
	       args = eatws(args);
	       while(args < end && *args != ' ' && *args != '\t' && *args != '\n')
		  args++;
	       if(*args != '\n')
		  if((str = ParseNodeString(cl, obj, args, end, &len)) != NULL)
		     data->n_Title = CopyNodeString(cl, obj, str, len, data->n_Title);
	       DB(("title is \"%s\"\n", data->n_Title));
	       break;
            case CMD_SMARTWRAP:
               data->n_Flags.SmartWrap = TRUE;
               data->n_Flags.WordWrap = TRUE;
               break;
            case CMD_WORDWRAP:
               data->n_Flags.WordWrap = TRUE;
               break;
            case CMD_TAB:
               if(StrToLong(args, &val) > 0)
                  data->n_TabWidth = val;
               break;
            /* the following commands are processed during ScanFile() within
               amigaguideclass */
            case CMD_FONT:
            case CMD_ONOPEN:
            case CMD_ONCLOSE:
               break;
	    }
	 } else
	 {
	    /* skip chars until end of line */
	    while(ptr < end && *ptr != '\n')
	       ++ptr;
	    ++ptr;
	 }
      } else
      {
	 while(ptr < end && *ptr != '\n')
	    ++ptr;
	 ++ptr;
      }
   }
}

/* ------------------------------ functions ------------------------------- */

static
ULONG om_new(Class *cl, Object *obj, struct opSet *msg)
{
   ULONG rv = (ULONG) obj;
   CLASSBASE;
   INSTDATA;

   ULONG sourcetype = DTST_FILE;

   struct TagItem *tstate = msg->ops_AttrList;
   struct TagItem *tag;

   data->n_TabWidth = 8;
   data->n_WordDelim = " ,.;";

   /* initialize the instance data */
   data->n_Pool = CreatePool(MEMF_CLEAR | MEMF_ANY, AG_PUDDLE_SIZE, AG_PUDDLE_SIZE);

   /* process attributes that only belongs to this class */
   while((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
   {
      switch(tag->ti_Tag)
      {
      case AGNA_RootObject:
         data->n_RootObject = (Object *) tag->ti_Data;
         break;
      case AGNA_AGFile:
         /* set defaults from the AmigaGuide file */
         data->n_AGFile = (struct AmigaGuideFile *) tag->ti_Data;
         data->n_TabWidth = data->n_AGFile->agf_TabWidth;
         data->n_WordDelim = data->n_AGFile->agf_WordDelim;
         data->n_Flags.SmartWrap = (data->n_AGFile->agf_Flags.SmartWrap) ? TRUE : FALSE;
         data->n_Flags.WordWrap  = (data->n_AGFile->agf_Flags.WordWrap) ? TRUE : FALSE;
         /* smartwrap flag indicates also word wrapping mode in general */
         if(data->n_Flags.SmartWrap)
            data->n_Flags.WordWrap = TRUE;
         break;
      case DTA_SourceType:
	 sourcetype = tag->ti_Data;
	 break;
      case DTA_SourceAddress:
	 data->n_Buffer = (STRPTR) tag->ti_Data;
	 break;
      case DTA_SourceSize:
	 data->n_BufferLen = tag->ti_Data;
	 break;
      }
   }

   /* set attributes provided by the user */
   om_set(cl, obj, msg);

   /* now get the source type only DTST_FILE is supported.
      Then start scanning the AmigaGuide file */
   DB(("sourcetype : %ld\n",sourcetype));
   switch(sourcetype)
   {
   case DTST_FILE:
      /* obtain buffer and buffer length from textclass */
      GetDTAttrs(obj,
                 TDTA_Buffer,    (ULONG) &data->n_Buffer,
                 TDTA_BufferLen, (ULONG) &data->n_BufferLen,
                 TAG_DONE);
      break;
   case DTST_MEMORY:
      /* source type memory attribute are set during tagitem loop above. */
      break;
   default:
      rv = (ULONG) NULL;
   }

   /* in any case check if there is a valid buffer and buffer length */
   if(data->n_Buffer == NULL || data->n_BufferLen == 0)
   {
      DB(("buffer and/or buffer length are missing (buf=%lx, len=%ld)\n",
          data->n_Buffer, data->n_BufferLen));
      /* not all needed tags were passed so OM_NEW will fail */
      rv = (ULONG) NULL;
   }

   if(data->n_AGFile == NULL)
   {
      DB(("No AmigaGuideFile structure passed\n"));
      rv = (ULONG) NULL;
   }

   if(rv != (ULONG) NULL)
   {
      ScanNode(cl, obj);

      /* let the textclass know about our word delimiters */
      SetAttrs(obj, TDTA_WordDelim, (ULONG) data->n_WordDelim, TAG_DONE);
   }

   return rv;
}

static
ULONG om_dispose(Class *cl,Object *obj,Msg msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv;

   /* cleanup our data here */
   if(data->n_LinePool != NULL)
      DeletePool(data->n_LinePool);

   if(data->n_Pool != NULL)
      DeletePool(data->n_Pool);

   rv = DoSuperMethodA(cl,obj,msg);

   return rv;
}

static
ULONG om_get(Class *cl,Object *obj,struct opGet *msg)
{
   ULONG rv;
   INSTDATA;

   rv = 1;
   switch(msg->opg_AttrID)
   {
   case DTA_Title:
      *msg->opg_Storage = (ULONG) data->n_Title;
      break;
   case AGNA_Contents:
      *msg->opg_Storage = (ULONG) data->n_TOC;
      break;
   case AGNA_Help:
      *msg->opg_Storage = (ULONG) data->n_Help;
      break;
   case AGNA_Index:
      *msg->opg_Storage = (ULONG) data->n_Index;
      break;
   case AGNA_Previous:
      *msg->opg_Storage = (ULONG) data->n_Prev;
      break;
   case AGNA_Next:
      *msg->opg_Storage = (ULONG) data->n_Next;
      break;
   case TDTA_WordWrap:
      *msg->opg_Storage = data->n_Flags.WordWrap;
      break;
   default:
      rv = DoSuperMethodA(cl,obj,(Msg) msg);
   }

   return rv;
}


static
ULONG om_set(Class *cl,Object *obj,struct opSet *msg)
{
   CLASSBASE;
   INSTDATA;
   ULONG rv = 0;

   struct TagItem *tstate = msg->ops_AttrList;
   struct TagItem *tag;

   /* process attributes that only belongs to this class */
   while((tag = NextTagItem((const struct TagItem **)&tstate)) != NULL)
   {
      switch(tag->ti_Tag)
      {
      case DTA_Sync:
	 /* now redraw the object */
	 rv = 1;
	 break;
      case AGNA_Help:
         data->n_Help = CopyNodeString(cl, obj, (STRPTR) tag->ti_Data, -1, data->n_Help);
         break;
      case DTA_Title:
	 data->n_Title = CopyNodeString(cl, obj, (STRPTR) tag->ti_Data, -1, data->n_Title);
	 break;
      case TDTA_WordDelim:
         data->n_WordDelim = (STRPTR) tag->ti_Data;
         break;
      }
   }

   return rv;
}


static
ULONG dtm_asynclayout(Class *cl, Object *obj, struct gpLayout *msg)
{
   CLASSBASE;
   INSTDATA;
   struct DTSpecialInfo *si= CAST_GAD(obj)->SpecialInfo;
   ULONG lines = 0;
   struct IBox *domain;
   struct List *list;

   ObtainSemaphore(&si->si_Lock);

   if(GetDTAttrs(obj,
                 DTA_Domain,    (ULONG) &domain,
                 TDTA_LineList, (ULONG) &list,
                 TAG_DONE) == 2)
   {
      BOOL clear = FALSE;
      if(msg->gpl_Initial || data->n_Flags.WordWrap)
      {
         struct ParseNodeData *pd;

         if(data->n_LinePool != NULL)
         {
            DeletePool(data->n_LinePool);
            NewList(list);
         }
         data->n_LinePool = CreatePool(MEMF_CLEAR | MEMF_ANY, AG_PUDDLE_SIZE, AG_PUDDLE_SIZE);

         pd = AllocNodeMem(cl, obj, sizeof(struct ParseNodeData));
         if(pd != NULL)
         {
            /* setup draw info field for parsing node */
            pd->p_DrawInfo = msg->gpl_GInfo->gi_DrInfo;

            if(msg->gpl_Initial && data->n_Flags.SmartWrap)
            {
               PrepareSmartWrap(cl, obj, pd);
            }

            lines = ParseNode(cl, obj, pd);

            DB(("domain: (%ld,%ld)-(%ld,%ld)\n",
                domain->Left, domain->Top,
                domain->Width, domain->Height));

   	    si->si_VertUnit = pd->p_Font->tf_YSize + ((pd->p_Links > 0) ? 1 : 0);
   	    si->si_TopVert = 0;
   	    si->si_VisVert = domain->Height / si->si_VertUnit;
   	    si->si_TotVert = lines;

   	    si->si_HorizUnit = pd->p_Font->tf_XSize;
   	    si->si_TopHoriz = 0;
   	    si->si_VisHoriz = domain->Width / si->si_HorizUnit;
   	    si->si_TotHoriz = pd->p_MaxWidth / si->si_HorizUnit;

            SetAttrs(obj, DTA_VertUnit, si->si_VertUnit, TAG_DONE);

            FreeNodeMem(cl, obj, pd, sizeof(struct ParseNodeData));

            NotifyAttrs(obj,         msg->gpl_GInfo, 0,
         	        GA_ID,       CAST_GAD(obj)->GadgetID,
         	        DTA_VertUnit,   si->si_VertUnit,
         	        DTA_HorizUnit,  si->si_HorizUnit,
                        TAG_DONE);
         }
         /* if word wrap is turned on clear object domain because the
            layout could have changed! */
         clear = data->n_Flags.WordWrap;
      } else
      {
	 si->si_VisVert = domain->Height / si->si_VertUnit;
	 si->si_VisHoriz = domain->Width / si->si_HorizUnit;
      }

      /* layout justification */
      {
         struct Line *line;
         struct Line *start,*end;
         BOOL adjusted = FALSE;
         line = (struct Line *) list->lh_Head;
         while(line->ln_Link.mln_Succ != NULL)
         {
            if(line->ln_Flags & LNF_JMASK)
            {
               LONG width = 0;
               start = end = NULL;
               switch(line->ln_Flags & LNF_JMASK)
               {
               case LNF_JCENTER:
                  start = line;
                  while(line->ln_Link.mln_Succ != NULL &&
                        (line->ln_Flags & LNF_JMASK) == LNF_JCENTER)
                  {
                     width += line->ln_Width;
                     if(line->ln_Flags & LNF_LF)
                        break;
                     line = (struct Line *) line->ln_Link.mln_Succ;
                  }
                  end = line;
                  break;
               case LNF_JRIGHT:
                  start = line;
                  while(line->ln_Link.mln_Succ != NULL &&
                        (line->ln_Flags & LNF_JMASK) == LNF_JRIGHT)
                  {
                     width += line->ln_Width;
                     if(line->ln_Flags & LNF_LF)
                        break;
                     line = (struct Line *) line->ln_Link.mln_Succ;
                  }
                  end = line;
                  break;
               }
               if(start != NULL && end != NULL)
               {
                  LONG x = domain->Width - width;
                  if(x > 0)
                  {
                     if((start->ln_Flags & LNF_JMASK) == LNF_JCENTER)
                        x = x >> 1;
                     while(TRUE)
                     {
                        start->ln_XOffset = x;
                        x += start->ln_Width;
                        if(start == end)
                           break;
                        start = (struct Line *) start->ln_Link.mln_Succ;
                     }
                     /* if the text was justified lets clear the object domain */
                     clear = adjusted = TRUE;
                  }
                  if(end == line)
                     line = (struct Line *) end->ln_Link.mln_Succ;
                  else
                     line = end;
               } else
               {
                  line = (struct Line *) line->ln_Link.mln_Succ;
               }
            } else
            {
               line = (struct Line *) line->ln_Link.mln_Succ;
            }
         }

         /* calculate new max width */
         if(adjusted)
         {
            LONG maxwidth = 0;
            line = (struct Line *) list->lh_Head;
            while(line->ln_Link.mln_Succ != NULL)
            {
               if(line->ln_XOffset + line->ln_Width > maxwidth)
                  maxwidth = line->ln_XOffset + line->ln_Width;
               line = (struct Line *) line->ln_Link.mln_Succ;
            }
            si->si_TotHoriz = maxwidth / si->si_HorizUnit;
         }
      }

      {
         LONG topv = si->si_TopVert;
         LONG toph = si->si_TopHoriz;
         if(si->si_TopVert + si->si_VisVert > si->si_TotVert)
            si->si_TopVert = si->si_TotVert - si->si_VisVert;
         if(si->si_TopVert < 0)
            si->si_TopVert = 0;

         if(si->si_TopHoriz + si->si_VisHoriz > si->si_TotHoriz)
            si->si_TopHoriz = si->si_TotHoriz - si->si_VisHoriz;
         if(si->si_TopHoriz < 0)
            si->si_TopHoriz = 0;
         if(topv != si->si_TopVert || toph != si->si_TopHoriz)
         {
            NotifyAttrs(obj,         msg->gpl_GInfo, 0,
         	        GA_ID,       CAST_GAD(obj)->GadgetID,
         	        DTA_TopVert,   si->si_TopVert,
         	        DTA_TopHoriz,  si->si_TopHoriz,
                        TAG_DONE);

         }
      }

      if(clear)
      {
         struct RastPort *rp;

         rp = ObtainGIRPort(msg->gpl_GInfo);
         if(rp != NULL)
         {
            EraseRect(rp, domain->Left, domain->Top,
                          domain->Left+domain->Width-1,
                          domain->Top+domain->Height-1);
            ReleaseGIRPort(rp);
         }
      }
   }

#if 0
   /* objects of this class are only used within the context of the
    * amigaguide.datatype and the class propagates any changes attributes
    * to the notify chain.!
    */
   NotifyAttrs(obj,         msg->gpl_GInfo, NULL,
	       GA_ID,       CAST_GAD(obj)->GadgetID,
	       DTA_TopVert,    si->si_TopVert,
	       DTA_TotalVert,  si->si_TotVert,
	       DTA_VertUnit,   si->si_VertUnit,
	       DTA_VisibleVert,si->si_VisVert,
	       DTA_TopHoriz,   si->si_TopHoriz,
	       DTA_TotalHoriz, si->si_TotHoriz,
	       DTA_HorizUnit,  si->si_HorizUnit,
	       DTA_VisibleHoriz,si->si_VisHoriz,
	       DTA_Busy,    FALSE,
	       DTA_Sync,    TRUE,
	       TAG_DONE);
#endif

   ReleaseSemaphore(&si->si_Lock);

   return lines;
}

static
ULONG dtm_trigger(Class *cl, Object *obj, struct dtTrigger *msg)
{
   INSTDATA;
   ULONG rv;

   switch(msg->dtt_Function & STMF_METHOD_MASK)
   {
   case STM_COMMAND:
      /* forward trigger command to root (amigaguide.datatype) object */
      rv = DoMethodA(data->n_RootObject, (Msg) msg);
      break;
   default:
      rv = DoSuperMethodA(cl, obj, (Msg) msg);
   }
   return rv;
}

static
ULONG nodeclass_dispatcher(Class *cl, Object *obj, Msg msg)
{
   ULONG rv = 0;

   switch(msg->MethodID)
   {
   case OM_NEW:
      if((rv = DoSuperMethodA(cl, obj, msg)) != 0)
      {
	 Object *newobj = (Object *) rv;

	 if((rv = om_new(cl, newobj, CAST_SET(msg))) == (ULONG) NULL)
	    CoerceMethod(cl, newobj, OM_DISPOSE);
      }
      break;
   case OM_DISPOSE:
      rv = om_dispose(cl, obj, msg);
      break;

   case OM_GET:
      rv = om_get(cl,obj,(struct opGet *) msg);
      break;

   case OM_UPDATE:
   case OM_SET:
      rv = DoSuperMethodA(cl, obj, (Msg) msg);

      rv += om_set(cl, obj, CAST_SET(msg));

      break;

   case DTM_PROCLAYOUT:
      rv = DoSuperMethodA(cl, obj, msg);

      /* fall through */
   case DTM_ASYNCLAYOUT:
      rv = dtm_asynclayout(cl, obj, CAST_GPL(msg));
      break;
   case DTM_TRIGGER:
      rv = dtm_trigger(cl, obj, (struct dtTrigger *) msg);
      break;
   default:
      rv = DoSuperMethodA(cl,obj,msg);
   }

   return rv;
}


/* ------------------------------ class init ------------------------------ */

static ClassCall
ULONG dispatcher REGARGS((REG(a0,Class *cl),
                          REG(a2,Object *obj),
                          REG(a1,Msg msg)))
{
   MREG(a0, Class *, cl);
   MREG(a2, Object *, obj);
   MREG(a1, Msg, msg);

   return nodeclass_dispatcher(cl, obj, msg);
}
#ifdef __MORPHOS__
static struct EmulLibEntry GATEhook = {
    TRAP_LIB, 0, (void (*)(void)) dispatcher
};
#endif
#ifdef __AROS__
AROS_UFH3S(IPTR, arosdispatcher,
    AROS_UFHA(Class *, cl, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(Msg, msg, A1))
{
    AROS_USERFUNC_INIT
    
    return dispatcher(cl, obj, msg);
    
    AROS_USERFUNC_EXIT
}
#endif

Class *MakeNodeClass(struct ClassBase *cb)
{
   Class *cl = NULL;

   if((cb->cb_TextDTBase = OpenLibrary("Libs:datatypes/text.datatype", 39)) != NULL)
   {
      if((cl = MakeClass("amigaguidenode.datatype", "text.datatype",NULL,sizeof(struct NodeData),0)) != NULL)
      {
#ifdef __MORPHOS__
         cl->cl_Dispatcher.h_Entry = (HOOKFUNC) &GATEhook;
#elif defined(__AROS__)
         cl->cl_Dispatcher.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(arosdispatcher);
#else
         cl->cl_Dispatcher.h_Entry = (HOOKFUNC) dispatcher;
#endif
	 cl->cl_UserData = (ULONG) cb;

	 AddClass(cl);
      }
   }
   return cl;
}

BOOL FreeNodeClass(struct ClassBase *cb, Class *cl)
{
   BOOL rv = FreeClass(cl);

   if(rv)
   {
      CloseLibrary(cb->cb_TextDTBase);
      cb->cb_TextDTBase = NULL;
   }
   return rv;
}

