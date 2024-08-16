
# PostgreSQL Replication Setup

## Step 1: Setup Master and Replica Servers

```bash
sudo ./helper_scripts/add_postgres_server.sh kv-master-1 /opt/postgres-servers/master-1 5434

sudo ./helper_scripts/add_postgres_server.sh kv-replica-1 /opt/postgres-servers/replica-1 5435
```

## Step 2: Configure Master for Replication

```bash
sudo ./helper_scripts/configure_master_replication.sh 127.0.0.1 5434 replica_user replica_password /opt/postgres-servers/master-1/kv-master-1
```

**Note:** To check if everything went well, look at:

```bash
sudo cat /opt/postgres-servers/master-1/kv-master-1/logfile
```

## Step 3: Configure Replica for Replication

```bash
sudo ./helper_scripts/configure_slave_replication.sh 127.0.0.1 5434 127.0.0.1 5435 replica_user replica_password /opt/postgres-servers/replica-1/kv-replica-1
```

**Note:** In case of successful replication setup, you would see something like:

```bash
sudo cat /opt/postgres-servers/replica-1/kv-replica-1/logfile

# Example log output
2024-08-17 02:45:01.085 AEST [126508] LOG:  starting PostgreSQL 14.12 (Ubuntu 14.12-0ubuntu0.22.04.1) on x86_64-pc-linux-gnu, compiled by gcc (Ubuntu 11.4.0-1ubuntu1~22.04) 11.4.0, 64-bit
2024-08-17 02:45:01.086 AEST [126508] LOG:  listening on IPv4 address "127.0.0.1", port 5434
2024-08-17 02:45:01.089 AEST [126508] LOG:  listening on Unix socket "/var/run/postgresql/.s.PGSQL.5434"
...
2024-08-17 02:58:44.904 AEST [133774] LOG:  started streaming WAL from primary at 0/5000000 on timeline 1
```

## Replication Setup Issues

You might face issues while setting up replication. Below are common troubleshooting steps:

### Issue 1: WAL Senders Mismatch

1. Check where the WAL senders are coming from:

```bash
psql -h localhost -p 5434 -U postgres -c "SELECT name, setting, source FROM pg_settings WHERE name = 'max_wal_senders';"
```

2. If you need to update it, do:

```bash
sudo vim /opt/postgres-servers/master-1/kv-master-1/postgresql.conf
```

3. Check if they are set to the correct values:

```bash
psql -h localhost -p 5434 -U postgres -c "SHOW max_wal_senders;"
```

4. Just restart the master server (not the entire cluster):

```bash
sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D /opt/postgres-servers/master-1/kv-master-1 restart
```

### Issue 2: Replication Slot Not Set

1. Create a physical replication slot:

```bash
psql -h localhost -p 5434 -U postgres -c "SELECT * FROM pg_create_physical_replication_slot('replica_slot');"
```

2. Check the slot:

```bash
psql -h localhost -p 5434 -U postgres -c "SELECT * FROM pg_replication_slots;"
```

3. Restart the replica to reload only the replication config:

```bash
sudo -u postgres /usr/lib/postgresql/14/bin/pg_ctl -D /opt/postgres-servers/replica-1/kv-replica-1 restart
```

4. Check replication status:

```bash
psql -h localhost -p 5434 -U postgres -c "SELECT * FROM pg_stat_replication;"
```

## Quick Test for Replication

1. Insert data into the master:

```bash
psql -h localhost -p 5434 -U postgres -d key_value_db -c "INSERT INTO kv_store (key, value, expiry) VALUES ('test_key', 'test_value', NOW() + interval '1 day');"
```

2. Verify the data on the replica:

```bash
psql -h localhost -p 5435 -U postgres -d key_value_db -c "SELECT * FROM kv_store;"
```

Expected output:

```bash
   key    |   value    |            expiry            
----------+------------+------------------------------
 test_key | test_value | 2024-08-18 02:36:43.32984+10
(1 row)
```

---
