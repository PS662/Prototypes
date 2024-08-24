package main

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
	"path/filepath"

	"github.com/gin-gonic/gin"
)

var originMap = map[string]string{
	"localhost:8080": "https://www.google.com",
}

var cacheDir = "./cache"

func proxy(c *gin.Context) {
	originBase, ok := originMap[c.Request.Host]
	if !ok {
		c.JSON(http.StatusNotFound, gin.H{"error": "origin not found"})
		return
	}

	originURL := originBase + c.Request.RequestURI
	cacheKey := generateCacheKey(originURL)

	if cachedResponse, found := cacheGet(cacheKey); found {
		log.Println(colorize("Cache hit for: "+originURL, "blue"))
		c.Data(http.StatusOK, c.GetHeader("Content-Type"), cachedResponse)
		return
	}

	log.Printf(colorize("Proxying and caching: %s -> %s", "red"), c.Request.URL.String(), originURL)

	resp, err := http.Get(originURL)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to proxy request"})
		return
	}
	defer resp.Body.Close()

	if resp.StatusCode == http.StatusOK {
		bodyBytes, _ := io.ReadAll(resp.Body)
		cachePut(cacheKey, bodyBytes)

		// Forward the correct Content-Type header
		contentType := resp.Header.Get("Content-Type")
		c.Data(http.StatusOK, contentType, bodyBytes)
	} else {
		c.Status(resp.StatusCode)
		io.Copy(c.Writer, resp.Body)
	}
}

func colorize(text string, color string) string {
	colors := map[string]string{
		"red":   "\033[31m",
		"blue":  "\033[34m",
		"reset": "\033[0m",
	}
	return fmt.Sprintf("%s%s%s", colors[color], text, colors["reset"])
}

func cacheGet(key string) ([]byte, bool) {
	filePath := filepath.Join(cacheDir, key)
	data, err := os.ReadFile(filePath)
	if err != nil {
		return nil, false
	}
	return data, true
}

func cachePut(key string, data []byte) {
	filePath := filepath.Join(cacheDir, key)
	err := os.WriteFile(filePath, data, 0644)
	if err != nil {
		log.Println(colorize("Failed to write cache file: "+err.Error(), "red"))
	}
}

func generateCacheKey(url string) string {
	hash := sha256.Sum256([]byte(url))
	return hex.EncodeToString(hash[:])
}

func main() {
	if _, err := os.Stat(cacheDir); os.IsNotExist(err) {
		os.Mkdir(cacheDir, 0755)
	}
	r := gin.Default()

	// Define routes
	r.Any("/*proxyPath", proxy)

	// Start the server
	log.Println(colorize("Starting server on :8080", "blue"))
	log.Fatal(r.Run(":8080"))
}
