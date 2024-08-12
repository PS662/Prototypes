#!/bin/bash

# Exit script on error
set -e

log() {
    echo -e "\e[32m$1\e[0m"
}

error_log() {
    echo -e "\e[31m$1\e[0m"
}

# Function to uninstall Docker and Docker Compose
uninstall_docker() {
    if command -v docker &> /dev/null; then
        log "Uninstalling Docker and Docker Compose..."
        sudo apt-get remove --purge -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
        sudo rm -rf /var/lib/docker
        sudo rm -rf /etc/docker
        sudo rm -rf /etc/apt/keyrings/docker.asc
        sudo apt-get autoremove -y
        sudo apt-get autoclean -y
        log "Docker and Docker Compose uninstalled successfully."
    else
        log "Docker is not installed. Skipping uninstallation."
    fi
}

# Function to uninstall RabbitMQ
uninstall_rabbitmq() {
    if dpkg -l | grep -q rabbitmq-server; then
        log "Uninstalling RabbitMQ..."
        sudo apt-get remove --purge -y rabbitmq-server erlang
        sudo apt-get autoremove -y
        sudo apt-get autoclean -y
        log "RabbitMQ uninstalled."
    else
        log "RabbitMQ is not installed. Skipping uninstallation."
    fi
}

# Function to uninstall PostgreSQL
uninstall_postgresql() {
    if dpkg -l | grep -q postgresql; then
        log "Uninstalling PostgreSQL..."
        sudo apt-get remove --purge -y postgresql postgresql-contrib
        sudo apt-get autoremove -y
        sudo apt-get autoclean -y
        sudo rm -rf /var/lib/postgresql
        sudo rm -rf /etc/postgresql
        sudo rm -rf /etc/postgresql-common
        sudo rm -rf /var/log/postgresql
        sudo deluser postgres
        sudo delgroup postgres
        log "PostgreSQL uninstalled."
    else
        log "PostgreSQL is not installed. Skipping uninstallation."
    fi
}

# Function to uninstall Redis
uninstall_redis() {
    if dpkg -l | grep -q redis-server; then
        log "Uninstalling Redis..."
        sudo systemctl stop redis-server
        sudo systemctl disable redis-server
        sudo apt-get remove --purge -y redis-server
        sudo apt-get autoremove -y
        sudo apt-get autoclean -y
        sudo rm -rf /etc/redis
        sudo rm -rf /var/lib/redis
        sudo rm -rf /var/log/redis
        log "Redis uninstalled."
    else
        log "Redis is not installed. Skipping uninstallation."
    fi
}

# Function to uninstall Kafka and Zookeeper
uninstall_kafka() {
    if systemctl is-active --quiet kafka; then
        log "Stopping Kafka service..."
        sudo systemctl stop kafka
        sudo systemctl disable kafka
        sudo rm /etc/systemd/system/kafka.service
        log "Kafka service stopped and disabled."
    else
        log "Kafka service is not running. Skipping..."
    fi

    if systemctl is-active --quiet zookeeper; then
        log "Stopping Zookeeper service..."
        sudo systemctl stop zookeeper
        sudo systemctl disable zookeeper
        sudo rm /etc/systemd/system/zookeeper.service
        log "Zookeeper service stopped and disabled."
    else
        log "Zookeeper service is not running. Skipping..."
    fi

    if [ -d "/opt/kafka" ]; then
        log "Removing Kafka and Zookeeper files..."
        sudo rm -rf /opt/kafka
        log "Kafka and Zookeeper files removed."
    else
        log "Kafka and Zookeeper files not found. Skipping..."
    fi
}

# Function to uninstall MySQL
uninstall_mysql() {
    if dpkg -l | grep -q mysql-server; then
        log "Uninstalling MySQL..."
        sudo systemctl stop mysql
        sudo systemctl disable mysql
        sudo apt-get remove --purge -y mysql-server mysql-client mysql-common
        sudo apt-get autoremove -y
        sudo apt-get autoclean -y
        sudo rm -rf /etc/mysql
        sudo rm -rf /var/lib/mysql
        sudo rm -rf /var/log/mysql
        sudo deluser mysql
        sudo delgroup mysql
        log "MySQL uninstalled."
    else
        log "MySQL is not installed. Skipping uninstallation."
    fi
}

# Main script logic to uninstall services by name or all by default
uninstall_all_services() {
    uninstall_docker
    uninstall_rabbitmq
    uninstall_postgresql
    uninstall_redis
    uninstall_kafka
    uninstall_mysql
}

if [ "$#" -eq 0 ]; then
    log "Uninstalling all services..."
    uninstall_all_services
else
    for service in "$@"; do
        case $service in
            docker)
                uninstall_docker
                ;;
            rabbitmq)
                uninstall_rabbitmq
                ;;
            postgresql)
                uninstall_postgresql
                ;;
            redis)
                uninstall_redis
                ;;
            kafka)
                uninstall_kafka
                ;;
            mysql)
                uninstall_mysql
                ;;
            *)
                error_log "Unknown service: $service"
                ;;
        esac
    done
fi
