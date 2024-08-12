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
    elif [[ $SERVICE == "Zookeeper" ]]; then
        log "$SERVICE is not running via systemctl. Attempting to stop manually..."
        sudo pkill -f "org.apache.zookeeper.server.quorum.QuorumPeerMain"
        if pgrep -f "org.apache.zookeeper.server.quorum.QuorumPeerMain" > /dev/null; then
            log "$SERVICE process could not be killed."
        else
            log "$SERVICE process killed."
        fi
    else
        log "$SERVICE is not running."
    fi
}

case "$1" in
    rabbitmq)
        stop_service "RabbitMQ" "rabbitmq-server"
        ;;
    postgresql)
        stop_service "PostgreSQL" "postgresql"
        ;;
    redis)
        stop_service "Redis" "redis-server"
        ;;
    kafka)
        stop_service "Kafka" "kafka"
        ;;
    zookeeper)
        stop_service "Zookeeper" "zookeeper"
        ;;
    mysql)
        stop_service "MySQL" "mysql"
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
