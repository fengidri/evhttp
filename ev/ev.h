/**
 *   author       :   丁雪峰
 *   time         :   2016-01-11 13:10:42
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __EV_H__
#define __EV_H__

#include <stdarg.h>
#include <stdarg.h>
#include "evhttp.h"


void logdebug(const char *fmt, ...);
void print_http_info(struct http *h);
int update_time(struct http *h);

static inline void logerr(struct http *h, const char *fmt, ...)
{
    va_list arg_ptr;
    printf("%d: ", h->port);

    va_start(arg_ptr, fmt);
    vprintf(fmt, arg_ptr);
    va_end(arg_ptr);
}


#endif


