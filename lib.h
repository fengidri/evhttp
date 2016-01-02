/**
 *   author       :   丁雪峰
 *   time         :   2016-01-01 22:45:30
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __LIB_H__
#define __LIB_H__

#define EV_OK  0
#define EV_ERR -2
#define EV_AG  -3

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#define logerr(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

char* strnstr(char* s1, char* s2, size_t size);
int size_fmt(char *buf, size_t len, long long size);

int net_noblock(int fd, bool b);
int net_recv(int fd, char *buf, size_t len);
int net_connect(const char *addr, int port);
bool net_resolve(const char *addr, char *buf, size_t size);

static inline void ev_strncpy(char *dest, const char *src, size_t size)
{
    strncpy(dest, src, size);
    dest[sizeof(size) - 1] = 0;
}


#endif


