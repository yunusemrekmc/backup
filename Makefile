# Compiler settings
CC      := gcc
CFLAGS  := -ggdb -O2 -march=native -pipe -Iinclude -std=c99 -pedantic -Wall \
-Wpedantic -Wextra -Wshadow -Wpointer-arith -Wcast-qual \
-Wstrict-prototypes -Wmissing-prototypes -Werror
LDFLAGS := 

# Directories
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Target
TARGET  := $(BIN_DIR)/backup

# Source and object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Default rule
all: $(TARGET)

# Link step
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Built $@"

# Compilation rule for .c -> .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

tags:
	etags $(SRCS)

.PHONY: all clean
