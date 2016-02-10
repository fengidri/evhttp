# evhttp
http client with ae event lib for analytics or benchmark.


# command line args
## remote
The is options for the global remote info.
* -d: set the domain for http host and the server ip. This also can be set by
  ip.
* -i: set the remote ip. If not set, then set this by resolved the domain.
* -p: set the remote port. Default 80.

## global
* -n: set the total request. 0 means not limit. Default is 1.
* -x: set the parallel num
* -m: set the default request method. Default is "GET".
* -r: set the recycle limit. when tagert the recycle limit. the url will
  restart. the arg of this is like 1000n, 1000k, 1000m.
  the last char set the type of recycle.
     * n: when request times target the limit.
     * b,k,m,g,t: is the total bytes recved by net.

## random
* -f: set the flag for random url;

## debug
* -v: debug level, print the request and response header and etc;
* -s: sum level, print the sum info every 1s

# URL
if not special url, then evhttp will use random url.

## random url
The random url like to:

```
/<flag><num>/<random>
```

`flag` can be set by -f. `num` will differ in defferent forked process.

The default host is 127.0.0.1. And default addr and port is 127.0.0.1 and 80;
Can be set by -h, -p, -H.

## special url
The url must include the domain name and the url. So the http\_host will use the
domain name. Url may include the port, if not use the default port seted by -p.
The remote ip will use the default ip seted by -h. If not will try to resolved.
So you can not set different ip for every domain in command line.



