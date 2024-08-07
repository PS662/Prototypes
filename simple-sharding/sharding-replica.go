package main

import (
	"database/sql"
	"log"
	"net/http"
	"strconv"

	"github.com/gin-gonic/gin"
	_ "github.com/go-sql-driver/mysql"
)

type DatabaseConnections struct {
	Connections []*sql.DB
}

func InitConnections() *DatabaseConnections {
	dbConns := &DatabaseConnections{}

	masterDB, err := sql.Open("mysql", "root:Password123!@tcp(localhost:3307)/sharding_prototype_test_db?parseTime=true")
	if err != nil {
		log.Fatal("Error connecting to master:", err)
	}
	dbConns.Connections = append(dbConns.Connections, masterDB)

	replica1DB, err := sql.Open("mysql", "root:Password123!@tcp(localhost:3308)/sharding_prototype_test_db?parseTime=true")
	if err != nil {
		log.Fatal("Error connecting to replica-1:", err)
	}
	dbConns.Connections = append(dbConns.Connections, replica1DB)

	replica2DB, err := sql.Open("mysql", "root:Password123!@tcp(localhost:3309)/sharding_prototype_test_db?parseTime=true")
	if err != nil {
		log.Fatal("Error connecting to replica-2:", err)
	}
	dbConns.Connections = append(dbConns.Connections, replica2DB)

	return dbConns
}

func (d *DatabaseConnections) shardIndex(userID int) *sql.DB {
	if userID%2 == 0 {
		return d.Connections[1] // Replica-1
	} else if userID%3 == 0 {
		return d.Connections[2] // Replica-2
	}
	return d.Connections[0] // Master
}

func getHeartbeat(d *DatabaseConnections) gin.HandlerFunc {
	return func(c *gin.Context) {
		userIDStr := c.Param("user_id")
		userID, err := strconv.Atoi(userIDStr)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid user ID"})
			return
		}

		db := d.shardIndex(userID)
		row := db.QueryRow("SELECT last_heartbeat FROM pulse WHERE id = ?", userID)
		var lastHeartbeat sql.NullTime
		err = row.Scan(&lastHeartbeat)
		if err != nil {
			var valueType string
			row.Scan(&valueType) // Attempt to read as string to see the raw output
			log.Printf("Error querying the database: %v, value type: %s", err, valueType)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "Error fetching data"})
			return
		}

		if lastHeartbeat.Valid {
			c.JSON(http.StatusOK, gin.H{"Last Heartbeat": lastHeartbeat.Time.String()})
		} else {
			c.JSON(http.StatusOK, gin.H{"message": "No heartbeat data available"})
		}
	}
}

func populateDB(d *DatabaseConnections) gin.HandlerFunc {
	return func(c *gin.Context) {
		numRecordsStr := c.Param("num_records")
		numRecords, err := strconv.Atoi(numRecordsStr)
		if err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "Invalid number of records"})
			return
		}

		db := d.Connections[0]
		for i := 0; i < numRecords; i++ {
			_, err := db.Exec("INSERT INTO pulse (last_heartbeat) VALUES (NOW())")
			if err != nil {
				c.JSON(http.StatusInternalServerError, gin.H{"error": "Error populating database"})
				return
			}
		}

		c.JSON(http.StatusOK, gin.H{"message": "Database populated successfully"})
	}
}

func deleteAllRows(d *DatabaseConnections) gin.HandlerFunc {
	return func(c *gin.Context) {
		for i, db := range d.Connections {
			// Perform the DELETE operation
			_, err := db.Exec("DELETE FROM pulse")
			if err != nil {
				log.Printf("Error deleting data from connection %d: %v", i, err)
				c.JSON(http.StatusInternalServerError, gin.H{"error": "Error deleting data"})
				return
			}

			// Reset the auto-increment value
			_, err = db.Exec("ALTER TABLE pulse AUTO_INCREMENT = 1")
			if err != nil {
				log.Printf("Error resetting auto-increment on connection %d: %v", i, err)
				c.JSON(http.StatusInternalServerError, gin.H{"error": "Error resetting auto-increment"})
				return
			}
		}

		// Return a success response
		c.JSON(http.StatusOK, gin.H{"message": "All rows deleted and auto-increment reset successfully"})
	}
}

func main() {
	dbConns := InitConnections()
	r := gin.Default()

	r.GET("/heartbeat/status/:user_id", getHeartbeat(dbConns))
	r.GET("/populate_db/:num_records", populateDB(dbConns))
	r.DELETE("/delete_all_rows", deleteAllRows(dbConns))

	log.Fatal(r.Run(":8080")) // listen and serve on 0.0.0.0:8080
}
