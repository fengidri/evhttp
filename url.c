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

void make_url(char *url, size_t size)
{
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
}
