/**
 *   author       :   丁雪峰
 *   time         :   2016-03-30 02:37:23
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __FORMAT_H__
#define __FORMAT_H__
int format_compile(struct config *config);
void format_handle(struct config *config, struct http *h);
void format_destroy(struct config *config);

#endif


