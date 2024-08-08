package main

import (
	"database/sql"
	"fmt"
	"sync"
	"time"

	_ "github.com/go-sql-driver/mysql"
)

const dsn = "testuser:Password123!@tcp(localhost:3306)/prototype_test_db"

type ConnectionPool interface {
	Get() (*sql.DB, error)
	Put(*sql.DB)
	Close()
}

type cpool struct {
	conns   []*sql.DB
	channel chan struct{}
	mu      sync.Mutex
}

// NewPool creates a new connection pool
func NewPool(size int) (*cpool, error) {
	pool := &cpool{
		conns:   make([]*sql.DB, 0, size),
		channel: make(chan struct{}, size),
	}
	for i := 0; i < size; i++ {
		conn, err := createNewConnection()
		if err != nil {
			return nil, err
		}
		pool.conns = append(pool.conns, conn)
		pool.channel <- struct{}{} // Signal initial availability
	}
	return pool, nil
}

// Get acquires a connection from the pool
func (pool *cpool) Get() (*sql.DB, error) {
	<-pool.channel // Wait for availability
	pool.mu.Lock()
	conn := pool.conns[0]
	pool.conns = pool.conns[1:] // Remove it from the slice
	pool.mu.Unlock()
	return conn, nil
}

// Put returns a connection to the pool
func (pool *cpool) Put(conn *sql.DB) {
	pool.mu.Lock()
	pool.conns = append(pool.conns, conn) // Add connection back to the slice
	pool.mu.Unlock()
	pool.channel <- struct{}{} // Signal that a connection is available
}

// Close shuts down the pool and closes all connections
func (pool *cpool) Close() {
	pool.mu.Lock()
	defer pool.mu.Unlock()
	for _, conn := range pool.conns {
		conn.Close()
	}
	pool.conns = nil // Clear the slice of connections
}

func createNewConnection() (*sql.DB, error) {
	db, err := sql.Open("mysql", dsn)
	if err != nil {
		return nil, err
	}
	if err = db.Ping(); err != nil {
		return nil, err
	}
	return db, nil
}

func simulateDatabaseOperation(db *sql.DB) error {
	_, err := db.Exec("SELECT SLEEP(0.01);")
	return err
}

func BenchmarkNonPool(nThreads int) (time.Duration, error) {
	start := time.Now()
	var wg sync.WaitGroup
	var once sync.Once
	var operationError error

	for i := 0; i < nThreads; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			conn, err := createNewConnection()
			if err != nil {
				once.Do(func() {
					operationError = err
				})
				return
			}
			defer conn.Close()
			if err := simulateDatabaseOperation(conn); err != nil {
				once.Do(func() {
					operationError = err
				})
			}
		}()
	}
	wg.Wait()
	return time.Since(start), operationError
}

func BenchmarkPool(nThreads int) (time.Duration, error) {
	var pool ConnectionPool
	var err error
	pool, err = NewPool(10)
	if err != nil {
		return 0, fmt.Errorf("Error initializing connection pool: %v", err)
	}

	start := time.Now()
	var wg sync.WaitGroup
	var once sync.Once
	var operationError error

	for i := 0; i < nThreads; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			conn, err := pool.Get()
			if err != nil {
				once.Do(func() {
					operationError = err
				})
				return
			}
			if err := simulateDatabaseOperation(conn); err != nil {
				once.Do(func() {
					operationError = err
				})
			}
			pool.Put(conn)
		}()
	}
	wg.Wait()
	pool.Close() // Close all connections in the pool after benchmarking
	return time.Since(start), operationError
}

func main() {
	tests := []int{10, 100, 300, 500, 1000}
	fmt.Println("Starting non-pool benchmarks:")
	for _, n := range tests {
		elapsed, err := BenchmarkNonPool(n)
		if err != nil {
			fmt.Printf("Error running non-pool benchmark for %d threads: %v\n", n, err)
		} else {
			fmt.Printf("Non-pool time for %d threads: %v\n", n, elapsed)
		}
	}

	fmt.Println("\nStarting pool benchmarks:")
	for _, n := range tests {
		elapsed, err := BenchmarkPool(n)
		if err != nil {
			fmt.Printf("Error running pool benchmark for %d threads: %v\n", n, err)
		} else {
			fmt.Printf("Pool time for %d threads: %v\n", n, elapsed)
		}
	}
}
