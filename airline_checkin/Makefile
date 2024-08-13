# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS = -lpqxx -lpq

# Target executable
TARGET = airline_checkin

# Source file
SRC = airline_checkin.cpp

# Database setup script
DB_SCRIPT = ./create_airline_db.sh

# Default target
all: $(TARGET)

# Build the project
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Run approach 1
run-approach1: $(TARGET)
	./$(TARGET) --approach1

# Run approach 2
run-approach2: $(TARGET)
	./$(TARGET) --approach2

# Run approach 3
run-approach3: $(TARGET)
	./$(TARGET) --approach3

# Set up the database
db:
	$(DB_SCRIPT)

# Clean build
clean:
	$(DB_SCRIPT) clean
	rm -f $(TARGET)

# Run everything (setup database, build, and run all approaches)
all: db all run-approach1 run-approach2 run-approach3

.PHONY: all clean db make-all run-approach1 run-approach2 run-approach3
