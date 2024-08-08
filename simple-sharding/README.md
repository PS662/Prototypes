# Sharding Replication with MySQL and Go

This guide covers setting up multiple MySQL servers using `mysqld_multi`, configuring replication, and running a Go application to interact with the database.

My endgoal was to configure mysql read replicas locally and get IDs from differnt connection objects. 

I have not written to read replicas but there is no reason why you can not do that. 

## Prerequisites

- MySQL server installed on your system.

```bash
  sudo apt update
  sudo apt install mysql-server
  sudo mysql_secure_installation
```

- Go (Golang) installed on your system. Install deps.

```
go mod init sharding-modules
go get -u github.com/go-sql-driver/mysql
go get -u github.com/gin-gonic/gin
```

- Basic understanding of MySQL administration and Go programming.

## 1. Setting Up MySQL Multi Server

We will use scripts located in the `sql_scripts/` directory to set up and configure our MySQL servers.

### Scripts Used

- `init_tables.sql`: Initializes database tables.
- `multi_sharding.cnf`: Configuration file for multiple MySQL instances.
- `setup_mysql.sh`: Shell script to set up MySQL directories and permissions.

### Execution

Run the following command to set up your MySQL servers:

```bash
./sql_scripts/setup_mysql.sh <database-name> <root-dir>

# I did these, sometimes AppArmor prevents you from using /home (I did not look into it much):

sudo ./setup_mysql.sh master-server /opt/mysql-servers/
sudo ./setup_mysql.sh replica-server-1 /opt/mysql-servers
sudo ./setup_mysql.sh replica-server-2 /opt/mysql-servers

# In another terminal, monitor the log. This is important MYSQL will set up a temporary root password for Master and Replicas, which you need to set up your users.
# I could not find a way around it.
sudo tail -f /var/log/mysql/error.log

sudo mysqld_multi --defaults-file=/etc/mysql/multi_sharding.cnf --no-log start

# Use the temporary pass
sudo mysql -u root -p -P 3307 -S /var/run/mysqld/master-server.sock
sudo mysql -u root -p -P 3308 -S /var/run/mysqld/replica-server-2.sock
sudo mysql -u root -p -P 3309 -S /var/run/mysqld/replica-server-1.sock

# If you need to stop the servers.
sudo mysqld_multi --defaults-file=/etc/mysql/multi_sharding.cnf --no-log stop
```

This script initializes the MySQL directories, sets appropriate permissions, and configures AppArmor (if on Ubuntu).

## 2. Setting Up Replication

Replication is set up manually on each server. Follow these steps for both master and slave configurations:

### Master Configuration

```


CREATE USER 'repl'@'%' IDENTIFIED BY 'password';
GRANT REPLICATION SLAVE ON *.* TO 'repl'@'%';
FLUSH PRIVILEGES;
```

Obtain the binary log position:

```
### This will have something like the following, you need this to set up replicas.
SHOW MASTER STATUS;

+------------------+----------+--------------+------------------+-------------------+
| File             | Position | Binlog_Do_DB | Binlog_Ignore_DB | Executed_Gtid_Set |
+------------------+----------+--------------+------------------+-------------------+
| mysql-bin.000002 |     6796 |              |                  |                   |
+------------------+----------+--------------+------------------+-------------------+
```

### Slave Configuration

```
mysql -u root -p -h slave_host -P slave_port

CHANGE MASTER TO
    ->     MASTER_HOST='127.0.0.1',
    ->     MASTER_USER='repl',
    ->     MASTER_PASSWORD='replpass',
    ->     MASTER_PORT=3307,
    ->     MASTER_LOG_FILE='mysql-bin.000002',
    ->     MASTER_LOG_POS=6796;  -- Use the latest position

START SLAVE;
SHOW SLAVE STATUS\G;
```

## 3. Running the Go Application

The Go application provides endpoints to interact with the database.

Execute the Go application with the following command:

```
go run sharding-replica.go
```

- GET /heartbeat/status/:user_id: Fetches the last heartbeat for the specified user ID.
- GET /populate_db/:num_records: Populates the database with the specified number of records.
- DELETE /delete_all_rows: Deletes all rows in the database.

You can use curl or any HTTP client to test the endpoints (You can use your browser, except for delete).  For example:

```
curl http://localhost:8080/populate_db/10
curl http://localhost:8080/heartbeat/status/1
curl -X DELETE http://localhost:8080/delete_all_rows
```

NOTES (Which I'll probably not fix):

- Sometimes the replicas need to be restarted if there is an error in a transaction.
    - You can use SQL_SLAVE_SKIP_COUNTER or handle this better by periodically having a resync. This is out of scope atm.
    - Use the following for easy 
        ``` 
        mysql -u root -p < master_dump.sql  
        ```
- You can automate the replica set-up and server launch as well.