#!/bin/bash
# test_load.sh - Carga e Durabilidade (Req 13, 14, 21, 22)

SERVER_URL_SMALL="http://localhost:8080/index.html"
SERVER_URL_LARGE="http://localhost:8080/large_file.bin"
DOCUMENT_ROOT="www/"
LOG_FILE_LOAD="test_load_ab.log"
LOG_FILE_DURATION="test_duration_5min.log"

REQUESTS=10000
CONCURRENCY=100
DURATION=300

run_ab_test() {
    local n_requests="$1"
    local concurrency="$2"
    local duration="$3"
    local target_url="$4"
    local log_file="$5"

    local AB_CMD="ab -q -c $concurrency " # Adicionado -q (quiet)

    if [ "$duration" -gt 0 ]; then
        # Como o servidor não consegue processar 50M em 5 min, o teste vai correr até o tempo (-t) acabar.
        AB_CMD+="-t $duration -n 50000000 "
    else
        AB_CMD+="-n $n_requests "
    fi

    echo "Running AB test: $AB_CMD $target_url"
    $AB_CMD $target_url | tee $log_file
}

check_summary() {
    local log_file="$1"
    local test_name="$2"

    DROPPED_CONNECTIONS=$(grep "Non-OK responses:" $log_file | awk '{print $3}')
    TOTAL_REQUESTS=$(grep "Complete requests:" $log_file | awk '{print $3}')
    TIME_TAKEN=$(grep "Time taken for tests:" $log_file | awk '{print $5}')

    echo "--- $test_name SUMMARY ---"
    echo "Time: $TIME_TAKEN s | Requests: $TOTAL_REQUESTS"

    if [ -z "$DROPPED_CONNECTIONS" ] || [ "$DROPPED_CONNECTIONS" -eq 0 ]; then
        return 0
    else
        echo "[FAIL] Non-OK responses: $DROPPED_CONNECTIONS"
        return 1
    fi
}

if [ ! -f $DOCUMENT_ROOT/large_file.bin ]; then
    echo "Creating large_file.bin..."
    head -c 1500K /dev/urandom > $DOCUMENT_ROOT/large_file.bin
fi

# Teste 1: Carga (10k requests)
run_ab_test $REQUESTS $CONCURRENCY 0 $SERVER_URL_SMALL $LOG_FILE_LOAD
RESULT1=$?
check_summary $LOG_FILE_LOAD "Load Test"

# Teste 2: Durabilidade (5 min / I/O Bound)
echo "--- Starting 5-Minute Extreme Durability Test ---"
run_ab_test 0 200 $DURATION $SERVER_URL_LARGE $LOG_FILE_DURATION
RESULT2=$?
check_summary $LOG_FILE_DURATION "Durability Test"

exit $((RESULT1 + RESULT2))