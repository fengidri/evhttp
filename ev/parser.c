/**
 *   author       :   丁雪峰
 *   time         :   2016-03-29 04:22:38
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "parser.h"


int parser_get_http_field_value(struct http_response_header *res,
        const char *target, size_t target_n, char **tvalue, size_t *tvalue_n)
{
    char *t;

    char *header_e; // pos of \r\n;   //header == [field]:[value][\r\n]
    size_t header_n; // sizeof ([field]:[value])
    char *field;
    size_t field_n;
    char *value;
    size_t value_n;

    field = res->fields;
    while (1)
    {
        header_e = strstr(field, "\r\n");
        header_n = header_e - field;
        if (0 == header_n)
            break;

        t = strnstr(field, ":", header_n);
        if (t)
        {
            field_n =  t - field;
            value = t + 1;
            value_n = header_e - value;

            value = nskipspace(value, value_n);
            if (value)
                value_n = header_e - value;
            else{
                value = "";
                value_n = 0;
            }
        }
        else{
            field_n = header_n;
            value = "";
            value_n = 0;
        }

        if (field_n == target_n)
        {
            if (0 == strncasecmp(target, field, field_n))
            {
                *tvalue = value;
                *tvalue_n = value_n;
                return 0;
            }
        }
        field = header_e + 2;
    }
    return -1;
}

/**
 * read_chunk --
 * @buffer:
 * @size:
 * @length: chunk length
 * @recved: chunk recved
 * return : show the date has been readed;
 */
static int chunk_read(char *buffer, size_t size, int *length, int *recved,
        size_t *body)
{
    char *pos, *buf;
    size_t left, except;

    pos = buf = buffer;
    left = size;

    if (0 == *length) return 0;
    if (*length >= 1) goto read;

    while (1)
    {
        pos = strnstr(buf, "\r\n", left);
        if (!pos){
            *length = -1;
            return buf - buffer;
        };

        *length = strtol(buf, NULL, 16);
        if (0 == *length)
        {
            if (5 > left)
            {
                *length = -1;
                return 0;
            }
            if ('0' != *buf)
            {
                seterr("invalid chunked response!");
                return EV_ERR;
            }
            return pos - buffer + 4;
        }
        *body += *length;

        *length = *length + 2;
        *recved = 0;

        buf = pos + 2;
        left = size - (buf - buffer);
read:
        except = *length - *recved;
        if (left <= except)
        {
            *recved += left;
            if (left == except) *length = -1;
            return size;
        }

        buf = buf + except;
        left = size - (buf - buffer);
    }
}


int http_chunk_read(struct http *h, char *buf, size_t size)
{
    int n;
    size_t nn = 0;
    n = chunk_read(buf, size,
            &h->chunk_length, &h->chunk_recv, &nn);

    if (-1 == n) return EV_ERR;

    h->buf_offset = size - n;
    h->content_recv += nn;

    if (h->buf_offset > 0)
        memmove(h->buf, buf + n, h->buf_offset);
    return EV_OK;
}

int process_header(struct http *h)
{
    char *pos;
    struct http_response_header *res;
    res = &h->header_res;


    pos = strnstr(res->buf, "\r\n\r\n", res->buf_offset);
    if (!pos)
        return EV_AG;

    res->length = pos - res->buf + 4; // header length

    pos = strnstr(res->buf, "\r\n", res->length);
    if (*(pos + 1) == '\r')
    {
        //logerr("error response header.\n");
        return EV_ERR;
    }

    if (config.print & PRINT_RESPONSE)
        logdebug("%.*s", res->length, res->buf);

    res->response_line = res->buf;
    res->response_line_n = pos - res->buf;

    res->fields = pos + 2;

    res->status = atoi(res->response_line + 9);


    char *value;
    size_t value_n;
    int r;
    r = parser_get_http_field_value(res, "content-length", 14,
            &value, &value_n);
    if (0 == r)
    {
        res->chunked = false;
        res->content_length = atoi(value);

        h->content_recv = res->buf_offset - res->length;
        return EV_OK;
    }
    else {
        r = parser_get_http_field_value(res, "transfer-encoding", 17,
                &value, &value_n);
        if (0 == r)
        {
            h->chunk_length = -1;
            h->chunk_recv = 0;

            res->chunked = true;
            return http_chunk_read(h, res->buf + res->length,
                    res->buf_offset - res->length);
        }
    }


    res->content_length = -1;
    res->chunked = false;
    return EV_OK;
}
