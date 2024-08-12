#!/bin/bash

# Exit on any error
set -e

# Check if the script is run as root
if [ "$EUID" -ne 0 ]; then 
  echo "Please run as root"
  exit
fi

# Validate input arguments
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <database-name> <root-dir>"
  exit 1
fi

DB_NAME=$1
ROOT_DIR=$2
DATA_DIR="${ROOT_DIR}/${DB_NAME}"
LOGFILE="${DATA_DIR}/logfile"
PORT=$((5432 + $(date +%s) % 1000)) # Randomly generated port

# Step 1: Prepare the directory
sudo rm -rf "${DATA_DIR}"
sudo mkdir -p "${DATA_DIR}"
sudo chown postgres:postgres "${DATA_DIR}"
sudo chmod 750 "${DATA_DIR}"

# Change to a directory accessible by postgres
cd /tmp

# Step 2: Initialize the PostgreSQL database cluster
sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D "${DATA_DIR}" --locale=en_US.UTF-8 -E UTF8

# Step 3: Modify the postgresql.conf to use the specified port
echo "port = ${PORT}" >> "${DATA_DIR}/postgresql.conf"

# Step 4: Start the PostgreSQL server
sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D "${DATA_DIR}" -l "${LOGFILE}" start

echo "PostgreSQL instance ${DB_NAME} created and running on port ${PORT}."
