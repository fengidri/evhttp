# evhttp
http client with ae event lib for analytics or benchmark.

# work mode
evhttp support three work mode: URLS, FILE, RANDOM.
* URLS: get the urls from the command line
```
evhttp test.com/test
```

* FILE: get the urls from the file specialed by -F
```
evhttp -F urls
```

* RANDOME: use the opt -R to work as RANDOME. This mode will request use randome
url.

```
evhttp -f T -R -h 127.0.0.1
```



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



## output format

### -w
You can choose the value to be output by -w.

'-w' special the format to print the value. like this:
```
-w 'age: $header.res.age$ DNS: $time.dns$ CON: $time.con$ RES: $time.res$ MAXREAD: $time.max_read$, TRANS: $time.trans$ Total: $time.total$'
```

will print this:
```
age: 164956 DNS: 0.000 CON: 0.012 RES: 0.016 MAXREAD: 0.005, TRANS: 0.125 Total: 0.142
```

There are many value can be used:

* header.res.[field]
* time.dns
* time.con
* time.res
* time.trans
* time.total
* time.max\_read
* info.status
* info.index
* info.lport
* info.recv
* info.speed


### -W
you can use the -W to special file as the fmt.
like this:
```
------------------------ Header -------------------------------
Via: $header.res.via$
Age: $header.res.age$
------------------------ Time -------------------------------
DNS: $time.dns$
CON: $time.con$
TOTAL: $time.total$
```

This output like this:

```
------------------------ Header -------------------------------
Via: nginx server
Age: 171865
------------------------ Time -------------------------------
DNS: 0.000
CON: 0.012
TOTAL: 0.190
```


