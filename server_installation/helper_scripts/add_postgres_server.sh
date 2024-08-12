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
CONFIG_DIR="${DATA_DIR}/config"
PORT=$((5432 + $(date +%s) % 1000)) # Randomly generated port

# Step 1: Prepare the directory
mkdir -p "${DATA_DIR}"
mkdir -p "${CONFIG_DIR}"
chown postgres:postgres "${DATA_DIR}"
chmod 700 "${DATA_DIR}"

# Step 2: Initialize the PostgreSQL database cluster
sudo -u postgres /usr/lib/postgresql/$(pg_config --version | awk '{print $2}')/bin/initdb -D "${DATA_DIR}" --locale=en_US.UTF-8 -E UTF8

# Step 3: Configure the new PostgreSQL instance
CONFIG_FILE="${CONFIG_DIR}/postgresql.conf"
HBA_FILE="${CONFIG_DIR}/pg_hba.conf"

cp "${DATA_DIR}/postgresql.conf" "${CONFIG_FILE}"
cp "${DATA_DIR}/pg_hba.conf" "${HBA_FILE}"

# Modify the configuration to use the new data directory and port
sed -i "s@^#data_directory =.*@data_directory = '${DATA_DIR}'@" "${CONFIG_FILE}"
sed -i "s/^#port = 5432/port = ${PORT}/" "${CONFIG_FILE}"
sed -i "s@^#logging_collector =.*@logging_collector = on@" "${CONFIG_FILE}"
sed -i "s@^#log_directory =.*@log_directory = '${DATA_DIR}/log'@" "${CONFIG_FILE}"
sed -i "s@^#log_filename =.*@log_filename = 'postgresql-${DB_NAME}.log'@" "${CONFIG_FILE}"
sed -i "s@^#unix_socket_directories =.*@unix_socket_directories = '${DATA_DIR}/socket'@" "${CONFIG_FILE}"

# Ensure correct permissions
chown -R postgres:postgres "${DATA_DIR}"

# Step 4: Create a systemd service for the new instance
SERVICE_FILE="/etc/systemd/system/postgresql-${DB_NAME}.service"

cat <<EOF > "${SERVICE_FILE}"
[Unit]
Description=PostgreSQL database server for ${DB_NAME}
Documentation=man:postgres(1)
After=network.target

[Service]
Type=forking
User=postgres
Group=postgres

# The data directory
Environment=PGDATA=${DATA_DIR}

ExecStart=/usr/lib/postgresql/$(pg_config --version | awk '{print $2}')/bin/pg_ctl start -D \${PGDATA} -l \${PGDATA}/log/postgresql.log -o "-c config_file=${CONFIG_FILE}"
ExecStop=/usr/lib/postgresql/$(pg_config --version | awk '{print $2}')/bin/pg_ctl stop -D \${PGDATA}
ExecReload=/usr/lib/postgresql/$(pg_config --version | awk '{print $2}')/bin/pg_ctl reload -D \${PGDATA}
OOMScoreAdjust=-1000

[Install]
WantedBy=multi-user.target
EOF

# Step 5: Enable and start the new PostgreSQL instance
systemctl daemon-reload
systemctl enable postgresql-${DB_NAME}.service
systemctl start postgresql-${DB_NAME}.service

echo "PostgreSQL instance ${DB_NAME} created and running on port ${PORT}."

