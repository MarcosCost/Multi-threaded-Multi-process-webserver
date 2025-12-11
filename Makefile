# Compilador e Flags
CC = gcc
# Flags para garantir POSIX 2008 e threads
CFLAGS = -Wall -Wextra -std=c99 -pedantic -pthread -Isrc -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L
# Flags de Otimização (Para a entrega final)
RELEASE_FLAGS = -O3

# Bibliotecas necessárias
LIBS = -pthread -lrt

# Nomes dos executáveis
TARGET = server
TEST_CLIENT = tests/test_client

# --- Estrutura do Projeto ---
SRCDIR = src
OBJDIR = obj
LOGDIR = logs
DOCUMENT_ROOT = www

# --- Deteção de Ficheiros ---
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
TEST_CLIENT_SRC = tests/test_concurrent.c

# --- Targets Principais ---

all: $(TARGET) $(TEST_CLIENT)

# Target de Debug (inclui símbolos e define DEBUG)
debug: CFLAGS += -g -DDEBUG
debug: all

# Target de Release (Otimizado)
release: CFLAGS += $(RELEASE_FLAGS)
release: all

$(TARGET): $(OBJS)
	@echo "A linkar $@"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compilação do Cliente de Teste
$(TEST_CLIENT): $(TEST_CLIENT_SRC)
	@echo "A compilar Cliente de Teste: $@"
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)


# --- Gestão de Objetos ---
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@echo "A compilar $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $@

# --- Targets de Execução e Teste ---

run: clean all
	@echo "A iniciar Servidor (Ctrl+C para parar)..."
	@mkdir -p $(LOGDIR)
	./$(TARGET)

test: all
	@echo "A iniciar Suite de Testes..."
	@mkdir -p $(LOGDIR)

	# 1. Testes Funcionais (Req 9-12)
	@echo "--- 1. Testes Funcionais ---"
	./tests/test_functional.sh
	@mv access.log $(LOGDIR)/functional_access.log 2>/dev/null || true
	@mv test_functional.log $(LOGDIR)/functional_report.log 2>/dev/null || true

	# 2. Testes de Carga e Durabilidade (Req 13, 21)
	@echo "--- 2. Testes de Carga e Durabilidade ---"
	./tests/test_load.sh
	@mv access.log $(LOGDIR)/load_access.log 2>/dev/null || true
	@mv test_load_ab.log $(LOGDIR)/load_ab_report.log 2>/dev/null || true

	# 3. Testes de Concorrência Pura (Req 15, 20)
	@echo "--- 3. Teste de Cliente Concorrente ---"
	./$(TEST_CLIENT)
	@mv access.log $(LOGDIR)/concurrent_access.log 2>/dev/null || true

	@echo "Testes finalizados. Logs disponíveis em $(LOGDIR)/"


# --- Targets de Análise e Debugging (Requisitos 17, 22) ---

# Requisito 22: Fugas de Memória (Valgrind)
valgrind: clean debug
	@echo "A iniciar Valgrind (Verificação de Fugas de Memória)"
	@echo "Ação: Execute os testes de carga manualmente noutro terminal."
	valgrind --leak-check=full \
	         --show-leak-kinds=all \
	         --track-origins=yes \
	         --log-file=$(LOGDIR)/valgrind_output.log \
	         ./$(TARGET)

# Requisito 17: Race Conditions (Helgrind)
helgrind: clean debug
	@echo "A iniciar Helgrind (Verificação de Race Conditions)"
	@echo "Ação: Execute os testes de concorrência manualmente noutro terminal."
	valgrind --tool=helgrind \
	         --log-file=$(LOGDIR)/helgrind_output.log \
	         ./$(TARGET)


# --- Limpeza ---
clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_CLIENT) *.log $(LOGDIR) $(DOCUMENT_ROOT)/large_file.bin 2>/dev/null || true
	@echo "Limpeza concluída."

.PHONY: all debug release clean run test valgrind helgrind