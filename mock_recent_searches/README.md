
# Recent Search Query Application

This application stores the 10 most recent searches in Redis and indexes search queries with metadata in Elasticsearch. It provides API endpoints for performing searches, retrieving recent queries, getting all queries from Elasticsearch, and deleting all entries from Elasticsearch.

## Prerequisites

- **Go** (Golang)
- **Redis**
- **Elasticsearch**

## Setup

### 1. Install and Setup Elasticsearch

```bash
curl -fsSL https://artifacts.elastic.co/GPG-KEY-elasticsearch | sudo gpg --dearmor -o /usr/share/keyrings/elasticsearch-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/elasticsearch-keyring.gpg] https://artifacts.elastic.co/packages/8.x/apt stable main" | sudo tee /etc/apt/sources.list.d/elastic-8.x.list
sudo apt update && sudo apt install elasticsearch
sudo systemctl enable elasticsearch
sudo systemctl start elasticsearch
```

Verify Elasticsearch:
```bash
curl -u elastic:<password> -k https://localhost:9200
```

### 2. Install Redis

```bash
sudo apt update
sudo apt install redis-server
sudo systemctl enable redis-server
sudo systemctl start redis-server
```

### 3. Set Elasticsearch Credentials

```bash
export ELASTIC_USER="elastic"
export ELASTIC_PASSWORD=$ELASTIC_PASSWORD
```

### 4. Run the Application

```bash
go run main.go
```

## API Endpoints

- `/search` --> Perform a search with user-id and search query
- `/get_recent_search_queries` --> Gives you recent searches
- `/get_all_search_queries` --> Gives you all search queries for a given user id
- `/delete_elastic_search` --> Cleanup
  
**Examples**:
```bash
curl "http://localhost:8080/search?q=iphone&user_id=2"
curl "http://localhost:8080/get_recent_search_queries?user_id=2"
curl "http://localhost:8080/get_all_search_queries?user_id=2"
curl -X DELETE "http://localhost:8080/delete_elastic_search"
```

