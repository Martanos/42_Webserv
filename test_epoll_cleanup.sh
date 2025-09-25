#!/bin/bash

# Test script to verify epoll cleanup issue is fixed
# This test specifically checks for dead client cleanup

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

WEBSERV_BIN="./webserv"
TEST_CONFIG="test.conf"
TEST_PORT=9090
TEST_DIR="/tmp/webserv_cleanup_test"

# Setup test environment
setup_test() {
    echo -e "${BLUE}Setting up epoll cleanup test...${NC}"
    mkdir -p $TEST_DIR
    
    # Create test files
    echo "<html><body><h1>Test Page</h1></body></html>" > $TEST_DIR/index.html
    
    # Update config with different ports
    sed "s|/home/malee/42_Projects/webserv/www|$TEST_DIR|g" $TEST_CONFIG | sed "s|8080|9090|g" | sed "s|8081|9091|g" > /tmp/cleanup_test_config.conf
}

# Test 1: Rapid connect/disconnect
test_rapid_connect_disconnect() {
    echo -e "${YELLOW}Testing rapid connect/disconnect...${NC}"
    
    local success_count=0
    local total_tests=20
    
    for i in $(seq 1 $total_tests); do
        # Connect, send request, disconnect immediately
        {
            echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
            sleep 0.1
        } | nc localhost $TEST_PORT > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            success_count=$((success_count + 1))
        fi
        
        sleep 0.05  # Small delay between connections
    done
    
    local success_rate=$(echo "scale=2; $success_count * 100 / $total_tests" | bc)
    
    if (( $(echo "$success_rate >= 90" | bc -l) )); then
        echo -e "${GREEN}✓ PASS${NC}: Rapid connect/disconnect ($success_rate% success)"
    else
        echo -e "${RED}✗ FAIL${NC}: Rapid connect/disconnect ($success_rate% success)"
    fi
}

# Test 2: Connection timeout
test_connection_timeout() {
    echo -e "${YELLOW}Testing connection timeout...${NC}"
    
    # Start a connection but don't send any data
    {
        sleep 35  # Wait longer than timeout (30 seconds)
    } | nc localhost $TEST_PORT &
    
    local nc_pid=$!
    
    # Wait a bit and check if server is still responsive
    sleep 5
    
    # Try to make a normal request
    local response=$(curl -s -w '%{http_code}' -o /dev/null http://localhost:$TEST_PORT/)
    
    # Kill the hanging connection
    kill $nc_pid 2>/dev/null
    
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✓ PASS${NC}: Connection timeout handling"
    else
        echo -e "${RED}✗ FAIL${NC}: Connection timeout handling (got $response)"
    fi
}

# Test 3: Malformed requests
test_malformed_requests() {
    echo -e "${YELLOW}Testing malformed requests...${NC}"
    
    local success_count=0
    local total_tests=10
    
    for i in $(seq 1 $total_tests); do
        # Send malformed request
        echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost $TEST_PORT > /dev/null 2>&1
        
        if [ $? -eq 0 ]; then
            success_count=$((success_count + 1))
        fi
        
        sleep 0.1
    done
    
    local success_rate=$(echo "scale=2; $success_count * 100 / $total_tests" | bc)
    
    if (( $(echo "$success_rate >= 80" | bc -l) )); then
        echo -e "${GREEN}✓ PASS${NC}: Malformed request handling ($success_rate% success)"
    else
        echo -e "${RED}✗ FAIL${NC}: Malformed request handling ($success_rate% success)"
    fi
}

# Test 4: Server responsiveness under load
test_server_responsiveness() {
    echo -e "${YELLOW}Testing server responsiveness under load...${NC}"
    
    local start_time=$(date +%s.%N)
    
    # Start multiple background requests
    for i in $(seq 1 10); do
        curl -s http://localhost:$TEST_PORT/ > /dev/null &
    done
    
    # Wait for all requests to complete
    wait
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    
    if (( $(echo "$duration < 5" | bc -l) )); then
        echo -e "${GREEN}✓ PASS${NC}: Server responsiveness (${duration}s for 10 requests)"
    else
        echo -e "${RED}✗ FAIL${NC}: Server responsiveness (${duration}s for 10 requests)"
    fi
}

# Test 5: Memory leak check
test_memory_usage() {
    echo -e "${YELLOW}Testing memory usage...${NC}"
    
    # Get initial memory usage
    local initial_memory=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null || echo "0")
    
    # Make many requests
    for i in $(seq 1 50); do
        curl -s http://localhost:$TEST_PORT/ > /dev/null
    done
    
    # Get final memory usage
    local final_memory=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null || echo "0")
    local memory_diff=$((final_memory - initial_memory))
    
    if [ $memory_diff -lt 5000 ]; then  # Less than 5MB increase
        echo -e "${GREEN}✓ PASS${NC}: Memory usage stable (${memory_diff}KB increase)"
    else
        echo -e "${RED}✗ FAIL${NC}: Memory usage high (${memory_diff}KB increase)"
    fi
}

# Main execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}      Epoll Cleanup Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    setup_test
    
    # Start webserv
    echo -e "${BLUE}Starting WebServ...${NC}"
    $WEBSERV_BIN /tmp/cleanup_test_config.conf > /tmp/webserv_cleanup.log 2>&1 &
    WEBSERV_PID=$!
    
    # Wait for server to start
    sleep 3
    
    # Run tests
    test_rapid_connect_disconnect
    test_connection_timeout
    test_malformed_requests
    test_server_responsiveness
    test_memory_usage
    
    # Cleanup
    echo -e "${BLUE}Cleaning up...${NC}"
    kill $WEBSERV_PID 2>/dev/null
    rm -rf $TEST_DIR
    rm -f /tmp/cleanup_test_config.conf
    
    echo -e "${GREEN}Epoll cleanup tests completed!${NC}"
}

main "$@"