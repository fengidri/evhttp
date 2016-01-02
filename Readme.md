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

* -l: set the parallel num
* -h: set the remote addr
* -p: set the remote port
* -t: set the total request. 0 means not limit. Default is 1.
* -f: set the flag
* -H: set the http host
* -r: set the recycle limit. when tagert the recycle limit. the url will
  restart. the arg of this is like 1000n, 1000k, 1000m.
  the last char set the type of recycle.
     * n: when request times target the limit.
     * b,k,m,g,t: is the total bytes recved by net.
* -v: print the request and response header;

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



