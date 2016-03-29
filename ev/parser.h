/**
 *   author       :   丁雪峰
 *   time         :   2016-03-29 04:23:37
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __PARSER_H__
#define __PARSER_H__
#include "evhttp.h"
int parser_get_http_field_value(struct http_response_header *res,
        const char *target, size_t target_n, char **tvalue, size_t *tvalue_n);
int chunk_read(char *buffer, size_t size, int *length, int *recved,
        size_t *body, char **err);

#endif


