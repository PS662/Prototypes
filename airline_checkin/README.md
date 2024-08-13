# Airline Check-in prototype

This project is a prototype for an airline check-in system implemented in C++. It uses PostgreSQL for database management and `libpqxx` for interacting with the database.

## Prerequisites

- **C++17 compiler** (e.g., `g++`)
- **PostgreSQL** installed and running
- **libpqxx** library for C++ PostgreSQL interaction

    ```
    sudo apt-get install libpqxx-dev
    ```

## Files

- **`airline_checkin.cpp`**: The main source code for the project.
- **`create_airline_db.sh`**: A script to set up the PostgreSQL database.
- **`Makefile`**: A makefile to build, run, and manage the project.

## Makefile Targets

- **`make`**: Builds the project, creating the `airline_checkin` executable.
- **`make db`**: Runs the database setup script to create and configure the PostgreSQL database.
- **`make run-approach1`**: Runs the check-in system using approach 1.
- **`make run-approach2`**: Runs the check-in system using approach 2.
- **`make run-approach3`**: Runs the check-in system using approach 3.
- **`make clean`**: Cleans the project by removing the compiled executable.
- **`make all`**: Sets up the database, builds the project, and runs all three approaches sequentially.

## Usage

1. **Set up the database**
   ```bash
   make db
    ```

2. **Build the project**

    ```
    make
     ```

3. **Run a specific approach**

    ```
    make run-approach3
    ```

4. **Run everything (setup, build, and run all approaches)**

    ```
    make all    
    ```

5. **Clean the build**

    ```
    make clean
    ```