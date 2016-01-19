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

int http_new();
void http_destory(struct http *);
void http_chunk_read(struct http *h, size_t offset);

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



int recv_header(int fd, struct http *h)
{
    int n;
    while (1)
    {
        n = ev_recv(fd, h->buf + h->buf_offset, sizeof(h->buf) - h->buf_offset);

        if (n < 0)  return n;
        if (n == 0){
            logerr("Server close connection prematurely "
                    "while reading header!!\n");
            return EV_ERR;
        }

        h->buf_offset += n;

        if (EV_OK == process_header(h))
        {
            return EV_OK;
        }
        if (h->buf_offset >= sizeof(h->buf))
        {
            logerr("Header Too big!");
            return EV_ERR;
        }
    }
    return EV_ERR;
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
                logerr("Server close connection prematurely!!");
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
                logerr("Chunked Fmort Error!\n");
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
    n = chunk_read(h->buf + offset, h->buf_offset - offset,
            &h->chunk_length, &h->chunk_recv, &nn);

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
        logerr("Server close connection prematurely!!");
        return  EV_ERR;
    }

    n = ev_recv(fd, h->buf + h->buf_offset, sizeof(h->buf) - h->buf_offset);
    if (n < 0) return n;
    if (n == 0) h->eof = true;
    h->buf_offset += n;

    http_chunk_read(h, 0);
    goto check;
}


void recv_response(aeEventLoop *el, int fd, void *priv, int mask)
{
    struct http *h;
    int (*handle)(int , struct http *);
    int ret;

    h = (struct http *)priv;

    if (h->read_header)
    {
        if (0 == h->buf_offset)
        {
            h->time_recv = update_time(h);
            h->time_start_read = h->time_last;
        }
        ret = recv_header(fd, h);
        if (EV_OK == ret)
        {
            recv_response(el, fd, priv, mask);
            return;
        }
    }
    else{
        int t = update_time(h);
        if (t > h->time_max_read) h->time_max_read = t;

        if (h->chunked)
            handle = recv_is_chunked;
        else
            handle = recv_no_chunked;

        ret = handle(fd, h);
    }

    if (EV_AG == ret) return;

    h->time_last = h->time_start_read;
    h->time_trans = update_time(h);

    if (EV_OK == ret)
    {
        print_http_info(h);
    }

    http_destory(h);
}


void send_request(aeEventLoop *el, int fd, void *priv, int mask)
{
#define request_fmt "%s %s HTTP/1.1\r\n" \
                    "Host: %s\r\n" \
                    "Content-Length: 0\r\n" \
                    "User-Agent: evhttp\r\n" \
                    "Accept: */*\r\n"
    int n, l;
    struct http *h = priv;

    h->time_connect = update_time(h);

    l = sizeof(h->buf) - config.headers_n - 2;
    n = snprintf(h->buf, l, request_fmt,
            config.method, h->url, h->remote->domain);

    if (n >= l)
    {
        logerr("URL too long!!!!");
        http_destory(h);
        return;
    }

    memcpy(h->buf + n, config.headers, config.headers_n);
    n += config.headers_n;

    h->buf[n] = '\r';
    ++n;
    h->buf[n] = '\n';
    ++n;


    logdebug("===========================================\n");
    logdebug("%.*s", n, h->buf);

    n = send(fd, h->buf, n, 0);
    if (n < 0)
    {
        http_destory(h);
        return ;
    }

    aeDeleteFileEvent(el, fd, AE_WRITABLE);
    aeCreateFileEvent(el, fd, AE_READABLE, recv_response, h);
}

int http_new()
{
    struct http *h;

    h = malloc(sizeof(*h));
    h->buf_offset  = 0;
    h->eof         = 0;
    h->read_header = true;
    h->remote      = &h->_remote;
    h->content_recv = 0;

url:
    if (!get_url(h))
    {
        free(h);
        if (config.active <= 0)
        {
            aeStop(config.el);
        }
        return EV_OK;
    }

    update_time(h);
    if (!h->remote->ip[0])
    {
        logdebug("resolve %s\n", h->remote->domain);
        if (!net_resolve(h->remote->domain, h->remote->ip,
                    sizeof(h->remote->ip)))
        {
            usleep(100000);
            goto url;
        }
    }

    h->time_dns = update_time(h);

    logdebug("Connecting to %s:%d....\n", h->remote->ip, h->remote->port);
    h->fd = net_connect(h->remote->ip, h->remote->port);

    if (h->fd < 0){
        usleep(100000);
        goto url;
    }

    aeCreateFileEvent(config.el, h->fd, AE_WRITABLE, send_request, h);
    config.active += 1;

    return EV_OK;
}

void http_destory(struct http *h)
{
    aeDeleteFileEvent(config.el, h->fd, AE_READABLE);
    aeDeleteFileEvent(config.el, h->fd, AE_WRITABLE);
    close(h->fd);
    free(h);

    config.active -= 1;
    config.total += 1;

    http_new();
}
