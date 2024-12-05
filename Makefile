# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wformat

# Source files
SOURCES = server.c server_client.c list.c
HEADERS = server.h list.h
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = server

# Default rule
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -lpthread -o $(TARGET)

# Compile each .c file into a .o file
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJECTS) $(TARGET)
