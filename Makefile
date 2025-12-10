# Compiler and flags
CC = gcc
# Adicionado -D_POSIX_C_SOURCE=200809L para garantir acesso a strdup, sigaction, etc.
CFLAGS = -Wall -Wextra -std=c99 -pedantic -pthread -Isrc -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3

# Bibliotecas necessárias (ordem importa no linker)
LIBS = -pthread -lrt

# Target executable
TARGET = server

# --- Project Structure ---
# Assume que os ficheiros .c estão na raiz ou em src/?
# Com base nos teus uploads, parece que tens tudo na mesma pasta.
# Se tiveres em 'src/', mantém SRCDIR=src. Se estiver na raiz, SRCDIR=.
SRCDIR = src
OBJDIR = obj

# --- File Discovery ---
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

# --- Targets ---
all: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

release: CFLAGS += $(RELEASE_FLAGS)
release: all

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all debug release clean