# ==============================
#  Compiler and basic settings
# ==============================
CC       ?= cc
PREFIX   ?= /usr/local
INCLUDES := -Iinclude

# ==============================
#  Base flags (safe defaults)
# ==============================
BASE_CFLAGS := -std=c99 -pedantic -Wall -Werror -Wpedantic -Wextra \
               -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes \
               -Wmissing-prototypes -fno-exceptions -fno-unwind-tables \
               -fno-asynchronous-unwind-tables -fstack-protector-strong

# ==============================
#  Optimization and portability
# ==============================
# Optimizing the binary, and the build process
OPTFLAGS  := -O2 -march=native -pipe
# For debugging purposes only
DEBUGFLAGS := -ggdb2 -Og -fsanitize=address
# To statically build the program, not recommended with glibc
STATICFLAGS := -static
# musl libc flags: I prefer statically linking when using musl
# but this definitely isn't enforced, do what you want
MUSLFLAGS := -static -fno-stack-protector -U_FORTIFY_SOURCE

# ==============================
#  Build modes
# ==============================
# You can invoke these:
#   make MODE=debug
#   make MODE=static
#   make MODE=musl
# or just plain `make` for default dynamic build.

ifeq ($(MODE),debug)
  CFLAGS  := $(BASE_CFLAGS) $(DEBUGFLAGS) $(INCLUDES)
  LDFLAGS := -fsanitize=address
  BUILD   := build/debug
else ifeq ($(MODE),static)
  CFLAGS  := $(BASE_CFLAGS) $(OPTFLAGS) $(STATICFLAGS) $(INCLUDES)
  LDFLAGS :=
  BUILD   := build/static
else ifeq ($(MODE),musl)
  CC      := musl-gcc
  CFLAGS  := $(BASE_CFLAGS) $(OPTFLAGS) $(MUSLFLAGS) $(INCLUDES)
  LDFLAGS :=
  BUILD   := build/musl
else
  # Default dynamic build
  CFLAGS  := $(BASE_CFLAGS) $(OPTFLAGS) $(INCLUDES)
  LDFLAGS :=
  BUILD   := build/normal
endif

# Directories
SRC_DIR := src
OBJ_DIR := $(BUILD)
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

etags:
	ctags --links=no -e -f TAGS -R .

ctags:
	ctags --links=no -f tags -R .

.PHONY: all clean
