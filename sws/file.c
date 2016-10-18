/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:06:22
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "sws.h"

struct sws_filebuf *sws_fileread(const char *path)
{
    struct stat st;
    struct sws_filebuf *buf;

    int f = open(path, O_RDONLY);
    if (-1 == f){
        seterr("open: %s", strerror(errno));
        return NULL;
    }

    if(-1 == fstat(f, &st))
    {
        seterr("stat: %s", strerror(errno));
        return NULL;
    }

    buf = malloc(st.st_size + sizeof(struct sws_filebuf) + 2);

    buf->size = read(f, buf->buf, st.st_size);
    buf->buf[buf->size] = 0;

    return buf;
}
