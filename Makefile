CC = g++
CFLAGS = -Wall -I/usr/include -L/usr/lib
LDFLAGS = -lraylib -lGL -lm -lpthread
TARGET = logic_gates
SRC = src/main.cpp src/Simulator.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) src/*.o

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run