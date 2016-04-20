/*
 * :ts=4
 *
 * SMB file system wrapper for AmigaOS, using the AmiTCP V3 API
 *
 * Copyright (C) 2000-2016 by Olaf `Olsen' Barthel <obarthel -at- gmx -dot- net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stddef.h>

#include "utf-8-iso-8859-1-conversion.h"

/* Encode an ISO 8859 Latin 1 character (default character set for
 * the Amiga) in UTF-8 representation (rfc2279). Returns the number
 * of characters written to the buffer, or -2 for 'buffer overflow'
 * in which case no data is written.
 *
 * If the address of the buffer to write to is NULL, then no data
 * will be written; only the number of bytes that would have been
 * written if the buffer address were not NULL will be returned.
 */
static int
encode_iso8859_1_as_utf8_char(unsigned char c,unsigned char * string,int size)
{
	int len;

	if((c & 0x80) == 0)
	{
		/* ASCII characters can be encoded as a single octet. */
		if(string == NULL || size >= 1)
		{
			len = 1;

			if(string != NULL)
				string[0] = c;
		}
		else
		{
			/* Not enough room... */
			len = -2;
		}
	}
	else
	{
		/* ISO 8859 Latin 1 characters must be encoded as two octets. */
		if(string == NULL || size >= 2)
		{
			len = 2;

			if(string != NULL)
			{
				string[0] = 0xc0 | ((c >> 6) & 0x03);
				string[1] = 0x80 | (c & 0x3f);
			}
		}
		else
		{
			/* Not enough room... */
			len = -2;
		}
	}

	return(len);
}

/****************************************************************************/

/* Data used by the decoder. */
struct utf8_decoding_entry
{
	unsigned char	mask;		/* Mask and pattern are used to identify */
	unsigned char	pattern;	/* the type of multi-octet sequence */
	int				len;		/* Number of octets in the sequence */
	long			first;		/* First and last are for checking the */
	long			last;		/* resulting character against its code range */
};

/****************************************************************************/

/* Decode a character in UTF-8 representation (rfc2279) and return
 * how many bytes contributed to that character (1-6). Returns
 * -1 if the character could not be decoded or -2 if more bytes
 * would be required for decoding than the input buffer holds.
 * Returns -3 if the character was not encoded as the shortest
 * possible UTF-8 sequence.
 *
 * If an error is indicated, no data will be written.
 *
 * If the address of the output buffer to write to is NULL, then no
 * data will be written; only the number of bytes that would have
 * been decoded if the buffer address were not NULL will be returned.
 */
static int
decode_utf8_char(const unsigned char * const string,int size,unsigned long * result_ptr)
{
	int len;

	if(size > 0)
	{
		int c,i;

		/* Assume a seven bit ASCII character. */
		c = string[0];

		/* Could this be an UTF-8 encoded character? */
		if((c & 0x80) != 0)
		{
			static const struct utf8_decoding_entry utf8_decoding_table[5] =
			{
				{ 0xfe,0xfc,6,0x04000000,0x7FFFFFFF }, /* 1111110x (UCS-4 range 04000000-7FFFFFFF) */
				{ 0xfc,0xf8,5,0x00200000,0x03FFFFFF }, /* 111110xx (UCS-4 range 00200000-03FFFFFF) */
				{ 0xf8,0xf0,4,0x00010000,0x001FFFFF }, /* 11110xxx (UCS-4 range 00010000-001FFFFF) */
				{ 0xf0,0xe0,3,0x00000800,0x0000FFFF }, /* 1110xxxx (UCS-4 range 00000800-0000FFFF) */
				{ 0xe0,0xc0,2,0x00000080,0x000007FF }  /* 110xxxxx (UCS-4 range 00000080-000007FF) */
			};

			/* Find the bit pattern that corresponds to the
			 * code; if none matches, then we have an
			 * invalid code.
			 */
			len = -1;

			for(i = 0 ; i < 5 ; i++)
			{
				if((c & utf8_decoding_table[i].mask) == utf8_decoding_table[i].pattern)
				{
					/* Strip the encoding pattern and retain
					 * the 'payload'.
					 */
					c &= ~utf8_decoding_table[i].mask;

					/* If the character would consist of more octects
					 * than the input buffer holds, we flag an underflow
					 * error.
					 */
					len = utf8_decoding_table[i].len;
					if(len <= size)
					{
						int j,d;

						/* The next few octets contain six bits of
						 * character data each.
						 */
						for(j = 1 ; j < len ; j++)
						{
							d = string[j];

							/* Each octet must be in the form
							 * of 10xxxxxx.
							 */
							if((d & 0xc0) == 0x80)
							{
								c = (c << 6) | (d & 0x3f);
							}
							else
							{
								/* Bad code... */
								len = -1;
								break;
							}
						}

						if(len > 0)
						{
							/* Verify that the character was encoded
							 * in the shortest form possible.
							 */
							if(c < utf8_decoding_table[i].first ||
							   c > utf8_decoding_table[i].last)
							{
								len = -3;
							}
						}
					}
					else
					{
						len = -2;
					}

					break;
				}
			}
		}
		else
		{
			len = 1;
		}

		if(len > 0 && result_ptr != NULL)
			(*result_ptr) = c;
	}
	else
	{
		len = 0;
	}

	return(len);
}

/****************************************************************************/

/* Encode a string of characters in ISO 8859 Latin-1 encoding into
 * UTF-8 representation (rfc2279). Will encode as many characters as
 * will fit into the output buffer, and NUL-terminates the result.
 * Returns the number of UTF-8 characters in the output buffer.
 */
int
encode_iso8859_1_as_utf8_string(const unsigned char * const from,int from_len,unsigned char * to,int to_size)
{
	int i,char_len,total_len;
	int result;

	total_len = 0;

	for(i = 0 ; i < from_len ; i++)
	{
		result = encode_iso8859_1_as_utf8_char(from[i],to,to_size-1);
		if(result < 0)
		{
			/* Stop on buffer overflow or error. */
			goto out;
		}

		char_len = result;

		if(to != NULL)
		{
			to += char_len;

			to_size -= char_len;
		}

		total_len += char_len;
	}

	/* Provide for NUL termination. */
	if(to != NULL && to_size > 0)
		(*to) = '\0';

	result = total_len;

 out:

	return(result);
}

/****************************************************************************/

/* Decode a string of characters encoded in UTF-8 representation (rfc2279).
 * Will decode and retain only characters that can be decoded properly
 * and which fit into the ASCII/BMP Latin-1 supplementary range. Will
 * decode as many characters as will fit into the output buffer, and
 * NUL-terminates the result. Returns the number of characters in the
 * output buffer, or -1 for decoding error.
 *
 * Note that decoding will stop once a NUL has been found in the
 * input string to be decoded.
 */
int
decode_utf8_as_iso8859_1_string(const unsigned char * const from,int from_len,unsigned char * to,int to_size)
{
	unsigned long c;
	int i,char_len,total_len;
	int result = -1;

	total_len = 0;

	i = 0;

	/* Process the entire input buffer unless we hit
	 * a NUL first.
	 */
	while(from_len > 0)
	{
		char_len = decode_utf8_char(&from[i],from_len,&c);
		if(char_len > 0)
		{
			from_len -= char_len;
			i += char_len;

			/* Allow only for ASCII/BMP Latin-1 supplementary
			 * characters.
			 */
			if(c >= 256)
				goto out;

			/* Is there still enough room for the character
			 * and a terminating NUL byte?
			 */
			if(to == NULL || to_size-1 > 0)
			{
				/* Add this only if it's not the terminating
				 * NUL byte.
				 */
				if(c != '\0')
				{
					if(to != NULL)
					{
						(*to++) = c;

						to_size--;
					}

					total_len++;
				}
				else
				{
					/* Found a terminating NUL byte. */
					break;
				}
			}
			else
			{
				/* No more room in the buffer. */
				if(to != NULL)
					break;
			}
		}
		else
		{
			/* Underflow or invalid code. */
			goto out;
		}
	}

	/* Provide for NUL-termination. */
	if(to != NULL && to_size > 0)
		(*to) = '\0';

	result = total_len;

 out:

	return(result);
}
