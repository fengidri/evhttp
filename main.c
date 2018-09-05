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

#include "format.h"
#include "sws.h"


static int url_from_file(const char *filename)
{
    struct sws_filebuf *buf;
    buf = sws_fileread(filename);
    if (!buf)
    {
        printf("read file fail!: %s: %s\n", filename, geterr());
        return  -1;
    }

    uint linenu;
    uint i;
    linenu = 0;
    for (i=0; i < buf->size; ++i)
    {
        if (buf->buf[i] == '\n') ++linenu;
    }

    config.urls = malloc(sizeof(char *) * linenu);
    config.urls[0] = buf->buf;
    config.urls_n += 1;
    for (i=0; i < buf->size; ++i)
    {
        if (buf->buf[i] == '\n')
        {
            buf->buf[i] = 0;
            config.urls[config.urls_n] = buf->buf + i + 1;
            config.urls_n += 1;
        }
    }
    --config.urls_n;

    config.total_limit = config.urls_n;
    printf("Total URLS Number: %d\n", config.urls_n);
}

int sum_handler(aeEventLoop *el, long long id, void * priv)
{
    char recv[20];
    char speed[20];
    char band[20];
    int n;
    static struct timeval start;
    struct timeval now;
    int t;

    if (!start.tv_sec)
    {
        gettimeofday(&start, NULL);
        t = 1000;
    }
    else{
        gettimeofday(&now, NULL);

        t = timeval_diff(start, now);

        start = now;
    }

    n = sws_size_fmt(recv, sizeof(recv) - 1, config.sum_recv);
    recv[n] = 0;

    n = sws_size_fmt(speed, sizeof(speed) - 1, config.sum_recv_cur * 1000 / t);
    speed[n] = 0;

    n = sws_size_fmt(band, sizeof(band) - 1, config.sum_recv_cur * 1000/ t *  8);
    band[n] = 0;

    config.sum_recv_cur = 0;
    printf("Total: %d 200: %lld Active: %d Fail: %d Recv: %s Speed: %s Band: %s\n",
            config.total, config.sum_status_200, config.active, config.fail_n,
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
            if (work_mode(WORK_MODE_URLS)) return SWS_ERR;

            if (config.urls_n >= sizeof(config._urls))
            {
                printf("Too many urls in command line.");
                return SWS_ERR;
            }
            config.urls[config.urls_n] = arg;
            config.urls_n += 1;

            config.total_limit = config.urls_n;
            continue;
        }

        ch = arg[1];
        switch(ch)
        {
            case 's': config.sum   = true; continue;
            case 'R':
                      if (work_mode(WORK_MODE_RANDOM))
                          return SWS_ERR;

                      if (i + 1 >= argc)
                      {
                          continue;
                      }
                      if (argv[i + 1][0] == '-')
                      {
                          continue;
                      }
                      config.rand_max = atoi(argv[i + 1]);
                      ++i;
                      continue;

            case '\0':
                      printf("arg: - : error\n");
                      return SWS_ERR;
        }

        if (0 != arg[2]) optarg = arg + 2;
        else{
            ++i;
            if (i >= argc){
                printf("arg: %s need opt\n", arg);
                return SWS_ERR;
            }
            optarg = argv[i];
        }

        switch (ch) {
            case 'd':
                sws_strncpy(config.remote.domain, optarg,
                        sizeof(config.remote.domain));
                break;
            case 'h':
                sws_strncpy(config.remote.ip, optarg, sizeof(config.remote.ip));
                break;
            case 'f': config.flag             = optarg;       break;
            case 'p': config.remote.port      = atoi(optarg); break;
            case 'x': config.parallel         = atoi(optarg); break;
            case 'n': config.total_limit      = atoi(optarg); break;
            case 'r': config.recycle          = optarg;       break;
            case 'm': config.method           = optarg;       break;
            case 'F':
                      if (work_mode(WORK_MODE_FILE)) return SWS_ERR;
                      url_from_file(optarg);
                      break;
            case 'w':
                      if (0 != format_compile(&config, optarg, 0))
                          return SWS_ERR;
                          break;
            case 'W':
                      if (0 != format_compile(&config, optarg, 1))
                          return SWS_ERR;
                          break;
            case 'H':
                      {
                          size_t l;
                          l =  strlen(optarg);
                          if (l + 2 > sizeof(config.headers) - config.headers_n)
                          {
                              printf("Too many headers in command line.");
                              return SWS_ERR;
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
    return SWS_OK;
}

int config_init(int argc, char **argv)
{
    if (!config.work_mode)
    {
        printf("Please set the work mode!\n");
        return SWS_ERR;
    }

    // total
    if (0 == config.total_limit) config.total_limit = -1;

    if (WORK_MODE_RANDOM == config.work_mode) // use random url
    {
        if (!config.remote.domain[0] && !config.remote.ip[0])
        {
            printf("Please set domain(-d) or ip(-i) for random url!!\n");
            return SWS_ERR;
        }
        if (!config.remote.ip[0])
        {
            if (!sws_net_resolve(config.remote.domain, config.remote.ip,
                        sizeof(config.remote.ip)))
            {
                printf("Cannot resolve the add: %s\n", config.remote.domain);
                return SWS_ERR;
            }
        }
        if (!config.remote.domain)
        {
            sws_strncpy(config.remote.domain, config.remote.ip,
                    sizeof(config.remote.ip));
        }
    }

    if (1 != config.total_limit)
    {
        //config.print = PRINT_TIME;
    }

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
                      printf("%s connot unrecognized", config.recycle);
                      return SWS_ERR;
        }
    }

    config.el = aeCreateEventLoop(config.parallel + 129);

    if (config.sum)
    {
        config.print = 0;
        aeCreateTimeEvent(config.el, 1000, sum_handler, NULL, NULL);
    }

    if (WORK_MODE_RANDOM == config.work_mode)
    {
        printf("Use Random URL. Domain: %s/%s:%d Parallel: %d\n",
            config.remote.domain,
            config.remote.ip, config.remote.port, config.parallel);
    }

    return SWS_OK;
}


int main(int argc, char **argv)
{
    struct timeval start, now;
    int t;

    if (SWS_OK != arg_parser(argc, argv)) return SWS_ERR;
    if (SWS_OK != config_init(argc, argv)) return SWS_ERR;

    srand(time(NULL));

    int p = config.parallel;
    while (p)
    {
        http_new();
        p--;
    }


    gettimeofday(&start, NULL);
    aeMain(config.el);

    if (config.total > 2)
    {
        gettimeofday(&now, NULL);
        t = timeval_diff(start, now);
        printf("Time Total: %d.%03ds\n", t/1000, t%1000);
    }

    format_destroy(&config);
}
