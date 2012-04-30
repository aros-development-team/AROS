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

#include <stdio.h>

int fprintf(FILE *stream, const char *format, ...) {
	return -1;
}

FILE *fopen(const char *filename, const char *mode) {
	return NULL;
}

int fclose(FILE *stream) {
	return EOF;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
	return 0;
}

int fseek(FILE *stream, long int offset, int origin) {
	return -1;
}

long int ftell(FILE *stream) {
	return -1;
}

int fputc(int character, FILE *stream) {
	return EOF;
}

int fileno(FILE *stream) {
	return -1;
}

int ferror(FILE *stream) {
	return 0;
}

int feof(FILE *stream) {
	return 0;
}

int ungetc(int c, FILE *stream) {
	return EOF;
}
