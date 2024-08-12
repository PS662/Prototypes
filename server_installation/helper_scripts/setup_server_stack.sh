#!/bin/bash

# Exit script on error
set -e

log() {
    echo -e "\e[32m$1\e[0m"
}

error_log() {
    echo -e "\e[31m$1\e[0m"
}

# Function to install Docker and Docker Compose
install_docker() {
    if ! command -v docker &> /dev/null; then
        log "Docker is not installed. Installing Docker..."

        sudo apt-get install -y ca-certificates curl
        sudo install -m 0755 -d /etc/apt/keyrings
        sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
        sudo chmod a+r /etc/apt/keyrings/docker.asc

        echo \
        "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu \
        $(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
        sudo tee /etc/apt/sources.list.d/docker.list > /dev/null
        sudo apt-get update

        sudo apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin

        # Run hello-world to verify installation
        sudo docker run hello-world

        log "Docker and Docker Compose installed successfully."

        # Add user to the docker group
        if ! getent group docker > /dev/null; then
            log "Docker group does not exist. Creating docker group and adding the current user to it..."
            sudo groupadd docker
            sudo usermod -aG docker $USER
            newgrp docker
            log "Docker group created, and user $USER added to the docker group."
        else
            log "Docker group already exists. Skipping group creation and user modification."
        fi
    else
        log "Docker is already installed. Skipping installation."
    fi
}

# Function to install Docker Compose separately if needed
install_docker_compose() {
    if ! docker compose version &> /dev/null; then
        log "Docker Compose is not installed. Installing Docker Compose..."
        sudo apt-get install docker-compose-plugin -y
        docker compose version
        log "Docker Compose installed successfully."
    else
        log "Docker Compose is already installed. Skipping installation."
    fi
}

# Function to install RabbitMQ
install_rabbitmq() {
    if ! dpkg -l | grep -q rabbitmq-server; then
        log "Installing Erlang (RabbitMQ dependency)..."
        sudo apt-get install -y erlang

        log "Installing RabbitMQ..."
        sudo apt-get install -y rabbitmq-server

        sudo systemctl enable rabbitmq-server
        sudo systemctl start rabbitmq-server

        sudo rabbitmq-plugins enable rabbitmq_management
        log "RabbitMQ installation completed."
    else
        log "RabbitMQ is already installed. Skipping installation."
    fi
}

# Function to check if PostgreSQL is installed
is_postgresql_installed() {
    dpkg -l | grep -q postgresql && [ -d "/etc/postgresql" ] && return 0 || return 1
}

# Function to install PostgreSQL
install_postgresql() {
    if ! is_postgresql_installed; then
        log "Installing PostgreSQL..."
        sudo apt-get install -y postgresql postgresql-contrib

        log "Creating PostgreSQL cluster..."
        sudo pg_createcluster 14 main --start

        log "PostgreSQL cluster created and started successfully."

        # Create PostgreSQL user and database if they don't exist
        if sudo -i -u postgres psql -tc "SELECT 1 FROM pg_roles WHERE rolname='testuser'" | grep -q 1; then
            log "PostgreSQL user 'testuser' already exists. Skipping user creation."
        else
            sudo -i -u postgres psql -c "CREATE USER testuser WITH PASSWORD 'Password123!';"
            log "PostgreSQL user 'testuser' created."
        fi
        
        if sudo -i -u postgres psql -lqt | cut -d \| -f 1 | grep -qw testdb; then
            log "PostgreSQL database 'testdb' already exists. Skipping database creation."
        else
            sudo -i -u postgres psql -c "CREATE DATABASE testdb OWNER testuser;"
            log "PostgreSQL database 'testdb' created."
        fi

    else
        log "PostgreSQL is already installed. Skipping installation."
    fi
}

# Function to install Redis
install_redis() {
    if ! dpkg -l | grep -q redis-server; then
        log "Installing Redis..."
        sudo apt-get install -y redis-server

        sudo systemctl enable redis-server
        sudo systemctl start redis-server

        log "Redis installation completed."
    else
        log "Redis is already installed. Skipping installation."
    fi
}

# Function to install Kafka and Zookeeper
install_kafka() {
    if [ ! -d "/opt/kafka" ]; then
        log "Installing Java (Kafka dependency)..."
        sudo apt-get install -y default-jre

        log "Downloading and extracting Kafka..."
        wget https://dlcdn.apache.org/kafka/3.8.0/kafka_2.13-3.8.0.tgz
        tar -xvzf kafka_2.13-3.8.0.tgz
        sudo mv kafka_2.13-3.8.0 /opt/kafka

        log "Creating systemd service files for Zookeeper and Kafka..."

        sudo bash -c 'cat <<EOF > /etc/systemd/system/zookeeper.service
[Unit]
Description=Apache Zookeeper server
Documentation=http://zookeeper.apache.org
Requires=network.target remote-fs.target
After=network.target remote-fs.target

[Service]
Type=simple
ExecStart=/opt/kafka/bin/zookeeper-server-start.sh /opt/kafka/config/zookeeper.properties
ExecStop=/opt/kafka/bin/zookeeper-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF'

        sudo bash -c 'cat <<EOF > /etc/systemd/system/kafka.service
[Unit]
Description=Apache Kafka Server
Documentation=http://kafka.apache.org/documentation.html
Requires=zookeeper.service
After=zookeeper.service

[Service]
Type=simple
Environment="KAFKA_HEAP_OPTS=-Xmx512M -Xms512M"
ExecStart=/opt/kafka/bin/kafka-server-start.sh /opt/kafka/config/server.properties
ExecStop=/opt/kafka/bin/kafka-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF'

        sudo systemctl daemon-reload
        sudo systemctl enable zookeeper
        sudo systemctl start zookeeper
        sudo systemctl enable kafka
        sudo systemctl start kafka

        log "Kafka and Zookeeper installation completed."
    else
        log "Kafka is already installed. Skipping installation."
    fi
}

# Function to install MySQL
install_mysql() {
    if ! dpkg -l | grep -q mysql-server; then
        log "Installing MySQL..."
        sudo apt-get install -y mysql-server

        sudo systemctl enable mysql
        sudo systemctl start mysql

        log "Securing MySQL installation..."
        sudo mysql_secure_installation

        sudo mysql -e "CREATE USER IF NOT EXISTS 'testuser'@'localhost' IDENTIFIED BY 'Password123!';"
        sudo mysql -e "CREATE DATABASE IF NOT EXISTS testdb;"
        sudo mysql -e "GRANT ALL PRIVILEGES ON testdb.* TO 'testuser'@'localhost';"
        sudo mysql -e "FLUSH PRIVILEGES;"

        log "MySQL installation completed."
    else
        log "MySQL is already installed. Skipping installation."
    fi
}

# Main script logic to install services by name or all by default
install_all_services() {
    install_docker
    install_rabbitmq
    install_postgresql
    install_redis
    install_kafka
    install_mysql
}

if [ "$#" -eq 0 ]; then
    log "Installing all services..."
    install_all_services
else
    for service in "$@"; do
        case $service in
            docker)
                install_docker
                ;;
            docker-compose)
                install_docker_compose
                ;;
            rabbitmq)
                install_rabbitmq
                ;;
            postgresql)
                install_postgresql
                ;;
            redis)
                install_redis
                ;;
            kafka)
                install_kafka
                ;;
            mysql)
                install_mysql
                ;;
            *)
                error_log "Unknown service: $service"
                ;;
        esac
    done
fi
