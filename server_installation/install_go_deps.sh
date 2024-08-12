#!/bin/bash

# Exit script on error
set -e

log() {
    echo -e "\e[32m$1\e[0m"
}

# Check if Go is installed
if ! command -v go &> /dev/null; then
    log "Go is not installed. Installing Go..."

    # Download and install Go
    GO_VERSION="1.20.6"
    OS="linux"
    ARCH="amd64"
    wget https://dl.google.com/go/go$GO_VERSION.$OS-$ARCH.tar.gz

    # Extract and move to /usr/local
    sudo tar -C /usr/local -xzf go$GO_VERSION.$OS-$ARCH.tar.gz

    # Remove the downloaded tar file
    rm go$GO_VERSION.$OS-$ARCH.tar.gz

    # Set up Go environment variables
    echo "export PATH=\$PATH:/usr/local/go/bin" >> ~/.profile
    echo "export GOPATH=\$HOME/go" >> ~/.profile
    echo "export PATH=\$PATH:\$GOPATH/bin" >> ~/.profile
    source ~/.profile

    log "Go installed successfully."
else
    log "Go is already installed. Skipping installation."
fi

# Initialize Go module
log "Initializing Go modules..."
go mod init check_services

# Install necessary Go packages
log "Installing Go dependencies..."
go get github.com/go-redis/redis/v8
go get github.com/lib/pq
go get github.com/segmentio/kafka-go
go get github.com/streadway/amqp
go get github.com/go-sql-driver/mysql

log "Go setup completed. You can now run your Go program with 'go run check_installation.go'"

