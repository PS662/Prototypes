#!/bin/bash

# Default values for parameters
DB_HOST="localhost"
DB_PORT="5432"
DB_USER="postgres"
DB_NAME="key_value_db"
SQL_SCRIPT_PATH="init_db.sql"

# Parse command-line arguments
while getopts h:p:u:d:f: flag
do
    case "${flag}" in
        h) DB_HOST=${OPTARG};;
        p) DB_PORT=${OPTARG};;
        u) DB_USER=${OPTARG};;
        d) DB_NAME=${OPTARG};;
        f) SQL_SCRIPT_PATH=${OPTARG};;
    esac
done

# Prompt the user if they want to drop existing tables
read -p "Do you want to drop and recreate the tables if they exist? (y/n): " REPLY

# Check if the database exists
DB_EXISTS=$(psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname='$DB_NAME'")

if [[ "$REPLY" =~ ^[Yy]$ ]]; then
    if [ "$DB_EXISTS" = "1" ]; then
        # Connect to the database and drop the kv_store table if it exists
        psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "DROP TABLE IF EXISTS kv_store CASCADE;"
        echo "Table dropped if it existed. Running the script to recreate the tables..."
    else
        echo "Database does not exist. Creating the database and tables..."
    fi
else
    echo "Using existing tables. Running the script..."
fi

# Run the init_db.sql script
psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d postgres -f $SQL_SCRIPT_PATH

