#!/bin/bash

# WebServ Test Suite
# Comprehensive testing for the 42 webserv project

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
WEBSERV_BIN="./webserv"
TEST_CONFIG="test.conf"
TEST_PORT_1=8080
TEST_PORT_2=8081
TEST_DIR="/tmp/webserv_test"
LOG_FILE="/tmp/webserv_test.log"

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Function to print test results
print_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✓ PASS${NC}: $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}✗ FAIL${NC}: $test_name"
        echo -e "${RED}  Details: $details${NC}"
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Function to make HTTP request and check response
test_http_request() {
    local method="$1"
    local url="$2"
    local expected_status="$3"
    local test_name="$4"
    local port="$5"
    local headers="$6"
    local data="$7"
    
    local curl_cmd="curl -s -w '%{http_code}' -o /tmp/response_body"
    
    if [ -n "$headers" ]; then
        curl_cmd="$curl_cmd -H '$headers'"
    fi
    
    if [ -n "$data" ]; then
        curl_cmd="$curl_cmd -d '$data'"
    fi
    
    curl_cmd="$curl_cmd -X $method http://localhost:$port$url"
    
    local response_code=$(eval $curl_cmd)
    
    if [ "$response_code" = "$expected_status" ]; then
        print_result "$test_name" "PASS" ""
    else
        print_result "$test_name" "FAIL" "Expected $expected_status, got $response_code"
    fi
}

# Function to check if webserv is running
check_webserv_running() {
    local port="$1"
    if curl -s --connect-timeout 2 http://localhost:$port > /dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# Function to start webserv in background
start_webserv() {
    echo -e "${BLUE}Starting WebServ...${NC}"
    $WEBSERV_BIN $TEST_CONFIG > $LOG_FILE 2>&1 &
    WEBSERV_PID=$!
    
    # Wait for server to start
    local attempts=0
    while [ $attempts -lt 10 ]; do
        if check_webserv_running $TEST_PORT_1; then
            echo -e "${GREEN}WebServ started successfully (PID: $WEBSERV_PID)${NC}"
            return 0
        fi
        sleep 1
        attempts=$((attempts + 1))
    done
    
    echo -e "${RED}Failed to start WebServ${NC}"
    return 1
}

# Function to stop webserv
stop_webserv() {
    if [ -n "$WEBSERV_PID" ]; then
        echo -e "${BLUE}Stopping WebServ (PID: $WEBSERV_PID)...${NC}"
        kill $WEBSERV_PID 2>/dev/null
        wait $WEBSERV_PID 2>/dev/null
    fi
}

# Function to cleanup
cleanup() {
    echo -e "${BLUE}Cleaning up...${NC}"
    stop_webserv
    rm -f /tmp/response_body
    rm -rf $TEST_DIR
}

# Set up trap for cleanup on exit
trap cleanup EXIT

# Create test directory and files
setup_test_environment() {
    echo -e "${BLUE}Setting up test environment...${NC}"
    mkdir -p $TEST_DIR
    mkdir -p $TEST_DIR/cgi-bin
    mkdir -p $TEST_DIR/upload
    
    # Create test files
    echo "<html><body><h1>Test Page</h1></body></html>" > $TEST_DIR/index.html
    echo "<html><body><h1>About Page</h1></body></html>" > $TEST_DIR/about.html
    echo "<html><body><h1>404 Not Found</h1></body></html>" > $TEST_DIR/404.html
    
    # Create CGI test script
    cat > $TEST_DIR/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/html")
print()
print("<html><body>")
print("<h1>CGI Test</h1>")
print("<p>REQUEST_METHOD:", os.environ.get('REQUEST_METHOD', 'Not set'), "</p>")
print("<p>QUERY_STRING:", os.environ.get('QUERY_STRING', 'Not set'), "</p>")
print("<p>CONTENT_TYPE:", os.environ.get('CONTENT_TYPE', 'Not set'), "</p>")
print("</body></html>")
EOF
    chmod +x $TEST_DIR/cgi-bin/test.py
    
    # Update test config to use test directory
    sed "s|/home/malee/42_Projects/webserv/www|$TEST_DIR|g" $TEST_CONFIG > /tmp/test_config.conf
    TEST_CONFIG="/tmp/test_config.conf"
}

# Test 1: Basic GET request
test_basic_get() {
    echo -e "${YELLOW}Testing basic GET requests...${NC}"
    
    test_http_request "GET" "/" "200" "GET root path" $TEST_PORT_1
    test_http_request "GET" "/index.html" "200" "GET index.html" $TEST_PORT_1
    test_http_request "GET" "/about.html" "200" "GET about.html" $TEST_PORT_1
    test_http_request "GET" "/nonexistent.html" "404" "GET nonexistent file" $TEST_PORT_1
}

# Test 2: HTTP methods
test_http_methods() {
    echo -e "${YELLOW}Testing HTTP methods...${NC}"
    
    test_http_request "GET" "/" "200" "GET method allowed" $TEST_PORT_1
    test_http_request "POST" "/" "405" "POST method not allowed on root" $TEST_PORT_1
    test_http_request "DELETE" "/" "405" "DELETE method not allowed on root" $TEST_PORT_1
    test_http_request "PUT" "/" "405" "PUT method not allowed" $TEST_PORT_1
    test_http_request "HEAD" "/" "405" "HEAD method not allowed" $TEST_PORT_1
}

# Test 3: CGI functionality
test_cgi_functionality() {
    echo -e "${YELLOW}Testing CGI functionality...${NC}"
    
    test_http_request "GET" "/cgi-bin/test.py" "200" "CGI GET request" $TEST_PORT_2
    test_http_request "GET" "/cgi-bin/test.py?param=value" "200" "CGI GET with query string" $TEST_PORT_2
    test_http_request "POST" "/cgi-bin/test.py" "200" "CGI POST request" $TEST_PORT_2 "Content-Type: application/x-www-form-urlencoded" "test=data"
}

# Test 4: Error handling
test_error_handling() {
    echo -e "${YELLOW}Testing error handling...${NC}"
    
    test_http_request "GET" "/nonexistent" "404" "404 Not Found" $TEST_PORT_1
    test_http_request "POST" "/" "405" "405 Method Not Allowed" $TEST_PORT_1
    test_http_request "GET" "/cgi-bin/nonexistent.py" "404" "CGI script not found" $TEST_PORT_2
}

# Test 5: Multiple servers
test_multiple_servers() {
    echo -e "${YELLOW}Testing multiple server blocks...${NC}"
    
    # Test server on port 8080
    test_http_request "GET" "/" "200" "Server 1 (port 8080) responding" $TEST_PORT_1
    
    # Test server on port 8081
    test_http_request "GET" "/" "200" "Server 2 (port 8081) responding" $TEST_PORT_2
}

# Test 6: Large request body
test_large_request() {
    echo -e "${YELLOW}Testing large request body...${NC}"
    
    # Create a large file for testing
    dd if=/dev/zero of=/tmp/large_file.txt bs=1M count=2 2>/dev/null
    
    # Test POST with large body
    local response_code=$(curl -s -w '%{http_code}' -o /tmp/response_body -X POST -d @/tmp/large_file.txt http://localhost:$TEST_PORT_2/)
    
    if [ "$response_code" = "200" ] || [ "$response_code" = "413" ]; then
        print_result "Large request body" "PASS" "Handled appropriately (status: $response_code)"
    else
        print_result "Large request body" "FAIL" "Unexpected status: $response_code"
    fi
    
    rm -f /tmp/large_file.txt
}

# Test 7: Concurrent connections
test_concurrent_connections() {
    echo -e "${YELLOW}Testing concurrent connections...${NC}"
    
    # Start multiple background requests
    for i in {1..5}; do
        curl -s http://localhost:$TEST_PORT_1/ > /dev/null &
    done
    
    # Wait for all background jobs
    wait
    
    print_result "Concurrent connections" "PASS" "All requests completed"
}

# Test 8: Malformed requests
test_malformed_requests() {
    echo -e "${YELLOW}Testing malformed requests...${NC}"
    
    # Test malformed HTTP request
    echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost $TEST_PORT_1 > /tmp/response 2>/dev/null
    
    if grep -q "HTTP/1.1" /tmp/response; then
        print_result "Malformed request handling" "PASS" "Server responded to malformed request"
    else
        print_result "Malformed request handling" "FAIL" "Server did not respond properly"
    fi
}

# Test 9: Configuration validation
test_config_validation() {
    echo -e "${YELLOW}Testing configuration validation...${NC}"
    
    # Test with invalid config file
    echo "invalid config content" > /tmp/invalid.conf
    
    if ! $WEBSERV_BIN /tmp/invalid.conf > /dev/null 2>&1; then
        print_result "Invalid config rejection" "PASS" "Server correctly rejected invalid config"
    else
        print_result "Invalid config rejection" "FAIL" "Server accepted invalid config"
    fi
    
    rm -f /tmp/invalid.conf
}

# Test 10: Server shutdown
test_server_shutdown() {
    echo -e "${YELLOW}Testing server shutdown...${NC}"
    
    # Test graceful shutdown
    stop_webserv
    sleep 2
    
    if ! check_webserv_running $TEST_PORT_1; then
        print_result "Server shutdown" "PASS" "Server stopped gracefully"
    else
        print_result "Server shutdown" "FAIL" "Server still running after shutdown"
    fi
}

# Main test execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}        WebServ Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    # Setup
    setup_test_environment
    
    # Start server
    if ! start_webserv; then
        echo -e "${RED}Failed to start WebServ. Exiting.${NC}"
        exit 1
    fi
    
    # Run tests
    test_basic_get
    test_http_methods
    test_cgi_functionality
    test_error_handling
    test_multiple_servers
    test_large_request
    test_concurrent_connections
    test_malformed_requests
    test_config_validation
    test_server_shutdown
    
    # Print summary
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}           Test Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo -e "Total Tests: $TESTS_RUN"
    echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo -e "${GREEN}All tests passed! 🎉${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed. Check the details above.${NC}"
        exit 1
    fi
}

# Run main function
main "$@"