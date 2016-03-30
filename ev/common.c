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
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <malloc.h>
#include <stdarg.h>

#include "common.h"
#include "strings.h"

char * errstr;
char errbuf[1024];

int fileread(const char *path, char **buf, size_t *size)
{
    struct stat st;

    int f = open(path, O_RDONLY);
    if (-1 == f){
        seterr("open: %s", strerror(errno));
        return -1;
    }

    if(-1 == fstat(f, &st))
    {
        seterr("stat: %s", strerror(errno));
        return -1;
    }

    *buf = malloc(st.st_size + 2);
    *size = read(f, *buf, st.st_size);

    (*buf)[*size] = 0;
    close(f);
    return 0;
}

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
        seterr("gethostbyname error: %s", addr);
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
        seterr("connect error: create socket: %s", strerror(errno));
        return -1;
    }

    if (net_noblock(s, true) < 0)
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

int net_client_port(int fd)
{
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);

    getsockname(fd, (struct sockaddr *)&c, &cLen);
    return ntohs(c.sin_port);
}

int net_info(int fd, bool local, int *port, char **ip, size_t size)
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

int size_fmt(char *buf, size_t len, double s)
{
    size_t p = 0;
    char *ps = " KMGT";
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

char *nskipspace(char *s, int n)
{
    char *ss;
    ss = s;
    while (ss - s < n)
    {
        if (*ss == ' ')
            ++ss;
        else
            return ss;
    }
    return NULL;
}
