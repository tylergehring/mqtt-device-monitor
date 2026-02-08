# MQTT Device Monitor Makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LIBS = -lmosquitto
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Targets
all: $(BINDIR)/mqtt_test $(BINDIR)/device_agent

# Build mqtt_test
$(BINDIR)/mqtt_test: $(OBJDIR)/mqtt_test.o
	$(CC) $^ -o $@ $(LIBS)

# Build device_agent
$(BINDIR)/device_agent: $(OBJDIR)/device_agent.o $(OBJDIR)/mqtt_util.o $(OBJDIR)/metrics.o
	$(CC) $^ -o $@ $(LIBS)

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build files
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Test the MQTT connection
test: $(BINDIR)/mqtt_test
	./$(BINDIR)/mqtt_test

# Run device agent
device: $(BINDIR)/device_agent
	./$(BINDIR)/device_agent

.PHONY: all clean test device