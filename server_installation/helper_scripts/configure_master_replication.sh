#!/bin/bash

# Usage: ./configure_master_replication.sh <master_ip> <master_port> <replica_user> <replica_password> <db_dir> [<max_wal_senders>] [<wal_keep_size>]
# Example: ./configure_master_replication.sh localhost 5435 replica_user replica_password /opt/postgres-servers/master-1/kv-master-1 5 128MB

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit 1
fi

if [ "$#" -lt 5 ]; then
  echo "Usage: $0 <master_ip> <master_port> <replica_user> <replica_password> <db_dir> [<max_wal_senders>] [<wal_keep_size>]"
  exit 1
fi

MASTER_IP=$1
MASTER_PORT=$2
REPLICA_USER=$3
REPLICA_PASSWORD=$4
DB_DIR=$5

# Optional Arguments with Defaults
MAX_WAL_SENDERS=${6:-10}  # Default value: 10
WAL_KEEP_SIZE=${7:-64MB} # Default value: 64MB

POSTGRES_CONF="${DB_DIR}/postgresql.conf"
PG_HBA_CONF="${DB_DIR}/pg_hba.conf"

echo "Configuring PostgreSQL master at ${MASTER_IP}:${MASTER_PORT}..."

# Add replication settings to postgresql.conf
echo "wal_level = replica" >> $POSTGRES_CONF
echo "max_wal_senders = $MAX_WAL_SENDERS" >> $POSTGRES_CONF
echo "wal_keep_size = $WAL_KEEP_SIZE" >> $POSTGRES_CONF
echo "archive_mode = on" >> $POSTGRES_CONF  # Ensure archiving is enabled for better WAL management
echo "archive_command = 'cp %p /var/lib/postgresql/14/main/archive/%f'" >> $POSTGRES_CONF  # Customize this based on your setup

# Add replication access for the replica in pg_hba.conf
echo "host replication ${REPLICA_USER} ${MASTER_IP}/32 trust" >> $PG_HBA_CONF

# Restart PostgreSQL to apply changes
sudo systemctl restart postgresql

# Create the replication role
psql -h $MASTER_IP -p $MASTER_PORT -U postgres -d postgres -c "CREATE ROLE ${REPLICA_USER} WITH REPLICATION LOGIN PASSWORD '${REPLICA_PASSWORD}';"

# Create the physical replication slot
psql -h $MASTER_IP -p $MASTER_PORT -U postgres -d postgres -c "SELECT * FROM pg_create_physical_replication_slot('replica_slot');"

echo "Master replication setup completed."

