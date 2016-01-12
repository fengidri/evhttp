/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 22:43:02
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"

char *strnstr(const char *s1, const char *s2, size_t len)
{
    size_t l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;

    while (len >= l2) {
        len--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
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

bool net_resolve(const char *addr, char *buf, size_t size)
{
    // resolve
    struct hostent *hptr;
    hptr = gethostbyname(addr);

    if (NULL == hptr)
    {
        logerr("gethostbyname error: %s\n", addr);
        return false;
    }

    inet_ntop(hptr->h_addrtype, *hptr->h_addr_list, buf, size);
    return true;
}

int net_connect(const char *addr, int port)
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
    remote_addr.sin_addr.s_addr = inet_addr(addr);//服务器IP地址
    remote_addr.sin_port = htons(port); //服务器端口号

    int ret = connect(s, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
    while(ret < 0) {
        if( errno == EINPROGRESS ) {
            break;
        }  else {
            perror("connect fail");
            return -1;
        }
    }

    return s;
}


int net_send_fast(int *fd, const char *addr, int port, char *buf, size_t size)
{
    if (!*fd)
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

        *fd = s;
    }

    struct sockaddr_in remote_addr; //服务器端网络地址结构体
    memset(&remote_addr,0,sizeof(remote_addr)); //数据初始化--清零
    remote_addr.sin_family = AF_INET; //设置为IP通信
    remote_addr.sin_addr.s_addr = inet_addr(addr);//服务器IP地址
    remote_addr.sin_port = htons(port); //服务器端口号


    return sendto(*fd, buf, size, MSG_FASTOPEN,
            (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
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

int size_fmt(char *buf, size_t len, long long size)
{
    size_t p = 0;
    char *ps = " KMGT";
    double  s;
    int n;
    s = size;
    while(1)
    {
        if (s >= 1024)
        {
            s = s / 1024;
            p += 1;
            if (p >= strlen(ps) - 1) break;
        }
        else
            break;
    }

    return snprintf(buf, len, "%.3f%c", s, ps[p]);
}
