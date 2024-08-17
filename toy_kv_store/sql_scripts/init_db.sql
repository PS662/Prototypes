-- Step 1: Connect to the default 'postgres' database
\c postgres

-- Step 2: Create the database 'key_value_db' if it doesn't exist
CREATE DATABASE key_value_db;

-- Step 3: Connect to the new database
\c key_value_db

-- Step 4: Create the testuser role if it doesn't exist
DO $$
BEGIN
   IF NOT EXISTS (SELECT FROM pg_catalog.pg_roles WHERE rolname = 'testuser') THEN
      CREATE ROLE testuser WITH LOGIN PASSWORD 'Password123!';
   END IF;
END
$$;

-- Step 5: Create the table only if it doesn't already exist
CREATE TABLE IF NOT EXISTS kv_store (
    "key" TEXT PRIMARY KEY,
    "value" TEXT NOT NULL,
    "expiry" TIMESTAMPTZ
);

-- Step 6: Grant privileges to testuser on the database and table
GRANT ALL PRIVILEGES ON DATABASE key_value_db TO testuser;
GRANT ALL PRIVILEGES ON TABLE kv_store TO testuser;

