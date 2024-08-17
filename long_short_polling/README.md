
# Long and Short Polling with Go

This prototype demonstrates how to implement both short polling and long polling using Go, Gin, and PostgreSQL.

## Setup Instructions

### Initialize the Go Modules

```bash
go mod init long-short-polling-modules
go get github.com/gin-gonic/gin
go get github.com/lib/pq
```

### Setup PostgreSQL Database
Make sure you have a PostgreSQL database running and accessible. Then, initialize the database:

```bash
psql -h localhost -p 5432 -U testuser -d testdb -f init_sql.sql
```

## Usage Instructions

### Create a New Server (Auto-Increment ID)

Create a new server entry in the database:
```bash
curl -X POST http://localhost:8080/servers
```

### Short Polling
Check the current status of a specific server using short polling:
```bash
curl http://localhost:8080/short/status/1
```

### Long Polling
Long poll to check if the server has been created:
```bash
curl http://localhost:8080/long/status/1
```

Long poll to check if the status has changed to a specific status:
```bash
curl http://localhost:8080/long/status/1?status=IN_PROGRESS
```

### Delete All Rows
Delete all rows from the `p_servers` table and reset the primary key sequence:
```bash
curl -X DELETE http://localhost:8080/delete