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
extern struct config config;

bool check_limit()
{
    if (config.total_limit > 0)
    {
        if (config.total >=  config.total_limit)
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

bool make_url(char *url, size_t size)
{
    if (check_limit()) return false;

    size_t l, c, len;
    int index = config.index;
    config.index += 1;

    url[0] = '/';
    url[1] = 0;
    strcat(url, config.flag);
    l = strlen(url);
    url[l] = '/';
    ++l;

    len = strlen(letters);
    while (index)
    {
        c = index % len;
        url[l] = letters[c];
        ++l;
        index = index / len;
    }

    url[l] = 0;
    return true;
}
