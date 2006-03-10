/***************************************************************************

 TextEditor.mcc - Textediting MUI Custom Class
 Copyright (C) 1997-2000 Allan Odgaard
 Copyright (C) 2005 by TextEditor.mcc Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 TextEditor class Support Site:  http://www.sf.net/projects/texteditor-mcc

 $Id: ImportHook.c,v 1.26 2005/07/31 12:39:36 damato Exp $

***************************************************************************/

#include <string.h>

#include <proto/dos.h>

#include "TextEditor_mcc.h"
#include "private.h"

/*************************************************************************/

struct grow
{
	UWORD *array;
	
	int current;
	int max;

	APTR pool;
};

/*************************************************************************/



/*************************************************************************/

STATIC LONG GetHex(char *src)
{
	if ((src[0] >= '0' && src[0] <= '9')) return src[0] - '0';
	if ((src[0] >= 'a' && src[0] <= 'f')) return src[0] - 'a' + 10;
	if ((src[0] >= 'A' && src[0] <= 'F')) return src[0] - 'A' + 10;
	return -1;
}

/************************************************************************
 Convert a =XX string to it's value (into *val). Returns TRUE if
 conversion was successfull in that case *src_ptr will advanved as well.
*************************************************************************/
STATIC BOOL GetQP(char **src_ptr, unsigned char *val)
{
	unsigned char v;
	char *src = *src_ptr;
	int rc;

	rc = GetHex(src);
	if (rc != -1)
	{
		v = rc << 4;

		rc = GetHex(&src[1]);
		if (rc != -1)
		{
			v |= rc;
			*val = v;
			*src_ptr = src + 2;
			return TRUE;
		}
	}
	return FALSE;
}

/************************************************************************
 Reads out the next value at *src_ptr and advances src_ptr.
 Returns TRUE if succedded else FALSE
*************************************************************************/
STATIC BOOL GetLong(char **src_ptr, LONG *val)
{
	LONG chars = StrToLong(*src_ptr,val);
	if (chars != -1)
	{
		*src_ptr += chars;
		return TRUE;
	}
	return FALSE;
}

/************************************************************************
 Returns the end of line in the current line (pointing at the linefeed).
 If a 0 byte is encountered it returns the pointer to the 0 byte.

 This function also counts the number of tabs within this line.
*************************************************************************/
STATIC char *FindEOL(char *src, int *tabs_ptr)
{
	int tabs;
	char c;
	char *eol;

	tabs = 0;
	eol = src;

	while ((c = *eol))
	{
		if (c == '\t') tabs++;
		else if (c == '\r' || c == '\n') break;
		eol++;
	}

	if (tabs_ptr) *tabs_ptr = tabs;
	return eol;
}

/************************************************************************
 Adds two new values to the given grow. This function guarantees
 that there is at least space for 2 additional values.
*************************************************************************/
STATIC VOID AddToGrow(struct grow *grow, UWORD val1, UWORD val2)
{
	if (grow->current >= grow->max)
	{
		UWORD *new_array;

		if ((new_array = MyAllocPooled(grow->pool, sizeof(grow->array[0])*2*(grow->max+9)))) /* we reserve one more for the ending */
		{
			/* Copy old contents into new array */
			if (grow->array)
			{
				memcpy(new_array,grow->array,sizeof(grow->array[0])*2*grow->current);
				MyFreePooled(grow->pool,grow->array);
			}
			grow->array = new_array;
			grow->max += 8;
		}
	}

	if (grow->current < grow->max)
	{
		grow->array[grow->current*2] = val1;
		grow->array[grow->current*2+1] = val2;
		grow->current++;
	}
}

// searches through a string and returns TRUE if the string contains
// any text (except newlines) until the stopchar is found
static BOOL ContainsText(char *str, char stopchar)
{
  if(str)
  {
    BOOL foundText = FALSE;

    while(*str >= ' ')
    {
      if(*str == stopchar)
        return foundText;
      else if(*str > ' ') // greater than 0x20 (space) == readable text
        foundText = TRUE;

      str++;
    }
  }

  return FALSE;
}

/************************************************************************
 The plain import hook. It supports following escape sequences:

  <ESC> + u      Set the soft style to underline.
  <ESC> + b      Set the soft style to bold.
  <ESC> + i      Set the soft style to italic.
  <ESC> + n      Set the soft style back to normal.
  <ESC> + h      Highlight the current line.
  <ESC> + p[x]   Change to color x, where x is taken from the colormap.
                 0 means normal. The color is reset for each new line.


 The following sequences are only valid at the beginning of a line.

  <ESC> + l      Left justify current and following lines.
  <ESC> + r      Right justify current and following lines.
  <ESC> + c      Center current and following lines.
  <ESC> + [s:x]  Create a separator. x is a bit combination of flags:
                 Placement (mutually exclusive):
                     1 = Top
                     2 = Middle
                     4 = Bottom
                 Cosmetical:
                     8 = StrikeThru   - Draw separator ontop of text.
                     16 = Thick        - Make separator extra thick.

 Note: Tabs are converted to spaces with a tab size of 4.
*************************************************************************/
#ifdef __AROS__
AROS_HOOKPROTONHNO(PlainImportHookFunc, STRPTR, struct ImportMessage *, msg)
#else
HOOKPROTONHNO(PlainImportHookFunc, STRPTR, struct ImportMessage *msg)
#endif
{
    	HOOK_INIT
	
	char *eol;
	char *src = msg->Data;
	int len;
	int tabs;
	struct LineNode *line = msg->linenode;
	ULONG wrap = msg->ImportWrap;

	if (!(eol = FindEOL(src,&tabs)))
		return NULL;

	len = eol - src + 4 * tabs;

 /* allocate some more memory for the possible quote mark '>', note that if
  * a '=' is detected at the end of a line this memory is not sufficient! */
	if ((line->Contents = MyAllocPooled(msg->PoolHandle,len+4)))
	{
		unsigned char *dest_start = (unsigned char *)line->Contents;
		unsigned char *dest = dest_start;
		unsigned char *dest_word_start = dest_start;
		unsigned char *src_word_start = (unsigned char *)src;

		/* Style and color state */
		int state = 0;

		struct grow style_grow;
		struct grow color_grow;

		memset(&color_grow,0,sizeof(color_grow));
		memset(&style_grow,0,sizeof(style_grow));

		color_grow.pool = style_grow.pool = msg->PoolHandle;

		/* Copy loop */
		while (src < eol)
		{
			unsigned char c = *src++;

			if(c == '\t')
			{
				int i;
				for (i=(dest - dest_start)% 4; i < 4; i++)
					*dest++ = ' ';
				continue;
			}
      else if(c == '\033') // ESC sequence
			{
				switch(*src++)
				{
					case 'b':
    				AddToGrow(&style_grow, dest - dest_start + 1, BOLD);
            state |= BOLD;
          break;

					case 'i':
    				AddToGrow(&style_grow, dest - dest_start + 1, ITALIC);
            state |= ITALIC;
          break;

					case 'u':
    				AddToGrow(&style_grow, dest - dest_start + 1, UNDERLINE);
            state |= UNDERLINE;
					break;

          case 'h':
            line->Color = TRUE;
          break;

          case 'n':
            if(state & BOLD)      AddToGrow(&style_grow, dest - dest_start + 1, ~BOLD);
    				if(state & ITALIC)    AddToGrow(&style_grow, dest - dest_start + 1, ~ITALIC);
    				if(state & UNDERLINE) AddToGrow(&style_grow, dest - dest_start + 1, ~UNDERLINE);
            state ^= ~(BOLD | ITALIC | UNDERLINE);
          break;

					case 'l': line->Flow = MUIV_TextEditor_Flow_Left; break; // left
          case 'c': line->Flow = MUIV_TextEditor_Flow_Center; break; // centered
					case 'r': line->Flow = MUIV_TextEditor_Flow_Right; break; // right

					case 'p':
          {
					  if(*src == '[')
						{
						  LONG pen;
							src++;

              if(GetLong(&src,&pen))
							{
							  if(*src == ']')
								{
          				AddToGrow(&color_grow, dest - dest_start + 1, pen);

                  if(pen == 0)
                    state ^= COLOURED;
                  else
                    state |= COLOURED;

                  src++;
								}
							}
						}
          }
					break;

          case '[':
					{
            if(*src == 's')
						{
						  if(*(++src) == ':')
							{
							  LONG flags;
								src++;

                if(GetLong(&src,&flags))
								{
								  if(*src == ']')
									{
									  line->Separator = flags;
									  src++;
									}
								}
							}
						}
					}
          break;
				}

				continue;
			}

			if(c == ' ')
			{
				/* src is already advanced */
				src_word_start = (unsigned char *)src;
				dest_word_start = dest;
			}

			if (wrap && ((ULONG)(dest - dest_start)) >= wrap)
			{
				/* Only leave the loop, if we really have added some characters
				 * (at least one word) to the line */
				if (dest_word_start != dest_start)
				{
					/* src points to the real word start, but we add one when we return eol */
					eol = (char *)(src_word_start - 1);
					dest = dest_word_start;
					break;
				}
			}

			*dest++ = c;

		} /* while (src < eol) */

		line->Colors = color_grow.array;
		line->Styles = style_grow.array;

		/* Mark the end of the color array (space is ensured) */
		if (line->Colors)
		{
			line->Colors[color_grow.current*2] = ~0;
			line->Colors[color_grow.current*2+1] = 0;
		}

		/* Mark the end of the style array (space is ensured) */
		if (line->Styles)
		{
			line->Styles[style_grow.current*2] = ~0;
			line->Styles[style_grow.current*2+1] = 0;
		}

		*dest++ = '\n';
		*dest = 0;

		line->Length = dest - dest_start; /* this excludes \n */
	}

	if (!eol || eol[0] == 0) return NULL;
	return eol + 1;
	
	HOOK_EXIT
}
MakeHook(ImPlainHook, PlainImportHookFunc);

/************************************************************************
 The MIME import hook. It supports following escape sequences:

  <ESC> + u      Set the soft style to underline.
  <ESC> + b      Set the soft style to bold.
  <ESC> + i      Set the soft style to italic.
  <ESC> + n      Set the soft style back to normal.
  <ESC> + h      Highlight the current line.
  <ESC> + p[x]   Change to color x, where x is taken from the colormap.
                 0 means normal. The color is reset for each new line.


 The following sequences are only valid at the beginning of a line.

  <ESC> + l      Left justify current and following lines.
  <ESC> + r      Right justify current and following lines.
  <ESC> + c      Center current and following lines.
  <ESC> + [s:x]  Create a separator. x is a bit combination of flags:
                 Placement (mutually exclusive):
                     1 = Top
                     2 = Middle
                     4 = Bottom
                 Cosmetical:
                     8 = StrikeThru   - Draw separator ontop of text.
                     16 = Thick        - Make separator extra thick.

 Note: Tabs are converted to spaces with a tab size of 4.
*************************************************************************/
STATIC STRPTR MimeImport(struct ImportMessage *msg, LONG type)
{
	char *eol;
	char *src = msg->Data;
	int len;
	int tabs;
	struct LineNode *line = msg->linenode;
	ULONG wrap = msg->ImportWrap;

	if (!(eol = FindEOL(src,&tabs)))
		return NULL;

	len = eol - src + 4 * tabs;

 /* allocate some more memory for the possible quote mark '>', note that if
  * a '=' is detected at the end of a line this memory is not sufficient! */
	if ((line->Contents = MyAllocPooled(msg->PoolHandle,len+4)))
	{
    BOOL lastWasSeparator = TRUE;
		unsigned char *dest_start = (unsigned char *)line->Contents;
		unsigned char *dest = dest_start;
		unsigned char *dest_word_start = dest_start;
		unsigned char *src_word_start = (unsigned char *)src;
		
		/* Style and color state */
		int state = 0;
    int escstate = 0;
    int shownext = 0;

		struct grow style_grow;
		struct grow color_grow;

		memset(&color_grow,0,sizeof(color_grow));
		memset(&style_grow,0,sizeof(style_grow));

		color_grow.pool = style_grow.pool = msg->PoolHandle;

		if (src[0] == '>')
      line->Color = TRUE;
		else if (src[0] == '<')
		{
			if (src[1] == 's' && src[2] == 'b' && src[3] == '>')
			{
				line->Separator = 2;	
				src+= 4;
				line->Flow = MUIV_TextEditor_Flow_Center;
        line->clearFlow = TRUE;
			}
			else if (src[1] == 't' && src[2] == 's' && src[3] == 'b' && src[4] == '>')
			{
				src += 5;
				line->Separator = 18;
				line->Flow = MUIV_TextEditor_Flow_Center;
        line->clearFlow = TRUE;
			}
		}

		if (type == 2)
		{
			*dest++ = '>';
			line->Color = TRUE;
		}

		/* Copy loop */
		while (src < eol)
		{
      unsigned char c = *src++;

      if(c == '\n')
      {
        lastWasSeparator = TRUE;
      }
			else if(c == '\t')
			{
				int i;
				
        for(i=(dest - dest_start)% 4; i < 4; i++)
					*dest++ = ' ';

        lastWasSeparator = TRUE;

				continue;				
			}
      else if(c == '/')
			{
        if(escstate == 0)
        {
          if(shownext & ITALIC)
            shownext ^= ITALIC;
          else if((state & ITALIC) || (lastWasSeparator && ContainsText(src, '/')))
          {
				    AddToGrow(&style_grow, dest - dest_start + 1, (state & ITALIC) ? ~ITALIC : ITALIC);
				    state ^= ITALIC;

            lastWasSeparator = TRUE;
            continue;
          }
          else
            shownext |= ITALIC;
				}

        lastWasSeparator = TRUE;
			}
      else if(c == '*')
			{
        if(escstate == 0)
        {
          if(shownext & BOLD)
            shownext ^= BOLD;
          else if((state & BOLD) || (lastWasSeparator && ContainsText(src, '*')))
    			{
          	AddToGrow(&style_grow, dest - dest_start + 1, (state & BOLD) ? ~BOLD : BOLD);
	    			state ^= BOLD;

            lastWasSeparator = TRUE;
            continue;
          }
          else
            shownext |= BOLD;
		    }

        lastWasSeparator = TRUE;
			}
      else if(c == '_')
			{
        if(escstate == 0)
        {
          if(shownext & UNDERLINE)
            shownext ^= UNDERLINE;
          else if((state & UNDERLINE) || (lastWasSeparator && ContainsText(src, '_')))
  				{
            AddToGrow(&style_grow, dest - dest_start + 1, (state & UNDERLINE) ? ~UNDERLINE : UNDERLINE);
	  			  state ^= UNDERLINE;

            lastWasSeparator = TRUE;
            continue;
          }
          else
            shownext |= UNDERLINE;
        }

        lastWasSeparator = TRUE;
			}
      else if(c == '#')
			{
        if(escstate == 0)
        {
          if(shownext & COLOURED)
            shownext ^= COLOURED;
          else if((state & COLOURED) || (lastWasSeparator && ContainsText(src, '#')))
          {
  				  AddToGrow(&color_grow, dest - dest_start + 1, (state & COLOURED) ? 0 : 7);
	  			  state ^= COLOURED;

            lastWasSeparator = TRUE;
            continue;
          }
          else
            shownext |= COLOURED;
		    }

        lastWasSeparator = TRUE;
			}
      else if(c == '=')
			{
				/* This is a concatenated line */
				if(type > 0 && !GetQP(&src,&c))
				{
					int i;
					
					i = 0;
					if (src[0] == '\r') i++;

					if (src[i] == '\n')
					{
						unsigned char *new_dest_start;

						src += i + 1;

						if (!(eol = FindEOL(src,&tabs)))
							break;

						/* The size of the dest buffer has to be increased now */
						len += eol - src + 4 * tabs;
						
						if (!(new_dest_start = (unsigned char*)MyAllocPooled(msg->PoolHandle, len + 4)))
							break;

						memcpy(new_dest_start, dest_start, dest - dest_start);
						MyFreePooled(msg->PoolHandle,dest_start);

						/* Update all dest variables */
						dest_word_start = new_dest_start + (dest_word_start - dest_start);
						dest = new_dest_start + (dest - dest_start);
            line->Contents = (char *)new_dest_start;
						dest_start = (unsigned char *)line->Contents;

            lastWasSeparator = FALSE;
						continue;
					}
				}
			}
      else if(c == '\033') // like the plain import hook we manage ESC sequences as well
			{
				switch(*src++)
				{
					case 'b':
    				AddToGrow(&style_grow, dest - dest_start + 1, BOLD);
            escstate |= BOLD;
          break;

					case 'i':
    				AddToGrow(&style_grow, dest - dest_start + 1, ITALIC);
            escstate |= ITALIC;
          break;

					case 'u':
    				AddToGrow(&style_grow, dest - dest_start + 1, UNDERLINE);
            escstate |= UNDERLINE;
					break;

          case 'h':
            line->Color = TRUE;
          break;
					
          case 'n':
            if(state & BOLD)      AddToGrow(&style_grow, dest - dest_start + 1, ~BOLD);
    				if(state & ITALIC)    AddToGrow(&style_grow, dest - dest_start + 1, ~ITALIC);
    				if(state & UNDERLINE) AddToGrow(&style_grow, dest - dest_start + 1, ~UNDERLINE);
            state ^= ~(BOLD | ITALIC | UNDERLINE);
            escstate ^= ~(BOLD | ITALIC | UNDERLINE);
          break;

					case 'l': line->Flow = MUIV_TextEditor_Flow_Left; break; // left
          case 'c': line->Flow = MUIV_TextEditor_Flow_Center; break; // centered
					case 'r': line->Flow = MUIV_TextEditor_Flow_Right; break; // right

					case 'p':
          {
					  if(*src == '[')
						{
						  LONG pen;
							src++;
							
              if(GetLong(&src,&pen))
							{
							  if(*src == ']')
								{
          				AddToGrow(&color_grow, dest - dest_start + 1, pen);

                  if(pen == 0)
                    escstate ^= COLOURED;
                  else
                    escstate |= COLOURED;

                  src++;
								}
							}
						}
          }
					break;
					
          case '[':
					{
            if(*src == 's')
						{
						  if(*(++src) == ':')
							{
							  LONG flags;
								src++;
								
                if(GetLong(&src,&flags))
								{
								  if(*src == ']')
									{
									  line->Separator = flags;
									  src++;
									}
								}
							}
						}
					}
          break;
				}

        lastWasSeparator = FALSE;
				continue;
			}

			if(c == ' ')
			{
				/* src is already advanced */
				src_word_start = (unsigned char *)src;
				dest_word_start = dest;

        lastWasSeparator = TRUE;
			}
      else if(c == '\n')
      {
        lastWasSeparator = TRUE;
      }
      else
      {
        lastWasSeparator = FALSE;
      }

			if(wrap && ((ULONG)(dest - dest_start)) >= wrap)
			{
				/* Only leave the loop, if we really have added some characters
				 * (at least one word) to the line */
				if (dest_word_start != dest_start)
				{
					/* src points to the real word start, but we add one when we return eol */
					eol = (char *)(src_word_start - 1);
					dest = dest_word_start;
					break;
				}
			}

			*dest++ = c;
		} /* while (src < eol) */

		line->Colors = color_grow.array;
		line->Styles = style_grow.array;

		/* Mark the end of the color array (space is ensured) */
		if (line->Colors)
		{
			line->Colors[color_grow.current*2] = ~0;
			line->Colors[color_grow.current*2+1] = 0;
		}

		/* Mark the end of the style array (space is ensured) */
		if (line->Styles)
		{
			line->Styles[style_grow.current*2] = ~0;
			line->Styles[style_grow.current*2+1] = 0;
		}

		*dest++ = '\n';
		*dest = 0;

		line->Length = dest - dest_start; /* this excludes \n */
	}

	if (!eol || eol[0] == 0) return NULL;
	return eol + 1;
}

#ifdef __AROS__
AROS_HOOKPROTONHNO(EMailImportHookFunc, STRPTR , struct ImportMessage *, msg)
#else
HOOKPROTONHNO(EMailImportHookFunc, STRPTR , struct ImportMessage *msg)
#endif
{
    	HOOK_INIT
	
	return MimeImport(msg,0);
	
	HOOK_EXIT
}
MakeHook(ImEMailHook, EMailImportHookFunc);

#ifdef __AROS__
AROS_HOOKPROTONHNO(MIMEImportHookFunc, STRPTR, struct ImportMessage *, msg)
#else
HOOKPROTONHNO(MIMEImportHookFunc, STRPTR, struct ImportMessage *msg)
#endif
{
    	HOOK_INIT
	
	return MimeImport(msg,1);
	
	HOOK_EXIT
}
MakeHook(ImMIMEHook, MIMEImportHookFunc);

#ifdef __AROS__
AROS_HOOKPROTONHNO(MIMEQuoteImportHookFunc, STRPTR, struct ImportMessage *, msg)
#else
HOOKPROTONHNO(MIMEQuoteImportHookFunc, STRPTR, struct ImportMessage *msg)
#endif
{
    	HOOK_INIT
	
	return MimeImport(msg,2);
	
	HOOK_EXIT
}
MakeHook(ImMIMEQuoteHook, MIMEQuoteImportHookFunc);
