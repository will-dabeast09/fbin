# Makefile Template for MinGW

# Project Name
PROJECT_NAME = fbin

# Source Files
SRC = src/main.c

# Header Files Directory
INC_DIR = include

# Library Directory
LIB_DIR = lib
LIB_IMAGEQUANT = $(LIB_DIR)/libimagequant.a # Full path to the library

# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -g -I$(INC_DIR) -fopenmp

# Linker Flags
LDFLAGS = -L$(LIB_DIR) -limagequant -lm -fopenmp

# Output Executable Name
OUT_EXE = $(PROJECT_NAME).exe # Changed to .exe

# Object Files (automatically generated)
OBJ = $(SRC:.c=.o)

# Default target: build the executable
all: $(OUT_EXE)

# Rule to link object files to create the executable
$(OUT_EXE): $(OBJ)
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
