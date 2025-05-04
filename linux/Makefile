# Makefile Template for MinGW

# Project Name
PROJECT_NAME = fbin

# Source Files
SRC = src/main.c

# Header Files Directory
INC_DIR = include

# Library Directory
LIB_DIR = lib # Added library directory
LIB_IMAGEQUANT = $(LIB_DIR)/libimagequant.a # Full path to the library

# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -g -I$(INC_DIR) -fopenmp # Added -fopenmp to CFLAGS

# Linker Flags
LDFLAGS = -L$(LIB_DIR) -limagequant -lm -fopenmp # Added -lm and -fopenmp to LDFLAGS

# Output Executable Name
OUT_EXE = $(PROJECT_NAME) # Changed from .exe

# Object Files (automatically generated)
# The following line is modified to handle subdirectories
OBJ = $(SRC:.c=.o) # Change .c to .cpp if you are compiling C++

# Default target: build the executable
all: $(OUT_EXE)

# Rule to link object files to create the executable
$(OUT_EXE): $(OBJ) # Removed LIB_IMAGEQUANT from here
	$(CC) -o $(OUT_EXE) $(OBJ) $(LDFLAGS)
	@echo "Build complete: $(OUT_EXE)"

# Rule to compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiling: $<"

# Clean target: remove object files and the executable
clean:
	rm -f $(OBJ) $(OUT_EXE)
	@echo "Cleaning project"

# Phony targets (not actual files)
.PHONY: all clean

# Example with a library (assumes you have libmylib.a)
# LDFLAGS = -Llib -lmylib
#
# Example for C++
# CC = g++
# SRC = main.cpp
# OBJ = $(SRC:.cpp=.o)
