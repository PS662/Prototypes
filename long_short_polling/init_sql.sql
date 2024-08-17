-- Create the table p_servers with columns server_id and status
CREATE TABLE IF NOT EXISTS p_servers (
    server_id SERIAL PRIMARY KEY,
    status VARCHAR(50)
);
