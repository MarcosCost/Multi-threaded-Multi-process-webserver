#!/bin/bash
# tests/test_load.sh

# Cores para output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SERVER_URL="http://localhost:8080/index.html"

echo -e "${YELLOW}=== INICIANDO TESTES DE CARGA DO WEBSERVER ===${NC}"
echo "Certifica-te que o servidor está a correr (./server) noutro terminal!"
echo "A aguardar 2 segundos..."
sleep 2

# Função para executar teste
run_test() {
    local requests=$1
    local concurrency=$2
    local label=$3

    echo -e "\n${YELLOW}--- TESTE: $label ($requests pedidos, $concurrency concorrentes) ---${NC}"

    # Executa o Apache Bench
    # -n: Total pedidos
    # -c: Concorrência
    # -k: Keep-Alive (opcional, o nosso servidor suporta se implementado, senão ignora)
    # -r: Não sair em erro de socket (importante para stress tests)
    ab -n $requests -c $concurrency -r $SERVER_URL > ab_temp.log 2>&1

    # Analisa resultados
    if grep -q "Complete requests:" ab_temp.log; then
        local complete=$(grep "Complete requests:" ab_temp.log | awk '{print $3}')
        local failed=$(grep "Failed requests:" ab_temp.log | awk '{print $3}')
        local rps=$(grep "Requests per second:" ab_temp.log | awk '{print $4}')

        echo -e "Status: ${GREEN}SUCESSO${NC}"
        echo "  - Pedidos Completos: $complete"
        echo "  - Falhas: $failed"
        echo "  - Requests/segundo: $rps"

        if [ "$failed" -gt 0 ]; then
            echo -e "${RED}  AVISO: Houve pedidos falhados! (Verifica logs: Queue cheia?)${NC}"
        fi
    else
        echo -e "Status: ${RED}ERRO CRÍTICO (O servidor caiu?)${NC}"
        cat ab_temp.log
    fi
    rm ab_temp.log
}

# 1. Aquecimento (Warm-up)
run_test 100 1 "Aquecimento (Cache Population)"

# 2. Carga Leve
run_test 1000 10 "Carga Leve (10 threads)"

# 3. Carga Média
run_test 5000 50 "Carga Média (50 threads)"

# 4. Teste de Stress (Requisito do Projeto: 10.000 pedidos)
# Nota: Se o MAX_QUEUE_SIZE for 100, concorrência de 100 pode gerar 503s. É esperado.
run_test 10000 100 "STRESS TEST (100 threads)"

echo -e "\n${YELLOW}=== FIM DOS TESTES ===${NC}"