/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 21:59:01
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __EVHTTP_H__
#define __EVHTTP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include <stdbool.h>
#include "ae.h"
#include "lib.h"

#define RECYCLE_TIMES 1
#define RECYCLE_BYTES 2

struct remote{
    char domain[200];
    int  port;
    char ip[32];
};

// global config
struct config{
    aeEventLoop *el;
    struct remote remote;

    int         parallel;
    bool        debug;

    char      **urls;
    char       *_urls[100];
    size_t      urls_n;

    int         total;
    int         total_limit;
    int         active;

    const char *recycle;
    int         recycle_times;
    int         recycle_type;
    long long   recycle_limit;

    int         index;
    const char *flag;

    bool               sum;
    int                sum_timer_id;
    long long          sum_recv;
    long long          sum_recv_cur;
    long long          sum_status_200;
    long long          sum_status_other;
};

// http struct for every connect and request
struct http{
    struct remote _remote;
    struct remote *remote;

    int fd;
    bool eof;

    char url[1024];

    char buf[4 * 1024];
    int  buf_offset;

    bool read_header;
    int status;

    bool chunked;
    int chunk_length;
    int chunk_recv;

    int content_length;
    int content_recv;
};

int http_new();
extern struct config config;

#endif


