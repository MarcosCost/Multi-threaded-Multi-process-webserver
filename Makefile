# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -pthread -Isrc -D_XOPEN_SOURCE=700
LDFLAGS = -pthread
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O2

# Target executable
TARGET = httpserver

# --- Project Structure ---
SRCDIR = src
OBJDIR = obj

# --- File Discovery ---
# Find all .c files in the source directory
SRCS = $(wildcard $(SRCDIR)/*.c)

# Object files (in obj/ directory)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# --- Targets ---
# Default target
all: $(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Release build
release: CFLAGS += $(RELEASE_FLAGS)
release: all

# Link the target (executable in current directory)
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# Compile source files to object files in obj/ directory
# The | $(OBJDIR) is an "order-only prerequisite", ensuring the directory
# is created before compilation begins, but not triggering a recompile if
# the directory timestamp changes.
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the object directory
$(OBJDIR):
	mkdir -p $@

# --- Housekeeping ---
# Clean build files
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Clean and rebuild
distclean: clean
	rm -f *~ $(SRCDIR)/*~

# Phony targets
.PHONY: all debug release clean distclean