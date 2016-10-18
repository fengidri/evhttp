/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:02:30
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <strings.h>

#include "sws.h"

int sws_net_noblock(int fd, bool b)
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

bool sws_net_resolve(const char *addr, char *buf, size_t size)
{
    // resolve
    struct hostent *hptr;
    hptr = gethostbyname(addr);

    if (NULL == hptr)
    {
        seterr("gethostbyname error: %s", addr);
        return false;
    }

    inet_ntop(hptr->h_addrtype, *hptr->h_addr_list, buf, size);
    return true;
}


int sws_net_server(const char *addr, int port, bool noblock, int backlog)
{
    int s;
    int flag = 1;

    s = socket(AF_INET,  SOCK_STREAM, 0);
    if (s < 0)
    {
        seterr("bind error: create socket: %s", strerror(errno));
        return -1;
    }

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        seterr("connect error: set SO_REUSEADDR fail %s", strerror(errno));
        close(s);
        return -1;
    }

    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(addr);
    remote_addr.sin_port = htons(port);

    if(bind(s, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr)))
    {
        seterr("bind error: %s", strerror(errno));
        close(s);
        return -1;
    }

    if (listen(s, backlog))
    {
        seterr("%s\n", strerror(errno));
        return -1;
    }

    if (noblock && sws_net_noblock(s, true) < 0)
    {
        seterr("connect error: set noblock: %s", strerror(errno));
        return -1;
    }

    return s;
}



int sws_net_connect(const char *addr, int port, bool noblock)
{
    int s;
    s = socket(AF_INET,  SOCK_STREAM, 0);
    if (s < 0)
    {
        seterr("connect error: create socket: %s", strerror(errno));
        return -1;
    }
    if (noblock && sws_net_noblock(s, true) < 0)
    {
        seterr("connect error: set noblock: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in remote_addr; //服务器端网络地址结构体
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family = AF_INET; //设置为IP通信
    remote_addr.sin_addr.s_addr = inet_addr(addr);//服务器IP地址
    remote_addr.sin_port = htons(port); //服务器端口号

    int ret = connect(s, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
    while(ret < 0) {
        if( errno == EINPROGRESS ) {
            break;
        }  else {
            seterr("connect error: connect fail: %s", strerror(errno));
            return -1;
        }
    }

    return s;
}

int sws_net_recv(int fd, char *buf, size_t len)
{
    int n, err;

    n = recv(fd, buf, len, 0);
    err = errno; // save off errno, because because the printf statement might reset it
    if (n < 0)
    {
        if ((err == EAGAIN) || (err == EWOULDBLOCK))
        {
            return SWS_AG;
        }
        else
        {
            return SWS_ERR;
        }
    }
    return n;
}

int sws_net_client_port(int fd)
{
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);

    getsockname(fd, (struct sockaddr *)&c, &cLen);
    return ntohs(c.sin_port);
}

int sws_net_info(int fd, bool local, int *port, char **ip, size_t size)
{
    struct sockaddr_in c;
    int rc;

    socklen_t cLen = sizeof(c);

    if (local)
        rc = getsockname(fd, (struct sockaddr *)&c, &cLen);
    else
        rc = getpeername(fd, (struct sockaddr *)&c, &cLen);

    if (-1 == rc) return -1;

    if (port)
        *port = ntohs(c.sin_port);

    //if (ip)
    //    ip =

    return 0;
}
