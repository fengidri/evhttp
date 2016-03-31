/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:09:04
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __NET_H__
#define __NET_H__
int sws_net_noblock(int fd, bool b);
int sws_net_recv(int fd, char *buf, size_t len);
int sws_net_connect(const char *addr, int port);
int sws_net_client_port(int fd);
bool sws_net_resolve(const char *addr, char *buf, size_t size);

#endif


