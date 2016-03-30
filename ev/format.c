/**
 *   author       :   丁雪峰
 *   time         :   2016-03-29 14:18:47
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include "evhttp.h"
#include "parser.h"

static int
format_header(struct http *h, struct format_item *item, char *buf, size_t size)
{
    char *value;
    size_t value_n;

    value = "-";
    value_n = 1;

    parser_get_http_field_value(&h->header_res, item->item, item->item_n,
            &value, &value_n);

    if (size > value_n)
    {
        size = value_n;
    }
    memcpy(buf, value, size);

    return (int)size;
}

static int
format_time(struct http *h, struct format_item *item,
        char *buf, size_t size)
{
    int t;
    const char *field = item->item;
    t = 0;
    if (0 == strncmp(field, "dns", 3)) t = h->time_dns;
    else if (0 == strncmp(field, "con",      3)) t = h->time_connect;
    else if (0 == strncmp(field, "res",      3)) t = h->time_response;
    else if (0 == strncmp(field, "trans",    5)) t = h->time_trans;
    else if (0 == strncmp(field, "total",    5)) t = h->time_total;
    else if (0 == strncmp(field, "max_read", 8)) t = h->time_max_read;

    return snprintf(buf, size, "%d.%03d", t/1000, t % 1000);
}

static int
format_info(struct http *h, struct format_item *item,
        char *buf, size_t size)
{
    int t = 0;
    const char *field = item->item;
    if (0 == strncmp(field, "speed", 5))
    {
        if (h->time_trans)
        {
            return size_fmt(buf, size,
                (double)h->content_recv/h->time_total * 1000);
        }
        else{
            *buf = '-';
            return 1;
        }
    }

    if (0 == strncmp(field, "status", 5)) t = h->header_res.status;
    else if (0 == strncmp(field, "index", 5)) t = h->index;
    else if (0 == strncmp(field, "lport", 5)) t = h->port;
    else if (0 == strncmp(field, "recv",  4)) t = h->content_recv;

    return snprintf(buf, size, "%d", t);
}


struct map{
    const char *flag;
    void *handle;
};
struct map map_handles[] = {
    {"header.res.", format_header},
    {"time.",       format_time},
    {"info.",       format_info},
    {NULL,          NULL},
};

static int format_set_handle(struct format_item *item)
{
    size_t flag_n;
    struct map *m;

    m = map_handles;
    while (m->flag)
    {
        flag_n = strlen(m->flag);
        if (item->item_n > flag_n)
        {
            if (0 == strncmp(item->item, m->flag, flag_n))
            {
                item->item += flag_n;
                item->item_n -= flag_n;
                item->handle = m->handle;
                return 0;
            }
        }
        ++m;
    }

    seterr("not fond the handle for this item: %.*s", item->item_n, item->item);
    return -1;
}



static int _format_compile(struct config *config)
{
    const char *pos, *start;
    struct format_item *item;

    item = config->fmt_items;
    start = pos = config->fmt;

    item->type = FORMAT_TYPE_NONE;

try:
    while (*pos)
    {
        if ('$' == *pos)
        {
            if (pos - start > 0)
            {
                item->type = FORMAT_TYPE_ORGIN;
                item->item = start;
                item->item_n = pos - start;
                ++item;
            }

            start = pos + 1;
            if (0 == *start){
                seterr("fmt last char is '$'");
                return -1;
            }
            pos = start + 1;


            while (*pos)
            {
                if ('$' == *pos)
                {
                    item->type = FORMAT_TYPE_FMT;
                    item->item = start;
                    item->item_n = pos - start;
                    if (-1 == format_set_handle(item)) return -1;
                    ++item;

                    pos = start = pos + 1; // new begin
                    goto try;
                }
                ++pos;

            }
            seterr("the last fmt not end with '$'");
            return -1;
        }
        ++pos;
    }

    if (pos - start > 0)
    {
        item->type = FORMAT_TYPE_ORGIN;
        item->item = start;
        item->item_n = pos - start;
        ++item;
    }

    item->type = FORMAT_TYPE_NONE;
    return 0;
}

int format_compile(struct config *config, const char * arg, bool isfile)
{
    const char *pos;
    size_t dollar_num, item_num;
    int res;

    if (config->fmt) return 0;
    config->print = PRINT_FMT;

    if (isfile)
    {
        size_t l;
        res = fileread(arg, &config->fmt, &l);
        if (-1 == res)
        {
            printf("Format Error: %s\n", geterr());
            return -1;
        }
    }
    else{
        config->fmt  = malloc(strlen(arg) + 1);
        memcpy(config->fmt, arg, strlen(arg) + 1);
    }

    pos = config->fmt;
    dollar_num = 0;

    while (*pos)
    {
        if (*pos == '$') dollar_num += 1;
        ++pos;
    }
    item_num = dollar_num + 3;

    config->fmt_items = malloc(item_num * sizeof(*config->fmt_items));

    res = _format_compile(config);
    if (-1 == res)
    {
        printf("Format Error: %s\n", geterr());
        free(config->fmt_items);
        return -1;
    }
    return 0;
}

const char * format_handle(struct config *config, struct http *h)
{
    struct format_item *item;
    char *pos;
    size_t size, left;

    item = config->fmt_items;
    pos = config->fmt_buffer;
    while (FORMAT_TYPE_NONE != item->type)
    {
        left = sizeof(config->fmt_buffer) - (pos - config->fmt_buffer) - 2;
        if (FORMAT_TYPE_ORGIN == item->type)
        {
            if (item->item_n > left)
                size = left;
            else
                size = item->item_n;

            memcpy(pos, item->item, size);
            pos += size;
        } else {
            pos += item->handle(h, item, pos, left);
        }
        ++item;
    }

    if ('\n' != *(pos - 1))
    {
        *pos = '\n';
        ++pos;
    }
    *pos = 0;

    return config->fmt_buffer;
}

void format_destroy(struct config *config)
{
    if (config->fmt_items)
    {
        free(config->fmt_items);
        config->fmt_items = NULL;
    }

    if (config->fmt)
    {
        free(config->fmt);
        config->fmt = NULL;
    }
}
