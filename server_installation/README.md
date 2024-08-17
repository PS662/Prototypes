
# Server Installation

## Overview

This repo is designed to automate the setup, management, and teardown of various server components, including RabbitMQ, PostgreSQL, Redis, Kafka, Zookeeper, and MySQL. This includes Docker Compose configurations and helper scripts to simplify server management.

### Prerequisites

- Docker and Docker Compose installed on your machine.
- Go installed if you want to run the `check_installation.go` script.

## Docker Compose

Before starting the services, configure the environment variables in the `.env` file. This file contains the necessary configurations for each service.

To start all the services defined in the `docker-compose.yml` using Docker Compose, run:

```bash
docker compose up -d
```

To stop the running services, use:

```bash
docker compose down
```

## Using Helper Scripts - Local configuration

The `helper_scripts` directory contains various scripts for managing services. I have tested this on Ubuntu 22.04, might need modifications on other versions.

### 1. **Start Services**

You can start individual services or all services using the `start_services.sh` script.

- To start a specific service (e.g., RabbitMQ):

  ```bash
  ./helper_scripts/start_services.sh rabbitmq
  ```

- To start all services:

  ```bash
  ./helper_scripts/start_services.sh
  ```

### 2. **Stop Services**

You can stop individual services or all services using the `stop_services.sh` script.

- To stop a specific service (e.g., Kafka):

  ```bash
  ./helper_scripts/stop_services.sh kafka
  ```

- To stop all services:

  ```bash
  ./helper_scripts/stop_services.sh
  ```

### 3. **Teardown the Server Stack**

The `teardown_server_stack.sh` script uninstalls and cleans up the services from your machine.

- To uninstall all services:

  ```bash
  ./helper_scripts/teardown_server_stack.sh
  ```

- To uninstall a specific service (e.g., MySQL):

  ```bash
  ./helper_scripts/teardown_server_stack.sh mysql
  ```

### 4. **Add PostgreSQL or MySQL Server**

These scripts allow you to add additional PostgreSQL or MySQL instances:

- **Add PostgreSQL Server**:

  ```bash
  ./helper_scripts/add_postgres_server.sh <database-name> <root-dir>
  ```

- **Add MySQL Server**:

  ```bash
  ./helper_scripts/add_mysql_server.sh <database-name> <root-dir>
  ```

## Using `check_installation.go`

### 1. **With Docker Services and Environment Variables**

If the services are running via Docker and you have environment variables configured in the `.env` file:

- First, source the environment variables:

  ```bash
  . ./set_env.sh
  ```

- Run the Go script to check the services:

  ```bash
  go run check_installation.go
  ```

This will verify the connectivity to the services using the environment variables.

### 2. **Without Environment Variables**

If you want to run the script without using environment variables:

- Directly run the Go script with default or hardcoded values:

  ```bash
  go run check_installation.go
  ```

Make sure to modify the `check_installation.go` file if you need to set specific default values for the services.
