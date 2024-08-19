
# Key-Value Store with PostgreSQL Replication

This is a Go-based key-value store using PostgreSQL as the storage layer. It supports sharding, replication, and provides options for consistent reads and periodic purging of expired data.

## Setup

Follow these steps to set up and run the key-value store:

```bash
# Initialize a new Go module
go mod init keyvalue-store

# Install dependencies
go get github.com/gin-gonic/gin
go get github.com/lib/pq

# Run the SQL initialization script
cd sql_scripts
./init_sql.sh
```

## Replication

For setting up PostgreSQL replication, please refer to the detailed guide:

[PostgreSQL Replication Setup Guide](https://github.com/PS662/Prototypes/blob/main/server_installation/helper_scripts/PGSQL_REPLICATION.md)

## Usage

### Available Endpoints

| HTTP Method | Endpoint         | Description                               |
|-------------|-----------------|-------------------------------------------|
| `GET`       | `/get/:key`     | Retrieve the value for a given key        |
| `PUT`       | `/put`          | Insert or update a key-value pair         |
| `DELETE`    | `/delete/:key`  | Mark a key as expired (soft delete)       |

### Consistent Reads

By default, the key-value store reads from replicas for performance. However, you can enforce consistent reads from the master by setting the `consistent` query parameter to `true`:

```bash
curl "http://localhost:8080/get/someKey?consistent=true"
```

This ensures that you always read the most up-to-date value.

### Periodic Purge

The application automatically purges expired entries from the database at regular intervals. The purge interval is configurable using the `PURGE_INTERVAL` environment variable (default: 10 minutes).

To customize the purge interval, set the environment variable before running the application:

```bash
export PURGE_INTERVAL=5m
```

The purge process removes entries with `expiry < NOW()`, including those marked as deleted with `expiry = '-infinity'`.

## Running the Application

Start the application with:

```bash
go run kv_store.go
```

By default, the server runs on port `8080`. You can interact with the API using the provided endpoints.
