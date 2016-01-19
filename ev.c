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
    int lens[8];
    char speed[20] = {"-"};
    char *pos, *next;

    if (h->time_trans)
        size_fmt(speed, sizeof(speed),
                (double)h->content_recv/h->time_trans * 1000);

    snprintf(value, sizeof(value),
            "%-4d"     "|%-5d|%-5d|%-5d|%-5d|%-5d"    "|%-4d|%-5s|%-s",
            h->status,

            h->time_dns, h->time_connect, h->time_recv, h->time_max_read,
            h->time_trans,

            h->content_recv, speed, h->url);

    pos = value;
    for (size_t i=0; i < sizeof(lens)/sizeof(lens[0]); ++i)
    {
        next = strstr(pos, "|");
        lens[i] = next - pos;
        *next = ' ';
        pos = next + 1;
    }

    logdebug("%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s URL\n",
         *lens, "CODE", lens[1], "DNS",  lens[2], "CON",
         lens[3], "RECV",   lens[4], "READ", lens[5], "TRANS", lens[6], "BODY",
         lens[7], "Speed", "ULR");

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


