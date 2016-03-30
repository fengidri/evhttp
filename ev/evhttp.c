/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 08:11:35
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */

#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>

#include "url.h"
#include "evhttp.h"
#include "parser.h"
#include "format.h"

void http_destory(struct http *);
void http_chunk_read(struct http *h, char *buf, size_t size);
int httpsm(struct http *h, int mask);

struct config config = {
    .remote.port   = 80,
    .parallel      = 1,
    .total_limit   = 1,
    .flag          = "M",
    .sum           = false,
    .recycle       = NULL,
    .recycle_times = 1,
    .urls          = config._urls,
    .loglevel      = LOG_DEBUG,
    .method        = "GET",
    .print         = PRINT_RESPONSE | PRINT_REQUEST | PRINT_TIME_H \
                     | PRINT_TIME | PRINT_CON | PRINT_DNS,
};

static inline int ev_recv(int fd, char *buf, size_t len)
{
    int n;
    n = net_recv(fd, buf, len);
    if (n > 0)
    {
        config.sum_recv += n;
        config.sum_recv_cur += n;
    }
    return n;
}

void http_reset(struct http *h)
{
    memset(h, 0, sizeof(*h));
    config.total += 1;

    h->remote       = &h->_remote;
    h->fd           = -1;
    h->next_state   = HTTP_NEW;
    h->index        = config.total;

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
    if (0 == parser_get_http_field_value(res, "content-length", 14, &value, &value_n))
    {
        res->chunked = false;
        res->content_length = atoi(value);

        h->content_recv = res->buf_offset - res->length;
        return EV_OK;
    }

    if (0 == parser_get_http_field_value(res, "transfer-encoding", 17, &value, &value_n))
    {
        h->chunk_length = -1;
        h->chunk_recv = 0;

        res->chunked = true;
        http_chunk_read(h, res->buf + res->length, res->buf_offset - res->length);
        return EV_OK;
    }

    res->content_length = -1;
    res->chunked = false;
    return EV_OK;
}

int recv_no_chunked(int fd, struct http *h)
{
    int n;
check:
    if (h->header_res.content_length >= 0)
    {
        if (h->content_recv >= h->header_res.content_length)
        {
            return EV_OK;
        }
        else{
            if (h->eof)
            {
                logerr(h, "Server close connection prematurely!!\n");
                return EV_ERR;
            }
        }
    }
    else{
        if (h->eof)
            return EV_OK;
    }

    n = ev_recv(fd, h->buf, sizeof(h->buf));
    if (n < 0) return n;
    if (n == 0) h->eof = true;

    h->content_recv += n;
    goto check;
}



void http_chunk_read(struct http *h, char *buf, size_t size)
{
    int n;
    size_t nn = 0;
    char *err = NULL;
    n = chunk_read(buf, size,
            &h->chunk_length, &h->chunk_recv, &nn, &err);

    if (err)
        logerr(h, err);

    h->buf_offset = size - n;
    h->content_recv += nn;

    if (h->buf_offset > 0)
        memmove(h->buf, buf + n, h->buf_offset);
}

int recv_is_chunked(int fd, struct http *h)
{
    int n;
check:
    if (0 == h->chunk_length) return EV_OK;
    if (h->eof)
    {
        logerr(h, "Server close connection prematurely!!\n");
        return  EV_ERR;
    }

    n = ev_recv(fd, h->buf + h->buf_offset, sizeof(h->buf) - h->buf_offset);
    if (n < 0) return n;
    if (n == 0) h->eof = true;
    h->buf_offset += n;

    http_chunk_read(h, h->buf, h->buf_offset);
    goto check;
}


int recv_response(struct http *h)
{
    int (*handle)(int , struct http *);

    update_time(h, HTTP_RECV_BODY);

    if (h->header_res.chunked)
        handle = recv_is_chunked;
    else
        handle = recv_no_chunked;

    if (EV_AG != handle(h->fd, h))
    {
        h->next_state = HTTP_END;
        return EV_AG;
    }

    if (config.print & PRINT_BAR)
        logdebug("\rRecv: %d ", h->content_recv);
    return EV_OK;

}

int recv_header(struct http *h)
{
    int n;
    struct http_response_header *res;
    res = &h->header_res;

    if (0 == res->buf_offset)
    {
        update_time(h, HTTP_RECV_HEADER);
    }
    while (1)
    {
        n = ev_recv(h->fd, res->buf + res->buf_offset,
                sizeof(res->buf) - res->buf_offset);

        if (EV_AG  == n) return EV_OK;
        if (EV_ERR == n) return EV_ERR;

        if (n == 0){
            logerr(h, "Server close connection prematurely "
                    "while reading header!!\n");
            return EV_ERR;
        }

        res->buf_offset += n;

        if (EV_OK == process_header(h))
        {
            if (200 == h->header_res.status)
            {
                config.sum_status_200++;
            }
            else
                config.sum_status_other++;

            h->next_state = HTTP_RECV_BODY;
            return EV_AG;
        }

        if (res->buf_offset >= sizeof(res->buf))
        {
            logerr(h, "Header Too big!\n");
            return EV_ERR;
        }
    }
    return EV_ERR;
}


int send_request(struct http *h)
{
#define request_fmt "%s %s HTTP/1.1\r\n" \
                    "Host: %s\r\n" \
                    "Content-Length: 0\r\n" \
                    "User-Agent: evhttp\r\n" \
                    "Accept: */*\r\n"
    int n, l;

    l = sizeof(h->buf) - config.headers_n - 2;
    n = snprintf(h->buf, l, request_fmt,
            config.method, h->url, h->remote->domain);

    if (n >= l)
    {
        h->next_state = HTTP_END;
        logerr(h, "URL too long!!!!\n");
        return EV_ERR;
    }

    memcpy(h->buf + n, config.headers, config.headers_n);
    n += config.headers_n;

    h->buf[n] = '\r';
    ++n;
    h->buf[n] = '\n';
    ++n;

    n = send(h->fd, h->buf, n, 0);
    if (n < 0)
    {
        h->next_state = HTTP_END;
        logerr(h, "Send Error\n");
        return EV_ERR;
    }

    update_time(h, HTTP_SEND_REQUEST);

    if (config.print & PRINT_RESPONSE)
    {
        logdebug("===========================================\n");
        logdebug("%.*s", n, h->buf);
    }

    h->next_state = HTTP_RECV_HEADER;

    // get local port
    h->port = net_client_port(h->fd);

    return EV_OK;
}

int http_end(struct http *h)
{
    update_time(h, HTTP_END);

    if (config.fmt_items && config.print & PRINT_FAT)
    {
        format_handle(&config, h);
        logdebug("%s", config.fmt_buffer);
    }

    if (h->fd > 0)
    {
        aeDeleteFileEvent(config.el, h->fd, AE_READABLE);
        aeDeleteFileEvent(config.el, h->fd, AE_WRITABLE);
        close(h->fd);
    }

    http_reset(h);

    return EV_AG;
}

void ev_handler(aeEventLoop *el, int fd, void *priv, int mask)
{
    struct http *h = priv;
    int ret;
    do{
        ret = httpsm(h, mask);
    }while(ret == EV_AG);
}


int httpsm(struct http *h, int mask)
{
    switch(h->next_state)
    {
        case HTTP_NEW:
            update_time(h, HTTP_NEW);
            h->next_state = HTTP_DNS;
            if (!get_url(h))
            {
                free(h); // STOP
                config.active -= 1;
                if (config.active <= 0) aeStop(config.el);
                return EV_ERR;
            }
            return EV_AG;

        case HTTP_DNS:
            if (!h->remote->ip[0])
            {
                if (config.print & PRINT_DNS)
                    logdebug("resolve %s\n", h->remote->domain);
                if (!net_resolve(h->remote->domain, h->remote->ip,
                            sizeof(h->remote->ip)))
                {
                    logerr(h, "reslove fail: %s\n", h->remote->domain);
                    usleep(100000);
                    h->next_state = HTTP_NEW;
                    return EV_AG;
                }
            }
            h->next_state = HTTP_CONNECT;
            return EV_AG;

        case HTTP_DNS_POST:
            update_time(h, HTTP_DNS_POST);

        case HTTP_CONNECT:
            if (config.print & PRINT_CON)
                logdebug("Connecting to %s:%d....\n",
                        h->remote->ip, h->remote->port);
            h->fd = net_connect(h->remote->ip, h->remote->port);

            if (h->fd < 0){
                //usleep(100000);
                h->next_state = HTTP_END;
                return EV_AG;
            }

            h->next_state = HTTP_CONNECT_POST;
            aeCreateFileEvent(config.el, h->fd, AE_WRITABLE, ev_handler, h);
            aeCreateFileEvent(config.el, h->fd, AE_READABLE, ev_handler, h);
            return EV_OK;

        case HTTP_CONNECT_POST:
            h->next_state = HTTP_SEND_REQUEST;
            return EV_AG;


        case HTTP_SEND_REQUEST: return send_request(h);
        case HTTP_RECV_HEADER:  return recv_header(h);
        case HTTP_RECV_BODY:    return recv_response(h);
        case HTTP_END:          return http_end(h);
    }
    return EV_OK;
}




void http_new()
{
    struct http *h;

    h = malloc(sizeof(*h));
    http_reset(h);

    config.active += 1;
    ev_handler(NULL, -1, h, 0);
}


