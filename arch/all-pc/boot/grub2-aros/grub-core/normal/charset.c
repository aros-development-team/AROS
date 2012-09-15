/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2002,2003,2004,2005,2006,2007,2008,2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
  Current problems with Unicode rendering: 
  - B and BN bidi type characters (ignored)
  - Mc type characters with combining class 0 (poorly combined)
  - Mn type characters with combining class 0 (poorly combined)
  - Me type characters with combining class 0 (poorly combined)
  - Cf type characters (ignored)
  - Cc type characters (ignored)
  - Line-breaking rules (e.g. Zs type characters)
  - Indic languages
  - non-Semitic shaping (rarely used)
  - Zl and Zp characters
  - Combining characters of types 7, 8, 9, 21, 35, 36, 84, 91, 103, 107,
  118, 122, 129, 130, 132, 218, 224, 226, 233, 234
  - Private use characters (not really a problem)
  - Variations (no font support)
  - Vertical text
  - Ligatures
  Font information ignored:
  - Kerning
  - Justification data
  - Glyph posititioning
  - Baseline data
  Most underline diacritics aren't displayed in gfxterm
 */

#include <grub/charset.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/unicode.h>
#include <grub/term.h>
#include <grub/normal.h>

#ifdef HAVE_UNIFONT_WIDTHSPEC
#include "widthspec.h"
#endif

/* Returns -2 if not enough space, -1 on invalid character.  */
grub_ssize_t
grub_encode_utf8_character (grub_uint8_t *dest, grub_uint8_t *destend,
			    grub_uint32_t code)
{
  if (dest >= destend)
    return -2;
  if (code <= 0x007F)
    {
      *dest++ = code;
      return 1;
    }
  if (code <= 0x07FF)
    {
      if (dest + 1 >= destend)
	return -2;
      *dest++ = (code >> 6) | 0xC0;
      *dest++ = (code & 0x3F) | 0x80;
      return 2;
    }
  if ((code >= 0xDC00 && code <= 0xDFFF)
      || (code >= 0xD800 && code <= 0xDBFF))
    {
      /* No surrogates in UCS-4... */
      return -1;
    }
  if (code < 0x10000)
    {
      if (dest + 2 >= destend)
	return -2;
      *dest++ = (code >> 12) | 0xE0;
      *dest++ = ((code >> 6) & 0x3F) | 0x80;
      *dest++ = (code & 0x3F) | 0x80;
      return 3;
    }
  {
    if (dest + 3 >= destend)
      return -2;
    *dest++ = (code >> 18) | 0xF0;
    *dest++ = ((code >> 12) & 0x3F) | 0x80;
    *dest++ = ((code >> 6) & 0x3F) | 0x80;
    *dest++ = (code & 0x3F) | 0x80;
    return 4;
  }

}

/* Convert UCS-4 to UTF-8.  */
grub_size_t
grub_ucs4_to_utf8 (const grub_uint32_t *src, grub_size_t size,
		   grub_uint8_t *dest, grub_size_t destsize)
{
  /* Keep last char for \0.  */
  grub_uint8_t *destend = dest + destsize - 1;
  grub_uint8_t *dest0 = dest;

  while (size-- && dest < destend)
    {
      grub_uint32_t code = *src++;
      grub_ssize_t s;
      s = grub_encode_utf8_character (dest, destend, code);
      if (s == -2)
	break;
      if (s == -1)
	{
	  *dest++ = '?';
	  continue;
	}
      dest += s;
    }
  *dest = 0;
  return dest - dest0;
}

/* Returns the number of bytes the string src would occupy is converted
   to UTF-8, excluding trailing \0.  */
grub_size_t
grub_get_num_of_utf8_bytes (const grub_uint32_t *src, grub_size_t size)
{
  grub_size_t remaining;
  const grub_uint32_t *ptr;
  grub_size_t cnt = 0;

  remaining = size;
  ptr = src;
  while (remaining--)
    {
      grub_uint32_t code = *ptr++;
      
      if (code <= 0x007F)
	cnt++;
      else if (code <= 0x07FF)
	cnt += 2;
      else if ((code >= 0xDC00 && code <= 0xDFFF)
	       || (code >= 0xD800 && code <= 0xDBFF))
	/* No surrogates in UCS-4... */
	cnt++;
      else if (code < 0x10000)
	cnt += 3;
      else
	cnt += 4;
    }
  return cnt;
}

/* Convert UCS-4 to UTF-8.  */
char *
grub_ucs4_to_utf8_alloc (const grub_uint32_t *src, grub_size_t size)
{
  grub_uint8_t *ret;
  grub_size_t cnt = grub_get_num_of_utf8_bytes (src, size) + 1;

  ret = grub_malloc (cnt);
  if (!ret)
    return 0;

  grub_ucs4_to_utf8 (src, size, ret, cnt);

  return (char *) ret;
}

int
grub_is_valid_utf8 (const grub_uint8_t *src, grub_size_t srcsize)
{
  int count = 0;
  grub_uint32_t code = 0;

  while (srcsize)
    {
      if (srcsize != (grub_size_t)-1)
	srcsize--;
      if (!grub_utf8_process (*src++, &code, &count))
	return 0;
      if (count != 0)
	continue;
      if (code == 0)
	return 1;
      if (code > GRUB_UNICODE_LAST_VALID)
	return 0;
    }

  return 1;
}

grub_ssize_t
grub_utf8_to_ucs4_alloc (const char *msg, grub_uint32_t **unicode_msg,
			 grub_uint32_t **last_position)
{
  grub_size_t msg_len = grub_strlen (msg);

  *unicode_msg = grub_malloc (msg_len * sizeof (grub_uint32_t));
 
  if (!*unicode_msg)
    return -1;

  msg_len = grub_utf8_to_ucs4 (*unicode_msg, msg_len,
  			      (grub_uint8_t *) msg, -1, 0);

  if (last_position)
    *last_position = *unicode_msg + msg_len;

  return msg_len;
}

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UCS-4 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
grub_size_t
grub_utf8_to_ucs4 (grub_uint32_t *dest, grub_size_t destsize,
		   const grub_uint8_t *src, grub_size_t srcsize,
		   const grub_uint8_t **srcend)
{
  grub_uint32_t *p = dest;
  int count = 0;
  grub_uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
    {
      int was_count = count;
      if (srcsize != (grub_size_t)-1)
	srcsize--;
      if (!grub_utf8_process (*src++, &code, &count))
	{
	  code = '?';
	  count = 0;
	  /* Character c may be valid, don't eat it.  */
	  if (was_count)
	    src--;
	}
      if (count != 0)
	continue;
      if (code == 0)
	break;
      *p++ = code;
      destsize--;
    }

  if (srcend)
    *srcend = src;
  return p - dest;
}

static grub_uint8_t *join_types = NULL;

static void
unpack_join (void)
{
  unsigned i;
  struct grub_unicode_compact_range *cur;

  join_types = grub_zalloc (GRUB_UNICODE_MAX_CACHED_CHAR);
  if (!join_types)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  for (cur = grub_unicode_compact; cur->len; cur++)
    for (i = cur->start; i < cur->start + (unsigned) cur->len
	   && i < GRUB_UNICODE_MAX_CACHED_CHAR; i++)
      join_types[i] = cur->join_type;
}

static grub_uint8_t *bidi_types = NULL;

static void
unpack_bidi (void)
{
  unsigned i;
  struct grub_unicode_compact_range *cur;

  bidi_types = grub_zalloc (GRUB_UNICODE_MAX_CACHED_CHAR);
  if (!bidi_types)
    {
      grub_errno = GRUB_ERR_NONE;
      return;
    }
  for (cur = grub_unicode_compact; cur->len; cur++)
    for (i = cur->start; i < cur->start + (unsigned) cur->len
	   && i < GRUB_UNICODE_MAX_CACHED_CHAR; i++)
      if (cur->bidi_mirror)
	bidi_types[i] = cur->bidi_type | 0x80;
      else
	bidi_types[i] = cur->bidi_type | 0x00;
}

static inline enum grub_bidi_type
get_bidi_type (grub_uint32_t c)
{
  struct grub_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return bidi_types[c] & 0x7f;

  for (cur = grub_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->bidi_type;

  return GRUB_BIDI_TYPE_L;
}

static inline enum grub_join_type
get_join_type (grub_uint32_t c)
{
  struct grub_unicode_compact_range *cur;

  if (!join_types)
    unpack_join ();

  if (join_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return join_types[c];

  for (cur = grub_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->join_type;

  return GRUB_JOIN_TYPE_NONJOINING;
}

static inline int
is_mirrored (grub_uint32_t c)
{
  struct grub_unicode_compact_range *cur;

  if (!bidi_types)
    unpack_bidi ();

  if (bidi_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return !!(bidi_types[c] & 0x80);

  for (cur = grub_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->bidi_mirror;

  return 0;
}

enum grub_comb_type
grub_unicode_get_comb_type (grub_uint32_t c)
{
  static grub_uint8_t *comb_types = NULL;
  struct grub_unicode_compact_range *cur;

  if (!comb_types)
    {
      unsigned i;
      comb_types = grub_zalloc (GRUB_UNICODE_MAX_CACHED_CHAR);
      if (comb_types)
	for (cur = grub_unicode_compact; cur->len; cur++)
	  for (i = cur->start; i < cur->start + (unsigned) cur->len
		 && i < GRUB_UNICODE_MAX_CACHED_CHAR; i++)
	    comb_types[i] = cur->comb_type;
      else
	grub_errno = GRUB_ERR_NONE;
    }

  if (comb_types && c < GRUB_UNICODE_MAX_CACHED_CHAR)
    return comb_types[c];

  for (cur = grub_unicode_compact; cur->len; cur++)
    if (cur->start <= c && c < cur->start + (unsigned) cur->len)
      return cur->comb_type;

  return GRUB_UNICODE_COMB_NONE;
}

#ifdef HAVE_UNIFONT_WIDTHSPEC

grub_ssize_t
grub_unicode_estimate_width (const struct grub_unicode_glyph *c)
{
  if (grub_unicode_get_comb_type (c->base))
    return 0;
  if (widthspec[c->base >> 3] & (1 << (c->base & 7)))
    return 2;
  else
    return 1;
}

#endif

static inline int
is_type_after (enum grub_comb_type a, enum grub_comb_type b)
{
  /* Shadda is numerically higher than most of Arabic diacritics but has
     to be rendered before them.  */
  if (a == GRUB_UNICODE_COMB_ARABIC_SHADDA 
      && b <= GRUB_UNICODE_COMB_ARABIC_KASRA
      && b >= GRUB_UNICODE_COMB_ARABIC_FATHATAN)
    return 0;
  if (b == GRUB_UNICODE_COMB_ARABIC_SHADDA 
      && a <= GRUB_UNICODE_COMB_ARABIC_KASRA
      && a >= GRUB_UNICODE_COMB_ARABIC_FATHATAN)
    return 1;
  return a > b;
}

grub_size_t
grub_unicode_aglomerate_comb (const grub_uint32_t *in, grub_size_t inlen,
			      struct grub_unicode_glyph *out)
{
  int haveout = 0;
  const grub_uint32_t *ptr;
  unsigned last_comb_pointer = 0;

  grub_memset (out, 0, sizeof (*out));

  if (inlen && grub_iscntrl (*in))
    {
      out->base = *in;
      out->variant = 0;
      out->attributes = 0;
      out->ncomb = 0;
      out->estimated_width = 1;
      out->combining = NULL;
      return 1;
    }

  for (ptr = in; ptr < in + inlen; ptr++)
    {
      /* Variation selectors >= 17 are outside of BMP and SMP. 
	 Handle variation selectors first to avoid potentially costly lookups.
      */
      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_16)
	{
	  if (haveout)
	    out->variant = *ptr - GRUB_UNICODE_VARIATION_SELECTOR_1 + 1;
	  continue;
	}
      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_256)
	{
	  if (haveout)
	    out->variant = *ptr - GRUB_UNICODE_VARIATION_SELECTOR_17 + 17;
	  continue;
	}
	
      enum grub_comb_type comb_type;
      comb_type = grub_unicode_get_comb_type (*ptr);
      if (comb_type)
	{
	  struct grub_unicode_combining *n;
	  unsigned j;

	  if (!haveout)
	    continue;

	  if (comb_type == GRUB_UNICODE_COMB_MC
	      || comb_type == GRUB_UNICODE_COMB_ME
	      || comb_type == GRUB_UNICODE_COMB_MN)
	    last_comb_pointer = out->ncomb;
	  n = grub_realloc (out->combining,
			    sizeof (n[0]) * (out->ncomb + 1));
	  if (!n)
	    {
	      grub_errno = GRUB_ERR_NONE;
	      continue;
	    }
	  out->combining = n;

	  for (j = last_comb_pointer; j < out->ncomb; j++)
	    if (is_type_after (out->combining[j].type, comb_type))
	      break;
	  grub_memmove (out->combining + j + 1,
			out->combining + j,
			(out->ncomb - j)
			* sizeof (out->combining[0]));
	  out->combining = n;
	  out->combining[j].code = *ptr;
	  out->combining[j].type = comb_type;
	  out->ncomb++;
	  continue;
	}
      if (haveout)
	return ptr - in;
      haveout = 1;
      out->base = *ptr;
      out->variant = 0;
      out->attributes = 0;
      out->ncomb = 0;
      out->estimated_width = 1;
      out->combining = NULL;
    }
  return ptr - in;
}

static grub_ssize_t
bidi_line_wrap (struct grub_unicode_glyph *visual_out,
		struct grub_unicode_glyph *visual,
		grub_size_t visual_len, unsigned *levels,
		grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
		grub_size_t maxwidth, grub_size_t startwidth)
{
  struct grub_unicode_glyph *outptr = visual_out;
  unsigned line_start = 0;
  grub_ssize_t line_width = startwidth;
  unsigned k;
  grub_ssize_t last_space = -1;
  grub_ssize_t last_space_width = 0;

  auto void revert (unsigned start, unsigned end);
  void revert (unsigned start, unsigned end)
  {
    struct grub_unicode_glyph t;
    unsigned i, tl;
    for (i = 0; i < (end - start) / 2 + 1; i++)
      {
	t = visual[start + i];
	visual[start + i] = visual[end - i];
	visual[end - i] = t;
	tl = levels[start + i];
	levels[start + i] = levels[end - i];
	levels[end - i] = tl;
      }
  }

  if (!visual_len)
    return 0;

  for (k = 0; k <= visual_len; k++)
    {
      grub_ssize_t last_width = 0;

      if (getcharwidth && k != visual_len)
	line_width += last_width = getcharwidth (&visual[k]);

      if (k != visual_len && (visual[k].base == ' '
			      || visual[k].base == '\t'))
	{
	  last_space = k;
	  last_space_width = line_width;
	}

      if (((grub_ssize_t) maxwidth > 0 
	   && line_width > (grub_ssize_t) maxwidth) || k == visual_len)
	{	  
	  unsigned min_odd_level = 0xffffffff;
	  unsigned max_level = 0;

	  if (k != visual_len && last_space > (signed) line_start)
	    k = last_space;
	  else if (k != visual_len && line_start == 0 && startwidth != 0)
	    {
	      k = 0;
	      last_space_width = startwidth;
	    }
	  else
	    last_space_width = line_width - last_width;

	  {
	    unsigned i;
	    for (i = line_start; i < k; i++)
	      {
		if (levels[i] > max_level)
		  max_level = levels[i];
		if (levels[i] < min_odd_level && (levels[i] & 1))
		  min_odd_level = levels[i];
	      }
	  }

	  {
	    unsigned j;	  
	    /* FIXME: can be optimized.  */
	    for (j = max_level; j > min_odd_level - 1; j--)
	      {
		unsigned in = line_start;
		unsigned i;
		for (i = line_start; i < k; i++)
		  {
		    if (i != line_start && levels[i] >= j && levels[i-1] < j)
		      in = i;
		    if (levels[i] >= j && (i + 1 == k || levels[i+1] < j))
		      revert (in, i);
		  }
	      }
	  }
	  
	  {
	    unsigned i;
	    for (i = line_start; i < k; i++)
	      {
		if (is_mirrored (visual[i].base) && levels[i])
		  visual[i].attributes |= GRUB_UNICODE_GLYPH_ATTRIBUTE_MIRROR;
		if ((visual[i].attributes & GRUB_UNICODE_GLYPH_ATTRIBUTES_JOIN)
		    && levels[i])
		  {
		    int left, right;
		    left = visual[i].attributes
		      & (GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED 
			 | GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT);
		    right = visual[i].attributes
		      & (GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED 
			 | GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT);
		    visual[i].attributes &= ~GRUB_UNICODE_GLYPH_ATTRIBUTES_JOIN;
		    left <<= GRUB_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT;
		    right >>= GRUB_UNICODE_GLYPH_ATTRIBUTES_JOIN_LEFT_TO_RIGHT_SHIFT;
		    visual[i].attributes |= (left | right);
		  }
	      }
	  }

	  {
	    int left_join = 0;
	    unsigned i;
	    for (i = line_start; i < k; i++)
	      {
		enum grub_join_type join_type = get_join_type (visual[i].base);
		if (!(visual[i].attributes
		      & GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT)
		    && (join_type == GRUB_JOIN_TYPE_LEFT
			|| join_type == GRUB_JOIN_TYPE_DUAL))
		  {
		    if (left_join)
		      visual[i].attributes
			|= GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		    else
		      visual[i].attributes
			&= ~GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		  }
		if (join_type == GRUB_JOIN_TYPE_NONJOINING
		    || join_type == GRUB_JOIN_TYPE_LEFT)
		  left_join = 0;
		if (join_type == GRUB_JOIN_TYPE_RIGHT
		    || join_type == GRUB_JOIN_TYPE_DUAL
		    || join_type == GRUB_JOIN_TYPE_CAUSING)
		  left_join = 1;
	      }
	  }

	  {
	    int right_join = 0;
	    signed i;
	    for (i = k - 1; i >= 0 && (unsigned) i + 1 > line_start;
		 i--)
	      {
		enum grub_join_type join_type = get_join_type (visual[i].base);
		if (!(visual[i].attributes
		      & GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT)
		    && (join_type == GRUB_JOIN_TYPE_RIGHT
			|| join_type == GRUB_JOIN_TYPE_DUAL))
		  {
		    if (right_join)
		      visual[i].attributes
			|= GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
		    else
		      visual[i].attributes
			&= ~GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
		  }
		if (join_type == GRUB_JOIN_TYPE_NONJOINING
		    || join_type == GRUB_JOIN_TYPE_RIGHT)
		  right_join = 0;
		if (join_type == GRUB_JOIN_TYPE_LEFT
		    || join_type == GRUB_JOIN_TYPE_DUAL
		    || join_type == GRUB_JOIN_TYPE_CAUSING)
		  right_join = 1;
	      }
	  }		

	  grub_memcpy (outptr, &visual[line_start],
		       (k - line_start) * sizeof (visual[0]));
	  outptr += k - line_start;
	  if (k != visual_len)
	    {
	      grub_memset (outptr, 0, sizeof (visual[0]));
	      outptr->base = '\n';
	      outptr++;
	    }

	  if ((signed) k == last_space)
	    k++;

	  line_start = k;
	  line_width -= last_space_width;
	}
    }

  return outptr - visual_out;
}


static grub_ssize_t
grub_bidi_line_logical_to_visual (const grub_uint32_t *logical,
				  grub_size_t logical_len,
				  struct grub_unicode_glyph *visual_out,
				  grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
				  grub_size_t maxwidth, grub_size_t startwidth)
{
  enum grub_bidi_type type = GRUB_BIDI_TYPE_L;
  enum override_status {OVERRIDE_NEUTRAL = 0, OVERRIDE_R, OVERRIDE_L};
  unsigned *levels;
  enum grub_bidi_type *resolved_types;
  unsigned base_level;
  enum override_status cur_override;
  unsigned i;
  unsigned stack_level[GRUB_BIDI_MAX_EXPLICIT_LEVEL + 3];
  enum override_status stack_override[GRUB_BIDI_MAX_EXPLICIT_LEVEL + 3];
  unsigned stack_depth = 0;
  unsigned invalid_pushes = 0;
  unsigned visual_len = 0;
  unsigned run_start, run_end;
  struct grub_unicode_glyph *visual;
  unsigned cur_level;
  int bidi_needed = 0;

  auto void push_stack (unsigned new_override, unsigned new_level);
  void push_stack (unsigned new_override, unsigned new_level)
  {
    if (new_level > GRUB_BIDI_MAX_EXPLICIT_LEVEL)
      {
	invalid_pushes++;
	return;
      }
    stack_level[stack_depth] = cur_level;
    stack_override[stack_depth] = cur_override;
    stack_depth++;
    cur_level = new_level;
    cur_override = new_override;
  }

  auto void pop_stack (void);
  void pop_stack (void)
  {
    if (invalid_pushes)
      {
	invalid_pushes--;
	return;
      }
    if (!stack_depth)
      return;
    stack_depth--;
    cur_level = stack_level[stack_depth];
    cur_override = stack_override[stack_depth];
  }

  levels = grub_malloc (sizeof (levels[0]) * logical_len);
  if (!levels)
    return -1;

  resolved_types = grub_malloc (sizeof (resolved_types[0]) * logical_len);
  if (!resolved_types)
    {
      grub_free (levels);
      return -1;
    }

  visual = grub_malloc (sizeof (visual[0]) * logical_len);
  if (!visual)
    {
      grub_free (resolved_types);
      grub_free (levels);
      return -1;
    }

  for (i = 0; i < logical_len; i++)
    {
      type = get_bidi_type (logical[i]);
      if (type == GRUB_BIDI_TYPE_L || type == GRUB_BIDI_TYPE_AL
	  || type == GRUB_BIDI_TYPE_R)
	break;
    }
  if (type == GRUB_BIDI_TYPE_R || type == GRUB_BIDI_TYPE_AL)
    base_level = 1;
  else
    base_level = 0;
  
  cur_level = base_level;
  cur_override = OVERRIDE_NEUTRAL;
  {
    const grub_uint32_t *lptr;
    enum {JOIN_DEFAULT, NOJOIN, JOIN_FORCE} join_state = JOIN_DEFAULT;
    int zwj_propagate_to_previous = 0;
    for (lptr = logical; lptr < logical + logical_len;)
      {
	grub_size_t p;

	if (*lptr == GRUB_UNICODE_ZWJ)
	  {
	    if (zwj_propagate_to_previous)
	      {
		visual[visual_len - 1].attributes
		  |= GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT
		  | GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
	      }
	    zwj_propagate_to_previous = 0;
	    join_state = JOIN_FORCE;
	    lptr++;
	    continue;
	  }

	if (*lptr == GRUB_UNICODE_ZWNJ)
	  {
	    if (zwj_propagate_to_previous)
	      {
		visual[visual_len - 1].attributes
		  |= GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED_EXPLICIT;
		visual[visual_len - 1].attributes 
		  &= ~GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED;
	      }
	    zwj_propagate_to_previous = 0;
	    join_state = NOJOIN;
	    lptr++;
	    continue;
	  }

	/* The tags: deprecated, never used.  */
	if (*lptr >= GRUB_UNICODE_TAG_START && *lptr <= GRUB_UNICODE_TAG_END)
	  continue;

	p = grub_unicode_aglomerate_comb (lptr, logical + logical_len - lptr, 
					  &visual[visual_len]);
	
	type = get_bidi_type (visual[visual_len].base);
	switch (type)
	  {
	  case GRUB_BIDI_TYPE_RLE:
	    bidi_needed = 1;
	    push_stack (cur_override, (cur_level | 1) + 1);
	    break;
	  case GRUB_BIDI_TYPE_RLO:
	    bidi_needed = 1;
	    push_stack (OVERRIDE_R, (cur_level | 1) + 1);
	    break;
	  case GRUB_BIDI_TYPE_LRE:
	    push_stack (cur_override, (cur_level & ~1) + 2);
	    break;
	  case GRUB_BIDI_TYPE_LRO:
	    push_stack (OVERRIDE_L, (cur_level & ~1) + 2);
	    break;
	  case GRUB_BIDI_TYPE_PDF:
	    pop_stack ();
	    break;
	  case GRUB_BIDI_TYPE_BN:
	    break;
	  case GRUB_BIDI_TYPE_R:
	  case GRUB_BIDI_TYPE_AL:
	    bidi_needed = 1;
	  default:
	    {
	      if (join_state == JOIN_FORCE)
		{
		  visual[visual_len].attributes
		    |= GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT
		    | GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		}
	      
	      if (join_state == NOJOIN)
		{
		  visual[visual_len].attributes
		    |= GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED_EXPLICIT;
		  visual[visual_len].attributes
		    &= ~GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED;
		}

	      join_state = JOIN_DEFAULT;
	      zwj_propagate_to_previous = 1;

	      levels[visual_len] = cur_level;
	      if (cur_override != OVERRIDE_NEUTRAL)
		resolved_types[visual_len] = 
		  (cur_override == OVERRIDE_L) ? GRUB_BIDI_TYPE_L
		  : GRUB_BIDI_TYPE_R;
	      else
		resolved_types[visual_len] = type;
	      visual_len++;
	    }
	  }
	lptr += p;
      }
  }

  if (bidi_needed)
    {
      for (run_start = 0; run_start < visual_len; run_start = run_end)
	{
	  unsigned prev_level, next_level, cur_run_level;
	  unsigned last_type, last_strong_type;
	  for (run_end = run_start; run_end < visual_len &&
		 levels[run_end] == levels[run_start]; run_end++);
	  if (run_start == 0)
	    prev_level = base_level;
	  else
	    prev_level = levels[run_start - 1];
	  if (run_end == visual_len)
	    next_level = base_level;
	  else
	    next_level = levels[run_end];
	  cur_run_level = levels[run_start];
	  if (prev_level & 1)
	    last_type = GRUB_BIDI_TYPE_R;
	  else
	    last_type = GRUB_BIDI_TYPE_L;
	  last_strong_type = last_type;
	  for (i = run_start; i < run_end; i++)
	    {
	      switch (resolved_types[i])
		{
		case GRUB_BIDI_TYPE_NSM:
		  resolved_types[i] = last_type;
		  break;
		case GRUB_BIDI_TYPE_EN:
		  if (last_strong_type == GRUB_BIDI_TYPE_AL)
		    resolved_types[i] = GRUB_BIDI_TYPE_AN;
		  break;
		case GRUB_BIDI_TYPE_L:
		case GRUB_BIDI_TYPE_R:
		  last_strong_type = resolved_types[i];
		  break;
		case GRUB_BIDI_TYPE_ES:
		  if (last_type == GRUB_BIDI_TYPE_EN
		      && i + 1 < run_end 
		      && resolved_types[i + 1] == GRUB_BIDI_TYPE_EN)
		    resolved_types[i] = GRUB_BIDI_TYPE_EN;
		  else
		    resolved_types[i] = GRUB_BIDI_TYPE_ON;
		  break;
		case GRUB_BIDI_TYPE_ET:
		  {
		    unsigned j;
		    if (last_type == GRUB_BIDI_TYPE_EN)
		      {
			resolved_types[i] = GRUB_BIDI_TYPE_EN;
			break;
		      }
		    for (j = i; j < run_end
			   && resolved_types[j] == GRUB_BIDI_TYPE_ET; j++);
		    if (j != run_end && resolved_types[j] == GRUB_BIDI_TYPE_EN)
		      {
			for (; i < run_end
			       && resolved_types[i] == GRUB_BIDI_TYPE_ET; i++)
			  resolved_types[i] = GRUB_BIDI_TYPE_EN;
			i--;
			break;
		      }
		    for (; i < run_end
			   && resolved_types[i] == GRUB_BIDI_TYPE_ET; i++)
		      resolved_types[i] = GRUB_BIDI_TYPE_ON;
		    i--;
		    break;		
		  }
		  break;
		case GRUB_BIDI_TYPE_CS:
		  if (last_type == GRUB_BIDI_TYPE_EN
		      && i + 1 < run_end 
		      && resolved_types[i + 1] == GRUB_BIDI_TYPE_EN)
		    {
		      resolved_types[i] = GRUB_BIDI_TYPE_EN;
		      break;
		    }
		  if (last_type == GRUB_BIDI_TYPE_AN
		      && i + 1 < run_end 
		      && (resolved_types[i + 1] == GRUB_BIDI_TYPE_AN
			  || (resolved_types[i + 1] == GRUB_BIDI_TYPE_EN
			      && last_strong_type == GRUB_BIDI_TYPE_AL)))
		    {
		      resolved_types[i] = GRUB_BIDI_TYPE_EN;
		      break;
		    }
		  resolved_types[i] = GRUB_BIDI_TYPE_ON;
		  break;
		case GRUB_BIDI_TYPE_AL:
		  last_strong_type = resolved_types[i];
		  resolved_types[i] = GRUB_BIDI_TYPE_R;
		  break;
		default: /* Make GCC happy.  */
		  break;
		}
	      last_type = resolved_types[i];
	      if (resolved_types[i] == GRUB_BIDI_TYPE_EN
		  && last_strong_type == GRUB_BIDI_TYPE_L)
		resolved_types[i] = GRUB_BIDI_TYPE_L;
	    }
	  if (prev_level & 1)
	    last_type = GRUB_BIDI_TYPE_R;
	  else
	    last_type = GRUB_BIDI_TYPE_L;
	  for (i = run_start; i < run_end; )
	    {
	      unsigned j;
	      unsigned next_type;
	      for (j = i; j < run_end &&
		     (resolved_types[j] == GRUB_BIDI_TYPE_B
		      || resolved_types[j] == GRUB_BIDI_TYPE_S
		      || resolved_types[j] == GRUB_BIDI_TYPE_WS
		      || resolved_types[j] == GRUB_BIDI_TYPE_ON); j++);
	      if (j == i)
		{
		  if (resolved_types[i] == GRUB_BIDI_TYPE_L)
		    last_type = GRUB_BIDI_TYPE_L;
		  else
		    last_type = GRUB_BIDI_TYPE_R;
		  i++;
		  continue;
		}
	      if (j == run_end)
		next_type = (next_level & 1) ? GRUB_BIDI_TYPE_R : GRUB_BIDI_TYPE_L;
	      else
		{
		  if (resolved_types[j] == GRUB_BIDI_TYPE_L)
		    next_type = GRUB_BIDI_TYPE_L;
		  else
		    next_type = GRUB_BIDI_TYPE_R;
		}
	      if (next_type == last_type)
		for (; i < j; i++)
		  resolved_types[i] = last_type;
	      else
		for (; i < j; i++)
		  resolved_types[i] = (cur_run_level & 1) ? GRUB_BIDI_TYPE_R
		    : GRUB_BIDI_TYPE_L;
	    }
	}

      for (i = 0; i < visual_len; i++)
	{
	  if (!(levels[i] & 1) && resolved_types[i] == GRUB_BIDI_TYPE_R)
	    {
	      levels[i]++;
	      continue;
	    }
	  if (!(levels[i] & 1) && (resolved_types[i] == GRUB_BIDI_TYPE_AN
				   || resolved_types[i] == GRUB_BIDI_TYPE_EN))
	    {
	      levels[i] += 2;
	      continue;
	    }
	  if ((levels[i] & 1) && (resolved_types[i] == GRUB_BIDI_TYPE_L
				  || resolved_types[i] == GRUB_BIDI_TYPE_AN
				  || resolved_types[i] == GRUB_BIDI_TYPE_EN))
	    {
	      levels[i]++;
	      continue;
	    }
	}
    }
  else
    {
      for (i = 0; i < visual_len; i++)
	levels[i] = 0;
    }
  grub_free (resolved_types);

  {
    grub_ssize_t ret;
    ret = bidi_line_wrap (visual_out, visual, visual_len, levels, 
			  getcharwidth, maxwidth, startwidth);
    grub_free (levels);
    grub_free (visual);
    return ret;
  }
}

grub_ssize_t
grub_bidi_logical_to_visual (const grub_uint32_t *logical,
			     grub_size_t logical_len,
			     struct grub_unicode_glyph **visual_out,
			     grub_ssize_t (*getcharwidth) (const struct grub_unicode_glyph *visual),
			     grub_size_t max_length, grub_size_t startwidth)
{
  const grub_uint32_t *line_start = logical, *ptr;
  struct grub_unicode_glyph *visual_ptr;
  *visual_out = visual_ptr = grub_malloc (2 * sizeof (visual_ptr[0])
					  * logical_len);
  if (!visual_ptr)
    return -1;
  for (ptr = logical; ptr <= logical + logical_len; ptr++)
    {
      if (ptr == logical + logical_len || *ptr == '\n')
	{
	  grub_ssize_t ret;
	  ret = grub_bidi_line_logical_to_visual (line_start,
						  ptr - line_start,
						  visual_ptr,
						  getcharwidth,
						  max_length,
						  startwidth);
	  startwidth = 0;

	  if (ret < 0)
	    {
	      grub_free (*visual_out);
	      return ret;
	    }
	  visual_ptr += ret; 
	  line_start = ptr;
	  if (ptr != logical + logical_len)
	    {
	      grub_memset (visual_ptr, 0, sizeof (visual_ptr[0]));
	      visual_ptr->base = '\n';
	      visual_ptr++;
	      line_start++;
	    }
	}
    }
  return visual_ptr - *visual_out;
}

grub_uint32_t
grub_unicode_mirror_code (grub_uint32_t in)
{
  int i;
  for (i = 0; grub_unicode_bidi_pairs[i].key; i++)
    if (grub_unicode_bidi_pairs[i].key == in)
      return grub_unicode_bidi_pairs[i].replace;
  return in;
}

grub_uint32_t
grub_unicode_shape_code (grub_uint32_t in, grub_uint8_t attr)
{
  int i;
  if (!(in >= GRUB_UNICODE_ARABIC_START
	&& in < GRUB_UNICODE_ARABIC_END))
    return in;

  for (i = 0; grub_unicode_arabic_shapes[i].code; i++)
    if (grub_unicode_arabic_shapes[i].code == in)
      {
	grub_uint32_t out = 0;
	switch (attr & (GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED
			| GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED))
	  {
	  case 0:
	    out = grub_unicode_arabic_shapes[i].isolated;
	    break;
	  case GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED:
	    out = grub_unicode_arabic_shapes[i].right_linked;
	    break;
	  case GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED:
	    out = grub_unicode_arabic_shapes[i].left_linked;
	    break;
	  case GRUB_UNICODE_GLYPH_ATTRIBUTE_RIGHT_JOINED
	    |GRUB_UNICODE_GLYPH_ATTRIBUTE_LEFT_JOINED:
	    out = grub_unicode_arabic_shapes[i].both_linked;
	    break;
	  }
	if (out)
	  return out;
      }

  return in;
}

const grub_uint32_t *
grub_unicode_get_comb_start (const grub_uint32_t *str, 
			     const grub_uint32_t *cur)
{
  const grub_uint32_t *ptr;
  for (ptr = cur; ptr >= str; ptr--)
    {
      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_1
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_16)
	continue;

      if (*ptr >= GRUB_UNICODE_VARIATION_SELECTOR_17
	  && *ptr <= GRUB_UNICODE_VARIATION_SELECTOR_256)
	continue;
	
      enum grub_comb_type comb_type;
      comb_type = grub_unicode_get_comb_type (*ptr);
      if (comb_type)
	continue;
      return ptr;
    }
  return str;
}

