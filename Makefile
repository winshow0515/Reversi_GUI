CC = g++
CFLAGS = -std=c++11 -Wall `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0` -lpthread

TARGET = reversi_gtk
SERVER = server

all: $(TARGET) $(SERVER)

$(TARGET): gui.cpp game.hpp network.hpp
	$(CC) $(CFLAGS) gui.cpp -o $(TARGET) $(LIBS)

$(SERVER): server.cpp game.hpp
	$(CC) -std=c++11 -Wall server.cpp -o $(SERVER)

clean:
	rm -f $(TARGET) $(SERVER)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run