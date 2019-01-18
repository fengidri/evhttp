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
#include "parser.h"
#include "format.h"


void logdebug(const char *fmt, ...)
{
    va_list argList;
    if (config.loglevel < LOG_DEBUG) return;

    va_start(argList, fmt);
    vprintf(fmt, argList);
    va_end(argList);
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
            config.sum_time_total += h->time_total;
            config.sum_time_total_n += 1;
            break;

        default:
            ;;
    }
}


