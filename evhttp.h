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
struct config{
    const char *remote_addr;
    char        remote_add_resolved[32];
    int         remote_port;
    int         parallel;
    int         total;
    const char  *http_host;
    aeEventLoop *el;
    int         index;
    const char * flag;
    struct hostent *hptr;
    bool        debug;
    int active;
};


#endif


