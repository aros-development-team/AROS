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

#ifndef _UTF_8_ISO_8859_1_CONVERSION_H
#define _UTF_8_ISO_8859_1_CONVERSION_H

/****************************************************************************/

/* Encode a string of characters in ISO 8859 Latin-1 encoding into
 * UTF-8 representation (rfc2279). Will encode as many characters as
 * will fit into the output buffer, and NUL-terminates the result.
 * Returns the number of UTF-8 characters in the output buffer.
 */
extern int encode_iso8859_1_as_utf8_string(const unsigned char * const from,int from_len,unsigned char * to,int to_size);

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
extern int decode_utf8_as_iso8859_1_string(const unsigned char * const from,int from_len,unsigned char * to,int to_size);

/****************************************************************************/

#endif /* _UTF_8_ISO_8859_1_CONVERSION_H */
