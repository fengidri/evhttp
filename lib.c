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

#include "lib.h"

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
