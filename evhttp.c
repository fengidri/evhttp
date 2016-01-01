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
static aeEventLoop *el;

struct response{
    char buf[4 * 1024];
    int buf_offset;
    bool read_header;
    bool chunked;
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

    pos = strstr(res->buf, "\r\n\r\n");
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

    pos = strstr(res->buf, "content-length:");
    if (pos)
    {
        res->content_length = atoi(pos + 15);
        res->chunked = false;
        return EV_OK;
    }

    pos = strstr(res->buf, "transfer-encoding:");
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
        res->buf[res->buf_offset] = 0;

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
    if (res->chunk_length == -1)
    {
        res->buf[res->buf_offset] = 0;
        if (strstr(res->buf, "\r\n"))
        {
            res->chunk_length = strtol(res->buf, NULL, 16);
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
        handle = recv_no_chunked;

        ret = handle(fd, res);
    }

    if (EV_AG == ret) return;

    // END
    if (EV_ERR == ret)
    {
        printf("Error!!!");
    }

    if (EV_OK == ret)
    {
        printf("Status: %d Recv-Length: %d\n", res->status, res->content_recv);
    }

    free(priv);
    aeDeleteFileEvent(el, fd, AE_READABLE);
    close(fd);
    aeStop(el);
}


void send_request(aeEventLoop *el, int fd, void *priv, int mask)
{
    int n;
    char *request = "GET /index.html HTTP/1.1\r\n";
    char *headers = "Host: archlinux\r\n"
                    "Content-Length: 0\r\n"
                    "\r\n";
    n = send(fd, request, strlen(request), 0);
    if (n < 0)
    {
        perror("send");
        return;
    }
    n = send(fd, headers, strlen(headers), 0);
    if (n < 0)
    {
        perror("send");
        return;
    }

    struct response *res;
    res = malloc(sizeof(*res));
    res->buf_offset = 0;
    res->read_header = true;


    aeDeleteFileEvent(el, fd, AE_WRITABLE);
    aeCreateFileEvent(el, fd, AE_READABLE, recv_response, res);
}

int main(int argc, char **argv)
{
    int ch;
    while ((ch = getopt(argc, argv, "o:h:p:m:")) != -1) {
        switch (ch) {
            case 'h':
                printf("host is %s\n", optarg);
                strncpy(server_host, optarg, 15);
                break;
            case 'p':
                printf("port is %s\n", optarg);
                server_port = atoi(optarg);
                /*strncpy(server_ip, optarg, 15);*/
                break;
        }
    }

    el = aeCreateEventLoop(129);
    int fd =  http_connect();
    aeCreateFileEvent(el, fd, AE_WRITABLE, send_request, NULL);
    aeMain(el);
}
