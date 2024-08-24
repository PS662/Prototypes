
# Mock CDN Proxy

This is a simple CDN proxy implementation using Go and the Gin web framework. It caches responses for faster subsequent requests.

## Setup

1. Initialize a Go module:
    ```bash
    go mod init cdn_mod
    ```

2. Install the Gin framework:
    ```bash
    go get -u github.com/gin-gonic/gin
    ```

3. Run the server:
    ```bash
    go run main.go
    ```

## Usage

You can test the CDN proxy using `curl`:

```bash
curl -H "Host: localhost:8080" http://localhost:8080/search?q=software
```

The first request proxies the request and caches the response. Subsequent requests will fetch the response from the cache.

You should see something similar to the following in your server logs:


```
[GIN-debug] Listening and serving HTTP on :8080
2024/08/25 02:25:51 Proxying and caching: /search?q=software -> https://www.google.com/search?q=software
[GIN] 2024/08/25 - 02:25:51 | 200 |  727.194752ms |       127.0.0.1 | GET      "/search?q=software"
2024/08/25 02:27:00 Cache hit for: https://www.google.com/search?q=software
[GIN] 2024/08/25 - 02:27:00 | 200 |    1.670646ms |       127.0.0.1 | GET      "/search?q=software"
```