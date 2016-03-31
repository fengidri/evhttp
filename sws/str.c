/**
 *   author       :   丁雪峰
 *   time         :   2016-03-31 02:07:53
 *   email        :   fengidri@yeah.net
 *   version      :   1.0.1
 *   description  :
 */
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
char *sws_strncpy(char *s1, const char *s2, size_t len)
{
    strncpy(s1, s2, len);
    s1[len - 1] = 0;
    return s1;
}

char *sws_strnstr(const char *s1, const char *s2, size_t len)
{
    size_t l2;

    l2 = strlen(s2);
    if (!l2)
        return (char *)s1;

    while (len >= l2) {
        len--;
        if (!memcmp(s1, s2, l2))
            return (char *)s1;
        s1++;
    }
    return NULL;
}

int sws_size_fmt(char *buf, size_t len, double s)
{
    size_t p = 0;
    char *ps = " KMGT";
    while(1)
    {
        if (s >= 1024)
        {
            s = s / 1024;
            p += 1;
            if (p >= strlen(ps) - 1) break;
        }
        else
            break;
    }

    return snprintf(buf, len, "%.3f%c", s, ps[p]);
}

char *sws_nskipspace(char *s, int n)
{
    char *ss;
    ss = s;
    while (ss - s < n)
    {
        if (*ss == ' ')
            ++ss;
        else
            return ss;
    }
    return NULL;
}
