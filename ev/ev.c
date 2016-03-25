/**
 *   author       :   丁雪峰
 *   time         :   2016-01-11 13:13:36
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "ev.h"

void logdebug(const char *fmt, ...)
{
    va_list argList;
    if (config.loglevel < LOG_DEBUG) return;

    va_start(argList, fmt);
    vprintf(fmt, argList);
    va_end(argList);
}

void print_http_info(struct http *h)
{
    char value[1204];
    int lens[20];
    char speed[20] = {"-"};
    char *pos, *next;

    if (h->time_trans)
        size_fmt(speed, sizeof(speed),
                (double)h->content_recv/h->time_trans * 1000);

    snprintf(value, sizeof(value),
            "%llu: %-4d %-d D:%d.%d C:%d.%d F:%d.%d W:%d.%d R:%d.%d T:%-4d %-5s/s %-s",
            h->index, h->status, h->port,
            h->time_dns/1000,      h->time_dns      % 1000,
            h->time_connect/1000,  h->time_connect  % 1000,
            h->time_recv/1000,     h->time_recv     % 1000,
            h->time_max_read/1000, h->time_max_read % 1000,
            h->time_trans/1000,    h->time_trans    % 1000,

            h->content_recv, speed, h->url);

//#pos = value;
//    for (size_t i=0; i < sizeof(lens)/sizeof(lens[0]); ++i)
//    {
//        next = strstr(pos, "|");
//        if (next)
//        {
//            lens[i] = next - pos;
//            *next = ' ';
//            pos = next + 1;
//        }
//        else break;
//    }
//
//
//    if (config.print & PRINT_TIME_H)
//        logdebug("%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s URL\n",
//                lens[0], "INDEX",
//                lens[1], "CODE", lens[2], "PORT", lens[3], "DNS",  lens[4], "CON",
//                lens[5], "RECV",   lens[6], "READ", lens[7], "TRANS", lens[8], "BODY",
//                lens[9], "Speed", "ULR");

    if (config.print & PRINT_TIME)
        logdebug("%s\n", value);

}

int update_time(struct http *h)
{
    struct timeval now;
    int t;

    gettimeofday(&now, NULL);

    t = timeval_diff(h->time_last, now);
    h->time_last = now;

    return t;
}


