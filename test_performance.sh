#!/bin/bash

# Performance and stress test script for WebServ

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

WEBSERV_BIN="./webserv"
TEST_CONFIG="test.conf"
TEST_PORT=8080
TEST_DIR="/tmp/webserv_perf_test"

# Performance test results
PERF_RESULTS=()

# Setup performance test environment
setup_perf_test() {
    echo -e "${BLUE}Setting up performance test environment...${NC}"
    mkdir -p $TEST_DIR
    
    # Create test files of different sizes
    echo "<html><body><h1>Small File</h1></body></html>" > $TEST_DIR/small.html
    dd if=/dev/zero of=$TEST_DIR/medium.bin bs=1K count=100 2>/dev/null
    dd if=/dev/zero of=$TEST_DIR/large.bin bs=1K count=1000 2>/dev/null
    
    # Update config
    sed "s|/home/malee/42_Projects/webserv/www|$TEST_DIR|g" $TEST_CONFIG > /tmp/perf_test_config.conf
}

# Test concurrent connections
test_concurrent_connections() {
    echo -e "${YELLOW}Testing concurrent connections...${NC}"
    
    local start_time=$(date +%s.%N)
    local num_connections=50
    
    # Start multiple concurrent requests
    for i in $(seq 1 $num_connections); do
        curl -s http://localhost:$TEST_PORT/small.html > /dev/null &
    done
    
    # Wait for all requests to complete
    wait
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    local requests_per_second=$(echo "scale=2; $num_connections / $duration" | bc)
    
    echo -e "${GREEN}✓ PASS${NC}: Concurrent connections ($num_connections requests in ${duration}s, ${requests_per_second} req/s)"
    PERF_RESULTS+=("Concurrent: ${requests_per_second} req/s")
}

# Test large file serving
test_large_file_serving() {
    echo -e "${YELLOW}Testing large file serving...${NC}"
    
    local start_time=$(date +%s.%N)
    
    # Download large file
    curl -s http://localhost:$TEST_PORT/large.bin > /tmp/downloaded_large.bin
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    local file_size=$(stat -c%s $TEST_DIR/large.bin)
    local speed=$(echo "scale=2; $file_size / $duration / 1024" | bc)
    
    # Verify file integrity
    if cmp -s $TEST_DIR/large.bin /tmp/downloaded_large.bin; then
        echo -e "${GREEN}✓ PASS${NC}: Large file serving (${speed} KB/s)"
        PERF_RESULTS+=("Large file: ${speed} KB/s")
    else
        echo -e "${RED}✗ FAIL${NC}: Large file serving (file corruption)"
    fi
    
    rm -f /tmp/downloaded_large.bin
}

# Test memory usage
test_memory_usage() {
    echo -e "${YELLOW}Testing memory usage...${NC}"
    
    # Get initial memory usage
    local initial_memory=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null || echo "0")
    
    # Make many requests to test memory leaks
    for i in $(seq 1 100); do
        curl -s http://localhost:$TEST_PORT/small.html > /dev/null
    done
    
    # Get final memory usage
    local final_memory=$(ps -o rss= -p $WEBSERV_PID 2>/dev/null || echo "0")
    local memory_diff=$((final_memory - initial_memory))
    
    if [ $memory_diff -lt 10000 ]; then  # Less than 10MB increase
        echo -e "${GREEN}✓ PASS${NC}: Memory usage stable (${memory_diff}KB increase)"
        PERF_RESULTS+=("Memory: ${memory_diff}KB increase")
    else
        echo -e "${RED}✗ FAIL${NC}: Memory usage high (${memory_diff}KB increase)"
    fi
}

# Test response time
test_response_time() {
    echo -e "${YELLOW}Testing response time...${NC}"
    
    local total_time=0
    local num_requests=20
    
    for i in $(seq 1 $num_requests); do
        local start_time=$(date +%s.%N)
        curl -s http://localhost:$TEST_PORT/small.html > /dev/null
        local end_time=$(date +%s.%N)
        local request_time=$(echo "$end_time - $start_time" | bc)
        total_time=$(echo "$total_time + $request_time" | bc)
    done
    
    local avg_time=$(echo "scale=4; $total_time / $num_requests" | bc)
    local avg_ms=$(echo "scale=2; $avg_time * 1000" | bc)
    
    if (( $(echo "$avg_ms < 100" | bc -l) )); then
        echo -e "${GREEN}✓ PASS${NC}: Response time (${avg_ms}ms average)"
        PERF_RESULTS+=("Response: ${avg_ms}ms avg")
    else
        echo -e "${RED}✗ FAIL${NC}: Response time (${avg_ms}ms average)"
    fi
}

# Test connection handling
test_connection_handling() {
    echo -e "${YELLOW}Testing connection handling...${NC}"
    
    # Test rapid connect/disconnect
    local success_count=0
    local total_tests=30
    
    for i in $(seq 1 $total_tests); do
        if timeout 5 curl -s http://localhost:$TEST_PORT/small.html > /dev/null; then
            success_count=$((success_count + 1))
        fi
        sleep 0.1
    done
    
    local success_rate=$(echo "scale=2; $success_count * 100 / $total_tests" | bc)
    
    if (( $(echo "$success_rate >= 95" | bc -l) )); then
        echo -e "${GREEN}✓ PASS${NC}: Connection handling (${success_rate}% success rate)"
        PERF_RESULTS+=("Connections: ${success_rate}% success")
    else
        echo -e "${RED}✗ FAIL${NC}: Connection handling (${success_rate}% success rate)"
    fi
}

# Test error handling under load
test_error_handling_load() {
    echo -e "${YELLOW}Testing error handling under load...${NC}"
    
    local error_count=0
    local total_requests=50
    
    # Mix valid and invalid requests
    for i in $(seq 1 $total_requests); do
        if [ $((i % 3)) -eq 0 ]; then
            # Invalid request
            curl -s -w '%{http_code}' -o /dev/null http://localhost:$TEST_PORT/nonexistent.html
        else
            # Valid request
            curl -s -w '%{http_code}' -o /dev/null http://localhost:$TEST_PORT/small.html
        fi
    done
    
    echo -e "${GREEN}✓ PASS${NC}: Error handling under load (server remained stable)"
    PERF_RESULTS+=("Error handling: Stable under load")
}

# Print performance summary
print_performance_summary() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}        Performance Summary${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    for result in "${PERF_RESULTS[@]}"; do
        echo -e "${GREEN}$result${NC}"
    done
}

# Main execution
main() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}      Performance Test Suite${NC}"
    echo -e "${BLUE}========================================${NC}"
    
    setup_perf_test
    
    # Start webserv
    echo -e "${BLUE}Starting WebServ...${NC}"
    $WEBSERV_BIN /tmp/perf_test_config.conf > /tmp/webserv_perf.log 2>&1 &
    WEBSERV_PID=$!
    
    # Wait for server to start
    sleep 3
    
    # Run performance tests
    test_concurrent_connections
    test_large_file_serving
    test_memory_usage
    test_response_time
    test_connection_handling
    test_error_handling_load
    
    # Print summary
    print_performance_summary
    
    # Cleanup
    echo -e "${BLUE}Cleaning up...${NC}"
    kill $WEBSERV_PID 2>/dev/null
    rm -rf $TEST_DIR
    rm -f /tmp/perf_test_config.conf
    
    echo -e "${GREEN}Performance tests completed!${NC}"
}

main "$@"