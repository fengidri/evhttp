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
    bool eof;
};


int http_new();
void http_destory(struct response *);

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
    if (200 == res->status)
    {
        config.sum_status_200++;
    }
    else
        config.sum_status_other++;

    if (config.debug)
        printf("%.*s", length, res->buf);

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



int recv_header(int fd, struct response *res)
{
    int n;
    while (1)
    {
        n = ev_recv(fd, res->buf + res->buf_offset,
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
check:
    if (res->content_length > -1)
    {
        if (res->content_recv >= res->content_length)
        {
            return EV_OK;
        }
        else{
            if (res->eof)
            {
                logerr("Server close connection prematurely!!")
                return EV_OK;
            }
        }
    }
    else{
        if (res->eof)
            return EV_OK;
    }

    n = ev_recv(fd, res->buf, sizeof(res->buf));
    if (n < 0) return n;
    if (n == 0) res->eof = true;

    res->content_recv += n;
    goto check;

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

            n = ev_recv(fd, res->buf + res->buf_offset,
                    sizeof(res->buf) - res->buf_offset);

            if (n < 0) return n;
            if (0 == n) return EV_ERR;
            res->buf_offset += n;
        }
    }

chunk:
    if (res->chunk_recv - res->chunk_length == 2)
    {
        res->chunk_length = -1;
        res->content_recv -= 2;
        res->buf_offset = 0;
        return recv_is_chunked(fd, res);
    }

    size = MIN(sizeof(res->buf), res->chunk_length + 2 - res->chunk_recv);
    n = ev_recv(fd, res->buf, size);
    if (n < 0) return n;
    if (n == 0) res->eof = true;
    res->content_recv += n;
    res->chunk_recv += n;
    goto chunk;

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
        logerr("Recv Response Error!!!\n");
    }

    if (EV_OK == ret && !config.sum)// just output when not sum
    {
        printf("Status: %d Recv: %d URL: %s\n",
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
    n = snprintf(res->buf, sizeof(res->buf), request, res->url, config.http_host);
    if (n >= sizeof(res->buf))
    {
        http_destory(res);
        return ;
    }
    if (config.debug)
        printf("%s", res->buf);

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
    int fd =  http_connect();
    struct response *res;

    if (fd < 0){
        return EV_ERR;
    }

    res = malloc(sizeof(*res));
    res->buf_offset = 0;
    res->read_header = true;
    make_url(res->url, sizeof(res->url));
    res->fd = fd;
    res->eof = 0;

    aeCreateFileEvent(config.el, fd, AE_WRITABLE, send_request, res);

    config.active += 1;
    return EV_OK;
}

void http_destory(struct response *res)
{
    aeDeleteFileEvent(config.el, res->fd, AE_READABLE);
    aeDeleteFileEvent(config.el, res->fd, AE_WRITABLE);
    close(res->fd);
    free(res);

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

