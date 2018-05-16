/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 21:58:21
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "evhttp.h"

static char *letters = "/1234567890asbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY";

struct url{
    bool https;
    const char *domain;
    const char *url;
    size_t domain_n;
    size_t url_n;
    int port;
};

bool url_parser(struct url *u, const char *url)
{
    char *p;
    u->https = false;
    if (':' == url[4])
    {
        u->domain = url + 7;
    }
    else{
        if (':' == url[5])
        {
            u->domain = url + 8;
            u->https = true;
        }
        else
            u->domain = url;
    }

    u->url = strstr(u->domain, "/");
    if (NULL == u->url)
    {
        u->domain_n = strlen(u->domain);
    }
    else{
        u->domain_n = u->url - u->domain;
    }

    u->port = 0;
    p = strstr(u->domain, ":");
    if (p)
    {
        u->port = atoi(p + 1);
        if (0 == u->port){
            seterr("port error: %s", url);
            return false;
        }
        u->domain_n = p - u->domain;
    }
    return true;
}

static bool check_limit()
{
    if (config.total_limit > 0)
    {
        if (config.total >  config.total_limit)
        {
            return true;
        }
    }

    if (config.recycle)
    {
        long long target;
        target = 0;
        if (RECYCLE_TIMES == config.recycle_type)
            target = config.total;

        if (RECYCLE_BYTES == config.recycle_type)
            target = config.sum_recv;

        if (target > config.recycle_limit * config.recycle_times)
        {
            config.index         =  0;
            config.recycle_times += 1;
        }
    }
    return false;
}

void random_url(struct http *h)
{
    size_t l, c, len;
    int r;
    int index = config.index;
    config.index += 1;

    h->url[0] = '/';
    h->url[1] = 0;
    strcat(h->url, config.flag);
    l = strlen(h->url);
    h->url[l] = '/';
    ++l;

    l += sprintf(&h->url[l], "%x", rand() % config.rand_max);

    //len = strlen(letters);
    //while (index)
    //{
    //    c = index % len;
    //    h->url[l] = letters[c];
    //    ++l;
    //    index = index / len;
    //}

    h->url[l] = 0;
    h->remote = &config.remote;
}

bool select_url(struct http *h)
{
    struct url u;
    const char *url;

    if (config.index >= config.urls_n) config.index = 0;

    url = config.urls[config.index];
    config.index++;

    if (!url_parser(&u, url)){
        seterr("url parser error: %s", url);
        return false;
    }

    if (u.url)
    {
        strncpy(h->url, u.url, sizeof(h->url));
        h->url[sizeof(h->url) - 1] = 0;
    }
    else{
        h->url[0] = '/';
        h->url[1] = '\0';
    }

    if (u.domain_n >= sizeof(h->remote->domain))
    {
        seterr("%s: domain to long", url);
        return false;
    }

    memcpy(h->remote->domain, u.domain, u.domain_n);
    h->remote->domain[u.domain_n] = 0;

    if (u.port)
        h->remote->port = u.port;
    else
        h->remote->port = config.remote.port;

    if (config.remote.ip[0])
        strcpy(h->remote->ip, config.remote.ip);
    else
        h->remote->ip[0] = 0;
    return true;
}

bool get_url(struct http *h)
{
    if (check_limit()) return false;

    if (WORK_MODE_RANDOM == config.work_mode)
    {
        random_url(h);
        return true;
    }
    else
        return select_url(h);

}
