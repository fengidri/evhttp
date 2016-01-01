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
* -h: set the remote add
* -p: set the remote port
* -t: set the total request. 0 means not limit. Default is 1.
* -f: set the flag
* -H: set the http host


