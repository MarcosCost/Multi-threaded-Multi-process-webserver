#!/bin/bash
# test_functional.sh - Versão Verbose para Relatório

SERVER_URL="http://localhost:8080"
LOG_FILE="test_functional.log"
DOCUMENT_ROOT="www/"
DUMMY_JS_FILE="$DOCUMENT_ROOT/test_js_func.js"
DUMMY_CSS_FILE="$DOCUMENT_ROOT/test_css_func.css"
DUMMY_PDF_FILE="$DOCUMENT_ROOT/test_pdf_func.pdf"

# Limpa o log antigo e cria um novo
echo "--- Starting Functional Tests ---" | tee $LOG_FILE

# Funções de ajuda alteradas para usar 'tee -a' (append)
check_status() {
    local url="$1"
    local expected_status="$2"
    local method="${3:-GET}"

    RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" -X $method "$url")
    if [ "$RESPONSE" == "$expected_status" ]; then
        echo -e "\033[32m[PASS]\033[0m $method $url -> Status $RESPONSE" | tee -a $LOG_FILE
    else
        echo -e "\033[31m[FAIL]\033[0m $method $url -> Expected $expected_status, Got $RESPONSE" | tee -a $LOG_FILE
        FAILED=1
    fi
}

check_content_type() {
    local url="$1"
    local expected_type="$2"

    CONTENT_TYPE=$(curl -s -I "$url" | grep -i "Content-Type:" | awk '{print $2}' | tr -d '\r')
    if [[ "$CONTENT_TYPE" == *"$expected_type"* ]]; then
        echo -e "\033[32m[PASS]\033[0m $url -> Content-Type: $CONTENT_TYPE" | tee -a $LOG_FILE
    else
        echo -e "\033[31m[FAIL]\033[0m $url -> Expected Type: $expected_type, Got: $CONTENT_TYPE" | tee -a $LOG_FILE
        FAILED=1
    fi
}

# Setup dos ficheiros dummy
echo "console.log('test');" > $DUMMY_JS_FILE
echo "body{color:red;}" > $DUMMY_CSS_FILE
head -c 10K /dev/zero > $DUMMY_PDF_FILE
FAILED=0

# --- Execução dos Testes ---
check_status "$SERVER_URL/index.html" 200
check_content_type "$SERVER_URL/index.html" "text/html"

check_status "$SERVER_URL/index.html" 200 HEAD

check_status "$SERVER_URL/" 200

check_status "$SERVER_URL/test_css_func.css" 200
check_content_type "$SERVER_URL/test_css_func.css" "text/css"

check_status "$SERVER_URL/test_js_func.js" 200
check_content_type "$SERVER_URL/test_js_func.js" "application/javascript"

check_status "$SERVER_URL/test_pdf_func.pdf" 200
check_content_type "$SERVER_URL/test_pdf_func.pdf" "application/pdf"

check_status "$SERVER_URL/nonexistent_file.xyz" 404

# Cleanup
rm $DUMMY_JS_FILE $DUMMY_CSS_FILE $DUMMY_PDF_FILE
echo "--- Test Summary ---" | tee -a $LOG_FILE

if [ $FAILED -eq 0 ]; then
    echo -e "\033[32mALL TESTS PASSED\033[0m" | tee -a $LOG_FILE
    exit 0
else
    echo -e "\033[31mSOME TESTS FAILED\033[0m. Check $LOG_FILE." | tee -a $LOG_FILE
    exit 1
fi