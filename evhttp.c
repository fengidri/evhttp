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
#include <arpa/inet.h>

#include "url.h"

struct config config;

struct http{
    char url[1024];
    char buf[4 * 1024];
    int buf_offset;
    bool read_header;
    bool chunked;
    int fd;
    int status;
    int content_length;
    int content_recv;
    int chunk_length;
    int chunk_recv;
    bool eof;
};


int http_new();
void http_destory(struct http *);

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

int http_connect()
{
    int s;
    s = socket(AF_INET,  SOCK_STREAM, 0);
    if (s < 0)
    {
        perror("socket");
        return -1;
    }
    if (net_noblock(s, true) < 0)
    {
        perror("noblock");
        return -1;
    }

    struct sockaddr_in remote_addr; //服务器端网络地址结构体
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family = AF_INET; //设置为IP通信
    remote_addr.sin_addr.s_addr = inet_addr(config.remote_add_resolved);//服务器IP地址
    remote_addr.sin_port = htons(config.remote_port); //服务器端口号

    int ret = connect(s, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
    while(ret < 0) {
        if( errno == EINPROGRESS ) {
            break;
        }  else {
            perror("connect fail'\n");
            return -1;
        }
    }

    return s;
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

    if (config.debug)
        printf("%.*s", length, h->buf);

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
        n = ev_recv(fd, h->buf + h->buf_offset,
                sizeof(h->buf) - h->buf_offset);

        if (n < 0)  return n;
        if (n == 0) return EV_ERR;

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
                logerr("Server close connection prematurely!!")
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

    size = MIN(sizeof(h->buf), h->chunk_length + 2 - h->chunk_recv);
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

    if (EV_OK == ret && !config.sum)// just output when not sum
    {
        printf("Status: %d Recv: %d URL: %s\n",
                h->status, h->content_recv, h->url);
    }

    http_destory(h);
}


void send_request(aeEventLoop *el, int fd, void *priv, int mask)
{
    int n;
    struct http *h = priv;

    char *request = "GET %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Content-Length: 0\r\n"
                    "User-Agent: evhttp\r\n"
                    "Accept: */*\r\n"
                    "\r\n";
    n = snprintf(h->buf, sizeof(h->buf), request, h->url, config.http_host);
    if (n >= sizeof(h->buf))
    {
        http_destory(h);
        return ;
    }
    if (config.debug)
        printf("%s", h->buf);

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
    int fd =  http_connect();
    struct http *h;

    if (fd < 0){
        return EV_ERR;
    }

    h = malloc(sizeof(*h));
    h->buf_offset = 0;
    h->read_header = true;
    make_url(h->url, sizeof(h->url));
    h->fd = fd;
    h->eof = 0;

    aeCreateFileEvent(config.el, fd, AE_WRITABLE, send_request, h);

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

    if (config.total_limit > 0)
    {
        if (config.total >=  config.total_limit)
        {
            if (config.active <= 0)
            {
                aeStop(config.el);
            }
        }
        else{
            http_new();
        }
        return;
    }

    if (config.recycle)
    {
        long long target;
        target = 0;
        if (RECYCLE_TIMES == config.recycle_type)
            target = config.total;

        if (RECYCLE_BYTES == config.recycle_type)
            target = config.sum_recv;

        if (target > config.recycle_limit * config.recycle_times)
        {
            config.index         =  0;
            config.recycle_times += 1;
        }
    }

    http_new();
}

