/*
 * support.c
 *
 *  Created on: Aug 13, 2008
 *      Author: misc
 */

#include <of1275.h>
#include <support.h>
#include <stdarg.h>

char *remove_path(const char *in)
{
    char *p = (char *)&in[strlen(in)-1];

    while (p > in && p[-1] != '/' && p[-1] != ':') p--;
    return p;
}

int32_t strlen(const char *c)
{
	int32_t result = 0;
	while (*c++)
		result++;

	return result;
}

int isblank(char c)
{
    return (c == ' ' || c == '\t');
}

int isspace(char c)
{
    return (c == ' ' || c == '\t' || c == '\f' || c == '\n' || c == '\r' || c == '\v');
}

int isdigit(char c)
{
    return (c >= '0' && c <= '9');
}

int isxdigit(char c)
{
    return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

int tolower(char c)
{
    if (c >= 'A' && c <= 'Z')
        c -= ('A'-'a');

    return c;
}

int strncasecmp(const char *s1, const char *s2, int max)
{
    int diff = 0;
    for (; max && !(diff = tolower(*s2)-tolower(*s1)) && *s1; max--, s1++, s2++);
    return diff;
}

int strcasecmp(const char *s1, const char *s2)
{
    int diff = 0;
    for (; !(diff = tolower(*s2)-tolower(*s1)) && *s1; s1++, s2++);
    return diff;
}

int strncmp(const char *s1, const char *s2, int max)
{
    int diff = 0;
    for (; max && !(diff = *s2-*s1) && *s1; max--, s1++, s2++);
    return diff;
}

void bzero(void *dest, int length)
{
    char *d = dest;

    while (length--)
        *d++ = 0;
}

void memcpy(void *dest, const void *src, int length)
{
    const char *s = src;
    char *d = dest;

    while (length--)
        *d++ = *s++;
}


void __itoa(char *buf, intptr_t val, char code)
{
	static const char convtbl[] = "0123456789ABCDEF";
	int i;
	if (code == 'p')
	{
		for (i=0; i < 8; i++, val <<= 4)
			*buf++ = convtbl[(val >> 28) & 0xf];

		*buf = 0;

		return;
	}
	else
	{
		uint32_t ud = val;
		int divisor = 10;
		char *p, *p1, *p2;

		if (code == 'd' && val < 0)
		{
			val = -val;
			*buf++ = '-';
		}
		if (code == 'x')
			divisor = 16;

		p = buf;

		do {
			*p++ = convtbl[ud % divisor];
		} while(ud /= divisor);

		*p = 0;

		p1 = buf;
		p2 = p - 1;

		while(p1 < p2)
		{
			char temp = *p1;
			*p1 = *p2;
			*p2 = temp;

			p1++;
			p2--;
		}
	}
}

void sprintf(char *dest, char *str, ...)
{
	va_list a;
	char c;
	char buf[20];

	va_start(a, str);

	while ((c = *str++) != 0)
	{
		if (c != '%')
			*dest++ = c;

		else
		{
			char *p, tmp;

			c = *str++;

			switch (c)
			{
			case 'c':
				tmp = va_arg(a, int);
				*dest++ = tmp;
				break;

			case 'p':
			case 'x':
			case 'd':
			case 'u':
				p = buf;
				intptr_t val = va_arg(a, intptr_t);
				__itoa(p, val, c);
				goto string;

			case 's':
				p = va_arg(a, char *);
				if (!p)
					p = "(null)";
			string:
				while (*p)
					*dest++ = *p++;
				break;

			default:
				*dest++ = c;
				break;
			}
		}
	}

	*dest = 0;

	va_end(a);
}

void printf(char *str, ...)
{
	va_list a;
	char c;
	char buf[20];

	va_start(a, str);

	while ((c = *str++) != 0)
	{
		if (c != '%')
			ofw_write(stdout, &c, 1);

		else
		{
			char *p, tmp;

			c = *str++;

			switch (c)
			{
			case 'c':
				tmp = va_arg(a, int);
				ofw_write(stdout, &tmp, 1);
				break;

			case 'p':
			case 'x':
			case 'd':
			case 'u':
				p = buf;
				intptr_t val = va_arg(a, intptr_t);
				__itoa(p, val, c);
				goto string;

			case 's':
				p = va_arg(a, char *);
				if (!p)
					p = "(null)";
			string:
				ofw_write(stdout, p, strlen(p));
				break;

			default:
				ofw_write(stdout, &c, 1);
				break;
			}
		}
	}

	va_end(a);
}

int atoi(const char *str)
{
    int val = 0;

    if ((str[0] == '0') && (tolower(str[1]) == 'x'))
    {
	str += 2;

	while (*str && isxdigit(*str))
	{
	    char c = tolower(*str++);

	    val <<= 4;
	    if (c > '9')
		val += c - 'a' + 10;
	    else
		val += c - '0';
	}
    }    

    while(*str && isdigit(*str))
    {
        val *= 10;
        val += *str - '0';
        str++;
    }

    return val;
}
