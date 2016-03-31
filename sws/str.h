/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:11:36
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __STR_H__
#define __STR_H__
char *sws_strnstr(const char *s1, const char *s2, size_t len);
char *sws_nskipspace(char *s, int n);
int sws_size_fmt(char *buf, size_t len, double s);
char *sws_strncpy(char *s1, const char *s2, size_t len);

#endif


