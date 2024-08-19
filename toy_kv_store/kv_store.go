package main

import (
	"database/sql"
	"hash/crc32"
	"log"
	"math/rand"
	"net/http"
	"os"
	"time"

	"github.com/gin-gonic/gin"
	_ "github.com/lib/pq"
)

// KeyValue represents the key-value structure.
type KeyValue struct {
	Key    string    `json:"key"`
	Value  string    `json:"value"`
	Expiry time.Time `json:"expiry"`
}

type KVStore interface {
	GetExpiryTime() time.Duration
	GetMasterInstance(key string) *sql.DB
	GetReplicaInstance(masterIndex int) *sql.DB
	PurgeExpiredEntries()
}

type kvStoreImpl struct {
	connections   []*sql.DB
	replicaMap    map[int][]*sql.DB
	expiryTime    time.Duration
	purgeInterval time.Duration
}

func NewKVStore() KVStore {
	db1, err := sql.Open("postgres", "host=localhost port=5434 user=testuser password=Password123! dbname=key_value_db sslmode=disable")
	if err != nil {
		log.Fatalf("Failed to connect to master db1: %v", err)
	}

	db2, err := sql.Open("postgres", "host=localhost port=5436 user=testuser password=Password123! dbname=key_value_db sslmode=disable")
	if err != nil {
		log.Fatalf("Failed to connect to master db2: %v", err)
	}

	// Initialize database connections for replicas
	replica1, err := sql.Open("postgres", "host=localhost port=5435 user=testuser password=Password123! dbname=key_value_db sslmode=disable")
	if err != nil {
		log.Fatalf("Failed to connect to replica1 for master db1: %v", err)
	}

	replica2, err := sql.Open("postgres", "host=localhost port=5437 user=testuser password=Password123! dbname=key_value_db sslmode=disable")
	if err != nil {
		log.Fatalf("Failed to connect to replica2 for master db2: %v", err)
	}

	// Default expiry time
	expiryTime := 60 * time.Second
	if duration, exists := os.LookupEnv("EXPIRY_TIME"); exists {
		if parsedDuration, err := time.ParseDuration(duration); err == nil {
			expiryTime = parsedDuration
		}
	}

	// Default purge interval
	purgeInterval := 10 * time.Minute
	if duration, exists := os.LookupEnv("PURGE_INTERVAL"); exists {
		if parsedDuration, err := time.ParseDuration(duration); err == nil {
			purgeInterval = parsedDuration
		}
	}

	store := &kvStoreImpl{
		connections: []*sql.DB{db1, db2},
		replicaMap: map[int][]*sql.DB{
			0: {replica1},
			1: {replica2},
		},
		expiryTime:    expiryTime,
		purgeInterval: purgeInterval,
	}

	go store.runPurgeTask()

	return store
}

func (k *kvStoreImpl) getShardIndex(key string) int {
	hash := crc32.ChecksumIEEE([]byte(key))
	shardIndex := int(hash % uint32(len(k.connections)))
	return shardIndex
}

func (k *kvStoreImpl) GetMasterInstance(key string) *sql.DB {
	shardIndex := k.getShardIndex(key)
	log.Printf("Writing at master index %d", shardIndex)
	return k.connections[shardIndex]
}

func (k *kvStoreImpl) GetReplicaInstance(masterIndex int) *sql.DB {
	replicas := k.replicaMap[masterIndex]
	if len(replicas) == 0 {
		// If no replicas are available, fall back to the master
		log.Printf("No replicas found for master index %d, falling back to master", masterIndex)
		return k.connections[masterIndex]
	}
	// Randomly pick a replica to distribute load
	selectedReplica := replicas[rand.Intn(len(replicas))]
	log.Printf("Using replica for master index: %d", masterIndex)
	return selectedReplica
}

func (k *kvStoreImpl) GetExpiryTime() time.Duration {
	return k.expiryTime
}

func (k *kvStoreImpl) PurgeExpiredEntries() {
	for _, db := range k.connections {
		query := `DELETE FROM kv_store WHERE expiry < NOW()`
		_, err := db.Exec(query)
		if err != nil {
			log.Printf("Error purging expired entries: %v", err)
		} else {
			log.Printf("Purged expired entries")
		}
	}
}

func (k *kvStoreImpl) runPurgeTask() {
	ticker := time.NewTicker(k.purgeInterval)
	defer ticker.Stop()

	for range ticker.C {
		k.PurgeExpiredEntries()
	}
}

func getValue(kvStore KVStore, c *gin.Context) {
	key := c.Param("key")
	var kv KeyValue
	consistent := c.DefaultQuery("consistent", "false") == "true"

	query := `SELECT "key", "value", "expiry" FROM kv_store WHERE "key"=$1 AND "expiry" > NOW()`
	var db *sql.DB

	if consistent {
		db = kvStore.GetMasterInstance(key)
		log.Printf("Using master for consistent read.")
	} else {
		shardIndex := kvStore.(*kvStoreImpl).getShardIndex(key)
		db = kvStore.GetReplicaInstance(shardIndex)
	}

	err := db.QueryRow(query, key).Scan(&kv.Key, &kv.Value, &kv.Expiry)
	if err == sql.ErrNoRows {
		log.Printf("Error fetching value: %v", err)
		c.JSON(http.StatusNotFound, gin.H{"error": "Key not found or expired"})
		return
	} else if err != nil {
		log.Printf("Error querying database: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Database query failed", "details": err.Error()})
		return
	}

	c.JSON(http.StatusOK, kv.Value)
}

func putValue(kvStore KVStore, c *gin.Context) {
	var kv KeyValue
	if err := c.ShouldBindJSON(&kv); err != nil {
		log.Printf("Error parsing JSON: %v", err)
		c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid JSON data", "details": err.Error()})
		return
	}

	kv.Expiry = time.Now().Add(kvStore.GetExpiryTime())

	query := `
		INSERT INTO kv_store ("key", "value", "expiry")
		VALUES ($1, $2, $3)
		ON CONFLICT ("key") DO UPDATE SET "value" = EXCLUDED.value, "expiry" = EXCLUDED.expiry
	`

	dbInstance := kvStore.GetMasterInstance(kv.Key)
	_, err := dbInstance.Exec(query, kv.Key, kv.Value, kv.Expiry)
	if err != nil {
		log.Printf("Error executing query: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to upsert value", "details": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "Value upserted successfully"})
}

func deleteValue(kvStore KVStore, c *gin.Context) {
	key := c.Param("key")

	query := `UPDATE kv_store SET "expiry" = '-infinity' WHERE "key" = $1 AND "expiry" > NOW()`
	dbInstance := kvStore.GetMasterInstance(key)
	result, err := dbInstance.Exec(query, key)
	if err != nil {
		log.Printf("Error executing delete query: %v", err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to delete value", "details": err.Error()})
		return
	}

	rowsAffected, _ := result.RowsAffected()
	if rowsAffected == 0 {
		log.Printf("Key not found or already expired: %v", key)
		c.JSON(http.StatusNotFound, gin.H{"error": "Key not found or already expired"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "Key marked as expired"})
}

func main() {
	r := gin.Default()
	kvStore := NewKVStore()

	r.GET("/get/:key", func(c *gin.Context) { getValue(kvStore, c) })
	r.PUT("/put", func(c *gin.Context) { putValue(kvStore, c) })
	r.DELETE("/delete/:key", func(c *gin.Context) { deleteValue(kvStore, c) })

	if err := r.Run(":8080"); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
