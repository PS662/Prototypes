# Go Database Connection Pool Benchmark

This project includes Go code to benchmark database operations using pooled and non-pooled connections. It utilizes MySQL as the backend database.

## Prerequisites

Before running the benchmarks, ensure you have MySQL and Go installed on your system.

### Setting Up MySQL

1. **Install MySQL**: If MySQL is not installed, you can install it on Ubuntu with the following commands:

    ```bash
    sudo apt update
    sudo apt install mysql-server
    sudo mysql_secure_installation
    ```

2. **Configure MySQL**:

    - Log in to MySQL as root:

      ```bash
      sudo mysql
      ```

    - Create a database and user for the benchmarks:

      ```sql
      CREATE DATABASE prototype_test_db;
      CREATE USER 'testuser'@'localhost' IDENTIFIED BY 'Password123!';
      GRANT ALL PRIVILEGES ON prototype_test_db.* TO 'testuser'@'localhost';
      FLUSH PRIVILEGES;
      EXIT;
      ```

### Setup Go Environment

1. **Fetch Go Modules**: Navigate to your project directory where your Go files are located and initialize the modules:

    ```bash
    go mod init yourmodule
    go get -u github.com/go-sql-driver/mysql
    ```

## Running the Examples

1. **Update Connection String**: Modify the `dsn` constant in your Go file to match the credentials and database name you've configured in MySQL:

    ```go
    const dsn = "testuser:Password123!@tcp(localhost:3306)/prototype_test_db"
    ```

2. **Run the Benchmark**:

    Execute the benchmarks by running:

    ```bash
    go run connection-pool.go
    ```
    
    This will execute both non-pooled and pooled connection benchmarks and display the results in the terminal.
    ```
    ## A sample example would be
    Starting non-pool benchmarks:
    Non-pool time for 10 threads: 11.904069ms
    Non-pool time for 100 threads: 17.472303ms
    Error running non-pool benchmark for 300 threads: Error 1040 (08004): Too many connections
    Error running non-pool benchmark for 500 threads: Error 1040: Too many connections
    Error running non-pool benchmark for 1000 threads: Error 1040: Too many connections

    Starting pool benchmarks:
    Pool time for 10 threads: 10.770849ms
    Pool time for 100 threads: 104.328077ms
    Pool time for 300 threads: 318.484346ms
    Pool time for 500 threads: 531.818956ms
    Pool time for 1000 threads: 1.072567285s

    ## You might sometimes see this in non pool benchmark
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:41006->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:43516->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:41830->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:42552->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:42326->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:41806->127.0.0.1:3306: read: connection reset by peer
    [mysql] 2024/08/05 23:47:49 connection.go:49: read tcp 127.0.0.1:42354->127.0.0.1:3306: read: connection reset by peer

    ```

