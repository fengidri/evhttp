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

#define logerr(fmt, ...)

char* strnstr(char* s1, char* s2, size_t size);
int net_noblock(int fd, bool b);
int net_recv(int fd, char *buf, size_t len);


#endif


