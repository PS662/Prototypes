CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

TARGET = bitcask_dictionary

SRC = bitcask_dictionary.cpp

all: $(TARGET)

$(TARGET):
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	rm -f *.bitcask
	rm -f *.config

.PHONY: all clean
