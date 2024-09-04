# Nginx Pathological

An Nginx module that crafts bad HTTP responses.

## Installation

Clone the repo and submodules:

```
git clone --recurse-submodules https://github.com/steve-chavez/ngx_pathological
```

Compile the module:

```
cd /path/to/nginx-source
./configure --add-module=/path/to/ngx_pathological
make
sudo make install
```

## Usage

Add a local directory for nginx:

```
mkdir -p nginx/conf
```

Add nginx config:

```
echo nginx/conf/nginx.conf <<EOF
daemon off;
pid        ./nginx.pid;

events {}

http {

  access_log /dev/stdout;

  server {
    listen 8080;

    location /pathological {
      pathological;
    }
  }
}
EOF
```

Start nginx:

```
$ nginx -p nginx -e stderr
```

### Setting status

By setting an integer on the `status` query string parameter, you can set a custom status code.

```
$ curl 'localhost:8080/pathological?status=204' -i
HTTP/1.1 204 No Content
```

### Malformed header

By setting a value on the `malformed-header` query string parameter, you can elicit responses with malformed headers. The allowed values are:

- spaces-in-header-name
- trailing-whitespace
- header-injection
- non-printable-chars
- missing-value
- multiple-colons
- invalid-encoding
- oversized-headers
- duplicate-headers
- invalid-characters

Examples:

```
$ curl 'localhost:8080/pathological?malformed-header=header-injection' -i

HTTP/1.1 200 OK
...
HeaderInjection
Injected-Header: This header contains an injection
```

```
$ curl 'localhost:8080/pathological?malformed-header=missing-value' -i

HTTP/1.1 200 OK
...
MissingValue:
```

### Response Delay

By setting an integer on the `delay` query string parameter, you can delay the response.

```
$ curl 'localhost:8080/pathological?delay=3' -i

## After 3 seconds...

HTTP/1.1 200 OK
```

### Disconnect Request

By setting `true` on the `disconnect` query string parameter, you can immediately cause a disconnection.

```
$ curl "localhost:8080/pathological?disconnect=true"
curl: (52) Empty reply from server
```

Setting it to `false` makes the request proceed. A different value will give an error.

## Development

[Nix](https://nixos.org/download/) is used for providing dependencies.

```bash
nix-shell

# to run the tests
make test

# to run a local nginx server on port 8080
make start
```
