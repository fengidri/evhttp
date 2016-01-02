/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 22:49:35
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "evhttp.h"
#include <netdb.h>
#include <arpa/inet.h>

#include "lib.h"

extern struct config config;

int sum_handler(aeEventLoop *el, long long id, void * priv)
{
    char recv[20];
    char speed[20];
    char band[20];
    int n;

    n = size_fmt(recv, sizeof(recv) - 1, config.sum_recv);
    recv[n] = 0;

    n = size_fmt(speed, sizeof(speed) - 1, config.sum_recv_cur);
    speed[n] = 0;

    n = size_fmt(band, sizeof(band) - 1, config.sum_recv_cur * 8);
    band[n] = 0;

    config.sum_recv_cur = 0;
    printf("Total: %d OK: %lld Active: %d Recv: %s Speed: %s Band: %s\n",
            config.total, config.sum_status_200, config.active,
            recv, speed, band);

    return 1000;
}

int config_init(int argc, char **argv)
{
    config.remote_addr = "127.0.0.1";
    config.remote_port = 80;
    config.parallel    = 1;
    config.total       = 1;
    config.http_host   = NULL;
    config.index       = 0;
    config.flag        = "M";
    config.debug       = false;
    config.active      = 0;
    config.total_limit = 1;
    config.total       = 0;
    config.sum         = false;
    config.recycle     = NULL;
    config.recycle_times  = 1;

    char ch;
    while ((ch = getopt(argc, argv, "H:h:p:l:f:t:vsr:")) != -1) {
        switch (ch) {
            case 'h': config.remote_addr = optarg;       break;
            case 'p': config.remote_port = atoi(optarg); break;
            case 'l': config.parallel    = atoi(optarg); break;
            case 'H': config.http_host   = optarg;       break;
            case 'f': config.flag        = optarg;       break;
            case 't': config.total_limit = atoi(optarg); break;
            case 'v': config.debug       = true;         break;
            case 's': config.sum       = true;         break;
            case 'r': config.recycle       = optarg;         break;
        }
    }

    config.hptr = gethostbyname(config.remote_addr);
    if (NULL == config.hptr)
    {
        logerr("gethostbyname error: %s\n", config.remote_addr);
    }
    inet_ntop(config.hptr->h_addrtype, *config.hptr->h_addr_list,
        config.remote_add_resolved, sizeof(config.remote_add_resolved));

    if (NULL == config.http_host) config.http_host = config.remote_addr;

    if (0 == config.total_limit){
        config.total_limit = -1;
    }

    char flag = config.recycle[strlen(config.recycle) - 1];
    flag = tolower(flag);
    config.recycle_limit = atoi(config.recycle);
    switch(flag)
    {
        case 'n':
            config.recycle_type = RECYCLE_TIMES;
            break;

        case 't': config.recycle_limit *= 1024;
        case 'g': config.recycle_limit *= 1024;
        case 'm': config.recycle_limit *= 1024;
        case 'k': config.recycle_limit *= 1024;
        case 'b':
            config.recycle_type = RECYCLE_BYTES;
        default:
            logerr("%s connot unrecognized");
            return EV_ERR;
    }

    if (config.recycle >= 0)
    {
        logerr("when total nolimit, can not set recycle!");
        return EV_ERR;
    }

    config.el = aeCreateEventLoop(config.parallel + 129);

    if (config.sum)
    {
        aeCreateTimeEvent(config.el, 1000, sum_handler, NULL, NULL);
    }
    return EV_OK;
}


int main(int argc, char **argv)
{
    if (EV_OK != config_init(argc, argv)) return -1;
    printf("Start: Host: %s:%d Parallel: %d\n",
            config.remote_add_resolved, config.remote_port, config.parallel);


    int p = config.parallel;
    while (p)
    {
        http_new();
        p--;
    }

    aeMain(config.el);
}
