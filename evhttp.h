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

struct config{
    aeEventLoop *el;

    const char     *remote_addr;
    char            remote_add_resolved[32];
    int             remote_port;
    struct hostent *hptr;

    int         parallel;
    bool        debug;

    int         total;
    int         total_limit;
    int         active;
    int        recycle;

    const char  *http_host;

    int         index;
    const char *flag;


    bool               sum;
    int                sum_timer_id;
    long long          sum_recv;
    long long          sum_recv_cur;
    long long          sum_status_200;
    long long          sum_status_other;
};

int http_new();

#endif


