/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 08:11:35
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include <stdbool.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <error.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>

#include "ae.h"

static char server_host[20] = "127.0.0.1";
static int server_port = 80;
static int parallel = 1;
static int total_request = 1;
static char *http_host = "archlinux";
static char *flag = "M";
static aeEventLoop *el;
static int Index;
static char *letters = "/1234567890asbcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXY";

struct response{
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
};

#define EV_OK 0
#define EV_ERR -2
#define EV_AG  -3

#define logerr(fmt, ...)
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
int http_new();
void http_destory(struct response *);

char* strnstr(char* s1, char* s2, size_t size)
{
    char c;
    char *p;
    c = s1[size -1];
    s1[size - 1] = 0;
    p = strstr(s1, s2);
    s1[size - 1] = c;
    return p;
}

int net_noblock(int fd, bool b)
{
    int flags;

    /* Set the socket blocking (if non_block is zero) or non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        return -1;
    }

    if (b)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        return -1;
    }
    return 0;
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
    remote_addr.sin_addr.s_addr = inet_addr(server_host);//服务器IP地址
    remote_addr.sin_port = htons(server_port); //服务器端口号

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

int process_header(struct response *res)
{
    char *pos;
    int length = 0;

    pos = strnstr(res->buf, "\r\n\r\n", res->buf_offset);
    if (!pos)
        return EV_AG;
    res->read_header = false;

    length = pos - res->buf + 4;
    res->content_recv = res->buf_offset - length;
    res->status = atoi(res->buf + 9);

    int i = 0;
    while(i<length)
    {
        res->buf[i] = tolower(res->buf[i]);
        i++;
    }

    pos = strnstr(res->buf, "content-length:", length);
    if (pos)
    {
        res->content_length = atoi(pos + 15);
        res->chunked = false;
        return EV_OK;
    }

    pos = strnstr(res->buf, "transfer-encoding:", length);
    if (pos)
    {
        res->buf_offset = res->buf_offset - length;
        memcpy(res->buf, res->buf + length, res->buf_offset);
        res->chunk_length = -1;
        res->chunked = true;
        return EV_OK;
    }
    res->content_length = -1;
    res->chunked = false;
    return EV_OK;
}

int net_recv(int fd, char *buf, size_t len)
{
    int n, err;

    n = recv(fd, buf, len, 0);
    err = errno; // save off errno, because because the printf statement might reset it
    if (n < 0)
    {
        if ((err == EAGAIN) || (err == EWOULDBLOCK))
        {
            return EV_AG;
        }
        else
        {
            return EV_ERR;
        }
    }
    return n;
}

void make_url(char *url, size_t size)
{
    size_t l, c, len;
    int index = Index;
    Index += 1;

    url[0] = '/';
    url[1] = 0;
    strcat(url, flag);
    l = strlen(url);
    url[l] = '/';
    ++l;

    len = strlen(letters);
    while (index)
    {
        c = index % len;
        url[l] = letters[c];
        ++l;
        index = index / len;
    }

    url[l] = 0;

}

int recv_header(int fd, struct response *res)
{
    int n;
    while (1)
    {
        n = net_recv(fd, res->buf + res->buf_offset,
                sizeof(res->buf) - res->buf_offset);

        if (n < 0)  return n;
        if (n == 0) return EV_ERR;

        res->buf_offset += n;

        if (EV_OK == process_header(res))
        {
            return EV_OK;
        }
        if (res->buf_offset >= sizeof(res->buf))
        {
            logerr("Header Too big!");
            return EV_ERR;
        }
    }
    return EV_ERR;
}

int recv_no_chunked(int fd, struct response *res)
{
    int n;
    while (1)
    {
        n = net_recv(fd, res->buf, sizeof(res->buf));
        if (n < 0) return n;

        res->content_recv += n;

        if (res->content_length > -1)
        {
            if (res->content_recv >= res->content_length)
            {
                return EV_OK;
            }
            else{
                if (0 == n)
                    return EV_OK;
            }
        }
        else{
            if (0 == n)
                return EV_OK;
        }
    }
}

int recv_is_chunked(int fd, struct response *res)
{
    int n, size;
    char *pos;

    if (res->chunk_length == -1)
    {
        while(1)
        {
            if (res->buf_offset > 2)
            {
                pos = strnstr(res->buf, "\r\n", res->buf_offset);
                if (pos)
                {
                    res->chunk_length = strtol(res->buf, NULL, 16);
                    if (0 == res->chunk_length) return EV_OK; // END

                    res->content_recv += res->buf_offset - (pos - res->buf + 2);
                    res->chunk_recv = res->buf_offset - (pos - res->buf + 2);
                    goto chunk;
                }
            }
            if (res->buf_offset > 300) return EV_ERR;

            n = net_recv(fd, res->buf + res->buf_offset,
                    sizeof(res->buf) - res->buf_offset);

            if (n < 0) return n;
            if (0 == n) return EV_ERR;
            res->buf_offset += n;
        }
    }

chunk:

    while (1) // recv chunk
    {
        size = MIN(sizeof(res->buf), res->chunk_length + 2 - res->chunk_recv);
        n = net_recv(fd, res->buf, size);
        if (n < 0) return n;
        if (n == 0) return EV_ERR;

        res->content_recv += n;
        res->chunk_recv += n;
        if (res->chunk_recv - res->chunk_length == 2)
        {
            res->chunk_length = -1;
            res->content_recv -= 2;
            res->buf_offset = 0;
            return recv_is_chunked(fd, res);
        }
    }
}

void recv_response(aeEventLoop *el, int fd, void *priv, int mask)
{
    struct response *res;
    int (*handle)(int , struct response *);
    int ret;

    res = (struct response *)priv;

    if (res->read_header)
    {
        ret = recv_header(fd, res);
        if (EV_OK == ret)
        {
            recv_response(el, fd, priv, mask);
            return;
        }
    }
    else{
        if (res->chunked)
            handle = recv_is_chunked;
        else
            handle = recv_no_chunked;

        ret = handle(fd, res);
    }

    if (EV_AG == ret) return;

    // END
    if (EV_ERR == ret)
    {
        printf("Recv Response Error!!!\n");
    }

    if (EV_OK == ret)
    {
        printf("Status: %d Recv-Length: %d URL: %s\n",
                res->status, res->content_recv, res->url);
    }

    http_destory(res);
}


void send_request(aeEventLoop *el, int fd, void *priv, int mask)
{
    int n;
    struct response *res = priv;

    char *request = "GET %s HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Content-Length: 0\r\n"
                    "User-Agent: evhttp\r\n"
                    "Accept: */*\r\n"
                    "\r\n";
    n = snprintf(res->buf, sizeof(res->buf), request, res->url, http_host);
    if (n >= sizeof(res->buf))
    {
        http_destory(res);
        return ;
    }

    n = send(fd, res->buf, n, 0);
    if (n < 0)
    {
        http_destory(res);
        return ;
    }

    aeDeleteFileEvent(el, fd, AE_WRITABLE);
    aeCreateFileEvent(el, fd, AE_READABLE, recv_response, res);
}

int http_new()
{
    if (total_request == 0) return 0;

    int fd =  http_connect();
    if (fd < 0){
        return EV_ERR;
    }

    struct response *res;
    res = malloc(sizeof(*res));
    res->buf_offset = 0;
    res->read_header = true;
    make_url(res->url, sizeof(res->url));
    res->fd = fd;

    aeCreateFileEvent(el, fd, AE_WRITABLE, send_request, res);
    return EV_OK;
}

void http_destory(struct response *res)
{
    aeDeleteFileEvent(el, res->fd, AE_READABLE);
    aeDeleteFileEvent(el, res->fd, AE_WRITABLE);
    close(res->fd);
    free(res);

    total_request -= 1;

    http_new();
}

int main(int argc, char **argv)
{
    int ch;
    while ((ch = getopt(argc, argv, "H:h:p:l:f:t:")) != -1) {
        switch (ch) {
            case 'h':
                strncpy(server_host, optarg, 15);
                break;
            case 'p':
                server_port = atoi(optarg);
                break;
            case 'l':
                parallel = atoi(optarg);
                break;

            case 'H':
                http_host = optarg;
                break;

            case 'f':
                flag = optarg;
                break;

            case 't':
                total_request = atoi(optarg);
                if (0 == total_request) total_request = -1;
                break;
        }
    }
    printf("Start: Host: %s:%d Parallel: %d\n", server_host, server_port, parallel);

    el = aeCreateEventLoop(129);
    while (parallel)
    {
        http_new();
        parallel--;
    }
    aeMain(el);
}
