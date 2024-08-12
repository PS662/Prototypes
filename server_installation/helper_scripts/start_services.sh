#!/bin/bash

# Exit script on error
set -e

log() {
    echo -e "\e[32m$1\e[0m"
}

start_service() {
    SERVICE=$1
    SYSTEMCTL_NAME=$2

    if systemctl is-active --quiet $SYSTEMCTL_NAME; then
        log "$SERVICE is already running."
    else
        log "Starting $SERVICE..."
        sudo systemctl start $SYSTEMCTL_NAME
        log "$SERVICE started."
    fi
}

case "$1" in
    rabbitmq|postgresql|redis|kafka|zookeeper|mysql)
        start_service $1 $1
        ;;
    "")
        log "Starting all services..."
        start_service "RabbitMQ" "rabbitmq-server"
        start_service "PostgreSQL" "postgresql"
        start_service "Redis" "redis-server"
        start_service "Kafka" "kafka"
        start_service "Zookeeper" "zookeeper"
        start_service "MySQL" "mysql"
        ;;
    *)
        log "Error: Invalid service name '$1'."
        log "Valid service names are: rabbitmq, postgresql, redis, kafka, zookeeper, mysql."
        exit 1
        ;;
esac
