#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "trim.h"

/**
 * trim(<string>, <string length>)
 *
 * Utility function that trims the whitespace from the end of a string in-place.  Returns the (new)
 * length of the trimmed string.
 **/
size_t trim(unsigned char *buf, size_t n)
{
	size_t i;

#ifdef __DEBUG__
	char *original_buf = strndup((char *)buf, n);
#endif
	for (i = strnlen((char *)buf, n) - 1; i >= 0; i--)
	{
		if (! isspace(buf[i]))
		{
			break;
		}
	}
	i++;
	buf[i] = '\0';
#ifdef __DEBUG__
	if (original_buf)
	{
		printf("trim.c debug: '%s'(%lu) => '%s'(%lu)\n", original_buf, strnlen(original_buf, n), buf, i);
		free(original_buf);
	}
	else
	{
		printf("trim.c debug: NULL/OOM => '%s'(%lu)\n", buf, i);
	}
#endif
	return i;
}
