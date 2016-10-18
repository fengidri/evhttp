/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:08:44
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#ifndef  __FILE_H__
#define __FILE_H__

struct sws_filebuf{
    size_t size;
    char buf[];
};

struct sws_filebuf *sws_fileread(const char *path);

#endif


