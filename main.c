/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 22:49:35
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "evhttp.h"
#include <netdb.h>
#include <arpa/inet.h>

extern struct config config;

void config_init(int argc, char **argv)
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

    char ch;
    while ((ch = getopt(argc, argv, "H:h:p:l:f:t:v")) != -1) {
        switch (ch) {
            case 'h': config.remote_addr = optarg;       break;
            case 'p': config.remote_port = atoi(optarg); break;
            case 'l': config.parallel    = atoi(optarg); break;
            case 'H': config.http_host   = optarg;       break;
            case 'f': config.flag        = optarg;       break;
            case 't': config.total_limit = atoi(optarg); break;
            case 'v': config.debug       = true;         break;
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

    if (0 == config.total_limit) config.total_limit = -1;


    config.el = aeCreateEventLoop(config.parallel + 129);
}

int main(int argc, char **argv)
{
    config_init(argc, argv);
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
