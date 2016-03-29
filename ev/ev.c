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
                (double)h->content_recv/h->time_total * 1000);
    if (config.print & PRINT_TIME_H)
        logdebug("DNS CON RES MAXREAD TRANS TOTAL\n");

    if (config.print & PRINT_TIME)
    {
        logdebug("%llu %d %d %4d %5s/s ",
                h->index, h->header_res.status, h->port,
                h->content_recv, speed);
        logdebug("%d.%.3d %d.%.3d %d.%.3d %d.%.3d | %d.%.3d | %d.%.3d %d %d %s\n",
            h->time_dns/1000,      h->time_dns      % 1000,
            h->time_connect/1000,  h->time_connect  % 1000,
            h->time_response/1000, h->time_response % 1000,
            h->time_trans/1000,    h->time_trans    % 1000,
            h->time_total/1000,    h->time_total    % 1000,
            h->time_max_read/1000, h->time_max_read % 1000,
            h->time_max_read_n,
            h->body_read_times,
            h->url
            );
    }



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
//        logdebug("%-*s %-*s %-*s %-*s %-*s %-*s %-*s %-*s URL\n",
//                lens[0], "INDEX",
//                lens[1], "CODE", lens[2], "PORT", lens[3], "DNS",  lens[4], "CON",
//                lens[5], "RECV",   lens[6], "READ", lens[7], "TRANS", lens[8], "BODY",
//                lens[9], "Speed", "ULR");


}

void update_time(struct http *h, enum http_state state)
{
    struct timeval now;
    int t;

    gettimeofday(&now, NULL);

    switch (state)
    {
        case HTTP_NEW: // just init time_last
            h->time_last = now;
            break;

        case HTTP_DNS_POST:
            h->time_dns = timeval_diff(h->time_last, now);
            h->time_last = now;
            break;

        case HTTP_SEND_REQUEST:
            h->time_connect = timeval_diff(h->time_last, now);
            h->time_send_request = now;
            h->time_last = now;
            break;

        case HTTP_RECV_HEADER:
            h->time_response = timeval_diff(h->time_last, now);
            h->time_last = now;
            if (0 == h->time_start_read.tv_sec && \
                    0 == h->time_start_read.tv_usec)
                h->time_start_read = now;
            break;

        case HTTP_RECV_BODY:
            h->body_read_times += 1;
            t = timeval_diff(h->time_last, now);
            if (t > h->time_max_read) {
                h->time_max_read = t;
                h->time_max_read_n = h->body_read_times;
            }
            h->time_last = now;
            break;

        case HTTP_END:
            h->time_trans = timeval_diff(h->time_start_read, now);
            h->time_total = timeval_diff(h->time_send_request, now);
            break;

        default:
            ;;
    }
}


