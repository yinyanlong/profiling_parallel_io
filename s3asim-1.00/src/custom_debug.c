/* (C) Northwestern University
 * See COPYING in the top-level directory . */

#include "custom_debug.h"

extern int global_debug_mask;

/** This prototype is defined here to get rid of the ANSI warning */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/**
 * A custom function for debugging based on a debug mask compared
 * against a global debug mask.
 *
 * @param mask   A mask to compare against the global_debug_mask.
 * @param format Format string for output.
 * @param ...    Additional arguments for variables to be displayed.
 * @return       0 on success.
 */
int custom_debug(uint64_t mask,
		 const char *format,
		 ...)
{
    char buffer[DEBUG_BUF_SIZE], *bptr = buffer;
    int bsize = sizeof(buffer);
    struct timeval tv;
    time_t tp;
    va_list ap;
    int ret = -1;
    FILE *fp = stderr;
    
    if (!(mask & global_debug_mask))
	return 0;
    
    va_start(ap, format);

    gettimeofday(&tv, 0);
    tp = tv.tv_sec;
    strftime(bptr, 15, "[%m/%d %H:%M] ", localtime(&tp));
    bptr += 14;
    bsize -= 14;
    
    ret = vsnprintf(bptr, bsize, format, ap);
    if (ret < 0)
        return -errno;
    
    ret = fprintf(fp, buffer);
    if (ret < 0)
	return -errno;

    fflush(fp);

    return 0;
}
