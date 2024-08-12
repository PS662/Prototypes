#!/bin/bash

set -e

if [ "$EUID" -ne 0 ]; then 
  echo "Please run as root"
  exit
fi

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <database-name> <root-dir>"
  exit 1
fi

DB_NAME=$1
ROOT_DIR=$2
DATA_DIR="${ROOT_DIR}/${DB_NAME}"
LOGFILE="${DATA_DIR}/logfile"
PORT=5434

sudo rm -rf "${DATA_DIR}"
sudo mkdir -p "${DATA_DIR}"
sudo chown postgres:postgres "${DATA_DIR}"
sudo chmod 750 "${DATA_DIR}"

cd /tmp

sudo -u postgres /usr/lib/postgresql/14/bin/initdb -D "${DATA_DIR}" --locale=en_US.UTF-8 -E UTF8

echo "port = ${PORT}" >> "${DATA_DIR}/postgresql.conf"

sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D "${DATA_DIR}" -l "${LOGFILE}" start

echo "PostgreSQL instance ${DB_NAME} created and running on port ${PORT}."
