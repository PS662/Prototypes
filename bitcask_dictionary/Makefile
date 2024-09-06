CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

TARGET = bitcask_dictionary

SRC = bitcask_dictionary.cpp

OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
	rm -f *.bitcask
	rm -f *.config

.PHONY: all clean
