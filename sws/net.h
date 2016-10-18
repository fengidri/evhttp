/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:09:04
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __NET_H__
#define __NET_H__
#include <fcntl.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

int sws_net_noblock(int fd, bool b);
int sws_net_recv(int fd, char *buf, size_t len);
int sws_net_connect(const char *addr, int port, bool noblock);
int sws_net_client_port(int fd);
bool sws_net_resolve(const char *addr, char *buf, size_t size);
int sws_net_server(const char *addr, int port, bool noblock, int backlog);
#endif


