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

int sws_fileread(const char *path, char **buf, size_t *size)
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
