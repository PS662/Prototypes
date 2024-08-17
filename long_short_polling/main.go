package main

import (
	"database/sql"
	"fmt"
	"log"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	_ "github.com/lib/pq"
)

var db *sql.DB

func initDB() {
	var err error
	connStr := "host=localhost port=5432 user=testuser password=Password123! dbname=testdb sslmode=disable"
	db, err = sql.Open("postgres", connStr)
	if err != nil {
		log.Fatal(err)
	}

	err = db.Ping()
	if err != nil {
		log.Fatal(err)
	}
}

func createEC2(serverID int) {
	fmt.Println("Creating server", serverID)
	updateServerStatus(serverID, "TODO")
	time.Sleep(5 * time.Second)

	updateServerStatus(serverID, "IN_PROGRESS")
	fmt.Println("Server creation in progress", serverID)
	time.Sleep(20 * time.Second)

	updateServerStatus(serverID, "DONE")
	fmt.Println("Server created", serverID)
}

func updateServerStatus(serverID int, status string) {
	_, err := db.Exec("UPDATE p_servers SET status = $1 WHERE server_id = $2", status, serverID)
	if err != nil {
		log.Fatal("Failed to update server status:", err)
	}
}

func main() {
	initDB()

	r := gin.Default()

	r.POST("/servers", func(c *gin.Context) {
		_, err := db.Exec("INSERT INTO p_servers(status) VALUES($1) RETURNING server_id", "TODO")
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to create server"})
			return
		}

		serverID := 0
		err = db.QueryRow("SELECT currval(pg_get_serial_sequence('p_servers', 'server_id'))").Scan(&serverID)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to retrieve server ID"})
			return
		}

		go createEC2(serverID)

		c.JSON(http.StatusOK, gin.H{"message": "Submitted server creation request", "server_id": serverID})
	})

	r.GET("/short/status/:server_id", func(c *gin.Context) {
		serverID := c.Param("server_id")

		var status string
		err := db.QueryRow("SELECT status FROM p_servers WHERE server_id = $1", serverID).Scan(&status)
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to retrieve server status"})
			return
		}

		c.JSON(http.StatusOK, gin.H{"status": status})
	})

	r.GET("/long/status/:server_id", func(c *gin.Context) {
		serverID := c.Param("server_id")
		currentStatus := c.DefaultQuery("status", "DONE")

		var status string

		for {
			err := db.QueryRow("SELECT status FROM p_servers WHERE server_id = $1", serverID).Scan(&status)
			if err != nil {
				c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to retrieve server status"})
				return
			}

			fmt.Println("Client status: ", currentStatus, "Server status: ", status)

			if status == currentStatus || status == "DONE" {
				break
			}

			time.Sleep(1 * time.Second)
		}

		c.JSON(http.StatusOK, gin.H{"status": status})
	})

	r.DELETE("/delete", func(c *gin.Context) {
		_, err := db.Exec("TRUNCATE TABLE p_servers RESTART IDENTITY CASCADE")
		if err != nil {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "Failed to delete rows"})
			return
		}
		c.JSON(http.StatusOK, gin.H{"message": "All rows deleted, identity reset"})
	})

	r.Run(":8080")
}
