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
int chunk_read(char *buffer, size_t size, int *length, int *recved,
        size_t *body, char **err)
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
                *err = "invalid chunked response!\n";
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
