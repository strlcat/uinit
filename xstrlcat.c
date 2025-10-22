#include <string.h>

size_t xstrlcat(char *dst, const char *src, size_t size)
{
	size_t dstlen = strnlen(dst, size);
	if (dstlen == size) return dstlen + strlen(src);
	return dstlen + xstrlcpy(dst+dstlen, src, size-dstlen);
}
