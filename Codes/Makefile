# Compiler Settings
CC = gcc
CFLAGS = -w

# Source
SRC_SERVER = server.c
SRC_CLIENT = client.c

# Objects
OBJ_SERVER = server.o
OBJ_CLIENT = client.o

# Headers
HDR_SERVER = server.h
HDR_CLIENT = client.h

# Executables
TARGET_SERVER = server
TARGET_CLIENT = client

# Target
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Compile Server Object
$(OBJ_SERVER): $(SRC_SERVER) $(HDR_SERVER)
	$(CC) $(CFLAGS) -c $(SRC_SERVER)

# Compile Client Object
$(OBJ_CLIENT): $(SRC_CLIENT) $(HDR_CLIENT)
	$(CC) $(CFLAGS) -c $(SRC_CLIENT)

# Link Server Executable
$(TARGET_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJ_SERVER)

# Link Client Executable
$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJ_CLIENT)

# Clean
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT) $(OBJ_SERVER) $(OBJ_CLIENT)

# Phony
.PHONY: all clean