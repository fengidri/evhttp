/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 08:11:35
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */

#include "evhttp.h"
#include <sys/socket.h>
#include <error.h>
#include <ctype.h>
#include <errno.h>

#include "url.h"

int http_new();
void http_destory(struct http *);

struct config config = {
    .remote.port   = 80,
    .parallel      = 1,
    .total_limit   = 1,
    .flag          = "M",
    .debug         = false,
    .sum           = false,
    .recycle       = NULL,
    .recycle_times = 1,
    .urls          = config._urls,
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
    h->content_recv = h->buf_offset - length;
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
        h->content_length = atoi(pos + 15);
        h->chunked = false;
        return EV_OK;
    }

    pos = strnstr(h->buf, "transfer-encoding:", length);
    if (pos)
    {
        h->buf_offset = h->buf_offset - length;
        memcpy(h->buf, h->buf + length, h->buf_offset);
        h->chunk_length = -1;
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
        if (n == 0) return EV_ERR;

        h->buf_offset += n;

        if (EV_OK == process_header(h))
        {
            return EV_OK;
        }
        if (h->buf_offset >= (int)sizeof(h->buf))
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
                return EV_OK;
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

int recv_is_chunked(int fd, struct http *h)
{
    int n, size;
    char *pos;

    if (h->chunk_length == -1)
    {
        while(1)
        {
            if (h->buf_offset > 2)
            {
                pos = strnstr(h->buf, "\r\n", h->buf_offset);
                if (pos)
                {
                    h->chunk_length = strtol(h->buf, NULL, 16);
                    if (0 == h->chunk_length) return EV_OK; // END

                    h->content_recv += h->buf_offset - (pos - h->buf + 2);
                    h->chunk_recv = h->buf_offset - (pos - h->buf + 2);
                    goto chunk;
                }
            }
            if (h->buf_offset > 300) return EV_ERR;

            n = ev_recv(fd, h->buf + h->buf_offset,
                    sizeof(h->buf) - h->buf_offset);

            if (n < 0) return n;
            if (0 == n) return EV_ERR;
            h->buf_offset += n;
        }
    }

chunk:
    if (h->chunk_recv - h->chunk_length == 2)
    {
        h->chunk_length = -1;
        h->content_recv -= 2;
        h->buf_offset = 0;
        return recv_is_chunked(fd, h);
    }

    size = MIN((int)sizeof(h->buf), h->chunk_length + 2 - h->chunk_recv);
    n = ev_recv(fd, h->buf, size);
    if (n < 0) return n;
    if (n == 0) h->eof = true;
    h->content_recv += n;
    h->chunk_recv += n;
    goto chunk;

}

void recv_response(aeEventLoop *el, int fd, void *priv, int mask)
{
    struct http *h;
    int (*handle)(int , struct http *);
    int ret;

    h = (struct http *)priv;

    if (h->read_header)
    {
        ret = recv_header(fd, h);
        if (EV_OK == ret)
        {
            recv_response(el, fd, priv, mask);
            return;
        }
    }
    else{
        if (h->chunked)
            handle = recv_is_chunked;
        else
            handle = recv_no_chunked;

        ret = handle(fd, h);
    }

    if (EV_AG == ret) return;

    // END
    if (EV_ERR == ret)
    {
        logerr("Recv Response Error!!!\n");
    }

    if (EV_OK == ret)// just output when not sum
    {
        logdebug("Status: %d Recv: %d URL: %s\n",
                h->status, h->content_recv, h->url);
    }

    http_destory(h);
}


void send_request(aeEventLoop *el, int fd, void *priv, int mask)
{
#define request_fmt "GET %s HTTP/1.1\r\n" \
                    "Host: %s\r\n" \
                    "Content-Length: 0\r\n" \
                    "User-Agent: evhttp\r\n" \
                    "Accept: */*\r\n" \
                    "\r\n"
    int n;
    struct http *h = priv;

    n = snprintf(h->buf, sizeof(h->buf), request_fmt,
            h->url,
            h->remote->domain);

    if (n >= (int)sizeof(h->buf))
    {
        http_destory(h);
        return ;
    }

    logdebug("===========================================\n");
    logdebug("%s", h->buf);

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
