#!/bin/bash

# Exit script on error
set -e

log() {
    echo -e "\e[32m$1\e[0m"
}

stop_service() {
    SERVICE=$1
    SYSTEMCTL_NAME=$2

    if systemctl is-active --quiet $SYSTEMCTL_NAME; then
        log "Stopping $SERVICE..."
        sudo systemctl stop $SYSTEMCTL_NAME
        log "$SERVICE stopped."
    else
        log "$SERVICE is not running."
    fi
}

case "$1" in
    rabbitmq|postgresql|redis|kafka|zookeeper|mysql)
        stop_service $1 $1
        ;;
    "")
        log "Stopping all services..."
        stop_service "RabbitMQ" "rabbitmq-server"
        stop_service "PostgreSQL" "postgresql"
        stop_service "Redis" "redis-server"
        stop_service "Kafka" "kafka"
        stop_service "Zookeeper" "zookeeper"
        stop_service "MySQL" "mysql"
        ;;
    *)
        log "Error: Invalid service name '$1'."
        log "Valid service names are: rabbitmq, postgresql, redis, kafka, zookeeper, mysql."
        exit 1
        ;;
esac
