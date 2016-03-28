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
#include <sys/time.h>
#include "ae.h"
#include "common.h"

#define RECYCLE_TIMES 1
#define RECYCLE_BYTES 2

struct remote{
    char domain[200];
    int  port;
    char ip[32];
};

enum loglevel{
    LOG_ERROR,
    LOG_DEBUG,
    LOG_INFO,
    LOG_SUM,
};

enum http_state{
    HTTP_NEW,
    HTTP_DNS,
    HTTP_DNS_POST,
    HTTP_CONNECT,
    HTTP_CONNECT_POST,
    HTTP_SEND_REQUEST,
    HTTP_RECV_HEADER,
    HTTP_RECV_BODY,
    HTTP_END,
};

#define PRINT_SUM 0
#define PRINT_REQUEST 1
#define PRINT_RESPONSE 1 << 1
#define PRINT_TIME_H   1 << 2
#define PRINT_TIME     1 << 3
#define PRINT_DNS      1 << 4
#define PRINT_CON      1 << 5

struct config{
    aeEventLoop *el;
    struct remote remote;


    int         parallel;
    enum loglevel  loglevel;

    char   headers[2048];
    size_t headers_n;

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

    size_t      index;
    const char *flag;

    bool               sum;
    int                sum_timer_id;
    long long          sum_recv;
    long long          sum_recv_cur;
    long long          sum_status_200;
    long long          sum_status_other;

    const char * method;

    const char dns[20];
    int print;
};


// http struct for every connect and request
struct http{
   unsigned long long index;
    struct remote _remote;
    struct remote *remote;

    enum http_state next_state;
    int fd;
    int port;
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

    struct timeval time_last;
    struct timeval time_start_read;
    struct timeval time_send_request;

    int time_dns;
    int time_connect;
    int time_response;
    int time_trans;
    int time_max_read;
    int time_total; // from send request to end
    //float time_max_read;
};

void http_new();
extern struct config config;

#include "ev.h"

#endif


