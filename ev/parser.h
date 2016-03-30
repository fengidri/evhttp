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
int process_header(struct http *h);
int http_chunk_read(struct http *h, char *buf, size_t size);

#endif


