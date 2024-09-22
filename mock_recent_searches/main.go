package main

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/elastic/go-elasticsearch/v8"
	"github.com/elastic/go-elasticsearch/v8/esapi"
	"github.com/gin-gonic/gin"
	"github.com/go-redis/redis/v8"
)

var ctx = context.Background()
var redisClient = redis.NewClient(&redis.Options{
	Addr: "localhost:6379",
	DB:   0, // Use default DB
})

var es *elasticsearch.Client

var dummySearchResults = map[string][]string{
	"iphone":      {"iPhone 14 Pro", "iPhone 13 Mini", "iPhone SE"},
	"macbook":     {"MacBook Air M2", "MacBook Pro 14-inch", "MacBook Pro 16-inch"},
	"ipad":        {"iPad Pro 12.9", "iPad Mini", "iPad Air"},
	"apple watch": {"Apple Watch Series 8", "Apple Watch SE", "Apple Watch Ultra"},
	"airpods":     {"AirPods Pro", "AirPods Max", "AirPods 3rd Generation"},
}

type SearchData struct {
	Timestamp  time.Time `json:"ts"`
	UserID     string    `json:"userid"`
	Query      string    `json:"query"`
	Cohort     string    `json:"cohort"`
	Region     string    `json:"region"`
	OS         string    `json:"os"`
	Browser    string    `json:"browser"`
	NumResults int       `json:"num_results"`
}

func initElasticsearch() {
	username := os.Getenv("ELASTIC_USER")
	password := os.Getenv("ELASTIC_PASSWORD")

	if username == "" || password == "" {
		log.Fatalf("Error: ELASTIC_USER or ELASTIC_PASSWORD environment variables not set")
	}

	cfg := elasticsearch.Config{
		Addresses: []string{
			"https://localhost:9200",
		},
		Username: username,
		Password: password,
		Transport: &http.Transport{
			TLSClientConfig: &tls.Config{
				InsecureSkipVerify: true,
			},
		},
	}

	var err error
	es, err = elasticsearch.NewClient(cfg)
	if err != nil {
		log.Fatalf("Error creating the Elasticsearch client: %s", err)
	}

	log.Println("Connected to Elasticsearch.")
}

func addSearchToRedis(userID string, query string) error {
	key := fmt.Sprintf("recent_searches:%s", userID)
	timestamp := float64(time.Now().Unix())

	// Add the search query with the current timestamp
	_, err := redisClient.ZAdd(ctx, key, &redis.Z{
		Score:  timestamp,
		Member: query,
	}).Result()

	if err != nil {
		return err
	}

	_, err = redisClient.ZRemRangeByRank(ctx, key, 0, -11).Result()
	return err
}

func getCohort(userID string) string {
	idNum, _ := strconv.Atoi(userID)
	if idNum%2 == 0 {
		return "beta_user"
	}
	return "normal_user"
}

func getSearchResults(query string) ([]string, int) {
	results, exists := dummySearchResults[strings.ToLower(query)]
	if exists {
		return results, len(results)
	}
	return []string{}, 0
}

func addSearchToElasticsearch(userID string, query string, numResults int) error {
	timestamp := time.Now()

	// Create the document to index
	doc := SearchData{
		Timestamp:  timestamp,
		UserID:     userID,
		Query:      query,
		Cohort:     getCohort(userID),
		Region:     "asia-se",
		OS:         "linux",
		Browser:    "firefox",
		NumResults: numResults,
	}

	data, err := json.Marshal(doc)
	if err != nil {
		return err
	}

	req := esapi.IndexRequest{
		Index:      "search_queries",
		DocumentID: fmt.Sprintf("%s-%d", userID, time.Now().UnixNano()), // unique doc id
		Body:       bytes.NewReader(data),
		Refresh:    "true",
	}
	res, err := req.Do(ctx, es)
	if err != nil {
		return fmt.Errorf("error: executing elasticsearch request: %v", err)
	}
	defer res.Body.Close()

	if res.IsError() {
		var errorBody map[string]interface{}
		if err := json.NewDecoder(res.Body).Decode(&errorBody); err != nil {
			return fmt.Errorf("error: reading elasticsearch response: %v", err)
		}
		return fmt.Errorf("error indexing document in elasticsearch: %v", errorBody)
	}

	return nil
}

func searchHandler(c *gin.Context) {
	query := c.Query("q")
	userID := c.Query("user_id")

	if query == "" || userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Missing query or user_id"})
		return
	}

	results, numResults := getSearchResults(query)

	err := addSearchToRedis(userID, query)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to add search to Redis"})
		return
	}

	err = addSearchToElasticsearch(userID, query, numResults)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": fmt.Sprintf("Failed to index search in Elasticsearch: %v", err),
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"query":   query,
		"results": results,
	})
}

func getRecentSearchQueriesHandler(c *gin.Context) {
	userID := c.Query("user_id")

	if userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Missing user_id"})
		return
	}

	key := fmt.Sprintf("recent_searches:%s", userID)

	recentSearches, err := redisClient.ZRevRange(ctx, key, 0, 9).Result()
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to retrieve recent searches"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"recent_searches": recentSearches})
}

func getAllSearchQueriesHandler(c *gin.Context) {
	userID := c.Query("user_id")

	if userID == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Missing user_id"})
		return
	}

	// Build Elasticsearch query
	var buf bytes.Buffer
	searchQuery := map[string]interface{}{
		"query": map[string]interface{}{
			"match": map[string]interface{}{
				"userid": userID,
			},
		},
	}
	if err := json.NewEncoder(&buf).Encode(searchQuery); err != nil {
		log.Fatalf("Error encoding search query: %s", err)
	}

	res, err := es.Search(
		es.Search.WithContext(ctx),
		es.Search.WithIndex("search_queries"),
		es.Search.WithBody(&buf),
		es.Search.WithTrackTotalHits(true),
	)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to query Elasticsearch"})
		return
	}
	defer res.Body.Close()

	if res.IsError() {
		c.JSON(http.StatusInternalServerError, gin.H{"error": fmt.Sprintf("Error querying Elasticsearch: %s", res.String())})
		return
	}

	// Decode the search results
	var searchResult map[string]interface{}
	if err := json.NewDecoder(res.Body).Decode(&searchResult); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to decode Elasticsearch response"})
		return
	}

	c.JSON(http.StatusOK, searchResult)
}

func deleteAllSearchQueriesHandler(c *gin.Context) {
	var buf bytes.Buffer
	deleteQuery := map[string]interface{}{
		"query": map[string]interface{}{
			"match_all": map[string]interface{}{},
		},
	}
	if err := json.NewEncoder(&buf).Encode(deleteQuery); err != nil {
		log.Fatalf("Error encoding delete query: %s", err)
	}

	res, err := es.DeleteByQuery(
		[]string{"search_queries"}, // Index name
		&buf,                       // Query body
		es.DeleteByQuery.WithContext(ctx),
		es.DeleteByQuery.WithRefresh(true), // Immediate refresh
	)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to delete search queries from Elasticsearch"})
		return
	}
	defer res.Body.Close()

	if res.IsError() {
		c.JSON(http.StatusInternalServerError, gin.H{"error": fmt.Sprintf("Error deleting documents from Elasticsearch: %s", res.String())})
		return
	}

	// Return success message
	c.JSON(http.StatusOK, gin.H{"message": "Successfully deleted all search queries from Elasticsearch"})
}

func main() {
	initElasticsearch()

	r := gin.Default()

	r.GET("/search", searchHandler)
	r.GET("/get_recent_search_queries", getRecentSearchQueriesHandler)
	r.GET("/get_all_search_queries", getAllSearchQueriesHandler)
	r.DELETE("/delete_elastic_search", deleteAllSearchQueriesHandler)

	r.Run(":8080")
}
