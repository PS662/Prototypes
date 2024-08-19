#!/bin/bash

# Usage: ./configure_slave_replication.sh <master_ip> <master_port> <replica_ip> <replica_port> <replica_user> <replica_password> <replica_data_dir>
# Example: ./configure_slave_replication.sh localhost 5434 localhost 5435 replica_user replica_password /opt/postgres-servers/replica-1/kv-replica-1

MASTER_IP=$1
MASTER_PORT=$2
REPLICA_IP=$3
REPLICA_PORT=$4
REPLICA_USER=$5
REPLICA_PASSWORD=$6
REPLICA_DATA_DIR=$7
MAX_WAL_SENDERS=${8:-10}

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit
fi

if [ $# -ne 7 ]; then
  echo "Usage: $0 <master_ip> <master_port> <replica_ip> <replica_port> <replica_user> <replica_password> <replica_data_dir>"
  exit 1
fi

# Step 1: Stop the replica server
sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D $REPLICA_DATA_DIR stop

# Step 2: Clean the data directory
sudo -u postgres rm -rf ${REPLICA_DATA_DIR}/*

# Step 3: Use pg_basebackup to copy data from the master
sudo -u postgres /usr/lib/postgresql/14/bin/pg_basebackup -h $MASTER_IP -p $MASTER_PORT -D $REPLICA_DATA_DIR -U $REPLICA_USER -v -P --wal-method=stream --password

# Step 4: Create the standby.signal file
sudo -u postgres touch ${REPLICA_DATA_DIR}/standby.signal

# Step 5: Edit the replica’s postgresql.conf to set the correct port and replication settings
REPLICA_CONF="${REPLICA_DATA_DIR}/postgresql.conf"

echo "port = ${REPLICA_PORT}" >> $REPLICA_CONF
echo "primary_conninfo = 'host=${MASTER_IP} port=${MASTER_PORT} user=${REPLICA_USER} password=${REPLICA_PASSWORD}'" >> $REPLICA_CONF
echo "primary_slot_name = 'replica_slot'" >> $REPLICA_CONF
echo "hot_standby = on" >> $REPLICA_CONF
echo "wal_level = replica" >> $REPLICA_CONF  # Ensure WAL level is set to replica on the replica as well
echo "max_wal_senders = ${MAX_WAL_SENDERS}" >> $REPLICA_CONF  # Set a higher number to avoid any WAL streaming issues

# Step 6: Start the replica server
sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D $REPLICA_DATA_DIR -l ${REPLICA_DATA_DIR}/logfile start

echo "Slave replication setup completed."

