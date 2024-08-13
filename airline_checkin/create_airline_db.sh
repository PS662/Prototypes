#!/bin/bash

# Function to create a database and tables
create_airline_checkin_db() {
    DB_NAME="airline_checkin_testdb"
    DB_USER="testuser"
    DB_PASSWORD="Password123!"

    # Check if the database already exists
    DB_EXIST=$(sudo -i -u postgres psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME; echo $?)

    if [ $DB_EXIST -eq 0 ]; then
        echo "Database '$DB_NAME' already exists."
    else
        # Create the database
        echo "Creating database '$DB_NAME'..."
        sudo -i -u postgres psql -c "CREATE DATABASE $DB_NAME OWNER $DB_USER;"
        echo "Database '$DB_NAME' created successfully."
    fi

    # Check if tables exist
    TABLES_EXIST=$(sudo -i -u postgres psql -d $DB_NAME -tAc "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name IN ('users', 'seats', 'trips'))")

    if [ "$TABLES_EXIST" = "t" ]; then
        echo "Tables 'users', 'seats', or 'trips' already exist in the database."
        read -p "Do you want to drop these tables and recreate them? (y/n): " CONFIRM
        if [ "$CONFIRM" = "y" ]; then
            echo "Dropping existing tables..."
            sudo -i -u postgres psql -d $DB_NAME -c "DROP TABLE IF EXISTS seats CASCADE;"
            sudo -i -u postgres psql -d $DB_NAME -c "DROP TABLE IF EXISTS users CASCADE;"
            sudo -i -u postgres psql -d $DB_NAME -c "DROP TABLE IF EXISTS trips CASCADE;"
            echo "Tables dropped successfully."
        else
            echo "Exiting without making changes."
            exit 0
        fi
    fi

    # Create tables without foreign key constraints
    echo "Creating tables 'users', 'seats', and 'trips' in database '$DB_NAME'..."

    sudo -i -u postgres psql -d $DB_NAME -c "
        CREATE TABLE IF NOT EXISTS users (
            user_id SERIAL PRIMARY KEY,
            name VARCHAR(50) NOT NULL
        );

        CREATE TABLE IF NOT EXISTS trips (
            trip_id SERIAL PRIMARY KEY,
            name VARCHAR(50) NOT NULL
        );

        CREATE TABLE IF NOT EXISTS seats (
            seat_id SERIAL PRIMARY KEY,
            name VARCHAR(10) NOT NULL,
            trip_id INT,
            user_id INT
        );
    "

    echo "Tables created successfully."

    # Change ownership of tables to testuser (this also changes the ownership of linked sequences)
    echo "Changing ownership of tables to '$DB_USER'..."
    sudo -i -u postgres psql -d $DB_NAME -c "
        ALTER TABLE users OWNER TO $DB_USER;
        ALTER TABLE trips OWNER TO $DB_USER;
        ALTER TABLE seats OWNER TO $DB_USER;
    "
    echo "Ownership of tables (and linked sequences) changed successfully."

    # Grant privileges to testuser
    echo "Granting privileges to user '$DB_USER'..."
    sudo -i -u postgres psql -d $DB_NAME -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO $DB_USER;"
    sudo -i -u postgres psql -d $DB_NAME -c "GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO $DB_USER;"
    sudo -i -u postgres psql -d $DB_NAME -c "GRANT TRUNCATE ON ALL TABLES IN SCHEMA public TO $DB_USER;"
    echo "Privileges granted successfully."
}

# Function to clean everything from the database
clean_airline_checkin_db() {
    DB_NAME="airline_checkin_testdb"

    # Check if the database exists
    DB_EXIST=$(sudo -i -u postgres psql -lqt | cut -d \| -f 1 | grep -qw $DB_NAME; echo $?)

    if [ $DB_EXIST -eq 0 ]; then
        echo "Dropping database '$DB_NAME'..."
        sudo -i -u postgres psql -c "DROP DATABASE $DB_NAME;"
        echo "Database '$DB_NAME' dropped successfully."
    else
        echo "Database '$DB_NAME' does not exist."
    fi
}

# If the script is called with an argument, use it to determine the function to run
if [ "$1" == "clean" ]; then
    clean_airline_checkin_db
else
    create_airline_checkin_db
fi
