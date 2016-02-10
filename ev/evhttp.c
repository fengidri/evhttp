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

void http_destory(struct http *);
void http_chunk_read(struct http *h, size_t offset);
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
    h->buf_offset   = 0;
    h->eof          = 0;
    h->read_header  = true;
    h->remote       = &h->_remote;
    h->content_recv = 0;
    h->fd           = -1;
    h->next_state   = HTTP_NEW;
}


int process_header(struct http *h)
{
    char *pos;
    int length = 0;

    pos = strnstr(h->buf, "\r\n\r\n", h->buf_offset);
    if (!pos)
        return EV_AG;
    h->read_header = false;

    length = pos - h->buf + 4;
    h->status = atoi(h->buf + 9);
    if (200 == h->status)
    {
        config.sum_status_200++;
    }
    else
        config.sum_status_other++;

    logdebug("%.*s", length, h->buf);

    int i = 0;
    while(i<length)
    {
        h->buf[i] = tolower(h->buf[i]);
        i++;
    }

    pos = strnstr(h->buf, "content-length:", length);
    if (pos)
    {
        h->content_recv = h->buf_offset - length;
        h->content_length = atoi(pos + 15);
        h->chunked = false;
        return EV_OK;
    }

    pos = strnstr(h->buf, "transfer-encoding:", length);
    if (pos)
    {
        h->chunk_length = -1;
        h->chunk_recv = 0;
        http_chunk_read(h, length);
        h->chunked = true;
        return EV_OK;
    }
    h->content_length = -1;
    h->chunked = false;
    return EV_OK;
}

int recv_no_chunked(int fd, struct http *h)
{
    int n;
check:
    if (h->content_length > -1)
    {
        if (h->content_recv >= h->content_length)
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

void http_chunk_read(struct http *h, size_t offset)
{
    int n;
    size_t nn = 0;
    char *err = NULL;
    n = chunk_read(h->buf + offset, h->buf_offset - offset,
            &h->chunk_length, &h->chunk_recv, &nn, &err);

    if (err)
        logerr(h, err);

    h->buf_offset = h->buf_offset - offset - n;
    h->content_recv += nn;

    if (h->buf_offset > 0)
        memmove(h->buf, h->buf + offset + n, h->buf_offset);
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

    http_chunk_read(h, 0);
    goto check;
}


int recv_response(struct http *h)
{
    int (*handle)(int , struct http *);

    int t = update_time(h);
    if (t > h->time_max_read) h->time_max_read = t;

    if (h->chunked)
        handle = recv_is_chunked;
    else
        handle = recv_no_chunked;

    if (EV_AG != handle(h->fd, h))
    {
        h->next_state = HTTP_END;
        return EV_AG;
    }
    return EV_OK;

}

int recv_header(struct http *h)
{
    int n;
    if (0 == h->buf_offset)
    {
        h->time_recv = update_time(h);
        h->time_start_read = h->time_last;
    }
    while (1)
    {
        n = ev_recv(h->fd, h->buf + h->buf_offset,
                sizeof(h->buf) - h->buf_offset);

        if (EV_AG  == n) return EV_OK;
        if (EV_ERR == n) return EV_ERR;

        if (n == 0){
            logerr(h, "Server close connection prematurely "
                    "while reading header!!\n");
            return EV_ERR;
        }

        h->buf_offset += n;

        if (EV_OK == process_header(h))
        {
            h->next_state = HTTP_RECV_BODY;
            return EV_AG;
        }

        if (h->buf_offset >= sizeof(h->buf))
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

    h->time_connect = update_time(h);

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


    logdebug("===========================================\n");
    logdebug("%.*s", n, h->buf);

    n = send(h->fd, h->buf, n, 0);
    if (n < 0)
    {
        h->next_state = HTTP_END;
        logerr(h, "Send Error\n");
        return EV_ERR;
    }
    h->next_state = HTTP_RECV_HEADER;

#include <sys/socket.h>
#include <arpa/inet.h>
    // get local port
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);

    getsockname(h->fd, (struct sockaddr *)&c, &cLen);
    h->port = ntohs(c.sin_port);

    return EV_OK;
}

int http_end(struct http *h)
{
    h->time_last = h->time_start_read;
    h->time_trans = update_time(h);
    print_http_info(h);

    if (h->fd > 0)
    {
        aeDeleteFileEvent(config.el, h->fd, AE_READABLE);
        aeDeleteFileEvent(config.el, h->fd, AE_WRITABLE);
        close(h->fd);
    }

    config.total += 1;
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
            update_time(h);
            if (!h->remote->ip[0])
            {
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
            h->time_dns = update_time(h);
            return EV_AG;

        case HTTP_DNS_POST:

        case HTTP_CONNECT:
            logdebug("Connecting to %s:%d....\n", h->remote->ip, h->remote->port);
            h->fd = net_connect(h->remote->ip, h->remote->port);

            if (h->fd < 0){
                usleep(100000);
                h->next_state = HTTP_NEW;
                return EV_AG;
            }

            h->next_state = HTTP_SEND_REQUEST;
            aeCreateFileEvent(config.el, h->fd, AE_WRITABLE, ev_handler, h);
            aeCreateFileEvent(config.el, h->fd, AE_READABLE, ev_handler, h);
            return EV_OK;


        case HTTP_SEND_REQUEST: return send_request(h);
        case HTTP_RECV_HEADER:  return recv_header(h);
        case HTTP_RECV_BODY:    return recv_response(h);
        case HTTP_END:          return http_end(h);
    }
}




void http_new()
{
    struct http *h;

    h = malloc(sizeof(*h));
    memset(h, 0, sizeof(*h));
    http_reset(h);

    config.active += 1;
    ev_handler(NULL, -1, h, 0);
}


