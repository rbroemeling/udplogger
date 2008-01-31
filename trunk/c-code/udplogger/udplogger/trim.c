#include <ctype.h>
#include <string.h>
#include "trim.h"

size_t trim(unsigned char *buf)
{
	size_t i;

#ifdef __DEBUG__
	fprintf(stderr, "trim('%s') = ", buf);
#endif
	for (i = strlen((char *)buf) - 1; i >= 0; i--)
	{
		if (! isspace(buf[i]))
		{
			break;
		}
	}
	i++;
	buf[i] = '\0';
#ifdef __DEBUG__
	fprintf(stderr, "'%s'\n", buf);
#endif
	return i;
}
