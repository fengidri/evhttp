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
#include <arpa/inet.h>

#include "common.h"

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

int arg_parser(int argc, char **argv)
{
    int i = 0;
    char *arg, ch, *optarg;

    for (i=1; i < argc; ++i)
    {
        arg = argv[i];
        if ('-' != arg[0])
        {
            if (config.urls_n >= sizeof(config._urls))
            {
                perr("Too many urls in command line.");
                return EV_ERR;
            }
            config.urls[config.urls_n] = arg;
            config.urls_n += 1;
            continue;
        }

        ch = arg[1];
        switch(ch)
        {
            case 's': config.sum   = true; continue;
            case '\0':
                      perr("arg: - : error\n");
                      return EV_ERR;
        }

        if (0 != arg[2]) optarg = arg + 2;
        else{
            ++i;
            if (i >= argc){
                perr("arg: %s need opt\n", arg);
                return EV_ERR;
            }
            optarg = argv[i];
        }

        switch (ch) {
            case 'd':
                ev_strncpy(config.remote.domain, optarg,
                        sizeof(config.remote.domain));
                break;
            case 'h':
                ev_strncpy(config.remote.ip, optarg, sizeof(config.remote.ip));
                break;
            case 'f': config.flag             = optarg;       break;
            case 'p': config.remote.port      = atoi(optarg); break;
            case 'x': config.parallel         = atoi(optarg); break;
            case 'n': config.total_limit      = atoi(optarg); break;
            case 'r': config.recycle          = optarg;       break;
            case 'm': config.method           = optarg;       break;
            case 'H':
                      {
                          size_t l;
                          l =  strlen(optarg);
                          if (l + 2 > sizeof(config.headers) - config.headers_n)
                          {
                              perr("Too many headers in command line.");
                              return EV_ERR;
                          }

                          strcpy(config.headers + config.headers_n, optarg);
                          config.headers_n += l;

                          config.headers[config.headers_n] = '\r';
                          config.headers_n += 1;
                          config.headers[config.headers_n] = '\n';
                          config.headers_n += 1;
                          break;
                      }
        }
    }
    return EV_OK;
}

int config_init(int argc, char **argv)
{
    if(EV_OK != arg_parser(argc, argv)) return EV_ERR;

    if (!config.urls_n) // use random url
    {
        if (!config.remote.domain[0] && !config.remote.ip[0])
        {
            perr("Please set domain(-d) or ip(-i) for random url!!\n");
            return EV_ERR;
        }
        if (!net_resolve(config.remote.domain, config.remote.ip,
                    sizeof(config.remote.ip)))
        {
            perr("Cannot resolve the add: %s\n", config.remote.domain);
            return EV_ERR;
        }
    }

    if (0 == config.remote.ip[0] && 0 == config.urls_n)
    {
        if (!net_resolve(config.remote.domain, config.remote.ip,
                    sizeof(config.remote.ip)))
        {
            perr("Cannot resolve the add: %s\n", config.remote.domain);
            return EV_ERR;
        }
    }

    // total
    if (0 == config.total_limit) config.total_limit = -1;

    // recycle
    if (config.recycle)
    {
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
                      break;
            default:
                      perr("%s connot unrecognized", config.recycle);
                      return EV_ERR;
        }
    }

    config.el = aeCreateEventLoop(config.parallel + 129);

    if (config.sum)
    {
        config.loglevel = LOG_ERROR;
        aeCreateTimeEvent(config.el, 1000, sum_handler, NULL, NULL);
    }
    return EV_OK;
}


int main(int argc, char **argv)
{
    if (EV_OK != config_init(argc, argv)) return -1;

    if (!config.urls_n)
    {
        printf("Use Random URL. Domain: %s/%s:%d Parallel: %d\n",
            config.remote.domain,
            config.remote.ip, config.remote.port, config.parallel);
    }

    int p = config.parallel;
    while (p)
    {
        http_new();
        p--;
    }

    aeMain(config.el);
}
