CREATE DATABASE IF NOT EXISTS prototype_test_db;
CREATE USER IF NOT EXISTS 'testuser'@'localhost' IDENTIFIED BY 'Password123!';
GRANT ALL PRIVILEGES ON prototype_test_db.* TO 'testuser'@'localhost';
FLUSH PRIVILEGES;

