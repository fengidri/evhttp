## evhttp
http client with ae event lib.

make http request to remote\_addr:remote\_port

```
GET /<flag><num>/<random> HTTP/1.1
Host: <http_host>
Content-Length: 0
User-Agent: evhttp
Accept: */*
```

support respose with chuned or with content-length or without content-length.


# opt
## remote
The is options for the global remote info.
* -d: set the domain for http host and the server ip. This also can be set by
  ip.
* -i: set the remote ip. If not set, then set this by resolved the domain.
* -p: set the remote port. Default 80.

## global
* -l: set the parallel num
* -t: set the total request. 0 means not limit. Default is 1.
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



