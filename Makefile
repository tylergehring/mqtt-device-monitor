# MQTT Device Monitor Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LIBS = -lmosquitto
SRCDIR = src
OBJDIR = build
TARGET = mqtt_test

# Create build directory if it doesn't exist
$(shell mkdir -p $(OBJDIR))

# Source files
SOURCES = $(SRCDIR)/mqtt_test.c
OBJECTS = $(OBJDIR)/mqtt_test.o

# Default target
all: $(TARGET)

# Build the main target
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $(TARGET)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Test the MQTT connection
test: $(TARGET)
	./$(TARGET)

.PHONY: all clean test